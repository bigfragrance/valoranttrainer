"""Regression checks for startup claims and intentionally narrow output."""
import contextlib
import io
import json
from pathlib import Path
import tempfile
import unittest

from tools.native_status import analyze_files, analyze_runtime, main


DUEL = "/Game/Maps/Duel/Duel_1/Skirmish_A"
INIT = "/Game/Maps/Init/Init"


class NativeStatusTests(unittest.TestCase):
    def test_exit_zero_and_requested_duel_do_not_prove_entry(self):
        log = (
            f"LogMapLoadModel: Update: [Map Ready: TRUE | Changed: TRUE] [URL: {INIT}?Name=PrivateUser]\n"
            "LogPlatformInitializerV2: Initialized: NativePlatformAuthManager in 0.000s\n"
        )
        report = analyze_runtime(log, {"arguments": [DUEL], "exit_code": 0})
        self.assertEqual(report["launch_record"]["recorded_exit_code"], 0)
        self.assertFalse(report["assessment"]["target_world_observed"])
        self.assertFalse(report["assessment"]["target_map_ready_observed"])
        self.assertFalse(report["assessment"]["account_login_success_proven"])
        self.assertEqual(report["runtime"]["world_state_events"][0]["package"], INIT)
        self.assertNotIn("PrivateUser", json.dumps(report))
        self.assertEqual(report["assessment"]["active_blockers"][0]["code"], "target_world_not_observed")

    def test_actual_platform_failure_has_system_reason_and_evidence_lines(self):
        log = (
            "LogPlatformInitializerV2: Beginning platform initialization\n"
            "LogPlatformInitializerV2: Status is now: Initializing\n"
            "LogPlatformInitializerV2: Initializing: RiotGamesApiClient\n"
            "LogPlatformInitializerV2: Error: Failed to initialize: RiotGamesApiClient (error: LaunchRiotClient) after 0.035s\n"
            "LogPlatformInitializerV2: Status is now: InitFailure\n"
        )
        report = analyze_runtime(log)
        self.assertEqual(report["runtime"]["platform_final_status"], "InitFailure")
        self.assertEqual(report["runtime"]["platform_failures"], [
            {"line": 4, "system": "RiotGamesApiClient", "reason": "LaunchRiotClient"}
        ])
        self.assertEqual(report["assessment"]["active_blockers"][0]["evidence_lines"], [4, 5])
        self.assertEqual(report["runtime"]["platform_events"][2]["state"], "initializing")

    def test_repeated_serial_error_is_grouped_without_raw_object_or_hash(self):
        message = (
            "LogWindows: Error: ObjectSerializationError: /Game/Globals/HUDGlobals_DesktopDataAsset "
            "(0xPRIVATE_HASH) HUDGlobalsDataAsset: Serial size mismatch: Expected read size 1792, Actual read size 552"
        )
        report = analyze_runtime(message + "\n" + message)
        self.assertEqual(report["runtime"]["serialization_errors"], [{
            "asset": "/Game/Globals/HUDGlobals_DesktopDataAsset", "expected_bytes": 1792,
            "actual_bytes": 552, "evidence_lines": [1, 2],
        }])
        self.assertNotIn("PRIVATE_HASH", json.dumps(report))
        self.assertEqual(report["assessment"]["active_blockers"][0]["code"], "object_serialization_mismatch")

    def test_browse_and_loadmap_are_requests_and_not_observed_worlds(self):
        log = f"LogNet: Browse: {DUEL}?Name=Secret\nLogLoad: LoadMap: {DUEL}\n"
        report = analyze_runtime(log, {"arguments": [DUEL]})
        self.assertEqual(len(report["runtime"]["map_travel_requests"]), 2)
        self.assertFalse(report["assessment"]["target_world_observed"])
        self.assertEqual(report["runtime"]["world_state_events"], [])

    def test_ready_target_is_evidence_of_map_state_not_working_training(self):
        log = f"LogMapLoadModel: Update: [Local World: TRUE | Changed: TRUE] [Map Ready: TRUE | Changed: TRUE] [URL: {DUEL}?Name=Secret]\n"
        report = analyze_runtime(log, {"arguments": [DUEL]})
        self.assertTrue(report["assessment"]["target_world_observed"])
        self.assertTrue(report["assessment"]["target_map_ready_observed"])
        self.assertFalse(report["assessment"]["training_ready_proven"])
        self.assertEqual(report["assessment"]["outcome"], "target_map_ready_observed")
        self.assertTrue(report["runtime"]["world_state_events"][0]["local_world"])

    def test_command_line_text_never_becomes_runtime_world_evidence(self):
        report = analyze_runtime(f"LogInit: Command line: {DUEL} -token=Secret\n", target_map=DUEL)
        self.assertEqual(report["runtime"]["world_state_events"], [])
        self.assertFalse(report["assessment"]["target_world_observed"])
        self.assertNotIn("Secret", json.dumps(report))

    def test_class_names_keep_evidence_but_drop_instance_ids_and_launch_secrets(self):
        log = (
            "LogScript: No world was found for object (/Engine/Transient.AresGameEngine_2147482591:"
            "BPAresGameInstance_C_2147482543.BPAresClientGameInstance_C_2147482475.OtherObject_99)\n"
            "LogRSOManager: Error: RNET_NOT_CONNECTED token=PRIVATE_TOKEN subject=PRIVATE_USER\n"
        )
        report = analyze_runtime(log, {
            "arguments": [DUEL + "?Name=PRIVATE_USER", "-token=PRIVATE_TOKEN"],
            "pid": 123456789, "credentials_supplied": False, "login_action": False,
            "extra": {"token": "PRIVATE_TOKEN"},
        })
        encoded = json.dumps(report)
        for secret in ("2147482591", "2147482543", "2147482475", "PRIVATE_TOKEN", "PRIVATE_USER", "123456789"):
            self.assertNotIn(secret, encoded)
        self.assertEqual(report["runtime"]["engine_classes_from_object_paths"], [
            {"class": "AresGameEngine", "evidence_lines": [1]}
        ])
        self.assertEqual([item["class"] for item in report["runtime"]["game_instance_classes_from_object_paths"]],
                         ["BPAresGameInstance_C", "BPAresClientGameInstance_C"])
        self.assertEqual(report["runtime"]["rnet_not_connected_lines"], [2])

    def test_later_overall_initialized_status_preserves_but_clears_old_failure(self):
        log = (
            "LogPlatformInitializerV2: Error: Failed to initialize: ExampleSystem (error: Timeout)\n"
            "LogPlatformInitializerV2: Status is now: Initialized\n"
        )
        report = analyze_runtime(log)
        self.assertEqual(len(report["runtime"]["platform_failures"]), 1)
        self.assertEqual(report["assessment"]["active_blockers"], [])

    def test_utf16_log_bom_launch_and_cli_write_sanitized_report(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            log = root / "runtime.log"
            launch = root / "launch.json"
            output = root / "reports" / "status.json"
            log.write_text(f"LogWorld: Bringing World {DUEL}.Skirmish_A up for play\n", encoding="utf-16")
            launch.write_text(json.dumps({"arguments": [DUEL], "exit_code": 0}), encoding="utf-8-sig")
            self.assertTrue(analyze_files(log, launch)["assessment"]["target_map_ready_observed"])
            with contextlib.redirect_stdout(io.StringIO()) as stdout:
                result = main(["--log", str(log), "--launch", str(launch), "--output", str(output)])
            self.assertEqual(result, 0)
            self.assertEqual(json.loads(stdout.getvalue()), json.loads(output.read_text(encoding="utf-8")))

    def test_invalid_input_reports_error_without_echoing_input_contents(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            log, launch = root / "runtime.log", root / "launch.json"
            log.write_text("", encoding="utf-8")
            launch.write_text('{"token": "PRIVATE_VALUE"', encoding="utf-8")
            with contextlib.redirect_stderr(io.StringIO()) as stderr:
                result = main(["--log", str(log), "--launch", str(launch)])
            self.assertEqual(result, 2)
            self.assertNotIn("PRIVATE_VALUE", stderr.getvalue())
        with self.assertRaisesRegex(ValueError, "JSON object"):
            analyze_runtime("", [])


if __name__ == "__main__":
    unittest.main()
