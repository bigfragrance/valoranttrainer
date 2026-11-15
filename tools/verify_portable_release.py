"""Bind native smoke checks and zero resource-read errors to one portable manifest."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re


def assess_game_log(text: str) -> dict:
    patterns = {'signature_errors': r'Signature error detected',
                'failed_package_reads': r'StartBundleIoRequests: FailedRead:',
                'corruption_dialogs': r'Corrupt Game Data Detected!',
                'repair_mode_entries': r'Loading game in repair mode',
                'critical_error_logs': r'=== Critical error:'}
    counts = {key: len(re.findall(pattern, text, re.I)) for key, pattern in patterns.items()}
    return {'passed': not any(counts.values()), **counts}


def digest(path: Path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def verify(package: Path, reports: dict[str, Path]) -> dict:
    package = package.resolve()
    config = json.loads((package / 'project.json').read_text(encoding='utf-8'))
    version_path = package / 'playtest-version.json'
    version = json.loads(version_path.read_text(encoding='utf-8'))
    tests = {}
    for label, report_path in reports.items():
        report_path = report_path.resolve(strict=True)
        if not report_path.is_relative_to(package / '.local'):
            raise RuntimeError('Native evidence must belong to this portable test directory')
        document = json.loads(report_path.read_text(encoding='utf-8'))
        run = document.get('run') or json.loads(report_path.with_name('run.json').read_text(encoding='utf-8'))
        run_dir = Path(run['report_path']).resolve()
        if run_dir.name != 'report.json' or not run_dir.is_relative_to(package / '.local/native-baseline'):
            raise RuntimeError('Native launch evidence is outside the package')
        log = run_dir.with_name('runtime.log')
        assessment = assess_game_log(log.read_text(encoding='utf-8', errors='replace'))
        assertions = document.get('controls', {}).get('assertions') or document.get('assertions', {})
        same_image = (run.get('preflight', {}).get('process_scope') == 'bundled_client'
                      and run.get('preflight', {}).get('game_sha256') == config['expected_snapshot_sha256']
                      and run.get('image_unchanged') is True)
        same_module = run.get('training_module', {}).get('sha256') == version['module_sha256']
        lifecycle = run.get('original_game_instance_lifecycle', {})
        startup = lifecycle.get('final_state') or json.loads(Path(lifecycle['report_path']).read_text(encoding='utf-8'))
        instance_lifecycle = (lifecycle.get('module_sha256') == version.get('startup_module_sha256')
                              and startup.get('instances_configured') == 1 and not startup.get('error')
                              and startup.get('installed_before_cdo') == 1
                              and startup.get('source_valid_and_shared') == 1
                              and startup.get('original_constructor_preserved') == 1
                              and startup.get('other_slots_preserved') == 1
                              and startup.get('secondary_vtable_preserved') == 1
                              and startup.get('image_pages_written') is False
                              and startup.get('checks_disabled') is False)
        passed = (document.get('passed') is True and bool(assertions) and all(value is True for value in assertions.values())
                  and assessment['passed'] and same_image and same_module and instance_lifecycle
                  and run.get('owned_process_exited') is True)
        tests[label] = {'passed': passed, 'assertions': assertions, 'game_log': assessment,
                        'same_pinned_image': same_image, 'same_pinned_module': same_module,
                        'private_instance_lifecycle_verified': instance_lifecycle,
                        'original_image_page_valid_and_shared': startup.get('source_valid_and_shared') == 1,
                        'evidence_sha256': digest(report_path), 'run_report_sha256': digest(run_dir),
                        'runtime_log_sha256': digest(log)}
    result = {'schema_version': 1, 'utc': datetime.now(timezone.utc).isoformat(),
              'passed': set(tests) == {'controls', 'training', 'jett_cq'} and all(t['passed'] for t in tests.values()),
              'resources_manifest_sha256': config['resources_manifest_sha256'],
              'module_sha256': version['module_sha256'], 'game_sha256': config['expected_snapshot_sha256'],
              'startup_module_sha256': version.get('startup_module_sha256'),
              'tests': tests, 'other_hardware_tested': False, 'visual_pixels_verified': False,
              'all_possible_inputs_covered': False, 'full_skill_validation_complete': False}
    (package / 'reports/portable-validation.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    version['standalone_resource_validation_complete'] = result['passed']
    version['validated_resource_manifest_sha256'] = result['resources_manifest_sha256']
    version_path.write_text(json.dumps(version, indent=2), encoding='utf-8')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--controls', type=Path, required=True)
    parser.add_argument('--training', type=Path, required=True)
    parser.add_argument('--jett-cq', type=Path, required=True)
    args = parser.parse_args()
    result = verify(args.package, {'controls': args.controls, 'training': args.training, 'jett_cq': args.jett_cq})
    print(json.dumps({'passed': result['passed'], 'tests': {key: value['passed'] for key, value in result['tests'].items()}}))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
