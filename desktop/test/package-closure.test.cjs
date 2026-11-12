'use strict';
const {test}=require('node:test');const assert=require('node:assert/strict');const fs=require('node:fs');const os=require('node:os');const path=require('node:path');
const {copyClosedTree,validateSymlinkClosure}=require('../package-closure.cjs');
function fixture(callback) {const root=fs.mkdtempSync(path.join(os.tmpdir(),'balatrom-closure-'));try{return callback(root);}finally{fs.rmSync(root,{recursive:true,force:true});}}
test('relative framework links stay relative and independent after source disappears',()=>fixture(root=>{
 const source=path.join(root,'source');const dest=path.join(root,'copy');fs.mkdirSync(path.join(source,'Versions','A'),{recursive:true});fs.writeFileSync(path.join(source,'Versions','A','Binary'),'qualified');fs.symlinkSync('A',path.join(source,'Versions','Current'));fs.symlinkSync('Versions/Current/Binary',path.join(source,'Binary'));
 assert.deepEqual(copyClosedTree(source,dest),{symlinks:2});assert.equal(fs.readlinkSync(path.join(dest,'Binary')),'Versions/Current/Binary');assert.equal(fs.readlinkSync(path.join(dest,'Versions','Current')),'A');fs.rmSync(source,{recursive:true});assert.equal(fs.readFileSync(path.join(dest,'Binary'),'utf8'),'qualified');assert.deepEqual(validateSymlinkClosure(dest),{symlinks:2});
}));
test('rejects absolute links even when their target is inside the source',()=>fixture(root=>{fs.writeFileSync(path.join(root,'file'),'x');fs.symlinkSync(path.join(root,'file'),path.join(root,'link'));assert.throws(()=>validateSymlinkClosure(root),/escapes/);}));
test('rejects relative outside links and dangling links',()=>fixture(root=>{const source=path.join(root,'source');fs.mkdirSync(source);fs.writeFileSync(path.join(root,'outside'),'x');fs.symlinkSync('../outside',path.join(source,'link'));assert.throws(()=>validateSymlinkClosure(source),/escapes/);fs.unlinkSync(path.join(source,'link'));fs.symlinkSync('missing',path.join(source,'link'));assert.throws(()=>validateSymlinkClosure(source),/dangling/);}));
test('Mac package metadata and normal executable use shared product version', {skip:process.platform!=='darwin'},()=>{
 const {plan,assemble}=require('../package-shared.cjs');const product=require('../product.cjs');
 const root=fs.mkdtempSync(path.join(os.tmpdir(),'bundle-metadata-'));
 try{
  const runtime=path.join(root,'Electron.app'),worker=path.join(root,'worker'),output=path.join(root,'New.app');
  fs.mkdirSync(path.join(runtime,'Contents/MacOS'),{recursive:true});fs.mkdirSync(worker);
  fs.writeFileSync(path.join(runtime,'Contents/MacOS/Electron'),'NONEXECUTED RUNTIME FIXTURE');fs.writeFileSync(path.join(worker,'balatrom-worker'),'NONEXECUTED WORKER FIXTURE');
  for(const name of ['LICENSE','LICENSES.chromium.html'])fs.writeFileSync(path.join(root,name),'notice fixture');
  const keys=['CFBundleDisplayName','CFBundleName','CFBundleIdentifier','CFBundleShortVersionString','CFBundleVersion','CFBundleIconFile','CFBundleExecutable','LSMinimumSystemVersion'];
  fs.writeFileSync(path.join(runtime,'Contents/Info.plist'),'<plist><dict>'+keys.map(k=>'<key>'+k+'</key><string>old</string>').join('')+'</dict></plist>');
  const result=assemble(plan('darwin',{runtime,workerDir:worker,output}));assert.equal(result.runtimeAcceptance,false);
  const text=fs.readFileSync(path.join(output,'Contents/Info.plist'),'utf8');assert(text.includes('<key>CFBundleVersion</key><string>'+product.version.split('-')[0]+'</string>'));assert(text.includes('<key>CFBundleShortVersionString</key><string>'+product.version.split('-')[0]+'</string>'));
  assert(fs.existsSync(path.join(output,'Contents/MacOS/BalatROM')));assert(!fs.existsSync(path.join(output,'Contents/MacOS/Electron')));
 }finally{fs.rmSync(root,{recursive:true,force:true});}
});
