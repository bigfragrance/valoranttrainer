"""Bounded, lossless evidence for ordinary interactive local-server sessions.

The inline samples are explicitly only the existing native gates' projection.
Full pair diagnostics remain in a rolling 240-record gzip/JSONL archive. This
module performs no game I/O, input, physics or pass/fail decisions.
"""
from __future__ import annotations

from collections import deque
from copy import deepcopy
import gzip
import hashlib
import json
from pathlib import Path

KIND = 'local-server-bounded-pair-history'
VERSION = 1
ARCHIVE_NAME = 'sample-history-v1.jsonl.gz'
LIMIT = 240

# These retain every field consumed by analyze_gameplay_samples and its
# analyze_connection_samples dependency, including the timed remote trajectory.
HOST_GATE_FIELDS = (
    'host_connections', 'host_shooter_world_ready', 'local_services_ready',
    'authority_hero_spawned', 'authority_hero_possessed', 'authority_hero',
    'authority_controller', 'authority_player_state', 'remote_pawn',
    'remote_ai_is_local', 'remote_pawn_is_local', 'server_remote_position',
    'remote_move_submissions', 'server_remote_sample_at_ms',
)
CLIENT_GATE_FIELDS = (
    'connection_state', 'own_local_links_verified',
    'local_controller_is_connection_owner', 'local_controller_role',
    'current_world', 'initial_world', 'pending_netgame',
    'world_authority_game_mode', 'local_player_state', 'local_pawn',
    'local_pawn_role', 'client_remote_count', 'client_remote_position',
    'client_remote_sample_at_ms', 'local_hero_received', 'local_services_ready',
)
LIVENESS_FIELDS = (
    'pid', 'role', 'stage', 'error', 'playable_error', 'failure_stage',
    'playable_stage', 'action_completed', 'pending_travel_failed',
)
FULL_REPORT_FIELDS = (
    'host', 'client', 'last_host', 'last_client', 'host_cleanup',
    'host_result', 'client_result',
)


def _encode(value):
    return json.dumps(value, ensure_ascii=False, separators=(',', ':')).encode('utf-8') + b'\n'


def gate_sample(row):
    """A detached projection; absent values remain absent, never synthesized."""
    result = {'elapsed': row['elapsed']}
    for side, fields in (('host', HOST_GATE_FIELDS), ('client', CLIENT_GATE_FIELDS)):
        state = row.get(side, {})
        result[side] = deepcopy({key: state[key] for key in (*fields, *LIVENESS_FIELDS) if key in state})
    return result


def compact_progress(report):
    """Normal interactive progress, with full diagnostics referenced explicitly."""
    samples = report.get('samples', [])
    result = {key: value for key, value in report.items() if key not in FULL_REPORT_FIELDS}
    result.update(samples=samples[-1:],
                  progress_samples_scope='native_gate_projection_v1',
                  history_samples_retained_for_verification=len(samples),
                  full_state_references={
                      'live_host': 'host/bridge-state.json',
                      'live_client': 'client/bridge-state.json',
                      'terminal_states_and_cleanup': 'verification.json',
                  })
    return result


class SampleHistory:
    """Same original sample count and 240-row window, compressed in memory."""
    def __init__(self, report):
        self.members = deque()
        self.plain_bytes = self.compressed_bytes = 0
        report['sample_evidence'] = {
            'schema_version': VERSION, 'kind': KIND,
            'inline_samples_scope': 'native_gate_projection_v1',
            'retention_limit_samples': LIMIT, 'nominal_interval_seconds': .5,
            'scope': 'Rolling last 240 full sampled pairs; not a whole-session trace.',
            'retained_samples': 0, 'total_samples': 0,
            'archive': {'name': ARCHIVE_NAME, 'format': 'gzip-members-jsonl-v1', 'status': 'pending'},
        }

    def append(self, report, row):
        # Encode before mutating either representation. No partial sample may
        # advance a counter or silently substitute the projection for the row.
        raw = _encode(row)
        packed = gzip.compress(raw, compresslevel=1, mtime=0)
        projected = gate_sample(row)
        if len(self.members) == LIMIT:
            previous, raw_size = self.members.popleft()
            self.compressed_bytes -= len(previous)
            self.plain_bytes -= raw_size
        self.members.append((packed, len(raw)))
        self.compressed_bytes += len(packed)
        self.plain_bytes += len(raw)
        report['samples'].append(projected)
        del report['samples'][:-LIMIT]
        report['total_samples'] += 1
        metadata = report['sample_evidence']
        metadata.update(retained_samples=len(self.members), total_samples=report['total_samples'])

    def write(self, directory, report):
        """Write once after owned cleanup; failure is reported by the caller."""
        directory = Path(directory)
        target = directory / ARCHIVE_NAME
        if target.exists():
            raise FileExistsError('Refusing to replace retained pair evidence: ' + str(target))
        header = {
            'kind': KIND, 'schema_version': VERSION,
            'proof_id': report['proof_id'], 'module_sha256': report['module_sha256'],
            'retained_samples': len(self.members), 'total_samples': report['total_samples'],
        }
        raw_header = _encode(header)
        compressed_header = gzip.compress(raw_header, compresslevel=1, mtime=0)
        digest = hashlib.sha256()
        temporary = target.with_name(target.name + '.tmp')
        with temporary.open('xb') as stream:
            for block in (compressed_header, *(entry[0] for entry in self.members)):
                stream.write(block)
                digest.update(block)
        temporary.replace(target)
        report['sample_evidence']['archive'].update(
            status='complete', sha256=digest.hexdigest(),
            bytes=len(compressed_header) + self.compressed_bytes,
            decompressed_bytes=len(raw_header) + self.plain_bytes)


def iter_full_samples(report, directory):
    """Read legacy raw samples or the explicitly identified full v1 archive.

    Readers must not interpret the v1 inline gate projection as full diagnostic
    evidence. A missing, corrupt, incomplete or differently bound archive raises.
    """
    metadata = report.get('sample_evidence')
    if metadata is None:
        yield from report.get('samples', [])
        return
    if (not isinstance(metadata, dict) or metadata.get('schema_version') != VERSION
            or metadata.get('kind') != KIND
            or metadata.get('inline_samples_scope') != 'native_gate_projection_v1'
            or metadata.get('retention_limit_samples') != LIMIT):
        raise ValueError('Unsupported pair sample evidence format')
    archive = metadata.get('archive', {})
    count, total = metadata.get('retained_samples'), metadata.get('total_samples')
    if (not isinstance(archive, dict) or archive.get('status') != 'complete'
            or archive.get('name') != ARCHIVE_NAME or archive.get('format') != 'gzip-members-jsonl-v1'
            or type(count) is not int or not 0 <= count <= LIMIT
            or type(total) is not int or total < count or report.get('total_samples') != total
            or not isinstance(report.get('samples'), list) or len(report['samples']) != count):
        raise ValueError('Full pair sample archive is incomplete or has invalid counts')
    directory = Path(directory).resolve()
    path = (directory / ARCHIVE_NAME).resolve(strict=True)
    if path.parent != directory:
        raise ValueError('Full pair sample archive is outside its pair directory')
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        while block := stream.read(1024 * 1024):
            digest.update(block)
    if path.stat().st_size != archive.get('bytes') or digest.hexdigest() != archive.get('sha256'):
        raise ValueError('Full pair sample archive size or SHA256 mismatch')
    expected_bytes = archive.get('decompressed_bytes')
    if type(expected_bytes) is not int or expected_bytes <= 0:
        raise ValueError('Full pair sample archive has no declared decoded size')
    consumed = 0
    with gzip.open(path, 'rb') as stream:
        def next_line():
            nonlocal consumed
            line = stream.readline(expected_bytes - consumed + 1)
            consumed += len(line)
            if consumed > expected_bytes or (line and not line.endswith(b'\n')):
                raise ValueError('Full pair sample archive exceeds its decoded size or has an incomplete row')
            return line
        header = json.loads(next_line())
        expected_header = {
            'kind': KIND, 'schema_version': VERSION,
            'proof_id': report.get('proof_id'), 'module_sha256': report.get('module_sha256'),
            'retained_samples': count, 'total_samples': total,
        }
        if header != expected_header:
            raise ValueError('Full pair sample archive identity or count mismatch')
        for _ in range(count):
            line = next_line()
            if not line:
                raise ValueError('Full pair sample archive is missing retained rows')
            row = json.loads(line)
            if not isinstance(row, dict) or 'elapsed' not in row or any(
                    not isinstance(row.get(side), dict) for side in ('host', 'client')):
                raise ValueError('Invalid full pair sample row')
            yield row
        if next_line() or consumed != expected_bytes:
            raise ValueError('Full pair sample archive has extra rows or a decoded size mismatch')
