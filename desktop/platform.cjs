'use strict';
const path=require('node:path');
function layout(platform){
 if(platform==='darwin')return {worker:'balatrom-worker',runtimeEntry:'Contents/MacOS/Electron',entry:'Contents/MacOS/BalatROM',resources:'Contents/Resources',locate:['openFile','openDirectory']};
 if(platform==='win32')return {worker:'balatrom-worker.exe',runtimeEntry:'electron.exe',entry:'BalatROM.exe',resources:'resources',locate:['openFile']};
 throw Error('This builder requires macOS or Windows.');
}
function workerPath(resources,platform){return (platform==='win32'?path.win32:path.posix).join(resources,'worker',layout(platform).worker);}
function workerLaunch(resources,platform){
 if(platform==='win32')return {binary:path.win32.join(resources,'worker','python.exe'),args:['-I','-B',path.win32.join(resources,'worker','bootstrap.py')]};
 return {binary:workerPath(resources,platform),args:[]};
}
module.exports={layout,workerPath,workerLaunch};
