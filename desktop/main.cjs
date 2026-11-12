'use strict';
const {app, BrowserWindow, ipcMain, dialog} = require('electron');
const path = require('node:path');
const {spawn} = require('node:child_process');
const readline = require('node:readline');
const {validateEvent} = require('./protocol.cjs');
const product=require('./product.cjs');
const {layout,workerPath,workerLaunch}=require('./platform.cjs');
app.setName(product.dataName);
let window, worker, currentGame, busy = false, sequence = 0, outstanding = false;
function emit(value) { if (window && !window.isDestroyed()) window.webContents.send('builder:event', value); }
function startWorker() {
  // Packaging sets this to the bundled, frozen worker. No shell or PATH lookup.
  const launch = app.isPackaged ? workerLaunch(process.resourcesPath,process.platform) : {binary:process.env.BALATROM_WORKER,args:[]};
  const binary = launch.binary;
  if (!binary || !path.isAbsolute(binary)) throw new Error('The bundled build service is missing. Reinstall this app.');
  const child = spawn(binary, [...launch.args, '--root', path.join(app.getPath('userData'),'versions')], {stdio:['pipe','pipe','pipe'], windowsHide:true});
  worker=child;
  function disconnect(message){if(worker!==child)return;worker=null;busy=false;outstanding=false;currentGame=null;if(!child.stdin.destroyed)child.stdin.end();emit({type:'error',message});}
  let stderr = '';
  const seenTerminal=new Set();
  function applyEvent(event) {
    if (event.type==='state') {if(outstanding&&!event.busy&&event.terminal)applyEvent(validateEvent(event.terminal));return;}
    if (['discovery','complete','error','cancelled','exported'].includes(event.type)) {
      const key=String(event.id)+':'+event.type;if(seenTerminal.has(key))return;seenTerminal.add(key);
      outstanding=false;busy=false;
    }
    if(event.type==='discovery')currentGame=event.path||null;
    emit(event);
  }
  child.stderr.on('data', chunk => { stderr = (stderr + chunk.toString()).slice(-4096); });
  readline.createInterface({input:child.stdout}).on('line', line => {
    if(worker!==child)return;
    if (line.length > 65536) { disconnect('The build service returned an invalid response. Choose Check again.');return; }
    try {
      const event = validateEvent(JSON.parse(line));
      applyEvent(event);
    } catch { disconnect('The build service returned an invalid response. Choose Check again.'); }
  });
  child.stdin.on('error', () => disconnect('The build service connection closed. Choose Check again to reconnect.'));
  child.on('error', () => disconnect('The build service could not start. Reinstall this app or choose Check again.'));
  child.on('exit', code => { if(worker!==child)return; worker=null; currentGame=null; busy=false; outstanding=false; emit({type:'error', message:`The build service stopped (${code}). Choose Check again to reconnect. Your installation and existing saves were not changed.`}); });
}
function send(command, args={}) { if (!worker) startWorker(); if(!['status','cancel'].includes(command))outstanding=true;worker.stdin.write(JSON.stringify({id:++sequence,command,args})+'\n'); }
// Resynchronize a missed terminal event without starting a second operation.
const statusTimer=setInterval(()=>{if(worker&&outstanding)send('status');},1000);
statusTimer.unref();
function trusted(event) { if (event.sender !== window.webContents || event.senderFrame !== window.webContents.mainFrame) throw new Error('Untrusted request'); }
app.whenReady().then(() => {
  window = new BrowserWindow({width:520,height:320,minWidth:400,minHeight:300,backgroundColor:'#101b19',title:'BalatROM',webPreferences:{preload:path.join(__dirname,'preload.cjs'),contextIsolation:true,nodeIntegration:false,sandbox:true}});
  window.webContents.session.setPermissionRequestHandler((_webContents,_permission,callback)=>callback(false));
  window.webContents.setWindowOpenHandler(() => ({action:'deny'}));
  window.webContents.on('will-navigate', event => event.preventDefault());
  ipcMain.handle('builder:discover', event => {trusted(event); if (!busy) {busy=true; try {send('discover');} catch(error) {busy=false; throw error;}}});
  ipcMain.handle('builder:locate', async event => { trusted(event); if (busy) return; busy=true; try {const choice=await dialog.showOpenDialog(window,{title:'Locate your Balatro installation',properties:layout(process.platform).locate}); if (choice.canceled) {busy=false; return;} send('locate',{path:choice.filePaths[0]});} catch(error) {busy=false; throw error;} });
  ipcMain.handle('builder:build', event => { trusted(event); if(busy)throw new Error('Wait for the current operation to finish.'); if(!currentGame)throw new Error('Find your owned Balatro installation before building.'); busy=true; try {send('build');} catch(error) {busy=false; throw error;} });
  ipcMain.handle('builder:export', async event => { trusted(event); if (busy) return; busy=true; try {const choice=await dialog.showSaveDialog(window,{title:'Save your BalatROM ROM',defaultPath:'BalatROM.gbc',filters:[{name:'Game Boy Color ROM',extensions:['gbc']}],properties:['showOverwriteConfirmation']}); if (choice.canceled) {busy=false; return;} send('export',{destination:choice.filePath});} catch(error) {busy=false; throw error;} });
  ipcMain.handle('builder:cancel', event => {trusted(event); if (busy) send('cancel');});
  window.on('close', event => { if (busy) {event.preventDefault(); emit({type:'close-request'});} });
  window.loadFile(path.join(__dirname,'renderer','index.html'));
});
app.on('window-all-closed', () => { if (worker) worker.stdin.end(); app.quit(); });
