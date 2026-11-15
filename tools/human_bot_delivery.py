"""Candidate adapters for the existing owned-pair control and Initial writer.

No PID attachment, extra transport, game launch, or root-source mutation here.
"""
from __future__ import annotations

import copy
import hashlib
import time

import human_bot_pack as wire
import humanlike_settings as settings


def legacy_transport_view(config):
    """Preserve the original CONFIG72/Initial7844, including old V2 bot fields."""
    result = copy.deepcopy(config)
    if result.get('schema_version') == 3:
        settings.validate(result.pop('human_bots'))
        result.pop('human_bots_migration')
        result['schema_version'] = 2
    elif result.get('schema_version') != 2:
        raise settings.ConfigError('仅接受已验证schema2/3设置')
    return result


def write_initial(directory, config, map_info, *, original_write_initial, root, transaction_id=None):
    """Call the original checks/writer, then its fixed independent sidefile.

    The caller passes a fully validated schema3 config and the original writer.
    The original ownership/session/hero/map validation stays authoritative.
    """
    human = settings.validate(config['human_bots']) if config.get('schema_version') == 3 else None
    legacy = legacy_transport_view(config)
    result = original_write_initial(directory, legacy, map_info, root=root)
    if human is None:
        return result
    bundle = wire.make_bundle(human, seed=config['seed'], revision=config['revision'],
                              identities=list(range(config['total_players'])), transaction_id=transaction_id)
    target = wire.write_initial_sidefile(result['path'], bundle, root=root)
    return {**result, 'human_bots': {**bundle.summary(), 'path': str(target),
                                   'sha256': hashlib.sha256(target.read_bytes()).hexdigest(),
                                   'status': 'initial_sidefile_written_consumer_pending'}}


def _result(bundle, status, replies, *, transport=False, effective=False, message=''):
    return {'status': status, 'revision': bundle.revision,
            'fingerprint': f'{bundle.checksum:016x}', 'schema_digest': settings.SCHEMA_DIGEST.hex(),
            'transport_applied': transport, 'consumer_effective': effective,
            'behavior_execution_verified': False, 'message': message, 'replies': replies}


def wait_effective(channel, bundle, *, timeout=3.0, poll_seconds=.05):
    """Only matching native consumer acknowledgements mean configuration effective."""
    if type(timeout) not in (int, float) or not 0 <= timeout <= 60:
        raise settings.ConfigError('等待时间范围为0–60秒')
    if type(poll_seconds) not in (int, float) or not .001 <= poll_seconds <= 1:
        raise settings.ConfigError('状态轮询间隔范围为.001–1秒')
    deadline = time.monotonic() + timeout
    while True:
        whole_state = channel.state()
        state = whole_state.get('human_bots') if type(whole_state) is dict else None
        if type(state) is not dict:
            return _result(bundle, 'native_state_unavailable', [], message='human_bots_summary_not_available')
        digest = state.get('schema_digest')
        revision = state.get('transport_applied_revision')
        fingerprint = state.get('transport_fingerprint')
        if digest is not None and digest != settings.SCHEMA_DIGEST.hex():
            return _result(bundle, 'schema_mismatch', [], message='native_schema_differs')
        if type(revision) is int and revision > bundle.revision:
            return _result(bundle, 'superseded', [], message='newer_configuration_committed')
        applied = (digest == settings.SCHEMA_DIGEST.hex() and type(revision) is int
                   and revision == bundle.revision and fingerprint == f'{bundle.checksum:016x}')
        effective = (applied and state.get('consumer_effective_known') is True
                     and type(state.get('consumer_effective_revision')) is int
                     and state['consumer_effective_revision'] == bundle.revision
                     and state.get('consumer_effective_fingerprint') == f'{bundle.checksum:016x}'
                     and state.get('configuration_effective') is True)
        if effective:
            return _result(bundle, 'consumer_effective', [], transport=True, effective=True,
                           message='configuration_acknowledged_not_a_behavior_runtime_proof')
        if time.monotonic() >= deadline:
            return _result(bundle, 'consumer_pending' if applied else 'transport_unconfirmed', [],
                           transport=applied, message='waiting_for_matching_native_consumer_ack')
        time.sleep(min(poll_seconds, max(0, deadline - time.monotonic())))


def send_bundle(channel, bundle, *, ack_timeout=3.0, transaction_timeout=25.0, effective_timeout=3.0):
    """One serial multipart transaction on existing PairChannel/RLock.

    Every frame uses existing send(), retaining ownership checks, pending-slot
    protection and ACK semantics. An unacknowledged timeout stops immediately:
    it never overwrites that command or emits an unsafe opportunistic Abort.
    """
    if type(ack_timeout) not in (int, float) or not 0 < ack_timeout <= 5:
        raise settings.ConfigError('ack_timeout范围为(0,5]秒')
    if type(transaction_timeout) not in (int, float) or not 0 < transaction_timeout <= 25:
        raise settings.ConfigError('事务总等待必须在(0,25]秒，短于native30秒限额')
    if not hasattr(channel, '_lock'):
        raise settings.ConfigError('需要现有owned PairChannel及其RLock')
    replies = []
    deadline = time.monotonic() + transaction_timeout
    with channel._lock:
        stages = bundle.stages()
        for n, frame in enumerate((*stages, bundle.commit())):
            left = deadline - time.monotonic()
            if left <= 0:
                return _result(bundle, 'stage_timeout', replies, message='old_configuration_preserved_until_native_commit')
            reply = channel.send('human_bots', frame, timeout=min(ack_timeout, left))
            replies.append(reply)
            if reply.get('acknowledged') is not True:
                return _result(bundle, 'command_pending', replies,
                               message='stop_without_overwrite_or_abort_unconsumed_command')
            expected_code = 1 if n < len(stages) else 0
            if type(reply.get('code')) is not int or reply['code'] != expected_code:
                return _result(bundle, 'rejected', replies, message=reply.get('message', 'native_rejected'))
    # Release the transaction lock so ordinary UI commands still work while
    # waiting for the already-committed config's normal decision-tick receipt.
    result = wait_effective(channel, bundle, timeout=effective_timeout)
    result['replies'] = replies
    return result


def ensure_bundle(channel, bundle, *, ack_timeout=3.0, transaction_timeout=25.0, effective_timeout=3.0):
    """Use real native summary to avoid replaying an already committed Initial.

    Matching schema/revision/FULL content checksum is a transport observation,
    never an invented command ACK. Same-revision different data is a conflict.
    A new revision still uses the existing serial send/pending-slot protection.
    """
    if not hasattr(channel, '_lock'):
        raise settings.ConfigError('需要现有owned PairChannel及其RLock')
    submitted = None
    with channel._lock:
        whole_state = channel.state()
        state = whole_state.get('human_bots') if type(whole_state) is dict else None
        if type(state) is not dict or state.get('initial_pending') is True:
            return _result(bundle, 'native_state_unavailable', [], message='native_initial_or_summary_not_ready')
        if state.get('schema_digest') != settings.SCHEMA_DIGEST.hex():
            return _result(bundle, 'schema_mismatch', [], message='native_schema_differs_or_is_not_ready')
        configured = state.get('configured')
        revision = state.get('transport_applied_revision')
        fingerprint = state.get('transport_fingerprint')
        if configured is True:
            if (type(revision) is not int or not 0 <= revision <= 0x7fffffff
                    or type(fingerprint) is not str or len(fingerprint) != 16
                    or any(c not in '0123456789abcdef' for c in fingerprint)):
                return _result(bundle, 'native_state_unavailable', [], message='native_transport_identity_invalid')
            if revision > bundle.revision:
                return _result(bundle, 'superseded', [], message='newer_configuration_committed')
            if revision == bundle.revision and fingerprint != f'{bundle.checksum:016x}':
                result = _result(bundle, 'revision_conflict', [], message='same_revision_has_different_native_content_checksum')
                result['observed_transport_revision'] = revision
                result['observed_transport_fingerprint'] = fingerprint
                return result
            equivalent = revision == bundle.revision and fingerprint == f'{bundle.checksum:016x}'
        elif configured is False:
            equivalent = False  # Explicit native legacy fallback, no active human-bot group.
        else:
            return _result(bundle, 'native_state_unavailable', [], message='native_configured_state_unknown')
        if not equivalent:
            # Keep the real-state precheck and whole multipart send in the same
            # existing RLock. Polling with zero timeout does not block the GT.
            submitted = send_bundle(channel, bundle, ack_timeout=ack_timeout,
                                    transaction_timeout=transaction_timeout, effective_timeout=0)
            replies = submitted['replies']
            committed = (len(replies) == len(bundle.stages()) + 1
                         and replies[-1].get('acknowledged') is True
                         and type(replies[-1].get('code')) is int and replies[-1]['code'] == 0)
            if not committed:
                return submitted
    # The outer transaction lock is released before waiting for an ordinary
    # decision tick, so menu commands remain possible while consumers apply.
    result = wait_effective(channel, bundle, timeout=effective_timeout)
    result['replies'] = submitted['replies'] if submitted is not None else []
    result['already_transport_committed'] = submitted is None
    return result
