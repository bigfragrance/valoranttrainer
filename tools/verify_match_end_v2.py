"""Wait on an actual offline match, without altering score or game clocks."""
from __future__ import annotations

from datetime import datetime, timezone
import json
import time

from training_runtime_client import OwnedSession
from verify_combat_v2 import snapshot


def verify(active, output, *, time_limit=False):
    report = {'passed': False, 'time_limit_test': time_limit, 'started_utc': datetime.now(timezone.utc).isoformat(), 'samples': []}
    with OwnedSession(active) as session:
        report['pid'] = session.active['pid']
        start = time.monotonic()
        try:
            state = session.wait(lambda s: s.get('ready'))
            report['initial_remaining'] = state['time_remaining']
            deadline = time.monotonic() + 560
            last_sample = -30
            while time.monotonic() < deadline:
                if not session.alive():
                    raise RuntimeError('Owned original client exited before the match finished')
                state = session.channel.state()
                if state.get('error'):
                    raise RuntimeError(state['error'])
                elapsed = time.monotonic()-start
                if elapsed-last_sample >= 30 or state.get('ended'):
                    row = {'elapsed_seconds': elapsed, 'remaining': state['time_remaining'], 'ended': state['ended'],
                           'kills': [p['kills'] for p in state['players']], 'deaths': sum(p['deaths'] for p in state['players'])}
                    report['samples'].append(row)
                    output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
                    print(json.dumps({'checkpoint': 'native_match_clock', **row}), flush=True)
                    last_sample = elapsed
                if state.get('ended'):
                    time.sleep(1.1)
                    break
                time.sleep(.2)
            rules = snapshot('deathmatch-rules')
            report.update(elapsed_seconds=time.monotonic()-start, final_state=state, rules=rules,
                          ended_utc=datetime.now(timezone.utc).isoformat())
            if time_limit:
                report['passed'] = rules['ended'] and rules['end_reason'] == 2 and rules['remaining_seconds'] == 0 and report['elapsed_seconds'] >= report['initial_remaining']-1
            else:
                report['passed'] = rules['ended'] and rules['end_reason'] == 1 and rules['leading_kills'] == 40 and max(p['match_kills'] for p in rules['players']) == 40
            if not report['passed']:
                raise RuntimeError('Actual match did not finish at the required native kill/time boundary')
        except Exception as error:
            report['error'] = str(error)
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    return report
