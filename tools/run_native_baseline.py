"""Run a bounded game-client experiment after the shared no-login checks."""
from __future__ import annotations

import argparse
import ctypes
from contextlib import ExitStack
from ctypes import wintypes
from datetime import datetime, timezone
import json
import hashlib
from pathlib import Path
import subprocess
import sys
import time

from native_status import analyze_files
from native_log_marker import NativeLogMarker
from native_observation import ConfigObserver
from native_profile import temporary_profile
from native_config_snapshot import CoreSnapshot
from native_maps import MAPS, map_url, map_package, parse_map
from local_environment import project_environment
from empty_bootstrap_probe import (normalize_nonce, validate_native_options, native_request,
                                   child_environment, arm_for_child, require_probe_build)
from native_world_observation import WorldObserver, assess_world_samples
from run_native_probe import ROOT, game_processes, preflight, save
from client_layout import client_executable, is_portable


def network_module_for_flavor(flavor):
    """Select one library for build, ownership, peer preflight and loading."""
    if flavor == 'proof':
        import local_network_probe
        return local_network_probe
    if flavor == 'playable':
        import local_server_module
        return local_server_module
    raise ValueError('Unknown native network flavor')


def network_world_package(flavor, map_number, mode):
    """The preserved DefaultPawn proof never inherits candidate map support."""
    if mode == 'navigation' and type(map_number) is int and map_number == 1:
        if flavor in ('proof', 'playable'):
            return map_package(1)
    if flavor == 'playable' and mode == 'training':
        from local_server_maps import selected_map
        return selected_map(map_number)['world_path']
    raise ValueError('Proof requires Duel 1 navigation; candidate training requires a whitelisted map id')


def load_selected_training_module(args, process, context, selected_network_module=None):
    """A network candidate cannot fall through to the standalone trainer DLL."""
    if args.network_probe:
        if selected_network_module is None:
            raise RuntimeError('A network runner must select its own module')
        return selected_network_module.load(process, context, role=args.network_probe,
                                            port=args.network_port, output=args.network_output,
                                            transport=args.network_transport)
    from native_module import load_owned_module
    return load_owned_module(process, context, args.module_stage)


def assign_owned_job(kernel, process_handle):
    class BasicLimits(ctypes.Structure):
        _fields_ = [("process_time", ctypes.c_int64), ("job_time", ctypes.c_int64),
                    ("flags", wintypes.DWORD), ("working_min", ctypes.c_size_t),
                    ("working_max", ctypes.c_size_t), ("active_limit", wintypes.DWORD),
                    ("affinity", ctypes.c_size_t), ("priority", wintypes.DWORD),
                    ("scheduling", wintypes.DWORD)]

    class Limits(ctypes.Structure):
        _fields_ = [("basic", BasicLimits), ("io", ctypes.c_uint64 * 6),
                    ("process_memory", ctypes.c_size_t), ("job_memory", ctypes.c_size_t),
                    ("peak_process", ctypes.c_size_t), ("peak_job", ctypes.c_size_t)]

    kernel.CreateJobObjectW.argtypes = [ctypes.c_void_p, wintypes.LPCWSTR]
    kernel.CreateJobObjectW.restype = wintypes.HANDLE
    kernel.SetInformationJobObject.argtypes = [wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p, wintypes.DWORD]
    kernel.AssignProcessToJobObject.argtypes = [wintypes.HANDLE, wintypes.HANDLE]
    job = kernel.CreateJobObjectW(None, None)
    if not job:
        raise RuntimeError("Baseline job creation failed")
    limits = Limits()
    limits.basic.flags = 0x3000  # KILL_ON_JOB_CLOSE | SILENT_BREAKAWAY_OK, as in the capture.
    if not kernel.SetInformationJobObject(job, 9, ctypes.byref(limits), ctypes.sizeof(limits)) or not kernel.AssignProcessToJobObject(job, process_handle):
        kernel.CloseHandle(job)
        raise RuntimeError("Baseline job assignment failed")
    return job


def resume_owned_initial_thread(kernel, pid):
    class ThreadEntry(ctypes.Structure):
        _fields_ = [("size", wintypes.DWORD), ("usage", wintypes.DWORD),
                    ("tid", wintypes.DWORD), ("pid", wintypes.DWORD),
                    ("base_priority", wintypes.LONG), ("delta_priority", wintypes.LONG),
                    ("flags", wintypes.DWORD)]

    kernel.CreateToolhelp32Snapshot.argtypes = [wintypes.DWORD, wintypes.DWORD]
    kernel.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
    kernel.Thread32First.argtypes = [wintypes.HANDLE, ctypes.POINTER(ThreadEntry)]
    kernel.Thread32Next.argtypes = [wintypes.HANDLE, ctypes.POINTER(ThreadEntry)]
    kernel.OpenThread.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.OpenThread.restype = wintypes.HANDLE
    kernel.ResumeThread.argtypes = [wintypes.HANDLE]
    kernel.ResumeThread.restype = wintypes.DWORD
    snapshot = kernel.CreateToolhelp32Snapshot(4, 0)
    if snapshot == wintypes.HANDLE(-1).value:
        raise RuntimeError("Could not enumerate the suspended child's initial thread")
    ids = []
    try:
        entry = ThreadEntry()
        entry.size = ctypes.sizeof(entry)
        found = kernel.Thread32First(snapshot, ctypes.byref(entry))
        while found:
            if entry.pid == pid:
                ids.append(entry.tid)
            found = kernel.Thread32Next(snapshot, ctypes.byref(entry))
    finally:
        kernel.CloseHandle(snapshot)
    if len(ids) != 1:
        raise RuntimeError("Expected exactly one thread in the newly created suspended child")
    thread = kernel.OpenThread(2, False, ids[0])
    if not thread:
        raise RuntimeError("Could not open the suspended child's initial thread")
    try:
        if kernel.ResumeThread(thread) != 1:
            raise RuntimeError("Unexpected initial suspend count")
    finally:
        kernel.CloseHandle(thread)


def cleanup_resources(report, native_channel, process, training_host, training_host_log,
                      kernel, image_handle, job_handle, profiles):
    """Attempt every independent release even if another release fails."""
    def attempt(name, action):
        try:
            action()
        except Exception as error:
            report.setdefault('cleanup_errors', []).append(
                {'action': name, 'type': type(error).__name__, 'message': str(error)})
            report['status'] = 'error'

    if native_channel is not None:
        attempt('native_channel.close', native_channel.close)
    if process is not None:
        running = [True]
        def stop_process():
            running[0] = process.poll() is None
            if running[0]:
                process.kill()
        attempt('process.kill', stop_process)
        if running[0]:
            attempt('process.wait', lambda: process.wait(timeout=5))
    if training_host is not None:
        timed_out = [False]
        def wait_host():
            try:
                training_host.wait(timeout=3)
            except subprocess.TimeoutExpired:
                timed_out[0] = True
        attempt('training_host.wait', wait_host)
        if timed_out[0]:
            attempt('training_host.terminate', training_host.terminate)
            attempt('training_host.wait_after_terminate', lambda: training_host.wait(timeout=3))
        report['training_host_exit_code'] = training_host.returncode
    if training_host_log is not None:
        attempt('training_host_log.close', training_host_log.close)
    def close_handle(handle):
        if not kernel.CloseHandle(handle):
            raise ctypes.WinError(ctypes.get_last_error())
    if image_handle is not None:
        attempt('image_handle.close', lambda: close_handle(image_handle))
    if job_handle is not None:
        attempt('job_handle.close', lambda: close_handle(job_handle))
    attempt('profiles.close', profiles.close)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", action="store_true")
    parser.add_argument('--peek-fixture-nonce')
    parser.add_argument('--map-hold-fixture-nonce')
    parser.add_argument('--native-test-scenario', choices=('shoot', 'shop', 'shoot-target', 'ffa-battle', 'phoenix-c-primary', 'jett-c', 'jett-q', 'jett-e', 'jett-x', 'jett-x-target', 'shop-correction-prelude', 'peek-fixture', 'map-hold-fixture', 'spectator-input-fixture'))
    parser.add_argument('--native-local-cipher', action='store_true')
    parser.add_argument("--seconds", type=float, default=20)
    parser.add_argument("--read-lock", action="store_true", help="Hold the same read/share-read image handle as the capture tools")
    parser.add_argument("--job", action="store_true", help="Use the capture's owned kill-on-close job")
    parser.add_argument("--suspended", action="store_true", help="Create suspended then resume, without memory inspection or modifications")
    parser.add_argument("--observe-config", action="store_true", help="Read only the fixed GEngineIni FString after the client has opened its log")
    parser.add_argument("--observe-world", action="store_true", help="Read the owned process's target World, local controller and Pawn state")
    parser.add_argument("--profile", choices=("source-probe", "base-engine"), help="Temporarily merge a local source overlay, restoring prior bytes after the child exits")
    parser.add_argument("--export-saved-config", action="store_true", help="Archive only the bounded preloaded shipped configuration buffer")
    parser.add_argument("--cached-config", action="store_true", help="Use the client's normal cached-configuration path instead of -textconfig")
    parser.add_argument("--observe-categories", action="store_true", help="Verify the metadata decoder on known categories; emit category names only")
    parser.add_argument("--export-core-config", action="store_true", help="Export nine core game categories without reading account-settings maps")
    parser.add_argument("--bootstrap", type=Path, help="Use a locally generated and decoded native IniBootstrap archive")
    parser.add_argument("--map", type=parse_map, default=1, dest="map_number")
    parser.add_argument("--mode", choices=("original", "scene", "navigation", "training"), default="original",
                        help="scene uses Engine.GameMode as a diagnostic control; it does not implement training")
    parser.add_argument("--result-file", type=Path, help="Also write the run report to a local orchestration path")
    parser.add_argument("--class-config-experiment", choices=("pawn", "player"),
                        help="Duel 1 only: mark selected class metadata Config before the owned child starts")
    parser.add_argument("--training-module", action="store_true", help="Load the built local training module after the owned Duel 1 world is observed")
    parser.add_argument("--module-stage", type=int, choices=(1, 2, 3, 4, 5, 6, 7, 8, 9), default=1)
    parser.add_argument('--original-game-instance', action='store_true',
                        help='Use original GameInstance with only the native engine offline lifecycle')
    parser.add_argument('--original-host-mode', action='store_true',
                        help='Opt-in fresh original ShooterMode before GI creation; paired training host only')
    parser.add_argument('--fixed-bot-roster-required', action='store_true',
                        help='Require the fixed bot roster sidecar on this original playable host')
    parser.add_argument('--original-visual-classes',action='store_true',
                        help='Create the original Ares viewport and ShooterLocalPlayer from the local bootstrap')
    parser.add_argument("--no-training-hud", action="store_true")
    parser.add_argument('--network-probe', choices=('host','client'), help='Isolated native loopback driver proof; uses its own library and diagnostics')
    parser.add_argument('--network-flavor', choices=('proof','playable'), default='proof',
                        help='Use the independent DefaultPawn proof or the ShooterGame candidate library')
    parser.add_argument('--network-output', type=Path)
    parser.add_argument('--network-port', type=int, default=17777)
    parser.add_argument('--network-transport',choices=('ares','ip'),default='ares')
    parser.add_argument('--network-proof-id')
    parser.add_argument('--network-peer', type=Path, help='Exact active record of this proof runner-owned host')
    parser.add_argument('--interactive-local-server',action='store_true',
                        help='Allow the explicitly paired local-server candidate up to 24 hours')
    parser.add_argument("--controller-settings", type=Path, help="Use the local WebUI command protocol and this validated project-owned configuration")
    parser.add_argument("--interactive-training", action="store_true", help="Allow a local training session up to 24 hours; original mode uses F10 to exit")
    parser.add_argument('--empty-bootstrap-probe-nonce', type=normalize_nonce,
                        help='Explicit child-bound empty-equipment marker; ordinary paired FFA only')
    args = parser.parse_args()
    try:
        validate_native_options(args)
    except ValueError as error:
        parser.error(str(error))
    if args.native_local_cipher and not (args.network_probe and args.network_flavor == 'playable'
                                         and args.interactive_local_server and args.network_transport == 'ares'):
        parser.error('Native local cipher requires the explicitly paired interactive Ares candidate')
    if args.map_hold_fixture_nonce:
        from humanlike_map_hold_fixture import validate_native_nonce
        validate_native_nonce(args.map_hold_fixture_nonce, args.network_probe, args.network_flavor, args.original_host_mode, args.native_test_scenario)
    if args.peek_fixture_nonce:
        from humanlike_peek_fixture import validate_native_nonce
        validate_native_nonce(args.peek_fixture_nonce, args.network_probe, args.network_flavor, args.original_host_mode, args.native_test_scenario)
    if args.native_test_scenario and not (args.network_probe == 'client' and args.network_flavor == 'playable'
                                          and args.interactive_local_server):
        parser.error('Native integration scenarios require an explicit interactive playable client pair')
    selected_network_module = None
    if args.fixed_bot_roster_required and not (args.original_host_mode and args.network_probe == 'host' and args.network_flavor == 'playable' and args.mode == 'training'):
        parser.error('Fixed bot roster requires the original playable training host')
    if args.original_host_mode and not (args.network_probe == 'host' and args.network_flavor == 'playable'
                                        and args.original_game_instance and args.interactive_local_server
                                        and args.mode == 'training'):
        parser.error('Original host mode requires the explicit interactive playable training host')
    if args.network_probe:
        import re
        selected_network_module = network_module_for_flavor(args.network_flavor)
        try:
            network_world_package(args.network_flavor, args.map_number, args.mode)
        except ValueError as error:
            parser.error(str(error))
        if (not args.bootstrap or not args.original_game_instance or args.controller_settings
                or not args.network_output or not args.result_file or not re.fullmatch('[0-9a-f]{32}',args.network_proof_id or '')
                or not 1024<=args.network_port<=65535 or (args.network_probe=='client')!=bool(args.network_peer)):
            parser.error('Native network requires an allowed map, offline original GI, isolated output, proof id, port and a bound peer; paired initial control comes from session-initial.bin')
        args.network_output=selected_network_module.local_path(args.network_output)
        args.network_output.mkdir(parents=True,exist_ok=True)
        args.training_module=True
        args.module_stage=1
        network_build = selected_network_module.verified_build()
        if args.empty_bootstrap_probe_nonce is not None:
            require_probe_build(network_build)
        if args.native_local_cipher and not any(Path(name).name == 'local_server_ares_local_key.h'
                                                for name in network_build.get('source_sha256', {})):
            parser.error('This compiled candidate does not include the native local cipher adapter')
    elif any((args.network_output,args.network_peer,args.network_proof_id,args.network_flavor!='proof')):
        parser.error('Network proof options require --network-probe')
    module_marker = (args.network_output/'bridge-state.json' if args.network_probe
                     else ROOT/'.local/native-module/bridge-state.json')
    selected_package = map_package(args.map_number, training=args.mode == 'training')
    if args.interactive_training and (not args.training_module or args.module_stage not in (3,8) or
            not (args.mode == 'training' or args.map_number == 1 and args.mode == "navigation")):
        parser.error("Interactive training is only available for the Duel 1 local training module")
    if args.interactive_training and args.module_stage==8 and not args.original_game_instance:
        parser.error("Original interactive training requires its offline GameInstance")
    if args.interactive_local_server and (not args.network_probe or args.network_flavor!='playable' or args.interactive_training):
        parser.error('Interactive local server requires the isolated playable network flavor')
    max_seconds = 86400 if args.interactive_training or args.interactive_local_server else (180 if args.mode in ("navigation", "training") else 45)
    if args.mode == 'training' and args.module_stage == 9 and args.controller_settings and args.original_game_instance:
        max_seconds = 900  # Bounded, explicitly owned offline match-rule verification.
    if not 1 <= args.seconds <= max_seconds:
        parser.error(f"seconds must be between 1 and {max_seconds} for this mode")
    if args.class_config_experiment and (args.mode != "navigation" or args.map_number != 1 or not args.bootstrap):
        parser.error("Class metadata experiment requires Duel 1 navigation and an explicit local bootstrap")
    if args.training_module:
        if not (args.mode == 'training' or args.mode == "navigation" and args.map_number == 1) or args.class_config_experiment:
            parser.error("Training module requires Duel 1 navigation without the class-metadata experiment")
        args.observe_world = True
    if args.original_game_instance and (not args.training_module or not args.bootstrap or args.class_config_experiment):
        parser.error('Original GameInstance requires a bound bootstrap and training module')
    if args.original_visual_classes and not args.original_game_instance:
        parser.error('Original visual classes require the original offline GameInstance')
    if args.controller_settings:
        args.controller_settings = args.controller_settings.resolve(strict=True)
        if ROOT / '.local' not in args.controller_settings.parents or args.mode != 'training' or not args.training_module:
            parser.error('WebUI control requires a project-owned configuration and training mode')
        from training_settings_v2 import validate as validate_controller_settings
        validate_controller_settings(json.loads(args.controller_settings.read_text(encoding='utf-8')))
    create_suspended = args.suspended or bool(args.class_config_experiment)
    if args.result_file:
        args.result_file = args.result_file.resolve()
        if ROOT / ".local" not in args.result_file.parents:
            parser.error("result-file must be inside the project's .local directory")
        if args.result_file.exists():
            parser.error("result-file already exists")
        args.result_file.parent.mkdir(parents=True, exist_ok=True)
    run_dir = ROOT / ".local/native-baseline" / datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    run_dir.mkdir(parents=True)
    config = json.loads((ROOT / "project.json").read_text(encoding="utf-8"))
    report = {"kind": "native-client-startup", "game_memory_modified": False,
              "report_path": str(run_dir / "report.json"),
              "debugger_attached": False, "executable_file_lock": args.read_lock,
              "owned_job": args.job,
              "created_suspended": create_suspended,
              "map_number": args.map_number, "mode": args.mode,
              "passive_config_observation": args.observe_config,
              "passive_world_observation": args.observe_world,
              "credentials_supplied": False, "login_action": False}
    process = None
    native_channel = native_probe = None
    native_probe_time = native_probe_changed = None
    native_stale_recorded = False
    native_fault_published = -1
    native_fault_sequences = set()
    next_native_probe_open = 0.0
    next_original_heartbeat_sample = 0.0
    owned_active_path = None
    training_host = None
    training_host_log = None
    def check_preflight(*, postflight=False):
        if args.network_probe:
            return selected_network_module.pair_preflight(config,args.network_peer,args.network_proof_id,allow_exited=postflight)
        return preflight(config)
    image_handle = None
    job_handle = None
    original_lifecycle_plan = None
    original_mode_request = None
    profiles = ExitStack()
    platform_log_marker = NativeLogMarker()
    hang_log_marker = NativeLogMarker(b'Hang detected on GameThread')
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, ctypes.c_void_p,
                                   wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE]
    kernel.CreateFileW.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    try:
        report["preflight"] = check_preflight()
        if not args.run:
            report["status"] = "preflight_only"
        else:
            if not report["preflight"]["ready"]:
                raise RuntimeError("Baseline blocked by shared no-login preflight")
            executable = client_executable(config, ROOT)
            if args.original_game_instance:
                from offline_gi_instance import lifecycle_plan, verified_build as verified_startup_build
                verified_startup_build()
                original_lifecycle_plan = lifecycle_plan(ROOT / config['analysis_snapshot'])
            if args.profile:
                overlay = (ROOT / "profiles" / (args.profile + ".ini")).read_text(encoding="utf-8")
                target = executable.parents[2] / "Config/UserEngine.ini"
                report["profile"] = profiles.enter_context(temporary_profile(target, overlay, run_dir))
            if args.read_lock:
                image_handle = kernel.CreateFileW(str(executable), 0x80000000, 1, None, 3, 0x80, None)
                if image_handle == wintypes.HANDLE(-1).value:
                    image_handle = None
                    raise RuntimeError("Could not open the baseline image read lock")
            runtime_log = run_dir / "runtime.log"
            arguments = [map_url(args.map_number, args.mode),
                         "-EngineINI=" + str(ROOT / "work/client-analysis/BaseEngine.ini"),
                         "-windowed", "-ResX=1280", "-ResY=720",
                         "-log", "-abslog=" + str(runtime_log), "-NoSplash", "-NoStartupMovies"]
            if args.original_host_mode:
                arguments[0] = selected_package + '?game=/Script/ShooterGame.ShooterGameMode'
            if args.network_probe:
                arguments.append('-MULTIHOME=127.0.0.1')
                if args.network_flavor=='playable' and args.network_probe=='host':
                    arguments.extend(('-nosound', '-nullrhi'))
                if args.native_local_cipher:
                    arguments.append('-LocalServerNativeCipher')
                if args.peek_fixture_nonce:
                    arguments.append('-LocalServerPeekFixture=' + args.peek_fixture_nonce)
                if args.map_hold_fixture_nonce:
                    arguments.append('-LocalServerMapHoldFixture=' + args.map_hold_fixture_nonce)
                if args.fixed_bot_roster_required:
                    arguments.append('-LocalServerBotRosterRequired=1')
                if args.native_test_scenario:
                    if args.network_flavor != 'playable' or args.network_probe != 'client':
                        raise RuntimeError('Native test scenario is confined to the owned candidate client')
                    arguments.append('-LocalServerNativeTest=' + args.native_test_scenario)
                report['network_probe']={'role':args.network_probe,'proof_id':args.network_proof_id,
                                         'flavor':args.network_flavor,
                                         'port':args.network_port,'output':str(args.network_output),
                                         'transport':args.network_transport}
            if not args.cached_config:
                arguments.append("-textconfig")
            if args.bootstrap:
                archive = args.bootstrap.resolve(strict=True)
                if ROOT not in archive.parents:
                    raise RuntimeError("Bootstrap archive must be inside this project")
                from ini_bootstrap import decode, encode, set_value
                snapshot = decode(archive.read_bytes())
                if args.original_host_mode:
                    set_value(snapshot['categories'][0]['file'], '/Script/EngineSettings.GameMapsSettings',
                              'LocalMapOptions', '?game=/Script/ShooterGame.ShooterGameMode')
                if args.network_probe and args.network_transport=='ip':
                    from local_network_probe import configure_ip_transport
                    report['network_probe']['configured_ip_driver_definitions']=configure_ip_transport(snapshot)
                if args.original_game_instance:
                    set_value(snapshot['categories'][0]['file'], '/Script/EngineSettings.GameMapsSettings',
                              'GameInstanceClass', '/Script/ShooterGame.AresGameInstance')
                    if args.original_visual_classes:
                        set_value(snapshot['categories'][0]['file'],'/Script/Engine.Engine',
                                  'GameViewportClientClassName','/Script/ShooterGame.AresGameViewportClient')
                        set_value(snapshot['categories'][0]['file'],'/Script/Engine.Engine',
                                  'LocalPlayerClassName','/Script/ShooterGame.ShooterLocalPlayer')
                        report['original_visual_classes']={'viewport':'/Script/ShooterGame.AresGameViewportClient',
                            'local_player':'/Script/ShooterGame.ShooterLocalPlayer','constructed_by_engine':True}
                    settings_profile = (args.network_output/'GameUserSettings.ini' if args.network_probe
                                        else ROOT / '.local/profiles/GameUserSettings.ini')
                    settings_profile.parent.mkdir(parents=True, exist_ok=True)
                    settings_ini = snapshot['global_ini_strings'][11]
                    if Path(settings_ini).name != 'GameUserSettings.ini':
                        raise RuntimeError('Pinned game-user-settings INI slot changed')
                    source_settings = (executable.parent / settings_ini).resolve()
                    if not is_portable(config) and not settings_profile.exists() and source_settings.is_file():
                        settings_profile.write_bytes(source_settings.read_bytes())
                    snapshot['global_ini_strings'][11] = str(settings_profile)
                    report['isolated_settings_profile'] = str(settings_profile)
                    archive = run_dir / 'original-game-instance.bin'
                    archive.write_bytes(encode(snapshot))
                    if decode(archive.read_bytes()) != snapshot:
                        raise RuntimeError('Original GameInstance bootstrap did not round trip')
                report["bootstrap_archive"] = {"path": str(archive), "bytes": archive.stat().st_size,
                                                "sha256": hashlib.sha256(archive.read_bytes()).hexdigest()}
                arguments.append("-IniBootstrap=" + str(archive))
            started = time.monotonic()
            if args.original_host_mode:
                # Complete snapshot/hash/export validation before a child exists.
                # The running-child pre-CDO window only performs owned loading,
                # POD publication and the unchanged verified primary hold.
                from local_server_mode_startup import request as mode_startup_request
                original_mode_request = mode_startup_request(output=args.network_output,
                    package=selected_package, port=args.network_port, transport=args.network_transport)
            empty_bootstrap_request = native_request(args, executable)
            game_child_environment = child_environment(project_environment(ROOT), empty_bootstrap_request)
            process = subprocess.Popen([str(executable), *arguments], cwd=executable.parent,
                                       creationflags=subprocess.CREATE_NO_WINDOW | (4 if create_suspended else 0),
                                       env=game_child_environment)
            if empty_bootstrap_request is not None:
                report['empty_equipment_bootstrap_probe'] = arm_for_child(process, empty_bootstrap_request)
            if args.job or args.class_config_experiment or args.training_module:
                job_handle = assign_owned_job(kernel, int(process._handle))
            if args.class_config_experiment:
                from native_class_config import descriptor_plan, apply_to_suspended_child
                plans = descriptor_plan(ROOT / config["analysis_snapshot"], args.class_config_experiment)
                # Any failure below leaves the child suspended until finally kills it.
                report["game_memory_modified"] = True
                report["class_config_experiment"] = apply_to_suspended_child(process, plans)
            if create_suspended:
                resume_owned_initial_thread(kernel, process.pid)
            report["pid"] = process.pid
            if args.result_file or args.training_module and args.module_stage >= 5:
                active = {"pid": process.pid, "run_dir": str(run_dir),
                    "mode": args.mode, "map_number": args.map_number, "module_stage": args.module_stage}
                if args.training_module and args.module_stage >= 5:
                    from capture_owned_thread import owned_launch_identity
                    active.update(owned_launch_identity(process, executable))
                if args.network_probe:
                    active.update(network_probe=report['network_probe'],
                                  network_probe_identity=selected_network_module.owned_identity(process,executable))
                owned_active_path = run_dir / 'owned.active.json'
                save(owned_active_path, active)
                if args.result_file:
                    save(args.result_file.with_suffix(".active.json"), active)
            report["arguments"] = arguments
            report["start_time"] = time.time()
            samples = []
            observations = []
            observer = ConfigObserver(process, executable.name) if args.observe_config or args.export_saved_config or args.observe_categories or args.export_core_config or args.observe_world else None
            world_observer = WorldObserver(observer, selected_package) if args.observe_world else None
            world_observations = []
            next_world_sample = 0.0
            next_config_sample = 0.0
            next_archive_sample = 0.0
            next_sample = 0.0
            while process.poll() is None and time.monotonic() - started < args.seconds:
                elapsed = time.monotonic() - started
                if args.original_game_instance and not report.get('original_game_instance_lifecycle'):
                    # Log creation precedes UGameEngine's GameInstance creation;
                    # wait for the executable's normal loader/startup first.
                    if runtime_log.is_file():
                        from offline_gi_instance import apply_to_owned_startup
                        report['game_memory_modified'] = True
                        report['startup_adapter'] = {'invoked_at_elapsed_seconds': elapsed}
                        report['original_game_instance_lifecycle'] = apply_to_owned_startup(
                            process, original_lifecycle_plan, report_directory=run_dir,
                            host_mode_startup=original_mode_request, diagnostics=report['startup_adapter'])
                        report['original_game_instance_lifecycle']['applied_at_seconds'] = elapsed
                        mode_receipt = report['original_game_instance_lifecycle'].get('host_mode_startup')
                        if mode_receipt:
                            report['training_module'] = dict(mode_receipt)
                    elif elapsed > 12:
                        raise RuntimeError('Original offline lifecycle startup checkpoint was not observed')
                if (run_dir / "stop-request.json").is_file():
                    report["stop_reason"] = "requested_shutdown"
                    break
                if report.get('training_module'):
                    try:
                        marker = json.loads(module_marker.read_text(encoding='utf-8'))
                        if (args.module_stage >= 8 and marker.get('pid') == process.pid
                                and marker.get('tick_thread') and observer is not None
                                and elapsed >= next_original_heartbeat_sample):
                            next_original_heartbeat_sample = elapsed + 5.
                            try:
                                from original_heartbeat_snapshot import snapshot as heartbeat_snapshot
                                if observer.base is None:
                                    observer.find_main_image()
                                heartbeat = heartbeat_snapshot(observer.read, observer.base, marker['tick_thread'])
                                heartbeat['elapsed_seconds'] = elapsed
                                save(run_dir / 'original-heartbeat.json', heartbeat)
                            except (OSError, ValueError, RuntimeError) as error:
                                save(run_dir / 'original-heartbeat.json', {'error': str(error), 'read_only': True})
                        if marker.get('pid') == process.pid and marker.get('error'):
                            report['stop_reason'] = 'native_module_reported_error'
                            report['native_module_error'] = marker
                            break
                        if marker.get('pid') == process.pid and marker.get('stage') == 'original_training_user_exit_requested':
                            report['stop_reason'] = 'requested_shutdown' if args.network_probe else 'user_f10'
                            break
                    except (OSError, ValueError):
                        pass
                    # Both manual and verifier sessions retain the shared fault
                    # ring even after the GameThread stops publishing JSON.
                    # Only manual sessions use this runner's frame-stale policy.
                    if args.module_stage >= 8:
                        from training_channel import TrainingChannel, ChannelError
                        from original_probe_channel import OriginalProbe
                        if native_channel is None and elapsed >= next_native_probe_open:
                            next_native_probe_open = elapsed + .25
                            try:
                                native_channel = TrainingChannel.open(process.pid,
                                    alive=lambda: process.poll() is None, timeout=.01)
                                native_probe = OriginalProbe(native_channel)
                            except ChannelError:
                                pass
                        if native_probe is not None:
                            try:
                                from native_fault_observation import sample as sample_native_faults
                                faults = sample_native_faults(native_channel.buffer)
                                if faults is not None and faults['published'] != native_fault_published:
                                    native_fault_published = faults['published']
                                    if faults['records']:
                                        try:
                                            from capture_owned_thread import _kernel, _modules, _address_record
                                            modules = _modules(_kernel(), process.pid)
                                            for fault in faults['records']:
                                                fault['instruction'] = _address_record(fault['rip'], modules)
                                        except (OSError, RuntimeError) as error:
                                            faults['module_resolution_error'] = str(error)
                                    fresh_faults = [fault for fault in faults['records']
                                                    if fault['sequence'] not in native_fault_sequences]
                                    if fresh_faults:
                                        with (run_dir/'passive-fault-events.jsonl').open('a', encoding='utf-8') as events:
                                            for fault in fresh_faults:
                                                native_fault_sequences.add(fault['sequence'])
                                                events.write(json.dumps(fault) + '\n')
                                    # This observed acquisition marker also
                                    # occurs in the user's incomplete dump.
                                    # It is a capture trigger, not a root-cause
                                    # classification. Keep the small dump local.
                                    if not report.get('owned_small_dump') and any(
                                            fault.get('instruction', {}).get('module') == 'stub.dll'
                                            and fault.get('instruction', {}).get('rva') == 0x2c4140
                                            for fault in fresh_faults):
                                        try:
                                            from capture_owned_dump import capture as capture_small_dump
                                            report['owned_small_dump'] = capture_small_dump(owned_active_path)
                                        except (OSError, ValueError, RuntimeError) as error:
                                            report['owned_small_dump'] = {'error': str(error)}
                                    save(run_dir/'passive-fault-observations.json', faults)
                                frame = native_probe.read()
                                if frame.time != native_probe_time:
                                    native_probe_time = frame.time
                                    native_probe_changed = time.monotonic()
                                    native_stale_recorded = False
                                elif (args.module_stage == 8 and native_probe_changed is not None
                                      and time.monotonic()-native_probe_changed > 2.5):
                                    if not native_stale_recorded:
                                        from capture_owned_thread import capture_owned_thread
                                        report['early_stale_capture'] = capture_owned_thread(owned_active_path,
                                            run_dir/'stale-game-thread.json', reason='manual_native_frame_stale')
                                        native_stale_recorded = True
                                    # A crash-dump writer suspends the target's
                                    # other threads too. Manual sessions must
                                    # allow it to finish, rather than truncate
                                    # the dump by treating suspension as a hang.
                                    if not args.interactive_training:
                                        report['stop_reason'] = 'native_game_thread_stale'
                                        break
                            except ChannelError:
                                pass
                if world_observer is not None and elapsed >= next_world_sample:
                    next_world_sample = elapsed + .1
                    try:
                        world_record = world_observer.sample()
                        if args.training_module and not report.get("training_module") and world_record.get("pawn_class"):
                            if args.controller_settings and not args.network_probe:
                                from training_control import initial_payload
                                from native_maps import training_maps
                                controller_config = json.loads(args.controller_settings.read_text(encoding='utf-8'))
                                validate_controller_settings(controller_config)
                                if controller_config['map_id'] != str(args.map_number):
                                    raise ValueError('The launch map and WebUI configuration changed during startup')
                                payload = initial_payload(controller_config, training_maps()[controller_config['map_id']])
                                initial_path = ROOT / '.local/native-module' / f'initial-control-{process.pid}.bin'
                                initial_path.write_bytes(payload)
                                report['webui_control'] = {'version': 2, 'configuration_revision': controller_config['revision']}
                            report["game_memory_modified"] = True
                            report['training_module'] = load_selected_training_module(
                                args, process, world_observer.context, selected_network_module)
                            if not args.network_probe and args.module_stage in (3, 4):
                                training_dir = run_dir / "training"
                                training_dir.mkdir()
                                training_host_log = (training_dir / "host.log").open("w", encoding="utf-8")
                                host_args = [sys.executable, str(ROOT / "tools/training_host.py"),
                                             "--pid", str(process.pid), "--run-dir", str(training_dir)]
                                if args.no_training_hud:
                                    host_args.append("--no-hud")
                                training_host = subprocess.Popen(host_args, cwd=ROOT, stdout=training_host_log,
                                    stderr=training_host_log, creationflags=subprocess.CREATE_NO_WINDOW)
                                report["training_host_directory"] = str(training_dir)
                        if not world_observations or world_observations[-1]["world"] != world_record:
                            world_observations.append({"seconds": round(elapsed, 3), "world": world_record})
                            pending = run_dir / "world-observations.pending.json"
                            save(pending, {"samples": world_observations})
                            pending.replace(run_dir / "world-observations.json")
                    except (OSError, ValueError, UnicodeError) as error:
                        report["last_world_observation_error"] = str(error) if isinstance(error, ValueError) else type(error).__name__
                if args.export_saved_config and not report.get("saved_config_archive") and elapsed >= next_archive_sample:
                    next_archive_sample = elapsed + .001
                    try:
                        if observer.base is None:
                            observer.find_main_image()
                        info = observer.saved_config_archive_info()
                        if info["bounded"]:
                            report["saved_config_archive"] = observer.export_saved_config_archive(run_dir / "saved-config.bin")
                    except (OSError, ValueError) as error:
                        report["last_archive_observation_error"] = type(error).__name__
                if elapsed >= next_sample:
                    samples.append({"seconds": round(elapsed, 3),
                                    "runtime_log_bytes": runtime_log.stat().st_size if runtime_log.exists() else None})
                    next_sample += 1
                if runtime_log.exists():
                    if hang_log_marker.scan(runtime_log):
                        report['native_engine_error']='Owned client reported a GameThread hang'
                        if not args.interactive_training:
                            report['stop_reason']='native_game_thread_hang'
                            break
                    if observer is not None and elapsed >= next_config_sample:
                        next_config_sample = elapsed + .1
                        try:
                            record = observer.engine_record()
                            if args.export_core_config and not report.get("core_config_snapshot"):
                                snapshot = CoreSnapshot(observer).capture()
                                save(run_dir / "core-config.json", snapshot)
                                report["core_config_snapshot"] = {"path": str(run_dir / "core-config.json"),
                                    "categories": [item["category"] for item in snapshot["categories"]],
                                    "section_counts": [len(item["file"]["sections"]) for item in snapshot["categories"]]}
                            if args.observe_categories and not report.get("category_names"):
                                report["category_names"] = observer.category_names()
                            if args.export_saved_config and not report.get("saved_config_archive") and record["preloaded_config_archive"]["bounded"]:
                                report["saved_config_archive"] = observer.export_saved_config_archive(run_dir / "saved-config.bin")
                            if not observations or observations[-1]["engine_config"] != record:
                                observations.append({"seconds": round(elapsed, 3), "engine_config": record})
                        except (OSError, ValueError, UnicodeError) as error:
                            report["last_observation_error"] = type(error).__name__
                    if platform_log_marker.scan(runtime_log):
                        if observer is not None:
                            try:
                                report["engine_ini_at_platform_start"] = observer.engine_ini()
                                report["engine_record_at_platform_start"] = observer.engine_record()
                            except (OSError, ValueError, UnicodeError) as error:
                                report["last_observation_error"] = type(error).__name__
                        report["stop_reason"] = "platform_initialization_reached"
                        break
                time.sleep(.001 if args.export_saved_config and not report.get("saved_config_archive") else .02)
            if process.poll() is None:
                report.setdefault("stop_reason", "bounded_deadline")
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
            else:
                report["stop_reason"] = "target_exited"
            report["exit_code"] = process.returncode
            report["elapsed_seconds"] = round(time.monotonic() - started, 3)
            report["log_samples"] = samples
            report["configuration_observations"] = observations
            report["world_observations"] = world_observations
            if report.get("training_module"):
                marker = module_marker
                if marker.is_file():
                    final_state = json.loads(marker.read_text(encoding="utf-8"))
                    if final_state.get("pid") == process.pid:
                        report["training_module"]["final_state"] = final_state
            report["runtime_world_assessment"] = assess_world_samples(world_observations, selected_package)
            report["owned_process_exited"] = process.poll() is not None
            save(run_dir / "launch.json", report)
            if runtime_log.exists():
                analysis = analyze_files(runtime_log, run_dir / "launch.json")
                save(run_dir / "analysis.json", analysis)
                report["runtime_assessment"] = analysis["assessment"]
                report["actual_worlds"] = analysis["runtime"]["world_state_events"]
            report["remaining_game_or_launcher_processes"] = game_processes()
            report["postflight"] = check_preflight(postflight=True)
            report["image_unchanged"] = report["preflight"]["game_sha256"] == report["postflight"]["game_sha256"]
            report["status"] = "recorded"
    except Exception as error:
        report["status"] = "error"
        report["error"] = str(error) if isinstance(error, RuntimeError) else type(error).__name__
        report["error_detail"] = {"type": type(error).__name__, "message": str(error)}
    finally:
        try:
            cleanup_resources(report, native_channel, process, training_host, training_host_log,
                              kernel, image_handle, job_handle, profiles)
            if process is not None:
                lifecycle = report.get('original_game_instance_lifecycle')
                if lifecycle and lifecycle.get('report_path'):
                    try:
                        lifecycle['final_state'] = json.loads(Path(lifecycle['report_path']).read_text(encoding='utf-8'))
                    except (OSError, ValueError):
                        lifecycle['final_state_missing'] = True
                report.setdefault('owned_process_exited', process.poll() is not None)
                if 'remaining_game_or_launcher_processes' not in report:
                    report['remaining_game_or_launcher_processes'] = game_processes()
                if 'postflight' not in report:
                    try:
                        report['postflight'] = check_preflight(postflight=True)
                        report['image_unchanged'] = bool(report.get('preflight', {}).get('game_sha256')) and (
                            report['preflight']['game_sha256'] == report['postflight'].get('game_sha256'))
                    except Exception as postflight_error:
                        report['postflight_error'] = {'type': type(postflight_error).__name__, 'message': str(postflight_error)}
                        report['status'] = 'error'
                if report.get('training_module') and not args.network_probe:
                    captured = {}
                    for name in ('original-player-defaults.json', 'original-player-probe.json',
                                 'original-weapon-probe.json', 'original-hud.json', 'original-simulation.json',
                                 'original-content.json', 'original-enemies.json', 'original-camera.json',
                                 'original-content-services.json', 'original-scoreboard.json', 'original-enemy-weapons.json',
                                 'original-champions.json', 'original-inventory-slots.json', 'original-jett.json',
                                 'original-render.json', 'original-shot-origin-probe.json', 'original-effects.json',
                                 'original-client-lifecycle.json','original-enemy-view.json','original-camera-owners.json',
                                 'original-training.json','original-damage.json','original-damage-tuning.json','original-enemy-respawn.json',
                                 'original-input-bindings.json','original-ability-tuning.json','original-gameplay.json','original-jett-world.json',
                                 'original-jett-events.json','original-jett-sync.json','original-infinite-ammo.json',
                                 'original-ui-input-mode.json','original-ads.json','original-fault-observer.json',
                                 'original-scope.json','original-ads-refresh.json'):
                        source = ROOT / '.local/native-module' / name
                        try:
                            if source.stat().st_mtime >= report.get('start_time', float('inf')):
                                value = json.loads(source.read_text(encoding='utf-8'))
                                save(run_dir / name, value)
                                captured[name] = str(run_dir / name)
                        except (OSError, ValueError):
                            pass
                    report['native_diagnostics'] = captured
        except Exception as cleanup_error:
            report.setdefault('cleanup_errors', []).append({'type': type(cleanup_error).__name__, 'message': str(cleanup_error)})
            report['status'] = 'error'
    save(run_dir / "report.json", report)
    if args.result_file:
        save(args.result_file, report)
    summary = {key: report.get(key) for key in ("status", "stop_reason", "elapsed_seconds",
                                               "actual_worlds", "image_unchanged",
                                               "remaining_game_or_launcher_processes", "error")}
    record = report.get("engine_record_at_platform_start")
    if record:
        summary["engine_config"] = {key: value for key, value in record.items() if key != "source_ini_paths"}
        summary["engine_config"]["source_ini_path_count"] = len(record["source_ini_paths"])
    summary["saved_config_archive"] = report.get("saved_config_archive")
    summary["category_names"] = report.get("category_names")
    summary["core_config_snapshot"] = report.get("core_config_snapshot")
    print(json.dumps(summary, ensure_ascii=False))
    print(str(run_dir / "report.json"))
    return int(report["status"] == "error")


if __name__ == "__main__":
    raise SystemExit(main())
