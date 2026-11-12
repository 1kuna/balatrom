'use strict';
const {test}=require('node:test');const assert=require('node:assert/strict');const {validateEvent}=require('../protocol.cjs');
test('accepts legitimate progress including zero and final',()=>{for(const completed of [0,14,198])assert.equal(validateEvent({type:'progress',completed,total:198}).completed,completed);});
test('rejects nonsensical progress and unknown event types',()=>{for(const e of [{type:'exec'}, {type:'progress',completed:199,total:198},{type:'progress',completed:-1,total:198},{type:'progress',completed:1.5,total:198}])assert.throws(()=>validateEvent(e));});
test('native paths never enter markup',()=>{const fs=require('node:fs');const source=fs.readFileSync(require('node:path').join(__dirname,'../renderer/app.js'),'utf8');assert(!source.includes('innerHTML'));});
test('renderer cannot execute arbitrary node commands',()=>{const fs=require('node:fs');const main=fs.readFileSync(require('node:path').join(__dirname,'../main.cjs'),'utf8');assert(main.includes('nodeIntegration:false'));assert(main.includes('contextIsolation:true'));assert(main.includes('sandbox:true'));assert(!main.includes('shell:true'));});
