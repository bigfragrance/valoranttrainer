"""Launch one owned stage9 session and run offline feature checks autonomously."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import subprocess
import sys
import time

from local_environment import project_environment
from training_settings_v2 import Settings
from training_runtime_client import OwnedSession, ROOT
from verify_weapon_switches_v2 import verify as verify_weapons


def read_checkpoint(path):
    for attempt in range(20):
        try:
            return json.loads(path.read_text(encoding='utf-8'))
        except (FileNotFoundError, PermissionError, json.JSONDecodeError):
            if attempt == 19:
                raise
            time.sleep(.01)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', action='store_true')
    parser.add_argument('--shop', action='store_true')
    parser.add_argument('--combat', action='store_true')
    parser.add_argument('--feedback', action='store_true')
    parser.add_argument('--topbar-streak', action='store_true')
    parser.add_argument('--original-streak-hud', action='store_true')
    parser.add_argument('--ui-input', action='store_true')
    parser.add_argument('--audio', action='store_true')
    parser.add_argument('--skins', action='store_true')
    parser.add_argument('--bots', action='store_true')
    parser.add_argument('--firearms', action='store_true')
    parser.add_argument('--melee', action='store_true')
    parser.add_argument('--respawn', action='store_true')
    parser.add_argument('--abilities', action='store_true')
    parser.add_argument('--ability-keys', default='cqex', choices=['cqex', 'cqe', 'x'])
    parser.add_argument('--post-ability-respawn', action='store_true')
    parser.add_argument('--time-limit', action='store_true')
    parser.add_argument('--kill-limit', action='store_true')
    parser.add_argument('--limited-ammo', action='store_true')
    parser.add_argument('--gameplay', action='store_true')
    parser.add_argument('--player-roles', action='store_true')
    parser.add_argument('--npc-lifecycle', action='store_true')
    parser.add_argument('--posture', action='store_true')
    parser.add_argument('--skip-weapons', action='store_true')
    parser.add_argument('--map', default='duel1')
    parser.add_argument('--deathmatch', action='store_true')
    parser.add_argument('--players', type=int, default=4)
    parser.add_argument('--seed', type=int)
    args = parser.parse_args()
    if (args.time_limit or args.kill_limit) and (not args.deathmatch or args.map == 'duel1'):
        parser.error('Match-limit verification requires a real 5v5 map in deathmatch mode')
    if args.build:
        from build_native_module import build
        build()
    tag = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    directory = ROOT / '.local/feature-verification' / tag
    directory.mkdir(parents=True)
    session_directory = ROOT / '.local/training-sessions' / ('verify-' + tag)
    config = Settings(ROOT / '.local/controller/config.json').read()
    config.update(mode='deathmatch' if args.deathmatch else 'practice', map_id=args.map,
                  invincible=True, paused=True, total_players=args.players)
    config['practice']['return_fire'] = False
    if args.seed is not None:
        if not 1 <= args.seed <= 0xffffffff:
            parser.error('Seed must fit the original nonzero32-bit configuration')
        config['seed'] = args.seed
    if args.limited_ammo:
        config['infinite_ammo'] = False
    if args.kill_limit:
        config.update(paused=False, invincible=False)
    config_path = directory / 'config.json'
    config_path.write_text(json.dumps(config, ensure_ascii=False, indent=2), encoding='utf-8')
    command = [sys.executable, '-X', 'utf8', str(ROOT / 'tools/run_duel1_training.py'), '--prebuilt', '--seconds', '650' if args.time_limit or args.kill_limit else '240' if args.shop else '180',
               '--controller-settings', str(config_path), '--session-dir', str(session_directory), '--test-input']
    report = {'directory': str(directory), 'passed': False}
    session = None
    with (directory / 'runner.log').open('w', encoding='utf-8') as log:
        process = subprocess.Popen(command, cwd=ROOT, stdout=log, stderr=log,
                                   creationflags=subprocess.CREATE_NO_WINDOW, env=project_environment(ROOT))
        try:
            deadline = time.monotonic() + 45
            active_path = session_directory / 'result.active.json'
            while time.monotonic() < deadline and process.poll() is None:
                if active_path.is_file():
                    active = read_checkpoint(active_path)
                    marker = ROOT / '.local/native-module/bridge-state.json'
                    state = read_checkpoint(marker) if marker.is_file() else {}
                    if state.get('pid') == active['pid'] and state.get('error'):
                        raise RuntimeError(state['stage'])
                    if state.get('pid') == active['pid'] and state.get('stage') == 'original_training_v2_active':
                        session = OwnedSession(active_path)
                        session.wait(lambda s: s.get('ready'), timeout=25)
                        break
                time.sleep(.1)
            if session is None:
                raise RuntimeError('The owned V2 test client did not reach gameplay')
            report['pid'] = session.active['pid']
            print(json.dumps({'checkpoint': 'original_gameplay_ready', 'pid': report['pid']}), flush=True)
            report['initial_state'] = session.channel.state()
            if args.gameplay:
                from verify_deathmatch_gameplay_v2 import verify as verify_gameplay
                report['gameplay'] = verify_gameplay(active_path)
            if not args.skip_weapons or args.shop:
                report['weapons'] = verify_weapons(active_path, shop=args.shop, equip=not args.skip_weapons)
            if args.combat:
                from verify_combat_v2 import verify as verify_combat
                report['combat'] = verify_combat(active_path, config)
            if args.feedback:
                from verify_kill_feedback_v2 import verify as verify_feedback
                report['feedback'] = verify_feedback(active_path, config)
            if args.skins:
                from verify_skins_v2 import verify as verify_skins
                report['skins'] = verify_skins(active_path)
            if args.bots:
                from verify_bots_v2 import verify as verify_bots
                report['bots'] = verify_bots(active_path, config)
            if args.original_streak_hud:
                from verify_original_streak_v3 import verify as verify_original_streak
                report['original_streak_hud'] = verify_original_streak(active_path, config)
            if args.ui_input:
                from verify_ui_input_v3 import verify as verify_ui_input
                report['ui_input'] = verify_ui_input(active_path, config)
            if args.audio:
                from verify_audio_live_v3 import verify as verify_audio
                report['audio'] = verify_audio(active_path, config)
            if args.npc_lifecycle:
                from verify_npc_lifecycle_v2 import verify as verify_npc_lifecycle
                report['npc_lifecycle'] = verify_npc_lifecycle(active_path, config, ui_respawn=args.ui_input)
                if args.audio:
                    from verify_audio_snapshot_v3 import audit as audit_audio
                    report['audio_after_lifecycle'] = audit_audio(ROOT / '.local/native-module', session.active['pid'])
            if args.topbar_streak:
                from verify_topbar_streak_v2 import verify as verify_topbar_streak
                report['topbar_streak'] = verify_topbar_streak(active_path, config)
            if args.posture:
                from verify_local_posture_v2 import verify as verify_posture
                report['posture'] = verify_posture(active_path, config)
            if args.player_roles:
                from verify_player_roles_v2 import verify as verify_player_roles
                report['player_roles'] = verify_player_roles(active_path, config)
            if args.firearms:
                from verify_firearms_v2 import verify as verify_firearms
                report['firearms'] = verify_firearms(active_path)
            if args.melee:
                from verify_melee_v2 import verify as verify_melee
                report['melee'] = verify_melee(active_path)
            if args.respawn:
                from verify_player_respawn_v2 import verify as verify_respawn
                report['respawn'] = verify_respawn(active_path, config)
                if args.player_roles and report['respawn']['passed']:
                    report['player_roles_after_respawn'] = verify_player_roles(active_path, config)
            if args.abilities:
                from verify_abilities_v2 import verify as verify_abilities
                report['abilities'] = verify_abilities(active_path, args.ability_keys)
            if args.post_ability_respawn:
                from verify_player_respawn_v2 import verify as verify_respawn
                report['ability_respawn'] = verify_respawn(active_path, config)
                if report['ability_respawn']['passed']:
                    from verify_abilities_v2 import verify as verify_abilities
                    report['abilities_after_respawn'] = verify_abilities(active_path, 'x')
            if args.time_limit or args.kill_limit:
                from copy import deepcopy
                from training_control import pack_configuration
                match_config = deepcopy(config)
                match_config.update(paused=bool(args.time_limit), invincible=bool(args.time_limit))
                match_config['revision'] = max(match_config['revision'], session.channel.state().get('applied_revision', 0)) + 1
                session.command('configure', pack_configuration(match_config))
                from verify_match_end_v2 import verify as verify_match_end
                report['match'] = verify_match_end(active_path, directory / 'match-progress.json', time_limit=args.time_limit)
            report['passed'] = all(report.get(k, {'passed': True}).get('passed', False) for k in ('weapons', 'combat', 'feedback', 'skins', 'bots', 'original_streak_hud', 'ui_input', 'audio', 'audio_after_lifecycle', 'npc_lifecycle', 'topbar_streak', 'posture', 'player_roles', 'player_roles_after_respawn', 'firearms', 'melee', 'respawn', 'abilities', 'ability_respawn', 'abilities_after_respawn', 'gameplay', 'match'))
        except Exception as error:
            report['error'] = str(error)
            print(json.dumps({'error': str(error)}), flush=True)
        finally:
            if session:
                session.stop()
                session.close()
            elif active_path.is_file():
                active = read_checkpoint(active_path)
                run = Path(active['run_dir']).resolve()
                if ROOT / '.local/native-baseline' in run.parents:
                    (run / 'stop-request.json').write_text(json.dumps({'reason': 'feature_verifier_failure'}), encoding='utf-8')
            try:
                process.wait(timeout=20)
            except subprocess.TimeoutExpired:
                report['runner_cleanup_pending'] = True
            for name in ('build', 'all-weapons', 'all-skins', 'training-combat-runtime', 'original-melee-combat',
                         'deathmatch-rules', 'original-buy-menu', 'training-bot-controller', 'kill-feedback-view',
                         'original-armor', 'skin-effect-controls', 'training-champion-state', 'training-maps',
                         'deathmatch-pickups', 'original-recovery', 'original-effects', 'original-timed-streak', 'original-streak-hud', 'original-streak-effect-authority', 'original-audio-listeners', 'original-ui-input-mode', 'training-performance',
                         'original-jett', 'original-jett-world', 'original-jett-sync', 'original-jett-priming', 'original-dagger-combat', 'training-player-roles', 'original-npc-locality', 'original-deathmatch-topbar', 'training-local-posture-probe', 'training-landing-shake-probe', 'original-heroes'):
                path = ROOT / '.local/native-module' / (name + '.json')
                if path.is_file():
                    (directory / path.name).write_bytes(path.read_bytes())
    (directory / 'verification.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    (ROOT / '.local/feature-verification/latest.json').write_text(json.dumps({'directory': str(directory), 'passed': report['passed']}), encoding='utf-8')
    print(json.dumps({'passed': report['passed'], 'report': str(directory / 'verification.json')}), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
