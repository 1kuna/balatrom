'use strict';
const TYPES = new Set(['discovery','progress','complete','error','cancelled','exported','state']);
function validateEvent(value) {
  if (!value || typeof value !== 'object' || !TYPES.has(value.type)) throw new Error('Invalid worker event');
  if (value.type === 'progress' && value.total != null && (!Number.isInteger(value.total) || value.total < 0 || !Number.isInteger(value.completed) || value.completed < 0 || value.completed > value.total)) throw new Error('Invalid progress');
  if (value.type === 'state' && (typeof value.busy !== 'boolean' || (value.terminal != null && !['discovery','complete','error','cancelled','exported'].includes(value.terminal.type)))) throw new Error('Invalid state');
  return value;
}
module.exports = {validateEvent};
