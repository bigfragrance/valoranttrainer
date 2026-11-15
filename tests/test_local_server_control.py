"""Native shim contract/wire fixtures; native harness supplied separately, not executed."""
from copy import deepcopy
import json
from pathlib import Path
import re
import struct
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from training_control import (BYTES, MAGIC, VERSION, STATE_OFFSET, Channel, initial_payload,
                              pack_configuration, pack_routes)
from training_settings_v2 import DEFAULTS
from local_server_controller import UI_COMMANDS

HEADER = ROOT / 'native/local_server_control.h'
EVIDENCE = ROOT / 'work/local-server-playable/control'


def function_body(source, signature):
    start = source.index(signature)
    if signature.startswith('struct '):
        opening = source.index('{', start)
    else:
        match = re.search(r'\)\s*\{', source[start:])
        if not match:
            raise AssertionError('Missing function definition: ' + signature)
        opening = start + match.end() - 1
    depth = 1
    for at in range(opening + 1, len(source)):
        if source[at] == '{':
            depth += 1
        elif source[at] == '}':
            depth -= 1
            if depth == 0:
                return source[opening + 1:at]
    raise AssertionError('Missing function body: ' + signature)


class ShimBoundaryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = HEADER.read_text(encoding='utf-8')

    def test_shim_has_only_three_parent_hooks_and_read_only_initial_api(self):
        hooks = function_body(self.source, 'struct Hooks')
        self.assertEqual(len(re.findall(r'=nullptr;', hooks)), 3)
        for name in ('dispatch', 'stateJson', 'requestStop'):
            self.assertIn(name, hooks)
        self.assertIn('const training_control::Initial& Initial()', self.source)
        self.assertIn('const State& GetState()', self.source)
        self.assertIn('bool Enabled()', self.source)
        self.assertNotRegex(self.source, r'local_server_match::(?:Start|Initialize|Poll|RegisterHuman)\s*\(')
        self.assertNotIn('CreateProcess', self.source)
        self.assertNotIn('TerminateProcess', self.source)

    def test_strict_loader_does_not_use_compatibility_bytes_zero(self):
        body = function_body(self.source, 'bool LoadInitial(')
        self.assertIn('bytes.QuadPart==static_cast<LONGLONG>(sizeof(candidate))', body)
        for condition in ('candidate.magic!=training_control::Magic', 'candidate.version!=training_control::Version',
                          'candidate.bytes!=sizeof(candidate)', 'local_server_match::ValidInitial(candidate)'):
            self.assertIn(condition, body)
        self.assertIn('FILE_SHARE_READ', body)
        self.assertIn('GENERIC_READ', body)
        self.assertNotIn('GENERIC_WRITE', body)
        self.assertIn('GetLastError()!=ERROR_FILE_NOT_FOUND', body)
        self.assertIn('state.loaded=true;state.enabled=false', body)
        self.assertNotIn('state.ready=true', body)
        self.assertIn('session-initial.bin', self.source)
        self.assertIn('sizeof(training_control::Initial)==7844', self.source)

    def test_channel_open_cannot_promote_ready(self):
        body = function_body(self.source, 'bool Initialize(')
        self.assertIn('training_control::Initialize()', body)
        self.assertIn('state.gameThread=GetCurrentThreadId()', body)
        self.assertIn('PublishWaiting(', body)
        self.assertNotIn('PublishParent(', body)
        self.assertNotIn('state.ready=true', body)

    def test_one_v2_consumer_and_one_ack_implementation(self):
        poll = function_body(self.source, 'bool Poll(')
        self.assertIn('if(!Thread())', poll)
        self.assertEqual(self.source.count('training_control::Poll(Dispatch)'), 1)
        self.assertNotIn('InterlockedExchange', self.source)
        self.assertNotIn('commandSequence', self.source)
        self.assertIn('now-state.lastPublish<100', poll)
        self.assertIn('state.publishedCommands==state.commands', poll)

    def test_role_boundary_matches_existing_pairchannel(self):
        body = function_body(self.source, 'bool ClientUI(')
        expected = {'toggle_buy_menu': 'ToggleBuyMenu', 'toggle_options': 'ToggleOptions',
                    'close_active_ui': 'CloseActiveUI'}
        self.assertEqual(set(expected), set(UI_COMMANDS))
        self.assertEqual(set(re.findall(r'Command::(\w+)', body)), set(expected.values()))
        dispatch = function_body(self.source, 'Result Dispatch(')
        self.assertLess(dispatch.index('ValidPayload('), dispatch.index('hooks.dispatch('))
        self.assertLess(dispatch.index('state.host==ClientUI(command)'), dispatch.index('hooks.dispatch('))
        self.assertIn('Result::Unavailable', dispatch)
        self.assertNotIn('initial.config=', dispatch)
        self.assertNotIn('state.appliedRevision=', dispatch)

    def test_publication_metadata_comes_from_same_parent_snapshot(self):
        body = function_body(self.source, 'bool PublishParent(')
        self.assertIn('hooks.stateJson()', body)
        self.assertIn('reader.Read(publication)', body)
        self.assertIn('state.ready=publication.ready', body)
        self.assertIn('state.appliedRevision=publication.revision', body)
        self.assertIn('state.flags=state.ready?1U:0U', body)
        self.assertIn('training_control::Publish(json.data(),json.size(),state.appliedRevision,state.flags)', body)
        self.assertIn('PublishWaiting(state.stage,true)', body)

    def test_request_stop_is_idempotent_and_does_not_reinterpret_restart(self):
        body = function_body(self.source, 'bool RequestStop(')
        self.assertIn('if(state.stopRequested)return true', body)
        self.assertIn('hooks.requestStop()', body)
        self.assertEqual(body.count('hooks.requestStop()'), 1)
        dispatch = function_body(self.source, 'Result Dispatch(')
        self.assertNotIn('RequestStop(', dispatch)
        self.assertNotIn('Command::RestartMatch', dispatch)

    def test_native_harness_covers_unexecuted_semantic_cases(self):
        harness = (EVIDENCE / 'control_harness.cpp').read_text(encoding='utf-8')
        for label in ('nested ready cannot promote session', 'shim rejects legacy bytes zero',
                      'Pending ACK never advances applied revision or ready',
                      'same sequence never repeats parent action', 'client rejects authority command',
                      'basic probe cannot dispatch or become ready from default settings'):
            self.assertIn(label, harness)
        self.assertNotIn('CreateProcess', harness)


class V2InitialWireTests(unittest.TestCase):
    def test_existing_python_encoder_matches_native_strict_layout(self):
        config = deepcopy(DEFAULTS)
        config['revision'] = 77
        payload = initial_payload(config, {'spawn_anchors': [{'raw_actor_location_cm': [1, 2, 3], 'yaw_deg': 90}]})
        self.assertEqual(len(payload), 7844)
        self.assertEqual(struct.unpack_from('<Q2I', payload), (MAGIC, VERSION, 7844))
        self.assertEqual(payload[16:88], pack_configuration(config))
        self.assertEqual(payload[6752:6816].rstrip(b'\0'), b'duel1')
        self.assertEqual(struct.unpack_from('<I3f', payload, 6816), (1, 1., 2., 3.))
        self.assertEqual(struct.unpack_from('<f', payload, 7588), (90.,))
        routes = pack_routes(config['routes']['duel1'], config['revision'])
        self.assertEqual(payload[88:88 + len(routes)], routes)

    def test_no_sidecar_default_initial_cannot_accidentally_match_wire_header(self):
        # Aggregate defaults have magic/version, but bytes=0 and no map id. The
        # strict shim requires a real sidecar before enabling gameplay dispatch.
        defaults = struct.pack('<Q2I', MAGIC, VERSION, 0) + bytes(7844 - 16)
        self.assertEqual(struct.unpack_from('<I', defaults, 12)[0], 0)
        self.assertNotEqual(defaults, initial_payload(deepcopy(DEFAULTS), {}))


class V2ParentStateWireTests(unittest.TestCase):
    def read_fixture(self, body, revision, ready):
        memory = bytearray(BYTES)
        struct.pack_into('<Q3I', memory, 0, MAGIC, VERSION, BYTES, 71)
        payload = json.dumps(body, separators=(',', ':')).encode('utf-8')
        struct.pack_into('<2I', memory, 56, 2, len(payload))
        struct.pack_into('<Q2I', memory, 64, 123456, 1 if ready else 0, revision)
        memory[STATE_OFFSET:STATE_OFFSET + len(payload)] = payload
        channel = Channel(71, memory=memory)
        self.addCleanup(channel.close)
        return channel.state()

    def test_channel_exposes_same_applied_revision_and_real_heartbeat(self):
        body = {'ready': True, 'applied_revision': 77, 'error': None, 'map_id': 'duel1',
                'infinite_ammo': True, 'players': [], 'routes': [], 'effects': [], 'weapon': {'ready': False}}
        result = self.read_fixture(body, 77, True)
        self.assertTrue(result['ready'])
        self.assertEqual(result['applied_revision'], 77)
        self.assertEqual(result['heartbeat_ms'], 123456)
        self.assertFalse(result['weapon']['ready'])

    def test_waiting_parent_snapshot_does_not_enable_webui_startup_actions(self):
        body = {'ready': False, 'applied_revision': 0, 'error': None, 'map_id': '', 'players': [],
                'effects': [], 'weapon': {'ready': False}}
        result = self.read_fixture(body, 0, False)
        self.assertFalse(result['ready'])
        self.assertEqual(result['applied_revision'], 0)
        self.assertEqual(result['effects'], [])


if __name__ == '__main__':
    unittest.main()
