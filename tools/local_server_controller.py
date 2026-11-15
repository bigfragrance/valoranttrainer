"""Opt-in WebUI control for an exact, runner-owned loopback host/client pair.

This is a development backend. The authoritative V2 channel belongs to the
host; menu presentation uses the client, spectator commands use the host.
No PID-only attachment or process-name shutdown path is provided.
"""
from __future__ import annotations

import copy
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import sys
import threading
import time

from client_layout import client_executable
from local_server_module import FLAVOR
from local_server_maps import selected_map
from training_control import Channel, initial_payload
from training_settings_v2 import validate, MAX_UNIQUE_PLAYERS
from training_heroes import (DEFAULT_HERO_ID, SELECTION_NAME, selected_hero,
                             selection_payload, require_selection_build)
from training_controller_store import controller_directory

BACKEND = 'local-server-candidate'
KIND = 'native-shooter-candidate-interactive'
UI_COMMANDS = frozenset(('toggle_buy_menu', 'toggle_options', 'close_active_ui'))
TRAINING_SOURCES = frozenset(('native/training_control.h', 'native/local_server_runtime.h',
                              'native/local_server_control.h', 'native/local_server_match.h'))
HUMAN_CONFIG_SOURCES = frozenset(('native/human_bot_wire.h', 'native/human_bot_store.h',
                                 'native/human_bot_initial.h', 'native/humanlike_bot_config.h'))
HUMAN_RUNTIME_SOURCES = frozenset(('native/local_server_humanlike_bots.h',
                                  'native/humanlike_bot_brain.h', 'native/humanlike_bot_brain.inl',
                                  'native/local_server_humanlike_sensors.h',
                                  'native/local_server_humanlike_space.h',
                                  'native/humanlike_bot_weapon_feedback.h',
                                  'native/humanlike_bot_native_events.h'))
ROOT = Path(__file__).resolve().parents[1]


def read_json(path):
    try:
        value = json.loads(Path(path).read_text(encoding='utf-8'))
        return value if isinstance(value, dict) else {}
    except (FileNotFoundError, json.JSONDecodeError):
        return {}


def atomic_json(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    pending = path.with_suffix(path.suffix + '.tmp')
    pending.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')
    for attempt in range(5):
        try:
            pending.replace(path)
            return
        except PermissionError:
            if attempt == 4:
                raise
            time.sleep(.01)


def pair_directory(path, root=ROOT):
    root, path = Path(root).resolve(), Path(path).resolve()
    if path.parent != (root / '.local/local-server').resolve() or not path.name.startswith('pair-'):
        raise ValueError('Local server session must be a project-owned pair directory')
    return path


def check_candidate_config(config, *, root=ROOT):
    from human_bot_integration import validate_config
    config = validate_config(config)
    selected_map(config['map_id'], config['mode'], root=root)
    return config


def require_training_build(manifest, config=None):
    required = TRAINING_SOURCES
    if config and config.get('schema_version') == 3:
        required |= HUMAN_CONFIG_SOURCES
        if config.get('human_bots', {}).get('enabled'):
            required |= HUMAN_RUNTIME_SOURCES
    if config and config.get('human_bots', {}).get('bot_roster') is not None:
        from human_bot_roster import NATIVE_SOURCES
        required |= NATIVE_SOURCES
    if not required <= set(manifest.get('source_sha256', {})):
        raise RuntimeError('本地服务器候选尚未构建完整 Initial/V2 训练运行时；请先完成候选模块构建')


def _write_legacy_initial(directory, config, map_info, *, root=ROOT):
    """The native host reads this exact training_control::Initial before listen."""
    directory = Path(directory)
    config = check_candidate_config(config, root=root)
    payload = initial_payload(config, map_info)
    hero_payload = selection_payload(config, root=root)
    hero, profile = selected_hero(config.get('local_hero_id', DEFAULT_HERO_ID), root=root)
    path = directory / 'session-initial.bin'
    hero_path = directory / SELECTION_NAME
    if path.exists() or hero_path.exists():
        raise FileExistsError('An existing local server Initial or hero selection cannot be replaced')
    # A session must never silently replace the configuration of an older pair.
    with path.open('xb') as stream:
        stream.write(payload)
    with hero_path.open('xb') as stream:
        stream.write(hero_payload)
    return {'path': str(path), 'bytes': len(payload), 'sha256': hashlib.sha256(payload).hexdigest(),
            'revision': config['revision'], 'map_id': config['map_id'], 'mode': config['mode'],
            'hero_selection': {'path': str(hero_path), 'bytes': len(hero_payload),
                               'sha256': hashlib.sha256(hero_payload).hexdigest(),
                               'revision': config['revision'], 'hero_index': hero['index'],
                               'hero_id': hero['id'], 'hero_name': hero['name'],
                               'profile_sha256': profile['sha256']}}


def write_initial(directory, config, map_info, *, root=ROOT):
    if config.get('schema_version') == 3:
        from human_bot_integration import write_initial as write_human_initial
        return write_human_initial(directory, config, map_info,
                                   original_write_initial=_write_legacy_initial, root=root)
    return _write_legacy_initial(directory, config, map_info, root=root)


def current_runner_identity():
    from capture_owned_thread import _kernel, _creation, _image
    kernel = _kernel()
    handle = kernel.OpenProcess(0x1000 | 0x100000, False, os.getpid())
    if not handle:
        raise RuntimeError('Cannot publish the local server supervisor identity')
    try:
        return {'schema_version': 1, 'pid': os.getpid(), 'creation_filetime': _creation(kernel, handle),
                'executable': str(_image(kernel, handle)), 'published_from_own_process': True}
    finally:
        kernel.CloseHandle(handle)


def validate_pair_record(record, directory, process, *, module_sha256, root=ROOT):
    """Bind the supervisor receipt to the WebUI's original Popen handle."""
    directory = pair_directory(directory, root)
    identity = record.get('runner_identity') or {}
    if (record.get('schema_version') != 1 or record.get('kind') != KIND
            or record.get('network_flavor') != FLAVOR
            or record.get('directory') != str(directory)
            or not re.fullmatch('[0-9a-f]{32}', record.get('proof_id') or '')
            or record.get('module_sha256') != module_sha256
            or identity.get('schema_version') != 1
            or identity.get('published_from_own_process') is not True
            or identity.get('pid') != process.pid
            or type(identity.get('creation_filetime')) is not int
            or identity['creation_filetime'] <= 0
            or Path(identity.get('executable', '')).resolve() != Path(sys.executable).resolve()):
        raise RuntimeError('Local server supervisor ownership record does not match this launch')
    from capture_owned_thread import _kernel, _creation, _image
    kernel, handle = _kernel(), int(process._handle)
    if (process.poll() is not None or kernel.GetProcessId(handle) != process.pid
            or kernel.WaitForSingleObject(handle, 0) != 0x102
            or _creation(kernel, handle) != identity['creation_filetime']
            or _image(kernel, handle) != Path(identity['executable']).resolve()):
        raise RuntimeError('Local server supervisor process identity changed')
    return record


def validate_child_record(active, role, pair, *, root=ROOT):
    """Check all receipt fields before opening a game handle or a V2 channel."""
    root = Path(root).resolve()
    directory = pair_directory(pair['directory'], root)
    metadata, identity = active.get('network_probe') or {}, active.get('network_probe_identity') or {}
    pid = active.get('pid')
    project = read_json(root / 'project.json')
    expected = client_executable(project, root)
    if (role not in ('host', 'client') or type(pid) is not int or pid <= 0
            or metadata.get('role') != role or metadata.get('proof_id') != pair.get('proof_id')
            or metadata.get('flavor') != FLAVOR or metadata.get('port') != pair.get('port')
            or metadata.get('transport') != pair.get('transport')
            or Path(metadata.get('output', '')).resolve() != directory / role
            or identity.get('schema_version') != 1 or identity.get('pid') != pid
            or identity.get('network_flavor') != FLAVOR
            or identity.get('module_sha256') != pair.get('module_sha256')
            or identity.get('image_sha256') != project.get('expected_snapshot_sha256')
            or identity.get('published_from_original_popen_handle') is not True
            or type(identity.get('creation_filetime')) is not int or identity['creation_filetime'] <= 0
            or Path(identity.get('executable', '')).resolve() != expected):
        raise RuntimeError('Local server child ownership record does not match this pair')
    if pair.get('engine_mode') == 'training' and (
            active.get('mode') != 'training' or active.get('map_number') != pair.get('map_id')):
        raise RuntimeError('Local server child map or mode differs from this pair')
    run = Path(active.get('run_dir', '')).resolve()
    if (root / '.local/native-baseline').resolve() not in run.parents:
        raise RuntimeError('Local server child directory is outside the owned baseline runs')
    return active


class OwnedChild:
    """Retain a read-only identity handle, so PID reuse cannot retarget commands."""
    def __init__(self, active):
        from capture_owned_thread import _kernel, _creation, _image
        self.kernel, self.handle = _kernel(), None
        self.pid = active['pid']
        identity = active['network_probe_identity']
        self.handle = self.kernel.OpenProcess(0x1000 | 0x100000, False, self.pid)
        if not self.handle:
            raise RuntimeError('The owned local server child no longer exists')
        try:
            if (not self.alive() or _creation(self.kernel, self.handle) != identity['creation_filetime']
                    or _image(self.kernel, self.handle) != Path(identity['executable']).resolve()):
                raise RuntimeError('Local server child process identity changed')
        except Exception:
            self.close()
            raise

    def alive(self):
        return bool(self.handle and self.kernel.WaitForSingleObject(self.handle, 0) == 0x102)

    def close(self):
        if self.handle:
            self.kernel.CloseHandle(self.handle)
        self.handle = None


def _spectator_player(players, *, roster_index=None, native_player_id=None):
    """Resolve an exact current bot identity; roster slots are never player IDs."""
    if not isinstance(players, list) or (roster_index is None) == (native_player_id is None):
        return None
    key, value = ('index', roster_index) if roster_index is not None else ('native_player_id', native_player_id)
    if type(value) is not int:
        return None
    rows = [row for row in players if isinstance(row, dict)]
    matches = [row for row in rows if type(row.get(key)) is int and row[key] == value]
    if len(matches) != 1:
        return None
    target = matches[0]
    index, player_id = target.get('index'), target.get('native_player_id')
    if (target.get('bot') is not True or type(index) is not int or not 0 < index < MAX_UNIQUE_PLAYERS
            or type(player_id) is not int or not 0 < player_id <= 0x7fffffff):
        return None
    if (sum(type(row.get('index')) is int and row['index'] == index for row in rows) != 1
            or sum(type(row.get('native_player_id')) is int and row['native_player_id'] == player_id for row in rows) != 1):
        return None
    return target


def _spectator_roster_identity(host_state, target):
    """Optional current native frozen-entry identity, never a saved roster lookup."""
    roster = host_state.get('configured_bot_roster')
    if (not isinstance(roster, dict) or any(roster.get(key) is not True for key in
            ('loaded', 'fixed', 'validated', 'native_catalog_verified'))
            or type(roster.get('error')) is not int or roster['error'] != 0
            or type(roster.get('io_error')) is not int or roster['io_error'] != 0):
        return {}
    entries = roster.get('entries')
    if not isinstance(entries, list):
        return {}
    rows = [row for row in entries if isinstance(row, dict)]
    index, player_id = target['index'], target['native_player_id']
    matches = [row for row in rows if type(row.get('slot')) is int and row['slot'] == index
               and type(row.get('native_player_id')) is int and row['native_player_id'] == player_id]
    if len(matches) != 1:
        return {}
    row = matches[0]
    token, hero_id, hero = row.get('entry_token'), row.get('hero_id'), row.get('hero_index')
    hex_digits = '0123456789abcdef'
    if (not isinstance(token, str) or len(token) != 16 or any(c not in hex_digits for c in token)
            or int(token, 16) == 0 or target.get('entry_token') != token
            or not isinstance(hero_id, str) or len(hero_id) != 32 or any(c not in hex_digits for c in hero_id)
            or int(hero_id, 16) == 0 or type(hero) is not int or hero < 0
            or type(target.get('hero_index')) is not int or hero != target['hero_index']):
        return {}
    if (sum(type(item.get('slot')) is int and item['slot'] == index for item in rows) != 1
            or sum(type(item.get('native_player_id')) is int and item['native_player_id'] == player_id for item in rows) != 1
            or sum(item.get('entry_token') == token for item in rows) != 1):
        return {}
    return {'entry_token': token, 'hero_id': hero_id}


class PairChannel:
    def __init__(self, active, *, channel_factory=Channel, watch_factory=OwnedChild):
        self._lock = threading.RLock()
        self.pid = active['pid']  # Authoritative host, never the rendering client.
        self.host = self.client = None
        self.watches = {}
        self._channel_factory = channel_factory
        self.children = active['pair_children']
        self._closed = False
        self._spectator_cache = self._spectator_client = None
        self._spectator_requested = False
        self._spectator_open_error = None
        try:
            for role in ('host', 'client'):
                self.watches[role] = watch_factory(self.children[role])
            self.host = channel_factory(self.pid)
            self._check()
        except Exception:
            self.close()
            raise

    def _check(self):
        if self._closed:
            raise RuntimeError('Local server control is closed')
        if not all(watch.alive() for watch in self.watches.values()):
            raise RuntimeError('An owned local server child has exited')

    def _open_spectator_client(self):
        """Open only the verified owned client mapping, for state after a request."""
        if self.client is not None:
            return True
        opened = None
        try:
            self._check()
            opened = self._channel_factory(self.children['client']['pid'])
            self._check()  # Mapping must not outlive the owned child identity.
        except (OSError, ValueError, RuntimeError) as error:
            if opened is not None:
                opened.close()
            self._spectator_open_error = str(error)
            return False
        self.client = opened
        self._spectator_open_error = None
        return True

    def _spectator_state(self, host_state):
        # Ordinary polls never open a mapping. After an explicit spectator
        # request they may retry its state mapping, never another command.
        if self._spectator_requested and self.client is None:
            self._open_spectator_client()
        client = self.client
        if client is not self._spectator_client:
            self._spectator_client, self._spectator_cache = client, None

        def unavailable(stage, *, stale=False, error=None):
            result = {'state_available': False, 'state_source': 'owned_client', 'stale': stale,
                      'active': None, 'applied': False, 'own_view_confirmed': None,
                      'stage': stage, 'matched_target': None,
                      'last_known': copy.deepcopy(self._spectator_cache)}
            if error is not None:
                result['state_error'] = str(error)
            return result

        if client is None:
            return unavailable('client_channel_unavailable', error=self._spectator_open_error)
        try:
            client_state = client.state()
        except (OSError, ValueError, RuntimeError) as error:
            stage = 'client_state_busy' if str(error) == 'Game state update is busy' else 'client_state_unavailable'
            return unavailable(stage, stale=True, error=error)
        value = client_state.get('spectator') if isinstance(client_state, dict) else None
        if not isinstance(value, dict) or not value:
            return unavailable('client_spectator_state_unavailable', stale=self._spectator_cache is not None)
        self._spectator_cache = copy.deepcopy(value)
        result = copy.deepcopy(value)
        # Match actual native selection after left/right switching, never the
        # last requested roster index. This is identity mapping, not active proof.
        target = _spectator_player(host_state.get('players'), native_player_id=value.get('native_player_id'))
        matched = None
        if target is not None:
            hero = target.get('hero_index')
            matched = {'selected_index': target['index'], 'native_player_id': target['native_player_id'],
                       'hero_index': hero if type(hero) is int and hero >= 0 else None}
            matched.update(_spectator_roster_identity(host_state, target))
        for key, item in {'state_available': True, 'state_source': 'owned_client', 'stale': False,
                          'matched_target': matched}.items():
            result.setdefault(key, item)  # Never overwrite a field published by native.
        return result

    def state(self):
        with self._lock:
            self._check()
            result = dict(self.host.state())
            result.setdefault('ready', False)
            result['control_authority'] = 'host'
            result['developmental'] = True
            result['playable_verified'] = False
            result['spectator'] = self._spectator_state(result)
            return result

    def send(self, kind, payload=b'', timeout=3):
        with self._lock:
            return self._send(kind, payload, timeout)

    def _send(self, kind, payload, timeout):
        self._check()
        if kind == 'spectate_bot':
            if not isinstance(payload, (bytes, bytearray, memoryview)) or len(payload) != 4:
                raise ValueError('Spectator command requires one uint32 roster index')
            index, = struct.unpack('<I', payload)
            if index >= MAX_UNIQUE_PLAYERS:
                raise ValueError('Spectator roster index is outside the roster limit')
            if index == 0:
                payload = bytes(16)  # Exit does not depend on a current roster lookup.
            else:
                host_state = self.host.state()
                if not isinstance(host_state, dict) or host_state.get('ready') is not True:
                    raise RuntimeError('local_server_spectator_session_not_ready')
                target = _spectator_player(host_state.get('players') if isinstance(host_state, dict) else None,
                                           roster_index=index)
                if target is None:
                    raise RuntimeError('local_server_spectator_target_unavailable_or_ambiguous')
                payload = struct.pack('<2IiI', 1, index, target['native_player_id'], 0)
        role = 'client' if kind in UI_COMMANDS else 'host'
        if role == 'client' and self.client is None:
            try:
                self.client = self._channel_factory(self.children['client']['pid'])
            except FileNotFoundError:
                return {'sequence': 0, 'status': 'unavailable', 'code': 3, 'acknowledged': False,
                        'message': 'local_server_client_ui_not_ready', 'target_role': role,
                        'target_pid': self.children[role]['pid']}
        channel = self.client if role == 'client' else self.host
        self._check()  # A lazy mapping may not outlive the identity it was opened for.
        result = channel.send(kind, payload, timeout)
        if kind == 'spectate_bot':
            self._spectator_requested = True
            self._open_spectator_client()  # State only; never send this command to client.
        return {**result, 'target_role': role, 'target_pid': self.children[role]['pid']}

    def close(self):
        with self._lock:
            self._close()

    def _close(self):
        self._closed = True
        for channel in (self.client, self.host):
            if channel:
                channel.close()
        for watch in self.watches.values():
            watch.close()
        self.host = self.client = None
        self.watches.clear()


class LocalServerBackend:
    def __init__(self, root=ROOT):
        self.root = Path(root).resolve()
        self.controller_root = controller_directory(self.root, BACKEND)
        self.module_sha256 = None

    def check_config(self, config):
        return check_candidate_config(config, root=self.root)

    def prepare_launch(self, config, tag):
        from local_server_module import verified_build
        check_candidate_config(config, root=self.root)
        manifest = verified_build()
        require_training_build(manifest, config)
        require_selection_build(manifest, config)
        self.module_sha256 = manifest['module_sha256']
        session = pair_directory(self.root / '.local/local-server' / ('pair-' + tag), self.root)
        config_path = self.controller_root / 'sessions' / (tag + '.json')
        atomic_json(config_path, config)
        command = [sys.executable, '-X', 'utf8', str(self.root / 'tools/run_local_server.py'),
                   '--interactive', '--original-host-mode', '--seconds', '86400', '--controller-settings', str(config_path),
                   '--session-dir', str(session)]
        if config.get('human_bots', {}).get('bot_roster') is not None:
            command.append('--fixed-bot-roster-required')
        return session, command

    def pair_record(self, session, process):
        record = read_json(Path(session) / 'session.active.json')
        if not record:
            return None
        return validate_pair_record(record, session, process, module_sha256=self.module_sha256, root=self.root)

    def active(self, session, process):
        pair = self.pair_record(session, process)
        if not pair:
            return None
        children = {}
        for role in ('host', 'client'):
            child = read_json(Path(session) / (role + '-result.active.json'))
            if not child:
                return None
            children[role] = validate_child_record(child, role, pair, root=self.root)
        if children['host']['pid'] == children['client']['pid']:
            raise RuntimeError('Local server pair must have two distinct owned children')
        return {**children['host'], 'pair_children': children, 'pair_session': pair,
                'client_pid': children['client']['pid']}

    def open_channel(self, active):
        return PairChannel(active)

    def request_stop(self, session, process):
        pair = self.pair_record(session, process)
        if not pair:
            return False  # The launching supervisor has not published its identity yet.
        atomic_json(Path(session) / 'stop-request.json',
                    {'reason': 'local_webui_stop', 'proof_id': pair['proof_id'],
                     'runner_pid': process.pid, 'directory': str(Path(session).resolve())})
        return True
