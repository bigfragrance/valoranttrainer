"""Explicit Scenario14 original-spawn tests. No persistent settings are written."""
from __future__ import annotations
import copy
import hashlib
import json
import math
from pathlib import Path
import secrets
import struct

import humanlike_settings as human
import human_bot_pack as wire
from human_bot_integration import validate_config, configuration_bundle
from export_humanlike_hold_positions import generate as generate_authored

ROOT=next(p for p in Path(__file__).resolve().parents if (p/'project.json').is_file())
SOURCE=ROOT/'profiles/humanlike-hold-positions.json'
CASES=('unconventional-1302','regular-1301','unconventional-1303','regular-1304')
# Fixture-only proposals. Historical geometry is provenance, never current proof.
TEST_ONLY_PROPOSALS={1303: {'id': 1303,
        'map_id': 'ascent',
        'name': 'test-only scene007 own-view geometry rerun',
        'enabled': False,
        'classification': 'unconventional',
        'range': None,
        'cover': [-2710.222656, -3945.100586, 102.15004],
        'position': [-2417.276367, -4374.730957, 102.15004],
        'retreat': [-2710.222656, -3945.100586, 102.15004],
        'focus': [-2587.116943, -3861.160156, 276.150024],
        'confidence': 0.5,
        'risk': None,
        'flank_risk': None,
        'exposed_angles': None,
        'human_spawn_proposal': [4720.668457, -4450.544434, 402.298492],
        'human_spawn_source': {'test_only_control': 'distant human anchor for an uninterrupted physical hold test; original vision/FOV/LOS unchanged',
                               'source_annotation_id': 1304, 'source_row0': 637, 'source_pass': 45413,
                               'document_sha256': '61b329e3e5ec160c40b8d24dd6a4b378dafffb6d0c14ab2677c5524ec93861d2',
                               'new_scene_Supported_observed': False},
        'source': {'document_sha256': 'f8655cb30416ba453a781f10c4d2539157e5cddb61bc105afa7c624518cf6f99',
                   'module_sha256': 'ca01adcf71b8b2a1bf1f5244e731e763cf1630a45da7dc589a61892217bd92eb',
                   'row0': 319,
                   'pass': 22165,
                   'life': {'stable_bot_id': 2, 'epoch': 11, 'token': 1843004826538584},
                   'raw_row_sha256': '8b0a655e093a1323da098752abeaf42dd8a9b2f83310d72313450646be7e1eeb',
                   'geometry_file_sha256': '9eba86ef9497e317ef68b1e29f1348501a3cb2355c2a2f95675f1c0cb139ea3c',
                   'kind': 'same-life local geometry positive; must rerun current OriginalPath, ground, '
                           'capsule and LOS',
                   'historical_capsule_radius': 42.0,
                   'historical_capsule_half_height': 98.0,
                   'historical_eye_height': 173.99998399999998,
                   'source_not_global_map_coverage': True,
                   'test_only': True,
                   'new_scene_native_preflight_observed': False}},
 1304: {'id': 1304,
        'map_id': 'ascent',
        'name': 'test-only scene007 independent regular geometry rerun',
        'enabled': False,
        'classification': 'conventional',
        'range': None,
        'cover': [4720.668457, -4450.544434, 402.298492],
        'position': [4448.901367, -4893.875977, 402.298492],
        'retreat': [4720.668457, -4450.544434, 402.298492],
        'focus': [4593.637207, -4372.672852, 576.298462],
        'confidence': 0.5,
        'risk': None,
        'flank_risk': None,
        'exposed_angles': None,
        'human_spawn_proposal': [-2710.222656, -3945.100586, 102.15004],
        'source': {'document_sha256': '61b329e3e5ec160c40b8d24dd6a4b378dafffb6d0c14ab2677c5524ec93861d2',
                   'module_sha256': 'ca01adcf71b8b2a1bf1f5244e731e763cf1630a45da7dc589a61892217bd92eb',
                   'row0': 637,
                   'pass': 45413,
                   'life': {'stable_bot_id': 5, 'epoch': 11, 'token': 1946766941458634},
                   'raw_row_sha256': '539ee36eef495db7a59374f8b118d1ab7760fa22471800e0103fbbb8b99c9c18',
                   'closed_observer_sha256': '9115f013b1dca887c53bc32e7506f590fd70c7931dbab19505e587322c17f436',
                   'kind': 'independent same-life own-view local geometry positive; must rerun current '
                           'OriginalPath, ground, capsule and LOS',
                   'historical_capsule_radius': 42.0,
                   'historical_capsule_half_height': 98.0,
                   'historical_eye_height': 173.99996999999996,
                   'source_not_global_map_coverage': True,
                   'test_only': True,
                   'new_scene_native_preflight_observed': False}}}
MAGIC=0x3431444c4f48544c
CONTRACT=struct.Struct('<Q8I4Q2I10d96s')
assert CONTRACT.size==256

def canonical(value):
    return json.dumps(value,ensure_ascii=False,sort_keys=True,separators=(',',':'),allow_nan=False).encode('utf-8')

def add_arguments(parser):
    parser.add_argument('--hold-case',choices=CASES,help='Required explicit Scenario14 authored map-hold physical arrival/dwell case')
    parser.add_argument('--hold-variant',choices=('stationary-prefire',),help='Explicit test-only Scenario14 1303/1304 stationary authored-focus original single-shot prefire')

def prepare_request(args,config,*,nonce=None,source=None):
    selected=getattr(args,'hold_case',None)
    variant=getattr(args,'hold_variant',None)
    if variant is not None and (variant!='stationary-prefire' or args.scenario!='map-hold-fixture'
                                or selected not in ('unconventional-1303','regular-1304')):
        raise ValueError('stationary-prefire requires explicit Scenario14 test-only 1303/1304')
    if selected is None and args.scenario!='map-hold-fixture':return None,config
    if (selected not in CASES or args.scenario!='map-hold-fixture' or not args.interactive or not args.original_host_mode
            or not 45<=args.seconds<=120 or getattr(args,'peek_fixture',None)):
        raise ValueError('Scenario14 requires explicit --scenario map-hold-fixture --hold-case, interactive original host and 45..120 seconds')
    source=json.loads(SOURCE.read_text(encoding='utf-8')) if source is None else copy.deepcopy(source)
    generated,source_metadata=generate_authored(source)
    if (ROOT/'native/humanlike_bot_space_authored.h').read_text(encoding='utf-8-sig').replace('\r\n','\n')!=generated:
        raise ValueError('Authored hold source does not match the current generated native header')
    annotation=int(selected.rsplit('-',1)[1]);row=copy.deepcopy(TEST_ONLY_PROPOSALS[annotation]) if annotation in TEST_ONLY_PROPOSALS else next(p for p in source['positions'] if p['id']==annotation)
    if row['enabled'] is not False or any(row.get(k) is not None for k in ('risk','flank_risk','exposed_angles','range')):
        raise ValueError('Map-hold fixture requires the disabled, explicitly unknown authored proposal')
    original_settings=config.get('human_bots',human.defaults())
    zero=human.resolve(original_settings,3,0);one=list(human.resolve(original_settings,3,1))
    overrides={'HoldConventionalWeight':100 if row['classification']=='conventional' else 0,'HoldUnconventionalWeight':100 if row['classification']=='unconventional' else 0,
               'DefendAreaWeight':100,'RoamWeight':0,'AdvanceWeight':0,'FireEnabled':0,'CoverMinimumConfidence':row['confidence'],
               'HoldDurationMs':2500,'PatienceMs':10000,'TimingMistakeProbability':0,'SoundApproachWeight':0,'ThirdPartyJoinWeight':0}
    # Explicit high-exposure mechanism test; native measured risk is unchanged.
    if annotation in (1303,1304):overrides['CoverMaximumExposedAngles']=4
    for name in ('Wide','Short','SlowCreep','Jump','Crouch','Poppin','Shoulder','InfoRepeek'):overrides[name+'Enabled']=0
    variant_overrides={};variant_changes={}
    if variant=='stationary-prefire':
        variant_overrides={'FireEnabled':1,'FirePrefireProbability':1,'FirePrefireConfidence':.5,
                           'FirePrefireMaxAgeMs':2000,'PreaimOmitProbability':1,'SingleShots':1}
        for distance in ('Near','Mid','Far'):
            for context in ('Steady','Moving','UnderFire','Partial'):
                for mode in ('Single','Double','Burst','Spray','MovingClose'):
                    variant_overrides[f'FireVandal{distance}{context}{mode}Weight']=100 if mode=='Single' else 0
        variant_changes={enum:{'before':human.f32(overrides.get(enum,one[human.BY_ENUM[enum]['id']])),
                               'fixture':human.f32(value)} for enum,value in variant_overrides.items()}
        overrides.update(variant_overrides)
    for enum,value in overrides.items():one[human.BY_ENUM[enum]['id']]=human.f32(value)
    human.validate_profile(one)
    settings=human.defaults();settings['enabled']=True
    settings['global_overrides']={p['key']:zero[p['id']] for p in human.PARAMS if p['editable']}
    settings['individual_overrides']={'1':{p['key']:one[p['id']] for p in human.PARAMS if p['scope']=='bot' and p['editable']}}
    fixture=copy.deepcopy(config);fixture.update(schema_version=3,total_players=2,seed=3,revision=config['revision']+1,map_id='ascent',
        mode='deathmatch',invincible=False,infinite_ammo=False,paused=False,human_bots=human.validate(settings))
    fixture=validate_config(fixture);bundle=configuration_bundle(fixture)
    assert wire._VALUES.unpack_from(bundle.records[0],16)==tuple(zero)
    assert wire._VALUES.unpack_from(bundle.records[1],16)==tuple(one)
    sut=list(row['cover'])
    if annotation==1301:
        dx,dy=sut[0]-row['position'][0],sut[1]-row['position'][1];length=math.hypot(dx,dy)
        sut[0]+=100*dx/length;sut[1]+=100*dy/length
    sut=list(map(human.f32,sut))
    other=next(p for p in source['positions'] if p['id']!=annotation)
    human_spawn=list(map(human.f32,row['human_spawn_proposal'] if annotation in TEST_ONLY_PROPOSALS else other['cover']))
    sut_yaw=human.f32(math.degrees(math.atan2(row['position'][1]-sut[1],row['position'][0]-sut[0])))
    manifest={'scenario':14,'case':selected,'annotation_id':annotation,'annotation_revision':source['revision'],
              'installed_annotation_revision':source['revision']+1,'author_classification':row['classification'],
              'classification_scope':'project_author_test_label_only_not_player_heatmap','author_confidence':row['confidence'],
              'proposal':row,'source_sha256':hashlib.sha256(canonical(source)).hexdigest(),'sut_spawn_feet':sut,'human_spawn_feet':human_spawn,
              'source_resolved_sha256':source_metadata['source_resolved_sha256'],
              'test_only_proposal':annotation in TEST_ONLY_PROPOSALS,'production_map_annotation_added':False,
              'same_life_local_proof_source_only':row.get('source') if annotation in TEST_ONLY_PROPOSALS else None,
              'sut_yaw':sut_yaw,'arrival_cm':one[human.BY_ENUM['MoveArrivalCm']['id']],'dwell_seconds':2.5,'case_seconds':40,
              'seed':3,'revision':fixture['revision'],'explicit_profile_overrides':overrides,
              'explicit_session_overrides':{k:{'before':config.get(k),'fixture':fixture[k]} for k in ('schema_version','total_players','seed','revision','map_id','mode','invincible','infinite_ammo','paused') if config.get(k)!=fixture[k]},
              'native_geometry_preflight_observed':False,'persistent_settings_written':False,'ordinary_human0_profile_preserved':True,
              'shared_query_budgets_increased':False,'per_intent_consumption_known':False,'competing_point_preference_verified':False}
    if variant is not None:
        manifest.update(hold_variant=variant,hold_variant_scope='explicit_stationary_AuthoredHold_original_prefire_test_not_default',
                        variant_profile_overrides=variant_overrides,variant_profile_changes=variant_changes)
    request={'nonce':nonce if nonce is not None else secrets.randbits(64) or 1,'manifest':manifest,
             'map_info':{'spawn_anchors':[{'position':sut,'yaw_deg':sut_yaw},{'position':human_spawn,'yaw_deg':0.0}]}}
    if type(request['nonce']) is not int or not 0<request['nonce']<2**64:raise ValueError('Map-hold nonce requires nonzero uint64')
    return request,fixture

def marker(nonce):
    if type(nonce) is not int or not 0<nonce<2**64:raise ValueError('Map-hold nonce requires nonzero uint64')
    return struct.pack('<Q4IQ8I',0x31545345544e534c,1,64,1,14,nonce,500,500,10000,45000,0,0,0,0)

def validate_native_nonce(nonce,role,flavor,original_host_mode,scenario):
    if (type(nonce) is not str or len(nonce)!=16 or any(c not in '0123456789abcdef' for c in nonce) or int(nonce,16)==0
            or flavor!='playable' or not ((role=='host' and original_host_mode is True and scenario is None)
                                        or (role=='client' and scenario=='map-hold-fixture'))):
        raise ValueError('Map-hold nonce requires exact original host or Scenario14 client')
    return nonce

def require_build(build):
    leaves={'native/map_hold_fixture_core.h','native/local_server_hold_fixture_contract.h','native/local_server_hold_fixture.h',
            'native/local_server_native_hold_fixture.inl','native/humanlike_bot_hold_positions.h'}
    if not leaves<=set(build.get('source_sha256',{})):raise RuntimeError('Module lacks Scenario14 and the MapHold Brain/DTO dependency')

def write_contract(directory,request,config,build):
    directory=Path(directory);initial=(directory/'session-initial.bin').read_bytes();bank=(directory/'session-human-bots.bin').read_bytes()
    if len(initial)!=7844 or len(bank)!=128+2*wire.RECORD_BYTES:raise ValueError('Map-hold requires exact ordinary two-person Initial/profile bank')
    project=json.loads((ROOT/'project.json').read_text(encoding='utf-8'))
    with (ROOT/project['analysis_snapshot']).open('rb') as stream:image=hashlib.file_digest(stream,'sha256').digest()
    if image.hex()!=project['expected_snapshot_sha256']:raise ValueError('Pinned original analysis image changed')
    manifest={**copy.deepcopy(request['manifest']),'nonce':f"{request['nonce']:016x}",'module_sha256':build['module_sha256'],
              'image_sha256':image.hex(),'initial_sha256':hashlib.sha256(initial).hexdigest(),'bank_sha256':hashlib.sha256(bank).hexdigest()}
    case=canonical(manifest);case_sha=hashlib.sha256(case).digest();c=request['manifest']
    data=CONTRACT.pack(MAGIC,1,256,14,c['annotation_id'],c['annotation_revision'],config['revision'],3,1,
        request['nonce'],wire.fnv(initial),wire.fnv(bank),wire.fnv(case),len(initial),len(bank),
        *c['sut_spawn_feet'],*c['human_spawn_feet'],c['sut_yaw'],c['arrival_cm'],c['dwell_seconds'],40.,
        image+bytes.fromhex(build['module_sha256'])+case_sha)
    for name,value in [('map-hold-fixture-case.json',case),('map-hold-fixture-contract.bin',data)]:
        with (directory/name).open('xb') as stream:stream.write(value)
    return manifest

def copy_role_contract(directory,role):
    directory=Path(directory)
    with (directory/role/'map-hold-fixture-contract.bin').open('xb') as stream:stream.write((directory/'map-hold-fixture-contract.bin').read_bytes())

def observation(client,nonce,host=None):
    fixture=client.get('native_integration_test',{});status=(host or {}).get('map_hold_fixture',{})
    exact=(type(fixture.get('scenario')) is int and fixture['scenario']==14 and type(fixture.get('nonce')) is int and fixture['nonce']==nonce
           and fixture.get('configured') is True and fixture.get('cli_matched') is True and fixture.get('armed') is True)
    observed=(exact and fixture.get('finished') is True and fixture.get('failed') is False and fixture.get('actions_completed') is True
              and status.get('configured') is True and status.get('armed') is True and status.get('setup') is True and status.get('failed') is False
              and status.get('nonce') == f'{nonce:016x}' and type(status.get('error')) is int and status['error']==0
              and status.get('not_observed') is False and status.get('actual_dwelled') is True and status.get('release_submitted') is True
              and status.get('release_unavailable') is False and status.get('annotations_restored') is True and status.get('route_native_path') is True
              and type(status.get('dwell_seconds')) in (int,float) and math.isfinite(status['dwell_seconds']) and status['dwell_seconds']>=2.5
              and type(status.get('release_submissions')) is int and status['release_submissions']>0
              and status.get('dropped')==0 and status.get('io_errors')==0 and type(status.get('request_key')) is int and status['request_key']>0)
    terminal=exact and fixture.get('finished') is True
    return {'scope':'explicit_original_map_hold_single_point_physics_and_dwell','exact_marker_observed':exact,'finished':bool(terminal),
            'input_actions_completed':bool(observed),'map_hold_physics_verified':bool(observed),'client':fixture if exact else {},'host':status,
            'per_intent_consumption_known':False,'all_firing_angles_covered':False,'competing_point_preference_verified':False,'playable_verified':False}
