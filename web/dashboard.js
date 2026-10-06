// Served from the A16 itself. No CDN, Home Assistant or browser-side control timers.
const inputNames = ['Tank · On', 'Tank · Auto', 'Pump · On', 'Pump · Auto', 'Spin button', 'UV / system button'];
const inputSlugs = ['in01_tank_on', 'in02_tank_auto', 'in03_pump_on', 'in04_pump_auto', 'in05_spin_button', 'in06_uv_button'];
const outputNames = ['Pump contactor', 'Tank contactor', 'UV contactor', 'Spin-flush valve', 'System-flush valve', 'Spare', 'Spare', 'Spare', 'Pump selector LED', 'Tank selector LED', 'Spin button LED', 'UV button LED', 'Spare', 'Spare', 'Spare', 'Spare'];
const pad = n => String(n).padStart(2, '0');
const key = (domain, name) => `${domain}-${name}`;
const states = new Map();
let connected = false;
let busy = false;
let lastMessage = 0;
let renderQueued = false;
let lastTesting = false;

document.title = 'Water Closet';
const viewport = document.createElement('meta');
viewport.name = 'viewport';
viewport.content = 'width=device-width, initial-scale=1';
document.head.append(viewport);

function inputCard(n) {
  const slug = inputSlugs[n - 1] || `in${pad(n)}_spare`;
  return `<div class="input-card" data-input="${slug}"><span class="terminal">IN${pad(n)}</span><span>${inputNames[n - 1] || 'Spare input'}</span><strong class="input-value">—</strong></div>`;
}
function outputCard(n) {
  return `<div class="output-card"><span class="terminal">OUT${pad(n)}</span><span class="output-name">${outputNames[n - 1]}</span><button class="toggle" data-output="${n}" aria-label="OUT${pad(n)} ${outputNames[n - 1]}" aria-pressed="false" disabled>—</button></div>`;
}
const numberSettings = [
  ['spin_duration', 'spin_flush_duration', 'Spin duration', 'seconds', 1, 300],
  ['spin_interval', 'spin_flush_interval', 'Spin interval', 'minutes', 1, 1440],
  ['system_duration', 'system_flush_duration', 'System refresh', 'seconds', 1, 300],
  ['system_interval', 'system_flush_interval', 'System interval', 'minutes', 1, 1440],
  ['away_duration', 'away_flush_duration', 'System exchange duration', 'seconds', 1, 1800],
  ['away_flushes_per_day', 'away_flushes_per_day', 'System exchange frequency', 'runs / day', 1, 4],
  ['away_spin_duration', 'away_spin_flush_duration', 'Away spin duration', 'seconds', 1, 300],
  ['away_spin_frequency', 'away_spin_flushes_per_day', 'Away spin frequency', 'runs / day', 1, 4]
];
const healthSettings = [
  ...['pump','tank','uv'].map(load => [`${load}_calibration`,`${load}_current_calibration`,`${load === 'uv' ? 'UV' : load[0].toUpperCase()+load.slice(1)} calibration`,'A / signal V',0,1000,0.01]),
  ...['pump','tank'].map(load => [`${load}_threshold`,`${load}_running_threshold`,`${load === 'pump' ? 'Pump' : 'Tank'} running threshold`,'A',0,100,0.05]),
  ['uv_minimum','uv_minimum_current','UV minimum current','A',0,100,0.01],
  ['uv_warmup','uv_current_warmup','UV warmup grace','seconds',0,600],
  ['uv_low_delay','uv_low_current_delay','UV low current delay','seconds',0,300]
];
const settingForms = list => list.map(([id,slug,label,unit,min,max,step=1])=>`<form data-number="${slug}"><label for="${id}">${label} <span>${unit}</span></label><div><input id="${id}" type="number" min="${min}" max="${max}" step="${step}" required><button type="submit">Save</button></div></form>`).join('');

document.querySelector('esp-app')?.remove();
document.body.insertAdjacentHTML('beforeend', `
<header class="masthead"><div class="brand"><span class="drop" aria-hidden="true"></span><div><h1>Water Closet</h1><span>A16 · Local control</span></div></div><span id="connection" class="badge">Connecting…</span></header>
<main>
  <section class="panel mode-panel" aria-labelledby="mode-heading">
    <div class="mode-intro"><span class="eyebrow">System mode</span><h2 id="mode-heading">Connecting…</h2></div>
    <div class="mode-actions" role="group" aria-label="System mode"><button data-mode="Normal" disabled>Normal</button><button data-mode="Away" disabled>Away</button><button data-mode="Shutdown" class="shutdown" disabled>Shutdown</button></div>
    <p id="mode-help" class="help">Waiting for the controller.</p>
    <div class="selector-strip"><span>Pump selector <strong id="pump-selector">—</strong></span><span>Tank selector <strong id="tank-selector">—</strong></span><span>Controller uptime <strong id="uptime">—</strong></span></div>
    <div class="equipment-grid">
      <div class="equipment-card" id="pump-card"><span class="eyebrow">Water pump</span><strong id="pump-state">—</strong><p id="pump-detail">Waiting for controller data</p></div>
      <div class="equipment-card" id="tank-card"><span class="eyebrow">Hot water tank</span><strong id="tank-state">—</strong><p id="tank-detail">Waiting for controller data</p></div>
      <div class="equipment-card" id="uv-card"><span class="eyebrow">UV light</span><strong id="uv-state">—</strong><span id="uv-countdown" class="countdown" hidden></span><p id="uv-detail">Waiting for controller data</p></div>
      <div class="equipment-card" id="flush-card"><span class="eyebrow">Flushing</span><strong id="flush-status">—</strong><span id="flush-countdown" class="countdown" hidden></span><p id="flush-detail">Waiting for controller data</p></div>
    </div>
    <div id="control-fault" role="status" hidden></div>
    <div class="network-strip"><span>Wi-Fi <strong id="wifi-state">—</strong></span><span title="Authenticated Home Assistant client connection; this does not check HA automations.">Home Assistant <strong id="ha-state">—</strong></span></div>
    <div id="away-plan" hidden><div class="section-heading"><h3>Away plan</h3><span class="muted">Water flushing · UV lamp stays off</span></div><div class="schedule-grid">${['system','spin'].map(kind=>`<div class="schedule-card"><strong>${kind === 'system' ? 'System / UV water exchange' : 'Spin filter flush'}</strong><p id="away-${kind}-settings">—</p><b id="away-${kind}-next">—</b></div>`).join('')}</div><p class="small-note">Each run temporarily enables the pump. One valve at a time. Clock estimates use Central time; the A16 runs the schedules without Wi-Fi.</p></div>
    <p class="status-note">Equipment states show output commands. Current and pressure sensors are not installed yet.</p>
  </section>
  <div id="message" role="status" aria-live="polite" hidden></div>
  <section class="panel" aria-labelledby="manual-heading">
    <div class="section-heading"><h2 id="manual-heading">Manual controls</h2><span class="muted">Normal operating rules apply</span></div>
    <div class="manual-actions"><button data-press="spin_flush_start_or_cancel">Spin flush · start / cancel</button><button data-press="system_flush_start_or_cancel">System flush · start / cancel</button><button data-press="run_away_flush">Run Away flush</button><button data-press="cancel_flush">Cancel flush</button><button id="uv-enable" aria-pressed="false">UV permission · —</button></div>
    <details class="led-guide"><summary>What the door LEDs mean</summary><div class="led-grid"><p><strong>Pump & tank</strong>Steady: enabled. Tank slow blink: heat requested, waiting for pump startup or blocked by pump. Fast blink: invalid selector contacts.</p><p><strong>Spin button</strong>Steady: automatic spin schedule enabled for this mode. Fast blink: spin flushing. Off in Shutdown.</p><p><strong>UV / system button</strong>Slow blink: waiting to start UV. Steady: UV enabled. Fast blink: system flush. Rapid flicker: confirmed low current.</p><p><strong>Both button LEDs</strong>Two short flashes together, then a pause: Wi-Fi disconnected. Active flushing takes priority; a UV current fault always takes priority on its own LED.</p></div><p class="small-note">Local operation continues without Wi-Fi or Home Assistant. UV current faults require installed, calibrated CT monitoring; current does not prove UV treatment performance.</p></details>
  </section>
  <details class="panel advanced" id="advanced-panel"><summary>Advanced <span id="advanced-status">Bench test & live I/O</span></summary>
  <section class="bench-panel" aria-labelledby="bench-heading">
    <div class="section-heading"><div><span class="eyebrow">Wiring & commissioning</span><h2 id="bench-heading">Bench test</h2></div><div class="bench-actions"><button id="bench-toggle" class="primary" disabled>Enter bench test</button><button id="all-off" class="danger" disabled>All outputs off</button></div></div>
    <p id="bench-help" class="help">Enter bench test to control each output independently. Normal automation and operating interlocks are bypassed during testing. Leaving test mode turns every output off.</p>
    <div id="bench-banner" class="bench-banner" hidden>Bench test active · You control the outputs. Door inputs are read-only here.</div>
    <div class="io-columns"><div><h3>Equipment outputs</h3><div class="output-list">${[1,2,3,4,5].map(outputCard).join('')}</div></div><div><h3>Door LEDs</h3><div class="output-list">${[9,10,11,12].map(outputCard).join('')}</div><p class="small-note">Output states show commands, not measured current.</p></div></div>
    <details class="spare-block"><summary>Spare outputs · OUT06–08 & OUT13–16</summary><div class="spare-outputs">${[6,7,8,13,14,15,16].map(outputCard).join('')}</div></details>
    <div class="section-heading input-heading"><h3>Live inputs</h3><span class="muted">Move a selector or press a button</span></div>
    <div class="input-grid">${[1,2,3,4,5,6].map(inputCard).join('')}</div>
    <details class="spare-block"><summary>Spare inputs · IN07–16</summary><div class="input-grid">${Array.from({length:10},(_,i)=>inputCard(i+7)).join('')}</div></details>
    <details class="activity"><summary>Input activity <span id="event-count">0 changes</span></summary><button id="clear-log" class="text-button">Clear activity</button><ol id="input-log"><li class="muted">Input changes will appear here.</li></ol></details>
  </section>
  <details class="spare-block settings"><summary>Current sensors & UV fault setup</summary><p class="help">Enable after CT installation. Set each calibration from measured amps ÷ CT signal volts, then set its running/minimum-current threshold. Zero leaves that channel unconfigured. UV monitoring only reports an electrical fault; it does not switch equipment off.</p><div class="setting-toggles"><button data-setting="ct_sampling_enabled">CT sampling · —</button><button data-setting="uv_current_monitoring">UV current monitoring · —</button></div><div class="settings-grid">${settingForms(healthSettings)}</div></details>
  </details>
  <details class="panel settings"><summary>Flush schedules & settings</summary><p class="help">Schedules run locally. Enabling an Away schedule, changing its frequency, entering Away or rebooting starts a full interval. Duration changes apply to the next run.</p><h3>Normal</h3><div class="setting-toggles"><button data-setting="automatic_spin_flushing">Normal spin · —</button><button data-setting="automatic_system_flushing">Normal system · —</button></div><div class="settings-grid">${settingForms(numberSettings.slice(0,4))}</div><h3 class="settings-heading">Away</h3><div class="setting-toggles"><button data-setting="scheduled_away_flushing">Away system · —</button><button data-setting="scheduled_away_spin_flushing">Away spin · —</button></div><div class="settings-grid">${settingForms(numberSettings.slice(4))}</div></details>
  <details class="panel diagnostics"><summary>Equipment activity</summary><p class="help">CT-based estimates since restart. Samples are taken every 10 seconds; short cycles may be missed. Missing or uncalibrated current is unknown.</p><div class="schedule-grid">${['pump','tank'].map(load=>`<div class="schedule-card"><strong>${load === 'pump' ? 'Pump' : 'Hot water tank'}</strong><p id="${load}-activity">—</p><p id="${load}-usage">—</p></div>`).join('')}</div><div class="status-strip"><span>UV electrical check <strong id="uv-electrical">—</strong></span><span>UV current <strong id="uv-current">—</strong></span></div></details>
  <details class="panel diagnostics"><summary>Sensor readings</summary><p class="help">Pressure and current sensors are not installed yet. Missing readings are expected.</p><div class="status-strip"><span>Pressure <strong id="pressure">—</strong></span><span>Pressure loop <strong id="pressure-voltage">—</strong></span><span>Input communication <strong id="input-health">—</strong></span></div></details>
  <footer>Runs on the A16 · No Home Assistant connection required</footer>
</main>`);

const $ = selector => document.querySelector(selector);
const bool = (domain, slug) => {
  const s = states.get(key(domain, slug));
  if (!s || s.value === null || s.state === 'NA') return null;
  return s.value === true || s.value === 1 || s.state === 'ON';
};
const textState = (domain, slug) => states.get(key(domain, slug))?.state ?? '—';
const seconds = slug => {
  const data = states.get(key('sensor', slug));
  const value = parseFloat(data?.value ?? data?.state);
  return Number.isFinite(value) ? Math.max(0, Math.ceil(value)) : null;
};
const duration = value => `${Math.floor(value / 60)}:${String(value % 60).padStart(2, '0')}`;
const numeric = (domain, slug) => {
  const data = states.get(key(domain,slug));
  const value = parseFloat(data?.value ?? data?.state);
  return Number.isFinite(value) ? value : null;
};
const cabinTime = new Intl.DateTimeFormat('en-US',{timeZone:'America/Chicago',weekday:'short',hour:'numeric',minute:'2-digit',timeZoneName:'short'});
function renderAwayPlan(online, testing, mode) {
  $('#away-plan').hidden = testing || mode !== 'Away';
  for (const kind of ['system','spin']) {
    const frequency = numeric('number',kind === 'system' ? 'away_flushes_per_day' : 'away_spin_flushes_per_day');
    const run = numeric('number',kind === 'system' ? 'away_flush_duration' : 'away_spin_flush_duration');
    $(`#away-${kind}-settings`).textContent = !online || !frequency || run === null ? '—' : `${frequency} / day · every ${24 / frequency} hours · run ${run < 60 ? `${run} seconds` : `${Math.floor(run / 60)}m ${run % 60}s`}`;
    const status = textState('text_sensor',`away_${kind}_schedule`);
    const sample = states.get(key('sensor',`away_${kind}_next_flush`));
    const remaining = seconds(`away_${kind}_next_flush`);
    let next = status;
    if (status === 'Scheduled' && remaining !== null) {
      const age = Math.max(0,(performance.now() - sample.receivedAt) / 1000);
      const left = Math.max(0,remaining - age);
      const minutes = Math.ceil(left / 60);
      next = left > 0 ? `Next ≈ ${cabinTime.format(new Date(Date.now() + left * 1000))} · in ${minutes >= 60 ? `${Math.floor(minutes / 60)}h ` : ''}${minutes % 60}m` : 'Due now · waiting for controller';
    }
    $(`#away-${kind}-next`).textContent = online ? next : 'Waiting for live controller data';
  }
}
const bench = () => bool('switch', 'bench_test') === true;
function message(text, error = false) {
  $('#message').hidden = !text;
  $('#message').textContent = text;
  $('#message').className = error ? 'error' : '';
}
function scheduleRender() {
  if (!renderQueued) { renderQueued = true; requestAnimationFrame(() => { renderQueued = false; render(); }); }
}
function render() {
  const online = connected && Date.now() - lastMessage < 20000;
  const testing = bench();
  const mode = textState('select', 'system_mode');
  const remote = bool('binary_sensor', 'remote_mode_available') === true;
  const knownBench = bool('switch', 'bench_test') !== null;
  $('#connection').textContent = online ? 'Connected to A16' : 'Disconnected · reconnecting';
  $('#connection').className = `badge ${online ? 'online' : 'offline'}`;
  $('#wifi-state').textContent = !online ? 'Unknown' : bool('binary_sensor','wi-fi_connected') === null ? '—' : bool('binary_sensor','wi-fi_connected') ? 'Connected' : 'Disconnected';
  $('#ha-state').textContent = !online ? 'Unknown' : bool('binary_sensor','home_assistant_connected') === null ? '—' : bool('binary_sensor','home_assistant_connected') ? 'Connected' : 'Not connected';
  renderAwayPlan(online, testing, mode);
  $('#mode-heading').textContent = online ? (testing ? 'Bench test' : mode) : 'Disconnected';
  document.querySelectorAll('[data-mode]').forEach(button => {
    button.disabled = !online || busy || (button.dataset.mode !== 'Shutdown' && !remote);
    button.classList.toggle('selected', !testing && mode === button.dataset.mode);
  });
  $('#mode-help').textContent = !online ? 'Waiting for live controller data. Controls are disabled.' : testing ? 'Bench test is active under Advanced. Normal interlocks are bypassed. Shutdown ends testing and turns all outputs off.' : remote ? 'Both selectors are in Auto. Choose a mode here; moving a selector back to On or Auto returns to Normal.' : 'Move both selectors to Auto for Normal and web mode selection. Both Off selects Shutdown. Pump Auto with tank Off gives cold water only.';
  $('#pump-selector').textContent = online ? textState('text_sensor', 'pump_selector') : '—';
  $('#tank-selector').textContent = online ? textState('text_sensor', 'tank_selector') : '—';
  const up = seconds('controller_uptime');
  $('#uptime').textContent = !online || up === null ? '—' : up < 3600 ? `${Math.floor(up / 60)} min` : `${Math.floor(up / 3600)}h ${Math.floor(up % 3600 / 60)}m`;
  const flushStatus = textState('text_sensor', 'flush_status');
  $('#flush-status').textContent = !online ? '—' : testing ? 'Bench control' : flushStatus;
  const pumpOn = bool('binary_sensor', 'pump_enabled') === true;
  for (const name of ['pump','tank','uv']) {
    const value = bool('binary_sensor', `${name}_enabled`);
    $(`#${name}-state`).textContent = !online || value === null ? '—' : value ? 'Enabled' : 'Off';
  }
  if (!testing && online) {
    $('#tank-state').textContent = textState('text_sensor', 'tank_status');
    $('#uv-state').textContent = textState('text_sensor', 'uv_status');
  }
  const pending = online && !testing && bool('binary_sensor', 'uv_start_pending') === true;
  const blocked = online && !testing && bool('binary_sensor', 'tank_blocked_by_pump') === true;
  const tankPending = online && !testing && textState('text_sensor', 'tank_status') === 'Waiting: pump startup';
  const flushing = online && !testing && ['spin_flush_active', 'system_flush_active'].some(slug => bool('binary_sensor', slug) === true);
  const uvWait = seconds('uv_start_remaining');
  const flushWait = seconds('flush_remaining');
  $('#uv-countdown').hidden = !pending;
  $('#uv-countdown').textContent = uvWait === null ? '…' : duration(uvWait);
  if (pending) $('#uv-state').textContent = 'Waiting to start';
  if (blocked) $('#tank-state').textContent = 'Blocked by pump';
  $('#flush-countdown').hidden = !flushing || flushWait === null;
  $('#flush-countdown').textContent = flushWait === null ? '' : duration(flushWait);
  for (const name of ['pump','tank','uv']) $(`#${name}-card`).classList.toggle('enabled', online && bool('binary_sensor', `${name}_enabled`) === true);
  $('#uv-card').classList.toggle('waiting', pending);
  $('#tank-card').classList.toggle('waiting', blocked || tankPending);
  $('#flush-card').classList.toggle('enabled', flushing);
  const unavailable = !online ? 'Waiting for live controller data' : testing ? 'Direct output control · Advanced' : null;
  $('#pump-detail').textContent = unavailable ?? (pumpOn ? 'Pump selector LED · steady' : 'Pump selector LED · off');
  $('#tank-detail').textContent = unavailable ?? (blocked ? 'Pump must be enabled · slow blink' : tankPending ? 'Five-second startup delay · slow blink' : bool('binary_sensor', 'tank_enabled') ? 'Tank selector LED · steady' : 'No heat requested');
  $('#uv-detail').textContent = unavailable ?? (pending ? 'Minimum-off protection · slow blink' : bool('binary_sensor', 'uv_enabled') ? 'UV button LED · steady unless flushing' : bool('switch', 'uv_enable') === false ? 'UV permission is off' : 'Waiting for an operating request');
  $('#flush-detail').textContent = unavailable ?? (flushing ? 'Time remaining · valve commanded open' : flushStatus === 'Idle' ? 'No flush active' : 'Sequence controlled by the A16');
  const uvFault = online && !testing && bool('binary_sensor','uv_low_current_fault') === true;
  const faults = [];
  if (bool('binary_sensor','control_input_communication_fault')) faults.push('Control input communication fault · equipment is inhibited.');
  if (bool('binary_sensor','control_output_communication_fault')) faults.push('Control output communication fault · check the output hardware.');
  if (bool('binary_sensor','selector_fault')) faults.push('Invalid selector contacts · check the selector showing Invalid.');
  if (uvFault) faults.push('UV low current · check the bulb, ballast and current reading.');
  $('#control-fault').hidden = !online || testing || !faults.length;
  $('#control-fault').textContent = faults.join(' ');
  $('#uv-card').classList.toggle('fault',uvFault);
  if (uvFault) { $('#uv-state').textContent = 'Low current fault'; $('#uv-detail').textContent = 'UV button LED · rapid flicker'; }
  $('#advanced-status').textContent = testing ? 'Bench test active' : 'Bench test & live I/O';
  $('#advanced-panel').classList.toggle('testing', testing);
  if (testing && !lastTesting) $('#advanced-panel').open = true;
  lastTesting = testing;
  document.querySelectorAll('[data-press]').forEach(button => {
    const name = button.dataset.press;
    const awayPump = mode === 'Away' && ['On','Auto'].includes(textState('text_sensor','pump_selector'));
    const permitted = name === 'cancel_flush' || (name === 'run_away_flush' ? awayPump : name === 'spin_flush_start_or_cancel' && awayPump || pumpOn && mode !== 'Shutdown');
    button.disabled = !online || busy || testing || !permitted;
  });
  $('#uv-enable').disabled = !online || busy || testing || bool('switch','uv_enable') === null;
  $('#uv-enable').textContent = `UV permission · ${bool('switch','uv_enable') ? 'On' : 'Off'}`;
  $('#uv-enable').setAttribute('aria-pressed', String(bool('switch','uv_enable') === true));
  $('#bench-toggle').disabled = !online || busy || !knownBench;
  $('#bench-toggle').textContent = testing ? 'Finish test · all off' : 'Enter bench test';
  $('#all-off').disabled = !online;
  $('#bench-banner').hidden = !testing;
  $('.bench-panel').classList.toggle('testing', testing);
  document.querySelectorAll('[data-output]').forEach(button => {
    const value = bool('switch', `test_out${pad(button.dataset.output)}`);
    button.disabled = !online || busy || !testing || value === null;
    button.textContent = !online || value === null ? '—' : value ? 'On' : 'Off';
    button.setAttribute('aria-pressed', String(online && value === true));
  });
  document.querySelectorAll('[data-input]').forEach(card => {
    const value = online ? bool('binary_sensor', card.dataset.input) : null;
    card.classList.toggle('closed', value === true);
    card.querySelector('.input-value').textContent = value === null ? '—' : value ? 'Closed' : 'Open';
  });
  document.querySelectorAll('[data-setting]').forEach(button => {
    const labels = {automatic_spin_flushing:'Normal spin',automatic_system_flushing:'Normal system',scheduled_away_flushing:'Away system',scheduled_away_spin_flushing:'Away spin',ct_sampling_enabled:'CT sampling',uv_current_monitoring:'UV current monitoring'};
    button.disabled = !online || busy || bool('switch',button.dataset.setting) === null;
    button.textContent = `${labels[button.dataset.setting]} · ${bool('switch',button.dataset.setting) ? 'On' : 'Off'}`;
  });
  document.querySelectorAll('[data-number]').forEach(form => {
    const input = form.querySelector('input');
    const data = states.get(key('number',form.dataset.number));
    if (data && document.activeElement !== input && !input.dataset.dirty) input.value = data.value;
    input.disabled = !online || !data;
    form.querySelector('button').disabled = !online || busy || !data;
  });
  $('#pressure').textContent = online ? textState('sensor','water_pressure') : '—';
  $('#pressure-voltage').textContent = online ? textState('sensor','pressure_loop_voltage') : '—';
  $('#input-health').textContent = !online ? '—' : bool('binary_sensor','control_input_communication_fault') === true ? 'Check connection' : 'OK';
  for (const load of ['pump','tank']) {
    const activity = textState('text_sensor',`${load}_activity`);
    const current = numeric('sensor',`${load}_measured_current`);
    const runtime = seconds(`${load}_observed_runtime`);
    const starts = numeric('sensor',`${load}_detected_starts`);
    $(`#${load}-activity`).textContent = !online ? '—' : `${activity}${current === null ? '' : ` · ${current.toFixed(2)} A`}`;
    $(`#${load}-usage`).textContent = !online || activity.startsWith('Unknown') || runtime === null || starts === null ? 'Waiting for installed, calibrated CT monitoring' : `${Math.floor(runtime / 3600)}h ${Math.floor(runtime % 3600 / 60)}m observed · ${starts} detected starts`;
  }
  $('#uv-electrical').textContent = online ? textState('text_sensor','uv_electrical_status') : '—';
  $('#uv-current').textContent = online ? textState('sensor','uv_measured_current') : '—';
}

async function command(domain, slug, action, params = {}, expected) {
  busy = true; message(''); render();
  try {
    // ESPHome 2026.7 streams legacy IDs but routes REST requests by entity name.
    const entityName = states.get(key(domain, slug))?.name;
    if (!entityName) throw new Error('Waiting for this control to finish loading. Try again shortly.');
    const path = `/${domain}/${encodeURIComponent(entityName)}`;
    const query = new URLSearchParams(params);
    const response = await fetch(`${path}/${action}${query.size ? `?${query}` : ''}`, {method:'POST',signal:AbortSignal.timeout(5000)});
    if (!response.ok) throw new Error(`Controller returned ${response.status}`);
    if (domain !== 'button') {
      await new Promise(resolve => setTimeout(resolve,150));
      const read = await fetch(path, {signal:AbortSignal.timeout(5000)});
      if (!read.ok) throw new Error('Could not confirm the controller state');
      const data = await read.json(); acceptState(data);
      if (expected !== undefined) {
        const actual = typeof expected === 'boolean' ? bool(domain,slug) : (data.value ?? data.state);
        const matches = typeof expected === 'number' && typeof actual === 'number'
          ? Math.abs(actual - expected) <= Math.max(1, Math.abs(expected)) * 1e-6
          : actual === expected;
        if (!matches) throw new Error('Request was not applied. Check the selector positions and test mode.');
      }
    }
  } catch (error) { message(error.message || 'Connection lost. Check the controller before retrying.',true); }
  finally { busy = false; render(); }
}
document.querySelectorAll('[data-mode]').forEach(button => button.addEventListener('click',()=>command('select','system_mode','set',{option:button.dataset.mode},button.dataset.mode)));
$('#bench-toggle').addEventListener('click',()=>command('switch','bench_test',bench()?'turn_off':'turn_on',{},!bench()));
$('#all-off').addEventListener('click',()=>command('button','all_outputs_off','press'));
document.querySelectorAll('[data-output]').forEach(button=>button.addEventListener('click',()=>{
  const slug=`test_out${pad(button.dataset.output)}`; const on=bool('switch',slug) !== true;
  command('switch',slug,on?'turn_on':'turn_off',{},on);
}));
document.querySelectorAll('[data-press]').forEach(button=>button.addEventListener('click',()=>command('button',button.dataset.press,'press')));
function toggleSetting(slug) { const on=bool('switch',slug)!==true; return command('switch',slug,on?'turn_on':'turn_off',{},on); }
$('#uv-enable').addEventListener('click',()=>toggleSetting('uv_enable'));
document.querySelectorAll('[data-setting]').forEach(button=>button.addEventListener('click',()=>toggleSetting(button.dataset.setting)));
document.querySelectorAll('[data-number]').forEach(form=>{
  const input=form.querySelector('input');
  input.addEventListener('input',()=>input.dataset.dirty='true');
  form.addEventListener('submit',async event=>{event.preventDefault(); if(form.reportValidity()){await command('number',form.dataset.number,'set',{value:input.value},Number(input.value)); delete input.dataset.dirty;render();}});
});
let events = 0;
$('#clear-log').addEventListener('click',()=>{$('#input-log').replaceChildren();events=0;$('#event-count').textContent='0 changes';});
function acceptState(data) {
  if (!data.id) return;
  const old=states.get(data.id);
  states.set(data.id,{...old,...data,receivedAt:performance.now()});
  if (/^binary_sensor-in\d\d_/.test(data.id) && old && old.state !== data.state) {
    if (!events) $('#input-log').replaceChildren();
    const li=document.createElement('li');
    li.textContent=`${new Date().toLocaleTimeString()} · ${(data.name || old.name || data.id.replace('binary_sensor-','')).replaceAll('_',' ')} · ${data.state === 'ON' ? 'Closed' : 'Open'}`;
    $('#input-log').prepend(li); while($('#input-log').children.length>30) $('#input-log').lastChild.remove();
    $('#event-count').textContent=`${++events} changes`;
  }
  scheduleRender();
}
const stream = new EventSource('/events');
stream.onopen=()=>{states.clear();connected=true;lastMessage=Date.now();scheduleRender();};
stream.onerror=()=>{connected=false;scheduleRender();};
stream.addEventListener('state',event=>{lastMessage=Date.now();try{acceptState(JSON.parse(event.data));}catch{message('Could not read a controller update.',true);}});
stream.addEventListener('ping',()=>{lastMessage=Date.now();scheduleRender();});
setInterval(scheduleRender,2000);
render();
