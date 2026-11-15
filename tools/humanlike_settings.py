"""Candidate-only schema3 settings; importing/reading never writes a settings file.

Numeric arrays match humanlike_bot_config.h. Human slot zero is never driven.
All behavior remains pending until native consumers explicitly acknowledge it.
"""
from __future__ import annotations

import copy
import hashlib
import json
import math
import os
import re
from pathlib import Path
import struct
import threading

DIRECTORY = Path(__file__).resolve().parent
SPEC = json.loads((DIRECTORY / 'humanlike_bot_spec.json').read_text(encoding='utf-8'))
SCHEMA_DIGEST = bytes.fromhex(SPEC['schema_digest_sha256'])
_canonical = {k: v for k, v in SPEC.items() if k != 'schema_digest_sha256'}
if hashlib.sha256(json.dumps(_canonical, ensure_ascii=False, sort_keys=True,
                            separators=(',', ':')).encode('utf-8')).digest() != SCHEMA_DIGEST:
    raise RuntimeError('Human-bot schema digest mismatch; do not mix generated contracts')
PARAMS = tuple(SPEC['parameters'])
PARAM_COUNT = len(PARAMS)
BY_KEY = {p['key']: p for p in PARAMS}
BY_ENUM = {p['enum']: p for p in PARAMS}
PRESETS = {p['id']: p for p in SPEC['presets']}
RANK_CATALOG = json.loads((DIRECTORY / 'humanlike_rank_presets.json').read_text(encoding='utf-8'))
if RANK_CATALOG.get('version') != 1 or RANK_CATALOG.get('schema_digest') != SCHEMA_DIGEST.hex():
    raise RuntimeError('Rank catalog does not match the frozen parameter schema')
ALL_PRESETS = dict(PRESETS)
for _rank in RANK_CATALOG['presets']:
    if _rank['id'] in PRESETS:
        if _rank['overrides'] != PRESETS[_rank['id']]['overrides']:
            raise RuntimeError('Legacy rank anchor changed')
    else:
        ALL_PRESETS[_rank['id']] = _rank

if len(BY_KEY) != PARAM_COUNT or any(p['id'] != i for i, p in enumerate(PARAMS)):
    raise RuntimeError('Non-unique keys or unstable parameter IDs')


class ConfigError(ValueError):
    pass


class RevisionConflict(ConfigError):
    pass


def f32(value):
    """The transport and native metadata use the same IEEE754 binary32 values."""
    return struct.unpack('<f', struct.pack('<f', value))[0]


def key(enum):
    return BY_ENUM[SPEC['cpp_aliases'].get(enum, enum)]['key']


def _object(value, allowed, name, *, complete=False):
    if type(value) is not dict or set(value) - set(allowed):
        raise ConfigError(f'{name} 包含未知字段或不是对象')
    if complete and set(value) != set(allowed):
        raise ConfigError(f'{name} 缺少字段')


def _uint(value, name, maximum=0xffffffff, minimum=0):
    if type(value) is not int or not minimum <= value <= maximum:
        raise ConfigError(f'{name} 必须为 {minimum}–{maximum} 的整数')
    return value


def _number(param, value, *, editable=True):
    if editable and not param['editable']:
        raise ConfigError(f"{param['key']} 是语义常量，不可编辑")
    try:
        finite = type(value) in (int, float) and math.isfinite(value)
    except OverflowError:
        finite = False
    if not finite:
        raise ConfigError(f"{param['key']} 必须是有限数字；开关使用数字0/1")
    try:
        normalized = f32(value)
    except (OverflowError, struct.error) as exc:
        raise ConfigError(f"{param['key']} 超出float32范围") from exc
    if not f32(param['min']) <= normalized <= f32(param['max']):
        raise ConfigError(f"{param['key']} 超出 {param['min']}–{param['max']}")
    if param['type'] != 'float' and value != math.trunc(value):
        raise ConfigError(f"{param['key']} 必须是整数")
    return normalized if param['type'] == 'float' else int(normalized)


def _values(values, *, scope='any'):
    if type(values) is not dict:
        raise ConfigError('参数覆盖必须为对象')
    result = {}
    for name, value in values.items():
        if name not in BY_KEY:
            raise ConfigError(f'未知参数 {name}')
        p = BY_KEY[name]
        if scope == 'bot' and p['scope'] != 'bot':
            raise ConfigError(f'{name} 是全局共享参数，不能逐人或随机分布覆盖')
        result[name] = _number(p, value)
    return result


def _interval(param, value):
    _object(value, {'min', 'max'}, param['key'], complete=True)
    low, high = _number(param, value['min']), _number(param, value['max'])
    if low > high:
        raise ConfigError(f"{param['key']} 分布最小值大于最大值")
    return {'min': low, 'max': high}


def _individual_values(values):
    _object(values, BY_KEY, '逐人能力')
    result = {}
    for name, value in values.items():
        p = BY_KEY[name]
        if p['scope'] != 'bot' or not p['editable']:
            raise ConfigError(f'{name} 不允许逐人覆盖')
        result[name] = _interval(p, value) if type(value) is dict else _number(p, value)
    return result


def _cross(intervals):
    """Conservative independent-range proof; no correlated/clamped repairs."""
    bounds = lambda enum: intervals[BY_ENUM[enum]['id']]
    for a, b, scale in SPEC['cross_constraints']['less_equal_scaled']:
        if bounds(a)[1] > f32(bounds(b)[0] * f32(scale)):
            raise ConfigError(f'{a} 的上界必须 <= {b} 下界×{scale}')
    for a, b in SPEC['cross_constraints']['strict_less']:
        if bounds(a)[1] >= bounds(b)[0]:
            raise ConfigError(f'{a} 的上界必须 < {b} 下界')
    for a, b, maximum in SPEC['cross_constraints']['sum_max']:
        if f32(bounds(a)[1] + bounds(b)[1]) > f32(maximum):
            raise ConfigError(f'{a}+{b} 的最坏组合超过 {maximum}')
    for a, b, c in SPEC['cross_constraints']['sum_le_param']:
        if f32(bounds(a)[1] + bounds(b)[1]) > bounds(c)[0]:
            raise ConfigError(f'{a}+{b} 的上界必须 <= {c} 下界')


def validate_profile(values):
    if not isinstance(values, (tuple, list)) or len(values) != PARAM_COUNT:
        raise ConfigError('Profile参数数量不匹配')
    result = tuple(f32(_number(p, v, editable=False)) for p, v in zip(PARAMS, values))
    _cross([(v, v) for v in result])
    return result


def defaults():
    return {'version': 1, 'enabled': False, 'preset': 'default', 'preset_overrides': {},
            'global_overrides': {}, 'distribution': {}, 'individual_overrides': {}}


def preset_ids(config):
    return {'default', *ALL_PRESETS, *config.get('custom_presets', {})}


def _preset_layers(config, selected):
    custom = config.get('custom_presets', {}).get(selected)
    if custom is not None:
        base = custom['base_preset']
        layers = [] if base == 'default' else [ALL_PRESETS[base]['overrides']]
        layers += [custom['overrides']]
    else:
        layers = [] if selected == 'default' else [ALL_PRESETS[selected]['overrides']]
    return layers + [config['preset_overrides'].get(selected, {})]


def _entry(config, stable_bot_id):
    roster = config.get('bot_roster')
    return roster[stable_bot_id - 1] if roster is not None and 1 <= stable_bot_id <= len(roster) else None


def selected_preset(config, stable_bot_id):
    entry = _entry(config, stable_bot_id)
    return entry['preset_id'] if entry is not None else config.get('individual_presets', {}).get(str(stable_bot_id), config['preset']) if stable_bot_id else config['preset']


def _base(config, stable_bot_id=0):
    values = [f32(p['default']) for p in PARAMS]
    # Shared fields always derive from the global preset, never a bot's hero/entry.
    for layer in _preset_layers(config, config['preset']):
        for name, value in layer.items():
            if BY_KEY[name]['scope'] == 'global_only':
                values[BY_KEY[name]['id']] = f32(value)
    for layer in _preset_layers(config, selected_preset(config, stable_bot_id)) + [config['global_overrides']]:
        for name, value in layer.items():
            if BY_KEY[name]['scope'] == 'bot' or layer is config['global_overrides']:
                values[BY_KEY[name]['id']] = f32(value)
    return values


def _identity(identity):
    if type(identity) is not str or not identity.isascii() or not identity.isdecimal() or len(identity) > 10 or identity != str(int(identity)):
        raise ConfigError('逐人覆盖键必须为规范十进制stableBotId字符串')
    _uint(int(identity), 'stableBotId', minimum=1)
    return identity


def _custom_id(identity):
    if type(identity) is not str or not re.fullmatch(r'custom:[a-z0-9][a-z0-9_-]{0,47}', identity):
        raise ConfigError('自定义预设ID必须为custom:加1–48位小写字母数字下划线或短横线')
    return identity


def _intervals(values, distribution, overrides):
    result = [(v, v) for v in values]
    for layer in (distribution, overrides):
        for name, value in layer.items():
            result[BY_KEY[name]['id']] = (value['min'], value['max']) if type(value) is dict else (value, value)
    return result


def validate(config):
    optional = {'individual_presets', 'custom_presets', 'bot_roster'}
    _object(config, set(defaults()) | optional, 'human_bots')
    if not set(defaults()) <= set(config):
        raise ConfigError('human_bots 缺少字段')
    if type(config['version']) is not int or config['version'] != 1:
        raise ConfigError('未知human_bots版本')
    if type(config['enabled']) is not bool:
        raise ConfigError('enabled必须为布尔值')
    result = copy.deepcopy(config)
    custom = config.get('custom_presets', {})
    if type(custom) is not dict or len(custom) > 64:
        raise ConfigError('自定义预设必须为对象且最多64项')
    normalized = {}
    for identity, row in custom.items():
        _custom_id(identity);_object(row, {'name', 'base_preset', 'overrides'}, '自定义预设', complete=True)
        name = row['name']
        if type(name) is not str or not 1 <= len(name.strip()) <= 80 or any(ord(c) < 32 for c in name):
            raise ConfigError('预设名称必须为1–80字且无控制字符')
        if type(row['base_preset']) is not str or row['base_preset'] not in {'default', *ALL_PRESETS}:
            raise ConfigError('自定义预设base只能引用内置预设，不允许继承链')
        normalized[identity] = {'name': name.strip(), 'base_preset': row['base_preset'], 'overrides': _values(row['overrides'], scope='bot')}
    if 'custom_presets' in result: result['custom_presets'] = normalized
    if type(config['preset']) is not str or config['preset'] not in preset_ids(result):
        raise ConfigError('未知预设')
    result['global_overrides'] = _values(config['global_overrides'])
    _object(config['preset_overrides'], preset_ids(result), '预设覆盖')
    result['preset_overrides'] = {p: _values(v, scope='bot' if p in normalized else 'any') for p, v in config['preset_overrides'].items()}
    # Presets remain independently valid even while unused; saved global values
    # are not allowed to conceal a malformed library entry.
    for preset in preset_ids(result):
        probe = dict(result, preset=preset, global_overrides={})
        probe.pop('bot_roster', None);probe.pop('individual_presets', None)
        standalone = _base(probe)
        if preset in normalized:
            shared = _base(result)
            for p in PARAMS:
                if p['scope'] == 'global_only': standalone[p['id']] = shared[p['id']]
        validate_profile(standalone)
    _object(config['distribution'], BY_KEY, '能力分布')
    result['distribution'] = {}
    for name, interval in config['distribution'].items():
        p = BY_KEY[name]
        if p['scope'] != 'bot' or not p['editable']:
            raise ConfigError(f'{name} 不能设置独立能力分布')
        result['distribution'][name] = _interval(p, interval)
    for map_name in ('individual_overrides', 'individual_presets'):
        source = config.get(map_name, {})
        if type(source) is not dict or len(source) > 31:
            raise ConfigError('逐人设置必须为对象且最多31个NPC')
        rows = {}
        for identity, value in source.items():
            _identity(identity)
            if map_name == 'individual_overrides': rows[identity] = _individual_values(value)
            else:
                if type(value) is not str or value not in preset_ids(result): raise ConfigError('未知个人预设')
                rows[identity] = value
        if map_name in result: result[map_name] = rows
    if 'bot_roster' in config:
        from human_bot_roster import validate_roster
        result['bot_roster'] = validate_roster(config['bot_roster'], result)
    _cross(_intervals(_base(result), result['distribution'], {}))
    if 'bot_roster' in result:
        for slot, entry in enumerate(result['bot_roster'], 1):
            _cross(_intervals(_base(result, slot), result['distribution'], entry['overrides']))
    else:
        for identity in set(result['individual_overrides']) | set(result.get('individual_presets', {})):
            _cross(_intervals(_base(result, int(identity)), result['distribution'], result['individual_overrides'].get(identity, {})))
    return result


def stable_quantile(seed, stable_bot_id, param_id):
    _uint(seed, 'seed', minimum=1)
    _uint(stable_bot_id, 'stableBotId')
    _uint(param_id, 'ParamId', PARAM_COUNT - 1)
    digest = hashlib.sha256(b'VTHBOT-QUANTILE-1\0' + struct.pack('<3I', seed, stable_bot_id, param_id)).digest()
    # 52-bit bin midpoints are exactly representable and strictly inside (0,1).
    return ((int.from_bytes(digest[:8], 'little') >> 12) + 0.5) / (1 << 52)


def _sample(param, interval, seed, stable_bot_id):
    low, high = interval['min'], interval['max']
    q = stable_quantile(seed, stable_bot_id, param['id'])
    value = low + q * (high - low) if param['type'] == 'float' else low + math.floor(q * (high - low + 1))
    return f32(value)


def _sample_entry(param, interval, seed, token):
    digest = hashlib.sha256(b'VTHBOT-ENTRY-QUANTILE-1\0' + struct.pack('<IQI', seed, int(token, 16), param['id'])).digest()
    q = ((int.from_bytes(digest[:8], 'little') >> 12) + 0.5) / (1 << 52)
    low, high = interval['min'], interval['max']
    value = low + q * (high - low) if param['type'] == 'float' else low + math.floor(q * (high - low + 1))
    return f32(value)


def resolve(config, seed, stable_bot_id, *, validated=False):
    config = config if validated else validate(config)
    _uint(seed, 'seed', minimum=1)
    _uint(stable_bot_id, 'stableBotId')
    if 'bot_roster' in config and stable_bot_id > len(config['bot_roster']):
        raise ConfigError('人机编号不在固定名单中')
    values = _base(config, stable_bot_id)
    entry = _entry(config, stable_bot_id)
    if stable_bot_id != 0:  # Slot zero contains shared fields but is never sampled/driven.
        for name, interval in config['distribution'].items():
            p = BY_KEY[name]
            values[p['id']] = _sample_entry(p, interval, seed, entry['id']) if entry else _sample(p, interval, seed, stable_bot_id)
        overrides = entry['overrides'] if entry else config['individual_overrides'].get(str(stable_bot_id), {})
        for name, value in overrides.items():
            p = BY_KEY[name]
            values[p['id']] = (_sample_entry(p, value, seed, entry['id']) if entry else _sample(p, value, seed, stable_bot_id)) if type(value) is dict else f32(value)
    return validate_profile(values)


def resolve_roster(config, seed, identities):
    config = validate(config)
    if not isinstance(identities, (list, tuple)) or not 1 <= len(identities) <= 32:
        raise ConfigError('Roster必须包含human0，且总人数为1–32')
    if identities[0] != 0 or type(identities[0]) is not int:
        raise ConfigError('Roster第一项必须是human stableBotId0')
    for n in identities[1:]:
        _uint(n, 'NPC stableBotId', minimum=1)
    if len(set(identities)) != len(identities):
        raise ConfigError('重复stableBotId')
    if 'bot_roster' in config and list(identities) != list(range(len(config['bot_roster']) + 1)):
        raise ConfigError('固定名单wire身份必须是human0及按顺序的1..N')
    return [resolve(config, seed, n, validated=True) for n in identities]


def _map_patch(current, patch, label, *, nested=False):
    if type(patch) is not dict:
        raise ConfigError(f'{label} patch必须为对象')
    result = copy.deepcopy(current)
    for name, value in patch.items():
        if value is None:
            result.pop(name, None)
        elif nested:
            result[name] = _map_patch(result.get(name, {}), value, label)
        else:
            result[name] = copy.deepcopy(value)
    return result


def merge(config, patch):
    _object(patch, (set(defaults()) | {'individual_presets', 'custom_presets', 'bot_roster'}) - {'version'}, 'human_bots patch')
    result = copy.deepcopy(config)
    for name, value in patch.items():
        if name in {'preset_overrides', 'individual_overrides'}:
            if type(value) is not dict: raise ConfigError('预设或逐人patch必须为对象')
            for identity, overrides in value.items():
                if name == 'individual_overrides': _identity(identity)
                elif identity not in preset_ids(config) | preset_ids(dict(result, **{'custom_presets': {**result.get('custom_presets', {}), **{k:v for k,v in patch.get('custom_presets', {}).items() if v is not None}}})): raise ConfigError('未知预设patch')
                if overrides is not None:
                    _object(overrides, BY_KEY, '参数patch')
                    for key in overrides:
                        p = BY_KEY[key]
                        if not p['editable'] or name == 'individual_overrides' and p['scope'] != 'bot': raise ConfigError('不允许此参数覆盖')
            result[name] = _map_patch(result.get(name, {}), value, name, nested=True)
        elif name in {'global_overrides', 'distribution'}:
            _object(value, BY_KEY, name + ' patch')
            for key in value:
                p = BY_KEY[key]
                if not p['editable'] or name == 'distribution' and p['scope'] != 'bot': raise ConfigError('不允许此参数覆盖')
            result[name] = _map_patch(result[name], value, name)
        elif name in {'individual_presets', 'custom_presets'}:
            if type(value) is not dict: raise ConfigError('映射patch必须为对象')
            for identity in value:
                _identity(identity) if name == 'individual_presets' else _custom_id(identity)
            result[name] = _map_patch(result.get(name, {}), value, name)
        else:
            result[name] = copy.deepcopy(value)
    return validate(result)


LEGACY_RANGES = {'reaction_ms': (60, 2000), 'reaction_spread_ms': (0, 1000),
                 'aim_speed': (20, 1080), 'aim_spread': (0, 800), 'accuracy': (.02, 1),
                 'accuracy_spread': (.01, .7), 'movement': (.05, 1), 'movement_spread': (.01, .7)}


def migrate_legacy_bots(bots):
    """Expand old coupled capabilities ONCE; future edits are independent.

    The legacy time-based burst cannot be reproduced as an exact shot count.
    Its eight fields remain provenance/old-V2 compatibility, not live abilities.
    """
    _object(bots, LEGACY_RANGES, '旧bots', complete=True)
    for name, value in bots.items():
        if type(value) not in (int, float) or not math.isfinite(value) or not LEGACY_RANGES[name][0] <= value <= LEGACY_RANGES[name][1]:
            raise ConfigError(f'旧bots.{name}无效')
    config = defaults()
    config['enabled'] = False  # Explicit activation awaits real consumer readiness.
    ranges = config['distribution']
    fixed = config['global_overrides']
    def interval(enum, low, high):
        p = BY_KEY[key(enum)]
        low, high = max(p['min'], low), min(p['max'], high)
        ranges[p['key']] = {'min': low, 'max': high}
    def value(enum, number):
        fixed[key(enum)] = number
    reaction_low = max(40, bots['reaction_ms'] - bots['reaction_spread_ms'])
    reaction_high = min(5000, bots['reaction_ms'] + bots['reaction_spread_ms'])
    for context in ('Hold', 'ActivePeek', 'ThirdParty', 'Flank', 'Hearing', 'Hurt', 'Reappear', 'DirectionChange', 'CrouchJump'):
        interval('Reaction' + context + 'PerceiveMs', reaction_low, reaction_high)
    interval('PrimarySpeedDegS', max(1, bots['aim_speed'] - bots['aim_spread']), min(1440, bots['aim_speed'] + bots['aim_spread']))
    a_low, a_high = max(0, bots['accuracy'] - bots['accuracy_spread']), min(1, bots['accuracy'] + bots['accuracy_spread'])
    for enum in ('PrimaryYawErrorDeg', 'PrimaryPitchErrorDeg'):
        interval(enum, .12 + 5.5 * (1 - a_high) ** 2, .12 + 5.5 * (1 - a_low) ** 2)
    interval('AimHeadWeight', .1 + .55 * a_low, .1 + .55 * a_high)
    interval('FireRoundPauseMs', (.12 + .45 * (1 - a_high)) * 1000, (.12 + .45 * (1 - a_low)) * 1000)
    value('PrimaryDelayMs', 0)
    m_low, m_high = max(0, bots['movement'] - bots['movement_spread']), min(1, bots['movement'] + bots['movement_spread'])
    interval('StrafeWeight', m_low * 100, m_high * 100)
    return validate(config), {'source_schema': 2, 'source_bots': copy.deepcopy(bots),
        'limitations': ['仅一次展开旧耦合数值；后续单项编辑不反向修改其他能力。',
                       '旧三角形能力分布改为按独立稳定身份的均匀范围，不宣称逐局轨迹兼容。',
                       '旧按时间开火不能推导真实弹数；采用新独立真实弹数默认值。',
                       '新参数均待consumer验证；读取旧文件不启用、不写回。']}


class CandidateSettings:
    """Inject the EXISTING V2 validator; do not duplicate or weaken non-bot checks."""
    def __init__(self, path, *, legacy_defaults, legacy_validate):
        self.path = Path(path)
        self._lock = threading.RLock()
        self._legacy_defaults = copy.deepcopy(legacy_defaults)
        self._legacy_validate = legacy_validate
        try:
            source = self.path.read_bytes()
            raw = json.loads(source.decode('utf-8'))
        except FileNotFoundError:
            source = None
            raw = copy.deepcopy(legacy_defaults)
        self._disk_digest = hashlib.sha256(source).digest() if source is not None else None
        if type(raw) is not dict:
            raise ConfigError('设置必须为对象')
        if type(raw.get('schema_version')) is not int:
            raise ConfigError('设置版本必须为整数')
        if raw.get('schema_version') == 2:
            # Keep existing read-only hero compatibility migration.
            raw.setdefault('local_hero_id', legacy_defaults['local_hero_id'])
            legacy = legacy_validate(copy.deepcopy(raw))
            human, migration = migrate_legacy_bots(legacy['bots'])
            raw = dict(legacy, schema_version=3, human_bots=human, human_bots_migration=migration)
        self.value = self._validate(raw)

    def _validate(self, raw):
        _object(raw, {*self._legacy_defaults, 'human_bots', 'human_bots_migration'}, 'schema3设置', complete=True)
        if type(raw['schema_version']) is not int or raw['schema_version'] != 3:
            raise ConfigError('设置版本必须为3')
        legacy = {k: copy.deepcopy(v) for k, v in raw.items() if k in self._legacy_defaults}
        legacy['schema_version'] = 2
        legacy = self._legacy_validate(legacy)
        human = validate(raw['human_bots'])
        from human_bot_roster import materialize_legacy, validate_session_roster
        human = materialize_legacy(human, legacy['seed'], legacy['total_players'], legacy['local_hero_id'])
        validate_session_roster(human, legacy['total_players'], legacy['local_hero_id'])
        migration = raw['human_bots_migration']
        _object(migration, {'source_schema', 'source_bots', 'limitations'}, '迁移来源', complete=True)
        if migration['source_schema'] not in (2, 3) or type(migration['source_schema']) is not int:
            raise ConfigError('无效迁移来源')
        if type(migration['limitations']) is not list or any(type(v) is not str for v in migration['limitations']):
            raise ConfigError('无效迁移说明')
        if migration['source_schema'] == 2:
            migrate_legacy_bots(migration['source_bots'])
        elif migration['source_bots'] != {}:
            raise ConfigError('新设置不能含伪造旧来源')
        return dict(legacy, schema_version=3, human_bots=human, human_bots_migration=copy.deepcopy(migration))

    def read(self):
        with self._lock:
            return copy.deepcopy(self.value)

    def validate_full(self, raw):
        """Pure complete-config validation for catalog/runtime preflight."""
        return self._validate(raw)

    def _propose(self, patch, expected_revision):
        if type(expected_revision) is not int or expected_revision != self.value['revision']:
            raise RevisionConflict('设置版本已变化，请刷新后重试')
        allowed = set(self._legacy_defaults) - {'schema_version', 'revision', 'bots'}
        _object(patch, allowed | {'human_bots'}, 'schema3 patch')
        result = copy.deepcopy(self.value)
        for name, value in patch.items():
            if name == 'human_bots':
                result[name] = merge(result[name], value)
            elif name == 'practice':
                _object(value, self._legacy_defaults['practice'], 'practice patch')
                result[name].update(copy.deepcopy(value))
            else:
                result[name] = copy.deepcopy(value)
        result['revision'] += 1
        return self._validate(result)

    def propose(self, patch, expected_revision):
        """Pure snapshot proposal: no file reads/writes or store mutation."""
        with self._lock:
            return self._propose(patch, expected_revision)

    def update(self, patch, expected_revision):
        with self._lock:
            result = self._propose(patch, expected_revision)
            try:
                current_digest = hashlib.sha256(self.path.read_bytes()).digest()
            except FileNotFoundError:
                current_digest = None
            if current_digest != self._disk_digest:
                raise RevisionConflict('设置文件已在当前实例外修改，请重新读取')
            # No disk mutation occurs until all validation and CAS have succeeded.
            self.path.parent.mkdir(parents=True, exist_ok=True)
            temporary = self.path.with_name(self.path.name + '.human-bots.tmp')
            payload = json.dumps(result, ensure_ascii=False, indent=2, allow_nan=False).encode('utf-8')
            with temporary.open('wb') as stream:
                stream.write(payload)
                stream.flush()
                os.fsync(stream.fileno())
            os.replace(temporary, self.path)
            self._disk_digest = hashlib.sha256(payload).digest()
            self.value = result
            return copy.deepcopy(result)
