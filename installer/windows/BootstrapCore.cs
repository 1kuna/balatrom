using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Security.Cryptography;
using System.Threading;
using System.Threading.Tasks;

namespace BalatROM.Setup {
 [DataContract] public sealed class Component {
  [DataMember] public string Name;
  [DataMember] public string Url;
  [DataMember] public long Bytes;
  [DataMember] public string Sha256;
  [DataMember] public string Destination;
  [DataMember] public string ArchiveFile;
 }
 [DataContract] public sealed class Config {
  [DataMember] public string Version;
  [DataMember] public string AppFolder;
  [DataMember] public Component[] Components;
  [DataMember] public string PayloadSha256;
  [DataMember] public string PackageManifestSha256;
  [DataMember] public string OriginalZipSha256;
 }
 [DataContract] public sealed class PackageRow {
  [DataMember(Name="path")] public string Path;
  [DataMember(Name="bytes")] public long Bytes;
  [DataMember(Name="sha256")] public string Sha256;
 }
 [DataContract] public sealed class PackageManifest {
  [DataMember(Name="files")] public PackageRow[] Files;
 }
 public sealed class ProgressInfo {
  public string Message; public long Completed; public long Total;
  public ProgressInfo(string message, long completed=0, long total=0) { Message=message;Completed=completed;Total=total; }
 }
 public static class Engine {
  public static T Json<T>(byte[] bytes) {
   using(var stream=new MemoryStream(bytes)) return (T)new DataContractJsonSerializer(typeof(T)).ReadObject(stream);
  }
  public static string Hash(Stream input) { using(var h=SHA256.Create())return BitConverter.ToString(h.ComputeHash(input)).Replace("-","").ToLowerInvariant(); }
  public static string Hash(byte[] bytes) { using(var s=new MemoryStream(bytes))return Hash(s); }
  public static string HashFile(string path) { using(var s=File.OpenRead(path))return Hash(s); }
  public static void NoLinks(string path) {
   string current=Path.GetFullPath(path);
   while(!String.IsNullOrEmpty(current)) {
    if((File.Exists(current)||Directory.Exists(current))&&(File.GetAttributes(current)&FileAttributes.ReparsePoint)!=0)
     throw new IOException("Linked setup paths are not supported. Existing files were preserved.");
    var parent=Directory.GetParent(current);current=parent==null?null:parent.FullName;
   }
  }
  public static string Child(string root,string relative) {
   if(String.IsNullOrWhiteSpace(relative)||relative.Contains("\\")||relative.Contains(":")||Path.IsPathRooted(relative)||relative.Split('/').Any(p=>p==".."||p=="."))
    throw new IOException("An unsafe setup archive path was refused.");
   string result=Path.GetFullPath(Path.Combine(root,relative.Replace('/',Path.DirectorySeparatorChar)));
   if(!result.StartsWith(Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar)+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase))
    throw new IOException("An archive member escapes its setup folder.");
   NoLinks(result);return result;
  }
  public static void Extract(Stream input,string root,CancellationToken cancel) {
   long expanded=0;var names=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
   using(var archive=new ZipArchive(input,ZipArchiveMode.Read,true)) {
    foreach(var entry in archive.Entries) {
     cancel.ThrowIfCancellationRequested();string name=entry.FullName;
     string output=Child(root,name.TrimEnd('/'));int mode=(entry.ExternalAttributes>>16)&0xF000;
     if(mode!=0&&mode!=0x8000&&mode!=0x4000)throw new IOException("Linked or special archive member refused.");
     if(!names.Add(name.TrimEnd('/')))throw new IOException("Duplicate archive member refused.");
     if(name.EndsWith("/")){Directory.CreateDirectory(output);continue;}
     expanded+=entry.Length;
     if(entry.Length>350L*1024*1024||expanded>1024L*1024*1024||names.Count>10000)throw new IOException("Setup archive exceeds its size limit.");
     Directory.CreateDirectory(Path.GetDirectoryName(output));
     using(var reader=entry.Open())using(var writer=new FileStream(output,FileMode.Create,FileAccess.Write,FileShare.None)) {
      byte[] buffer=new byte[65536];int n;long count=0;
      while((n=reader.Read(buffer,0,buffer.Length))!=0){cancel.ThrowIfCancellationRequested();count+=n;if(count>entry.Length)throw new IOException("Archive size mismatch.");writer.Write(buffer,0,n);}
      if(count!=entry.Length)throw new IOException("Archive is incomplete.");
     }
    }
   }
  }
  public static bool Verify(string app,byte[] manifest,string expectedHash,CancellationToken cancel) {
   if(Hash(manifest)!=expectedHash)throw new IOException("Embedded package manifest failed integrity verification.");
   if(!Directory.Exists(app))return false;NoLinks(app);
   string installed=Path.Combine(app,"PACKAGE-MANIFEST.json");
   if(!File.Exists(installed)||HashFile(installed)!=expectedHash)return false;
   var rows=Json<PackageManifest>(manifest).Files;var expected=new HashSet<string>(StringComparer.OrdinalIgnoreCase){"PACKAGE-MANIFEST.json"};
   foreach(var row in rows) {
    cancel.ThrowIfCancellationRequested();string path=Child(app,row.Path);expected.Add(row.Path);
    if(!File.Exists(path)||new FileInfo(path).Length!=row.Bytes||HashFile(path)!=row.Sha256)return false;
   }
   var directories=new Stack<string>();directories.Push(app);
   while(directories.Count>0) {
    string directory=directories.Pop();NoLinks(directory);
    foreach(var child in Directory.EnumerateDirectories(directory)){NoLinks(child);directories.Push(child);}
    foreach(var file in Directory.EnumerateFiles(directory)) {
     cancel.ThrowIfCancellationRequested();NoLinks(file);
     string name=file.Substring(app.TrimEnd(Path.DirectorySeparatorChar).Length+1).Replace(Path.DirectorySeparatorChar,'/');
     string extension=Path.GetExtension(file).ToLowerInvariant();
     if(!expected.Contains(name)&&new[]{".exe",".dll",".pyd",".py",".js",".cjs",".html",".asar",".pak",".dat"}.Contains(extension))return false;
    }
   }
   return true;
  }
  public static bool Official(Uri uri) {
   return uri.Scheme=="https"&&String.IsNullOrEmpty(uri.UserInfo)&&uri.Port==443&&new[]{"github.com","release-assets.githubusercontent.com","objects.githubusercontent.com","www.python.org","files.pythonhosted.org"}.Contains(uri.Host.ToLowerInvariant());
  }
  public static async Task Download(Component component,string output,CancellationToken cancel,Action<ProgressInfo> progress) {
   Uri uri=new Uri(component.Url);if(!Official(uri))throw new IOException("The setup URL is not an approved official source.");
   using(var timeout=CancellationTokenSource.CreateLinkedTokenSource(cancel)) {
    timeout.CancelAfter(TimeSpan.FromMinutes(8));var token=timeout.Token;
    using(var handler=new HttpClientHandler{AllowAutoRedirect=false,UseDefaultCredentials=false,CheckCertificateRevocationList=true})
    using(var client=new HttpClient(handler){Timeout=System.Threading.Timeout.InfiniteTimeSpan}) {
     client.DefaultRequestHeaders.UserAgent.ParseAdd("BalatROM-Private-Setup/1.0.0");
     for(int redirects=0;redirects<6;redirects++) {
      token.ThrowIfCancellationRequested();if(!Official(uri))throw new IOException("Download redirected outside the approved official hosts.");
      using(var response=await client.GetAsync(uri,HttpCompletionOption.ResponseHeadersRead,token).ConfigureAwait(false)) {
       if((int)response.StatusCode>=300&&(int)response.StatusCode<400) {
        if(response.Headers.Location==null)throw new IOException("Official download returned no redirect location.");
        uri=new Uri(uri,response.Headers.Location);continue;
       }
       response.EnsureSuccessStatusCode();
       if(response.Content.Headers.ContentLength.HasValue&&response.Content.Headers.ContentLength.Value!=component.Bytes)throw new IOException("Official download length differs from the pinned package.");
       using(var input=await response.Content.ReadAsStreamAsync().ConfigureAwait(false))
       using(var file=new FileStream(output,FileMode.CreateNew,FileAccess.Write,FileShare.None,65536,true)) {
        byte[] buffer=new byte[65536];long count=0;int n;var next=DateTime.UtcNow;
        while((n=await input.ReadAsync(buffer,0,buffer.Length,token).ConfigureAwait(false))>0) {
         count+=n;if(count>component.Bytes)throw new IOException("Download exceeds the pinned package size.");
         await file.WriteAsync(buffer,0,n,token).ConfigureAwait(false);
         if(DateTime.UtcNow>=next){progress(new ProgressInfo("Downloading "+component.Name,count,component.Bytes));next=DateTime.UtcNow.AddMilliseconds(150);}
        }
        if(count!=component.Bytes)throw new IOException("Download ended early. Retry setup when the connection is ready.");
       }
       token.ThrowIfCancellationRequested();
       if(HashFile(output)!=component.Sha256)throw new IOException("Official download failed its SHA256 integrity check.");
       return;
      }
     }
    }
    throw new IOException("Too many official download redirects.");
   }
  }
  public static async Task<string> Install(string root,Config config,byte[] payload,byte[] manifest,CancellationToken cancel,Action<ProgressInfo> progress,Func<Component,string,CancellationToken,Action<ProgressInfo>,Task> source=null) {
   if(Hash(payload)!=config.PayloadSha256||Hash(manifest)!=config.PackageManifestSha256)throw new IOException("Embedded setup data failed integrity verification.");
   if(config.AppFolder.IndexOfAny(new[]{'/','\\',':'})>=0||config.AppFolder=="..")throw new IOException("Invalid version folder.");
   NoLinks(root);Directory.CreateDirectory(root);
   string final=Path.Combine(root,"apps",config.AppFolder);progress(new ProgressInfo("Checking installed files"));
   if(Verify(final,manifest,config.PackageManifestSha256,cancel))return Path.Combine(final,"BalatROM.exe");
   if(new DriveInfo(Path.GetPathRoot(Path.GetFullPath(root))).AvailableFreeSpace<2L*1024*1024*1024)throw new IOException("Setup needs at least 2 GiB of free disk space.");
   string cache=Path.Combine(root,"downloads");Directory.CreateDirectory(cache);NoLinks(cache);
   string stage=Path.Combine(root,"pending-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(stage);
   try {
    foreach(var component in config.Components) {
     cancel.ThrowIfCancellationRequested();string cached=Path.Combine(cache,component.Sha256+".zip");NoLinks(cached);
     if(File.Exists(cached)&&(new FileInfo(cached).Length!=component.Bytes||HashFile(cached)!=component.Sha256)) {
      File.Move(cached,cached+".preserved-"+Guid.NewGuid().ToString("N"));
     }
     if(!File.Exists(cached)) {
      string temporary=cached+".pending-"+Guid.NewGuid().ToString("N");
      try{progress(new ProgressInfo("Getting "+component.Name+" from its official source"));await (source??Download)(component,temporary,cancel,progress).ConfigureAwait(false);cancel.ThrowIfCancellationRequested();if(new FileInfo(temporary).Length!=component.Bytes||HashFile(temporary)!=component.Sha256)throw new IOException("Official component failed verification.");File.Move(temporary,cached);}
      finally{if(File.Exists(temporary))File.Delete(temporary);}
     }
     cancel.ThrowIfCancellationRequested();progress(new ProgressInfo("Preparing "+component.Name));
     string destination=String.IsNullOrEmpty(component.Destination)?stage:Child(stage,component.Destination.TrimEnd('/'));Directory.CreateDirectory(destination);
     using(var archive=File.OpenRead(cached))Extract(archive,destination,cancel);
     if(component.Name=="electron")File.Move(Path.Combine(stage,"electron.exe"),Path.Combine(stage,"BalatROM.exe"));
    }
    progress(new ProgressInfo("Preparing the BalatROM builder"));using(var embedded=new MemoryStream(payload))Extract(embedded,stage,cancel);
    cancel.ThrowIfCancellationRequested();progress(new ProgressInfo("Verifying every application file"));
    if(!Verify(stage,manifest,config.PackageManifestSha256,cancel))throw new IOException("Setup contents differ from the reviewed app. Existing files were preserved.");
    cancel.ThrowIfCancellationRequested();Directory.CreateDirectory(Path.GetDirectoryName(final));NoLinks(Path.GetDirectoryName(final));
    if(Directory.Exists(final))Directory.Move(final,final+".preserved-"+Guid.NewGuid().ToString("N"));
    Directory.Move(stage,final);return Path.Combine(final,"BalatROM.exe");
   }finally{if(Directory.Exists(stage))Directory.Delete(stage,true);}
  }
 }
}
