"""Product adapters for schema3 preferences and the existing owned-pair channel.

Configuration previews are resolved input data, never measured player ability.
Only the native consumer acknowledgment can mark a configuration effective.
"""
from __future__ import annotations

import copy
import hashlib
import json
from pathlib import Path
import struct

import human_bot_delivery as delivery
import human_bot_pack as wire
import humanlike_settings as human
import human_bot_roster as roster
import training_settings_v2 as legacy


def _validate_legacy(value):
    return legacy.validate(copy.deepcopy(value))


class _PureValidator(human.CandidateSettings):
    def __init__(self):
        # validate_full needs only the injected existing validator/defaults.
        # This object performs no settings-file read or mutation.
        self._legacy_defaults = copy.deepcopy(legacy.DEFAULTS)
        self._legacy_validate = _validate_legacy


class HumanSettings(human.CandidateSettings):
    def __init__(self, path):
        self._reading_legacy = True
        self._validating_full = False
        try:
            super().__init__(path, legacy_defaults=legacy.DEFAULTS,
                             legacy_validate=self._validate_legacy_store)
        finally:
            self._reading_legacy = False

    def _validate_legacy_store(self, value):
        value = copy.deepcopy(value)
        population = value.get('total_players')
        if self._reading_legacy and not self._validating_full and type(population) is int and legacy.MAX_UNIQUE_PLAYERS < population <= 32:
            value['total_players'] = legacy.MAX_UNIQUE_PLAYERS
        return _validate_legacy(value)

    def _validate(self, raw):
        previous = self._validating_full
        self._validating_full = True
        try:
            return super()._validate(raw)
        finally:
            self._validating_full = previous

    def persist_roster(self):
        """First explicit save/start persists a deterministic legacy list once."""
        with self._lock:
            try:
                raw = self.path.read_bytes()
                stored = json.loads(raw.decode('utf-8'))
            except FileNotFoundError:
                raw = None
                stored = {}
            digest = hashlib.sha256(raw).digest() if raw is not None else None
            if digest != self._disk_digest:
                raise legacy.RevisionConflict('设置文件已在当前实例外修改，请重新读取')
            if 'bot_roster' not in stored.get('human_bots', {}):
                return self.update({}, self.value['revision'])
            return self.read()

    def _propose(self, patch, expected_revision):
        if type(patch) is not dict:
            raise ValueError('修改必须为对象')
        patch = copy.deepcopy(patch)
        old_bots_present = 'bots' in patch
        old_bots = patch.pop('bots', None)
        result = super()._propose(patch, expected_revision)
        if old_bots_present:
            if result['human_bots']['enabled'] and result['mode'] == 'deathmatch':
                raise ValueError('拟人人机已启用，请使用独立细项；旧版水平控件已停用')
            if type(old_bots) is not dict:
                raise ValueError('旧版人机修改必须为对象')
            result['bots'].update(old_bots)
            result = self.validate_full(result)
        return result

    def propose(self, patch, expected_revision):
        try:
            return super().propose(patch, expected_revision)
        except human.RevisionConflict as error:
            raise legacy.RevisionConflict(str(error)) from error

    def update(self, patch, expected_revision):
        try:
            return super().update(patch, expected_revision)
        except human.RevisionConflict as error:
            raise legacy.RevisionConflict(str(error)) from error


def validate_config(config):
    if type(config) is not dict:
        raise ValueError('设置必须为对象')
    if config.get('schema_version') == 2:
        return _validate_legacy(config)
    return _PureValidator().validate_full(config)


def configuration_bundle(config):
    config = validate_config(config)
    if config['schema_version'] != 3:
        return None
    identities = tuple(range(config['total_players']))
    canonical = {'schema': human.SCHEMA_DIGEST.hex(), 'revision': config['revision'],
                 'seed': config['seed'], 'identities': identities,
                 'human_bots': {k:v for k,v in human.validate(config['human_bots']).items()
                                if k not in {'individual_presets','custom_presets'} or v}}
    encoded = json.dumps(canonical, ensure_ascii=False, sort_keys=True,
                         separators=(',', ':'), allow_nan=False).encode('utf-8')
    transaction = int.from_bytes(hashlib.sha256(encoded).digest()[:8], 'little') or 1
    return wire.make_bundle(canonical['human_bots'], seed=config['seed'], revision=config['revision'],
                            identities=identities, transaction_id=transaction)


def write_initial(directory, config, map_info, *, original_write_initial, root):
    config = validate_config(config)
    bundle = configuration_bundle(config)
    result = delivery.write_initial(directory, config, map_info,
                                    original_write_initial=original_write_initial, root=root,
                                    transaction_id=bundle.transaction_id if bundle else None)
    if config.get('schema_version') == 3 and 'bot_roster' in config['human_bots']:
        result['bot_roster'] = roster.write_sidecar(directory, config, root=root)
    return result


def apply_to_channel(channel, config):
    bundle = configuration_bundle(config)
    if bundle is None:
        return {'status': 'legacy', 'consumer_effective': False}
    if not roster.live_match(config, channel.state()):
        return roster.pending_restart()
    result = delivery.ensure_bundle(channel, bundle)
    if result['status'] not in ('consumer_effective', 'consumer_pending', 'command_pending',
                                 'transport_unconfirmed'):
        raise RuntimeError('拟人人机设置未应用：' + str(result.get('message') or result['status']))
    return result


def runtime_summary(runtime):
    if type(runtime) is not dict:
        return {}
    game = runtime.get('game', runtime)
    if type(game) is not dict:
        return {}
    result = game.get('human_bots')
    return result if type(result) is dict else {}


def capabilities(runtime=None):
    actual = runtime_summary(runtime or {}).get('capabilities')
    if type(actual) is dict:
        return copy.deepcopy(actual)
    return {'by_key': {}, 'status': 'pending',
            'reason_zh': '等待当前训练器确认参数消费者；配置目录不等于动作已经执行。'}


def catalog(runtime=None):
    path = Path(__file__).with_name('parameter-explanations.json')
    explanations = json.loads(path.read_text(encoding='utf-8'))
    if explanations.get('schema_digest') != human.SCHEMA_DIGEST.hex() or explanations.get('param_count') != human.PARAM_COUNT:
        raise RuntimeError('人机参数说明与当前配置版本不匹配')
    return {'spec': human.SPEC, 'explanations': explanations, 'capabilities': capabilities(runtime),
            'preset_catalog': human.RANK_CATALOG['presets'], 'hero_catalog': roster.catalog()}


def preview(config, index=None, *, entry_id=None, runtime=None, draft=None, keys=None):
    config = validate_config(config)
    if config.get('schema_version') != 3:
        raise ValueError('当前后端没有拟人人机配置')
    if draft is not None:
        if type(draft) is not dict or set(draft) - {'human_bots', 'seed', 'total_players'}:
            raise ValueError('无效配置预览字段')
        config = copy.deepcopy(config)
        config.update(copy.deepcopy(draft))
        config = validate_config(config)
    if entry_id is not None:
        if index is not None: raise ValueError('entry_id与roster_index只能选择一个')
        index, _ = roster._entry(config['human_bots'], entry_id)
    if type(index) is not int or not 1 <= index < config['total_players']:
        raise ValueError('请选择当前人数范围内的人机，不能选择自己')
    if keys is not None and (type(keys) is not list or len(keys) > 256 or
                             any(type(key) is not str or key not in human.BY_KEY for key in keys)):
        raise ValueError('预览参数必须是目录中的至多256项')
    model = config['human_bots']
    values = human.resolve(model, config['seed'], index)
    fingerprint = wire.fnv(struct.pack('<2I', index, index) + struct.pack('<' + str(human.PARAM_COUNT) + 'f', *values))
    state = runtime_summary(runtime or {})
    fingerprints = state.get('profile_fingerprints')
    effective = (state.get('schema_digest') == human.SCHEMA_DIGEST.hex()
                 and state.get('transport_applied_revision') == config['revision']
                 and state.get('consumer_effective_known') is True
                 and state.get('consumer_effective_revision') == config['revision']
                 and state.get('configuration_effective') is True
                 and state.get('enabled') is model['enabled']
                 and type(fingerprints) is list and len(fingerprints) > index
                 and fingerprints[index] == f'{fingerprint:016x}')
    sources = {}
    preset = human.selected_preset(model, index)
    entry = human._entry(model, index)
    for key in keys or human.BY_KEY:
        effective_preset = preset if human.BY_KEY[key]['scope']=='bot' else model['preset']
        if entry is not None and key in entry['overrides']:
            source = '名单个人覆盖'
        elif entry is None and key in model['individual_overrides'].get(str(index), {}):
            source = '个体覆盖'
        elif key in model['distribution']:
            source = '全局个体分布'
        elif key in model['global_overrides']:
            source = '全局覆盖'
        elif key in model['preset_overrides'].get(effective_preset, {}):
            source = '预设修改'
        elif any(key in layer for layer in human._preset_layers(model, preset if human.BY_KEY[key]['scope']=='bot' else model['preset'])):
            source = '段位风格预设'
        else:
            source = '默认值'
        sources[key] = source
    return {'source': 'configuration_preview', 'revision': config['revision'], 'stable_bot_id': index,
            'wire_slot': index, 'entry_id': entry['id'] if entry else None,
            'hero_id': entry['hero_id'] if entry else None, 'selected_preset': preset,
            'preset_application': 'entry_overrides' if entry and entry['overrides'] else 'inherit',
            'profile_seed': config['seed'], 'schema_digest': human.SCHEMA_DIGEST.hex(),
            'values': ({key: values[human.BY_KEY[key]['id']] for key in keys} if keys is not None else values),
            'sources': sources, 'profile_fingerprint': f'{fingerprint:016x}',
            'consumer_effective': bool(effective), 'behavior_execution_verified': False}


def roster_action(config, body):
    config = validate_config(config)
    if config.get('schema_version') != 3:
        raise ValueError('当前后端没有拟人人机配置')
    try:
        return roster.action(config, body)
    except human.RevisionConflict as error:
        raise legacy.RevisionConflict(str(error)) from error
