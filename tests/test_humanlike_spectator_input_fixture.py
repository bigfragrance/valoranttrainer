"""Regress the actual bridge nesting, distinct from the client view record."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import humanlike_spectator_input_fixture as fixture


def native_client():
    receipts=[]
    for direction in range(2):
        start=1000+direction*700
        source,target=(3,2) if direction==0 else (2,3)
        receipts.append({
            'action':'SecondaryTriggerDown' if direction else 'PrimaryTriggerDown',
            'direction':direction,'bind_index':0,'down_calls':1,'up_calls':1,
            'request_before':direction,'request_after':direction+1,'ack_before':direction,'ack_after':direction+1,
            'pressed_at_ms':start,'completed_at_ms':start+150,'camera_sample_at_ms':start+150,
            'from_id':source,'to_id':target,'from_pawn_weak':[source*10,100+source],
            'to_pawn_weak':[target*10,100+target],'modifiers':[0,0,0,0,0],'tap_hold_type':0,
            **{key:True for key in ('down_dispatched','up_dispatched','release_observed',
                                   'request_observed','ack_observed','camera_observed')}})
    return {'native_integration_test':{
        'scenario':15,'nonce':99,'configured':True,'cli_matched':True,'armed':True,
        'finished':True,'failed':False,'error':0,'actions_completed':True,
        'spectator_input':{'bound':True,'metadata_verified':True,'release_unavailable':False,
            'skipped_or_not_observed':False,'key_source_rva':'0x708d250','shared_release_rva':'0x18a31e0',
            'direct_request_calls':0,'camera_updates':0,'target_writes':0,'receipts':receipts}}}


def native_rows():
    rows=[]
    for step in range(3):
        client=native_client()
        for receipt in client['native_integration_test']['spectator_input']['receipts'][step:]:
            receipt['pressed_at_ms']=0
            receipt['camera_observed']=False
        authority={'original_server_calls':1+step,'server_selections':1+step,'player_input_requests':step,
                   'last_direction':max(0,step-1),'server_policy_installed':True,'server_policy_profiles':True}
        rows.append({'host':{'session_runtime':{'spectator_authority':authority}},'client':client})
    return rows


class NativeSpectatorInputCorrelationTests(unittest.TestCase):
    def test_original_native_bridge_shape_has_two_exact_windows(self):
        result=fixture.correlate(native_rows(),99,native_client())
        self.assertTrue(result['client_key_ack_camera_verified'])
        self.assertTrue(result['host_rpc_counter_windows_verified'])
        for window in result['host_rpc_counter_windows']:
            self.assertEqual(window['deltas'],dict.fromkeys(
                ('original_server_calls','server_selections','player_input_requests'),1))
        self.assertFalse(result['physical_os_input_verified'])
        self.assertFalse(result['per_packet_rpc_causality_verified'])

    def test_client_view_key_cannot_stand_in_for_host_authority(self):
        rows=native_rows()
        for row in rows:
            row['host']={'spectator':row['host']['session_runtime']['spectator_authority']}
        self.assertFalse(fixture.correlate(rows,99)['host_rpc_counter_windows_verified'])

    def test_top_level_or_client_authority_cannot_stand_in(self):
        for misplaced in ('top_level','client'):
            rows=native_rows()
            for row in rows:
                authority=row['host'].pop('session_runtime')['spectator_authority']
                if misplaced=='top_level':row['host']['spectator_authority']=authority
                else:row['client']['session_runtime']={'spectator_authority':authority}
            self.assertFalse(fixture.correlate(rows,99)['host_rpc_counter_windows_verified'])

    def test_unavailable_or_malformed_runtime_never_guesses(self):
        for value in (None,[],3,'unavailable',{}):
            rows=native_rows()
            rows[1]['host']['session_runtime']=value
            self.assertFalse(fixture.correlate(rows,99)['host_rpc_counter_windows_verified'])

    def test_interference_is_not_accepted_as_one_click(self):
        rows=native_rows()
        rows[1]['host']['session_runtime']['spectator_authority']['player_input_requests']=2
        self.assertFalse(fixture.correlate(rows,99)['host_rpc_counter_windows_verified'])

    def test_actual_direction_and_policy_proof_remain_required(self):
        for field,value in (('last_direction',1),('server_policy_profiles',False),
                            ('original_server_calls','2')):
            rows=native_rows()
            rows[1]['host']['session_runtime']['spectator_authority'][field]=value
            self.assertFalse(fixture.correlate(rows,99)['host_rpc_counter_windows_verified'])

    def test_nonce_and_missing_before_sample_remain_required(self):
        self.assertFalse(fixture.correlate(native_rows(),100)['host_rpc_counter_windows_verified'])
        self.assertFalse(fixture.correlate(native_rows()[1:],99)['host_rpc_counter_windows_verified'])


if __name__=='__main__':
    unittest.main()
