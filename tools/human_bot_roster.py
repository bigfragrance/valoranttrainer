"""Fixed bot entry identities, pure edits, and exact native roster sidecar.

Entry tokens are saved strings; the original wire stableBotId remains 1..N.
No process or network is opened by this module.
"""
from __future__ import annotations
import copy,hashlib,json,re,secrets,struct
from pathlib import Path
import humanlike_settings as human
import human_bot_pack as wire
from training_heroes import hero_profile,selected_hero

ROOT=next(p for p in Path(__file__).resolve().parents if (p/'project.json').is_file())
NAME='session-bot-roster.bin'
MAGIC=0x3130545352425456
HEADER=struct.Struct('<Q10I4I32s4Q');ROW=struct.Struct('<IIQ4I')
assert HEADER.size==128 and ROW.size==32
NATIVE_SOURCES=frozenset(('native/local_server_bot_roster_wire.h','native/local_server_bot_roster_initial.h',
 'native/local_server_control.h','native/local_server_match.h','native/local_server_runtime.h'))

def token(value):
    if type(value)is not str or not re.fullmatch('[0-9a-f]{16}',value)or int(value,16)==0:
        raise human.ConfigError('人机id必须为非零16位小写十六进制字符串')
    return value

def catalog():
    return hero_profile(ROOT)['heroes']

def validate_roster(rows,config):
    if type(rows)is not list or not 1<=len(rows)<=29:
        raise human.ConfigError('乱斗至少保留1个人机，最多29个人机')
    known={r['id']for r in catalog()};seen=set();heroes=set();result=[]
    for entry in rows:
        human._object(entry,{'id','hero_id','preset_id','overrides'},'人机名单项',complete=True)
        identity=token(entry['id']);hero=entry['hero_id'];preset=entry['preset_id']
        if identity in seen:raise human.ConfigError('重复人机id')
        if type(hero)is not str or hero not in known or hero in heroes:raise human.ConfigError('英雄必须来自目录且不能重复')
        if type(preset)is not str or preset not in human.preset_ids(config):raise human.ConfigError('未知人机预设')
        seen.add(identity);heroes.add(hero)
        result.append({'id':identity,'hero_id':hero,'preset_id':preset,'overrides':human._individual_values(entry['overrides'])})
    return result

def validate_session_roster(model,total_players,local_hero_id):
    rows=model.get('bot_roster')
    if rows is None:return
    if total_players!=len(rows)+1:raise human.ConfigError('总人数必须等于固定人机名单数量加自己')
    if local_hero_id in{e['hero_id']for e in rows}:raise human.ConfigError('人机英雄不能与自己选择的英雄重复')

def editable_values(values):
    return {p['key']:values[p['id']]for p in human.PARAMS if p['scope']=='bot'and p['editable']}

def materialize_legacy(model,seed,total_players,local_hero_id):
    if 'bot_roster'in model:return model
    if type(total_players)is not int or not 2<=total_players<=30:raise human.ConfigError('固定名单需要总人数2–30')
    heroes=[h for h in catalog()if h['id']!=local_hero_id]
    result=copy.deepcopy(model);rows=[]
    for slot in range(1,total_players):
        identity=hashlib.sha256(b'VTHBOT-LEGACY-ENTRY-1\0'+struct.pack('<I',slot)).hexdigest()[:16]
        rows.append({'id':identity,'hero_id':heroes[slot-1]['id'],'preset_id':human.selected_preset(model,slot),
                     'overrides':editable_values(human.resolve(model,seed,slot,validated=True))})
    result['bot_roster']=rows
    return human.validate(result)

def structure(model,local_hero_id):
    rows=model.get('bot_roster')
    return None if rows is None else (local_hero_id,tuple((e['id'],e['hero_id'])for e in rows))

def structure_changed(before,after):
    # Both sides materialize through the same pure normalization, including
    # legacy lists, before deciding whether actors can retain their identity.
    return structure(before['human_bots'],before['local_hero_id'])!=structure(after['human_bots'],after['local_hero_id'])

def _native_roster(state):
    if type(state)is not dict:return {}
    if type(state.get('configured_bot_roster'))is dict:return state['configured_bot_roster']
    for key in ('game','session_runtime','runtime'):
        value=state.get(key)
        found=_native_roster(value)if type(value)is dict else{}
        if found:return found
    return {}

def live_match(config,state):
    rows=config['human_bots'].get('bot_roster')
    if rows is None:return True
    actual=_native_roster(state);profile=hero_profile(ROOT)
    if not(actual.get('fixed')is True and actual.get('loaded')is True and actual.get('validated')is True
           and actual.get('native_catalog_verified')is True and not actual.get('error')
           and type(actual.get('count'))is int and actual['count']==len(rows)
           and actual.get('human_hero_id')==config['local_hero_id']
           and actual.get('catalog_profile_sha256')==profile['sha256']):return False
    entries=actual.get('entries')
    if type(entries)is not list or len(entries)!=len(rows):return False
    heroes={h['id']:h['index']for h in profile['heroes']}
    if type(actual.get('human_hero_index'))is not int or actual['human_hero_index']!=heroes[config['local_hero_id']]:return False
    return all(a.get('slot')==slot and type(a.get('slot'))is int and a.get('entry_token')==e['id']
               and a.get('hero_id')==e['hero_id']and type(a.get('hero_index'))is int and a['hero_index']==heroes[e['hero_id']]
               for slot,(e,a)in enumerate(zip(rows,entries),1))

def pending_restart():
    return {'status':'saved','restart_required':True,'message':'名单已保存，下次开始或重启生效',
            'transport_applied':False,'consumer_effective':False,'behavior_execution_verified':False}

def payload(config,initial,*,root=ROOT):
    if len(initial)!=7844 or struct.unpack_from('<Q2I',initial)!=(0x32304C5443525456,2,7844):
        raise human.ConfigError('固定名单需要原Initial7844')
    model=human.validate(config['human_bots']);validate_session_roster(model,config['total_players'],config['local_hero_id'])
    rows=model.get('bot_roster')
    if rows is None:raise human.ConfigError('新名单启动不允许缺少bot_roster')
    hero,profile=selected_hero(config['local_hero_id'],root=root);heroes={h['id']:h for h in profile['heroes']}
    body=b''.join(ROW.pack(slot,heroes[e['hero_id']]['index'],int(e['id'],16),*heroes[e['hero_id']]['guid_words'])for slot,e in enumerate(rows,1))
    header=HEADER.pack(MAGIC,1,128,32,128+len(body),config['revision'],config['seed'],config['total_players'],len(rows),hero['index'],0,
                       *hero['guid_words'],bytes.fromhex(profile['sha256']),wire.fnv(initial),wire.fnv(body),0,0)
    return header+body

def write_sidecar(directory,config,*,root=ROOT):
    directory=Path(directory).resolve(strict=True);project=Path(root).resolve(strict=True)
    initial=directory/'session-initial.bin'
    if not directory.is_relative_to(project)or not initial.is_file():raise human.ConfigError('名单只可写入本项目已验证Initial目录')
    raw=payload(config,initial.read_bytes(),root=root);target=directory/NAME
    with target.open('xb')as stream:stream.write(raw)
    return {'path':str(target),'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest(),
            'rows_fnv':f'{wire.fnv(raw[128:]):016x}','required':True,'status':'fixed_roster_sidecar_written_not_native_execution'}

def _entry(model,identity):
    token(identity)
    for slot,entry in enumerate(model['bot_roster'],1):
        if entry['id']==identity:return slot,entry
    raise human.ConfigError('未知人机id')

def preset_profile(model,preset_id):
    if preset_id not in human.preset_ids(model):raise human.ConfigError('未知预设')
    probe=copy.deepcopy(model);probe.pop('bot_roster',None);probe.pop('individual_presets',None)
    probe['individual_overrides']={};probe['distribution']={};probe['global_overrides']={};probe['preset']=preset_id
    values=human._base(probe);shared=human._base(model)
    for p in human.PARAMS:
        if p['scope']=='global_only':values[p['id']]=shared[p['id']]
    return human.validate_profile(values)

def action(config,body):
    """Return a CAS patch only; caller uses its existing locked store.update."""
    if type(body)is not dict:raise human.ConfigError('名单操作必须为对象')
    action=body.get('action');common={'action','expected_revision'}
    fields={'add':{'hero_id','preset_id'},'remove':{'entry_id'},'reorder':{'entry_ids'},
     'update_entry':{'entry_id','hero_id','overrides'},'apply_preset':{'entry_id','preset_id','mode'},
     'save_preset':{'entry_id','preset_id','name'},'rename_preset':{'preset_id','name'},'delete_preset':{'preset_id'}}
    if action not in fields:raise human.ConfigError('未知名单操作')
    human._object(body,common|fields[action],'名单操作')
    if type(body.get('expected_revision'))is not int or body['expected_revision']!=config['revision']:
        raise human.RevisionConflict('设置版本已变化，请刷新后重试')
    model=human.validate(config['human_bots']);rows=copy.deepcopy(model['bot_roster']);updated=copy.deepcopy(model)
    identity=body.get('entry_id');preset=body.get('preset_id');summary={'action':action}
    if action=='add':
        while True:
            identity=secrets.token_hex(8)
            if int(identity,16)and identity not in{e['id']for e in rows}:break
        preset=preset or'rank:iron_ii'
        rows.append({'id':identity,'hero_id':body.get('hero_id'),'preset_id':preset,'overrides':editable_values(preset_profile(model,preset))})
    elif action=='reorder':
        order=body.get('entry_ids')
        if type(order)is not list or any(type(i)is not str for i in order)or len(order)!=len(rows)or len(set(order))!=len(rows)or set(order)!={e['id']for e in rows}:raise human.ConfigError('重排必须包含每个人机id且仅一次')
        by_id={e['id']:e for e in rows};rows=[by_id[i]for i in order]
    elif action in('remove','update_entry','apply_preset','save_preset'):
        slot,entry=_entry(model,identity);target=rows[slot-1]
        if action=='remove':rows.pop(slot-1)
        elif action=='update_entry':
            if 'hero_id'in body:target['hero_id']=body['hero_id']
            if 'overrides'in body:
                human._object(body['overrides'],human.BY_KEY,'个人参数patch')
                for key in body['overrides']:
                    p=human.BY_KEY[key]
                    if p['scope']!='bot'or not p['editable']:raise human.ConfigError('参数不可逐人编辑')
                target['overrides']=human._map_patch(target['overrides'],body['overrides'],'个人参数')
        elif action=='apply_preset':
            mode=body.get('mode','snapshot')
            if mode not in('snapshot','inherit'):raise human.ConfigError('未知预设应用模式')
            target['preset_id']=preset
            target['overrides']=editable_values(preset_profile(model,preset))if mode=='snapshot'else{}
            summary['mode']=mode
        else:
            human._custom_id(preset)
            updated.setdefault('custom_presets',{})[preset]={'name':body.get('name'),'base_preset':'default',
                'overrides':editable_values(human.resolve(model,config['seed'],slot,validated=True))}
    elif action=='rename_preset':
        human._custom_id(preset)
        if preset not in updated.get('custom_presets',{}):raise human.ConfigError('未知自定义预设')
        updated['custom_presets'][preset]['name']=body.get('name')
    elif action=='delete_preset':
        human._custom_id(preset)
        if preset not in updated.get('custom_presets',{}):raise human.ConfigError('未知自定义预设')
        if model['preset']==preset or preset in model.get('individual_presets',{}).values()or any(e['preset_id']==preset for e in rows):raise human.ConfigError('预设仍被引用，不能删除')
        del updated['custom_presets'][preset];updated['preset_overrides'].pop(preset,None)
    updated['bot_roster']=rows;updated=human.validate(updated)
    validate_session_roster(updated,len(rows)+1,config['local_hero_id'])
    # Array replacement is explicit. Map removals use the existing null-patch
    # semantics so stale presets/overrides cannot survive a full-state merge.
    patch={'bot_roster':updated['bot_roster']}
    for name in('custom_presets','preset_overrides'):
        before=model.get(name,{});after=updated.get(name,{})
        changed={k:copy.deepcopy(after[k])if k in after else None for k in set(before)|set(after)if before.get(k)!=after.get(k)}
        if changed:patch[name]=changed
    summary.update({k:v for k,v in [('entry_id',identity),('preset_id',preset)]if v is not None})
    return {'patch':{'human_bots':patch,'total_players':len(rows)+1},'expected_revision':body['expected_revision'],'roster_action':summary}
