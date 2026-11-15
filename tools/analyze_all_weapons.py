"""Generate original weapon identities and UI data from local read-only IoStore.

No game execution, network, source-installation writes, or invented weapon stats.
Uses the independently verified skin catalog's common original DataAsset reader.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from analyze_all_skins import Catalog,Reader,unversioned
sys.path.insert(0,str(ROOT/'work/duel1-respawn'))
import inspect_respawn as native


def chinese_strings(paks,output):
    """Read the verified unencrypted first locres payload, not the opaque index."""
    path=paks/'zh_TW_Text-WindowsClient.pak'
    if not path.exists():return {},{'available':False}
    from native_oodle_directory import decompress_block
    data=path.read_bytes();footer=data.rfind(b'\xe1\x12\x6f\x5a')
    if footer<0 or struct.unpack_from('<I',data,footer+4)[0]!=11:raise ValueError('Unrecognized local text Pak version')
    offset,size,expanded,method=struct.unpack_from('<QQQI',data,0)
    count=struct.unpack_from('<I',data,48)[0]
    if offset or method!=1 or not 1<=count<=64 or not 1<=expanded<=16*1024*1024:raise ValueError('Text first-entry profile changed')
    blocks=[struct.unpack_from('<QQ',data,52+16*i) for i in range(count)]
    flags=data[52+16*count];block_size=struct.unpack_from('<I',data,53+16*count)[0]
    if flags or blocks[0][0]!=57+16*count or blocks[-1][1]-blocks[0][0]!=size:raise ValueError('Text payload is not verified contiguous unencrypted data')
    if hashlib.sha1(data[blocks[0][0]:blocks[-1][1]]).digest()!=data[28:48]:raise ValueError('Original text compressed-payload SHA1 mismatch')
    raw=b''.join(decompress_block(data[a:b],min(block_size,expanded-i*block_size)) for i,(a,b) in enumerate(blocks))
    magic=bytes.fromhex('0e147475674a03fc4a15909dc3377f1b')
    if len(raw)!=expanded or raw[:16]!=magic or raw[16]!=3:raise ValueError('Original optimized locres identity mismatch')
    r=Reader(None,raw);r.p=17;strings_offset=struct.unpack('<Q',r.take(8))[0];total=r.u32();namespaces=r.u32()
    if total>1000000 or namespaces>10000:raise ValueError('Localization table count outside bound')
    entries=[]
    for _ in range(namespaces):
        r.u32();namespace=r.string();n=r.u32()
        if n>total:raise ValueError('Localization namespace count mismatch')
        for _ in range(n):
            r.u32();key=r.string();r.u32();entries.append((namespace,key,r.i32()))
    if len(entries)!=total or r.p!=strings_offset:raise ValueError('Localization key table did not consume exact bytes')
    n=r.u32()
    if n>100000:raise ValueError('Localization string array count outside bound')
    strings=[(r.string(),r.i32())[0] for _ in range(n)]
    if r.p!=len(raw) or any(i<0 or i>=n for _,_,i in entries):raise ValueError('Localization string table did not consume exact bytes')
    output.mkdir(parents=True,exist_ok=True);(output/'zh_TW_Text.locres').write_bytes(raw)
    return {(namespace,key):strings[i] for namespace,key,i in entries}, {
        'available':True,'locale':'zh-TW','path':str(path),'payload_encrypted':False,
        'pak_index_used':False,'pak_index_decrypted':False,'compressed_payload_sha1_verified':True,
        'payload_sha256':hashlib.sha256(raw).hexdigest(),'namespace_count':namespaces,'entry_count':total,
        'string_count':n,'exact_bytes_consumed':len(raw)}


class Metadata:
    def __init__(self):
        self.p=native.p;self.cache={}
        expected='147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f'
        if hashlib.sha256(self.p.b).hexdigest()!=expected:raise ValueError('Pinned original analysis image identity changed')
    def raw(self,r,n):
        o=self.p.pe.get_offset_from_rva(r);return self.p.b[o:o+n]
    def ptr(self,r):return struct.unpack('<Q',self.raw(r,8))[0]-self.p.BASE
    def string(self,r):
        o=self.p.pe.get_offset_from_rva(r);return self.p.b[o:self.p.b.index(0,o)].decode('ascii')
    def structure(self,getter):
        if getter in self.cache:return self.cache[getter]
        ins=native.insns(getter)
        desc=next(native.rip_target(i) for i in ins if i.mnemonic=='lea' and i.op_str.startswith('rdx,') and native.rip_target(i))
        name=self.string(self.ptr(desc+0x18));table=self.ptr(desc+0x20)
        count=struct.unpack('<H',self.raw(desc+0x28,2))[0];fields=[]
        for k in range(count):
            address=self.ptr(table+8*k);b=self.raw(address,64)
            n=self.string(self.ptr(address));kind=struct.unpack_from('<I',b,0x18)[0]&0x3f
            if n=='UnderlyingType':continue
            f={'name':n,'kind':kind,'descriptor':hex(address)}
            if kind==25:f['structure_getter']=self.ptr(address+0x38)
            if kind==22:
                if not fields or fields[-1]['name']!=n:raise ValueError('Array schema inner not proven')
                f['inner']=fields.pop()
            fields.append(f)
        result={'name':name,'getter':hex(getter),'descriptor':hex(desc),'fields':fields}
        self.cache[getter]=result;return result


class UIReader(Reader):
    def __init__(self,z,b,metadata):super().__init__(z,b);self.metadata=metadata
    def native_value(self,field,zero=False):
        if zero:return None
        kind=field['kind']
        if kind in (0,12,30):return self.take(1)[0]
        if kind==3:return self.i32()
        if kind==6:return self.u32()
        if kind==10:return struct.unpack('<f',self.take(4))[0]
        if kind==11:return struct.unpack('<d',self.take(8))[0]
        if kind==25:
            schema=self.metadata.structure(field['structure_getter'])
            if schema['name'] in ('Vector','Rotator'):return list(struct.unpack('<3d',self.take(24)))
            fields,size=unversioned(self.b[self.p:]);self.p+=size
            return {schema['fields'][i]['name']:self.native_value(schema['fields'][i],z) for i,z in fields}
        if kind==22:return [self.native_value(field['inner']) for _ in range(self.count())]
        raise ValueError(f'UI native field kind {kind} not supported')
    def text(self):
        flags=self.u32();history=self.take(1)[0]
        if history==11:return {'flags':flags,'history':'string_table','table':self.name(),'key':self.string()}
        if history==0:return {'flags':flags,'history':'base','namespace':self.string(),'key':self.string(),'source':self.string()}
        if history==255:return {'flags':flags,'history':'none'}
        raise ValueError(f'Original FText history {history} not decoded')


class Weapons:
    def __init__(self,paks,output):
        self.catalog=Catalog(paks,output);self.metadata=Metadata();self.ui_cache={};self.string_tables={}
        self.chinese,self.chinese_source=chinese_strings(paks,output)
    def object_path(self,z,index):
        if not index:return None
        if index>0:return z.exports[index-1]['name']
        value=z.imports[-index-1]
        if value>>62==2:
            package=z.imported_packages[(value>>32)&0x3fffffff]
            return package
        return self.catalog.global_name(value)
    def table(self,path):
        package=path.split('.')[0]
        if package in self.string_tables:return self.string_tables[package]
        z,_,_=self.catalog.extract(package);b=z.content(0);fields,p=unversioned(b)
        if fields:raise ValueError('StringTable unexpected reflected properties')
        r=Reader(z,b);r.p=p
        if r.u32()!=0:raise ValueError('StringTable unexpected object GUID flag')
        namespace=r.string();rows={}
        for _ in range(r.count()):key=r.string();rows[key]=r.string()
        self.string_tables[package]={'namespace':namespace,'rows':rows}
        return self.string_tables[package]
    def ui(self,path):
        package=path.split('.')[0]
        if package in self.ui_cache:return self.ui_cache[package]
        z,_,_=self.catalog.extract(package)
        cdo=next(e for e in z.exports if e['name'].startswith('Default__'))
        cls=next(e for e in z.exports if e['name']==cdo['name'].removeprefix('Default__'))
        direct_native=self.catalog.global_name(int(cls['super_index'],16))=='/Script/ShooterGame.EquippableUIData'
        fields,p=unversioned(z.content(cdo));r=UIReader(z,z.content(cdo),self.metadata);r.p=p
        result={'source_path':path,'sha256':hashlib.sha256(z.data).hexdigest(),'serialized_fields':[i for i,_ in fields]}
        # Gun_UIData_Base contributes CachedWidget before twelve native
        # EquippableUIData fields. All offsets are from native property metadata.
        for i,zero in fields:
            if zero:continue
            if direct_native:i+=1  # Melee has no Gun_UIData_Base.CachedWidget.
            if i in (0,1,15,17):
                value=self.object_path(z,r.i32())
                if i==1:result['kill_icon']=value
                if i==15:result['icon']=value
            elif i==2:result['category_value']=r.take(1)[0]
            elif i in (3,7,11):r.take(24)
            elif i in (4,5,6,8,9,10):r.take(4)
            elif i==12:result['original_ui_weapon_stats']=r.native_value({'kind':25,'structure_getter':0x67611d0})
            elif i in (13,14):
                text=r.text()
                if i==13:result['display_text']=text
            else:raise ValueError(f'UI schema property {i} unsupported')
        result['remaining_bytes']=len(r.b)-r.p
        if result['remaining_bytes']!=4 or r.take(4)!=b'\0'*4:raise ValueError('UI did not consume exact reflected export')
        text=result.get('display_text',{})
        if text.get('history')=='string_table':
            table=self.table(text['table']);result['display_namespace']=table['namespace']
            result['display_name']=table['rows'].get(text['key'])
            result['display_name_zh']=self.chinese.get((table['namespace'],text['key']))
        else:
            result['display_name']=text.get('source');result['display_name_zh']=self.chinese.get((text.get('namespace'),text.get('key')))
        self.ui_cache[package]=result;return result
    def run(self):
        candidates=[]
        for path in self.catalog.entries:
            if not path.endswith('PrimaryAsset.uasset') or '/Equippables/' not in path:continue
            relative=path.split('/Equippables/',1)[1].split('/')
            if (relative[0]=='Guns' and len(relative)==4) or (relative[0]=='Melee' and len(relative)==2):
                candidates.append('/Game/'+path.split('/Content/',1)[1][:-7])
        rows=[];errors=[]
        for package in sorted(candidates):
            try:
                item=self.catalog.decode(package)
                if not item or item['kind']!='weapon':continue
                f=item['original_fields'];row={
                    'guid':item['guid'],'words':item['words'],'data_asset_class_path':item['class_path'],
                    'weapon_class_path':f.get('equippable'),'ui_data_class_path':f.get('ui_data'),
                    'default_skin_class_path':f.get('default_skin'),'forced_attachments':f.get('forced_attachments'),
                    'source_path':item['package'],'source_sha256':item['sha256'],
                    'display_name':None,'display_name_zh':None,'icon':None,'category':None,'category_value':None,
                    'equipment_slot':None,'equipment_slot_status':'runtime original AresItem.GetItemSlot required',
                    'runtime_supported':False,'runtime_status':'not exercised; catalog identity only',
                    'original':item}
                try:
                    ui=self.ui(f['ui_data']);row.update({k:ui.get(k) for k in ['display_name','display_name_zh','icon','category_value']});row['ui']=ui
                    if row['display_name_zh']:row['display_name_zh_locale']='zh-TW'
                except Exception as exc:row['ui_decode_error']=str(exc)
                row['label']=row['display_name'] or package.rsplit('/',1)[-1]
                row['category']={1:'heavy_machine_gun',2:'rifle',3:'shotgun',4:'sidearm',5:'submachine_gun',6:'sniper_rifle',7:'melee'}.get(row['category_value'])
                rows.append(row)
            except Exception as exc:errors.append({'package':package,'error':str(exc)})
        return {'schema_version':1,'source':'original local international-client IoStore DataAsset and UIData serialization',
            'input_installation':str(self.catalog.paks),'binary_sha256':hashlib.sha256(self.metadata.p.b).hexdigest(),
            'candidate_count':len(candidates),'weapon_count':len(rows),'weapons':rows,'errors':errors,
            'native_ui_schemas':list(self.metadata.cache.values()),'chinese_localization':self.chinese_source,'target_executed':False,'network_used':False,
            'limits':['Catalog resources do not establish shop availability or working runtime support.',
                'Slot will be read from original loaded weapon CDO via AresItem.GetItemSlot (vslot798, native70f3150 reads+4a0).',
                'Chinese labels, when present, are the original zh-TW strings; no alternate regional names are invented.']}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--paks',type=Path,default=Path('F:/Riot Games/VALORANT/live/ShooterGame/Content/Paks'))
    parser.add_argument('--output',type=Path,default=ROOT/'work/all-weapons')
    parser.add_argument('--report',type=Path,default=ROOT/'reports/all-weapons.json')
    args=parser.parse_args();result=Weapons(args.paks,args.output).run()
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({'weapons':result['weapon_count'],'errors':len(result['errors']),
        'ui_decoded':sum(bool(x['display_name']) for x in result['weapons']),'report':str(args.report)}))


if __name__=='__main__':main()
