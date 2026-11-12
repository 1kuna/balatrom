'use strict';
// Single native packaging entry for both supported desktop targets.
try {require('./package-shared.cjs').cli(process.argv.slice(2));}
catch(error){console.error(error.message);process.exitCode=1;}
