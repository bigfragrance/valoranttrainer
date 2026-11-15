"""No games/network: exercise paired ownership and isolated transport config."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import Mock,patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import local_network_probe as probe
from run_local_native_network import assess_completion


class PairPreflightTests(unittest.TestCase):
    def result(self):
        return {'ready':False,'expected_image_matches':True,
                'existing_game_or_launcher_processes':True,
                'blocking_processes':[{'pid':41,'executable_path':'owned'}],
                'separate_installation_processes':[{'pid':99,'executable_path':'unrelated'}],
                'saved_login_present':False,'saved_authorization_present':False,
                'matching_saved_credential_present':False}

    def check(self,result):
        peer={'pid':41,'network_probe_identity':{'creation_filetime':123}}
        with patch.object(probe,'preflight',return_value=result),patch.object(probe,'validate_peer',return_value=peer):
            return probe.pair_preflight({},'owned.active.json','proof')

    def test_only_matching_owned_host_is_exempted(self):
        result=self.check(self.result())
        self.assertTrue(result['ready'])
        self.assertEqual(result['blocking_processes'],[])
        self.assertEqual(result['separate_installation_processes'][0]['pid'],99)

    def test_another_target_process_remains_blocking(self):
        result=self.result();result['blocking_processes'].append({'pid':42})
        result=self.check(result)
        self.assertFalse(result['ready'])
        self.assertEqual(result['blocking_processes'],[{'pid':42}])

    def test_exited_peer_postflight_keeps_inventory_and_rejects_pid_reuse(self):
        peer={'pid':41,'_peer_exited':True}
        result=self.result();result['blocking_processes']=[];result['ready']=True
        with patch.object(probe,'preflight',return_value=result), patch.object(probe,'validate_peer',return_value=peer):
            actual=probe.pair_preflight({},'host','proof',allow_exited=True)
            self.assertTrue(actual['ready'])
            self.assertEqual(actual['paired_owned_host_exited']['pid'],41)
            result['blocking_processes']=[{'pid':41}]
            with self.assertRaisesRegex(RuntimeError,'still present'):
                probe.pair_preflight({},'host','proof',allow_exited=True)

    def test_account_and_hash_gates_remain_effective(self):
        for key in ('saved_login_present','saved_authorization_present','matching_saved_credential_present','expected_image_matches'):
            with self.subTest(key=key):
                result=self.result();result[key]=key!='expected_image_matches'
                self.assertFalse(self.check(result)['ready'])

    def test_missing_peer_in_inventory_is_rejected(self):
        result=self.result();result['blocking_processes']=[{'pid':42}]
        with self.assertRaisesRegex(RuntimeError,'inventory'):self.check(result)


class PeerIdentityTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name).resolve();(self.root/'.local').mkdir()
        self.image=self.root/'fixture.exe'
        (self.root/'project.json').write_text('{}',encoding='utf-8')
        self.path=self.root/'.local/host.active.json'
        self.active={'pid':41,'network_probe':{'role':'host','proof_id':'proof'},
                     'network_probe_identity':{'pid':41,'creation_filetime':123,'image_sha256':probe.PIN,
                         'executable':str(self.image),'module_sha256':'module',
                         'published_from_original_popen_handle':True}}
        for target,value in (('ROOT',self.root),('verified_build',Mock(return_value={'module_sha256':'module'})),
                             ('client_executable',Mock(return_value=self.image))):
            p=patch.object(probe,target,value);p.start();self.addCleanup(p.stop)

    def save(self):self.path.write_text(json.dumps(self.active),encoding='utf-8')

    def test_cross_proof_peer_is_rejected(self):
        self.save()
        with self.assertRaisesRegex(RuntimeError,'matching runner-owned'):
            probe.validate_peer(self.path,'different-proof')

    def test_pid_reuse_is_rejected_and_handle_closed(self):
        self.save();kernel=Mock();kernel.OpenProcess.return_value=11;kernel.WaitForSingleObject.return_value=0x102
        with patch('capture_owned_thread._kernel',return_value=kernel),patch('capture_owned_thread._creation',return_value=124):
            with self.assertRaisesRegex(RuntimeError,'identity changed'):probe.validate_peer(self.path,'proof')
        kernel.CloseHandle.assert_called_once_with(11)

    def test_exited_peer_requires_matching_identity_and_postflight_opt_in(self):
        self.save();kernel=Mock();kernel.OpenProcess.return_value=11;kernel.WaitForSingleObject.return_value=0
        with patch('capture_owned_thread._kernel',return_value=kernel), patch('capture_owned_thread._creation',return_value=123), patch('capture_owned_thread._image',return_value=self.image):
            with self.assertRaisesRegex(RuntimeError, 'identity changed'):
                probe.validate_peer(self.path,'proof')
            self.assertTrue(probe.validate_peer(self.path,'proof',allow_exited=True)['_peer_exited'])
        kernel.CloseHandle.assert_called_with(11)

    def test_exited_reused_pid_remains_rejected(self):
        self.save();kernel=Mock();kernel.OpenProcess.return_value=11;kernel.WaitForSingleObject.return_value=0
        with patch('capture_owned_thread._kernel',return_value=kernel), patch('capture_owned_thread._creation',return_value=124):
            with self.assertRaisesRegex(RuntimeError,'identity changed'):
                probe.validate_peer(self.path,'proof',allow_exited=True)

    def test_paths_cannot_escape_project_local(self):
        with self.assertRaises(ValueError):probe.local_path(self.root/'.local/../../outside.json')


class TransportBootstrapTests(unittest.TestCase):
    def test_ip_selection_preserves_other_drivers_and_source_hierarchy(self):
        demo=['NetDriverDefinitions','(DefName="DemoNetDriver",DriverClassName="Demo")']
        game=['NetDriverDefinitions','(DefName="GameNetDriver",DriverClassName="Ares")']
        leaf={'sections':[['/Script/Engine.Engine',{'pairs':[game.copy(),copy.deepcopy(demo)],'array_of_struct_keys':[]}]],'source_config':None}
        top=copy.deepcopy(leaf);top['source_config']=copy.deepcopy(leaf)
        snapshot={'categories':[{'file':top}]}
        self.assertEqual(probe.configure_ip_transport(snapshot),2)
        for file in (top,top['source_config']):
            pairs=file['sections'][0][1]['pairs']
            self.assertIn('/Script/OnlineSubsystemUtils.IpNetDriver',pairs[0][1])
            self.assertEqual(pairs[1],demo)
            self.assertEqual(next(s for n,s in file['sections'] if n=='/Script/OnlineSubsystemUtils.IpNetDriver')['pairs'],
                             [['NetConnectionClassName','/Script/OnlineSubsystemUtils.IpConnection']])

    def test_missing_original_driver_definition_fails(self):
        with self.assertRaises(ValueError):probe.configure_ip_transport({'categories':[{'file':{'sections':[]}}]})


class NativeJoinEvidenceTests(unittest.TestCase):
    def sample(self):
        return {'host':{'host_connections':[{'state':3,'login_state':3,'owner_links_verified':True,'controller_role':3}]},
                'client':{'connection_state':3,'own_local_links_verified':True,
                          'local_controller_is_connection_owner':True,'local_controller_role':2,
                          'current_world':'20000','initial_world':'10000','pending_netgame':'0',
                          'world_authority_game_mode':'0','local_player_state':'0','local_pawn':'0','local_pawn_role':0}}

    def test_control_join_does_not_claim_pawn_or_remote_replication(self):
        result=probe.analyze_connection_samples([self.sample()])
        self.assertTrue(result['native_control_join_proven'])
        self.assertFalse(result['replicated_own_state_and_pawn_proven'])
        self.assertFalse(result['remote_character_replication_proven'])

    def test_local_temporary_world_does_not_pass_join(self):
        row=self.sample();row['client']['current_world']=row['client']['initial_world']
        self.assertFalse(probe.analyze_connection_samples([row])['native_control_join_proven'])

    def test_socket_open_without_native_server_login_does_not_pass(self):
        row=self.sample();row['host']['host_connections'][0]['login_state']=0
        self.assertFalse(probe.analyze_connection_samples([row])['native_control_join_proven'])

    def test_unowned_controller_does_not_pass(self):
        row=self.sample();row['client']['local_controller_is_connection_owner']=False
        self.assertFalse(probe.analyze_connection_samples([row])['native_control_join_proven'])

    def movement_samples(self,client_moves):
        rows=[]
        for i in range(3):
            row=self.sample()
            row['host'].update(remote_pawn='30000',remote_ai_is_local=True,remote_pawn_is_local=True,
                               remote_move_submissions=i*100,server_remote_position=[i*100,0,400],
                               server_remote_sample_at_ms=1000+i*100)
            row['client'].update(client_remote_count=1,client_remote_position=[i*100 if client_moves else 0,0,400],
                                 client_remote_sample_at_ms=1000+i*100)
            rows.append(row)
        return rows

    def test_server_motion_alone_never_proves_client_motion(self):
        result=probe.analyze_connection_samples(self.movement_samples(False))
        self.assertTrue(result['remote_character_replication_proven'])
        self.assertFalse(result['server_ai_movement_replicated_proven'])

    def test_remote_ai_motion_is_distinct_from_own_input_authority(self):
        result=probe.analyze_connection_samples(self.movement_samples(True))
        self.assertTrue(result['server_ai_movement_replicated_proven'])
        self.assertFalse(result['authority_movement_proven'])

    def test_moving_in_an_unrelated_place_does_not_pass(self):
        rows=self.movement_samples(True)
        for row in rows:row['client']['client_remote_position'][0]+=10000
        self.assertFalse(probe.analyze_connection_samples(rows)['server_ai_movement_replicated_proven'])


class ProofCompletionTests(unittest.TestCase):
    def complete(self):
        result={'owned_process_exited':True,'stop_reason':'requested_shutdown',
                'image_unchanged':True,'postflight':{'ready':True}}
        return {'observation_completed':True,'host_cleanup':{'cleaned':True},
                'accepted_playable_build_unchanged':True,
                'host_runner_exit_code':0,'client_runner_exit_code':0,
                'host_critical_errors':0,'client_critical_errors':0,
                'host_result':copy.deepcopy(result),'client_result':copy.deepcopy(result)}

    def test_complete_run_passes(self):
        self.assertTrue(assess_completion(self.complete())['passed'])

    def test_short_observation_fails(self):
        report=self.complete();report['observation_completed']=False
        self.assertFalse(assess_completion(report)['passed'])

    def test_late_native_exit_fails_after_successful_join(self):
        report=self.complete();report['client_result']['stop_reason']='process_exited'
        self.assertFalse(assess_completion(report)['passed'])

    def test_cleanup_failure_fails(self):
        report=self.complete();report['host_cleanup']['cleaned']=False
        self.assertFalse(assess_completion(report)['passed'])

    def test_critical_error_or_missing_log_fails(self):
        for count in (1,None):
            with self.subTest(count=count):
                report=self.complete();report['client_critical_errors']=count
                self.assertFalse(assess_completion(report)['passed'])

    def test_build_change_or_postflight_failure_fails(self):
        report=self.complete();report['accepted_playable_build_unchanged']=False
        self.assertFalse(assess_completion(report)['passed'])
        report=self.complete();report['host_result']['postflight']['ready']=False
        self.assertFalse(assess_completion(report)['passed'])


if __name__=='__main__':unittest.main()
