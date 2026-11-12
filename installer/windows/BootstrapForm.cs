using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Runtime.Versioning;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using BalatROM.Setup;
[assembly:TargetFramework(".NETFramework,Version=v4.8",FrameworkDisplayName=".NET Framework 4.8")]
[assembly:AssemblyTitle("BalatROM 1.0.0 Setup")]
[assembly:AssemblyDescription("BalatROM setup with verified official runtimes")]
[assembly:AssemblyVersion("1.0.0.0")]
[assembly:AssemblyFileVersion("1.0.0.0")]
internal sealed partial class SetupForm:Form {
 readonly Label status=new Label(),detail=new Label();readonly Button start=new Button(),cancel=new Button();readonly ProgressBar bar=new ProgressBar();
 readonly Config config;readonly byte[] payload,manifest;readonly string root;
 readonly SetupRunState runState=new SetupRunState();CancellationTokenSource token;bool closing;
 bool busy {get{return runState.IsRunning;}}
 static byte[] Resource(string name){using(var stream=Assembly.GetExecutingAssembly().GetManifestResourceStream(name))using(var output=new MemoryStream()){stream.CopyTo(output);return output.ToArray();}}
 public SetupForm(){
  config=Engine.Json<Config>(Resource("Config"));payload=Resource("Payload");manifest=Resource("Manifest");
  root=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"BalatROM","WindowsTest");
  sources="Electron — GitHub / Electron; Python — python.org; Pillow — PyPI; GBDK — GitHub / gbdk-2020.";
  BuildUi();
  start.Click+=async(s,e)=>await Run();
  cancel.Click+=(s,e)=>{if(busy){closing=true;token.Cancel();status.Text="Canceling…";}else Close();};
  FormClosing+=(s,e)=>{if(busy){e.Cancel=true;closing=true;token.Cancel();status.Text="Canceling…";}};
  Shown+=async(s,e)=>{
   try{bool ready=await Task.Run(()=>Engine.Verify(Path.Combine(root,"apps",config.AppFolder),manifest,config.PackageManifestSha256,CancellationToken.None));if(ready&&!runState.HasStarted&&!closing&&!IsDisposed)await Run();}
   catch(Exception ex){if(!runState.HasStarted&&!closing&&!IsDisposed){ShowErrorVisual("Setup needs attention",ex.GetBaseException().Message);ShowRetryVisual();}}
  };
 }
 void Report(ProgressInfo p){
  int generation=runState.Generation;if(IsDisposed||!IsHandleCreated)return;
  try{BeginInvoke(new Action(()=>{if(!IsDisposed&&runState.CanApplyProgress(generation))ShowProgressVisual(p);}));}catch(InvalidOperationException){}
 }
 async Task Run(){
  if(!runState.TryStart())return;
  token=new CancellationTokenSource();ShowBusyVisual();
  bool own=false,success=false;Mutex mutex=null;
  try{
   await Task.Yield();
   token.Token.ThrowIfCancellationRequested();
   if(!Environment.Is64BitOperatingSystem)throw new IOException("BalatROM requires 64-bit Windows.");
   mutex=new Mutex(false,"Local\\BalatROM-Private-Windows-Setup-0352");
   try{own=mutex.WaitOne(0);}catch(AbandonedMutexException){own=true;}
   if(!own)throw new IOException("Another setup is running. Let it finish, then retry.");
   var localToken=token.Token;
   string executable=await Task.Run(()=>Engine.Install(root,config,payload,manifest,localToken,Report));
   localToken.ThrowIfCancellationRequested();
   Process.Start(new ProcessStartInfo(executable){UseShellExecute=false,WorkingDirectory=Path.GetDirectoryName(executable)});success=true;
  }catch(OperationCanceledException){ShowErrorVisual(token.IsCancellationRequested?"Setup canceled":"Connection timed out","Existing files were preserved.");}
  catch(Exception ex){ShowErrorVisual("Setup couldn’t finish",ex.GetBaseException().Message);}
  finally{
   try{if(own)mutex.ReleaseMutex();}finally{if(mutex!=null)mutex.Dispose();token.Dispose();}
   runState.FinishAfterTeardown(success,closing);
   if(success||closing)Close();else ShowRetryVisual();
  }
 }
 [STAThread] public static void Main(){Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);Application.Run(new SetupForm());}
}
