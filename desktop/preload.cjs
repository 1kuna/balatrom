'use strict';
const {contextBridge, ipcRenderer} = require('electron');
contextBridge.exposeInMainWorld('balatrom', Object.freeze({
  discover: () => ipcRenderer.invoke('builder:discover'),
  locate: () => ipcRenderer.invoke('builder:locate'),
  build: () => ipcRenderer.invoke('builder:build'),
  export: () => ipcRenderer.invoke('builder:export'),
  cancel: () => ipcRenderer.invoke('builder:cancel'),
  onEvent: callback => { const listener = (_event, value) => callback(value); ipcRenderer.on('builder:event', listener); return () => ipcRenderer.removeListener('builder:event', listener); }
}));
