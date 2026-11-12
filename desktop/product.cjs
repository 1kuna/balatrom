'use strict';
const {version}=require('./package.json');
// Preserve the existing user-data directory and bundle identity across the product version change.
module.exports=Object.freeze({name:'BalatROM',version,dataName:'BalatROM Builder Preview 0.3.5-rc.8',bundleId:'local.balatrom.private.sharedbuilder',minimumMacOS:'15.0'});
