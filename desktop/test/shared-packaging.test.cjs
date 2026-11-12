'use strict';
const{test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),os=require('node:os');
const{plan,installResources,assemble,APP_FILES}=require('../package-shared.cjs');
test('both targets install exactly the same UI and protocol files',()=>{
 const root=fs.mkdtempSync(path.join(os.tmpdir(),'builder-plan-'));try{
  let hashes;
  for(const platform of ['darwin','win32']){
   const worker=path.join(root,platform+'worker');fs.mkdirSync(worker);fs.writeFileSync(path.join(worker,platform==='win32'?'balatrom-worker.exe':'balatrom-worker'),'DECLARED NONEXECUTED FIXTURE');
   const runtime=path.join(root,platform+'runtime');fs.mkdirSync(runtime);const license=platform==='darwin'?root:runtime;
   for(const name of ['LICENSE','LICENSES.chromium.html'])fs.writeFileSync(path.join(license,name),'fixture notice');
   const p=plan(platform,{runtime,workerDir:worker,output:path.join(root,platform+'output')});const resources=installResources(p);
   const crypto=require('node:crypto');const current=APP_FILES.filter(x=>x!=='renderer').map(x=>crypto.createHash('sha256').update(fs.readFileSync(path.join(resources,'app',x))).digest('hex'));
   for(const name of ['index.html','app.js','style.css'])current.push(crypto.createHash('sha256').update(fs.readFileSync(path.join(resources,'app/renderer',name))).digest('hex'));
   if(hashes)assert.deepEqual(current,hashes);else hashes=current;
  }
 }finally{fs.rmSync(root,{recursive:true,force:true});}
});
test('foreign-platform plan is allowed, native assembly refuses before writing',()=>{
 const foreign=process.platform==='darwin'?'win32':'darwin';const output=path.join(os.tmpdir(),'not-created-balatrom-'+process.pid);
 const p=plan(foreign,{runtime:path.resolve('runtime'),workerDir:path.resolve('worker'),output});
 assert.equal(p.runtimeAcceptance,false);assert.throws(()=>assemble(p),/matching authorized OS/);assert.equal(fs.existsSync(output),false);
});
test('Windows plan accepts native drive paths without claiming native execution',()=>{
 const p=plan('win32',{runtime:'C:\\Electron Runtime',workerDir:'C:\\Worker With Spaces',output:'C:\\New Builder'});
 assert.equal(p.layout.entry,'BalatROM.exe');assert.equal(p.nativeQualificationRequired,true);assert.equal(p.runtimeAcceptance,false);
});
