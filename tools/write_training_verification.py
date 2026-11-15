"""Collect immutable native/UI evidence into the current development report."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return json.loads((ROOT / path).read_text(encoding='utf-8'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--abilities', required=True, help='Feature verification directory name')
    parser.add_argument('--ability-life', required=True, help='Actual X, local death/rebind and X again verification directory')
    args = parser.parse_args()
    feature = '.local/feature-verification/'
    cases = {
        'core': feature + '20261101T224516.824405Z/verification.json',
        'shop': feature + '20261101T224828.270802Z/verification.json',
        'practice': feature + '20261101T234539.430292Z/verification.json',
        'melee': feature + '20261101T222029.959063Z/verification.json',
        'kill_limit': feature + '20261101T233155.738653Z/verification.json',
        'time_limit': feature + '20261101T234853.082757Z/verification.json',
        'finite_ammo': feature + '20261102T000148.226137Z/verification.json',
        'abilities': feature + args.abilities + '/verification.json',
        'ability_life': feature + args.ability_life + '/verification.json',
        'latest_skins_firearms': feature + '20261102T010128.295777Z/verification.json',
        'latest_combat_respawn': feature + '20261102T010539.078745Z/verification.json',
        'webui': '.local/webui-verification/verification.json',
        'http_native': '.local/webui-verification/live-native.json',
        'scene_switches': '.local/webui-verification/runtime-switches.json',
        'launch_cancel': '.local/webui-verification/launch-cancel.json',
    }
    documents = {key: read(path) for key, path in cases.items()}
    proof = {}
    for key, path in cases.items():
        data = (ROOT / path).read_bytes()
        proof[key] = {'path': path, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
                      'whole_run_passed': documents[key].get('passed')}
    core = documents['core']
    maps = [m for m in read('reports/training-maps.json')['maps'] if m['kind'] == 'standard_5v5']
    verified = {
        'all_20_native_firearms': documents['latest_skins_firearms']['firearms']['passed'] and len(documents['latest_skins_firearms']['firearms']['weapons']) == 20,
        'original_shop_20_purchases': documents['shop']['weapons']['passed'] and len(documents['shop']['weapons']['purchases']) == 20,
        'all_skin_catalog_combinations': bool(core['skins']['catalog']['passed']),
        'representative_skin_runtime': documents['latest_skins_firearms']['skins']['passed'] and len(documents['latest_skins_firearms']['skins']['cases']) == 11,
        'native_knife_contacts': documents['melee']['melee']['passed'],
        'six_practice_styles_and_routes': documents['practice']['bots']['passed'],
        'kill_feedback_timeout': documents['latest_combat_respawn']['combat']['passed'],
        'local_death_respawn': documents['latest_combat_respawn']['respawn']['passed'],
        'full_32_player_kill_limit': documents['kill_limit']['match']['passed'],
        'real_540_second_time_limit': documents['time_limit']['match']['passed'],
        'finite_ammo_and_spawn_protection': documents['finite_ammo']['gameplay']['passed'],
        'all_13_standard_maps': len(maps) == 13 and all(m.get('validation', {}).get('runtime') == 'passed' for m in maps),
        'jett_abilities': documents['abilities']['abilities']['passed'] and documents['abilities']['abilities'].get('selected') == 'cqex',
        'daggers_death_rebind_and_fire_again': (documents['ability_life']['abilities']['passed']
                                              and documents['ability_life']['ability_respawn']['passed']
                                              and documents['ability_life']['abilities_after_respawn']['passed']),
        'webui_layout_and_script': documents['webui']['passed'],
        'http_to_native_controls': documents['http_native']['passed'],
        'http_mode_map_population_restart': documents['scene_switches']['passed'],
        'cancel_loading_without_delayed_child': documents['launch_cancel']['passed'],
    }
    test_results = {}
    for key, path in [('python_tests', '.local/dev-test-verification.log'), ('node_tests', '.local/webui-unit-verification.log')]:
        payload = (ROOT / path).read_bytes()
        output = payload.decode('utf-16' if payload[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig')
        if key == 'python_tests':
            count = re.search(r'Ran (\d+) tests', output)
            passed = bool(count and re.search(r'^OK\s*$', output, re.M))
        else:
            count = re.search(r'# tests (\d+)', output)
            passed = bool(count and re.search(r'# fail 0\b', output))
        test_results[key] = {'count': int(count.group(1)) if count else 0, 'passed': passed,
                             'path': path, 'sha256': hashlib.sha256(payload).hexdigest()}
        verified[key] = passed
    skins = core['skins']['catalog']
    match = documents['kill_limit']['match']['rules']
    report = {
        'schema_version': 1, 'generated_utc': datetime.now(timezone.utc).isoformat(),
        'passed': all(verified.values()), 'verified': verified, 'evidence': proof,
        'tests': test_results,
        'catalog': {key: skins[key] for key in ('verified_skins', 'unique_levels', 'unique_chromas', 'verified_default_choices', 'verified_combinations')},
        'full_match': {key: match[key] for key in ('participants', 'kills', 'respawns', 'local_deaths', 'local_respawns', 'winner', 'leading_kills', 'end_reason')},
        'map_verification': [{'id': m['id'], 'validation': m['validation']} for m in maps],
        'build': read('.local/native-module/build.json'),
        'operation': {'entry': '启动训练控制台.cmd', 'url': 'http://127.0.0.1:8790',
                      'packaged_this_run': False, 'resource_mode': 'installed-readonly-v1',
                      'project_artifacts_drive': 'F:', 'account_login': False},
        'limits': [
            'Catalog identity validation covers every listed choice; runtime loading/ADS/effects covers 11 representative cases, not every skin pixel.',
            'Each standard map passed original layers, four-player spawning/collision and short navigation; every corner, door and map mechanism was not traversed.',
            'This run used expiring owned stage9 native test input and isolated headless browser verification; physical desktop input and game pixel review stayed stopped.',
            'The installed client owns assets, presentation, movement and weapon/ability behavior; stripped authority bookkeeping is supplied locally.',
            'Older evidence runs may contain failed unrelated subchecks; only explicitly selected passing subchecks are used above.',
            'Removal of the obsolete development resource backup was blocked by automatic approval policy; the backup remains.',
        ],
    }
    output = ROOT / 'reports/training-v2-verification.json'
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'passed': report['passed'], 'report': str(output), 'verified': verified}, ensure_ascii=False))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
