'use strict';
// One resource installation and packaging plan. OS-specific executable/bundle
// conventions are adapters, not separate application designs.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const product=require('./product.cjs');const {layout}=require('./platform.cjs');
const {validateSymlinkClosure,copyClosedTree}=require('./package-closure.cjs');
const APP_FILES=['package.json','product.cjs','platform.cjs','main.cjs','preload.cjs','protocol.cjs','renderer'];
function plan(platform,{runtime,workerDir,output}){
 const os=layout(platform);const targetPaths=platform==='win32'?path.win32:path.posix;
 for(const value of [runtime,workerDir,output])if(!value||!targetPaths.isAbsolute(value))throw Error('Use absolute runtime, worker and NEW output paths.');
 return {platform,version:product.version,runtime,workerDir,output,layout:os,appFiles:APP_FILES,
  nativeQualificationRequired:platform==='win32',runtimeAcceptance:false};
}
function installResources(plan,source=__dirname){
 const resources=path.join(plan.output,plan.layout.resources);fs.mkdirSync(resources,{recursive:true});
 const app=path.join(resources,'app');fs.mkdirSync(app);
 for(const name of APP_FILES)fs.cpSync(path.join(source,name),path.join(app,name),{recursive:true,verbatimSymlinks:true});
 copyClosedTree(plan.workerDir,path.join(resources,'worker'));
 const licenseRoot=plan.platform==='darwin'?path.dirname(plan.runtime):plan.runtime;
 for(const name of ['LICENSE','LICENSES.chromium.html'])fs.copyFileSync(path.join(licenseRoot,name),path.join(resources,`Electron-${name}`));
 return resources;
}
function assemble(plan,{qualification}={}){
 if(plan.platform!==process.platform)throw Error('Native packaging requires the matching authorized OS host; --plan is source preparation only.');
 if(fs.existsSync(plan.output))throw Error('Refusing to replace an existing output.');
 const executable=path.join(plan.workerDir,plan.layout.worker);
 if(!fs.statSync(executable).isFile())throw Error('Frozen worker missing.');
 if(plan.platform==='win32'){
  const sha=crypto.createHash('sha256').update(fs.readFileSync(executable)).digest('hex');
  if(!qualification||qualification.status!=='PASS_NATIVE_WINDOWS_WORKER'||qualification.worker_sha256!==sha||qualification.native_flow_verified!==true)
   throw Error('Windows native worker/flow qualification receipt is required; Mac tests do not qualify Windows.');
 }
 validateSymlinkClosure(plan.runtime);validateSymlinkClosure(plan.workerDir);
 copyClosedTree(plan.runtime,plan.output);
 fs.renameSync(path.join(plan.output,plan.layout.runtimeEntry),path.join(plan.output,plan.layout.entry));
 const resources=installResources(plan);
 if(plan.platform==='darwin'){
  fs.copyFileSync(path.join(__dirname,'app.icns'),path.join(resources,'app.icns'));
  const plist=path.join(plan.output,'Contents/Info.plist');let text=fs.readFileSync(plist,'utf8');
  const fields={CFBundleDisplayName:product.name,CFBundleName:product.name,CFBundleIdentifier:product.bundleId,CFBundleShortVersionString:product.version.split('-')[0],CFBundleVersion:product.version.split('-')[0],CFBundleIconFile:'app.icns',CFBundleExecutable:'BalatROM',LSMinimumSystemVersion:product.minimumMacOS};
  for(const [key,value] of Object.entries(fields)){
   const pattern=new RegExp('(<key>'+key+'<\\/key>\\s*<string>)[^<]*');
   if(!pattern.test(text))throw Error('Runtime lacks required bundle metadata: '+key);
   text=text.replace(pattern,(_match,prefix)=>prefix+value);
  }
  fs.writeFileSync(plist,text);
 }
 validateSymlinkClosure(plan.output);
 return {platform:plan.platform,version:product.version,assembled:true,runtimeAcceptance:false,signingOrPublication:false};
}
function cli(argv){
 const flag=argv.indexOf('--plan'),prepare=flag>=0;argv=argv.filter(x=>x!=='--plan');
 let platform=process.platform;const index=argv.indexOf('--platform');if(index>=0){platform=argv[index+1];argv.splice(index,2);}
 const qi=argv.indexOf('--qualification');let qualification;if(qi>=0){qualification=JSON.parse(fs.readFileSync(argv[qi+1],'utf8'));argv.splice(qi,2);}
 const [workerDir,output,explicitRuntime]=argv;
 const runtime=explicitRuntime||path.join(__dirname,'node_modules/electron/dist',platform==='darwin'?'Electron.app':'');
 const value=plan(platform,{runtime,workerDir,output});
 console.log(JSON.stringify(prepare?value:assemble(value,{qualification}),null,2));
}
module.exports={APP_FILES,plan,installResources,assemble,cli};
