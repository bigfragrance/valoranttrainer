"""Production Channel/send_bundle on an owned bytearray, never a game MMF.

The fixture acknowledges writes at the production command publication point.
These ACKs prove Python transport wiring only, never native consumption.
"""
import json
import struct
import tempfile
from pathlib import Path
import unittest

import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))

import human_bot_delivery as delivery
import human_bot_integration as integration
import human_bot_pack as wire
import training_control as control

PID=424242

def memory():
    result=bytearray(control.BYTES)
    struct.pack_into('<Q3I',result,0,control.MAGIC,control.VERSION,control.BYTES,PID)
    return result

class SelfConsumerMemory(bytearray):
    """Own synchronous fixture; no process, mapping, registry, or method patch."""
    def __init__(self,bundle):
        super().__init__(memory());self.bundle=bundle;self.received=[]

    def __setitem__(self,key,value):
        super().__setitem__(key,value)
        if not isinstance(key,slice) or (key.start,key.stop)!=(32,36):return
        sequence=struct.unpack_from('<I',self,32)[0]
        if sequence!=len(self.received)+1:raise AssertionError('command sequence not serial')
        prior=struct.unpack_from('<I',self,48)[0]
        if prior!=sequence-1:raise AssertionError('prior command was overwritten without ACK')
        command,size=struct.unpack_from('<2I',self,36)
        payload=bytes(self[control.COMMAND_OFFSET:control.COMMAND_OFFSET+size])
        self.received.append((sequence,command,payload))
        expected=(*self.bundle.stages(),self.bundle.commit())
        if command!=17 or payload!=expected[sequence-1]:raise AssertionError('wrong command17 frame payload/order')
        header=wire.HEADER.unpack_from(payload)
        operation=header[4]
        code=1 if operation==wire.STAGE else 0
        if operation not in (wire.STAGE,wire.COMMIT):raise AssertionError('unexpected operation')
        struct.pack_into('<2I',self,48,sequence,code)
        if operation==wire.COMMIT:
            state={'human_bots':{'schema_digest':integration.human.SCHEMA_DIGEST.hex(),
                   'transport_applied_revision':self.bundle.revision,
                   'transport_fingerprint':f'{self.bundle.checksum:016x}',
                   'consumer_effective_known':False,'configuration_effective':False}}
            encoded=json.dumps(state).encode('utf-8')
            struct.pack_into('<2I',self,56,1,len(encoded))
            super().__setitem__(slice(control.STATE_OFFSET,control.STATE_OFFSET+len(encoded)),encoded)
            struct.pack_into('<I',self,76,self.bundle.revision)
            struct.pack_into('<I',self,56,2)

class ProductionCommand17Tests(unittest.TestCase):
    def bundle(self):
        with tempfile.TemporaryDirectory() as folder:
            settings=integration.HumanSettings(Path(folder)/'unused.json')
            # Population and its real fixed entries are one atomic config edit.
            roster=settings.read()['human_bots']['bot_roster'][:2]
            config=settings.propose({'total_players':3,'human_bots':{'enabled':True,'bot_roster':roster}},0)
        return integration.configuration_bundle(config)

    def test_real_registry_keeps_original_ids_and_registers_human_bots_17(self):
        names=('configure','routes','weapon','skin','effect','restart_match','validate_routes',
               'toggle_buy_menu','buy_weapon','save_skin_selection','validate_skin_catalog',
               'resolve_map_point','toggle_options','close_active_ui','test_player_near_bot','test_audio_registration')
        self.assertEqual([control.COMMANDS[name] for name in names],list(range(1,17)))
        self.assertEqual(control.COMMANDS['human_bots'],17)
        self.assertEqual(len(set(control.COMMANDS.values())),len(control.COMMANDS))

    def test_real_channel_and_delivery_publish_all_command17_stages_then_commit_serially(self):
        bundle=self.bundle();owned=SelfConsumerMemory(bundle)
        with self.subTest(stages=len(bundle.stages())):
            self.assertGreater(len(bundle.stages()),1)
        channel=control.Channel(PID,memory=owned)
        try:result=delivery.send_bundle(channel,bundle,ack_timeout=.1,effective_timeout=0)
        finally:channel.close()
        expected=(*bundle.stages(),bundle.commit())
        self.assertEqual(owned.received,[(n+1,17,frame) for n,frame in enumerate(expected)])
        self.assertEqual([wire.HEADER.unpack_from(row[2])[4] for row in owned.received],[wire.STAGE]*len(bundle.stages())+[wire.COMMIT])
        self.assertEqual([reply['sequence'] for reply in result['replies']],list(range(1,len(expected)+1)))
        self.assertTrue(all(reply['acknowledged'] for reply in result['replies']))
        self.assertEqual([reply['code'] for reply in result['replies']],[1]*len(bundle.stages())+[0])
        self.assertEqual(result['status'],'consumer_pending')
        self.assertTrue(result['transport_applied'])
        self.assertFalse(result['consumer_effective'])
        self.assertFalse(result['behavior_execution_verified'])

    def test_unknown_command_is_rejected_before_any_bytearray_write(self):
        owned=memory();channel=control.Channel(PID,memory=owned);before=bytes(owned)
        try:
            with self.assertRaisesRegex(ValueError,'Invalid game command'):channel.send('human_bots_unknown',b'bad')
        finally:channel.close()
        self.assertEqual(bytes(owned),before)

    def test_unacknowledged_command17_stops_bundle_and_cannot_be_overwritten(self):
        bundle=self.bundle();owned=memory();channel=control.Channel(PID,memory=owned)
        try:
            result=delivery.send_bundle(channel,bundle,ack_timeout=.001,effective_timeout=0)
            after=bytes(owned)
            with self.assertRaisesRegex(RuntimeError,'previous game command is still pending'):
                channel.send('human_bots',bundle.commit(),timeout=0)
        finally:channel.close()
        self.assertEqual(result['status'],'command_pending');self.assertEqual(len(result['replies']),1)
        self.assertFalse(result['replies'][0]['acknowledged']);self.assertFalse(result['transport_applied'])
        self.assertEqual(struct.unpack_from('<3I',owned,32),(1,17,len(bundle.stages()[0])))
        self.assertEqual(bytes(owned[control.COMMAND_OFFSET:control.COMMAND_OFFSET+len(bundle.stages()[0])]),bundle.stages()[0])
        self.assertEqual(bytes(owned),after)
