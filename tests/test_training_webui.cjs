'use strict';
const assert = require('node:assert/strict');
const test = require('node:test');
const vm = require('node:vm');
const fs = require('node:fs');
const path = require('node:path');

function page() {
  const elements = new Map(), arcs = [];
  function element(tag = 'div') {
    const children = [], listeners = {};
    const item = {tagName: tag, children, listeners, dataset: {}, style: {}, value: '', textContent: '',
      classList: {toggle() {}, add() {}, remove() {}},
      append(...values) { children.push(...values); },
      replaceChildren(...values) { children.splice(0, children.length, ...values); },
      setAttribute() {}, removeAttribute() {}, addEventListener(name, fn) { listeners[name] = fn; },
      querySelector() { return element(); }, closest() { return null; }, scrollIntoView() {},
      reportValidity() { return true; },
      getContext() { return new Proxy({arc: (...args) => arcs.push(args)}, {get: (target, key) => target[key] || (() => {})}); }
    };
    return item;
  }
  const document = {activeElement: null, createElement: element, querySelectorAll: () => [],
    getElementById(id) { if (!elements.has(id)) elements.set(id, element()); return elements.get(id); }};
  const context = vm.createContext({document, structuredClone, Date, console,
    setTimeout: () => 1, clearTimeout() {}, setInterval() {},
    fetch: async () => { throw new Error('测试请求失败'); }});
  const source = fs.readFileSync(path.join(__dirname, '../webui/app.js'), 'utf8').replace(/\nboot\(\);\s*$/, '\n');
  vm.runInContext(source, context);
  const run = code => vm.runInContext(code, context);
  run(`config = {revision: 0, map_id: 'duel1', mode: 'practice', practice: {style: 'random'}, routes: {}, skins: {}, effects: {}};
       catalog = {styles: {jump: '跳拉', random: '随机'}, maps: [{id: 'duel1'}, {id: 'duel2'}], weapons: [{id: 'a'}, {id: 'b'}]};
       currentMap = {id: 'duel1', world_to_image: {matrix: [[1, 0, 0], [0, 1, 0]]}};
       runtime = {status: 'running', game: {map_id: 'duel1', player: {feet: [1, 2, 3]}, players: [{feet: [.1, .2, 0]}]}};`);
  return {run, elements, arcs, context};
}

test('failed route save never announces success or clears its draft; next queued task still runs', async () => {
  const {run, elements} = page();
  run(`draft = {start: [1,2,3], expose: [4,5,6], retreat: [7,8,9]};`);
  await run('saveDraftRoute()');
  assert.equal(elements.get('toast').textContent, '测试请求失败');
  assert.equal(run('draft.start[0]'), 1);
  assert.equal(elements.get('save-status').textContent, '应用失败，请查看提示');
  assert.equal(await run('enqueue(async () => 42)'), 42);
});

test('new route inherits global and editing keeps its style, weight and enabled state', async () => {
  const {run} = page();
  run(`updateRoutes = async routes => { config.routes[currentMap.id] = routes; return {runtime: {status:'saved'}}; };
       draft = {start:[1,2,3], expose:[4,5,6], retreat:[7,8,9]};`);
  await run('saveDraftRoute()');
  assert.equal(run('config.routes.duel1[0].style'), 'global');
  run(`Object.assign(config.routes.duel1[0], {style:'jump', weight:5, enabled:false});
       draftId=1; draft=structuredClone(config.routes.duel1[0]);`);
  await run('saveDraftRoute()');
  assert.equal(run('config.routes.duel1[0].style'), 'jump');
  assert.equal(run('config.routes.duel1[0].weight'), 5);
  assert.equal(run('config.routes.duel1[0].enabled'), false);
});

test('map mismatch and loading hide players and reject use-position, even with old game data', () => {
  const {run, elements, arcs} = page();
  run('drawMap()'); assert.equal(arcs.length, 1); arcs.length = 0;
  run(`config.map_id = currentMap.id = 'duel2'; drawMap();`);
  assert.equal(arcs.length, 0);
  elements.get('use-position').listeners.click();
  assert.equal(run('Object.keys(draft).length'), 0);
  assert.match(elements.get('toast').textContent, /尚未进入当前地图/);
  run(`runtime.game.map_id = 'duel2'; runtime.status = 'loading'; drawMap();`);
  assert.equal(arcs.length, 0);
  run(`runtime.status = 'running';`);
  elements.get('use-position').listeners.click();
  assert.equal(run('draft.start[2]'), 3);
});

test('native weapon updates selection while a pending user request stays selected until observed', () => {
  const {run} = page();
  run(`selectedWeapon = 'a'; runtime.game.weapon={ready:true,id:'b'}; syncWeaponSelection();`);
  assert.equal(run('selectedWeapon'), 'b');
  run(`pendingWeapon={id:'a',submitted:true,at:Date.now()}; selectedWeapon='a'; syncWeaponSelection();`);
  assert.equal(run('selectedWeapon'), 'a');
  assert.equal(run('pendingWeapon.id'), 'a');
  run(`runtime.game.weapon.id='a'; syncWeaponSelection();`);
  assert.equal(run('pendingWeapon'), null);
  run(`runtime.game.weapon.id='b'; syncWeaponSelection();`);
  assert.equal(run('selectedWeapon'), 'b');
});

test('numeric effect uses published range/value, preserves decimals, and disables unavailable controls', () => {
  const {run, elements} = page();
  run(`runtime.game.effects=[{key:'test_value',label:'数值效果',numeric:true,value:0.25,minimum:-1,maximum:1,available:false}]; renderEffects();`);
  const row = elements.get('effect-list').children[0], numeric = row.children[1];
  assert.equal(numeric.className, 'effect-number');
  assert.equal(numeric.min, -1); assert.equal(numeric.max, 1); assert.equal(numeric.value, .25);
  assert.equal(numeric.step, 'any'); assert.equal(numeric.disabled, true);
  assert.equal(row.children[2].disabled, true);
});

test('candidate mode changes preserve the selected 5v5 map', () => {
  const {run} = page();
  run(`config.mode='deathmatch'; config.map_id='ascent'; runtime.backend='local-server-candidate';
       let submitted; save = patch => { submitted=patch; }; setView('practice');`);
  assert.equal(run('submitted.mode'), 'practice');
  assert.equal(run('submitted.map_id'), undefined);
  assert.equal(run('config.map_id'), 'ascent');
  run(`runtime.backend='original'; submitted=undefined; setView('practice');`);
  assert.equal(run('submitted.mode'), 'practice');
  assert.equal(run('submitted.map_id'), undefined);
});

test('candidate map status cannot inherit standalone trainer verification', () => {
  const {run} = page();
  run(`runtime.backend='local-server-candidate';
       const info={validation:{runtime:'passed'},local_server_validation:{practice:{status:'passed'},deathmatch:{status:'not_tested'}}};`);
  assert.match(run(`mapStateText({validation:{runtime:'passed'}})`), /尚未验证/);
  assert.match(run('mapStateText(info)'), /检查已通过/);
  run(`config.mode='deathmatch';`);
  assert.match(run('mapStateText(info)'), /尚未验证/);
});

test('hold plan mode, shared cooldown and wave enemy controls persist through normal editors', async () => {
  const {run, elements} = page();
  run(`config.total_players=3;config.practice={mode:'continuous',minimum_interval:1,return_fire:false,waves:{}};
       config.routes.duel1=[{id:1,name:'A',style:'wide',fire:false,delay_min:0,delay_max:1}];
       catalog.route_styles={wide:'大拉',short:'小拉',peek:'静步拉出',wide_crouch:'大拉后蹲下',short_crouch:'小拉后蹲下'};
       save=async patch => { if(patch.practice)Object.assign(config.practice,patch.practice);if(patch.routes)config.routes=patch.routes;return {}; };
       renderHoldPlan(config.routes.duel1);`);
  const controls = elements.get('hold-plan-controls');
  const cooldown = controls.children[2].children[0];
  cooldown.value='2.5'; await cooldown.listeners.change();
  assert.equal(run('config.practice.minimum_interval'),2.5);
  controls.children[0].value='waves';await controls.children[0].listeners.change();
  run('renderHoldPlan(config.routes.duel1)');
  await elements.get('wave-list').children[0].listeners.click();
  assert.equal(run('config.practice.waves.duel1[0].length'),1);
  run('renderHoldPlan(config.routes.duel1)');
  const section=elements.get('wave-list').children[0];
  const enemy=section.children[1];
  const style=enemy.children[2].children[0];
  style.value='short_crouch'; await style.listeners.change();
  assert.equal(run('config.practice.waves.duel1[0][0].style'),'short_crouch');
  await section.children[2].listeners.click();
  assert.equal(run('config.practice.waves.duel1[0].length'),2);
  run('renderHoldPlan(config.routes.duel1)');
  assert.equal(elements.get('wave-list').children[0].children[3].disabled,true);
});

test('removing a point clears wave references and route geometry edits retain enemy timing', async () => {
  const {run} = page();
  run(`config.practice.waves={duel1:[[{route_id:1,style:'wide',fire:true,delay_min:2,delay_max:3}]]};
       config.routes.duel1=[{id:1,name:'A',style:'short_crouch',fire:true,delay_min:2,delay_max:3,weight:1,enabled:true,start:[1,2,3],expose:[4,5,6],retreat:[7,8,9]}];
       save=async patch => {if(patch.practice)Object.assign(config.practice,patch.practice);if(patch.routes)config.routes=patch.routes;return {};};
       draftId=1;draft=structuredClone(config.routes.duel1[0]);`);
  await run('saveDraftRoute()');
  assert.equal(run('config.routes.duel1[0].delay_min'),2);
  assert.equal(run('config.routes.duel1[0].fire'),true);
  await run('updateRoutes([])');
  assert.equal(run('config.practice.waves.duel1.length'),0);
});
