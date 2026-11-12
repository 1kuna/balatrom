const {contextBridge,ipcRenderer}=require('electron');
let receiver; const send=value=>setTimeout(()=>receiver(value),10);
contextBridge.exposeInMainWorld('balatrom',{onEvent:fn=>{receiver=fn;},discover:async()=>send({type:'discovery',status:'supported',path:'/fixture/Balatro.app',displayPath:'Steam · supported owned installation'}),locate:async()=>{},build:async()=>send({type:'progress',stage:'compile',message:'Compiling your ROM',completed:14,total:198}),cancel:async()=>send({type:'cancelled'}),export:async()=>send({type:'exported',path:'/fixture/ROM folder/BalatROM.gbc'})});
ipcRenderer.on('fixture:event',(_event,value)=>receiver(value));
