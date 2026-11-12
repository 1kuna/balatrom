'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const {EventEmitter}=require('node:events');const {PassThrough}=require('node:stream');
const tick=()=>new Promise(r=>setImmediate(r));
async function harness(platform){
 const handlers=new Map(),events=[],children=[],windows=[],timers=[];const windowsOS=platform==='win32';
 const resources=windowsOS?'C:\\Preview With Spaces\\resources':'/Preview With Spaces/Contents/Resources';
 const app=new EventEmitter();app.isPackaged=true;app.setName=()=>{};app.getPath=()=>windowsOS?'C:\\User Data':'/User Data';app.whenReady=()=>Promise.resolve();app.quit=()=>{};
 class Window extends EventEmitter{
  constructor(options){super();this.options=options;this.webContents=new EventEmitter();this.webContents.mainFrame={};this.webContents.send=(_channel,value)=>events.push(value);this.webContents.setWindowOpenHandler=()=>{};this.webContents.session={setPermissionRequestHandler:()=>{}};windows.push(this);}
  loadFile(name){this.loaded=name;}isDestroyed(){return false;}
 }
 const spawn=(binary,args,options)=>{
  const child=new EventEmitter();child.stdin=new PassThrough();child.stdout=new PassThrough();child.stderr=new PassThrough();child.commands=[];
  child.stdin.on('data',line=>child.commands.push(JSON.parse(line)));child.binary=binary;child.args=args;child.options=options;children.push(child);return child;
 };
 const dialog={showOpenDialog:async()=>({canceled:false,filePaths:[windowsOS?'C:\\Owned Game\\Balatro.exe':'/Owned Game/Balatro.app']}),showSaveDialog:async()=>({canceled:false,filePath:windowsOS?'C:\\ROM Output\\game.gbc':'/ROM Output/game.gbc'})};
 const source=fs.readFileSync(path.join(__dirname,'../main.cjs'),'utf8');
 vm.runInNewContext(source,{__dirname:path.join(__dirname,'..'),console,setInterval:fn=>{timers.push(fn);return {unref(){}};},process:{platform,resourcesPath:resources,env:{}},require:name=>name==='electron'?{app,BrowserWindow:Window,ipcMain:{handle:(name,fn)=>handlers.set(name,fn)},dialog}:name==='node:child_process'?{spawn}:name==='node:path'?(windowsOS?path.win32:path.posix):require(name.startsWith('./')?path.join(__dirname,'..',name):name)});
 await tick();const w=windows[0],sender={sender:w.webContents,senderFrame:w.webContents.mainFrame};
 return {events,children,w,dialog,timers,call:(name)=>handlers.get('builder:'+name)(sender),reply:async(child,value)=>{child.stdout.write(JSON.stringify(value)+'\n');await tick();},handlers};
}
for(const platform of ['darwin','win32']){
 test(`${platform}: same discover → Build → progress → save workflow and cancellation`,async()=>{
  const h=await harness(platform);await h.call('discover');const child=h.children[0];
  assert(child.binary.endsWith(platform==='win32'?'worker\\python.exe':'worker/balatrom-worker'));if(platform==='win32'){assert.deepEqual(Array.from(child.args.slice(0,2)),['-I','-B']);assert(child.args[2].endsWith('worker\\bootstrap.py'));}assert.equal(child.options.windowsHide,true);assert(!child.options.shell);
  await h.reply(child,{id:1,type:'discovery',status:'supported',path:'owned game'});await h.call('build');assert.equal(child.commands.at(-1).command,'build');
  await h.reply(child,{type:'progress',completed:14,total:198,message:'Creating ROM'});await h.call('cancel');assert.equal(child.commands.at(-1).command,'cancel');
  await h.reply(child,{type:'cancelled',message:'Canceled safely'});await h.call('build');await h.reply(child,{type:'complete'});await h.call('export');
  assert.equal(child.commands.at(-1).command,'export');assert(child.commands.at(-1).args.destination.includes('ROM Output'));
  await h.reply(child,{type:'exported',path:'chosen destination'});assert.equal(h.events.at(-1).type,'exported');
 });
 test(`${platform}: missing install cannot silently start Build`,async()=>{
  const h=await harness(platform);await h.call('discover');await h.reply(h.children[0],{type:'discovery',status:'not_found',path:null});
  assert.throws(()=>h.call('build'),/Find your owned/);assert.equal(h.children[0].commands.length,1);
 });
 test(`${platform}: stale worker exit/response cannot clear a replacement`,async()=>{
  const h=await harness(platform);await h.call('discover');const old=h.children[0];old.emit('error',Error('failed worker'));
  await h.call('discover');const current=h.children[1];await h.reply(current,{type:'discovery',status:'supported',path:'owned'});
  old.emit('exit',1);const count=h.events.length;await h.reply(old,{type:'error',message:'stale'});assert.equal(h.events.length,count);
  await h.call('build');assert.equal(h.children.length,2);assert.equal(current.commands.at(-1).command,'build');
 });
 test(`${platform}: cancelled native dialog emits no export request`,async()=>{
  const h=await harness(platform);await h.call('discover');const c=h.children[0];await h.reply(c,{type:'discovery',status:'supported',path:'owned'});
  h.dialog.showSaveDialog=async()=>({canceled:true});await h.call('export');assert.equal(c.commands.length,1);
 });
}
for(const platform of ['darwin','win32'])test(`${platform}: broken connection fails closed and can reconnect`,async()=>{
 const h=await harness(platform);await h.call('discover');const old=h.children[0];
 old.stdin.emit('error',Error('closed'));await h.call('discover');const next=h.children[1];
 await h.reply(next,{type:'discovery',status:'supported',path:'owned'});old.emit('exit',1);
 await h.call('build');assert.equal(h.children.length,2);assert.equal(next.commands.at(-1).command,'build');
});
for(const platform of ['darwin','win32'])test(`${platform}: idle worker exit invalidates selected game before restart`,async()=>{
 const h=await harness(platform);await h.call('discover');const old=h.children[0];await h.reply(old,{type:'discovery',status:'supported',path:'owned'});
 old.emit('exit',0);assert.equal(h.events.at(-1).type,'error');assert.throws(()=>h.call('build'),/Find your owned/);
 await h.call('discover');const next=h.children[1];await h.reply(next,{type:'discovery',status:'supported',path:'owned'});await h.call('build');assert.equal(next.commands.at(-1).command,'build');
});

for(const platform of ['darwin','win32'])test(`${platform}: missed terminal response is recovered by status without duplicate build`,async()=>{
 const h=await harness(platform);await h.call('discover');const c=h.children[0];await h.reply(c,{id:1,type:'discovery',path:'owned'});await h.call('build');
 const request=c.commands.at(-1);assert.equal(request.command,'build');h.timers[0]();assert.equal(c.commands.at(-1).command,'status');
 await h.reply(c,{id:99,type:'state',busy:false,terminal:{id:request.id,type:'error',message:'The ROM build stopped.',detail:'Declared failure control'}});
 assert.equal(h.events.at(-1).type,'error');const count=h.events.length;
 await h.reply(c,{id:100,type:'state',busy:false,terminal:{id:request.id,type:'error',message:'The ROM build stopped.'}});assert.equal(h.events.length,count);
 await h.call('discover');assert.equal(c.commands.at(-1).command,'discover');assert.equal(c.commands.filter(x=>x.command==='build').length,1);
});
