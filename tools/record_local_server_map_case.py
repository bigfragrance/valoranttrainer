"""Record a completed native pair's map-loading evidence, bound to its build.

This records original map/actor loading and orderly shutdown only. It never
certifies movement, collision, combat, audio, or complete playable acceptance.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re

from local_server_controller import atomic_json
import local_server_maps as maps
import local_server_module as module
from run_local_server import analyze_gameplay_samples
from local_server_sample_evidence import iter_full_samples

ROOT = Path(__file__).resolve().parents[1]


def load(path):
    return json.loads(Path(path).read_text(encoding='utf-8'))


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def checks_for_pair(report, host_environment, client_environment, *, package, map_id, session=None):
    if report.get('sample_evidence') is not None and session is None:
        raise ValueError('Full archived map evidence requires the actual pair directory')
    samples = iter_full_samples(report, session)
    matching, longest, span, previous = [], 0., 0., None
    for sample in samples:
        valid = all(side.get('observed_world_identity', {}).get('package_name') == package
                    and side.get('observed_world_identity', {}).get('world') == side.get('current_world')
                    and side.get('session_runtime', {}).get('ready') is True
                    for side in (sample.get('host', {}), sample.get('client', {})))
        if not valid:
            previous, span = None, 0.
            continue
        matching.append(sample)
        elapsed = sample['elapsed']
        span = span + elapsed - previous if previous is not None and 0 < elapsed - previous <= 1 else 0.
        previous, longest = elapsed, max(longest, span)
    gates = analyze_gameplay_samples(matching)
    return {
        'selected_map_package_matches_original_catalog': report.get('map_package') == package,
        'native_world_identity_seen_on_both_sides': bool(matching),
        'stable_native_graph_for_at_least_five_seconds': longest >= 5,
        'actual_join_and_owned_shooter_hero': gates.get('shooter_hero_round_trip_proven') is True,
        'host_native_environment_active': host_environment.get('initialized') is True
        and host_environment.get('host_ready') is True and host_environment.get('native_active') is True,
        'client_native_environment_active': client_environment.get('initialized') is True
        and client_environment.get('passive_client') is True
        and client_environment.get('native_active') is True,
        'selected_environment_map_matches': host_environment.get('map_id') == map_id
        and client_environment.get('map_id') == map_id,
        'client_streaming_and_original_readbacks': client_environment.get('client_layers_ready') is True
        and client_environment.get('client_local_readbacks_match') is True,
        'environment_reports_no_error': host_environment.get('error') == 0
        and client_environment.get('error') == 0,
        'complete_owned_pair_shutdown': report.get('completion', {}).get('passed') is True,
        'no_reported_session_error': report.get('error') is None,
        'no_critical_errors': report.get('host_critical_errors') == 0
        and report.get('client_critical_errors') == 0,
        'protected_builds_unchanged': report.get('protected_builds_unchanged') is True,
        'candidate_build_unchanged': report.get('candidate_build_unchanged') is True,
    }


def record(session, *, write=False, root=ROOT):
    root = Path(root).resolve()
    session = Path(session).resolve(strict=True)
    parent = (root / '.local/local-server').resolve()
    if session.parent != parent or not re.fullmatch(r'pair-\d{8}T\d{6}\.\d{6}Z', session.name):
        raise ValueError('Use a dated native pair directory directly under this project')
    report_path = session / 'verification.json'
    report = load(report_path)
    if (report.get('network_flavor') != 'playable'
            or report.get('kind') not in ('native-shooter-candidate', 'native-shooter-candidate-interactive')
            or Path(report.get('directory', '')).resolve() != session
            or not re.fullmatch('[0-9a-f]{32}', report.get('proof_id', ''))):
        raise ValueError('Native pair identity is incomplete')
    build = module.verified_build()
    if report.get('module_sha256') != build['module_sha256']:
        raise ValueError('This native pair belongs to a different compiled candidate')
    map_id, mode = report.get('map_id'), report.get('training_mode')
    selected = maps.selected_map(map_id, mode, root=root)
    paths = {role: session / role / 'environment-state.json' for role in ('host', 'client')}
    checks = checks_for_pair(report, load(paths['host']), load(paths['client']),
                             package=selected['world_path'], map_id=map_id, session=session)
    source_paths = [report_path, *paths.values()]
    if report.get('sample_evidence') is not None:
        source_paths.append(session / report['sample_evidence']['archive']['name'])
    evidence = {
        'schema_version': 1, 'kind': maps.CASE_KIND, 'backend': maps.BACKEND,
        'network_flavor': 'playable', 'module_sha256': build['module_sha256'],
        'map_id': map_id, 'mode': mode, 'map_package': selected['world_path'],
        'session': str(session), 'proof_id': report['proof_id'],
        'at_utc': datetime.now(timezone.utc).isoformat(), 'checks': checks,
        'passed': all(checks.values()), 'playable_verified': False,
        'scope': 'Original selected world, native Join/hero/environment loading and owned shutdown only; collision, combat and full play remain unverified.',
        'artifacts': {str(path.relative_to(root)): sha256(path)
                      for path in source_paths},
    }
    if write:
        destination = parent / 'map-validation' / f'{map_id}-{mode}-{session.name[5:]}.json'
        atomic_json(destination, evidence)
        index_path = root / maps.VALIDATION_INDEX
        index = maps.read_json(index_path)
        if index and (index.get('schema_version') != 1 or index.get('backend') != maps.BACKEND
                      or index.get('source') != maps.VALIDATION_SOURCE):
            raise ValueError('Existing candidate map evidence index has an incompatible identity')
        if not index:
            index = {'schema_version': 1, 'backend': maps.BACKEND,
                     'source': maps.VALIDATION_SOURCE, 'cases': []}
        entry = {'map_id': map_id, 'mode': mode, 'module_sha256': build['module_sha256'],
                 'evidence': str(destination.relative_to(root))}
        index['cases'] = [old for old in index['cases'] if old.get('evidence') != entry['evidence']]
        index['cases'].append(entry)
        atomic_json(index_path, index)
        evidence['recorded_at'] = str(destination)
    return evidence


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session-dir', type=Path, required=True)
    parser.add_argument('--write', action='store_true')
    args = parser.parse_args()
    result = record(args.session_dir, write=args.write)
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
