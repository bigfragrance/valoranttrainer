"""Verify each authored 5v5 map with four original offline FFA participants."""
from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
from datetime import datetime, timezone

from local_environment import project_environment

ROOT = Path(__file__).resolve().parents[1]


def main():
    catalog_path = ROOT / 'reports/training-maps.json'
    catalog = json.loads(catalog_path.read_text(encoding='utf-8'))
    directory = ROOT / '.local/map-verification'
    directory.mkdir(parents=True, exist_ok=True)
    results = []
    for item in catalog['maps']:
        if not item['supports_ffa']:
            continue
        with (directory / (item['id'] + '.log')).open('w', encoding='utf-8') as log:
            process = subprocess.run([sys.executable, '-X', 'utf8', str(ROOT / 'tools/develop_verify_v2.py'),
                                      '--deathmatch', '--map', item['id'], '--players', '4', '--skip-weapons', '--bots'],
                                     cwd=ROOT, stdout=log, stderr=log, env=project_environment(ROOT), timeout=110)
        latest = json.loads((ROOT / '.local/feature-verification/latest.json').read_text(encoding='utf-8'))
        evidence = Path(latest['directory'])
        report = json.loads((evidence / 'verification.json').read_text(encoding='utf-8'))
        state = report.get('initial_state', {})
        layers = json.loads((evidence / 'training-maps.json').read_text(encoding='utf-8'))
        rules = json.loads((evidence / 'deathmatch-rules.json').read_text(encoding='utf-8'))
        passed = process.returncode == 0 and report['passed'] and state.get('map_id') == item['id'] and layers['ready'] and layers['missing'] == 0 and rules['original_ffa_confirmed']
        validation = item['validation']
        validation.update(runtime='passed' if passed else 'failed', fully_loadable=bool(passed),
                          runtime_evidence=str(evidence.relative_to(ROOT)).replace('\\', '/'),
                          runtime_test_utc=datetime.now(timezone.utc).isoformat(),
                          runtime_scope='required original common/FFA layers, four real heroes, original heavy armor, spawn ground/capsule checks and eight seconds of native bot movement')
        if passed:
            bots = report['bots']['styles'][0]['diagnostics']
            validation['navigation'] = 'native_path_and_movement_verified' if bots['original_path_succeeded'] else 'validated_capsule_path_and_movement_verified'
            validation['spawn_collision'] = 'four_original_hero_capsules_verified'
        else:
            validation['runtime_error'] = report.get('error') or report.get('bots', {}).get('error')
        temporary = catalog_path.with_suffix('.json.tmp')
        temporary.write_text(json.dumps(catalog, ensure_ascii=False, indent=2), encoding='utf-8')
        temporary.replace(catalog_path)
        row = {'map_id': item['id'], 'passed': passed, 'evidence': str(evidence), 'error': validation.get('runtime_error')}
        results.append(row)
        (directory / 'results.json').write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding='utf-8')
        print(json.dumps(row, ensure_ascii=False), flush=True)
    return 0 if results and all(r['passed'] for r in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
