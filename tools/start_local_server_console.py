"""Open the local-server human-bot console with its own persistent preferences.

First use seeds a NEW schema3 store from saved original preferences. Later
launches never reset it. --check is read-only and starts no HTTP/game process.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
import uuid

from human_bot_integration import HumanSettings
from local_environment import project_environment
from training_controller_store import ROOT, CANDIDATE, ORIGINAL, settings_path

PORT = 8792
SETTINGS_RELATIVE = '.local/controller/local-server-candidate/humanlike-bots.json'


def saved_settings(root=ROOT):
    """Resolve this product store through the existing independent-path guard."""
    return settings_path(root, CANDIDATE, Path(root) / SETTINGS_RELATIVE)


def configuration_plan(root=ROOT):
    """Validate the current/new configuration entirely in memory."""
    root = Path(root).resolve()
    target = saved_settings(root)
    original = settings_path(root, ORIGINAL)
    if target.exists():
        raw = json.loads(target.read_text(encoding='utf-8'))
        if not isinstance(raw, dict) or type(raw.get('schema_version')) is not int or raw['schema_version'] != 3:
            raise ValueError('独立拟人人机设置文件已存在但不是schema3；为保留现有数据，本入口不会覆盖它。')
        value = HumanSettings(target).read()
        source = 'existing_independent_store'
        source_sha256 = hashlib.sha256(target.read_bytes()).hexdigest()
    else:
        # HumanSettings reads/migrates without saving. Use the real original
        # preference store, never a prior developer session or test fixture.
        source_store = HumanSettings(original)
        old = source_store.read()
        value = source_store.propose({
            'mode': 'deathmatch', 'map_id': 'ascent', 'total_players': 12,
            'human_bots': {'enabled': True, 'global_overrides': {'hearing.footstep_enabled': 1}},
        }, old['revision'])
        source = 'saved_original_preferences' if original.exists() else 'validated_project_defaults'
        source_sha256 = hashlib.sha256(original.read_bytes()).hexdigest() if original.exists() else None
        if value['infinite_ammo'] != old['infinite_ammo']:
            raise RuntimeError('Independent initialization changed the saved infinite-ammo preference')
    return {
        'backend': CANDIDATE, 'port': PORT, 'url': f'http://127.0.0.1:{PORT}',
        'settings_file': str(target), 'settings_source': source,
        'source_sha256': source_sha256, 'would_create_settings': not target.exists(),
        'configuration': value,
    }


def _publish_new(path, value):
    """Publish complete bytes atomically, with no replacement of an old file."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + '.' + uuid.uuid4().hex + '.seed')
    try:
        with temporary.open('x', encoding='utf-8', newline='\n') as stream:
            json.dump(value, stream, ensure_ascii=False, indent=2, allow_nan=False)
            stream.flush()
            os.fsync(stream.fileno())
        try:
            os.link(temporary, path)
            return True
        except FileExistsError:
            return False
    finally:
        temporary.unlink(missing_ok=True)


def ensure_settings(root=ROOT):
    plan = configuration_plan(root)
    target = Path(plan['settings_file'])
    if plan['would_create_settings']:
        _publish_new(target, plan['configuration'])
    # Re-read and validate the actual winner if two explicit starts raced.
    return Path(configuration_plan(root)['settings_file'])


def server_arguments(path, *, open_browser=False, start_game=False):
    result = ['--backend', CANDIDATE, '--port', str(PORT), '--settings-file', str(path)]
    if open_browser:
        result.append('--open')
    if start_game:
        result.append('--start')
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--open', action='store_true', help='Open the local console in the browser')
    parser.add_argument('--start', action='store_true', help='Also request the owned local host/client pair')
    parser.add_argument('--check', action='store_true', help='Print a read-only configuration plan; start nothing')
    args = parser.parse_args(argv)
    if args.check:
        plan = configuration_plan(ROOT)
        config = plan.pop('configuration')
        plan.update(read_only=True, game_started=False, service_started=False,
                    schema_version=config['schema_version'], revision=config['revision'],
                    mode=config['mode'], map_id=config['map_id'], total_players=config['total_players'],
                    infinite_ammo=config['infinite_ammo'], human_bots_enabled=config['human_bots']['enabled'],
                    parameter_effectiveness_claimed=False)
        print(json.dumps(plan, ensure_ascii=False, indent=2))
        return 0
    os.environ.update(project_environment(ROOT))
    path = ensure_settings(ROOT)
    import training_server
    arguments = [str(Path(ROOT) / 'tools/training_server.py'),
                 *server_arguments(path, open_browser=args.open, start_game=args.start)]
    previous = sys.argv
    try:
        sys.argv = arguments
        result = training_server.main()
        return 0 if result is None else result
    finally:
        sys.argv = previous


if __name__ == '__main__':
    raise SystemExit(main())
