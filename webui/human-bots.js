/* Humanlike bot parameter editor. No game commands, seed writes or polling loop. */
(function (root) {
  'use strict';
  const scopes = {global: '全局覆盖', preset: '风格默认', distribution: '个体分布', individual: '单个人机'};
  const groups = {vision: '视觉感知', sensor: '感知调度', hearing: '听觉与受伤', memory: '信息记忆', attention: '注意力', reaction: '情境反应', aim: '定位、微调与跟枪', fire: '射击与武器反馈', movement: '身法与移动', peek: '拉出决策', coordination: '移动与瞄准协调', preaim: '预瞄与搜点', tactics: '乱斗决策', space: '空间与碰撞', melee: '近战', fire_weights: '射击习惯矩阵', weapon_tuning: '武器射击细项', diagnostics: '诊断记录', 'peek.wide': '大拉', 'peek.short': '小拉', 'peek.slow_creep': '慢摸', 'peek.jump': '跳 peek', 'peek.crouch': '蹲拉', 'peek.poppin': 'Poppin swing', 'peek.shoulder': '晃身拿信息', 'peek.info_repeek': '拿信息后再拉'};
  const guns = {odin: '奥丁', ares: '战神', vandal: '狂徒', phantom: '幻影', bulldog: '獠犬', guardian: '戍卫', judge: '判官', bucky: '雄鹿', frenzy: '狂怒', classic: '标配', ghost: '鬼魅', sheriff: '正义', shorty: '短炮', operator: '冥驹', outlaw: '莽侠', marshal: '飞将', spectre: '骇灵', stinger: '蜂刺', bandit: 'Bandit', warden: 'Warden', knife: '近战'};
  const names = {near: '近距', mid: '中距', far: '远距', steady: '停稳', moving: '移动中', underfire: '受击中', partial: '部分露出', single: '单点', double: '两连发', burst: '短点', spray: '扫射', movingclose: '近距移动射击'};
  const unitNames = {deg: '度', cm: '厘米', ratio: '比例', points: '点', ms: '毫秒', probability: '概率（0–1）', queries: '次查询', events: '条事件', toggle: '关闭 0 / 开启 1', per_s: '每秒', 'cm/s': '厘米 / 秒', weight: '相对权重', per_min: '每分钟', 'deg/s': '度 / 秒', 'deg/s2': '度 / 秒²', count: '次', shots: '发', phase: '阶段编号', segments: '段', mask: '条件位掩码'};
  const conditionNames = {known_target: '已有目标信息', unknown_angle: '未检查角度', visible_opponent: '看见对手', heard_shot: '听见枪声', under_fire: '遭到射击', low_ammo: '低弹药', low_health: '低生命', target_airborne: '目标在空中', near_range: '近距', mid_range: '中距', far_range: '远距', multiple_threats: '多个威胁', cover_proven: '掩体已验证', information_goal: '目的为拿信息', gunfight_goal: '目的为对枪', roaming: '游走中'};
  const own = (object, key) => Object.prototype.hasOwnProperty.call(object || {}, key);
  const clone = value => JSON.parse(JSON.stringify(value));
  const fmt = value => typeof value === 'number' ? Number(value.toPrecision(7)).toString() : value == null ? '—' : String(value);
  function paramName(param) {const parts = param.key.split('.'); const gun = ['fire_weights', 'weapon_tuning', 'weapon'].includes(parts[0]) && guns[parts[1]]; return gun ? param.name_zh.replace(new RegExp('^' + parts[1], 'i'), gun) : param.name_zh;}
  function el(tag, className, text) { const node = document.createElement(tag); if (className) node.className = className; if (text != null) node.textContent = text; return node; }
  function option(value, text) { const node = el('option', null, text); node.value = value; return node; }
  function field(text, control) { const label = el('label', 'hb-field'); label.append(el('span', null, text), control); return label; }
  function button(text, action, className = '') { const node = el('button', `hb-button ${className}`, text); node.type = 'button'; if (action) node.addEventListener('click', action); return node; }
  function emptySettings() { return {version: 1, enabled: false, preset: 'default', preset_overrides: {}, global_overrides: {}, distribution: {}, individual_overrides: {}}; }
  function HumanBotsPanel(container, options) {
    if (!(this instanceof HumanBotsPanel)) return new HumanBotsPanel(container, options);
    if (!container || !options?.getConfig || !options.save) throw new Error('拟人人机编辑器缺少配置接口。');
    this.container = container; this.options = options; this.drafts = new Map(); this.version = 0; this.page = 0; this.pageSize = 24;
    this.scope = 'global'; this.bot = 1; this.group = ''; this.query = ''; this.view = 'common'; this.filters = {}; this.rows = new Map(); this.destroyed = false;
    this.previewSeq = 0; this.previewKey = ''; this.previewValues = {}; this.previewInfo = null; this.lastRevision = null; this.saving = false; this.controlSaving = 0;
    this.spectatorSubmitting = false; this.spectatorRequest = null; this.spectatorErrorText = '';
    this.selectedEntryId = null; this.rosterRows = new Map(); this.rosterPresetChoices = new Map(); this.rosterBusy = false;
    this.rosterPresetErrors = new Map(); this.rosterPresetApplying = null; this.rosterPresetFlush = null;
    this.spec = options.spec || {}; this.explanations = options.explanations || {}; this.capabilities = options.capabilities || {};
    this.presetCatalog = Array.isArray(options.presetCatalog) ? options.presetCatalog : options.presetCatalog?.presets || options.presetCatalog?.items || []; this.heroCatalog = options.heroCatalog || [];
    this.indexSpec(); this.build(); this.renderList(); this.refresh();
  }
  HumanBotsPanel.prototype.config = function () { return this.options.getConfig() || {}; };
  HumanBotsPanel.prototype.settings = function () { return {...emptySettings(), ...this.config().human_bots}; };
  HumanBotsPanel.prototype.roster = function (settings = this.settings()) {return Array.isArray(settings.bot_roster) ? settings.bot_roster : null;};
  HumanBotsPanel.prototype.entry = function (id = this.selectedEntryId, settings = this.settings()) {return this.roster(settings)?.find(item => item.id === id) || null;};
  HumanBotsPanel.prototype.heroRows = function () {const data = this.options.getHeroes?.() || this.heroCatalog || []; return Array.isArray(data) ? data : data.heroes || [];};
  HumanBotsPanel.prototype.heroName = function (id) {const hero = this.heroRows().find(item => item.id === id); return hero?.name_zh || hero?.name || '英雄待确认';};
  HumanBotsPanel.prototype.entryName = function (id) {const entries = this.roster() || []; const index = entries.findIndex(item => item.id === id); return index < 0 ? '已移除的人机条目' : `${this.heroName(entries[index].hero_id)} · 人机 ${index + 1}`;};
  HumanBotsPanel.prototype.presetRows = function () {
    const builtin = this.presetCatalog || this.options.presetCatalog || [];
    return [{id: 'default', name_zh: '内置默认'}, ...builtin, ...Object.entries(this.settings().custom_presets || {}).map(([id, item]) => ({id, name_zh: item.name, custom: true}))];
  };
  HumanBotsPanel.prototype.buildRoster = function () {
    this.rosterPanel = el('section', 'hb-roster'); this.rosterPanel.setAttribute('aria-label', '固定人机名单');
    const heading = el('div', 'hb-roster-heading'); heading.append(el('h3', null, '固定人机名单'), el('p', null, '每条人机独立保存英雄、预设与详细参数。总人数为名单人数加你。'));
    this.rosterState = el('p', 'hb-roster-state'); this.rosterState.setAttribute('role', 'status'); this.rosterState.setAttribute('aria-live', 'polite');
    this.rosterError = el('p', 'hb-roster-error'); this.rosterError.setAttribute('role', 'alert'); this.rosterError.hidden = true;
    this.rosterList = el('div', 'hb-roster-list');
    this.addHero = el('select'); this.addHero.setAttribute('aria-label', '新增人机英雄'); this.addPreset = el('select'); this.addPreset.setAttribute('aria-label', '新增人机段位预设');
    this.addBot = button('添加人机', () => this.rosterAction({action: 'add', hero_id: this.addHero.value, preset_id: this.addPreset.value}));
    const add = el('div', 'hb-roster-add'); add.append(field('新英雄', this.addHero), field('初始预设', this.addPreset), this.addBot);
    this.entryTitle = el('p', 'hb-entry-title'); this.entryTitle.setAttribute('aria-live', 'polite');
    const custom = el('div', 'hb-roster-custom'); this.customName = el('input'); this.customName.type = 'text'; this.customName.placeholder = '自定义预设名称'; this.customName.setAttribute('aria-label', '保存当前人机为自定义预设的名称');
    this.saveCustom = button('保存此人机为预设', () => {const id = this.options.customPresetId?.() || (root.crypto?.randomUUID ? 'custom:' + root.crypto.randomUUID().replaceAll('-', '') : null); if (!id) return this.rosterFailure('预设标识生成器尚未就绪。'); return this.rosterAction({action: 'save_preset', entry_id: this.selectedEntryId, preset_id: id, name: this.customName.value.trim()});});
    custom.append(this.customName, this.saveCustom, el('small', null, '只保存此条已保存的有效参数，不包含未保存草稿。'));
    const manage = el('details', 'hb-roster-manage'); manage.append(el('summary', null, '管理自定义预设')); this.managePreset = el('select'); this.managePreset.setAttribute('aria-label', '待管理的自定义预设'); this.renameName = el('input'); this.renameName.type = 'text'; this.renameName.setAttribute('aria-label', '新的自定义预设名称');
    this.managePreset.addEventListener('change', () => {this.renameName.value = this.settings().custom_presets?.[this.managePreset.value]?.name || '';});
    this.renameCustom = button('重命名', () => this.rosterAction({action: 'rename_preset', preset_id: this.managePreset.value, name: this.renameName.value.trim()}));
    this.deleteCustom = button('删除预设', () => this.rosterAction({action: 'delete_preset', preset_id: this.managePreset.value})); const manageRow = el('div', 'hb-roster-manage-row'); manageRow.append(this.managePreset, this.renameName, this.renameCustom, this.deleteCustom); manage.append(manageRow, el('small', null, '正在被名单引用的预设不能删除。'));
    this.rosterPanel.append(heading, this.rosterState, this.rosterList, add, el('p', 'hb-roster-replace-note', '选择本条预设后立即保存。应用预设会替换此条已保存的个人设置；细项草稿可继续编辑和保存。段位是强度风格梯度，尚未用真人数据精确校准。'), custom, manage, this.rosterError, this.entryTitle);
  };
  HumanBotsPanel.prototype.indexSpec = function () {
    if (this.explanations.schema_digest && this.explanations.schema_digest !== this.spec.schema_digest_sha256 || this.explanations.param_count != null && this.explanations.param_count !== (this.spec.parameters || []).length) throw new Error('参数说明与当前配置版本不匹配，请重新读取。');
    this.params = this.spec.parameters || []; this.byKey = new Map(this.params.map(p => [p.key, p])); this.byEnum = new Map(this.params.map(p => [p.enum, p]));
    const explanations = this.explanations.items || this.explanations;
    this.presentation = this.explanations.presentation || {}; this.commonKeys = new Set(this.presentation.common_keys || []);
    this.descriptions = new Map(Array.isArray(explanations) ? explanations.map(p => [p.key, p.description_zh]) : Object.entries(explanations || {}).map(([key, p]) => [key, typeof p === 'string' ? p : p.description_zh]));
    this.constraints = new Map(this.params.map(p => [p.key, []]));
    const addConstraint = (enums, describe) => {const params = enums.map(name => this.byEnum.get(name)); if (params.some(p => !p)) return; const text = describe(params.map(paramName)); for (const p of params) this.constraints.get(p.key).push(text);};
    const cross = this.spec.cross_constraints || {};
    for (const [a, b, scale] of cross.less_equal_scaled || []) addConstraint([a, b], ([x, y]) => `${x}上界 ≤ ${y}下界${scale === 1 ? '' : ` × ${fmt(scale)}`}`);
    for (const [a, b] of cross.strict_less || []) addConstraint([a, b], ([x, y]) => `${x}上界 < ${y}下界`);
    for (const [a, b, limit] of cross.sum_max || []) addConstraint([a, b], ([x, y]) => `${x}上界 + ${y}上界 ≤ ${fmt(limit)}`);
    for (const [a, b, c] of cross.sum_le_param || []) addConstraint([a, b, c], ([x, y, z]) => `${x}上界 + ${y}上界 ≤ ${z}下界`);
    this.searchText = new Map(this.params.map(p => [p.key, [p.id, p.key, p.enum, p.name_zh, paramName(p), this.descriptions.get(p.key), p.condition_zh, this.sourceDescription(p), ...this.constraints.get(p.key), ...this.behaviorDependencies(p)].join(' ').toLocaleLowerCase()]));
  };
  HumanBotsPanel.prototype.build = function () {
    const panel = this.container; panel.classList.add('hb-panel'); panel.replaceChildren();
    const heading = el('header', 'hb-heading'); const title = el('div'); title.append(el('h2', null, '拟人人机'), el('p', null, '分别调整感知、枪法、身法与决策。每个人机保持自己的习惯。'));
    this.enabled = el('input'); this.enabled.type = 'checkbox'; this.enabled.setAttribute('role', 'switch');
    this.enabled.addEventListener('change', () => this.persistControl({enabled: this.enabled.checked}));
    const enabledLabel = el('label', 'hb-enable'); enabledLabel.append(el('span', null, '使用拟人人机'), this.enabled); heading.append(title, enabledLabel);
    this.status = el('div', 'hb-status'); this.status.setAttribute('role', 'status'); this.status.setAttribute('aria-live', 'polite');
    this.statusMessage = el('strong'); this.statusRevisions = el('span'); this.status.append(this.statusMessage, this.statusRevisions);
    const adoptionNote = el('p', 'hb-adoption-note', '配置采用只确认参数传递，实际行为和地图覆盖仍需验证。');
    adoptionNote.style.flexBasis = '100%';
    this.status.append(adoptionNote);
    const setup = el('div', 'hb-setup'); this.presetSelect = el('select'); this.presetSelect.setAttribute('aria-label', '段位风格预设'); this.fillPresets();
    this.presetSelect.addEventListener('change', () => this.persistControl({preset: this.presetSelect.value}));
    this.botSelect = el('select'); this.botSelect.setAttribute('aria-label', '稳定人机编号与配置预览对象'); this.botSelect.addEventListener('change', () => {this.bot = +this.botSelect.value; this.page = 0; this.renderList(); this.refreshSpectator(); this.requestPreview(true);});
    this.presetField = field('全局默认风格', this.presetSelect); this.botField = field('人机 / 配置预览对象', this.botSelect); setup.append(this.presetField, this.botField);
    this.buildRoster();
    this.spectatorBar = el('section', 'hb-spectator'); this.spectatorBar.setAttribute('aria-label', '所选人机观战与实际参数');
    const spectatorCopy = el('div', 'hb-spectator-copy');
    this.spectatorState = el('p', 'hb-spectator-state'); this.spectatorState.setAttribute('role', 'status'); this.spectatorState.setAttribute('aria-live', 'polite');
    this.spectatorProfile = el('p', 'hb-spectator-profile'); this.spectatorProfile.setAttribute('aria-live', 'polite');
    this.spectatorNote = el('p', 'hb-spectator-note', '编号用于编辑和预览；观战仅切换观察目标，未保存草稿不生效。');
    this.spectatorError = el('p', 'hb-spectator-error'); this.spectatorError.setAttribute('role', 'alert'); this.spectatorError.hidden = true;
    spectatorCopy.append(this.spectatorState, this.spectatorProfile, this.spectatorNote, this.spectatorError);
    const spectatorButtons = el('div', 'hb-spectator-buttons');
    this.spectateButton = button('观战此人机', () => this.roster() ? this.requestRosterSpectator(this.selectedEntryId) : this.requestSpectator(this.bot));
    this.returnButton = button('返回自己', () => this.requestSpectator(0)); spectatorButtons.append(this.spectateButton, this.returnButton);
    this.spectatorBar.append(spectatorCopy, spectatorButtons);
    const presetNote = el('p', 'hb-note', '风格预设可编辑，尚未用真人数据校准段位。切换预设会保留全局、分布和个体覆盖。');
    this.scopes = el('div', 'hb-scopes'); this.scopes.setAttribute('role', 'group'); this.scopes.setAttribute('aria-label', '参数作用范围');
    for (const [id, name] of Object.entries(scopes)) { const b = button(name, () => {this.scope = id; this.page = 0; this.updateScope(); this.renderList();}); b.dataset.scope = id; this.scopes.append(b); }
    this.scopeNote = el('p', 'hb-scope-note');
    this.views = el('div', 'hb-views'); this.views.setAttribute('role', 'group'); this.views.setAttribute('aria-label', '常用与全部设置');
    for (const [id, label] of [['common', '常用设置'], ['all', '全部细项']]) {const b = button(label, () => {this.view = id; this.page = 0; if (id === 'common') {this.query = ''; this.group = ''; this.filters = {}; this.search.value = ''; this.groupSelect.value = ''; for (const select of this.matrixFilters.querySelectorAll('select')) select.value = '';} this.renderList();}); b.dataset.view = id; this.views.append(b);}
    const filterBar = el('div', 'hb-filterbar'); this.search = el('input'); this.search.type = 'search'; this.search.placeholder = '搜索全部参数、中文含义、编号…';
    this.search.setAttribute('aria-label', '搜索参数，与分组和枪械筛选同时生效');
    this.search.addEventListener('input', () => {this.query = this.search.value.trim().toLocaleLowerCase(); this.page = 0; this.renderList();});
    this.groupSelect = el('select'); this.groupSelect.setAttribute('aria-label', '参数分组'); this.fillGroups(); this.groupSelect.addEventListener('change', () => {this.group = this.groupSelect.value; this.view = 'all'; this.page = 0; this.renderList();});
    filterBar.append(this.search, this.groupSelect);
    this.matrixFilters = el('div', 'hb-matrix-filters');
    const filterEntries = [['weapon', '武器', [...new Set(this.params.filter(p => /^fire_weights\./.test(p.key)).map(p => p.key.split('.')[1]))]], ['distance', '距离', this.spec.distances || []], ['context', '情境', this.spec.fire_contexts || []], ['mode', '射击方式', this.spec.fire_modes || []]];
    for (const [key, label, values] of filterEntries) {
      const select = el('select'); select.dataset.filter = key; select.setAttribute('aria-label', `筛选${label}`); select.append(option('', `全部${label}`));
      for (const val of values) { const v = val.toLowerCase(); select.append(option(v, key === 'weapon' ? `${guns[v] || val} · ${val}` : names[v] || val)); }
      select.addEventListener('change', () => {this.filters[key] = select.value; this.view = 'all'; this.page = 0; this.renderList();}); this.matrixFilters.append(field(label, select));
    }
    const secondary = el('details', 'hb-secondary'); secondary.append(el('summary', null, '能力状态、继承顺序与使用说明'));
    this.inheritanceNote = el('p', null, '优先级：内置默认 → 风格预设及风格修改 → 全局固定 → 个体分布 → 单个人机。分布按种子和稳定编号采样，重生不重抽。单个人机可用固定值或范围。'); secondary.append(this.inheritanceNote);
    secondary.append(el('p', null, '全局选择固定值时会同时删除该项的个体分布，其他参数和单人覆盖保持。行内和修改审阅会列出此变化。恢复继承只清所选层；跨项校验失败会拒绝整批保存，不自动修正其他参数。'));
    this.samplingNote = el('p', null, this.presentation.sampling_zh || '个体范围按种子与稳定编号独立取样，重生不重抽；短时动作波动另行设置。'); secondary.append(this.samplingNote);
    secondary.append(el('p', null, this.presentation.precision_zh || '实际执行受感知、决策与原角色动作频率限制，不保证端到端 1 毫秒。'));
    secondary.append(el('p', null, '不可用或来源未确认的细项保留可见并锁定。数值保存、配置被采用和动作实际完成分别确认；目录条目不代表动作已经接入。'));
    this.migrationNote = el('p', 'hb-migration-note'); this.migrationNote.hidden = true; secondary.append(this.migrationNote);
    this.count = el('p', 'hb-results'); this.count.setAttribute('aria-live', 'polite');
    this.list = el('div', 'hb-list'); this.list.setAttribute('aria-label', '参数编辑器');
    this.pager = el('nav', 'hb-pager'); this.pager.setAttribute('aria-label', '参数分页');
    this.prev = button('上一页', () => {this.page--; this.renderList(); this.list.scrollIntoView({block: 'start'});}); this.next = button('下一页', () => {this.page++; this.renderList(); this.list.scrollIntoView({block: 'start'});}); this.pageLabel = el('span');
    this.sizeSelect = el('select'); this.sizeSelect.setAttribute('aria-label', '每页参数数'); for (const n of [24, 48]) this.sizeSelect.append(option(n, `${n} 项 / 页`)); this.sizeSelect.addEventListener('change', () => {this.pageSize = +this.sizeSelect.value; this.page = 0; this.renderList();}); this.pager.append(this.prev, this.pageLabel, this.next, this.sizeSelect);
    this.error = el('div', 'hb-error'); this.error.hidden = true; this.error.setAttribute('role', 'alert');
    this.review = el('details', 'hb-draft-review'); this.reviewSummary = el('summary', null, '检查未保存修改'); this.reviewSummary.tabIndex = 0;
    this.reviewList = el('ul'); this.reviewPage = 0; this.reviewVersion = '';
    const reviewPager = el('nav', 'hb-pager'); reviewPager.setAttribute('aria-label', '未保存修改分页');
    this.reviewPrev = button('上一页修改', () => {this.reviewPage--; this.renderReview(true);}); this.reviewNext = button('下一页修改', () => {this.reviewPage++; this.renderReview(true);}); this.reviewPageLabel = el('span');
    reviewPager.append(this.reviewPrev, this.reviewPageLabel, this.reviewNext); this.review.append(this.reviewSummary, this.reviewList, reviewPager); this.review.hidden = true;
    this.actions = el('div', 'hb-actions'); this.draftLabel = el('span'); this.saveButton = button('保存修改', () => this.saveDrafts(), 'hb-primary'); this.discardButton = button('放弃未保存', () => {this.drafts.clear(); this.error.hidden = true; this.renderList();});
    this.clearButton = button('恢复当前范围继承', () => this.clearScope()); this.actions.append(this.draftLabel, this.clearButton, this.discardButton, this.saveButton);
    panel.append(heading, this.status, this.rosterPanel, setup, this.spectatorBar, presetNote, this.scopes, this.scopeNote, this.views, filterBar, this.matrixFilters, secondary, this.count, this.list, this.pager, this.error, this.review, this.actions);
    this.updateScope();
  };
  HumanBotsPanel.prototype.fillPresets = function () {
    this.presetSelect.replaceChildren(option('default', '内置默认'));
    const rows = this.presetCatalog?.length ? this.presetCatalog : this.spec.presets || [];
    for (const preset of rows) this.presetSelect.append(option(preset.id, preset.name_zh));
    const current = this.settings().preset; if (current && current !== 'default' && !rows.some(row => row.id === current)) {const mapped = rows.find(row => row.legacy_anchor_id === current || current === 'radiant' && row.rank === 'radiant'); const old = option(current, `${mapped?.name_zh || this.settings().custom_presets?.[current]?.name || '原兼容风格'}（当前兼容值）`); old.disabled = true; this.presetSelect.append(old);}
  };
  HumanBotsPanel.prototype.fillRosterSelect = function (select, rows, value) {select.replaceChildren(); for (const row of rows) {const item = option(row.id, row.name_zh || row.name || row.id); item.disabled = row.disabled === true; select.append(item);} select.value = rows.some(row => row.id === value) ? value : rows[0]?.id || '';};
  HumanBotsPanel.prototype.rosterPresetRows = function (entry) {const rows = this.presetRows(); if (!rows.some(row => row.id === entry.preset_id)) {const legacy = (this.spec.presets || []).find(row => row.id === entry.preset_id); rows.push({id: entry.preset_id, name_zh: `沿用已保存设置 · ${legacy?.name_zh || entry.preset_id}（旧预设）`, disabled: true});} return rows;};
  HumanBotsPanel.prototype.rosterMapping = function (entryId, runtime = this.options.getRuntime?.() || {}) {
    const entry = this.entry(entryId); const roster = runtime.game?.configured_bot_roster;
    if (!entry || roster?.loaded !== true || roster.fixed !== true || roster.validated !== true || roster.native_catalog_verified !== true || !Array.isArray(roster.entries)) return null;
    const matches = roster.entries.filter(item => item.entry_token === entry.id && item.hero_id === entry.hero_id);
    return matches.length === 1 && Number.isInteger(matches[0].slot) && matches[0].slot > 0 && Number.isInteger(matches[0].native_player_id) && matches[0].native_player_id > 0 ? matches[0] : null;
  };
  HumanBotsPanel.prototype.rosterStructureMatches = function (runtime) {
    const roster = this.roster(); const actual = runtime.game?.configured_bot_roster;
    return !!roster && actual?.loaded === true && actual.fixed === true && actual.validated === true && actual.native_catalog_verified === true && actual.human_hero_id === this.config().local_hero_id
      && actual.entries?.length === roster.length && roster.every((entry, i) => actual.entries[i]?.slot === i + 1 && actual.entries[i].entry_token === entry.id && actual.entries[i].hero_id === entry.hero_id);
  };
  HumanBotsPanel.prototype.selectRosterEntry = function (id) {
    const roster = this.roster() || []; const index = roster.findIndex(entry => entry.id === id); if (index < 0) return;
    this.selectedEntryId = id; this.bot = index + 1; this.scope = 'individual'; this.page = 0; this.renderList(); this.refreshRoster(); this.requestPreview(true); this.entryTitle.scrollIntoView({block: 'nearest'});
  };
  HumanBotsPanel.prototype.requestRosterSpectator = function (id) {const mapping = this.rosterMapping(id); if (!mapping) {this.spectatorErrorText = '此条未以相同英雄出现在当前对局；名单改变后请手动重启对局。'; this.refreshSpectator(); return;} return this.requestSpectator(mapping.slot);};
  HumanBotsPanel.prototype.rosterFailure = function (message) {this.rosterError.hidden = false; this.rosterError.textContent = message;};
  HumanBotsPanel.prototype.selectRosterPreset = function (entryId, presetId) {
    if (!this.entry(entryId) || this.destroyed) return;
    this.rosterPresetChoices.set(entryId, presetId);
    this.rosterPresetErrors.delete(entryId);
    this.refresh();
    return this.flushRosterPresets();
  };
  HumanBotsPanel.prototype.flushRosterPresets = function () {
    if (this.rosterPresetFlush) return this.rosterPresetFlush;
    if (this.rosterBusy || this.destroyed || !this.rosterPresetChoices.size) return Promise.resolve();
    this.rosterPresetFlush = Promise.resolve().then(async () => {
      try {
        while (!this.destroyed) {
          const pending = [...this.rosterPresetChoices].find(([id]) => this.entry(id) && !this.rosterPresetErrors.has(id));
          if (!pending) break;
          const [entryId, presetId] = pending;
          this.rosterPresetApplying = {entryId, presetId};
          const result = await this.rosterAction({action: 'apply_preset', entry_id: entryId, preset_id: presetId, mode: 'snapshot'});
          if (this.rosterPresetChoices.get(entryId) === presetId) {
            const saved = result?.config?.human_bots?.bot_roster?.find(entry => entry.id === entryId);
            if (saved?.preset_id === presetId && this.entry(entryId)?.preset_id === presetId) {
              this.rosterPresetChoices.delete(entryId);
              this.rosterPresetErrors.delete(entryId);
            } else {
              this.rosterLastMessage = '';
              this.rosterPresetErrors.set(entryId, this.rosterLastError || '应用结果尚未确认，请重试。');
            }
          }
          this.rosterPresetApplying = null;
        }
      } finally {
        this.rosterPresetApplying = null;
        this.rosterPresetFlush = null;
        this.refresh();
      }
    });
    return this.rosterPresetFlush;
  };
  HumanBotsPanel.prototype.rosterAction = async function (body) {
    if (this.rosterBusy || this.destroyed) return;
    if (!this.options.rosterAction) return this.rosterFailure('固定名单操作接口尚未就绪。');
    if (['save_preset', 'rename_preset'].includes(body.action) && !body.name) return this.rosterFailure('请输入预设名称。');
    this.rosterBusy = true; this.rosterError.hidden = true; this.rosterLastMessage = ''; this.rosterLastError = ''; this.refreshRoster();
    try {
      const result = await this.options.rosterAction(clone(body)); if (!result?.config) throw new Error('保存未获确认，当前细项草稿仍保留。');
      if (result.runtime?.restart_required === true) this.rosterRestartRequired = true;
      if (body.action === 'apply_preset') this.rosterLastMessage = `已将预设应用到 ${this.entryName(body.entry_id)} 的保存参数。${this.drafts.size ? '未保存细项草稿仍保留，可放弃后查看预设值，或自行保存草稿。' : '实际行为采用版本见下方状态。'}`;
      if (body.action === 'save_preset') this.rosterLastMessage = '自定义预设已按此条保存参数建立，不包含未保存草稿。';
      if (body.action === 'add' && result.roster_action?.entry_id) this.selectedEntryId = result.roster_action.entry_id;
      if (body.action === 'save_preset') this.customName.value = '';
      this.refresh(); this.renderList(); this.requestPreview(true);
      return result;
    } catch (error) {if (body.action === 'update_entry' && body.hero_id) {const row = this.rosterRows.get(body.entry_id); if (row) row.hero.value = this.entry(body.entry_id)?.hero_id || '';} this.rosterLastError = String(error.message || error); this.rosterFailure(`名单未获保存确认：${this.rosterLastError}`); return null;}
    finally {this.rosterBusy = false; this.refreshRoster(); if (!this.rosterPresetFlush) this.flushRosterPresets();}
  };
  HumanBotsPanel.prototype.refreshRoster = function () {
    const entries = this.roster(); this.rosterPanel.hidden = !entries; this.botField.hidden = !!entries;
    if (!entries) return false;
    const config = this.config(); const runtime = this.options.getRuntime?.() || {}; const heroes = this.heroRows(); const presets = this.presetRows();
    for (const id of this.rosterPresetChoices.keys()) if (!entries.some(entry => entry.id === id)) {
      this.rosterPresetChoices.delete(id); this.rosterPresetErrors.delete(id);
    }
    const previous = this.selectedEntryId; if (!entries.some(entry => entry.id === this.selectedEntryId)) this.selectedEntryId = entries[0]?.id || null;
    this.bot = Math.max(1, entries.findIndex(entry => entry.id === this.selectedEntryId) + 1); if (previous == null && this.selectedEntryId) this.scope = 'individual';
    const shape = JSON.stringify([entries.map(entry => [entry.id, entry.hero_id]), heroes.map(hero => [hero.id, hero.name]), presets.map(preset => [preset.id, preset.name_zh]), config.local_hero_id]);
    if (this.rosterShape !== shape) {
      this.rosterShape = shape; this.rosterRows.clear(); this.rosterList.replaceChildren();
      for (const [index, entry] of entries.entries()) {
        const row = el('article', 'hb-roster-row'); row.dataset.entryId = entry.id; const label = el('strong', 'hb-roster-name', `${this.heroName(entry.hero_id)} · 人机 ${index + 1}`);
        const hero = el('select'); hero.setAttribute('aria-label', `人机 ${index + 1} 的英雄`); this.fillRosterSelect(hero, heroes.filter(item => item.id !== config.local_hero_id && !entries.some(other => other.id !== entry.id && other.hero_id === item.id)), entry.hero_id);
        hero.addEventListener('change', () => this.rosterAction({action: 'update_entry', entry_id: entry.id, hero_id: hero.value}));
        const preset = el('select'); preset.setAttribute('aria-label', `人机 ${index + 1} 的独立预设`); this.fillRosterSelect(preset, this.rosterPresetRows(entry), this.rosterPresetChoices.get(entry.id) || entry.preset_id); if (!this.presetCatalog?.length) {preset.replaceChildren(option(entry.preset_id, '预设目录待确认')); preset.value = entry.preset_id;}
        preset.addEventListener('change', () => this.selectRosterPreset(entry.id, preset.value));
        const apply = button('重新应用预设', () => this.selectRosterPreset(entry.id, preset.value)); apply.title = '将此条个人设置恢复为所选预设的参数。';
        const presetState = el('p', 'hb-roster-preset-state'); presetState.setAttribute('role', 'status'); presetState.setAttribute('aria-live', 'polite');
        const detail = button('详细参数', () => this.selectRosterEntry(entry.id)); const spectate = button('观战', () => this.requestRosterSpectator(entry.id)); const remove = button('删除', () => this.rosterAction({action: 'remove', entry_id: entry.id}));
        const actions = el('div', 'hb-roster-row-actions'); actions.append(apply, detail, spectate, remove); row.append(label, field('英雄', hero), field('本条预设 · 选择后保存', preset), actions, presetState); this.rosterList.append(row);
        this.rosterRows.set(entry.id, {row, hero, preset, apply, detail, spectate, remove, presetState});
      }
      const used = new Set([config.local_hero_id, ...entries.map(entry => entry.hero_id)]); this.fillRosterSelect(this.addHero, heroes.filter(hero => !used.has(hero.id)), this.addHero.value);
      this.fillRosterSelect(this.addPreset, presets, this.addPreset.value || 'rank:iron_ii');
      const custom = presets.filter(item => item.custom); const old = this.managePreset.value; this.fillRosterSelect(this.managePreset, custom, old);
      if (document.activeElement !== this.renameName) this.renameName.value = this.settings().custom_presets?.[this.managePreset.value]?.name || '';
    }
    const canMutate = !!this.options.rosterAction && !this.rosterBusy; const running = runtime.status === 'running' && runtime.backend === 'local-server-candidate';
    const matches = this.rosterStructureMatches(runtime); if (matches) this.rosterRestartRequired = false;
    this.rosterState.textContent = this.rosterBusy ? '正在保存名单设置…' : (running && (this.rosterRestartRequired || !matches) ? '名单按保存内容编辑；当前对局仍用启动名单。结构改变需手动重启对局，战斗参数仅在条目与英雄映射一致时热更新。' : `已保存 ${entries.length} 位固定人机，加上你共 ${entries.length + 1} 人；不会在开局重新随机英雄。`) + (this.rosterLastMessage ? ` ${this.rosterLastMessage}` : '');
    this.entryTitle.textContent = this.selectedEntryId ? `正在细调：${this.entryName(this.selectedEntryId)} · 只修改此名单条目；未保存草稿不会被预设或观战自动提交。` : '添加人机后可编辑详细参数。';
    for (const [id, row] of this.rosterRows) {
      const entry = this.entry(id);
      const presetName = value => this.rosterPresetRows(entry).find(preset => preset.id === value)?.name_zh || value;
      const desired = this.rosterPresetChoices.get(id);
      const failed = this.rosterPresetErrors.has(id);
      if (this.presetCatalog?.length) {
        const choices = this.rosterPresetRows(entry);
        const menuShape = JSON.stringify(choices.map(preset => [preset.id, preset.name_zh, preset.disabled]));
        if (row.presetOptionsShape !== menuShape) {
          row.presetOptionsShape = menuShape;
          this.fillRosterSelect(row.preset, choices, desired || entry.preset_id);
        }
        if (!desired && document.activeElement !== row.preset) row.preset.value = entry.preset_id;
      }
      row.presetState.textContent = `已保存 ${presetName(entry.preset_id)}`;
      if (desired) row.presetState.textContent += failed ? ` · ${presetName(desired)} 的应用结果待确认` : ` · ${this.rosterPresetApplying?.entryId === id ? '正在' : '等待'}应用 ${presetName(desired)}`;
      row.presetState.title = this.rosterPresetErrors.get(id) || '';
      row.apply.textContent = failed ? '重试应用' : desired ? '正在保存预设' : '重新应用预设';
      const selected = id === this.selectedEntryId; row.row.classList.toggle('selected', selected); row.detail.setAttribute('aria-pressed', String(selected));
      row.hero.disabled = !canMutate;
      row.preset.disabled = !this.options.rosterAction || this.rosterBusy && !this.rosterPresetApplying || !this.presetCatalog?.length;
      row.apply.disabled = !canMutate || !this.presetCatalog?.length || !presets.some(preset => preset.id === row.preset.value); row.apply.title = row.apply.disabled && !presets.some(preset => preset.id === row.preset.value) ? '沿用此条已保存设置；请先选择目录中的预设再应用。' : '会替换此条已保存的个人设置，不提交细项草稿。'; row.remove.disabled = !canMutate || entries.length <= 1;
      const mapping = this.rosterMapping(id, runtime); row.spectate.disabled = !running || !mapping || this.spectatorSubmitting; row.spectate.title = mapping ? '观战本局对应条目，不保存参数。' : '本局尚未以同一条目和英雄出场；修改名单后需重启。';
    }
    this.addHero.disabled = !canMutate || !this.addHero.children.length; this.addPreset.disabled = !canMutate; this.addBot.disabled = !canMutate || !this.addHero.value || !this.presetCatalog?.length;
    this.saveCustom.disabled = !canMutate || !this.selectedEntryId; this.renameCustom.disabled = !canMutate || !this.managePreset.value;
    const referenced = entries.some(entry => entry.preset_id === this.managePreset.value) || this.settings().preset === this.managePreset.value || Object.values(this.settings().individual_presets || {}).includes(this.managePreset.value); this.deleteCustom.disabled = !canMutate || !this.managePreset.value || referenced; this.deleteCustom.title = referenced ? '正在被名单条目或兼容设置使用，不能删除。' : '删除未被引用的自定义预设。';
    this.inheritanceNote.textContent = '优先级：内置默认 → 本条预设及其共用修改 → 全局基础 → 个体分布 → 本条覆盖。使用范围时按条目身份固定采样，名单排序或死亡重生不会把能力分配给另一条人机。';
    this.samplingNote.textContent = '本条的固定值与范围独立保存；按对局种子、条目身份和参数采样，重排不重抽。动作中的短时失误和波动另行设置。';
    return previous !== this.selectedEntryId;
  };
  HumanBotsPanel.prototype.fillGroups = function () {
    this.groupSelect.replaceChildren(option('', `全部参数 · ${this.params.length}`));
    const unique = [...new Set(this.params.map(p => p.group))];
    const append = (host, group) => host.append(option(group, `${groups[group] || (group.startsWith('weapon.') ? (guns[group.split('.')[1]] || group.split('.')[1]) + ' · 距离分段' : group)} · ${this.params.filter(p => p.group === group).length}`));
    const main = el('optgroup'); main.label = '感知与对枪'; unique.filter(g => !g.startsWith('peek.') && !g.startsWith('weapon.') && !['fire_weights', 'weapon_tuning'].includes(g)).forEach(g => append(main, g));
    const peeks = el('optgroup'); peeks.label = '具体拉出技巧'; unique.filter(g => g.startsWith('peek.')).forEach(g => append(peeks, g));
    const weapons = el('optgroup'); weapons.label = '武器与射击矩阵'; unique.filter(g => g.startsWith('weapon.') || ['fire_weights', 'weapon_tuning'].includes(g)).forEach(g => append(weapons, g));
    this.groupSelect.append(main, peeks, weapons);
  };
  HumanBotsPanel.prototype.updateScope = function () {
    for (const b of this.scopes.children) { const selected = b.dataset.scope === this.scope; b.setAttribute('aria-pressed', String(selected)); b.classList.toggle('selected', selected); }
    this.scopeNote.textContent = {global: '选择固定值会同时删除该项的个体分布，保留单人覆盖；恢复继承只清全局覆盖。最终取值见每项配置预览。', preset: '编辑当前风格的默认数值。既有全局、分布和个体覆盖仍优先；可逐项恢复风格原值。', distribution: '设定所有人机的个体范围。按稳定编号确定取值；单个人机覆盖仍优先。', individual: `仅编辑人机 #${this.bot}；支持固定值、范围或继承。编号 0 是你，不参与编辑。`}[this.scope];
    if (this.roster()) {for (const b of this.scopes.children) if (b.dataset.scope === 'individual') b.textContent = '当前人机条目'; if (this.scope === 'individual') this.scopeNote.textContent = `仅编辑 ${this.entryName(this.selectedEntryId)} 的战斗参数；范围和草稿随本条保存，共享查询预算仍只允许全局编辑。`; if (this.scope === 'preset') this.scopeNote.textContent = '编辑全局基础风格的共用修改；名单行的独立预设与条目覆盖仍按其来源优先，不会替换条目设置。'; if (this.scope === 'distribution') this.scopeNote.textContent = '为名单设置能力范围，按条目身份固定采样；本条覆盖仍优先，重排或死亡重生不重抽能力。';}
    if (this.controlSaving) this.scopeNote.textContent = '正在保存风格或启用状态，请稍候再修改数值。未保存的参数修改会保留。';
  };
  HumanBotsPanel.prototype.scopeContext = function () { return this.scope === 'individual' ? this.roster() ? this.selectedEntryId || '' : String(this.bot) : this.scope === 'preset' ? this.settings().preset : ''; };
  HumanBotsPanel.prototype.draftId = function (key, scope = this.scope, context = this.scopeContext()) { return `${scope}|${context}|${key}`; };
  HumanBotsPanel.prototype.layer = function (settings = this.settings(), scope = this.scope, context = this.scopeContext()) {
    if (scope === 'global') return settings.global_overrides || {};
    if (scope === 'distribution') return settings.distribution || {};
    if (scope === 'individual') return this.roster(settings) ? this.entry(context, settings)?.overrides || {} : settings.individual_overrides?.[context] || {};
    return settings.preset_overrides?.[context] || {};
  };
  HumanBotsPanel.prototype.displayValue = function (param) {
    const layer = this.layer(); const value = layer[param.key]; const draft = this.drafts.get(this.draftId(param.key));
    if (draft) return draft;
    return {mode: own(layer, param.key) ? (typeof value === 'object' ? 'range' : 'fixed') : 'inherit', min: typeof value === 'object' ? String(value.min) : String(value ?? this.inheritedValue(param)), max: typeof value === 'object' ? String(value.max) : String(value ?? this.inheritedValue(param))};
  };
  HumanBotsPanel.prototype.inheritedValue = function (param) {
    const settings = this.settings(); const presets = [...this.presetCatalog, ...(this.spec.presets || [])]; const custom = settings.custom_presets?.[settings.preset]; const preset = presets.find(p => p.id === (custom?.base_preset || settings.preset)); const base = custom?.overrides?.[param.key] ?? preset?.overrides?.[param.key] ?? param.default;
    if (this.scope === 'individual' && this.roster()) {const entry = this.entry(); return this.previewInfo?.revision === this.config().revision && this.previewInfo.entry_id === entry?.id && this.previewInfo.hero_id === entry?.hero_id && Number.isFinite(this.previewValues[param.key]) ? this.previewValues[param.key] : '';}
    if (this.scope === 'preset') return base;
    let result = settings.preset_overrides?.[settings.preset]?.[param.key] ?? base;
    if (['distribution', 'individual'].includes(this.scope)) result = settings.global_overrides?.[param.key] ?? result;
    return result;
  };
  HumanBotsPanel.prototype.sourceDescription = function (param) {
    const sources = this.presentation?.source_by_group || {};
    const limitation = this.limitation(param);
    return limitation?.reason_zh || sources[param.group] || sources[param.key.split('.')[0]] || '该项的可靠数据来源尚未确认；目录条目不能作为动作已接入的证明。';
  };
  HumanBotsPanel.prototype.limitation = function (param) {
    return (this.presentation.limitations || []).find(rule => rule.keys?.includes(param.key) || rule.prefix && param.key.startsWith(rule.prefix));
  };
  HumanBotsPanel.prototype.behaviorDependencies = function (param) {
    return (this.presentation.behavior_dependencies || []).filter(rule => rule.keys?.includes(param.key) || rule.groups?.includes(param.group) || rule.prefixes?.some(prefix => param.key.startsWith(prefix))).map(rule => rule.text_zh);
  };
  HumanBotsPanel.prototype.capabilityEntries = function (caps) {
    if (!caps || typeof caps !== 'object') return new Map();
    this.capabilityCache ||= new WeakMap(); if (this.capabilityCache.has(caps)) return this.capabilityCache.get(caps);
    const result = new Map();
    const add = (id, entry, status) => {const param = this.byKey.get(String(id)) || this.byEnum.get(String(id)) || (Number.isInteger(id) ? this.params[id] : null); if (!param) return; const value = typeof entry === 'object' && entry ? {...entry} : {reason_zh: typeof entry === 'string' && status ? entry : ''}; value.status = status || (typeof entry === 'string' ? entry : entry?.status); value.reason_zh ||= caps.reason_zh; result.set(param.key, value);};
    for (const status of ['implemented', 'conditional', 'unsupported']) {
      const values = caps[status]; if (Array.isArray(values)) for (const value of values) add(value?.key ?? value, typeof value === 'object' ? value : {}, status);
      else for (const [key, value] of Object.entries(values || {})) add(key, value, status);
    }
    for (const [key, value] of Object.entries({...caps.parameters, ...caps.by_key})) add(key, value);
    this.capabilityCache.set(caps, result); return result;
  };
  HumanBotsPanel.prototype.capabilityReport = function (param) {
    const runtime = this.options.getRuntime?.() || {}; const native = runtime.game?.human_bots || runtime.human_bots || {};
    this.knownCapabilities ||= new Map();
    if (native.schema_digest === this.spec.schema_digest_sha256 && native.capabilities !== this.lastCapabilities) {
      for (const [key, info] of this.capabilityEntries(native.capabilities)) this.knownCapabilities.set(key, info);
      this.lastCapabilities = native.capabilities;
    }
    if (this.knownCapabilities.has(param.key)) return {info: this.knownCapabilities.get(param.key), source: '当前版本客户端的消费者报告'};
    const catalog = this.capabilityEntries(this.capabilities); if (catalog.has(param.key)) return {info: catalog.get(param.key), source: '控制台读取的消费者报告'};
    const reviewed = this.presentation.reviewed_capabilities;
    const entries = reviewed?.schema_digest === this.spec.schema_digest_sha256 ? this.capabilityEntries(reviewed) : new Map();
    return entries.has(param.key) ? {info: entries.get(param.key), source: '当前版本已核对的消费者清单'} : {info: null, source: '尚无可靠消费者报告'};
  };
  HumanBotsPanel.prototype.capability = function (param) {
    const report = this.capabilityReport(param); const info = report.info;
    let status = info?.status || param.consumer?.status || 'unknown'; if (!['implemented', 'conditional', 'unsupported', 'pending'].includes(status)) status = 'unknown';
    const limitation = this.limitation(param); const sourceConfirmed = info?.source_verified === true && typeof info.source_zh === 'string' && info.source_zh.trim();
    let reason = info?.reason_zh || info?.reason || (status === 'implemented' ? '已接入对应消费者；是否执行仍取决于实际感知和动作条件。' : status === 'conditional' ? param.condition_zh : '可靠消费者或数据来源尚未确认，当前锁定。');
    if (limitation && (!limitation.requires_source_confirmation || !sourceConfirmed)) {status = limitation.status; reason = limitation.reason_zh;}
    const source = info?.source_zh || this.sourceDescription(param);
    return {status, label: {implemented: '已接入', conditional: '按条件可用', unsupported: '当前不可用', pending: '来源待确认'}[status] || '来源待确认', reason, source, report: report.source, precision: info?.precision_zh || this.presentation.precision_zh || param.precision_zh};
  };
  HumanBotsPanel.prototype.lockReason = function (param) {
    if (this.controlSaving) return '正在保存风格或启用状态，请稍候。';
    if (!param.editable || param.type === 'constant') return '固定语义，不可修改。';
    if (param.scope === 'global_only' && ['individual', 'distribution'].includes(this.scope)) return '此项只允许全局或风格设置，不能分配给单个人机或设为分布。';
    if (this.scope === 'individual' && this.bot >= (this.config().total_players || 2)) return '该人机编号不在当前人数范围内。';
    if (this.scope === 'individual' && this.roster() && !this.entry()) return '此名单条目已被移除；原草稿仍可在修改审阅中放弃。';
    const capability = this.capability(param); if (!['implemented', 'conditional'].includes(capability.status)) return capability.reason;
    return '';
  };
  HumanBotsPanel.prototype.filtered = function () {
    const query = this.query; const filter = this.filters;
    return this.params.filter(p => {
      const detailed = this.query || this.group || Object.values(filter).some(Boolean);
      if (this.view === 'common' && !detailed && this.commonKeys.size && !this.commonKeys.has(p.key)) return false;
      if (this.group && p.group !== this.group) return false;
      if (query && !this.searchText.get(p.key).includes(query)) return false;
      const parts = p.key.split('.'); const matrix = parts[0] === 'fire_weights'; const tuning = parts[0] === 'weapon_tuning'; const band = parts[0] === 'weapon';
      if (filter.weapon && (!matrix && !tuning && !band || parts[1] !== filter.weapon)) return false;
      if (filter.distance && (!matrix && !tuning || parts[2] !== filter.distance)) return false;
      if (filter.context && (!matrix || parts[3] !== filter.context)) return false;
      if (filter.mode && (!matrix || parts[4] !== filter.mode)) return false;
      return true;
    });
  };
  HumanBotsPanel.prototype.renderList = function () {
    if (this.destroyed) return;
    const values = this.filtered(); const pageCount = Math.max(1, Math.ceil(values.length / this.pageSize)); this.page = Math.max(0, Math.min(this.page, pageCount - 1));
    const detailed = this.query || this.group || Object.values(this.filters).some(Boolean); const activeView = detailed ? 'all' : this.view;
    for (const b of this.views.children) {const selected = b.dataset.view === activeView; b.setAttribute('aria-pressed', String(selected)); b.classList.toggle('selected', selected);}
    this.count.textContent = `${activeView === 'common' ? '常用设置' : '全部细项'} · ${values.length} / ${this.params.length} 项${values.length ? ` · 显示 ${this.page * this.pageSize + 1}–${Math.min(values.length, (this.page + 1) * this.pageSize)}` : ' · 请调整搜索或筛选'}；修改会保留，最后统一保存。`;
    this.list.replaceChildren(); this.rows.clear();
    for (const param of values.slice(this.page * this.pageSize, (this.page + 1) * this.pageSize)) this.list.append(this.renderParam(param));
    if (!values.length) {const empty = el('div', 'hb-empty', '没有匹配参数。'); empty.append(button('清除搜索和筛选', () => {this.search.value = ''; this.query = ''; this.group = ''; this.groupSelect.value = ''; this.filters = {}; for (const select of this.matrixFilters.querySelectorAll('select')) select.value = ''; this.renderList();})); this.list.append(empty);}
    this.prev.disabled = this.page === 0; this.next.disabled = this.page >= pageCount - 1; this.pageLabel.textContent = `第 ${this.page + 1} / ${pageCount} 页`;
    this.updateDraftStatus(); this.updateScope();
  };
  HumanBotsPanel.prototype.renderParam = function (param) {
    const row = el('article', 'hb-param'); row.dataset.key = param.key; row.dataset.paramId = param.id;
    const info = el('div', 'hb-param-info'); const heading = el('div', 'hb-param-heading'); const name = el('h3', null, paramName(param)); const capability = this.capability(param); const badge = el('span', `hb-badge hb-${capability.status}`, capability.label); badge.title = capability.reason; heading.append(name, badge);
    const desc = el('p', 'hb-description', this.descriptions.get(param.key) || param.description_zh); info.append(heading, desc);
    const preview = el('p', 'hb-preview'); preview.dataset.preview = param.key; this.updatePreviewLabel(preview, param); info.append(preview);
    const metadata = el('details', 'hb-metadata'); metadata.append(el('summary', null, '范围、条件与生效说明'));
    const dl = el('dl');
    const dependencyNames = (param.depends_on || []).map(key => {const item = this.byKey.get(key) || this.byEnum.get(key); return item ? paramName(item) : key;});
    const constraints = this.constraints.get(param.key) || []; const behaviorDependencies = this.behaviorDependencies(param);
    const pairs = [['单位', unitNames[param.unit] || param.unit || '无'], ['合法范围', `${fmt(param.min)} – ${fmt(param.max)}${['int', 'enum', 'mask', 'toggle'].includes(param.type) ? '（整数）' : ''}`], ['内置默认', fmt(param.default)], ['适用条件', param.condition_zh || '无额外条件'], ['动作依赖', dependencyNames.join('、') || '无额外启用依赖；仍须满足适用条件。'], ['数值依赖', constraints.length ? `${constraints.join('；')}。固定值的上下界相同；范围按最坏边界整批校验，失败不会自动改动其他参数。` : '无额外跨项数值约束。'], ['生效时机', param.effect === 'live_next_decision' ? this.presentation.effect_zh || '客户端接收且行为模块采用配置后，从后续决策开始；不会重抽重生能力。' : param.effect || '待确认'], ['精度限制', capability.precision || '受真实感知、决策与动作频率限制。'], ['数据来源', capability.source], ['能力依据', capability.report], ['能力状态', capability.reason], ['参数', `#${param.id} · ${param.key}`]];
    if (param.enum_values) pairs.splice(3, 0, ['编号含义', Object.entries(param.enum_values).map(([v, label]) => `${v}：${label}`).join('；')]);
    if (behaviorDependencies.length) pairs.splice(pairs.length - 1, 0, ['行为依赖', behaviorDependencies.join(' ')]);
    const metadataValues = {};
    for (const [label, value] of pairs) {const node = el('dd', null, value); metadataValues[label] = node; dl.append(el('dt', null, label), node);} metadata.append(dl);
    if (param.type === 'mask') {metadata.append(el('p', null, '0 表示不加此类情境限制。各条件编号可相加组合；“任一”“全部”“排除”按本参数名称解释。')); const bits = el('p', 'hb-mask-legend'); bits.textContent = Object.entries(this.spec.condition_bits || {}).map(([key, bit]) => `${conditionNames[key] || key} ${bit}`).join(' · '); metadata.append(bits);}
    info.append(metadata);
    const controls = el('div', 'hb-param-controls'); const mode = el('select'); mode.setAttribute('aria-label', `${paramName(param)}的覆盖方式`); mode.dataset.role = 'mode'; mode.append(option('inherit', '继承'));
    const fixedOption = this.scope !== 'distribution' ? option('fixed', '固定值') : null; if (fixedOption) mode.append(fixedOption);
    if (['individual', 'distribution'].includes(this.scope)) mode.append(option('range', '范围'));
    const min = this.numericInput(param, '数值 / 最小值'); const max = this.numericInput(param, '最大值'); max.dataset.role = 'max'; min.dataset.role = 'min';
    const range = el('div', 'hb-numbers'); const separator = el('span', null, '至'); range.append(min, separator, max);
    const unit = el('small', 'hb-unit', `${unitNames[param.unit] || param.unit} · ${fmt(param.min)}–${fmt(param.max)}`); unit.id = `hb-${param.id}-unit`;
    const error = el('small', 'hb-field-error'); error.id = `hb-${param.id}-error`; error.hidden = true;
    const reason = el('small', 'hb-lock-reason'); reason.id = `hb-${param.id}-reason`; const layerNote = el('small', 'hb-layer-note'); layerNote.id = `hb-${param.id}-layers`;
    for (const input of [mode, min, max]) input.setAttribute('aria-describedby', `${unit.id} ${layerNote.id} ${reason.id} ${error.id}`);
    controls.append(mode, range, unit, layerNote, reason, error);
    const current = this.displayValue(param); const setValues = () => {mode.value = current.mode; min.value = current.min; max.value = current.max;}; setValues();
    const state = {param, row, mode, min, max, separator, error, reason, layerNote, preview, badge, capability, metadataValues, fixedOption, dirty: false}; this.rows.set(param.key, state);
    mode.addEventListener('change', () => {this.capture(state); this.updateRow(state);});
    for (const input of [min, max]) {input.addEventListener('input', () => {this.capture(state); this.validateRow(state); this.updateDraftStatus();}); input.addEventListener('keydown', event => {if (event.key === 'Enter') {event.preventDefault(); this.saveDrafts();}});}
    row.append(info, controls); this.updateRow(state); return row;
  };
  HumanBotsPanel.prototype.numericInput = function (param, suffix) {
    const input = el('input'); input.type = 'number'; input.inputMode = ['int', 'mask', 'enum', 'toggle', 'constant'].includes(param.type) ? 'numeric' : 'decimal'; input.step = ['int', 'mask', 'enum', 'toggle', 'constant'].includes(param.type) ? '1' : 'any'; input.min = param.min; input.max = param.max; input.setAttribute('aria-label', `${paramName(param)}：${suffix}`); return input;
  };
  HumanBotsPanel.prototype.capture = function (state) {
    if (this.lockReason(state.param)) return;
    const key = state.param.key; const id = this.draftId(key); const draft = {id, key, scope: this.scope, context: this.scopeContext(), mode: state.mode.value, min: state.min.value, max: state.max.value, version: ++this.version};
    this.drafts.set(id, draft); state.row.classList.add('hb-dirty'); this.updateDraftStatus();
  };
  HumanBotsPanel.prototype.updateRow = function (state) {
    const capability = this.capability(state.param); state.capability = capability;
    state.badge.className = `hb-badge hb-${capability.status}`; state.badge.textContent = capability.label; state.badge.title = capability.reason;
    state.metadataValues['数据来源'].textContent = capability.source; state.metadataValues['能力依据'].textContent = capability.report;
    state.metadataValues['能力状态'].textContent = capability.reason; state.metadataValues['精度限制'].textContent = capability.precision || state.param.precision_zh;
    const reason = this.lockReason(state.param); const range = state.mode.value === 'range';
    state.mode.disabled = !!reason; state.min.disabled = !!reason || state.mode.value === 'inherit'; state.max.disabled = !!reason || !range;
    state.row.classList.toggle('hb-range', range);
    state.max.hidden = !range; state.separator.hidden = !range; state.reason.textContent = reason; state.reason.hidden = !reason;
    const settings = this.settings(); const notes = [];
    if (state.fixedOption) state.fixedOption.textContent = this.scope === 'global' && own(settings.distribution, state.param.key) ? '固定值 · 删除同项分布' : '固定值';
    if (this.scope === 'global' && state.mode.value === 'fixed' && own(settings.distribution, state.param.key)) notes.push('保存固定值将同时删除此项个体分布。');
    else if (this.scope === 'global' && own(settings.distribution, state.param.key)) notes.push('此项已有个体分布，优先于全局值；选择固定值可替换该分布。');
    if (this.scope !== 'individual' && own(settings.individual_overrides?.[String(this.bot)], state.param.key)) notes.push(`人机 #${this.bot} 的单人覆盖仍优先；请在“单个人机”中修改或恢复继承。`);
    state.layerNote.textContent = notes.join(' '); state.layerNote.hidden = !notes.length;
    state.row.classList.toggle('hb-dirty', this.drafts.has(this.draftId(state.param.key))); this.validateRow(state);
  };
  HumanBotsPanel.prototype.valueError = function (draft) {
    if (draft.mode === 'inherit') return '';
    const param = this.byKey.get(draft.key); if (!param) return '参数不存在。';
    const capability = this.capability(param); if (!param.editable || param.type === 'constant') return '固定语义，不可修改。';
    if (!['implemented', 'conditional'].includes(capability.status)) return capability.reason;
    for (const raw of draft.mode === 'range' ? [draft.min, draft.max] : [draft.min]) {
      if (String(raw).trim() === '') return '请输入数值。'; const value = Number(raw);
      if (!Number.isFinite(value)) return '数值必须有限。';
      if (value < param.min || value > param.max) return `合法范围为 ${fmt(param.min)}–${fmt(param.max)}。`;
      if (['int', 'enum', 'mask', 'toggle', 'constant'].includes(param.type) && !Number.isInteger(value)) return '此项必须是整数。';
      if (param.enum_values && !own(param.enum_values, String(value))) return '请选择说明中的有效阶段编号。';
    }
    if (draft.mode === 'range' && +draft.min > +draft.max) return '最小值不能大于最大值。'; return '';
  };
  HumanBotsPanel.prototype.validateRow = function (state) {
    const draft = this.drafts.get(this.draftId(state.param.key)); const error = draft ? this.valueError(draft) : '';
    state.error.hidden = !error; state.error.textContent = error; state.min.setAttribute('aria-invalid', String(!!error)); state.max.setAttribute('aria-invalid', String(!!error)); return !error;
  };
  HumanBotsPanel.prototype.updateDraftStatus = function () {
    const count = this.drafts.size; this.draftLabel.textContent = count ? `${count} 项未保存` : '没有未保存修改'; this.saveButton.textContent = this.saving ? '正在保存…' : `保存修改${count ? `（${count}）` : ''}`;
    this.saveButton.disabled = this.saving || !!this.controlSaving || !count; this.discardButton.disabled = this.saving || !count; this.clearButton.disabled = this.saving || !!this.controlSaving;
    for (const state of this.rows.values()) this.updatePreviewLabel(state.preview, state.param);
    this.renderReview();
    this.refreshSpectator();
  };
  HumanBotsPanel.prototype.renderReview = function (force = false) {
    if (!this.review) return;
    const tag = `${this.version}|${this.drafts.size}|${this.saving}|${this.config().revision}`; if (!force && this.reviewVersion === tag) return; this.reviewVersion = tag;
    const drafts = [...this.drafts.values()].sort((a, b) => a.version - b.version); this.review.hidden = !drafts.length; this.reviewSummary.textContent = `检查未保存修改（${drafts.length}）`;
    const pages = Math.max(1, Math.ceil(drafts.length / 24)); this.reviewPage = Math.max(0, Math.min(this.reviewPage, pages - 1)); this.reviewList.replaceChildren();
    for (const draft of drafts.slice(this.reviewPage * 24, (this.reviewPage + 1) * 24)) {
      const row = el('li'); const param = this.byKey.get(draft.key); const context = draft.scope === 'individual' ? this.roster() ? ` · ${this.entryName(draft.context)}` : ` #${draft.context}` : draft.scope === 'preset' ? ` · ${(this.spec.presets || []).find(p => p.id === draft.context)?.name_zh || '内置默认'}` : '';
      const value = draft.mode === 'inherit' ? '恢复继承' : draft.mode === 'range' ? `${draft.min || '空'} – ${draft.max || '空'}` : draft.min || '空';
      const linked = draft.scope === 'global' && draft.mode === 'fixed' && own(this.settings().distribution, draft.key) ? '；同时删除此项个体分布' : '';
      row.append(el('span', null, `${scopes[draft.scope]}${context} · ${param ? paramName(param) : draft.key}：${value}${linked}`));
      const discard = button('放弃此项', () => {this.drafts.delete(draft.id); this.renderList(); this.reviewSummary.focus();}); discard.disabled = this.saving; row.append(discard); this.reviewList.append(row);
    }
    this.reviewPrev.disabled = this.reviewPage === 0; this.reviewNext.disabled = this.reviewPage >= pages - 1; this.reviewPageLabel.textContent = `${this.reviewPage + 1} / ${pages}`;
  };
  HumanBotsPanel.prototype.clearScope = function () {
    const map = this.layer(); const keys = new Set(Object.keys(map));
    for (const draft of this.drafts.values()) if (draft.scope === this.scope && draft.context === this.scopeContext()) keys.add(draft.key);
    let count = 0; for (const key of keys) {const param = this.byKey.get(key); if (!param || (!param.editable || param.type === 'constant')) continue; const id = this.draftId(key); this.drafts.set(id, {id, key, scope: this.scope, context: this.scopeContext(), mode: 'inherit', min: String(param.default), max: String(param.default), version: ++this.version}); count++;}
    if (!count) this.showError('当前范围没有需要清除的覆盖。'); else {this.error.hidden = true; this.renderList();}
  };
  // Only fields actually edited are sent. Null tombstones are required by map merge.
  HumanBotsPanel.prototype.makePatch = function (drafts, latest) {
    const human = {};
    for (const draft of [...drafts].sort((a, b) => a.version - b.version)) {
      const param = this.byKey.get(draft.key); if (!param) throw new Error('参数已更新，请重新载入。');
      const roster = latest.human_bots?.bot_roster;
      if (draft.scope === 'individual' && !Array.isArray(roster) && (+draft.context < 1 || +draft.context >= latest.total_players)) throw new Error(`人机 #${draft.context} 已不在本局人数范围内，请放弃该项或恢复人数后保存。`);
      if (['individual', 'distribution'].includes(draft.scope) && param.scope === 'global_only') throw new Error(`${param.name_zh} 只允许全局设置。`);
      const value = draft.mode === 'inherit' ? null : draft.mode === 'range' ? {min: +draft.min, max: +draft.max} : +draft.min;
      if (draft.scope === 'global') {human.global_overrides ||= {}; human.global_overrides[draft.key] = value; if (draft.mode === 'fixed' && param.scope === 'bot') {human.distribution ||= {}; human.distribution[draft.key] = null;}}
      else if (draft.scope === 'distribution') {human.distribution ||= {}; human.distribution[draft.key] = value;}
      else if (draft.scope === 'individual' && Array.isArray(roster)) {
        human.bot_roster ||= clone(roster); const entry = human.bot_roster.find(item => item.id === draft.context); if (!entry) throw new Error('所编辑的人机条目已被移除，草稿不会转给其他人机；请放弃该项或恢复条目。');
        entry.overrides ||= {}; if (value == null) delete entry.overrides[draft.key]; else entry.overrides[draft.key] = value;
      } else {const map = draft.scope === 'individual' ? 'individual_overrides' : 'preset_overrides'; human[map] ||= {}; human[map][draft.context] ||= {}; human[map][draft.context][draft.key] = value;}
    }
    // Do not copy settings, seed or revision into this patch. The caller supplies CAS.
    return {human_bots: human};
  };
  HumanBotsPanel.prototype.showError = function (message) {this.error.hidden = false; this.error.textContent = message; this.options.onError?.(message);};
  HumanBotsPanel.prototype.saveDrafts = async function () {
    if (this.saving || !this.drafts.size) return;
    const drafts = [...this.drafts.values()].map(clone); for (const draft of drafts) {const error = this.valueError(draft); if (error) return this.showError(`${this.byKey.get(draft.key)?.name_zh || draft.key}：${error}`);}
    this.saving = true; this.error.hidden = true; this.updateDraftStatus();
    try {
      const result = await this.options.save(latest => this.makePatch(drafts, latest)); if (!result) throw new Error('保存未获确认。修改仍保留，请检查连接后重试。');
      for (const draft of drafts) if (this.drafts.get(draft.id)?.version === draft.version) this.drafts.delete(draft.id);
      this.previewKey = ''; this.refresh();
    } catch (error) {this.showError(`未保存：${error.message || error}。未保存修改已保留。`);}
    finally {this.saving = false; this.updateDraftStatus();}
  };
  HumanBotsPanel.prototype.persistControl = async function (patch) {
    this.error.hidden = true; this.controlSaving++; this.refresh();
    try {const result = await this.options.save(() => ({human_bots: patch})); if (!result) throw new Error('保存未获确认');}
    catch (error) {this.showError(`未保存：${error.message || error}`);}
    finally {this.controlSaving--; this.refresh(); this.renderList();}
  };
  HumanBotsPanel.prototype.spectatorSessionKey = function (runtime) {return `${runtime.session || ''}|${runtime.host_pid ?? runtime.pid ?? ''}`;};
  HumanBotsPanel.prototype.spectatorTargetReason = function (index, runtime) {
    if (!Number.isInteger(index) || index < 1) return '该人机编号不在当前对局范围内。';
    const roster = runtime.game?.configured_bot_roster;
    if (roster?.loaded === true && roster.fixed === true && roster.validated === true && Array.isArray(roster.entries)) {
      if (roster.entries.filter(entry => entry.slot === index).length !== 1) return '该人机编号不在当前对局名单中。';
    } else if (index >= this.config().total_players) return '该人机编号不在当前人数范围内。';
    if (this.roster()) {const snapshot = runtime.game?.configured_bot_roster; if (snapshot?.native_catalog_verified !== true) return '本局名单身份尚未由原生目录确认。'; const actual = snapshot.entries?.filter(entry => entry.slot === index); if (actual?.length !== 1 || !this.roster().some(entry => entry.id === actual[0].entry_token && entry.hero_id === actual[0].hero_id)) return '本局没有相同条目与英雄的唯一映射；请手动重启后观战。'; if (!Number.isInteger(actual[0].native_player_id) || actual[0].native_player_id <= 0) return '该条目的原角色尚未在本局分配，等待出场后观战。';}
    return this.options.spectatorTargetReason?.(index, this.config(), runtime) || '';
  };
  HumanBotsPanel.prototype.configurationAdopted = function (runtime, native) {
    const config = this.config(); const fingerprint = native.transport_fingerprint;
    return runtime.status === 'running' && config.mode === 'deathmatch' && this.settings().enabled === true && native.enabled === true
      && native.schema_digest === this.spec.schema_digest_sha256 && native.transport_applied_revision === config.revision
      && native.consumer_effective_known === true && native.configuration_effective === true && native.consumer_effective_revision === config.revision
      && typeof fingerprint === 'string' && /^[0-9a-f]{16}$/i.test(fingerprint) && fingerprint === native.consumer_effective_fingerprint;
  };
  HumanBotsPanel.prototype.selectedBehaviorProfile = function (native, index = this.bot) {
    const records = native.behavior?.bots; if (!Array.isArray(records)) return null;
    const stableId = Array.isArray(native.stable_bot_ids) ? native.stable_bot_ids[index] : index;
    if (!Number.isInteger(stableId) || stableId < 1) return null;
    const matches = records.filter(record => record && (record.index === index || record.stable_bot_id === stableId));
    const record = matches.length === 1 ? matches[0] : null;
    return record?.index === index && record.stable_bot_id === stableId && record.configured === true && Number.isInteger(record.profile_revision) && record.profile_revision >= 0 ? record : null;
  };
  HumanBotsPanel.prototype.requestProfileIdentity = function (index) {
    const config = this.config(); const key = `${config.revision}|${index}`; this.profileIdentities ||= new Map(); this.profileIdentityRequests ||= new Map();
    if (this.profileIdentities.has(key) || this.profileIdentityRequests.has(key)) return;
    const request = (this.profileIdentitySequence || 0) + 1; this.profileIdentitySequence = request; this.profileIdentityRequests.set(key, request); const session = this.spectatorSessionKey(this.options.getRuntime?.() || {});
    const read = this.options.preview ? () => this.options.preview(index, config) : () => fetch(`/api/human-bots/preview?index=${index}`, {cache: 'no-store'}).then(async response => {const data = await response.json(); if (!response.ok) throw new Error(data.error || '读取失败'); return data;});
    Promise.resolve().then(read).then(result => {
      if (this.destroyed || session !== this.spectatorSessionKey(this.options.getRuntime?.() || {}) || this.config().revision !== config.revision) return;
      if (result.source !== 'configuration_preview' || result.revision !== config.revision || result.stable_bot_id !== index) throw new Error('参数身份预览不匹配');
      this.profileIdentities.set(key, result);
    }).catch(() => {if (!this.destroyed && session === this.spectatorSessionKey(this.options.getRuntime?.() || {}) && this.config().revision === config.revision) this.profileIdentities.set(key, {unavailable: true});}).finally(() => {if (this.profileIdentityRequests.get(key) === request) this.profileIdentityRequests.delete(key); if (!this.destroyed) this.refreshSpectator();});
  };
  HumanBotsPanel.prototype.appliedProfileText = function (runtime, native, index = this.bot) {
    const report = this.selectedBehaviorProfile(native, index); const prefix = `人机 #${index}`;
    if (runtime.status !== 'running' || !report) return `${prefix} · 实际采用参数等待行为报告；已保存配置预览见下方。`;
    const fingerprint = typeof report.profile_fingerprint === 'string' && /^[0-9a-f]{16}$/i.test(report.profile_fingerprint) ? report.profile_fingerprint.toLowerCase() : null;
    const current = this.config(); const preview = index === this.bot ? this.previewInfo : this.profileIdentities?.get(`${current.revision}|${index}`);
    const matches = this.configurationAdopted(runtime, native) && fingerprint && preview?.revision === current.revision
      && preview.stable_bot_id === report.stable_bot_id && preview.consumer_effective === true
      && preview.profile_fingerprint?.toLowerCase() === fingerprint && report.profile_revision === current.revision;
    const revision = `行为报告 rev ${report.profile_revision} · Profile ${fingerprint ? fingerprint.slice(0, 8) : '待确认'}`;
    if (!matches) return `${prefix} · ${revision} · ${report.profile_revision < current.revision ? `本页已保存 rev ${current.revision}，风格等待新配置采用` : '风格等待版本与指纹确认'}。`;
    const settings = this.settings(); const entry = settings.bot_roster?.[index - 1]; const individual = settings.individual_presets?.[String(index)];
    if (entry) {const snapshot = runtime.game?.configured_bot_roster; const actual = snapshot?.entries?.filter(item => item.slot === index); if (snapshot?.native_catalog_verified !== true || actual?.length !== 1 || actual[0].entry_token !== entry.id || actual[0].hero_id !== entry.hero_id) return `${prefix} · ${revision} · 风格等待名单身份确认。`;}
    const presetId = entry?.preset_id || (typeof individual === 'string' ? individual : settings.preset);
    const presets = this.presetCatalog?.length ? this.presetCatalog : this.spec.presets || [];
    const preset = presets.find(item => item.id === presetId) || (this.spec.presets || []).find(item => item.id === presetId); const name = settings.custom_presets?.[presetId]?.name || preset?.name_zh || (presetId === 'default' ? '内置默认' : '目录待确认');
    return `${prefix} · 已采用${entry ? '名单条目' : individual ? '个体' : '全局'}风格来源：${name}（可含覆盖） · rev ${report.profile_revision} · Profile ${fingerprint.slice(0, 8)}。`;
  };
  HumanBotsPanel.prototype.refreshSpectator = function () {
    if (!this.spectatorBar || this.destroyed) return;
    const runtime = this.options.getRuntime?.() || {}; const config = this.config(); const native = runtime.game?.human_bots || runtime.human_bots || {};
    const key = this.spectatorSessionKey(runtime);
    if (this.spectatorSession != null && this.spectatorSession !== key) {this.spectatorRequest = null; this.lastSpectatorTarget = 0; this.spectatorHasTarget = false; this.profileIdentities?.clear(); this.profileIdentityRequests?.clear(); this.spectatorErrorText = '';}
    this.spectatorSession = key;
    const available = runtime.status === 'running' && runtime.backend === 'local-server-candidate'; const editorTarget = this.roster() ? this.rosterMapping(this.selectedEntryId, runtime)?.slot : this.bot;
    const targetReason = editorTarget == null ? '此条尚未以相同英雄在本局出场；修改名单后需重启。' : this.spectatorTargetReason(editorTarget, runtime); const valid = !targetReason;
    if (this.profileIdentityRevision !== config.revision) {this.profileIdentities?.clear(); this.profileIdentityRevision = config.revision;}
    const state = runtime.game?.spectator; const fresh = state?.state_available === true && state.stale === false && state.state_source === 'owned_client';
    const requested = fresh && Number.isInteger(state.requested_index) && state.requested_index >= 0 ? state.requested_index : null;
    if (requested != null) this.lastSpectatorTarget = requested;
    const rawTarget = fresh ? state.matched_target : null;
    const target = rawTarget && Number.isInteger(rawTarget.selected_index) && rawTarget.selected_index > 0 && Number.isInteger(rawTarget.hero_index) && rawTarget.hero_index >= 0
      && Number.isInteger(rawTarget.native_player_id) && rawTarget.native_player_id > 0 && rawTarget.native_player_id === state.native_player_id ? rawTarget : null;
    const active = fresh && state.active === true; const waiting = fresh && state.waiting_for_respawn === true;
    const returned = fresh && state.active === false && state.own_view_confirmed === true;
    if (active || waiting) this.spectatorHasTarget = true; else if (returned) this.spectatorHasTarget = false;
    if (this.spectatorRequest && (this.spectatorRequest.index === target?.selected_index && (active || waiting) || this.spectatorRequest.index === 0 && returned)) this.spectatorRequest = null;
    const pending = this.spectatorRequest; const timedOut = pending && Date.now() - pending.at > 15000;
    const heroes = this.options.getHeroes?.() || []; const hero = target ? heroes.filter(item => item.index === target.hero_index) : [];
    const targetText = target ? `人机 #${target.selected_index}${hero.length === 1 ? ` · ${hero[0].name_zh || hero[0].name || hero[0].id}` : ' · 英雄信息待确认'}` : '当前原生目标（身份待确认）';
    let text = !available ? '训练开始后可观战所选人机。' : !fresh ? '观战状态暂未读取，仍可提交观战请求。' : waiting ? `等待${targetText}重生 · 保持原生观战目标` : active ? `正在观战${targetText}` : returned ? '已返回自己视角' : requested > 0 ? '等待原角色视角确认' : '当前未确认观战其他人机';
    if (pending) {const requestText = timedOut ? '最近请求尚未匹配，可重试' : `${pending.index === 0 ? '已请求返回自己' : `已请求观战人机 #${pending.index}`} · 等待原生状态确认`; text = active || waiting ? `${text}；${requestText}` : requestText;}
    if (this.spectatorSubmitting) text = '正在提交观战请求…';
    this.spectatorState.textContent = text;
    const profileIndex = active || waiting ? target?.selected_index : this.bot;
    if (profileIndex != null && profileIndex !== this.bot) this.requestProfileIdentity(profileIndex);
    this.spectatorProfile.textContent = profileIndex == null ? '实际观战对象档案等待唯一身份映射；不会按最近请求编号猜测。' : `${active || waiting ? '观战对象' : '编辑对象'} · ${this.appliedProfileText(runtime, native, profileIndex)}`;
    this.spectatorNote.textContent = `编辑人机 #${this.bot}；切换编号不会自动切镜头。原生左右键切人后，以实际目标为准。${this.drafts.size ? '未保存草稿不生效，观战不会替你保存。' : '观战不修改能力参数、种子或配置版本。'}`;
    this.spectatorError.hidden = !this.spectatorErrorText; this.spectatorError.textContent = this.spectatorErrorText;
    this.spectateButton.disabled = !available || !valid || this.spectatorSubmitting || !!pending && !timedOut || target?.selected_index === editorTarget && (active || waiting);
    this.returnButton.disabled = !available || this.spectatorSubmitting || !(requested > 0 || this.lastSpectatorTarget > 0 || pending?.index > 0 || this.spectatorHasTarget);
    this.spectateButton.title = !available ? '仅训练中的本地服务器可观战。' : !valid ? targetReason : `观战所选人机；不保存参数草稿。`;
    this.spectatorProfile.title = profileIndex == null ? '身份待确认' : this.selectedBehaviorProfile(native, profileIndex)?.profile_fingerprint || '完整实际指纹等待行为报告';
  };
  HumanBotsPanel.prototype.requestSpectator = async function (index) {
    if (this.spectatorSubmitting || this.destroyed) return;
    const runtime = this.options.getRuntime?.() || {}; const session = this.spectatorSessionKey(runtime);
    if (runtime.status !== 'running' || runtime.backend !== 'local-server-candidate' || !Number.isInteger(index) || index < 0) {this.spectatorErrorText = '仅当前训练中的有效人机可观战；不会保存为启动后的请求。'; this.refreshSpectator(); return;}
    if (index > 0) {const reason = this.spectatorTargetReason(index, runtime); if (reason) {this.spectatorErrorText = reason; this.refreshSpectator(); return;}}
    this.spectatorSubmitting = true; this.spectatorErrorText = ''; this.refreshSpectator();
    try {
      const result = this.options.spectateBot ? await this.options.spectateBot(index) : await fetch('/api/spectate-bot', {method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({roster_index: index})}).then(async response => {const data = await response.json(); if (!response.ok) throw new Error(data.error || '观战请求失败'); return data;});
      if (this.destroyed) return;
      if (this.spectatorSessionKey(this.options.getRuntime?.() || {}) !== session) throw new Error('训练会话已变更，请在新会话中重新选择。');
      if (result?.status === 'saved') throw new Error('训练未就绪；观战请求不会保存或排到启动后执行。');
      if (!result || result.acknowledged !== true || !Number.isInteger(result.code) || result.code < 0 || result.code > 1) throw new Error(result?.message_text || result?.message || '观战请求未获确认；不会排队到启动后执行。');
      this.spectatorRequest = {index, at: Date.now()}; if (index > 0) this.lastSpectatorTarget = index;
    } catch (error) {this.spectatorErrorText = `观战请求未确认：${error.message || error}`;}
    finally {this.spectatorSubmitting = false; this.refreshSpectator();}
  };
  HumanBotsPanel.prototype.updatePreviewLabel = function (node, param) {
    if (this.bot >= (this.config().total_players || 2)) {node.textContent = `人机 #${this.bot} 已不在本局人数范围内，请选择其他人机。`; return;}
    const value = this.previewValues[param.key]; const settings = this.settings();
    let source = '内置默认'; if ((this.spec.presets || []).find(p => p.id === settings.preset)?.overrides?.[param.key] != null) source = '风格预设';
    if (own(settings.preset_overrides?.[settings.preset], param.key)) source = '风格修改';
    if (own(settings.global_overrides, param.key)) source = '全局固定';
    if (own(settings.distribution, param.key)) source = '个体分布';
    if (own(settings.individual_overrides?.[String(this.bot)], param.key)) source = '单个人机';
    if (this.roster()) {if (this.previewSources?.[param.key]) source = this.previewSources[param.key]; else if (own(this.entry()?.overrides, param.key)) source = '当前条目覆盖'; else source = '当前条目继承（等待来源确认）';}
    node.textContent = `人机 #${this.bot} 已保存配置预览：${value == null ? '未读取' : fmt(value)} · 来源：${source}${this.drafts.size ? ' · 未保存草稿不生效' : ''}`;
  };
  HumanBotsPanel.prototype.syncInheritedControls = function () {
    if (this.scope !== 'individual' || !this.roster()) return;
    for (const state of this.rows.values()) if (state.mode.value === 'inherit' && !this.drafts.has(this.draftId(state.param.key))) {const value = this.inheritedValue(state.param); state.min.value = String(value); state.max.value = String(value);}
  };
  HumanBotsPanel.prototype.requestPreview = async function (force = false) {
    const config = this.config(); const entry = this.entry(); const key = `${config.revision}|${entry?.id || this.bot}`; if (!force && key === this.previewKey) return; this.previewKey = key; const sequence = ++this.previewSeq; this.previewValues = {}; this.previewInfo = null; this.previewSources = {}; this.refreshSpectator();
    this.syncInheritedControls();
    for (const state of this.rows.values()) this.updatePreviewLabel(state.preview, state.param);
    if (this.bot < 1 || this.bot >= (config.total_players || 2)) {for (const state of this.rows.values()) state.preview.textContent = `人机 #${this.bot} 已不在本局人数范围内，请选择其他人机。`; return;}
    try {
      const result = entry ? this.options.previewEntry ? await this.options.previewEntry(entry.id, config) : await fetch('/api/human-bots/preview', {method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({entry_id: entry.id})}).then(async response => {const data = await response.json(); if (!response.ok) throw new Error(data.error || '读取失败'); return data;}) : this.options.preview ? await this.options.preview(this.bot, config) : await fetch(`/api/human-bots/preview?index=${this.bot}`, {cache: 'no-store'}).then(async response => {const data = await response.json(); if (!response.ok) throw new Error(data.error || '读取失败'); return data;});
      if (this.destroyed || sequence !== this.previewSeq) return;
      if (result.source !== 'configuration_preview') throw new Error('预览来源未确认');
      if (result.revision !== config.revision || entry && (result.entry_id !== entry.id || result.hero_id !== entry.hero_id) || !entry && result.stable_bot_id != null && result.stable_bot_id !== this.bot) throw new Error('预览配置已更新，等待下次同步');
      if (Array.isArray(result.values)) this.previewValues = Object.fromEntries(this.params.filter(p => Number.isFinite(result.values[p.id])).map(p => [p.key, result.values[p.id]]));
      else this.previewValues = result.values || {};
      this.previewSources = result.sources || {};
      this.previewInfo = {revision: result.revision, entry_id: result.entry_id, hero_id: result.hero_id, stable_bot_id: result.stable_bot_id, profile_fingerprint: result.profile_fingerprint, consumer_effective: result.consumer_effective === true};
      this.profileIdentities ||= new Map(); this.profileIdentities.set(`${config.revision}|${this.bot}`, this.previewInfo);
      this.syncInheritedControls();
      for (const state of this.rows.values()) this.updatePreviewLabel(state.preview, state.param);
    } catch (error) {if (sequence === this.previewSeq && !this.destroyed) for (const state of this.rows.values()) state.preview.textContent = `人机 #${this.bot} 已保存配置预览暂不可用；参数仍可保存。`;}
    finally {if (sequence === this.previewSeq && !this.destroyed) this.refreshSpectator();}
  };
  HumanBotsPanel.prototype.refresh = function () {
    if (this.destroyed) return;
    const config = this.config(); const settings = this.settings(); const runtime = this.options.getRuntime?.() || {}; const native = runtime.game?.human_bots || runtime.human_bots || {};
    const changedEntry = this.refreshRoster(); if (changedEntry) this.renderList();
    if (native.capabilities) {this.capabilityCache?.delete(native.capabilities); this.lastCapabilities = null;}
    const maxBot = Math.max(1, (config.total_players || 2) - 1); const rangeShape = `${maxBot}|${this.bot > maxBot ? this.bot : ''}`;
    if (this.botSelect.dataset.rangeShape !== rangeShape) {this.botSelect.dataset.rangeShape = rangeShape; this.botSelect.replaceChildren(); for (let i = 1; i <= maxBot; i++) this.botSelect.append(option(i, `人机 #${i}`)); if (this.bot > maxBot) {const invalid = option(this.bot, `人机 #${this.bot} · 已不在本局`); invalid.disabled = true; this.botSelect.append(invalid);}}
    if (document.activeElement !== this.botSelect) this.botSelect.value = this.bot;
    if (document.activeElement !== this.enabled) this.enabled.checked = !!settings.enabled;
    if (document.activeElement !== this.presetSelect) this.presetSelect.value = settings.preset;
    this.enabled.disabled = !!this.controlSaving; this.presetSelect.disabled = !!this.controlSaving;
    const running = runtime.status === 'running'; const applied = native.transport_applied_revision ?? native.config_applied_revision; const effective = native.consumer_effective_revision; const revision = config.revision;
    const expectedDigest = this.spec.schema_digest_sha256; const digestMatches = !!expectedDigest && native.schema_digest === expectedDigest;
    const digestMismatch = !!native.schema_digest && !digestMatches;
    const fingerprint = native.transport_fingerprint; const fingerprintMatches = typeof fingerprint === 'string' && /^[0-9a-f]{16}$/i.test(fingerprint) && fingerprint === native.consumer_effective_fingerprint;
    const confirmed = running && config.mode === 'deathmatch' && settings.enabled && native.enabled === true && digestMatches && applied === revision && native.consumer_effective_known === true && native.configuration_effective === true && effective === revision && fingerprintMatches;
    this.status.classList.toggle('hb-effective', !!confirmed);
    this.statusMessage.textContent = !settings.enabled ? '拟人人机未启用' : config.mode !== 'deathmatch' ? '已保存 · 在个人乱斗中采用' : !running ? '已保存 · 启动后等待应用' : digestMismatch ? '配置版本不兼容 · 行为生效未确认' : confirmed ? '行为模块已确认当前配置' : applied === revision ? '配置已接收 · 行为生效尚未确认' : '已保存 · 等待客户端接收';
    if (this.rosterPresetChoices.size) {
      if (confirmed) this.statusMessage.textContent = '已保存配置已采用';
      this.statusMessage.textContent += this.rosterPresetErrors.size ? ` · ${this.rosterPresetErrors.size} 条预设应用结果待确认` : ` · ${this.rosterPresetChoices.size} 条预设正在保存`;
    }
    this.statusRevisions.textContent = `保存版本 ${fmt(revision)} · 客户端接收 ${fmt(applied)} · 行为采用 ${fmt(effective)}`;
    const migration = config.human_bots_migration; const limitations = migration?.limitations;
    this.migrationNote.hidden = migration?.source_schema !== 2 || !Array.isArray(limitations) || !limitations.length;
    this.migrationNote.textContent = this.migrationNote.hidden ? '' : `旧版配置已一次展开为独立细项。${limitations.join(' ')}`;
    const changed = this.lastRevision !== revision; this.lastRevision = revision;
    for (const state of this.rows.values()) {
      const focused = state.row.contains(document.activeElement); const draft = this.drafts.get(this.draftId(state.param.key));
      if (!focused && !draft) {const current = this.displayValue(state.param); state.mode.value = current.mode; state.min.value = current.min; state.max.value = current.max;}
      this.updateRow(state); this.updatePreviewLabel(state.preview, state.param);
    }
    this.updateScope(); this.updateDraftStatus(); if (changed || this.previewKey !== `${config.revision}|${this.entry()?.id || this.bot}`) this.requestPreview();
    this.refreshSpectator();
  };
  HumanBotsPanel.prototype.setSpec = function (spec, explanations, capabilities) {
    if (spec && spec.schema_digest_sha256 !== this.spec.schema_digest_sha256) {this.knownCapabilities = null; this.capabilityCache = null; this.lastCapabilities = null;}
    this.spec = spec || this.spec; this.explanations = explanations || this.explanations; this.capabilities = capabilities || this.capabilities; this.indexSpec(); this.fillGroups(); this.fillPresets(); this.renderList(); this.refresh();
  };
  HumanBotsPanel.prototype.destroy = function () {this.destroyed = true; this.previewSeq++; this.container.replaceChildren();};
  root.HumanBotsPanel = HumanBotsPanel;
})(window);
