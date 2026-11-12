'use strict';
const fs = require('node:fs');
const path = require('node:path');
function inside(root, candidate) {
  const relative = path.relative(root, candidate);
  return relative === '' || (!relative.startsWith(`..${path.sep}`) && relative !== '..' && !path.isAbsolute(relative));
}
function validateSymlinkClosure(root) {
  const absolute = fs.realpathSync(root);
  let count = 0;
  function walk(directory) {
    for (const name of fs.readdirSync(directory)) {
      const entry = path.join(directory, name);
      const stat = fs.lstatSync(entry);
      if (stat.isSymbolicLink()) {
        const target = fs.readlinkSync(entry);
        if (path.isAbsolute(target) || !inside(absolute, path.resolve(path.dirname(entry), target))) {
          throw new Error(`Package symlink escapes its bundle: ${path.relative(absolute, entry)}`);
        }
        let resolved;
        try { resolved = fs.realpathSync(entry); } catch { throw new Error(`Package symlink is dangling or cyclic: ${path.relative(absolute, entry)}`); }
        if (!inside(absolute, resolved)) throw new Error(`Package symlink resolves outside its bundle: ${path.relative(absolute, entry)}`);
        count++;
      } else if (stat.isDirectory()) walk(entry);
      else if (!stat.isFile()) throw new Error(`Unsupported package member: ${path.relative(absolute, entry)}`);
    }
  }
  walk(absolute);
  return {symlinks: count};
}
function copyClosedTree(source, destination) {
  validateSymlinkClosure(source);
  fs.cpSync(source, destination, {recursive:true, dereference:false, verbatimSymlinks:true});
  return validateSymlinkClosure(destination);
}
module.exports = {validateSymlinkClosure, copyClosedTree};
