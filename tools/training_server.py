"""Local Chinese trainer control panel. Standard-library server, no cloud service."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import math
import mimetypes
import os
from pathlib import Path
import signal
import struct
import subprocess
import sys
import threading
import time
from urllib.parse import unquote, urlsplit
import webbrowser

from local_environment import project_environment
from native_maps import training_maps
from training_control import Channel, guid_words, pack_configuration, pack_routes, pack_practice_routes
from training_settings_v2 import Settings, RevisionConflict, uv_to_world, MAX_UNIQUE_PLAYERS
from training_heroes import DEFAULT_HERO_ID, hero_profile, local_hero_capability
from training_controller_store import controller_directory, identity_matches_settings, open_settings, settings_path

ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / 'webui'
STYLE_LABELS = {'jump': '跳拉', 'wide': '大拉', 'short': '小拉', 'crouch': '蹲拉', 'peek': 'Peek', 'random': '随机'}
ROUTE_STYLE_LABELS = {**STYLE_LABELS, 'global': '跟随全局', 'wide_crouch': '大拉后蹲下', 'short_crouch': '小拉后蹲下'}
ROUTE_STYLE_LABELS['peek'] = '静步拉出'
CATEGORY_LABELS = {1: '重机枪', 2: '步枪', 3: '霰弹枪', 4: '副武器', 5: '冲锋枪', 6: '狙击枪'}

NATIVE_TEXT = {
    'waiting_for_game_thread': '等待客户端应用',
    'configuration_applied': '训练设置已应用',
    'original_session_not_ready': '训练场景尚未准备好，请稍后重试',
    'map_point_has_no_original_ground_or_capsule_clearance': '此点没有可用地面或站立空间，请在附近重新选点',
    'map_mode_population_change_requires_original_session_restart': '切换地图、模式或人数需要重新开始训练',
    'routes_saved_navigation_checked_when_selected': '路线已提交，使用时检查可走性',
    'route_validation_requested': '已提交路线检查',
    'waiting_for_original_path_validation': '等待客户端检查可走性',
    'route_requires_original_nav_or_capsule_ground_validation': '此路线尚未通过导航或地面碰撞检查',
    'following_original_navigation_path': '沿原导航路线移动',
    'following_externally_validated_local_segment': '沿已检查的路线移动',
    'native_collision_or_path_stall_no_teleport': '路线被障碍物阻挡，请调整路线点',
    'no_saved_routes_for_this_map': '当前地图还没有保存路线',
    'no_enabled_positive_weight_routes': '没有已启用且权重大于零的路线',
    'bound_pending_observation': '等待角色状态',
    'not_bound': '尚未就绪',
    'awaiting_original_alive_ready_epoch': '等待角色就绪或重生',
    'deathmatch_requires_verified_map_anchors': '此地图缺少已验证的出生点',
    'original_crouch_input_not_consumed': '角色尚未执行蹲下输入',
    'original_weapon_switch_queued': '已提交切换武器，等待装备完成',
    'original_skin_loadout_validated_switch_queued': '外观选择已验证，等待换装完成',
    'original_skin_selection_saved': '武器外观选择已保存',
    'original_effect_override_submitted': '效果设置已提交，等待客户端应用',
    'actual_skin_not_ready': '当前武器外观尚未加载完成，稍后重试效果设置',
    'original_owned_champion_effect_not_ready': '当前冠军效果尚未就绪，等待原效果播放后重试',
    'effect_not_supported_by_current_original_skin': '当前外观不支持此效果，已保留设置',
    'effect_key_not_in_current_original_skin': '当前外观没有此效果，已保留设置',
    'original_champion_level4_required': '需要冠军皮肤的最高等级',
    'waiting_for_original_owned_champion_effect': '等待当前武器的冠军效果就绪',
    'native_override_map_and_original_getter_verified': '原效果已就绪',
    'native_override_request_and_getter_verified': '效果设置已应用并核对',
    'live_original_override_rule_preserved': '遵循原效果规则',
    'original_cosmetic_MyKills_and_original_clamp0_25_graph_completed': '击杀递进数值已应用（0–25）',
    'original_champion_event_and_client_leaf_completed': '冠军效果已应用',
    'training_session_waiting_original_world': '正在加载地图',
    'training_session_equipping_original_three_slot_loadout': '正在装备武器',
    'training_session_waiting_original_equip': '等待武器装备完成',
    'training_session_running_original_gameplay': '训练进行中',
    'The previous game command is still pending': '上一条设置仍在等待客户端处理，请稍后重试',
    'Game state update is busy': '客户端正在更新状态，请稍后重试',
    'Game control is not ready': '客户端控制通道尚未就绪',
}


def runtime_text(value):
    """Keep machine codes intact in the API; add a readable Chinese label."""
    if value is None or value == '':
        return ''
    text = str(value)
    if text in NATIVE_TEXT:
        return NATIVE_TEXT[text]
    if not text.isascii():
        return text
    if '_' not in text:
        return f'客户端返回：{text}'
    area = next((label for prefix, label in (
        ('training_bots_', '机器人'), ('training_spawn_', '角色出生'),
        ('training_session_', '训练场景'), ('original_buy_', '购买界面'),
        ('all_weapons_', '武器'), ('all_skins_', '皮肤'),
        ('skin_effect_', '皮肤效果'), ('original_', '原客户端'),
        ('route', '路线'), ('invalid_', '输入参数')) if text.startswith(prefix)), '客户端')
    if any(word in text for word in ('failed', 'exception', 'invalid', 'rejected', 'mismatch')):
        detail = '处理失败'
    elif any(word in text for word in ('unavailable', 'missing', 'required', 'requires', 'not_ready')):
        detail = '所需条件尚未就绪'
    elif any(word in text for word in ('waiting', 'pending', 'queued')):
        detail = '正在等待处理'
    elif any(word in text for word in ('completed', 'verified', 'applied', 'ready', 'bound', 'saved')):
        detail = '已完成当前步骤'
    else:
        detail = '当前阶段'
    return f'{area}{detail}'


def effect_title(row):
    key = row.get('key', '')
    if key == 'champion_glow':
        return '冠军领先描边'
    if key == 'skin_kill_count':
        return '皮肤击杀递进'
    raw = str(row.get('label', '')).lower()
    names = (('inspectloop', '检视循环特效'), ('inspect', '检视特效'),
             ('reload', '换弹特效'), ('smoke', '持枪烟雾'), ('whileequipped', '持枪特效'),
             ('fastequip', '快速切枪特效'), ('equip', '装备特效'),
             ('tracer', '弹道曳光'), ('finisher', '终结特效'), ('kill', '击杀特效'),
             ('fire', '射击特效'), ('impact', '命中特效'), ('smoke', '持枪烟雾'),
             ('music', '检视音乐'), ('voice', '语音反馈'), ('idle', '持枪特效'))
    title = next((title for token, title in names if token in raw), '附加特效')
    if '_1p' in raw:
        title += '（第一人称）'
    elif '_3p' in raw:
        title += '（第三人称）'
    return title


def describe_game(game):
    game['stage_text'] = runtime_text(game.get('stage'))
    game['error_text'] = runtime_text(game.get('error'))
    for row in game.get('players', []):
        row['phase_text'] = runtime_text(row.get('phase'))
    for row in game.get('routes', []):
        row['message_text'] = runtime_text(row.get('message'))
    for row in game.get('effects', []):
        row['label_zh'] = effect_title(row)
        if not row.get('supported'):
            row['description_zh'] = '当前外观不支持此项。'
        elif not row.get('available'):
            row['description_zh'] = '等待当前外观准备完成。'
        elif not row.get('verified'):
            row['description_zh'] = '已提交选项，等待客户端应用。'
        elif row.get('key') == 'champion_glow':
            row['description_zh'] = '遵循领先判定，或手动选择是否显示描边。'
        elif row.get('key') == 'skin_kill_count':
            row['description_zh'] = '调整皮肤的击杀递进效果，不改变实际战绩。'
        else:
            row['description_zh'] = '对应动作触发时生效；关闭后使用基础效果。'
    if isinstance(game.get('weapon'), dict):
        game['weapon']['stage_text'] = runtime_text(game['weapon'].get('stage'))
    return game


def atomic_json(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding='utf-8')
    os.replace(temporary, path)


def read_json(path, default=None):
    try:
        return json.loads(Path(path).read_text(encoding='utf-8'))
    except (FileNotFoundError, json.JSONDecodeError):
        return default


def guid_id(value):
    if value is None:
        return None
    if isinstance(value, dict):
        value = value.get('guid', value.get('words'))
    try:
        return ''.join(f'{w:08x}' for w in guid_words(value))
    except ValueError:
        return None


class Catalog:
    def __init__(self, root=ROOT):
        self.root = Path(root)
        self._cache = {}
        self.lock = threading.RLock()

    def _report(self, name):
        path = self.root / 'reports' / name
        try:
            stamp = path.stat().st_mtime_ns
        except FileNotFoundError:
            return {}
        with self.lock:
            old = self._cache.get(name)
            if old and old[0] == stamp:
                return old[1]
            value = read_json(path, {})
            self._cache[name] = (stamp, value)
            return value

    def heroes(self):
        try:
            profile = hero_profile(self.root)
        except FileNotFoundError:
            return []
        return [{'id': row['id'], 'index': row['index'], 'code': row['code'], 'name': row['name'],
                 'skills_runtime_status': 'not_verified'} for row in profile['heroes']]

    def weapons(self):
        report = self._report('all-weapons.json')
        result = []
        for row in report.get('weapons', []):
            key = guid_id(row.get('words', row.get('guid')))
            if not key:
                continue
            result.append({'id': key, 'guid': row.get('guid'), 'words': list(guid_words(key)),
                           'name': row.get('display_name_zh') or row.get('display_name') or row.get('name') or key,
                           'category': CATEGORY_LABELS.get(row.get('category_value'), row.get('category') or '近战'),
                           'category_value': row.get('category_value', 7), 'icon': row.get('icon'),
                           'weapon_class_path': row.get('weapon_class_path'),
                           'data_asset_class_path': row.get('data_asset_class_path'),
                           'runtime_status': row.get('runtime_status', 'not_tested'),
                           'runtime_supported': row.get('runtime_supported', False)})
        return result

    def skins(self, weapon=None):
        report = self._report('all-skins.json')
        rows = report.get('skins', [])
        if isinstance(rows, dict):
            rows = list(rows.values())
        weapons = self.weapons()
        by_class = {w['weapon_class_path']: w['id'] for w in weapons}
        by_asset = {w['data_asset_class_path']: w['id'] for w in weapons}
        result = []
        for row in rows:
            key = guid_id(row.get('words', row.get('guid', row.get('id'))))
            weapon_key = guid_id(row.get('weapon_guid', row.get('equippable_guid')))
            if not weapon_key:
                weapon_key = by_class.get(row.get('equippable', row.get('equippable_class_path')))
            if not weapon_key:
                weapon_key = by_asset.get(row.get('equippable_data_asset_class_path'))
            if not key or weapon is not None and weapon_key != weapon:
                continue
            def options(values, default_label):
                items = []
                for i, v in enumerate(values or []):
                    if not isinstance(v, dict):
                        v = {'guid': v}
                    option = guid_id(v.get('words', v.get('guid', v.get('id'))))
                    if option:
                        items.append({'id': option, 'name': v.get('display_name_zh') or v.get('display_name') or v.get('name') or f'{default_label} {i + 1}',
                                      'level': v.get('level', i + 1), 'icon': v.get('icon')})
                return items
            result.append({'id': key, 'weapon_id': weapon_key,
                           'name': row.get('display_name_zh') or row.get('display_name') or row.get('label') or row.get('name') or row.get('short_name') or key,
                           'icon': row.get('icon'), 'levels': options(row.get('levels'), '等级'),
                           'chromas': options(row.get('chromas'), '炫彩'),
                           'default_chroma': guid_id(row.get('default_chroma')) or next((guid_id(c.get('words', c.get('guid'))) for c in row.get('chromas', []) if isinstance(c, dict) and c.get('is_default')), None),
                           'runtime_status': row.get('runtime_status', 'not_tested'),
                           'runtime_supported': row.get('runtime_supported', False)})
        return result

    def effects(self):
        report = self._report('skin-effect-controls.json')
        return report.get('controls', report.get('effects', report.get('manual_controls', [])))

    def maps(self):
        return list(training_maps(self.root).values())

    def check_config(self, config):
        hero_id = config.get('local_hero_id', DEFAULT_HERO_ID)
        if hero_id != DEFAULT_HERO_ID and hero_id not in {hero['id'] for hero in self.heroes()}:
            raise ValueError('所选英雄不在当前原资源目录中')
        maps = {r['id']: r for r in self.maps()}
        if config['map_id'] not in maps:
            raise ValueError('所选地图不在当前资源目录中')
        if config['mode'] == 'deathmatch' and maps[config['map_id']].get('kind') == 'duel':
            raise ValueError('个人乱斗请选择 5v5 地图')
        if config['weapon_id'] and config['weapon_id'] not in {w['id'] for w in self.weapons()}:
            raise ValueError('未知武器')
        for map_id in config['routes']:
            if map_id not in maps:
                raise ValueError('路线引用了未知地图')
        for weapon, skin in config['skins'].items():
            self.skin_payload(weapon, skin['skin_id'], skin['level_id'], skin['chroma_id'])

    def skin_payload(self, weapon, skin, level, chroma):
        weapon = guid_id(weapon)
        if weapon not in {w['id'] for w in self.weapons()}:
            raise ValueError('未知武器')
        candidates = {s['id']: s for s in self.skins(weapon)}
        skin, level, chroma = guid_id(skin), guid_id(level), guid_id(chroma)
        if skin not in candidates:
            raise ValueError('此皮肤不属于所选武器')
        selected = candidates[skin]
        if level not in {s['id'] for s in selected['levels']}:
            raise ValueError('此等级不属于所选皮肤')
        valid_chromas = {c['id'] for c in selected['chromas']}
        if selected['default_chroma']:
            valid_chromas.add(selected['default_chroma'])
        if chroma not in valid_chromas:
            raise ValueError('此炫彩不属于所选皮肤')
        return b''.join(struct.pack('<4I', *guid_words(v)) for v in (weapon, skin, level, chroma))


class Runtime:
    def __init__(self, settings: Settings, catalog: Catalog, *, configuration_lock=None, backend='original'):
        self.settings, self.catalog = settings, catalog
        if backend not in ('original', 'local-server-candidate'):
            raise ValueError('Unknown local training backend')
        self.backend, self.backend_adapter = backend, None
        if backend == 'local-server-candidate':
            from local_server_controller import LocalServerBackend
            self.backend_adapter = LocalServerBackend(getattr(catalog, 'root', ROOT))
        self.lock = threading.RLock()
        self.configuration_lock = configuration_lock or threading.RLock()
        self.submitted_revision = -1
        self.startup_applying = False
        self.operation_lock = threading.Lock()
        self.launch_cancelled = threading.Event()
        self.restart_pending = False
        self.process = self.session = self.log = self.channel = None
        self.active = None
        self.status, self.error = 'stopped', None
        self.last_command = None
        self.launch_config = None
        self._reset_effect_sync()
        self.closed = threading.Event()
        self.worker = threading.Thread(target=self._monitor, name='trainer-state', daemon=True)
        self.worker.start()

    def _monitor(self):
        while not self.closed.wait(.15):
            with self.lock:
                if not self.process:
                    continue
                if self.process.poll() is not None:
                    code = self.process.returncode
                    report = read_json(self.session / ('verification.json' if self.backend_adapter else 'result.json'), {})
                    native_error = report.get('native_module_error') or {}
                    failure = native_error.get('stage') or report.get('error')
                    unexpected = bool(failure) or bool(code) and self.status != 'stopping'
                    if self.channel:
                        self.channel.close()
                    self.channel = None
                    self.status = 'error' if unexpected else 'stopped'
                    if unexpected:
                        self.error = f'训练器退出：{runtime_text(failure) if failure else f"退出码 {code}"}；请查看本次会话日志'
                    else:
                        self.error = None
                    self.process = None
                    if self.log:
                        self.log.close()
                    self.log = None
                    continue
                try:
                    active = (self.backend_adapter.active(self.session, self.process) if self.backend_adapter
                              else read_json(self.session / 'result.active.json'))
                except (OSError, ValueError, RuntimeError) as error:
                    self.error = runtime_text(error)
                    self.status = 'error'
                    continue
                if active:
                    # The active record is written by the specific wrapper that
                    # received this newly allocated session directory.
                    self.active = active
                    if self.channel is None and self.status != 'stopping':
                        try:
                            self.channel = (self.backend_adapter.open_channel(active) if self.backend_adapter
                                            else Channel(active['pid']))
                            self.status = 'running'
                            self.startup_applying = True
                            threading.Thread(target=self._apply_saved, daemon=True).start()
                        except (FileNotFoundError, OSError):
                            pass
                        except RuntimeError as error:
                            self.error = str(error)
            try:
                self.sync_native()
            except (OSError, ValueError, RuntimeError) as error:
                with self.lock:
                    self.error = runtime_text(error)
            self.sync_effects()

    def _apply_saved(self):
        try:
            with self.lock:
                channel = self.channel
            deadline = time.monotonic() + 180
            while not self.closed.wait(.1):
                with self.lock:
                    if self.channel is not channel or self.status != 'running':
                        return
                    try:
                        game = channel.state()
                    except RuntimeError as error:
                        if str(error) == 'Game state update is busy':
                            continue
                        raise
                if game.get('error'):
                    raise RuntimeError(runtime_text(game['error']))
                if game.get('ready'):
                    break
                if time.monotonic() >= deadline:
                    raise RuntimeError('场景准备超时，已保存设置尚未应用；请查看本次会话日志')
            else:
                return
            with self.configuration_lock:
                with self.lock:
                    if self.channel is not channel or self.status != 'running':
                        return
                current = self.settings.read()
                # Initial config is already loaded by the runner. Preserve an F6
                # toggle before this first ready snapshot unless the page changed
                # the preference while the scene was loading.
                initial = self.launch_config or {}
                if (game.get('map_id') == current['map_id'] and isinstance(game.get('infinite_ammo'), bool)
                        and current['infinite_ammo'] == initial.get('infinite_ammo')
                        and game['infinite_ammo'] != current['infinite_ammo']):
                    current = self.settings.update({'infinite_ammo': game['infinite_ammo']}, current['revision'])
                self.apply(current, selections=True)
        except (OSError, RuntimeError, ValueError) as error:
            with self.lock:
                if self.channel is channel:
                    self.error = str(error)
        finally:
            with self.lock:
                if self.channel is channel:
                    self.startup_applying = False

    def sync_native(self):
        """Persist F6 without echoing another Configure back to the game."""
        with self.configuration_lock:
            with self.lock:
                if self.status != 'running' or self.startup_applying or not self.channel:
                    return
                try:
                    game = self.channel.state()
                except RuntimeError as error:
                    if str(error) == 'Game state update is busy':
                        return
                    raise
                if game.get('applied_revision', -1) < self.submitted_revision:
                    return  # A snapshot from before the latest requested settings.
            current = self.settings.read()
            ammo = game.get('infinite_ammo')
            if (game.get('ready') and game.get('map_id') == current['map_id']
                    and isinstance(ammo, bool) and ammo != current['infinite_ammo']):
                self.settings.update({'infinite_ammo': ammo}, current['revision'])

    def _reset_effect_sync(self):
        self.effect_context = None
        self.effect_retry = {}
        self.effect_errors = {}
        self.effect_sync_error = None
        self.pending_startup_skin = False

    @staticmethod
    def _skin_matches(weapon, selected):
        return all(guid_id(weapon.get(key)) == guid_id(selected[key])
                   for key in ('skin_id', 'level_id', 'chroma_id'))

    def _apply_default_skin(self, config, game, *, during_startup=False):
        """Refresh an already equipped default gun after SaveSkinSelection."""
        if not self.pending_startup_skin:
            return False
        if config['weapon_id']:
            self.pending_startup_skin = False  # A newer explicit selection wins.
            return False
        weapon = game.get('weapon') or {}
        weapon_id = guid_id(weapon.get('id'))
        if not game.get('ready') or not weapon.get('ready') or not weapon_id or game.get('map_id') != config['map_id']:
            return False
        selected = config['skins'].get(weapon_id)
        if selected and not self._skin_matches(weapon, selected):
            payload = self.catalog.skin_payload(weapon_id, selected['skin_id'], selected['level_id'], selected['chroma_id'])
            result = self.command('skin', payload, during_startup=during_startup, defer_unavailable=True)
            if result.get('code') == 3 or result['status'] == 'saved':
                return True  # Keep the intent for the next ready snapshot.
            self.pending_startup_skin = False
            return True  # Wait for real skin readback before sending effects.
        self.pending_startup_skin = False
        return False

    def sync_effects(self):
        """Only the monitor sends deferred effects; HTTP/config writes stay independent."""
        with self.configuration_lock:
            try:
                self._sync_effects()
            except (OSError, RuntimeError, ValueError) as error:
                self.effect_errors['_transport'] = runtime_text(error)
            with self.lock:
                self.effect_sync_error = '；'.join(self.effect_errors.values()) or None

    def _sync_effects(self):
        with self.lock:
            if self.status != 'running' or self.startup_applying or not self.channel:
                return
            try:
                game = self.channel.state()
            except RuntimeError as error:
                if str(error) == 'Game state update is busy':
                    return
                raise
        self.effect_errors.pop('_transport', None)
        config = self.settings.read()
        if (not game.get('ready') or game.get('error') or game.get('map_id') != config['map_id']
                or game.get('applied_revision', -1) < self.submitted_revision):
            return
        weapon = game.get('weapon') or {}
        if not weapon.get('ready'):
            return
        context = tuple(guid_id(weapon.get(key)) for key in ('id', 'skin_id', 'level_id', 'chroma_id'))
        if not context[0]:
            return
        if context != self.effect_context:
            self.effect_context = context
            self.effect_retry.clear()
            self.effect_errors.clear()
        now = time.monotonic()
        if self.pending_startup_skin:
            if now < self.effect_retry.get('_startup_skin', (None, 0))[1]:
                return
            self.effect_retry['_startup_skin'] = (None, now + 1)
            try:
                queued = self._apply_default_skin(config, game)
                self.effect_errors.pop('_startup_skin', None)
            except (OSError, RuntimeError, ValueError) as error:
                self.effect_errors['_startup_skin'] = f'默认武器外观同步失败：{runtime_text(error)}'
                return
            if queued:
                return
        selected = config['skins'].get(context[0])
        if selected and not self._skin_matches(weapon, selected):
            return  # Weapon/Skin ACK is only a queue receipt, not an attachment.
        controls = {row.get('key', row.get('id')): row for row in game.get('effects', [])}
        for key in tuple(self.effect_errors):
            if key not in config['effects'] or key not in controls:
                self.effect_errors.pop(key, None)
        for key, desired in config['effects'].items():
            control = controls.get(key)
            if (not control or not control.get('supported')
                    or not control.get('available') and getattr(self, 'backend', 'original') != 'local-server-candidate'):
                continue  # This skin may never expose a saved key from another skin.
            numeric = bool(control.get('numeric'))
            value = desired.get('value', 0) if numeric else 0
            actual = control.get('value')
            matches_value = (not numeric or isinstance(actual, (float, int))
                             and math.isclose(actual, value, rel_tol=1e-6, abs_tol=1e-6))
            if control.get('requested') == desired['mode'] and matches_value:
                self.effect_errors.pop(key, None)
                continue  # Preserve native applied/verified; do not invent success.
            signature = (desired['mode'], value)
            previous, retry_at = self.effect_retry.get(key, (None, 0))
            if previous == signature and now < retry_at:
                continue
            self.effect_retry[key] = (signature, now + 1)
            if numeric and (not control.get('minimum', -math.inf) <= value <= control.get('maximum', math.inf)
                            or key == 'skin_kill_count' and value != int(value)):
                self.effect_errors[key] = f'皮肤效果 {control.get("label", key)} 的已保存数值超出当前允许范围，请调整后重试'
                continue
            payload = key.encode('ascii').ljust(80, b'\0') + struct.pack('<If', desired['mode'], value)
            try:
                # Availability can change between the published snapshot and dispatch.
                result = self.command('effect', payload, defer_unavailable=True)
                if result.get('code') != 3:
                    self.effect_errors.pop(key, None)
            except (OSError, RuntimeError, ValueError) as error:
                self.effect_errors[key] = f'皮肤效果同步失败：{runtime_text(error)}'
                self.effect_retry[key] = (signature, now + 2)
            return  # One command per observation; allow native state to catch up.

    def check_config(self, config):
        if getattr(self, 'backend_adapter', None):
            self.backend_adapter.check_config(config)
        elif config.get('local_hero_id', DEFAULT_HERO_ID) != DEFAULT_HERO_ID:
            raise ValueError('原训练器固定使用捷风；请选择本地服务器候选以切换英雄')

    def _prepare_launch(self, config, tag):
        if self.backend_adapter:
            return self.backend_adapter.prepare_launch(config, tag)
        from native_module import verified_build
        manifest = verified_build()
        if 'native/training_control.h' not in manifest['sha256']:
            raise RuntimeError('本地模块尚未构建 WebUI 控制功能')
        session = ROOT / '.local/training-sessions' / tag
        config_path = ROOT / '.local/controller/sessions' / (tag + '.json')
        atomic_json(config_path, config)
        command = [sys.executable, '-X', 'utf8', str(ROOT / 'tools/run_duel1_training.py'), '--prebuilt',
                   '--controller-settings', str(config_path), '--session-dir', str(session)]
        return session, command

    def start_async(self, *, restart=False):
        with self.lock:
            if not self.operation_lock.acquire(False):
                if restart:
                    self.restart_pending = True
                return {'status': 'loading', 'message': '启动或地图切换仍在进行'}
            if self.process and not restart:
                self.operation_lock.release()
                return {'status': self.status, 'message': '训练器已启动'}
            self.launch_cancelled.clear()
            self.restart_pending = False
            self.status, self.error = 'loading', None
        def work():
            owns_operation = True
            try:
                replacing = restart
                while not self.launch_cancelled.is_set():
                    if replacing:
                        self._stop_owned_process()
                    if self.launch_cancelled.is_set():
                        break
                    with self.lock:
                        self.restart_pending = False
                    with self.configuration_lock:
                        config = self.settings.persist_roster() if self.backend_adapter and hasattr(self.settings, 'persist_roster') else self.settings.read()
                    self.catalog.check_config(config)
                    self.check_config(config)
                    tag = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
                    session, command = self._prepare_launch(config, tag)
                    if self.launch_cancelled.is_set():
                        break
                    logs = ((self.backend_adapter.controller_root if self.backend_adapter else ROOT / '.local/controller')
                            / 'logs')
                    logs.mkdir(parents=True, exist_ok=True)
                    log = (logs / (tag + '.log')).open('w', encoding='utf-8')
                    try:
                        process = subprocess.Popen(command, cwd=ROOT, stdout=log, stderr=log,
                                                   creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0), env=project_environment(ROOT))
                    except Exception:
                        log.close()
                        raise
                    with self.lock:
                        self.process, self.session, self.log = process, session, log
                        self.active, self.channel = None, None
                        self.launch_config = config
                        self._reset_effect_sync()
                        self.submitted_revision = config['revision']
                        self.status, self.error = 'loading', None
                        if not self.launch_cancelled.is_set() and not self.restart_pending:
                            self.operation_lock.release()
                            owns_operation = False
                            return
                        replacing = True
                self._stop_owned_process()
            except Exception as error:
                with self.lock:
                    self.status, self.error = 'error', str(error)
            finally:
                if owns_operation:
                    self.operation_lock.release()
        threading.Thread(target=work, name='trainer-launch', daemon=True).start()
        return {'status': 'loading', 'message': '正在加载本地场景'}

    def stop(self):
        # Cancel before waiting for the launch lock. A process created while
        # Popen is completing must still be cleaned up before Stop returns.
        with self.lock:
            self.launch_cancelled.set()
            self.restart_pending = False
        with self.operation_lock:
            return self._stop_owned_process()

    def _stop_owned_process(self):
        with self.lock:
            process, active = self.process, self.active
            if not process:
                self.status = 'stopped'
                return {'status': 'stopped'}
            session = self.session
            self.status = 'stopping'
        # Request the existing runner's own lifecycle cleanup. No process-name
        # kill or opening another installation's handle is involved.
        deadline = time.monotonic() + (50 if self.backend_adapter else 20)
        requested = False
        while process.poll() is None and time.monotonic() < deadline:
            if not requested:
                if self.backend_adapter:
                    try:
                        requested = self.backend_adapter.request_stop(session, process)
                    except RuntimeError:
                        if process.poll() is not None:
                            break
                        raise
                else:
                    active = read_json(session / 'result.active.json', active)
                if active and not self.backend_adapter:
                    run = Path(active['run_dir']).resolve()
                    if ROOT / '.local/native-baseline' not in run.parents:
                        raise RuntimeError('会话目录身份不匹配')
                    atomic_json(run / 'stop-request.json', {'reason': 'local_webui_stop'})
                    requested = True
            time.sleep(.1)
        if process.poll() is None:
            raise RuntimeError('原会话仍在清理，请稍后重试')
        with self.lock:
            if self.channel:
                self.channel.close()
            self.channel = None
            self.process = None
            self.active = None
            if self.log:
                self.log.close()
            self.log = None
            self.status = 'stopped'
        return {'status': 'stopped'}

    def command(self, kind, payload=b'', *, during_startup=False, defer_unavailable=False):
        with self.lock:
            channel = self.channel
            waiting = self.startup_applying and not during_startup
        if channel is None or waiting:
            return {'status': 'saved', 'message': '已保存，训练器启动后生效'}
        result = channel.send(kind, payload)
        result['message_text'] = runtime_text(result.get('message'))
        with self.lock:
            self.last_command = {'kind': kind, **result}
        if result['code'] >= 2 and not (defer_unavailable and result['code'] == 3):
            raise RuntimeError(result.get('message_text') or '客户端未能应用此设置')
        return result

    def apply(self, config, *, selections=False):
        with self.configuration_lock:
            return self._apply(config, selections=selections)

    def _apply(self, config, *, selections=False):
        if self.backend_adapter and config.get('human_bots', {}).get('bot_roster') is not None:
            from human_bot_roster import live_match, pending_restart
            with self.lock:
                channel = self.channel
            if channel is None:
                return {'status': 'saved'}
            try:
                matches = live_match(config, channel.state())
            except RuntimeError:
                matches = False
            if not matches:
                return pending_restart()
        with self.lock:
            self.submitted_revision = config['revision']
        def send(kind, payload):
            return self.command(kind, payload, during_startup=selections)
        result = send('configure', pack_configuration(config))
        send('routes', pack_practice_routes(config))
        if config.get('schema_version') == 3 and self.backend_adapter:
            with self.lock:
                human_channel = self.channel
                human_waiting = self.startup_applying and not selections
            if human_channel is not None and not human_waiting:
                from human_bot_integration import apply_to_channel
                human_result = apply_to_channel(human_channel, config)
                effective = human_result.get('consumer_effective') is True
                result = {**result, 'status': 'applied' if effective else 'pending',
                          'code': 0 if effective else 1, 'human_bots': human_result,
                          'message': '人机参数已确认应用' if effective else '人机参数已提交，等待当前决策应用'}
        if selections:
            self.pending_startup_skin = not config['weapon_id'] and bool(config['skins'])
            for weapon, skin in config['skins'].items():
                send('save_skin_selection', self.catalog.skin_payload(weapon, skin['skin_id'], skin['level_id'], skin['chroma_id']))
        if selections and config['weapon_id']:
            send('weapon', struct.pack('<4I', *guid_words(config['weapon_id'])))
            skin = config['skins'].get(config['weapon_id'])
            if skin:
                send('skin', self.catalog.skin_payload(config['weapon_id'], skin['skin_id'], skin['level_id'], skin['chroma_id']))
        elif selections and self.pending_startup_skin:
            with self.lock:
                try:
                    game = self.channel.state() if self.channel else {}
                except RuntimeError as error:
                    if str(error) != 'Game state update is busy':
                        raise
                    game = {}  # The monitor will use the next complete snapshot.
            self._apply_default_skin(config, game, during_startup=True)
        # Effects are applied by the monitor only after the actual skin and its
        # controls are ready; a saved key for a different skin is not a failure.
        return result

    def state(self):
        with self.lock:
            result = {'status': self.status, 'error': self.error, 'pid': self.active.get('pid') if self.active else None,
                      'session': str(self.session) if self.session else None, 'last_command': self.last_command,
                      'game': {}, 'rules': {'kill_limit': 40, 'time_limit_seconds': 540, 'abilities_in_deathmatch': False}}
            result['backend'] = getattr(self, 'backend', 'original')
            if result['backend'] == 'local-server-candidate':
                result.update(developmental=True, playable_verified=False,
                              backend_notice='本地服务器开发候选，尚未通过可玩验收',
                              host_pid=self.active.get('pid') if self.active else None,
                              client_pid=self.active.get('client_pid') if self.active else None)
            if self.channel:
                try:
                    result['game'] = describe_game(self.channel.state())
                except (OSError, ValueError, RuntimeError) as error:
                    result['state_error'] = runtime_text(error)
            result['effect_sync_error'] = self.effect_sync_error
            result['error_text'] = runtime_text(result['error'] or self.effect_sync_error)
            return result

    def close(self):
        try:
            self.stop()
        finally:
            self.closed.set()
            self.worker.join(timeout=2)


class Application:
    def __init__(self, root=ROOT, *, runtime=True, backend='original', settings_file=None):
        self.root = Path(root)
        self.backend = backend
        self.settings = open_settings(self.root, backend, settings_file)
        self.catalog = Catalog(self.root)
        self.mutation = threading.RLock()
        self.runtime = Runtime(self.settings, self.catalog, configuration_lock=self.mutation, backend=backend) if runtime else None

    def maps(self):
        if self.backend == 'local-server-candidate':
            from local_server_maps import catalog_maps
            return catalog_maps(self.root)
        return self.catalog.maps()

    def capabilities(self):
        result = {'local_hero_selection': local_hero_capability(self.root, backend=self.backend)}
        if self.backend == 'local-server-candidate':
            from human_bot_integration import capabilities
            result['human_bots'] = capabilities(self.runtime.state() if self.runtime else None)
        return result

    def human_bot_catalog(self):
        if self.backend != 'local-server-candidate':
            raise ValueError('拟人人机设置用于本地服务器模式')
        from human_bot_integration import catalog
        return catalog(self.runtime.state() if self.runtime else None)

    def human_bot_preview(self, index=None, *, entry_id=None, draft=None, keys=None):
        if self.backend != 'local-server-candidate':
            raise ValueError('拟人人机设置用于本地服务器模式')
        from human_bot_integration import preview
        with self.mutation:
            return preview(self.settings.read(), index, entry_id=entry_id, draft=draft, keys=keys,
                           runtime=self.runtime.state() if self.runtime else None)

    def spectate_bot(self, index):
        if self.backend != 'local-server-candidate':
            raise ValueError('观战仅适用于本地服务器候选')
        if type(index) is not int or not 0 <= index < MAX_UNIQUE_PLAYERS:
            raise ValueError('无效观战序号')
        runtime = self.runtime
        if runtime is None or getattr(runtime, 'backend', None) != 'local-server-candidate':
            raise RuntimeError('本地服务器尚未运行')
        # Saved roster edits may belong to the next launch. PairChannel validates
        # this index against the current ready host and its actual native IDs.
        with runtime.lock:
            if runtime.status != 'running' or runtime.channel is None or runtime.startup_applying:
                raise RuntimeError('本地服务器尚未准备好观战')
            return runtime.command('spectate_bot', struct.pack('<I', index))

    def human_bot_roster_action(self, body):
        if self.backend != 'local-server-candidate':
            raise ValueError('固定人机名单用于本地服务器模式')
        from human_bot_integration import roster_action
        with self.mutation:
            operation = roster_action(self.settings.read(), body)
            response = self.update(operation['patch'], operation['expected_revision'])
            response['roster_action'] = operation['roster_action']
            return response

    def update(self, patch, revision):
        from training_settings_v2 import merge
        with self.mutation:
            current = self.settings.read()
            if isinstance(revision, bool) or revision != current['revision']:
                raise RevisionConflict('设置已在另一页面或游戏中更新，请刷新后再试')
            if self.runtime:
                self.runtime.sync_native()
                current = self.settings.read()
            proposed = (self.settings.propose(patch, current['revision'])
                        if hasattr(self.settings, 'propose') else merge(current, patch))
            if self.backend == 'original' and proposed['local_hero_id'] != DEFAULT_HERO_ID:
                raise ValueError('原训练器固定使用捷风；请选择本地服务器候选以切换英雄')
            self.catalog.check_config(proposed)
            if self.runtime:
                self.runtime.check_config(proposed)
            config = self.settings.update(patch, current['revision'])
            if self.backend == 'local-server-candidate' and config.get('schema_version') == 3:
                from human_bot_roster import structure_changed, pending_restart
                if structure_changed(current, config):
                    return {'config': config, 'runtime': pending_restart()}
            restart = any(config[k] != current[k] for k in ('mode', 'map_id', 'total_players', 'local_hero_id'))
            if self.runtime:
                running = self.runtime.state()['status'] in ('running', 'loading')
                applied = self.runtime.start_async(restart=True) if restart and running else self.runtime.apply(config)
            else:
                applied = {'status': 'saved'}
            return {'config': config, 'runtime': applied}


class Handler(BaseHTTPRequestHandler):
    server_version = 'LocalTrainer/2'

    def log_message(self, format, *args):
        pass

    def _json(self, value, status=200):
        payload = json.dumps(value, ensure_ascii=False, allow_nan=False).encode('utf-8')
        self.send_response(status)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.send_header('Cache-Control', 'no-store')
        self.send_header('Content-Length', str(len(payload)))
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.end_headers()
        self.wfile.write(payload)

    def _body(self):
        length = int(self.headers.get('Content-Length', '0'))
        if not 0 < length <= 16 * 1024 * 1024:
            raise ValueError('无效请求大小')
        result = json.loads(self.rfile.read(length))
        if not isinstance(result, dict):
            raise ValueError('请求必须为对象')
        return result

    def _same_origin(self):
        host = self.headers.get('Host', '')
        if host not in (f'127.0.0.1:{self.server.server_port}', f'localhost:{self.server.server_port}'):
            raise ValueError('只接受本机控制页面')
        origin = self.headers.get('Origin')
        if origin and origin not in ('http://' + host,):
            raise ValueError('请求来源不匹配')

    def do_GET(self):
        try:
            self._same_origin()
            path = urlsplit(self.path).path
            app = self.server.app
            if path == '/api/skin-preview' or path.startswith('/api/skin-previews/'):
                from training_skin_images import previews_for
                result = previews_for(app.root).handle_request(self.path)
                self.send_response(result.status)
                self.send_header('Content-Type', result.content_type)
                self.send_header('Cache-Control', result.cache_control)
                self.send_header('Content-Length', str(len(result.payload)))
                self.send_header('X-Content-Type-Options', 'nosniff')
                self.end_headers()
                self.wfile.write(result.payload)
                return
            if path == '/api/identity':
                return self._json({'application': 'valoranttrainer', 'protocol': 2, 'root': str(app.root.resolve()),
                                   'backend': app.backend, 'developmental': app.backend == 'local-server-candidate',
                                   'settings_file': str(app.settings.path.resolve())})
            if path == '/api/state':
                with app.mutation:
                    if app.runtime:
                        app.runtime.sync_native()
                    snapshot = {'config': app.settings.read(), 'runtime': app.runtime.state() if app.runtime else {'status': 'stopped'}}
                return self._json(snapshot)
            if path == '/api/catalog':
                local_hero_id = app.settings.read().get('local_hero_id', DEFAULT_HERO_ID)
                heroes = app.catalog.heroes()
                return self._json({'weapons': app.catalog.weapons(), 'maps': app.maps(),
                                   'effects': app.catalog.effects(), 'styles': STYLE_LABELS, 'route_styles': ROUTE_STYLE_LABELS,
                                   'heroes': heroes, 'capabilities': app.capabilities(),
                                   'limits': {'max_players': MAX_UNIQUE_PLAYERS, 'unique_heroes': True,
                                              'local_hero_id': local_hero_id,
                                              'local_hero': next((hero['name'] for hero in heroes if hero['id'] == local_hero_id), 'Jett')}})
            if path == '/api/human-bots/spec':
                return self._json(app.human_bot_catalog())
            if path == '/api/human-bots/preview':
                from urllib.parse import parse_qs
                query = parse_qs(urlsplit(self.path).query)
                if set(query) != {'index'} or len(query['index']) != 1:
                    raise ValueError('请提供一个人机编号')
                text = query['index'][0]
                if not text.isascii() or not text.isdecimal() or text != str(int(text)):
                    raise ValueError('无效人机编号')
                return self._json(app.human_bot_preview(int(text)))
            if path == '/api/skins':
                from urllib.parse import parse_qs
                weapon = parse_qs(urlsplit(self.path).query).get('weapon', [None])[0]
                return self._json({'skins': app.catalog.skins(guid_id(weapon))})
            if path.startswith('/api/maps/') and path.endswith('/image'):
                key = path.split('/')[3]
                info = next((m for m in app.maps() if m['id'] == key), None)
                image = (info or {}).get('minimap', {})
                relative = image.get('relative_path') if isinstance(image, dict) else image
                if not relative:
                    return self._json({'error': '地图图像尚未提取'}, 404)
                target = (app.root / relative).resolve()
                if app.root not in target.parents or target.suffix.lower() not in ('.png', '.webp', '.jpg', '.svg'):
                    raise ValueError('无效图像路径')
                return self._file(target)
            if path.startswith('/api/'):
                return self._json({'error': '接口不存在'}, 404)
            relative = unquote(path.lstrip('/')) or 'index.html'
            target = (WEB / relative).resolve()
            if WEB not in target.parents or target.suffix not in ('.html', '.css', '.js', '.svg', '.woff2'):
                return self._json({'error': '页面不存在'}, 404)
            return self._file(target)
        except (OSError, ValueError, RuntimeError) as error:
            self._json({'error': str(error)}, 400)

    def _file(self, path):
        try:
            payload = Path(path).read_bytes()
        except FileNotFoundError:
            return self._json({'error': '文件不存在'}, 404)
        self.send_response(200)
        self.send_header('Content-Type', mimetypes.guess_type(str(path))[0] or 'application/octet-stream')
        self.send_header('Content-Length', str(len(payload)))
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.send_header('Content-Security-Policy', "default-src 'self'; style-src 'self'; img-src 'self' data:; script-src 'self'; connect-src 'self'; frame-ancestors 'none'")
        self.end_headers()
        self.wfile.write(payload)

    def do_POST(self):
        self._mutate()

    def do_PATCH(self):
        self._mutate()

    def _mutate(self):
        try:
            self._same_origin()
            path, app = urlsplit(self.path).path, self.server.app
            body = self._body()
            if path == '/api/human-bots/roster' and self.command == 'POST':
                return self._json(app.human_bot_roster_action(body))
            if path == '/api/human-bots/preview' and self.command == 'POST':
                allowed = {'roster_index', 'entry_id', 'human_bots', 'seed', 'total_players', 'keys'}
                if set(body) - allowed or ('roster_index' in body) == ('entry_id' in body):
                    raise ValueError('无效配置预览字段')
                draft = {key: body[key] for key in ('human_bots', 'seed', 'total_players') if key in body}
                return self._json(app.human_bot_preview(body.get('roster_index'), entry_id=body.get('entry_id'),
                                                       draft=draft or None, keys=body.get('keys')))
            if path == '/api/spectate-bot' and self.command == 'POST':
                if not isinstance(body, dict) or set(body) != {'roster_index'}:
                    raise ValueError('观战请求只接受roster_index')
                return self._json(app.spectate_bot(body['roster_index']))
            if path == '/api/config' and self.command == 'PATCH':
                return self._json(app.update(body['patch'], body['revision']))
            if path in ('/api/start', '/api/restart'):
                return self._json(app.runtime.start_async(restart=path == '/api/restart'))
            if path == '/api/stop':
                return self._json(app.runtime.stop())
            if path == '/api/restart-match':
                return self._json(app.runtime.start_async(restart=True))
            if path == '/api/validate-routes':
                return self._json(app.runtime.command('validate_routes'))
            if path == '/api/weapon':
                weapon = guid_id(body['weapon_id'])
                if weapon not in {w['id'] for w in app.catalog.weapons()}:
                    raise ValueError('未知武器')
                updated = app.update({'weapon_id': weapon}, body['revision'])
                updated['runtime'] = app.runtime.command('weapon', struct.pack('<4I', *guid_words(weapon)))
                saved = updated['config']['skins'].get(weapon)
                if saved:
                    updated['skin_runtime'] = app.runtime.command('skin', app.catalog.skin_payload(weapon, saved['skin_id'], saved['level_id'], saved['chroma_id']))
                return self._json(updated)
            if path == '/api/skin':
                weapon = guid_id(body['weapon_id'])
                payload = app.catalog.skin_payload(weapon, body['skin_id'], body['level_id'], body['chroma_id'])
                skins = app.settings.read()['skins']
                skins[weapon] = {k: guid_id(body[k]) for k in ('skin_id', 'level_id', 'chroma_id')}
                updated = app.update({'skins': skins}, body['revision'])
                updated['runtime'] = app.runtime.command('skin', payload)
                return self._json(updated)
            if path == '/api/effect':
                key, mode = body['key'], body['mode']
                live = app.runtime.state().get('game', {}).get('effects', [])
                controls = [*live, *app.catalog.effects()]
                control = next((c for c in controls if c.get('key', c.get('id')) == key), None)
                if (not control or not isinstance(key, str) or not key.isascii() or not 0 < len(key) < 80
                        or isinstance(mode, bool) or mode not in (0, 1, 2)):
                    raise ValueError('未知特效或模式')
                numeric = key == 'skin_kill_count' or control.get('numeric')
                from training_settings_v2 import _number
                effect_value = _number(body.get('value', 0), control.get('minimum', 0), control.get('maximum', 25), '特效参数', integer=key == 'skin_kill_count') if numeric else 0
                effects = app.settings.read()['effects']
                effects[key] = {'mode': mode, 'value': effect_value}
                updated = app.update({'effects': effects}, body['revision'])
                if updated['runtime'].get('status') != 'saved':
                    updated['runtime'] = {'status': 'pending', 'message': '效果设置已保存，等待当前外观的对应效果就绪'}
                return self._json(updated)
            if path == '/api/map-point':
                key = body['map_id']
                info = next((m for m in app.maps() if m['id'] == key), None)
                transform = (info or {}).get('world_to_image') or (info or {}).get('minimap', {}).get('world_to_image')
                if not transform:
                    raise ValueError('此地图没有可用的原坐标变换')
                uv = body['uv']
                if len(uv) != 2 or not all(isinstance(v, (int, float)) and not isinstance(v, bool) and 0 <= v <= 1 for v in uv):
                    raise ValueError('地图点超出范围')
                position = uv_to_world(transform, uv, body.get('z', 0))
                live = app.runtime.state() if app.runtime else {}
                if live.get('status') == 'running' and live.get('game', {}).get('map_id') == key and live['game'].get('ready'):
                    result = app.runtime.command('resolve_map_point', struct.pack('<3f', *position))
                    if result.get('code') == 0:
                        verified = json.loads(result['message'])
                        verified['validation'] = 'original_ground_and_capsule_verified'
                        return self._json(verified)
                return self._json({'position': position, 'validation': 'pending_native_navigation'})
            return self._json({'error': '接口不存在'}, 404)
        except RevisionConflict as error:
            self._json({'error': str(error), 'config': self.server.app.settings.read()}, 409)
        except (ValueError, KeyError, TypeError) as error:
            self._json({'error': str(error)}, 400)
        except (OSError, RuntimeError) as error:
            self._json({'error': runtime_text(error)}, 503)


def make_server(port=8790, *, app=None, backend='original', settings_file=None):
    server = ThreadingHTTPServer(('127.0.0.1', port), Handler)
    server.daemon_threads = True
    server.app = app or Application(backend=backend, settings_file=settings_file)
    return server


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=8790)
    parser.add_argument('--open', action='store_true')
    parser.add_argument('--start', action='store_true')
    parser.add_argument('--build', action='store_true')
    parser.add_argument('--backend', choices=('original', 'local-server-candidate'), default='original',
                        help='Explicit development opt-in; the existing trainer remains the default')
    parser.add_argument('--settings-file', '--config', type=Path,
                        help='Candidate-only independent settings JSON inside project .local')
    args = parser.parse_args()
    if not 1024 <= args.port <= 65535:
        parser.error('Use a local port between 1024 and 65535')
    selected_settings = settings_path(ROOT, args.backend, args.settings_file)
    os.environ.update(project_environment(ROOT))
    address = f'http://127.0.0.1:{args.port}'
    from urllib.request import build_opener, ProxyHandler, Request
    opener = build_opener(ProxyHandler({}))
    try:
        with opener.open(address + '/api/identity', timeout=.8) as response:
            identity = json.loads(response.read(8192))
        existing = identity.get('application') == 'valoranttrainer' and identity.get('protocol') == 2 and Path(identity.get('root', '')).resolve() == ROOT
    except (OSError, ValueError):
        existing = False
    if existing:
        if identity.get('backend', 'original') != args.backend:
            raise RuntimeError('此端口已运行另一训练后端，请使用不同的本机端口启动候选控制台')
        if not identity_matches_settings(identity, selected_settings, root=ROOT, backend=args.backend):
            raise RuntimeError('此端口控制台的设置文件与本次请求不一致，请使用其他本机端口或重新启动该候选控制台')
        if args.build:
            raise RuntimeError('控制台正在运行，请先退出控制台再重新构建')
        if args.start:
            with opener.open(Request(address + '/api/start', data=b'{}', headers={'Content-Type': 'application/json'}), timeout=5) as response:
                response.read(8192)
        if args.open:
            webbrowser.open(address)
        print(address, flush=True)
        return
    if args.build:
        if args.backend == 'local-server-candidate':
            from local_server_module import build
        else:
            from build_native_module import build
        build()
    server = make_server(args.port, backend=args.backend, settings_file=args.settings_file)
    address = f'http://127.0.0.1:{server.server_port}'
    server_record = 'server.json' if args.backend == 'original' else f'server-{server.server_port}.json'
    atomic_json(controller_directory(ROOT, args.backend) / server_record,
                {'pid': os.getpid(), 'url': address, 'bind': '127.0.0.1', 'backend': args.backend,
                 'settings_file': str(server.app.settings.path.resolve())})
    print(address, flush=True)
    if args.open:
        webbrowser.open(address)
    if args.start:
        server.app.runtime.start_async()
    def stop_signal(*_):
        threading.Thread(target=server.shutdown, daemon=True).start()
    signal.signal(signal.SIGINT, stop_signal)
    if hasattr(signal, 'SIGTERM'):
        signal.signal(signal.SIGTERM, stop_signal)
    try:
        server.serve_forever(poll_interval=.25)
    finally:
        server.server_close()
        server.app.runtime.close()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
