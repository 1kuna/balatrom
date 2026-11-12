'use strict';
const api = window.balatrom;
const $ = id => document.getElementById(id);
const ui = {
  game: $('game'), gameTitle: $('game-title'), gameDetail: $('game-detail'), locate: $('locate'),
  rom: $('rom'), romTitle: $('rom-title'), romDetail: $('rom-detail'), time: $('time'), meter: $('meter'), fill: $('fill'),
  details: $('details'), detailsText: $('details-text'),
  hint: $('hint'), primary: $('primary'), secondary: $('secondary'), live: $('live')
};

// Worker stage ids, shown only when a progress event carries no message of its own.
const STAGES = {verify: 'Checking game', prepare: 'Preparing', download: 'Downloading tools', extract: 'Unpacking', convert: 'Converting', compile: 'Compiling', build: 'Building', link: 'Linking', package: 'Packaging', export: 'Saving'};

let state = 'discovering'; // discovering | ready | missing | building | cancelling | built | saved | failed
let phase = 'discover';    // the backend call a failure belongs to
let found = false, notice = '', started = 0, timer = 0, headline = '';
let onPrimary = null, onSecondary = null;

function put(node, value, title) { node.textContent = value || ''; if (title !== undefined) node.title = title || ''; }
function announce(message) { ui.live.textContent = ''; requestAnimationFrame(() => { ui.live.textContent = message; }); }
function classes(row, names) { row.classList.remove('found', 'failed', 'active', 'done', 'waiting'); if (names) row.classList.add(names); }
function gameView(title, detail, names) { put(ui.gameTitle, title); put(ui.gameDetail, detail, detail); classes(ui.game, names); }
function romView(title, detail, names) { put(ui.romTitle, title); put(ui.romDetail, detail, detail); classes(ui.rom, names); }

function clock(ms) { const s = Math.max(0, Math.floor(ms / 1000)); return `${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`; }
function tick() { ui.time.textContent = clock(Date.now() - started); }
function startClock() { if (timer) return; started = Date.now(); tick(); timer = setInterval(tick, 1000); }
function stopClock(keep) { clearInterval(timer); timer = 0; if (!keep) ui.time.textContent = ''; }

function primary(label, handler, busy) {
  ui.primary.hidden = !label; ui.primary.textContent = label || '';
  ui.primary.disabled = !handler; ui.primary.classList.toggle('busy', !!busy);
  onPrimary = handler || null;
}
function secondary(label, handler) {
  ui.secondary.hidden = !label; ui.secondary.textContent = label || '';
  ui.secondary.disabled = !handler; onSecondary = handler || null;
}
function settleFocus() {
  const active = document.activeElement;
  if (active && active !== document.body && !active.disabled && active.offsetParent !== null) return;
  for (const node of [ui.primary, ui.secondary, ui.locate]) if (!node.hidden && !node.disabled) { node.focus(); return; }
}
function nudge(node) { node.classList.remove('nudge'); void node.offsetWidth; node.classList.add('nudge'); }

// A bar only moves on real counts. No total, or nothing counted yet, means work of unknown size: sweep instead.
function showMeter(completed, total) {
  const counted = Number.isInteger(total) && total > 0 && Number.isInteger(completed);
  const moving = counted && completed > 0;
  ui.meter.hidden = false;
  ui.meter.classList.toggle('indeterminate', !moving);
  ui.fill.style.transform = moving ? `scaleX(${Math.min(1, completed / total)})` : '';
  if (counted) { ui.meter.setAttribute('aria-valuemax', String(total)); ui.meter.setAttribute('aria-valuenow', String(completed)); }
  else { ui.meter.removeAttribute('aria-valuemax'); ui.meter.removeAttribute('aria-valuenow'); }
  return counted;
}
function hideMeter() { ui.meter.hidden = true; ui.meter.classList.remove('indeterminate'); ui.fill.style.transform = ''; ui.meter.removeAttribute('aria-valuetext'); }
function showDetails(text) { ui.detailsText.textContent = text; ui.details.open = false; ui.details.hidden = false; }
function clearDetails() { ui.details.hidden = true; ui.details.open = false; ui.detailsText.textContent = ''; }

function stageName(stage) {
  if (typeof stage !== 'string' || !stage.trim()) return 'Building…';
  const key = stage.trim().toLowerCase();
  return `${STAGES[key] || key.replace(/[_-]+/g, ' ').replace(/^./, c => c.toUpperCase())}…`;
}

function call(method) {
  return Promise.resolve().then(() => method()).catch(error => fail((error && error.message) || 'The builder stopped responding. Reopen BalatROM.'));
}

function discover() {
  phase = 'discover'; state = 'discovering';
  stopClock(); hideMeter(); clearDetails(); put(ui.hint, '');
  gameView('Looking for Balatro…', '', '');
  romView('ROM', 'Not built', 'waiting');
  primary('Build ROM', null); secondary('');
  ui.locate.hidden = true;
  call(api.discover);
}
function locate() { phase = 'locate'; call(api.locate); }
function save() { phase = 'export'; call(api.export); }

function ready(title, detail) {
  state = 'ready';
  romView(title || 'ROM', detail === undefined ? 'Not built' : detail, '');
  primary('Build ROM', build); secondary('');
  ui.locate.hidden = false; ui.locate.disabled = false;
  put(ui.hint, notice);
  settleFocus();
}

function building() {
  state = 'building'; phase = 'build'; headline = '';
  clearDetails(); put(ui.hint, '');
  classes(ui.rom, 'active');
  startClock();
  primary('Building…', null, true);
  secondary('Cancel', cancel);
  ui.locate.disabled = true;
  settleFocus();
}

function build() {
  building();
  put(ui.romTitle, 'Starting…'); put(ui.romDetail, '', '');
  showMeter();
  call(api.build);
}

function cancel() {
  state = 'cancelling';
  put(ui.romTitle, 'Cancelling…'); put(ui.hint, '');
  primary(''); // a disabled "Building…" beside "Cancelling…" contradicts it; terminal events restore the primary
  secondary('Cancelling…', null);
  announce('Cancelling');
  call(api.cancel);
}

function fail(message, detail) {
  stopClock(); hideMeter();
  state = 'failed';
  const text = typeof message === 'string' && message.trim() ? message.trim() : 'Something went wrong.';
  const summary = text.split(/\r?\n/)[0];
  const full = [text, typeof detail === 'string' ? detail.trim() : ''].filter(Boolean).join('\n\n');
  if (full !== summary || summary.length > 120) showDetails(full); else clearDetails();
  put(ui.hint, '');
  ui.locate.disabled = false;
  if (phase === 'discover' || phase === 'locate' || !found) {
    gameView(phase === 'locate' ? 'Couldn’t use that copy' : 'Couldn’t check for Balatro', summary, 'failed');
    romView('ROM', 'Not built', 'waiting');
    ui.locate.hidden = true;
    primary('Try again', discover); secondary('Locate…', locate);
  } else if (phase === 'export') {
    romView('Couldn’t save ROM', summary, 'failed');
    primary('Save ROM…', save); secondary('');
  } else {
    romView('Build failed', summary, 'failed');
    primary('Try again', discover); secondary('');
  }
  announce(`${phase === 'export' ? 'Couldn’t save ROM' : 'Failed'}. ${summary}`);
  settleFocus();
}

function onDiscovery(v) {
  stopClock(); hideMeter(); clearDetails();
  found = v.status === 'supported';
  notice = typeof v.notice === 'string' ? v.notice : '';
  if (found) {
    gameView('Balatro', v.displayPath || 'Installed', 'found');
    ready();
    announce('Balatro found. Ready to build.');
    return;
  }
  state = 'missing';
  const multiple = v.status === 'multiple_supported';
  if (v.status === 'not_found') gameView('Balatro not found', 'Install your copy, or locate it.', '');
  else if (multiple) gameView('More than one copy found', v.message || 'Choose the one to use.', '');
  else {
    gameView('Can’t use this copy', v.message || 'Locate a supported copy of Balatro.', 'failed');
    if (v.message && v.message.length > 90) showDetails(v.message);
  }
  romView('ROM', 'Not built', 'waiting');
  primary(multiple ? 'Choose…' : 'Locate…', locate);
  secondary('Check again', discover);
  ui.locate.hidden = true;
  put(ui.hint, '');
  announce(`${ui.gameTitle.textContent}. ${ui.gameDetail.textContent}`);
  settleFocus();
}

function onProgress(v) {
  if (state !== 'building' && state !== 'cancelling') building();
  if (Number.isFinite(v.elapsedMs) && v.elapsedMs >= 0) { started = Date.now() - v.elapsedMs; tick(); }
  const counted = showMeter(v.completed, v.total);
  put(ui.romDetail, counted ? `${v.completed.toLocaleString()} / ${v.total.toLocaleString()}` : '', '');
  if (state === 'cancelling') return;
  const title = (typeof v.message === 'string' && v.message.trim()) || stageName(v.stage);
  put(ui.romTitle, title);
  ui.meter.setAttribute('aria-valuetext', counted ? `${title} ${v.completed} of ${v.total}` : title);
  if (title !== headline) { headline = title; announce(title); }
}

function onComplete() {
  stopClock(true); hideMeter();
  state = 'built';
  romView('ROM ready', 'Not saved yet', 'done');
  primary('Save ROM…', save); secondary('');
  ui.locate.disabled = false;
  announce('ROM ready');
  settleFocus();
  save();
}

function onExported(v) {
  state = 'saved';
  romView('Saved', v.path || '', 'done');
  primary(''); secondary('Save copy…', save);
  announce('ROM saved');
  settleFocus();
}

function onCancelled() {
  // A dismissed save dialog leaves the built ROM in place.
  if (state === 'built' || state === 'saved') return;
  stopClock(); hideMeter();
  ready('Build cancelled', '');
  announce('Build cancelled');
}

function onCloseRequest() {
  if (state !== 'building' && state !== 'cancelling') return;
  put(ui.hint, 'Cancel the build to close.');
  if (!ui.secondary.disabled) { ui.secondary.focus(); nudge(ui.secondary); }
}

function onEvent(v) {
  if (!v || typeof v !== 'object') return;
  if (v.type === 'discovery') onDiscovery(v);
  else if (v.type === 'progress') onProgress(v);
  else if (v.type === 'complete') onComplete(v);
  else if (v.type === 'exported') onExported(v);
  else if (v.type === 'cancelled') onCancelled(v);
  else if (v.type === 'error') fail(v.message, v.detail);
  else if (v.type === 'close-request') onCloseRequest(v);
}

// Disable synchronously on click, before any await, so a second click can't double-start.
ui.primary.addEventListener('click', () => { const run = onPrimary; if (run && !ui.primary.disabled) run(); });
ui.secondary.addEventListener('click', () => { const run = onSecondary; if (run && !ui.secondary.disabled) run(); });
ui.locate.addEventListener('click', () => { if (!ui.locate.disabled) locate(); });

if (api) { api.onEvent(onEvent); discover(); }
else { gameView('Builder unavailable', 'Open the installed BalatROM app.', 'failed'); primary(''); }
