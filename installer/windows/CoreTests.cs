using System;
using System.IO;
using System.IO.Compression;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Reflection.Metadata;
using System.Reflection.PortableExecutable;
using BalatROM.Setup;
class CoreTests {
 static int passed;static string home;static void Check(bool value,string message){if(!value)throw new Exception(message);}
 static void Pass(string name){Console.WriteLine("PASS "+name);passed++;}
 static byte[] Zip(Dictionary<string,byte[]> files){using(var memory=new MemoryStream()){using(var zip=new ZipArchive(memory,ZipArchiveMode.Create,true)){foreach(var row in files){using(var stream=zip.CreateEntry(row.Key).Open())stream.Write(row.Value,0,row.Value.Length);}}return memory.ToArray();}}
 static Tuple<Config,byte[],byte[],byte[]> Fixture(){
  byte[] exe=System.Text.Encoding.UTF8.GetBytes("DECLARED NONEXECUTED FIXTURE");string json="{\"files\":[{\"path\":\"BalatROM.exe\",\"bytes\":"+exe.Length+",\"sha256\":\""+Engine.Hash(exe)+"\"}]}";byte[] mf=System.Text.Encoding.UTF8.GetBytes(json);
  byte[] payload=Zip(new Dictionary<string,byte[]>{{"PACKAGE-MANIFEST.json",mf}});byte[] vendor=Zip(new Dictionary<string,byte[]>{{"electron.exe",exe}});
  var config=new Config{Version="0.3.5-rc.2",AppFolder="declared-fixture",PayloadSha256=Engine.Hash(payload),PackageManifestSha256=Engine.Hash(mf),Components=new[]{new Component{Name="electron",Destination="",Sha256=Engine.Hash(vendor),Bytes=vendor.Length,Url="https://github.com/declared-fixture.zip"}}};return Tuple.Create(config,payload,mf,vendor);
 }
 static async Task Fails(Func<Task> action,string name){try{await action();}catch(Exception){Pass(name);return;}throw new Exception("Expected rejection: "+name);}
 static Action<ProgressInfo> progress=info=>{};
 static async Task Run(string[] args){
  home=Path.GetFullPath(args[0]);var fixture=Fixture();var c=fixture.Item1;byte[] p=fixture.Item2,m=fixture.Item3,v=fixture.Item4;
  Func<Component,string,CancellationToken,Action<ProgressInfo>,Task> source=(component,file,token,report)=>{token.ThrowIfCancellationRequested();File.WriteAllBytes(file,v);return Task.CompletedTask;};
  string basic=Path.Combine(home,"QA fixture with spaces");string first=await Engine.Install(basic,c,p,m,CancellationToken.None,progress,source);Check(File.Exists(first),"Fixture not installed");Pass("fixture installs in a path with spaces");
  string again=await Engine.Install(basic,c,p,m,CancellationToken.None,progress,(a,b,d,e)=>throw new Exception("Unexpected download"));Check(first==again,"Different cached path");Pass("verified installed app reopens without downloader");
  byte[] bad=(byte[])p.Clone();bad[0]^=1;await Fails(()=>Engine.Install(Path.Combine(home,"bad-payload"),c,bad,m,CancellationToken.None,progress,source),"damaged embedded payload refused");
  await Fails(()=>Engine.Install(Path.Combine(home,"offline-first"),c,p,m,CancellationToken.None,progress,(a,b,d,e)=>throw new IOException("declared offline fixture")),"offline first setup returns actionable failure, no app published");
  string canceled=Path.Combine(home,"canceled");using(var stop=new CancellationTokenSource())await Fails(()=>Engine.Install(canceled,c,p,m,stop.Token,progress,(a,b,d,e)=>{File.WriteAllBytes(b,new byte[]{0});stop.Cancel();d.ThrowIfCancellationRequested();return Task.CompletedTask;}),"cancellation before publish refuses partial app");
  Check(!Directory.Exists(Path.Combine(canceled,"apps")),"Canceled app published");Check(!Directory.GetFiles(canceled,"*.pending-*",SearchOption.AllDirectories).Any(),"Partial download remains");Pass("owned partial files cleaned, no final app on cancellation");
  await Fails(()=>Engine.Install(Path.Combine(home,"bad-download"),c,p,m,CancellationToken.None,progress,(a,b,d,e)=>{File.WriteAllBytes(b,new byte[]{1});return Task.CompletedTask;}),"wrong component SHA refused before extraction");
  string repair=Path.Combine(home,"repair");Directory.CreateDirectory(Path.Combine(repair,"downloads"));File.WriteAllBytes(Path.Combine(repair,"downloads",c.Components[0].Sha256+".zip"),new byte[]{3});await Engine.Install(repair,c,p,m,CancellationToken.None,progress,source);Check(Directory.GetFiles(Path.Combine(repair,"downloads"),"*.preserved-*").Length==1,"Invalid cache not preserved");Pass("tampered cache preserved and replaced by verified component");
  string old=Path.Combine(home,"previous");string oldApp=Path.Combine(old,"apps",c.AppFolder);Directory.CreateDirectory(oldApp);File.WriteAllText(Path.Combine(oldApp,"keep-user-file.txt"),"preserve");await Engine.Install(old,c,p,m,CancellationToken.None,progress,source);Check(Directory.GetDirectories(Path.Combine(old,"apps"),"*.preserved-*").Select(x=>Path.Combine(x,"keep-user-file.txt")).Any(File.Exists),"Previous file lost");Pass("invalid existing version preserved separately");
  byte[] unsafeZip=Zip(new Dictionary<string,byte[]>{{"../outside.txt",new byte[]{1}}});await Fails(()=>Task.Run(()=>{using(var stream=new MemoryStream(unsafeZip))Engine.Extract(stream,Path.Combine(home,"unsafe"),CancellationToken.None);}),"traversal archive refused");
  Check(!Engine.Official(new Uri("http://github.com/x"))&&!Engine.Official(new Uri("https://github.com.evil.example/x"))&&!Engine.Official(new Uri("https://user:pass@github.com/x"))&&!Engine.Official(new Uri("https://github.com:444/x"))&&Engine.Official(new Uri("https://release-assets.githubusercontent.com/x")),"Official URL policy invalid");Pass("official HTTPS, redirect host, credentials and port policy");
  // Inspect the actual shipped PE without executing its Windows entry point.
  using(var file=File.OpenRead(Path.Combine(home,"BalatROM-Setup-1.0.0-Windows-x64.exe")))using(var pe=new PEReader(file)){
   Check(pe.PEHeaders.CoffHeader.Machine==Machine.Amd64,"Wrong target architecture");Check(pe.PEHeaders.PEHeader.Subsystem==Subsystem.WindowsGui,"Not a GUI EXE");Check(pe.PEHeaders.CorHeader!=null,"Managed runtime metadata absent");var reader=pe.GetMetadataReader();var directory=pe.GetSectionData(pe.PEHeaders.CorHeader.ResourcesDirectory.RelativeVirtualAddress);
   foreach(var handle in reader.ManifestResources){var resource=reader.GetManifestResource(handle);string name=reader.GetString(resource.Name);if(name!="Payload"&&name!="Config"&&name!="Manifest")continue;var data=directory.GetReader((int)resource.Offset,directory.Length-(int)resource.Offset);int length=data.ReadInt32();byte[] actual=data.ReadBytes(length);string expectedFile=name=="Payload"?"payload.zip":name=="Config"?"bootstrap-config.json":"package-manifest.json";Check(Engine.Hash(actual)==Engine.Hash(File.ReadAllBytes(Path.Combine(home,expectedFile))),"Embedded resource mismatch: "+name);}
  }Pass("actual x64 Windows GUI EXE and embedded bytes verified without execution");
  // Actual official components acquired earlier. No network or Windows process execution.
  var real=Engine.Json<Config>(File.ReadAllBytes(Path.Combine(home,"bootstrap-config.json")));byte[] payload=File.ReadAllBytes(Path.Combine(home,"payload.zip")),manifest=File.ReadAllBytes(Path.Combine(home,"package-manifest.json"));int copies=0;string originalComponents=args[1];
  string installed=await Engine.Install(Path.Combine(home,"QA actual installed app"),real,payload,manifest,CancellationToken.None,progress,(a,b,d,e)=>{d.ThrowIfCancellationRequested();File.Copy(Path.Combine(originalComponents,a.ArchiveFile),b);copies++;return Task.CompletedTask;});Check(copies==4,"Official component count differs");Check(Engine.Verify(Path.GetDirectoryName(installed),manifest,real.PackageManifestSha256,CancellationToken.None),"Full app differs");Pass("all four actual vendor components plus embedded data produce the exact reviewed manifest-bound app");
  int refusedCalls=0;await Engine.Install(Path.Combine(home,"QA actual installed app"),real,payload,manifest,CancellationToken.None,progress,(a,b,d,e)=>{refusedCalls++;throw new IOException("Offline fixture: must not download");});Check(refusedCalls==0,"Offline reopen attempted network");Pass("actual full installed app reopens with no component source/network request");
  Console.WriteLine("RESULT "+passed+" passed; runtime=.NET 10 on macOS; Windows .NET Framework/WinForms/native execution untested");
 }
 static async Task<int> Main(string[] args){try{await Run(args);return 0;}catch(Exception ex){Console.Error.WriteLine(ex);return 1;}}
}
