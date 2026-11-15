"""Explicit two-person original-spawn case preparation; no persistent settings.

Input actions live in native Scenario13 on the real role2 client. Geometry,
gait/stance/jump, visible shot and purpose remain separate saved-file verdicts.
"""
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

ROOT=next(p for p in Path(__file__).resolve().parents if (p/'project.json').is_file())
SOURCE=ROOT/'work/humanlike-bots/ai-peek-completion/peek-12-005-analysis/exact-session-human-bots.bin'
DESIGN=ROOT/'work/humanlike-bots/local-cover-candidate/peek-12-003-analysis/native-fixture-design/cases.json'
KINDS=('Wide','Short','SlowCreep','Jump','Crouch','Poppin','Shoulder','InfoRepeek')
CASE_IDS=tuple(k.lower()+'-full-mechanism' for k in KINDS)
VARIANTS=('captured_timing','explicit_sufficient_time_mechanism')
CONTRACT=struct.Struct('<Q8I7Q4I9d160s')
MAGIC=0x31584b455045544c
assert CONTRACT.size==344

# A single, explicit offline fixture-source conversion. This is not a wire
# decoder compatibility path and never submits the old Initial to Store.
PINNED_2353_SHA256='b3c929f53b7e676cc8cdb0ee1d65cf3c82001acbcd02fbaf712280f0f1b31f91'
PINNED_2353_DIGEST='071947f0b4bb560de777763eb3fa087ceac79af1b4c2ce8fc2f6865a675fcd90'
ONLY_2392_DIGEST='095e6326bff65ca9ea20e0a2825d52dd0b5ef97ad7a8fe8939aee111c150716b'
ONLY_2392_SPEC_SHA256='644b774a01aab9877af27577eedc68718128e87413d814533e5341b37d24202a'
_OLD_HEADER=struct.Struct('<Q16I32s3Q')
_OLD_VALUES=struct.Struct('<2353f')
_OLD_COUNT,_OLD_RECORD,_OLD_PROFILES,_OLD_TOTAL=2353,9428,12,113264
_OLD_CONTENT_FNV=0xa4ee20f5ef40bf31
_OLD_TRANSACTION=0x6601aeb7e2d6072d
_ONLY_DEFAULT_TAIL=bytes(37*4)+struct.pack('<f',1000.0)+bytes(4)


def _convert_pinned_2353_fixture_source_to_2392(source,spec_bytes):
    """Attest this one captured bank, preserve every old f32, append39 defaults.

    Neither production human_bot_pack nor native wire/Store is changed. Any
    other source, target count/digest or future schema is an explicit error.
    """
    if type(source) is not bytes or len(source)!=_OLD_TOTAL or hashlib.sha256(source).hexdigest()!=PINNED_2353_SHA256:
        raise ValueError('Offline peek fixture conversion requires the exact pinned 2353 SOURCE SHA/length')
    if (human.PARAM_COUNT!=2392 or human.SCHEMA_DIGEST.hex()!=ONLY_2392_DIGEST
            or wire.VERSION!=1 or wire.HEADER_BYTES!=128 or wire.RECORD_BYTES!=9584 or wire._VALUES.size!=9568):
        raise ValueError('Offline peek fixture conversion is limited to the explicit 2392/095e6326 target contract')
    if type(spec_bytes) is not bytes or hashlib.sha256(spec_bytes).hexdigest()!=ONLY_2392_SPEC_SHA256:
        raise ValueError('Offline peek fixture conversion requires the frozen 2392 spec bytes')
    spec=json.loads(spec_bytes.decode('utf-8'))
    if spec['param_count']!=2392 or spec['schema_digest_sha256']!=ONLY_2392_DIGEST or len(spec['parameters'])!=2392:
        raise ValueError('Offline peek fixture target metadata does not match its frozen contract')
    expected_header=(0x3130544f42485456,1,128,_OLD_TOTAL,1,2051,3,1,_OLD_PROFILES,_OLD_COUNT,_OLD_RECORD,
                     0,1,0,_OLD_PROFILES,_OLD_PROFILES*_OLD_RECORD,0,bytes.fromhex(PINNED_2353_DIGEST),
                     _OLD_CONTENT_FNV,_OLD_TRANSACTION,0)
    if _OLD_HEADER.unpack_from(source)!=expected_header:
        raise ValueError('Pinned offline source header/order/reserved/digest contract changed')
    prefix=struct.pack('<6I',2051,3,1,_OLD_PROFILES,_OLD_COUNT,_OLD_RECORD)+bytes.fromhex(PINNED_2353_DIGEST)+struct.pack('<Q',_OLD_TRANSACTION)
    computed_content=wire.fnv(source[128:],wire.fnv(prefix))
    if computed_content!=_OLD_CONTENT_FNV:raise ValueError('Pinned offline source content-prefix FNV changed')
    added=[];defaults=[]
    for index,p in enumerate(spec['parameters'][_OLD_COUNT:],_OLD_COUNT):
        value=human.f32(p['default'])
        if p['id']!=index or not math.isfinite(value):raise ValueError('Frozen appended parameter order/default invalid')
        defaults.append(value)
        added.append({'id':p['id'],'enum':p['enum'],'key':p['key'],'type':p['type'],'scope':p['scope'],
                      'default_f32':value,'default_f32_le':struct.pack('<f',value).hex()})
    tail=struct.pack('<39f',*defaults)
    if tail!=_ONLY_DEFAULT_TAIL:raise ValueError('Frozen39 defaults changed; no inferred future conversion allowed')
    profiles=[];records=[]
    for index in range(_OLD_PROFILES):
        record=source[128+index*_OLD_RECORD:128+(index+1)*_OLD_RECORD]
        roster,stable,fingerprint=struct.unpack_from('<IIQ',record)
        if roster!=index or stable!=index or wire.fnv(record[:8]+record[16:])!=fingerprint:
            raise ValueError('Pinned offline source record identity/fingerprint changed')
        old_bits=record[16:]
        values=human.validate_profile((*_OLD_VALUES.unpack(old_bits),*defaults))
        if _OLD_VALUES.pack(*values[:_OLD_COUNT])!=old_bits or struct.pack('<39f',*values[_OLD_COUNT:])!=tail:
            raise ValueError('Offline fixture validation changed an original f32 or an appended default')
        profiles.append(values)
        records.append({'index':index,'stable_bot_id':stable,'old_fingerprint_fnv':f'{fingerprint:016x}',
                        'old_f32_payload_sha256':hashlib.sha256(old_bits).hexdigest(),
                        'old2353_f32_bits_preserved':True,'appended_default_f32_count':39})
    conversion={'kind':'explicit_offline_pinned_fixture_source_2353_to_2392','production_wire_compatibility_added':False,
        'same_profile_as_2353_native_run':False,'new39_native_validation_claimed':False,
        'source_sha256':PINNED_2353_SHA256,'source_schema_digest':PINNED_2353_DIGEST,'source_param_count':_OLD_COUNT,
        'source_record_bytes':_OLD_RECORD,'source_header_bytes':128,'source_total_bytes':_OLD_TOTAL,
        'source_profile_count':_OLD_PROFILES,'source_content_prefix_fnv':f'{computed_content:016x}',
        'source_transaction':f'{_OLD_TRANSACTION:016x}','target_schema_digest':ONLY_2392_DIGEST,
        'target_spec_sha256':ONLY_2392_SPEC_SHA256,'target_param_count':2392,'target_record_bytes':9584,
        'old_f32_values_preserved':_OLD_PROFILES*_OLD_COUNT,'appended_default_f32_values':_OLD_PROFILES*39,
        'appended_defaults_payload_sha256':hashlib.sha256(tail).hexdigest(),'added_defaults':added,'source_records':records}
    return profiles,spec,conversion


def canonical(value):
    return json.dumps(value,ensure_ascii=False,sort_keys=True,separators=(',',':'),allow_nan=False).encode()


def add_arguments(parser):
    parser.add_argument('--peek-fixture',choices=CASE_IDS,help='Explicit Scenario13 two-person original-spawn acceptance case')
    parser.add_argument('--peek-variant',choices=VARIANTS,help='Required explicit timing variant; never automatically lengthens exposure')
    parser.add_argument('--peek-scene',choices=('near-own-shoulder','far-own-poppin','own-native-jump','normal007-ownview-long'))
    parser.add_argument('--peek-candidate',type=int,default=0,help='One of 8 local authored target/yaw hypotheses; unsupported scenes fail promptly')



def _scene007_design(args):
    # Explicit offline authorship from one original same-life geometry record.
    # This reads no process and supplies no Track/memory/SpaceHints to Brain.
    if (args.peek_fixture not in (CASE_IDS[5],CASE_IDS[6],CASE_IDS[7])
            or args.peek_variant!='explicit_sufficient_time_mechanism' or args.peek_candidate!=0):
        raise ValueError('normal007 scene requires explicit Poppin/Shoulder/Info sufficient-time test and candidate0')
    path=ROOT/'work/humanlike-bots/ai-peek-completion/v4-scene007/source-geometry.json'
    raw=path.read_bytes()
    if hashlib.sha256(raw).hexdigest()!='f8655cb30416ba453a781f10c4d2539157e5cddb61bc105afa7c624518cf6f99':
        raise ValueError('Pinned normal007 source geometry changed')
    evidence=json.loads(raw);snapshot=ROOT/'work/humanlike-bots/ai-peek-completion/peek-12-007-analysis/prefix-001/observer.jsonl.gz'
    if hashlib.sha256(snapshot.read_bytes()).hexdigest()!=evidence['observer_snapshot_sha256']:
        raise ValueError('Pinned normal007 original observer prefix changed')
    design=json.loads(DESIGN.read_text(encoding='utf-8'))
    selected=next(c for c in design['cases'] if c['case_id']==args.peek_fixture)
    selected['scene_candidates']=['normal007-ownview-long']
    if args.peek_fixture==CASE_IDS[6]:
        selected['explicit_profile_overrides'].update(ShoulderDistanceCm=500,ShoulderDistanceErrorCm=20)
        selected['variants']['explicit_sufficient_time_mechanism']['ShoulderExposureMs']=2500
        configuration='shoulder007_artificial_d500_error20_span520_exposure2500'
    elif args.peek_fixture==CASE_IDS[5]:
        selected['explicit_profile_overrides']['PoppinDistanceErrorCm']=20
        configuration='poppin007_original_d320_error20_extra180_span520_requires_new_acquire'
    else:
        # Artificial two-leg mechanism test. Geometry retention is separate
        # from the unchanged original 900ms enemy memory/re-peek admission.
        selected['explicit_profile_overrides'].update(InfoRepeekDistanceCm=500,InfoRepeekDistanceErrorCm=20,
            InfoRepeekConditionAny=0,InfoRepeekEngageWeight=100,InfoRepeekReturnWeight=0,SearchAngleMemoryMs=20000)
        selected['variants']['explicit_sufficient_time_mechanism']['InfoRepeekExposureMs']=2500
        configuration='info007_artificial_d500_error20_span520_exposure2500_staticproof20000_enemy900'
    target={'feet_proposal':evidence['target_spawn_feet_proposal'],
            'facing_yaw_proposal':evidence['target_safe_primary_yaw_proposal'],'native_validated':False}
    design['scene_candidates']=[{'scene_id':'normal007-ownview-long','map':'ascent',
        'sut_spawn_feet_proposal':evidence['sut_spawn_feet_proposal'],
        'sut_facing_yaw_proposals':[evidence['sut_initial_yaw_proposal']]*2,
        'target_spawn_proposals':[copy.deepcopy(target) for _ in range(4)]}]
    provenance={'configuration_name':configuration,'source_geometry_sha256':hashlib.sha256(raw).hexdigest(),
        'source_pair':evidence['source_pair'],'source_module_sha256':evidence['source_module_sha256'],
        'source_observer_row0':evidence['source_observer_row0'],'source_pass':evidence['source_pass'],
        'source_raw_row_sha256':evidence['source_raw_row_sha256'],'source_own_life':evidence['source_own_life'],
        'source_target_life':evidence['source_target_life'],'source_local_cover':evidence['source_local_cover'],
        'source_current_contact_is_visible_not_proved':True,'old_tracks_or_memory_inherited':False,
        'fresh_two_person_supported_spawn_and_original_geometry_required':True,
        'new_scene_native_validated':False,'ordinary_shoulder_profile_or_deception_claimed':False}
    if args.peek_fixture==CASE_IDS[7]:
        provenance['information_mechanism']={'first_leg':'genuine_cover_focus_proof_without_injected_track',
            'binding':'first_actual_recognized_source_in_this_life',
            'second_leg':'original_settled_retreat_then_fresh_same_track_under_unchanged_enemy_memory',
            'static_geometry_angle_memory_ms':20000,'enemy_memory_lost_hold_ms_unchanged':900,
            'condition_any':0,'condition_all':8192,'decision_ms_unchanged':100,
            'recover_choice_engage_weight':100,'recover_choice_return_weight':0,
            'target0_input':'existing_original_crouch_only_after_actual_acquire',
            'source0_state_or_memory_writes':False,'ordinary_default_info_profile_claimed':False}
    return design,provenance


def prepare_request(args,config,*,nonce=None,design=None,source=None):
    selected=getattr(args,'peek_fixture',None)
    if not selected:
        if any(getattr(args,key,None) for key in ('peek_variant','peek_scene')) or getattr(args,'peek_candidate',0):
            raise ValueError('Peek scene/variant/candidate requires explicit --peek-fixture')
        return None,config
    if (not args.interactive or not args.original_host_mode or args.scenario is not None
            or not 25<=args.seconds<=90 or args.peek_variant not in VARIANTS or not args.peek_scene
            or type(args.peek_candidate) is not int or not 0<=args.peek_candidate<8):
        raise ValueError('Peek fixture requires explicit interactive original-host, 25..90 seconds, variant, scene, candidate0..7 and no competing scenario')
    scene007=None
    if args.peek_scene=='normal007-ownview-long':
        if design is not None:raise ValueError('Pinned normal007 scene does not accept a caller-supplied design')
        design,scene007=_scene007_design(args)
    design=json.loads(DESIGN.read_text(encoding='utf-8')) if design is None else design
    case=next(q for q in design['cases'] if q['case_id']==selected)
    if args.peek_scene not in case['scene_candidates']:raise ValueError('Scene is outside this case bounded authored hypothesis list')
    scene=next(q for q in design['scene_candidates'] if q['scene_id']==args.peek_scene)
    source=SOURCE.read_bytes() if source is None else source
    source_profiles,spec,conversion=_convert_pinned_2353_fixture_source_to_2392(source,(ROOT/'tools/humanlike_bot_spec.json').read_bytes())
    parameters=spec['parameters'];by_enum={p['enum']:p for p in parameters}
    def profile(index):return list(source_profiles[index])
    human0=human.validate_profile(profile(0));sut=list(human.validate_profile(profile(case['profile_source_resolved_index'])))
    original=list(sut);overrides={**case['explicit_profile_overrides'],**case['variants'][args.peek_variant]}
    if selected==CASE_IDS[7]:
        # Stricter separately named test configuration. Original producer needs
        # a positive enabled/weight span for cover prewarming. InformationGoal
        # disallows this peek while a real visible target selects Engage.
        old=int(sut[by_enum['InfoRepeekConditionAll']['id']]);overrides['InfoRepeekConditionAll']=old|8192
    for enum,value in overrides.items():sut[by_enum[enum]['id']]=human.f32(value)
    human.validate_profile(sut)
    for p in parameters:
        if p['scope']=='global' and sut[p['id']]!=human0[p['id']]:
            raise ValueError('Captured bank has inconsistent shared parameter '+p['enum'])
        if any(v in p['enum'] for v in ('MaximumRisk','MinimumConfidence','MinimumSpaceCm')) or p['enum'].startswith(('Visual','Memory')):
            if sut[p['id']]!=original[p['id']]:raise ValueError('Fixture attempted to change perception or safety gate '+p['enum'])
    cfg=copy.deepcopy(config);cfg.update(total_players=2,seed=3,revision=config['revision']+1,map_id='ascent',
                                       mode='deathmatch',invincible=False,infinite_ammo=False,paused=False)
    settings=human.defaults();settings['enabled']=True
    settings['global_overrides']={p['key']:human0[p['id']] for p in parameters if p['type']!='constant'}
    settings['individual_overrides']={'1':{p['key']:sut[p['id']] for p in parameters if p['scope']=='bot' and p['type']!='constant'}}
    cfg['schema_version']=3;cfg['human_bots']=human.validate(settings);cfg=validate_config(cfg)
    bundle=configuration_bundle(cfg)
    assert wire._VALUES.unpack_from(bundle.records[0],16)==tuple(human0)
    assert wire._VALUES.unpack_from(bundle.records[1],16)==tuple(sut)
    target=scene['target_spawn_proposals'][args.peek_candidate%4]
    f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
    sut_feet=list(map(f32,scene['sut_spawn_feet_proposal']));target_feet=list(map(f32,target['feet_proposal']))
    sut_yaw=f32(scene['sut_facing_yaw_proposals'][args.peek_candidate//4]);target_yaw=f32(target['facing_yaw_proposal'])
    manifest={'scenario':13,'case_id':selected,'kind':KINDS.index(KINDS[case['profile_source_resolved_index']-1])+1,
        'variant':args.peek_variant,'info_configuration':'strict_recent_memory_InformationGoal' if selected==CASE_IDS[7] else None,
        'scene':args.peek_scene,'scene_index':design['scene_candidates'].index(scene)+1,'candidate':args.peek_candidate,
        'participants':2,'target_index':0,'sut_index':1,'seed':3,'revision':cfg['revision'],
        'source_initial_sha256':hashlib.sha256(source).hexdigest(),'source_resolved_profile_index':case['profile_source_resolved_index'],
        'explicit_test_overrides':overrides,'sut_spawn_feet':sut_feet,'target_spawn_feet':target_feet,
        'sut_yaw':sut_yaw,'target_yaw':target_yaw,'arrival_cm':sut[by_enum['MoveArrivalCm']['id']],
        'hypothesis_native_validated':False,'ordinary_human0_profile_preserved':False,'ordinary_human0_2353_prefix_preserved':True,
        'fixture_schema_conversion':conversion,'original_native_consumption_known':False,
        'persistent_settings_written':False,'case_seconds':20,'maximum_target_primary_bursts':3,
        'explicit_initial_configuration_overrides':{k:{'before':config.get(k),'fixture':cfg[k]} for k in ('total_players','seed','revision','map_id','mode','invincible','infinite_ammo','paused') if config.get(k)!=cfg[k]},
        'info_has_no_profile_commit_or_last_seen_write':True}
    if scene007 is not None:manifest['explicit_scene007_configuration']=scene007
    request={'nonce':nonce if nonce is not None else secrets.randbits(64) or 1,'manifest':manifest,
             'map_info':{'spawn_anchors':[{'position':sut_feet,'yaw_deg':sut_yaw},{'position':target_feet,'yaw_deg':target_yaw}]}}
    if type(request['nonce']) is not int or not 0<request['nonce']<2**64:raise ValueError('Peek nonce must be nonzero uint64')
    return request,cfg


def marker(nonce):
    return struct.pack('<Q4IQ8I',0x31545345544e534c,1,64,1,13,nonce,500,500,10000,30000,0,0,0,0)


def validate_native_nonce(nonce,role,flavor,original_host_mode,scenario):
    if (type(nonce) is not str or len(nonce)!=16 or any(c not in '0123456789abcdef' for c in nonce)
            or int(nonce,16)==0 or flavor!='playable'
            or not ((role=='host' and original_host_mode is True and scenario is None)
                    or (role=='client' and scenario=='peek-fixture'))):
        raise ValueError('Peek fixture nonce requires exact original host or Scenario13 role2 client with nonzero 16 lower hex digits')
    return nonce


def require_build(build):
    required={'native/local_server_peek_fixture_contract.h','native/local_server_peek_fixture_observer.h',
              'native/local_server_native_peek_fixture.inl','native/local_server_peek_input_profiles.inl'}
    if not required<=set(build.get('source_sha256',{})):raise RuntimeError('Candidate module lacks explicit Scenario13 original-native peek fixture source closure')


def write_contract(directory,request,config,build):
    directory=Path(directory);initial=(directory/'session-initial.bin').read_bytes();bank=(directory/'session-human-bots.bin').read_bytes()
    if len(initial)!=7844 or len(bank)!=128+2*wire.RECORD_BYTES:raise ValueError('Peek fixture requires exact normal two-person Initial/bank')
    manifest=copy.deepcopy(request['manifest']);manifest.update(nonce=f"{request['nonce']:016x}",module_sha256=build['module_sha256'])
    case=canonical(manifest);case_digest=hashlib.sha256(case).digest()
    image_config=json.loads((ROOT/'project.json').read_text(encoding='utf-8'));image=ROOT/image_config['analysis_snapshot']
    with image.open('rb') as stream:image_digest=hashlib.file_digest(stream,'sha256').digest()
    if image_digest.hex()!=image_config['expected_snapshot_sha256']:raise ValueError('Original PE changed since pinned input ABI proof')
    manifest.update(case_sha256=case_digest.hex(),image_sha256=image_digest.hex(),
                    initial_sha256=hashlib.sha256(initial).hexdigest(),bank_sha256=hashlib.sha256(bank).hexdigest())
    c=request['manifest'];data=CONTRACT.pack(MAGIC,1,344,13,c['kind'],VARIANTS.index(c['variant']),c['scene_index'],c['candidate'],1,
        request['nonce'],wire.fnv(initial),wire.fnv(bank),wire.fnv(case),0,0,0,len(initial),len(bank),config['revision'],3,
        *c['sut_spawn_feet'],*c['target_spawn_feet'],c['target_yaw'],c['sut_yaw'],c['arrival_cm'],
        image_digest+bytes.fromhex(build['module_sha256'])+case_digest+hashlib.sha256(initial).digest()+hashlib.sha256(bank).digest())
    with (directory/'peek-fixture-contract.bin').open('xb') as stream:stream.write(data)
    with (directory/'peek-fixture-case.json').open('x',encoding='utf-8') as stream:json.dump(manifest,stream,ensure_ascii=False,indent=2)
    return manifest


def copy_role_contract(directory,role):
    directory=Path(directory);data=(directory/'peek-fixture-contract.bin').read_bytes()
    with (directory/role/'peek-fixture-contract.bin').open('xb') as stream:stream.write(data)


def observation(client,nonce):
    value=client.get('native_integration_test',{})
    exact=value.get('scenario')==13 and value.get('nonce')==nonce and value.get('cli_matched') is True and value.get('armed') is True
    p=value.get('peek_fixture',{}) if exact else {}
    edges=p.get('failed') is False and p.get('release_unavailable') is False
    if value.get('kind')==8:edges=edges and p.get('crouch_held_observed') is True and p.get('crouch_released_observed') is True and p.get('move_override_cleared') is True
    if value.get('kind')==6:edges=edges and p.get('primary_held_observed') is True and p.get('primary_released_observed') is True
    return {'scope':'target_original_native_input_only','exact_marker_observed':exact,
            'finished':bool(exact and value.get('finished') is True),'input_actions_completed':bool(exact and value.get('actions_completed') is True and edges),
            'host_geometry_verified':False,'purpose_verified':False,'native_consumption_known':False,'client':p}
