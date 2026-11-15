"""Read the actual original kill-banner count across timed streak expiry."""
from __future__ import annotations

import time

from training_runtime_client import OwnedSession
from verify_combat_v2 import aim_at, prepare_target, snapshot, verify as verify_combat


def verify(active=None, config=None):
    report = {"passed": False}
    # This makes three real native kills, then waits the configured streak
    # interval. No damage, score or visual counter is fabricated by the test.
    report["combat"] = verify_combat(active, config)
    if not report["combat"]["passed"]:
        report["error"] = report["combat"].get("error", "Native combat failed")
        return report
    with OwnedSession(active) as session:
        probe = session.original()
        try:
            state = session.channel.state()
            before = state["player"]["kills"]
            report["expired_state"] = state["player"]
            report["expired_banner"] = snapshot("kill-feedback-view")
            report["kills_after_expiry"] = []
            for expected_streak in (1, 2):
                prepare_target(session, config)
                target_kills = before + expected_streak
                deadline = time.monotonic() + 12
                while time.monotonic() < deadline:
                    state = session.channel.state()
                    if state["player"]["kills"] >= target_kills:
                        break
                    enemies = [p for p in state["players"] if p["bot"] and p["alive"]]
                    if enemies:
                        feet = enemies[0]["feet"]
                        aim_at(probe, (feet[0], feet[1], feet[2] + 120))
                        probe.command(flags=9)
                    else:
                        probe.command(flags=8)
                    time.sleep(.04)
                probe.command(flags=8)
                if state["player"]["kills"] != target_kills:
                    raise RuntimeError("The real original shot did not complete the expected kill")
                deadline = time.monotonic() + 2
                row = None
                while time.monotonic() < deadline:
                    native = snapshot("kill-feedback-view")
                    rows = [r for r in native.get("display_records", [])
                            if r["origin"] == "announcement" and r["incoming"] == target_kills]
                    if rows:
                        row = rows[-1]
                        break
                    time.sleep(.05)
                if row is None:
                    raise RuntimeError("The original banner did not expose a readback for the native kill")
                report["kills_after_expiry"].append({"state": state["player"], "banner": row})
                if not row["readback"] or row["observed"] != expected_streak or not row["visible"]:
                    raise RuntimeError("The actual original banner did not display the short timed streak")
                session.wait(lambda s: any(p["bot"] and p["alive"] for p in s["players"]), timeout=6)
            expired = report["expired_banner"]
            report["checks"] = {
                "timed_counter_expired": report["expired_state"]["streak"] == 0,
                "original_banner_hidden_after_expiry": expired["original_banner_visibility_available"] and not expired["original_banner_visible"],
                "fourth_match_kill_displays_one": report["kills_after_expiry"][0]["banner"]["observed"] == 1,
                "next_quick_kill_displays_two": report["kills_after_expiry"][1]["banner"]["observed"] == 2,
                "match_score_preserved": state["player"]["kills"] == before + 2,
            }
            report["passed"] = all(report["checks"].values())
        except Exception as error:
            report["error"] = str(error)
        finally:
            probe.release()
    return report
