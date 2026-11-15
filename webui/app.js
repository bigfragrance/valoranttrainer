'use strict';
const $ = id => document.getElementById(id);
const all = selector => [...document.querySelectorAll(selector)];
const labels = {practice: '架枪练习', loadout: '武器与皮肤', deathmatch: '人机乱斗'};
const descriptions = {practice: '在地图上设置拉出路线，专注每一次预瞄和首发。', loadout: '从当前资源中选择武器外观，原动画与效果随换装生效。', deathmatch: '熟悉地图、练习转枪，与水平各异的人机进行个人乱斗。'};
const phaseLabels = {stopped: '未启动', loading: '正在加载', running: '训练中', stopping: '正在结束', error: '训练器已退出'};
let config, runtime = {}, catalog = {maps: [], weapons: [], effects: [], styles: {}}, view = 'practice';
let skins = [], selectedWeapon = null, selectedSkin = null, currentMap = null, imageReady = false;
let draft = {}, draftId = null, pointStep = 'start', floorDirty = false, mutation = Promise.resolve();
let editorBounds = {x: 0, y: 0, span: 1};
let pollBusy = false, saveTimer, savePatch = {}, toastTimer, selectedSkinFor = null;
let pendingWeapon = null;
let humanBotsPanel = null;
let skinLoadSequence = 0, skinPreviewSequence = 0;

function node(tag, text, className) { const e = document.createElement(tag); if (text != null) e.textContent = text; if (className) e.className = className; return e; }
function toast(message, error = false) { const e = $('toast'); e.textContent = message; e.classList.toggle('error', error); e.hidden = false; clearTimeout(toastTimer); toastTimer = setTimeout(() => e.hidden = true, error ? 6500 : 2800); }
async function api(path, body, method = 'POST') {
  const response = await fetch(path, body == null ? {cache: 'no-store'} : {method, headers: {'Content-Type': 'application/json'}, body: JSON.stringify(body)});
  const result = await response.json();
  if (!response.ok) { if (result.config) { config = result.config; renderSettings(); } throw new Error(result.error || `请求失败 (${response.status})`); }
  return result;
}
function accept(result) { if (result.config) { config = result.config; renderSettings(); } const status = result.runtime?.status; $('save-status').textContent = status === 'saved' ? '已保存 · 启动后生效' : status === 'pending' ? '已保存 · 客户端正在应用' : status === 'loading' ? '正在切换场景' : '已保存并提交客户端'; return result; }
function enqueue(fn) { const task = mutation.then(fn); mutation = task.catch(error => { toast(error.message, true); $('save-status').textContent = '应用失败，请查看提示'; }); return mutation; }
function save(patch) { return enqueue(async () => accept(await api('/api/config', {patch: typeof patch === 'function' ? patch(config) : patch, revision: config.revision}, 'PATCH'))); }
function debounceSave(patch) {
  for (const [key, value] of Object.entries(patch)) savePatch[key] = value && typeof value === 'object' && !Array.isArray(value) ? {...savePatch[key], ...value} : value;
  clearTimeout(saveTimer); $('save-status').textContent = '正在保存…';
  saveTimer = setTimeout(() => { const pending = savePatch; savePatch = {}; save(pending); }, 350);
}
function value(e, v) { if (document.activeElement !== e) e.value = v; }
function checkbox(selector, checked) { all(selector).forEach(e => e.checked = checked); }
function setView(next) {
  view = next; all('.view').forEach(e => e.classList.toggle('active', e.id === `view-${next}`));
  all('[data-view]').forEach(e => { const active = e.dataset.view === next; e.classList.toggle('active', active); if (active) e.setAttribute('aria-current', 'page'); else e.removeAttribute('aria-current'); });
  $('page-title').textContent = labels[next]; $('page-description').textContent = descriptions[next];
  if (config && next !== 'loadout' && config.mode !== next) {
    const patch = {mode: next};
    if (next === 'deathmatch' && mapById(config.map_id)?.kind === 'duel') patch.map_id = catalog.maps.find(m => m.kind !== 'duel')?.id || config.map_id;
    save(patch);
  }
  if (next === 'loadout' && (!skins.length || selectedSkinFor !== selectedWeapon)) loadSkins();
  drawMap();
}
function mapById(id) { return catalog.maps.find(m => m.id === id); }
function hasCurrentMapState() { return runtime.status === 'running' && currentMap?.id === config?.map_id && runtime.game?.map_id === currentMap?.id; }
function liveWeaponId() { const weapon = runtime.game?.weapon; return runtime.status === 'running' && weapon?.ready && catalog.weapons.some(w => w.id === weapon.id) ? weapon.id : null; }
function syncWeaponSelection() {
  const actual = liveWeaponId();
  if (pendingWeapon?.submitted && actual === pendingWeapon.id) pendingWeapon = null;
  if (pendingWeapon?.submitted && Date.now() - pendingWeapon.at > 30000) { pendingWeapon = null; toast('武器切换尚未确认，当前显示客户端实际装备。', true); }
  if (pendingWeapon || !actual || selectedWeapon === actual) return;
  selectedWeapon = actual; selectedSkin = null; renderWeapons(); if (view === 'loadout') loadSkins();
}
function displayMap(m) { return m?.label_zh || m?.label || m?.label_en || m?.id || '—'; }
function transformFor(m) { return m?.world_to_image || m?.minimap?.world_to_image; }
function toUV(point) {
  const t = transformFor(currentMap); if (!t || !point) return null;
  let uv;
  if (Array.isArray(t.matrix)) { const a = t.matrix; uv = [a[0][0] * point[0] + a[0][1] * point[1] + a[0][2], a[1][0] * point[0] + a[1][1] * point[1] + a[1][2]]; }
  else uv = [point[1] * t.x_multiplier + t.x_scalar_to_add, point[0] * t.y_multiplier + t.y_scalar_to_add];
  return [(uv[0] - editorBounds.x) / editorBounds.span, (uv[1] - editorBounds.y) / editorBounds.span];
}
function fitMapImage(image) {
  const canvas = document.createElement('canvas'); canvas.width = image.naturalWidth; canvas.height = image.naturalHeight;
  const ctx = canvas.getContext('2d', {willReadFrequently: true}); ctx.drawImage(image, 0, 0);
  const pixels = ctx.getImageData(0, 0, canvas.width, canvas.height).data; let x0 = canvas.width, y0 = canvas.height, x1 = 0, y1 = 0;
  for (let y = 0; y < canvas.height; y++) for (let x = 0; x < canvas.width; x++) if (pixels[(y * canvas.width + x) * 4 + 3] > 32) { x0 = Math.min(x0, x); x1 = Math.max(x1, x); y0 = Math.min(y0, y); y1 = Math.max(y1, y); }
  const span = Math.min(1, Math.max((x1 - x0) / canvas.width, (y1 - y0) / canvas.height) * 1.14);
  editorBounds = span > 0 ? {x: Math.max(0, Math.min(1 - span, (x0 + x1) / (2 * canvas.width) - span / 2)), y: Math.max(0, Math.min(1 - span, (y0 + y1) / (2 * canvas.height) - span / 2)), span} : {x: 0, y: 0, span: 1};
  image.style.width = image.style.height = `${100 / editorBounds.span}%`;
  image.style.left = `${-editorBounds.x / editorBounds.span * 100}%`; image.style.top = `${-editorBounds.y / editorBounds.span * 100}%`;
}
function mapStateText(m) {
  const v = m?.validation || {};
  if (runtime.backend === 'local-server-candidate' || v.runtime_source === 'local-server-map-validation-v1') {
    const check = m?.local_server_validation?.[config?.mode || 'practice'];
    if (check?.status === 'passed') return '本地服务器 · 当前地图与模式检查已通过';
    if (check?.status === 'failed') return '本地服务器 · 当前地图与模式检查未通过';
    if (check?.status === 'unsupported') return '本地服务器 · 此地图不支持当前模式';
    return '本地服务器 · 当前地图与模式尚未验证';
  }
  const state = typeof v.runtime === 'string' ? v.runtime : v.runtime?.status || v.runtime_status;
  if (state === 'verified' || state === 'passed' || state === 'playable') return '客户端加载已验证';
  if (state === 'failed' || state === 'blocked') return `加载检查未通过${v.reason ? `：${v.reason}` : ''}`;
  return '已找到原地图资源 · 客户端加载待验证';
}
function fillMaps() {
  for (const id of ['practice-map', 'deathmatch-map']) {
    const e = $(id); e.replaceChildren();
    for (const m of catalog.maps.filter(m => id === 'practice-map' || m.kind !== 'duel')) { const o = node('option', displayMap(m)); o.value = m.id; e.append(o); }
  }
}
function chooseMap() {
  const m = mapById(config.map_id); if (!m) return;
  $('live-map').textContent = displayMap(m); value($('practice-map'), m.id); value($('deathmatch-map'), m.id);
  $('map-validation').textContent = mapStateText(m);
  if (currentMap?.id === m.id) return;
  currentMap = m; draft = {}; draftId = null; floorDirty = false; imageReady = false; editorBounds = {x: 0, y: 0, span: 1};
  $('floor-height').value = 0; $('use-position').disabled = !hasCurrentMapState();
  const image = $('map-image'); image.hidden = true;
  $('map-placeholder').hidden = false;
  $('map-placeholder').querySelector('strong').textContent = '正在读取原小地图';
  $('map-placeholder').querySelector('p').textContent = '依次选择起点、露出点和撤回点。';
  image.onload = () => { imageReady = true; fitMapImage(image); image.hidden = false; $('map-placeholder').hidden = Boolean(transformFor(currentMap)); if (!transformFor(currentMap)) { $('map-placeholder').querySelector('strong').textContent = '地图坐标待验证'; $('map-placeholder').querySelector('p').textContent = '可先从游戏中取得三个真实位置。'; } drawMap(); };
  image.onerror = () => { imageReady = false; $('map-placeholder').hidden = false; $('map-placeholder').querySelector('strong').textContent = '原小地图尚不可用'; $('map-placeholder').querySelector('p').textContent = '可以在游戏中移动到指定位置，再点击“取玩家位置”。'; drawMap(); };
  image.src = `/api/maps/${encodeURIComponent(m.id)}/image`;
  setStep('start'); renderRoutes(); drawMap();
}
function renderSettings() {
  if (!config) return;
  renderHeroSelection();
  chooseMap();
  value($('peek-rate'), config.practice.peeks_per_minute); $('peek-rate-value').value = config.practice.peeks_per_minute;
  $('return-fire').checked = config.practice.return_fire; $('pause-bots').checked = config.paused;
  checkbox('.ammo-toggle', config.infinite_ammo); checkbox('.invincible-toggle', config.invincible);
  value($('streak-seconds'), config.streak_seconds); value($('respawn-seconds'), config.respawn_seconds);
  const maximumPlayers = catalog.limits?.max_players || 30;
  $('total-players').max = maximumPlayers;
  $('maximum-players-label').textContent = `${maximumPlayers} 人`;
  const selectedHero = catalog.heroes?.find(hero => hero.id === config.local_hero_id);
  const fixedRoster = config.human_bots?.bot_roster;
  const fixed = Array.isArray(fixedRoster);
  $('hero-pool-summary').textContent = fixed ? `固定名单 ${fixedRoster.length} 位人机，加上你共 ${fixedRoster.length + 1} 人；在下方名单添加、删除或改英雄，结构修改需手动重启当前对局。` : `每局英雄不重复。你使用 ${selectedHero?.name || 'Jett'}，机器人从其余 ${maximumPlayers - 1} 位英雄随机选取，死亡后保持本局英雄。`;
  $('total-players').disabled = fixed;
  value($('total-players'), fixed ? fixedRoster.length + 1 : config.total_players); $('total-players-value').value = fixed ? fixedRoster.length + 1 : config.total_players;
  all('#style-options button').forEach(e => { e.classList.toggle('selected', e.dataset.style === config.practice.style); e.setAttribute('aria-pressed', String(e.dataset.style === config.practice.style)); });
  for (const spec of difficultySpecs) { value($(spec.key), config.bots[spec.key]); value($(spec.spread), config.bots[spec.spread]); $(spec.key + '-value').textContent = spec.format(config.bots[spec.key]); }
  const usesHumanBots = config.human_bots?.enabled === true && config.mode === 'deathmatch';
  for (const spec of difficultySpecs) { $(spec.key).disabled = usesHumanBots; $(spec.spread).disabled = usesHumanBots; }
  $('legacy-bots-status').hidden = !usesHumanBots;
  const desired = pendingWeapon?.id || liveWeaponId() || config.weapon_id || catalog.weapons.find(w => w.id === '9c82e19d457502001a813eacf00cf872')?.id || catalog.weapons[0]?.id;
  if (selectedWeapon !== desired) { selectedWeapon = desired; renderWeapons(); if (view === 'loadout') loadSkins(); }
  renderRoutes(); if (!document.activeElement?.closest('#effect-list')) renderEffects();
  humanBotsPanel?.refresh();
}
function fillHeroes() {
  const select = $('local-hero-select'); select.replaceChildren();
  for (const hero of catalog.heroes || []) { const option = node('option', hero.name); option.value = hero.id; select.append(option); }
  if (!select.children.length) { const option = node('option', '捷风 · Jett'); option.value = catalog.capabilities?.local_hero_selection?.default_hero_id || ''; select.append(option); }
}
function renderHeroSelection() {
  const capability = catalog.capabilities?.local_hero_selection || {}, select = $('local-hero-select');
  value(select, config.local_hero_id || capability.default_hero_id || '');
  const loading = ['loading', 'stopping'].includes(runtime.status);
  select.disabled = capability.available !== true || loading;
  $('hero-selection-status').textContent = loading ? '正在切换场景，请等待本局准备完成。' : capability.reason || '当前版本固定使用捷风。';
}
const difficultySpecs = [
  {key: 'reaction_ms', spread: 'reaction_spread_ms', label: '反应速度', min: 60, max: 1500, step: 10, spreadMax: 1000, spreadStep: 10, unit: 'ms', format: v => `${Math.round(v)} ms`, left: '反应快', right: '反应慢'},
  {key: 'aim_speed', spread: 'aim_spread', label: '拉枪速度', min: 20, max: 1080, step: 10, spreadMax: 800, spreadStep: 10, unit: '°/s', format: v => `${Math.round(v)} °/s`, left: '缓慢调整', right: '快速转枪'},
  {key: 'accuracy', spread: 'accuracy_spread', label: '准度', min: .02, max: 1, step: .01, spreadMax: .7, spreadStep: .01, unit: '', format: v => `${Math.round(v * 100)}%`, left: '容错大', right: '瞄准精确'},
  {key: 'movement', spread: 'movement_spread', label: '身法', min: .05, max: 1, step: .01, spreadMax: .7, spreadStep: .01, unit: '', format: v => `${Math.round(v * 100)}%`, left: '基础走位', right: '更多变向与急停'},
];
function buildControls() {
  for (const [key, label] of Object.entries(catalog.styles)) { const b = node('button', label); b.dataset.style = key; b.addEventListener('click', () => save({practice: {style: key}})); $('style-options').append(b); }
  for (const s of difficultySpecs) {
    const row = node('div', null, 'difficulty-row'), header = node('header'), title = node('h3', s.label), output = node('output'); output.id = s.key + '-value'; header.append(title, output);
    const range = node('input'); Object.assign(range, {type: 'range', id: s.key, min: s.min, max: s.max, step: s.step}); range.setAttribute('aria-label', s.label);
    range.addEventListener('input', () => { output.textContent = s.format(+range.value); debounceSave({bots: {[s.key]: +range.value}}); });
    const captions = node('div', null, 'range-caption'); captions.append(node('span', s.left), node('span', s.right));
    const spreadLabel = node('label', null, 'spread-control'); spreadLabel.append(node('span', '个体浮动 ±'));
    const spread = node('input'); Object.assign(spread, {type: 'number', id: s.spread, min: .0, max: s.spreadMax, step: s.spreadStep}); spread.setAttribute('aria-label', s.label + '个体浮动'); spread.addEventListener('change', () => save({bots: {[s.spread]: +spread.value}})); spreadLabel.append(spread, node('span', s.unit));
    row.append(header, range, captions, spreadLabel); $('difficulty-controls').append(row);
  }
}
function setStep(step) { pointStep = step; all('[data-point]').forEach(e => e.classList.toggle('selected', e.dataset.point === step)); }
function drawMap() {
  const c = $('map-overlay'), ctx = c.getContext('2d'); ctx.clearRect(0, 0, 1024, 1024);
  if (!currentMap) return;
  const colors = {start: '#74d8c6', expose: '#ffbb7c', retreat: '#b6a2ed'};
  const routes = config?.routes[currentMap.id] || [];
  function drawRoute(r, alpha, dashed) {
    const points = ['start', 'expose', 'retreat'].map(k => toUV(r[k]));
    ctx.globalAlpha = alpha; ctx.strokeStyle = '#8ea6b9'; ctx.lineWidth = 3; ctx.setLineDash(dashed ? [10, 8] : []); ctx.beginPath(); let began = false;
    for (const p of points) { if (!p || !p.every(Number.isFinite)) continue; if (!began) { ctx.moveTo(p[0] * 1024, p[1] * 1024); began = true; } else ctx.lineTo(p[0] * 1024, p[1] * 1024); } ctx.stroke(); ctx.setLineDash([]);
    ['start', 'expose', 'retreat'].forEach((key, i) => { const p = points[i]; if (!p || !p.every(Number.isFinite)) return; const x = p[0] * 1024, y = p[1] * 1024; ctx.beginPath(); ctx.arc(x, y, dashed ? 10 : 13, 0, Math.PI * 2); ctx.fillStyle = colors[key]; ctx.fill(); ctx.strokeStyle = '#111d29'; ctx.lineWidth = 4; ctx.stroke(); if (!dashed) { ctx.fillStyle = '#0d1720'; ctx.font = 'bold 13px sans-serif'; ctx.textAlign = 'center'; ctx.textBaseline = 'middle'; ctx.fillText(String(i + 1), x, y + .5); } });
  }
  for (const route of routes) if (route.enabled && route.id !== draftId) drawRoute(route, .45, true);
  drawRoute(draft, 1, false);
  for (const p of hasCurrentMapState() ? runtime.game?.players || [] : []) { const uv = toUV(p.feet || p.position); if (!uv || !uv.every(Number.isFinite)) continue; ctx.globalAlpha = p.alive === false ? .35 : 1; ctx.beginPath(); ctx.arc(uv[0] * 1024, uv[1] * 1024, p.bot ? 6 : 9, 0, Math.PI * 2); ctx.fillStyle = p.bot ? '#e2dc67' : '#dceefa'; ctx.fill(); ctx.lineWidth = 3; ctx.strokeStyle = '#0d1720'; ctx.stroke(); }
  ctx.globalAlpha = 1;
  $('draft-coordinates').textContent = ['start', 'expose', 'retreat'].map((k, i) => `${i + 1} ${draft[k] ? draft[k].map(v => Math.round(v)).join(', ') : '未设置'}`).join('    /    ');
}
async function mapPoint(event) {
  if (!imageReady || !transformFor(currentMap)) return toast('此地图坐标尚未准备好，可以先取游戏中的玩家位置。', true);
  const rect = $('map-surface').getBoundingClientRect(), uv = [editorBounds.x + (event.clientX - rect.left) / rect.width * editorBounds.span, editorBounds.y + (event.clientY - rect.top) / rect.height * editorBounds.span];
  const mapId = currentMap.id;
  try { const result = await api('/api/map-point', {map_id: mapId, uv, z: +$('floor-height').value}); if (currentMap?.id === mapId) setDraftPoint(result.position); }
  catch (error) { toast(error.message, true); }
}
function setDraftPoint(point) { draft[pointStep] = point; if (pointStep === 'start') setStep('expose'); else if (pointStep === 'expose') setStep('retreat'); drawMap(); }
function updateRoutes(routes) {
  const ids = new Set(routes.map(r => r.id));
  const waves = (config.practice.waves?.[currentMap.id] || []).map(w => w.filter(e => ids.has(e.route_id))).filter(w => w.length);
  return save({routes: {...config.routes, [currentMap.id]: routes}, practice: {waves: {...config.practice.waves, [currentMap.id]: waves}}});
}
function enemyControls(enemy, update) {
  const box = node('div', null, 'enemy-controls');
  const style = node('select');
  for (const [key, text] of Object.entries(catalog.route_styles || {})) {
    const option = node('option', text); option.value = key; style.append(option);
  }
  style.value = enemy.style; style.setAttribute('aria-label', '拉出方式');
  style.addEventListener('change', () => update({style: style.value}));
  const fireLabel = node('label', '开枪 '), fire = node('input'); fire.type = 'checkbox'; fire.checked = enemy.fire ?? config.practice.return_fire;
  fire.addEventListener('change', () => update({fire: fire.checked})); fireLabel.append(fire);
  box.append(style, fireLabel);
  for (const [key, label] of [['delay_min', '最早拉出（秒）'], ['delay_max', '最晚拉出（秒）']]) {
    const wrapper = node('label', label), input = node('input');
    Object.assign(input, {type: 'number', min: '0', max: '120', step: '.1', value: enemy[key] ?? (key === 'delay_min' ? .1 : .4)});
    input.addEventListener('change', () => update({[key]: +input.value})); wrapper.append(input); box.append(wrapper);
  }
  return box;
}
function renderHoldPlan(routes) {
  const controls = $('hold-plan-controls'); controls.replaceChildren();
  const mode = node('select'); mode.setAttribute('aria-label', '架点模式');
  for (const [key, text] of [['continuous', '持续模式'], ['waves', '波次模式']]) { const option = node('option', text); option.value = key; mode.append(option); }
  mode.value = config.practice.mode || 'continuous'; mode.addEventListener('change', () => save({practice: {mode: mode.value}}));
  const restart = node('button', '重新开始练习');
  restart.addEventListener('click', () => save({practice: {restart_serial: (config.practice.restart_serial || 0) + 1}}));
  controls.append(mode, restart);
  if (mode.value === 'continuous') {
    const label = node('label', '总体最小拉出间隔（秒） '), interval = node('input');
    Object.assign(interval, {type: 'number', min: '0', max: '120', step: '.1', value: config.practice.minimum_interval ?? 1});
    interval.addEventListener('change', () => save({practice: {minimum_interval: +interval.value}})); label.append(interval); controls.append(label);
    controls.append(node('p', '所有点共用冷却，每次随机选点。各点时间范围从到达起点开始计时。'));
  } else controls.append(node('p', '每名敌人的时间从本波开始计。全员死亡或完成撤回后进入下一波，最后一波后停止。每波按列表顺序分配不同英雄，阻塞超过预定拉出时间30秒后结束该敌人。'));
  const list = $('wave-list'); list.replaceChildren(); list.hidden = mode.value !== 'waves';
  if (list.hidden) return;
  const waves = config.practice.waves?.[currentMap.id] || [];
  const update = transform => save({practice: {waves: {...config.practice.waves, [currentMap.id]: transform(config.practice.waves?.[currentMap.id] || [])}}});
  waves.forEach((wave, wi) => {
    const section = node('section', null, 'wave-editor'); section.append(node('h3', `第 ${wi + 1} 波`));
    wave.forEach((enemy, ei) => {
      const row = node('div', null, 'wave-enemy'), route = node('select'); route.setAttribute('aria-label', '敌人拉出点');
      for (const r of routes) { const option = node('option', r.name); option.value = r.id; route.append(option); } route.value = enemy.route_id;
      const change = patch => update(current => current.map((w, i) => i === wi ? w.map((e, j) => j === ei ? {...e, ...patch} : e) : w));
      route.addEventListener('change', () => change({route_id: +route.value}));
      const remove = node('button', '删除敌人'); remove.addEventListener('click', () => update(current => current.map((w, i) => i === wi ? w.filter((_, j) => j !== ei) : w).filter(w => w.length)));
      row.append(node('strong', `敌人 ${ei + 1}`), route, enemyControls(enemy, change), remove); section.append(row);
    });
    const add = node('button', '添加敌人'); add.disabled = !routes.length || wave.length >= config.total_players - 1;
    add.addEventListener('click', () => update(current => current.map((w, i) => i === wi ? [...w, {route_id: routes[0].id, style: 'wide', fire: false, delay_min: 0, delay_max: 1}] : w)));
    const remove = node('button', '删除波次'); remove.addEventListener('click', () => update(current => current.filter((_, i) => i !== wi)));
    section.append(add, remove); list.append(section);
  });
  const add = node('button', '添加波次'); add.disabled = !routes.length || waves.flat().length >= 64;
  add.addEventListener('click', () => update(current => [...current, [{route_id: routes[0].id, style: 'wide', fire: false, delay_min: 0, delay_max: 1}]])); list.append(add);
}
function renderRoutes() {
  if (!config || !currentMap || document.activeElement?.closest('#route-list, #wave-list, #hold-plan-controls')) return;
  const routes = config.routes[currentMap.id] || []; $('route-count').textContent = routes.length;
  renderHoldPlan(routes);
  const list = $('route-list'); list.replaceChildren();
  if (!routes.length) return list.append(node('div', '还没有保存路线。先在地图上设置三个点。', 'empty-state'));
  const currentRoutes = () => config.routes[currentMap.id] || [];
  const validation = hasCurrentMapState() ? runtime.game?.routes || [] : [];
  for (const r of routes) {
    const row = node('div', null, 'route-row'), enabled = node('input'); enabled.type = 'checkbox'; enabled.checked = r.enabled; enabled.setAttribute('aria-label', `启用路线 ${r.name}`); enabled.addEventListener('change', () => updateRoutes(currentRoutes().map(x => x.id === r.id ? {...x, enabled: enabled.checked} : x)));
    const title = node('div'), name = node('strong', r.name), result = validation.find(x => x.id === r.id);
    title.append(name, node('small', result?.message_text || ({valid: '原路径可用', blocked: '路径不可用', pending: '等待客户端检查'}[result?.status]) || '启动训练后检查可走性'));
    const style = node('select'); style.setAttribute('aria-label', `${r.name} 拉出方式`); for (const [k, v] of Object.entries(catalog.route_styles || {...catalog.styles, global: '跟随全局'})) { const o = node('option', v); o.value = k; style.append(o); } style.value = r.style; style.addEventListener('change', () => updateRoutes(currentRoutes().map(x => x.id === r.id ? {...x, style: style.value} : x)));
    const weight = node('input'); Object.assign(weight, {type: 'number', min: '.1', max: '100', step: '.1', value: r.weight}); weight.title = '随机抽取权重'; weight.setAttribute('aria-label', `${r.name} 抽取权重`); weight.addEventListener('change', () => updateRoutes(currentRoutes().map(x => x.id === r.id ? {...x, weight: +weight.value} : x)));
    const edit = node('button', '编辑'); edit.addEventListener('click', () => { draftId = r.id; draft = structuredClone(r); $('route-name').value = r.name; drawMap(); $('map-surface').scrollIntoView({behavior: 'smooth', block: 'center'}); });
    const remove = node('button', '删除'); remove.setAttribute('aria-label', `删除路线 ${r.name}`); remove.addEventListener('click', () => { if (draftId === r.id) { draftId = null; draft = {}; } updateRoutes(currentRoutes().filter(x => x.id !== r.id)); });
    const details = enemyControls(r, value => updateRoutes(currentRoutes().map(x => x.id === r.id ? {...x, ...value} : x)));
    row.append(enabled, title, details, weight, edit, remove); list.append(row);
  }
  drawMap();
}
function renderWeapons() {
  const list = $('weapon-list'); list.replaceChildren(); $('weapon-count').textContent = catalog.weapons.length;
  let category;
  for (const w of [...catalog.weapons].sort((a, b) => (a.category_value || 7) - (b.category_value || 7))) {
    if (category !== w.category) { category = w.category; list.append(node('div', category, 'weapon-group-label')); }
    const b = node('button', null, 'weapon-row'); b.append(node('span', w.name), node('small', w.category)); b.classList.toggle('selected', w.id === selectedWeapon); b.setAttribute('aria-pressed', String(w.id === selectedWeapon));
    b.addEventListener('click', () => {
      const request = {id: w.id, submitted: false, at: Date.now()}; pendingWeapon = request;
      selectedWeapon = w.id; selectedSkin = null; renderWeapons(); loadSkins();
      enqueue(async () => {
        try { const result = accept(await api('/api/weapon', {weapon_id: w.id, revision: config.revision})); request.submitted = true; request.at = Date.now(); if (result.runtime?.status === 'saved' && pendingWeapon === request) pendingWeapon = null; toast(result.runtime?.status === 'saved' ? '武器已保存，启动后装备。' : '已提交切换武器。'); }
        catch (error) { if (pendingWeapon === request) pendingWeapon = null; throw error; }
      });
    }); list.append(b);
  }
}
async function loadSkins() {
  if (!selectedWeapon) return;
  const weapon = selectedWeapon, sequence = ++skinLoadSequence;
  selectedSkinFor = weapon; skins = []; selectedSkin = null; renderSkinOptions();
  $('skin-count').textContent = '—'; $('skin-list').replaceChildren(node('div', '正在读取原皮肤目录…', 'empty-state'));
  $('selected-weapon-title').textContent = catalog.weapons.find(w => w.id === weapon)?.name || '武器外观';
  try { const result = await api(`/api/skins?weapon=${encodeURIComponent(weapon)}`); if (selectedWeapon !== weapon || skinLoadSequence !== sequence) return; skins = result.skins; selectedSkin = skins.find(s => s.id === config.skins[weapon]?.skin_id) || skins.find(s => /standard/i.test(s.name)) || skins[0]; $('skin-count').textContent = skins.length; renderSkins(); renderSkinOptions(); }
  catch (error) { if (selectedWeapon !== weapon || skinLoadSequence !== sequence) return; setSkinPreviewState('unavailable', '皮肤目录暂时无法读取'); $('skin-list').replaceChildren(node('div', '皮肤目录读取失败，请重新选择武器。', 'empty-state')); toast(error.message, true); }
}
function skinPreviewQuery(s, {selected = false, image = false} = {}) {
  const level = selected && s.levels.some(o => o.id === $('skin-level').value) ? $('skin-level').value : s.levels.at(-1)?.id;
  const chroma = selected && s.chromas.some(o => o.id === $('skin-chroma').value) ? $('skin-chroma').value : s.default_chroma || s.chromas[0]?.id;
  const values = {weapon: selectedWeapon, skin: s.id, level, chroma, ...(image ? {format: 'png'} : {})};
  return '/api/skin-preview?' + Object.entries(values).filter(([, value]) => value != null).map(([key, value]) => `${key}=${encodeURIComponent(value)}`).join('&');
}
function skinThumbnail(s, selected = false) {
  const wrap = node('span', null, 'skin-thumbnail'), image = node('img');
  const unavailable = node('span', '暂无图片', 'skin-thumbnail-empty'); unavailable.hidden = true;
  image.alt = ''; image.setAttribute('aria-hidden', 'true'); image.loading = 'lazy'; image.decoding = 'async';
  image.onload = () => wrap.classList.add('loaded');
  image.onerror = () => { image.hidden = true; unavailable.hidden = false; wrap.classList.add('unavailable'); };
  image.src = skinPreviewQuery(s, {selected, image: true}); wrap.append(image, unavailable); return wrap;
}
function updateSelectedSkinThumbnail() {
  const card = [...$('skin-list').children].find(e => e.dataset.skinId === selectedSkin?.id);
  if (!card || !selectedSkin) return;
  const previous = card.querySelector('.skin-thumbnail');
  if (previous) previous.replaceWith(skinThumbnail(selectedSkin, true));
}
function renderSkins() {
  const q = $('skin-search').value.trim().toLocaleLowerCase(), list = $('skin-list');
  const focusedSkin = document.activeElement?.dataset?.skinId, scrollTop = list.scrollTop;
  list.replaceChildren();
  for (const s of skins.filter(s => s.name.toLocaleLowerCase().includes(q))) { const b = node('button', null, 'skin-card'), selected = s.id === selectedSkin?.id; b.dataset.skinId = s.id; b.append(skinThumbnail(s, selected), node('span', s.name, 'skin-card-name'), node('small', `${s.levels.length} 等级 · ${s.chromas.length} 炫彩`)); b.classList.toggle('selected', selected); b.setAttribute('aria-pressed', String(selected)); b.addEventListener('click', () => { selectedSkin = s; renderSkins(); renderSkinOptions(); }); list.append(b); }
  if (!list.children.length) list.append(node('div', skins.length ? '没有匹配的皮肤。' : '原皮肤目录尚未准备好。', 'empty-state'));
  list.scrollTop = scrollTop;
  if (focusedSkin) [...list.children].find(e => e.dataset.skinId === focusedSkin)?.focus({preventScroll: true});
}
function renderSkinOptions() {
  const s = selectedSkin, saved = config.skins[selectedWeapon];
  for (const [id, options] of [['skin-level', s?.levels || []], ['skin-chroma', s?.chromas || []]]) { const e = $(id); e.replaceChildren(); for (const option of options) { const o = node('option', id === 'skin-level' ? `等级 ${option.level || options.indexOf(option) + 1} · ${option.name}` : option.name); o.value = option.id; e.append(o); } if (s && saved?.skin_id === s.id) { const chosen = id === 'skin-level' ? saved.level_id : saved.chroma_id; if (options.some(x => x.id === chosen)) e.value = chosen; } else if (id === 'skin-level' && options.length) e.value = options.at(-1).id; else if (id === 'skin-chroma' && options.some(o => o.id === s?.default_chroma)) e.value = s.default_chroma; }
  $('apply-skin').disabled = !s || !$('skin-level').value || !$('skin-chroma').value; renderSkinPreview(); renderEffects();
}
function setSkinPreviewState(state, message) {
  const figure = $('skin-preview'); figure.dataset.state = state; figure.setAttribute('aria-busy', String(state === 'loading'));
  $('skin-preview-picture').replaceChildren(); $('skin-preview-placeholder').hidden = false; $('skin-preview-status').textContent = message;
}
async function renderSkinPreview() {
  const s = selectedSkin, sequence = ++skinPreviewSequence;
  const level = s?.levels.find(o => o.id === $('skin-level').value), chroma = s?.chromas.find(o => o.id === $('skin-chroma').value);
  $('skin-preview-name').textContent = s?.name || '选择皮肤查看'; $('skin-preview-badge').textContent = '2D';
  $('skin-preview-variant').textContent = s && level && chroma ? `等级 ${level.level || s.levels.indexOf(level) + 1} · ${chroma.id === s.default_chroma ? '默认炫彩' : chroma.name}` : '武器与炫彩的二维预览';
  $('skin-preview-note').textContent = '静态预览，动态特效请在游戏内查看。';
  if (!s || selectedSkinFor !== selectedWeapon || !level || !chroma) { setSkinPreviewState('empty', '选择一款皮肤后显示预览'); return; }
  setSkinPreviewState('loading', '正在读取预览图…'); updateSelectedSkinThumbnail();
  const request = skinPreviewQuery(s, {selected: true});
  try {
    const result = await api(request);
    if (sequence !== skinPreviewSequence) return;
    if (!result.available || !result.image_url) { setSkinPreviewState('unavailable', '此款暂无二维预览'); $('skin-preview-note').textContent = result.note || '仍可正常选择并应用外观。'; return; }
    const image = node('img'); image.alt = `${s.name}，${chroma.id === s.default_chroma ? '默认炫彩' : chroma.name}二维预览`;
    image.width = result.width; image.height = result.height; image.decoding = 'async';
    image.onload = () => { if (sequence !== skinPreviewSequence) return; $('skin-preview-picture').replaceChildren(image); $('skin-preview-placeholder').hidden = true; $('skin-preview').dataset.state = 'ready'; $('skin-preview').setAttribute('aria-busy', 'false'); $('skin-preview-note').textContent = result.note; $('skin-preview-badge').textContent = result.fallback ? '款式图' : '2D'; };
    image.onerror = () => { if (sequence !== skinPreviewSequence) return; setSkinPreviewState('unavailable', '预览图暂时无法读取'); $('skin-preview-note').textContent = '请稍后重新选择；仍可正常应用外观。'; };
    image.src = result.image_url;
  } catch (_) { if (sequence !== skinPreviewSequence) return; setSkinPreviewState('unavailable', '预览图暂时无法读取'); $('skin-preview-note').textContent = '请稍后重新选择；仍可正常应用外观。'; }
}
function renderEffects() {
  const list = $('effect-list'); if (!list || !config) return; list.replaceChildren();
  const rows = runtime.game?.effects?.length ? runtime.game.effects : [];
  const applicable = rows.filter(e => (e.supported !== false || config.effects[e.key || e.id]) && (!e.skin_guids?.length || e.skin_guids.some(g => g.replaceAll('-', '').toLowerCase() === selectedSkin?.id)));
  for (const e of applicable) { const key = e.key || e.id; if (!key) continue; const row = node('div', null, 'effect-row'), title = node('div'); title.append(node('strong', e.label_zh || e.label || e.name || key), node('small', e.description_zh || e.description || '使用原皮肤效果；事件触发时生效。')); const select = node('select'); select.setAttribute('aria-label', e.label_zh || e.label || key); for (const [i, text] of ['遵循原规则', '强制开启', '强制关闭'].entries()) { const o = node('option', text); o.value = i; select.append(o); } select.value = config.effects[key]?.mode ?? e.requested ?? 0; select.disabled = e.supported === false || (e.available === false && runtime.backend !== 'local-server-candidate');
    let numeric; if (e.numeric || key === 'skin_kill_count') { numeric = node('input'); Object.assign(numeric, {type: 'number', min: e.minimum ?? 0, max: e.maximum ?? 25, step: e.step ?? (key === 'skin_kill_count' ? 1 : 'any'), value: config.effects[key]?.value ?? e.value ?? 0, disabled: select.disabled}); numeric.setAttribute('aria-label', `${e.label_zh || e.label || key} 数值`); numeric.className = 'effect-number'; numeric.title = `范围：${numeric.min}–${numeric.max}`; }
    const submit = () => { if (numeric && !numeric.reportValidity()) return; return enqueue(async () => accept(await api('/api/effect', {key, mode: +select.value, value: numeric ? +numeric.value : 0, revision: config.revision}))); }; select.addEventListener('change', submit); if (numeric) numeric.addEventListener('change', submit); row.append(title); if (numeric) row.append(numeric); row.append(select); list.append(row); }
  if (!list.children.length) list.append(node('div', runtime.status === 'running' ? '当前外观没有可控制的特殊效果。' : '装备外观后，显示客户端实际可用的原效果。', 'empty-state'));
}
// The Human branch only reads native human_bots.behavior.bots. Legacy profile
// fields never substitute for missing identity/version/readback in that branch.
function renderBotProfiles(profiles, game, currentConfig, currentRuntime) {
  profiles.replaceChildren();
  if (currentConfig?.human_bots?.enabled !== true || currentConfig.mode !== 'deathmatch') {
    for (const b of (game.players || []).filter(b => b.bot)) { const profile = b.profile || {}, row = node('div', null, 'bot-profile'), title = node('div'); title.append(node('strong', b.name || `BOT ${b.index}`), node('small', b.alive === false ? '等待重生' : b.phase_text || '训练中')); row.append(title, node('span', `${b.kills || 0} / ${b.deaths || 0}`), node('span', profile.reaction_ms != null ? `${Math.round(profile.reaction_ms)} ms` : '—'), node('span', profile.aim_speed != null ? `${Math.round(profile.aim_speed)} °/s` : '—')); profiles.append(row); }
    if (!profiles.children.length) profiles.append(node('div', '进入乱斗后显示每个人机的水平与比分。', 'empty-state'));
    return;
  }
  const phaseLabels = {dead: '死亡', respawn: '重生', roam: '游走', search: '搜点', hold: '架点', engage: '对枪', information: '拿信息', retreat: '撤退', relocate: '换位', reload: '换弹'};
  const reported = game.human_bots?.behavior?.bots ?? currentRuntime?.human_bots?.behavior?.bots;
  const records = Array.isArray(reported) ? reported.filter(b => b && typeof b === 'object') : [];
  const positiveId = value => Number.isInteger(value) && value > 0;
  const validRevision = value => Number.isInteger(value) && value >= 0;
  const settingText = (value, unit) => typeof value === 'number' && Number.isFinite(value) && value >= 0 ? `${Number(value.toPrecision(7))} ${unit}` : '待确认';
  const appendField = (row, label, value, className = '') => {const field = node('div', null, `human-profile-field ${className}`); field.append(node('small', label), node('span', value)); row.append(field);};
  for (const b of (game.players || []).filter(b => b?.bot === true && positiveId(b.index))) {
    const stableId = b.stable_bot_id == null ? b.index : b.stable_bot_id;
    // Both identifiers must agree; duplicate or colliding records are ambiguous.
    const candidates = positiveId(stableId) ? records.filter(r => r.index === b.index || r.stable_bot_id === stableId) : [];
    const match = candidates.length === 1 && candidates[0].index === b.index && candidates[0].stable_bot_id === stableId ? candidates[0] : null;
    const running = currentRuntime?.status === 'running';
    const ready = running && !!match && match.configured === true && validRevision(match.profile_revision);
    const report = ready ? match : null;
    const row = node('div', null, 'bot-profile human-profile'); row.dataset.index = b.index; if (positiveId(stableId)) row.dataset.stableBotId = stableId;
    row.dataset.source = ready ? 'native_human_bots_behavior' : 'human_behavior_pending';
    const heading = node('div', null, 'human-profile-heading'), title = node('div');
    title.append(node('strong', b.name || `BOT ${b.index}`), node('small', positiveId(stableId) ? `人机 #${stableId}${!running ? ' · 等待启动' : b.alive === false ? ' · 等待重生' : ''}` : '人机身份待确认'));
    const score = node('span', running ? `${b.kills || 0} / ${b.deaths || 0}` : '— / —', 'human-profile-score'); score.title = '击杀 / 死亡'; heading.append(title, score); row.append(heading);
    appendField(row, '架点察觉设定', report ? settingText(report.hold_perceive_ms, 'ms') : '待确认');
    appendField(row, '首次定位速度设定', report ? settingText(report.primary_speed_deg_s, '°/s') : '待确认');
    const phase = report && typeof report.phase === 'string' ? phaseLabels[report.phase] : null;
    appendField(row, '当前动作', !running ? '等待启动' : phase || (report ? '动作待确认' : '等待行为数据确认'), 'human-profile-action');
    const reason = report && typeof report.reason_text === 'string' && report.reason_text.trim() ? report.reason_text : report && typeof report.reason === 'string' && report.reason.trim() ? report.reason : report && Number.isInteger(report.reason_code) ? `原因代码 ${report.reason_code}` : '';
    if (reason) {const note = node('small', reason, 'human-profile-reason'); row.append(note);}
    let revision = '已应用 rev：待确认';
    if (report) {
      row.dataset.profileRevision = report.profile_revision; revision = `已应用 rev ${report.profile_revision}`;
      if (validRevision(currentConfig.revision) && currentConfig.revision !== report.profile_revision) revision += ` · 本页保存 rev ${currentConfig.revision}${currentConfig.revision > report.profile_revision ? '，等待更新' : ''}`;
    }
    appendField(row, '配置版本', revision, 'human-profile-revision'); profiles.append(row);
  }
  if (!profiles.children.length) profiles.append(node('div', currentRuntime?.status === 'running' ? '等待本局人机列表和行为数据确认。' : '启动乱斗后显示本局人机与行为状态。', 'empty-state'));
  else profiles.prepend(node('p', '以下数值是配置设定，不是实测反应或转向速度；当前动作由行为模块报告。', 'human-profile-note'));
}

function renderRuntime() {
  const g = runtime.game || {}, active = runtime.status === 'running', loading = ['loading', 'stopping'].includes(runtime.status);
  syncWeaponSelection();
  renderHeroSelection();
  $('runtime-status').textContent = active && g.ended ? '本局结束' : active && g.ready === false ? g.stage_text || '正在准备训练' : phaseLabels[runtime.status] || '连接中'; $('live-dot').classList.toggle('active', active && !g.ended); $('mode-label').textContent = g.mode === 'deathmatch' || config?.mode === 'deathmatch' ? '个人乱斗 · 40 杀 / 9 分钟' : '架枪模式';
  $('connection').classList.add('online'); $('connection').querySelector('span').textContent = runtime.developmental ? '开发候选 · 尚未通过可玩验收' : active ? '客户端已连接' : '控制台已连接';
  $('stop-button').hidden = !active && runtime.status !== 'loading'; $('start-button').disabled = loading; $('start-button').textContent = loading ? '正在加载…' : active ? '重新开始训练' : '开始训练';
  const p = g.player || {}; $('live-score').textContent = `${p.kills || 0} / ${p.streak || 0}`;
  $('live-players').textContent = g.players?.length ? `${g.players.length} 人` : '—';
  $('live-time').textContent = Number.isFinite(g.time_remaining) && g.mode === 'deathmatch' ? `${Math.floor(Math.max(0, g.time_remaining) / 60)}:${String(Math.floor(Math.max(0, g.time_remaining) % 60)).padStart(2, '0')}` : '—';
  const winner = (g.players || []).find(player => player.index === g.winner);
  const ended = g.ended ? `${g.end_reason === 1 ? '已达到 40 杀' : '时间结束'}${winner ? `，${winner.name} 获胜` : '，本局平局'}。可点击“重开本局”继续。` : null;
  const notice = runtime.error_text || runtime.error || g.error_text || (g.error ? '客户端运行异常，请查看本次会话日志。' : null) || runtime.state_error || ended; $('notice').hidden = !notice; $('notice').textContent = notice || '';
  $('runtime-status').title = g.stage_text || phaseLabels[runtime.status] || '';
  const current = hasCurrentMapState(), feet = current && (p.feet || p.position);
  $('use-position').disabled = !feet; $('use-position').title = feet ? '使用当前地图玩家脚下的位置' : '请等待客户端进入当前地图';
  if (feet && !floorDirty && document.activeElement !== $('floor-height')) $('floor-height').value = Math.round(feet[2]);
  renderBotProfiles($('bot-profiles'), g, config, runtime);
  if (view === 'loadout' && !document.activeElement?.closest('#effect-list')) renderEffects();
  drawMap();
  humanBotsPanel?.refresh();
}
async function poll() {
  if (pollBusy) return; pollBusy = true;
  try { const result = await api('/api/state'); runtime = result.runtime; if (!config || result.config.revision > config.revision) { config = result.config; renderSettings(); } renderRuntime(); }
  catch (_) { $('connection').classList.remove('online'); $('connection').querySelector('span').textContent = '控制台连接已断开'; }
  finally { pollBusy = false; }
}
async function boot() {
  try {
    const [c, s] = await Promise.all([api('/api/catalog'), api('/api/state')]); catalog = c; config = s.config; runtime = s.runtime; fillMaps(); fillHeroes(); buildControls(); renderSettings(); renderWeapons(); renderRuntime();
    if (config.mode === 'deathmatch') setView('deathmatch');
    setInterval(poll, 1000);
    // Fine-grained bot settings fail independently of maps, loadouts and legacy controls.
    try {
      const human = await api('/api/human-bots/spec');
      humanBotsPanel = window.HumanBotsPanel($('human-bots-settings'), {
        getConfig: () => config, getRuntime: () => runtime,
        getHeroes: () => catalog.heroes || [],
        save, spec: human.spec, explanations: human.explanations,
        presetCatalog: human.preset_catalog, heroCatalog: human.hero_catalog,
        previewEntry: entry_id => api('/api/human-bots/preview', {entry_id}),
        rosterAction: body => enqueue(async () => accept(await api('/api/human-bots/roster', {...body, expected_revision: config.revision}))),
        capabilities: human.capabilities
      });
    } catch (error) {
      $('human-bots-settings').replaceChildren(node('h2', '拟人人机'), node('p', `详细设置暂不可用：${error.message}。其他训练设置可继续使用。`));
    }
  } catch (error) { $('notice').hidden = false; $('notice').textContent = `控制台未能初始化：${error.message}`; }
}
all('[data-view]').forEach(e => e.addEventListener('click', () => setView(e.dataset.view)));
$('local-hero-select').addEventListener('change', e => { save({local_hero_id: e.target.value}).catch(() => renderSettings()); });
all('[data-point]').forEach(e => e.addEventListener('click', () => setStep(e.dataset.point)));
$('map-surface').addEventListener('click', mapPoint);
$('map-surface').addEventListener('mousemove', e => { const r = e.currentTarget.getBoundingClientRect(); $('map-hover').textContent = `u ${((e.clientX - r.left) / r.width).toFixed(3)}  v ${((e.clientY - r.top) / r.height).toFixed(3)}`; });
$('map-surface').addEventListener('mouseleave', () => $('map-hover').textContent = '');
$('map-surface').addEventListener('keydown', e => { if (e.key === 'Escape') { draft = {}; draftId = null; drawMap(); } });
$('floor-height').addEventListener('change', () => floorDirty = true);
$('use-position').addEventListener('click', () => { const point = hasCurrentMapState() && (runtime.game?.player?.feet || runtime.game?.player?.position); if (!point) return toast('客户端尚未进入当前地图，暂时不能取玩家位置。', true); setDraftPoint([...point]); $('floor-height').value = Math.round(point[2]); });
$('clear-route').addEventListener('click', () => { draft = {}; draftId = null; $('route-name').value = ''; setStep('start'); drawMap(); });
async function saveDraftRoute() {
  if (!draft.start || !draft.expose || !draft.retreat) return toast('请先设置完整的三个路线点。', true);
  const mapId = currentMap.id, savedDraft = draft, routes = [...(config.routes[mapId] || [])];
  if (!draftId && routes.length >= 64) return toast('每张地图最多保存 64 条路线。', true);
  const id = draftId || Math.max(0, ...routes.map(r => r.id)) + 1, index = routes.findIndex(r => r.id === id), previous = routes[index];
  const route = {...previous, id, name: $('route-name').value.trim() || `路线 ${id}`, start: draft.start, expose: draft.expose, retreat: draft.retreat, style: previous?.style || 'global', enabled: previous?.enabled ?? true, weight: previous?.weight ?? 1};
  if (index >= 0) routes[index] = route; else routes.push(route);
  const result = await updateRoutes(routes);
  if (!result) return; // enqueue reports failures; keep the draft for retry.
  if (currentMap?.id === mapId && draft === savedDraft) { draftId = null; draft = {}; $('route-name').value = ''; setStep('start'); drawMap(); }
  toast(result.runtime?.status === 'saved' ? '路线已保存，启动训练后检查可走性。' : '路线已保存，客户端会检查可走性。');
}
$('save-route').addEventListener('click', saveDraftRoute);
for (const id of ['practice-map', 'deathmatch-map']) $(id).addEventListener('change', e => save({map_id: e.target.value}));
$('peek-rate').addEventListener('input', e => { $('peek-rate-value').value = e.target.value; debounceSave({practice: {peeks_per_minute: +e.target.value}}); });
$('return-fire').addEventListener('change', e => save({practice: {return_fire: e.target.checked}}));
$('pause-bots').addEventListener('change', e => save({paused: e.target.checked}));
all('.ammo-toggle').forEach(e => e.addEventListener('change', () => save({infinite_ammo: e.checked})));
all('.invincible-toggle').forEach(e => e.addEventListener('change', () => save({invincible: e.checked})));
$('streak-seconds').addEventListener('change', e => save({streak_seconds: +e.target.value}));
$('respawn-seconds').addEventListener('change', e => save({respawn_seconds: +e.target.value}));
$('total-players').addEventListener('input', e => { if (Array.isArray(config.human_bots?.bot_roster)) return; $('total-players-value').value = e.target.value; debounceSave({total_players: +e.target.value}); });
$('reroll-bots').addEventListener('click', () => save({seed: crypto.getRandomValues(new Uint32Array(1))[0] || 1}));
$('skin-search').addEventListener('input', renderSkins);
for (const id of ['skin-level', 'skin-chroma']) $(id).addEventListener('change', renderSkinPreview);
$('apply-skin').addEventListener('click', () => enqueue(async () => { if (!selectedSkin) return; const result = accept(await api('/api/skin', {weapon_id: selectedWeapon, skin_id: selectedSkin.id, level_id: $('skin-level').value, chroma_id: $('skin-chroma').value, revision: config.revision})); $('skin-application-status').textContent = result.runtime?.status === 'saved' ? '已保存，启动训练后应用。' : '正在通过原换装流程应用；实际状态以客户端为准。'; toast('已提交皮肤切换。'); }));
$('start-button').addEventListener('click', () => enqueue(async () => { const result = await api(runtime.status === 'running' ? '/api/restart' : '/api/start', {}); toast(result.message || '正在启动训练器。'); await poll(); }));
$('stop-button').addEventListener('click', () => enqueue(async () => { await api('/api/stop', {}); await poll(); }));
$('restart-match').addEventListener('click', () => enqueue(async () => { const r = await api('/api/restart-match', {}); toast(r.message || '已提交重开本局。'); }));
$('validate-routes').addEventListener('click', () => enqueue(async () => { const r = await api('/api/validate-routes', {}); toast(r.message_text || '已提交路线检查。'); }));
boot();
