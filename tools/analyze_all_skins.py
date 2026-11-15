"""Read original local Zen DataAsset identities and skin relationships offline.

Input containers are opened read-only. Extracted evidence and catalogs stay on F:.
GUIDs, parent weapons, ordered levels, chromas and attachment overrides are taken
from the original CDO serialization, never inferred from a skin's filename.
"""
from __future__ import annotations

from collections import OrderedDict
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'work/extract'))
from zen import Zen,names_at
from iostore import IoStore
from decode_collision import unversioned

KINDS={
 '/Script/ShooterGame.EquippableSkinDataAsset':'skin',
 '/Script/ShooterGame.EquippableSkinLevelDataAsset':'level',
 '/Script/ShooterGame.EquippableSkinChromaDataAsset':'chroma',
 '/Script/ShooterGame.EquippableDataAsset':'weapon',
}
SCHEMAS={
 'skin': [('equippable','soft'),('levels','array_soft'),('default_chroma','soft'),('chromas','array_soft'),
          ('ui_data','soft'),('attachment_overrides','map_soft'),('theme','soft'),('content_tier','soft'),
          ('content_edition','soft'),('wallpaper','soft'),('use_highest_owned_level_name','bool'),('uuid','guid')],
 'level':[('skin_attachment','soft'),('ui_data','soft'),('model_viewer_fxc','soft'),('streamed_video','soft'),('uuid','guid')],
 'chroma':[('chroma_attachment','soft'),('ui_data','soft'),('attachment_overrides','map_soft'),
           ('model_viewer_fxc','soft'),('streamed_video','soft'),('when_focusing_buddy_disable_fxc','bool'),('uuid','guid')],
 'weapon':[('equippable','soft'),('ui_data','soft'),('max_attachment_cost','i32'),('forced_attachments','set_soft'),
           ('default_skin','soft'),('unseen_content_tag','tag'),('preload_platforms','u32'),('uuid','guid')],
}
for schema in SCHEMAS.values():
    # AresBasePrimaryDataAsset's verified native order after Uuid+38.
    schema.extend([('hide_if_not_owned','bool'),('base_content','bool')])

def guid(words):
    a,b,c,d=words
    return f'{a:08x}-{b>>16:04x}-{b&65535:04x}-{c>>16:04x}-{c&65535:04x}{d:08x}'


class Reader:
    def __init__(self,z,b):self.z,self.b,self.p=z,b,0
    def take(self,n):
        if not 0<=n<=len(self.b)-self.p:raise ValueError('CDO serialized value exceeds export')
        result=self.b[self.p:self.p+n];self.p+=n;return result
    def i32(self):return struct.unpack('<i',self.take(4))[0]
    def u32(self):return struct.unpack('<I',self.take(4))[0]
    def count(self):
        n=self.i32()
        if not 0<=n<=16384:raise ValueError('CDO collection count outside bound')
        return n
    def name(self):
        i,n=struct.unpack('<II',self.take(8));i&=0x3fffffff
        if i>=len(self.z.names):raise ValueError('CDO mapped FName outside package name table')
        name=self.z.names[i]
        return name+('_'+str(n-1) if n else '')
    def string(self):
        n=self.i32()
        if abs(n)>16384:raise ValueError('CDO subobject string outside bound')
        return self.take(n if n>=0 else -n*2).decode('utf-8' if n>=0 else 'utf-16le').rstrip('\0')
    def soft(self):
        package,asset,sub=self.name(),self.name(),self.string()
        if package=='None' and asset=='None' and not sub:return None
        if not package.startswith('/'):raise ValueError('CDO soft path is not an original absolute package')
        return package+('.'+asset if asset!='None' else '')+(':'+sub if sub else '')
    def value(self,kind,zero=False):
        if zero:return [] if kind in ('array_soft','map_soft','set_soft') else False if kind=='bool' else None
        if kind=='soft':return self.soft()
        if kind=='bool':return bool(self.take(1)[0])
        if kind=='i32':return self.i32()
        if kind=='u32':return self.u32()
        if kind=='guid':
            words=list(struct.unpack('<4I',self.take(16)))
            return {'guid':guid(words),'words':words,'serialized_le_hex':struct.pack('<4I',*words).hex()}
        if kind=='array_soft':return [self.soft() for _ in range(self.count())]
        if kind=='set_soft':
            removed=[self.soft() for _ in range(self.count())]
            return {'removed':removed,'added':[self.soft() for _ in range(self.count())]}
        if kind=='map_soft':
            removed=[self.soft() for _ in range(self.count())]
            return {'removed':removed,'entries':[{'key':self.soft(),'value':self.soft()} for _ in range(self.count())]}
        if kind=='tag':
            fields,used=unversioned(self.b[self.p:]);self.p+=used
            if any(i!=0 for i,_ in fields):raise ValueError('Unexpected GameplayTag schema')
            return next((None if z else self.name() for i,z in fields),None)
        raise ValueError('Unsupported explicit CDO schema type')


class CachedStore(IoStore):
    """Bounded shared-block cache avoids spawning a decoder for each tiny CDO."""
    def __init__(self,path):
        super().__init__(path);self.cache=OrderedDict();self.cached_bytes=0
        self.case_paths={p.casefold():p for p in self.files}
    def block(self,index):
        if index in self.cache:self.cache.move_to_end(index);return self.cache[index]
        physical,compressed,uncompressed,method=self.blocks[index]
        partition=physical//self.partition_size
        path=self.path.with_suffix('.ucas') if not partition else self.path.with_name(self.path.stem+'_s'+str(partition)+'.ucas')
        with path.open('rb') as stream:
            stream.seek(physical%self.partition_size)
            stored=stream.read((compressed+15)//16*16 if self.block_hashes else compressed)
        if self.block_hashes and hashlib.sha1(stored).digest()!=self.block_hashes[index]:raise ValueError('Original block SHA1 mismatch')
        packed=stored[:compressed];method=self.methods[method]
        data=packed if method=='None' else zlib.decompress(packed) if method=='Zlib' else self._oodle(packed,uncompressed) if method=='Oodle' else None
        if data is None or len(data)!=uncompressed:raise ValueError('Original block decompression mismatch')
        self.cache[index]=data;self.cached_bytes+=len(data)
        while self.cached_bytes>32*1024*1024:
            _,old=self.cache.popitem(last=False);self.cached_bytes-=len(old)
        return data
    def read(self,name):
        name=self.case_paths.get(name.casefold(),name)
        index=self.files[name];offset,size=self.ranges[index]
        data=b''.join(self.block(i) for i in range(offset//self.block_size,(offset+size+self.block_size-1)//self.block_size))
        begin=offset%self.block_size;data=data[begin:begin+size]
        if len(data)!=size:raise ValueError('Original package chunk length mismatch')
        return data


class Catalog:
    def __init__(self,paks,output):
        self.paks,self.output=paks.resolve(),output.resolve();self.stores={};self.errors=[];self.rows=[]
        self.ui_cache={};self.string_tables={};self.ui_errors=[];self.ui_metadata=None;self.decoded={}
        self.chinese=None;self.chinese_source={'available':False}
        if self.paks.drive.upper()!='F:' or self.output.drive.upper()!='F:' or self.paks in self.output.parents:
            raise ValueError('Original input and new output must remain on F:, outside the installation')
        # Discover today's actual container directory, rather than using the
        # historical extraction inventory. Package case and container contents
        # can change independently from the pinned native analysis image.
        self.entries={};self.case_entries={};self.duplicates=[];self.source_tocs=[]
        for path in sorted(self.paks.glob('*.utoc')):
            store=CachedStore(path);container=path.name;self.stores[container]=store
            fingerprint=hashlib.sha256(store.data).hexdigest()
            self.source_tocs.append({'container':container,'sha256':fingerprint,
                'entry_count':store.entry_count,'indexed_files':len(store.files),'version':store.version})
            for name,index in store.files.items():
                prior=self.case_entries.get(name.casefold())
                if prior is not None:
                    self.duplicates.append({'path':name,'previous_path':prior,
                        'previous_container':self.entries[prior][0],'container':container})
                    raise ValueError('Ambiguous current container package: '+name)
                entry={'path':name,'index':index,'chunk_id':store.ids[index],
                    'size':store.ranges[index][1],'toc_sha256':fingerprint}
                self.entries[name]=(container,entry);self.case_entries[name.casefold()]=name
        if not self.source_tocs:raise ValueError('No original current TOCs were found')
        self.inventory_sha256=hashlib.sha256(json.dumps(self.source_tocs,sort_keys=True).encode()).hexdigest()
        raw=(ROOT/'work/client-analysis/continue-blueprint/global-scriptobjects.bin').read_bytes()
        self.names,p=names_at(raw,0);count=struct.unpack_from('<I',raw,p)[0];p+=4;self.objects={}
        for i in range(count):
            ni,nn,index,outer,cdo=struct.unpack_from('<IIQQQ',raw,p+i*32)
            self.objects[index]=(self.names[ni&0x3fffffff]+('_'+str(nn-1) if nn else ''),outer)
    def global_name(self,index):
        if index==0xffffffffffffffff:return ''
        if index not in self.objects:return hex(index)
        name,parent=self.objects[index];prefix=self.global_name(parent)
        return prefix+'.'+name if prefix else name
    def extract(self,package):
        name='../../../ShooterGame/Content/'+package.removeprefix('/Game/')+'.uasset'
        name=self.case_entries.get(name.casefold(),name)
        container,entry=self.entries[name];destination=self.output/'assets'/(package.removeprefix('/Game/')+'.uasset')
        provenance=destination.with_name(destination.name+'.catalog-source.json')
        expected={'container':container,'toc_sha256':entry['toc_sha256'],
            'chunk_id':entry['chunk_id'],'size':entry['size'],'path':name}
        cached=False
        if destination.exists() and provenance.exists():
            prior=json.loads(provenance.read_text(encoding='utf-8'))
            cached=all(prior.get(k)==v for k,v in expected.items())
            if cached:cached=hashlib.sha256(destination.read_bytes()).hexdigest()==prior.get('sha256')
        if not cached:
            original=self.stores[container].read(name)
            destination.parent.mkdir(parents=True,exist_ok=True)
            if not destination.exists() or destination.read_bytes()!=original:destination.write_bytes(original)
            provenance.write_text(json.dumps({**expected,'sha256':hashlib.sha256(original).hexdigest()},indent=2)+'\n',encoding='utf-8')
        return Zen(destination),container,entry
    def decode(self,package,seen=()):
        key=package.casefold()
        if key in self.decoded:return self.decoded[key]
        if key in seen or len(seen)>16:raise ValueError('Original DataAsset inheritance cycle or bound exceeded')
        seen=(*seen,key)
        z,container,entry=self.extract(package)
        cls=next((e for e in z.exports if self.global_name(int(e['class_index'],16))=='/Script/Engine.BlueprintGeneratedClass'),None)
        if cls is None:return None
        superclass=int(cls['super_index'],16);native=self.global_name(superclass);kind=KINDS.get(native);inherited={};parent_path=native
        if superclass>>62==2:
            parent_package=z.imported_packages[(superclass>>32)&0x3fffffff]
            base=self.decode(parent_package,seen)
            if base:
                # These original DataAsset subclasses add no Blueprint fields;
                # therefore inherited native property indices remain unchanged.
                class_bytes=z.content(cls);class_fields,class_start=unversioned(class_bytes)
                if class_fields or struct.unpack_from('<I',class_bytes,class_start)[0]!=0 or struct.unpack_from('<I',class_bytes,class_start+12)[0]!=0:
                    raise ValueError('Original DataAsset subclass adds an unverified reflected schema')
                kind=base['kind'];native=base['native_super'];inherited=dict(base['original_fields']);parent_path=base['class_path']
        if kind is None:self.decoded[key]=None;return None
        cdo=next(e for e in z.exports if e['name']=='Default__'+cls['name'])
        content=z.content(cdo);fields,start=unversioned(content);reader=Reader(z,content);reader.p=start
        values=dict(inherited);offsets={};unknown=[]
        for i,zero in fields:
            if i>=len(SCHEMAS[kind]):unknown.append(i);continue
            if unknown:raise ValueError('Unknown CDO field precedes required identity')
            name,typ=SCHEMAS[kind][i];offsets[name]=cdo['start']+reader.p;values[name]=reader.value(typ,zero)
        identity=values.get('uuid')
        if not identity or not any(identity['words']):raise ValueError('Original CDO has no nonzero serialized identity')
        exact=not unknown and reader.b[reader.p:]==b'\0'*4
        if not unknown and not exact:raise ValueError('Original primary DataAsset did not consume the exact export')
        row={'kind':kind,**identity,'package':package,'class_path':package+'.'+cls['name'],
            'native_super':native,'super_class_path':parent_path,'inherited_field_names':sorted(set(inherited)-set(offsets)),
            'source_container':container,'source_chunk_id':entry.get('chunk_id'),
            'extracted_path':self.output.joinpath('assets',package.removeprefix('/Game/')+'.uasset').relative_to(ROOT).as_posix(),
            'sha256':hashlib.sha256(z.data).hexdigest(),'original_fields':values,'serialized_field_offsets':offsets,
            'remaining_base_field_indices':unknown,'remaining_export_bytes':len(content)-reader.p,
            'exact_reflected_export_consumed':exact,
            'display_name':None,'display_name_status':'original_ui_data_not_yet_decoded',
            'icon':None,'icon_status':'original_ui_data_not_yet_decoded'}
        self.decoded[key]=row;return row
    def object_path(self,z,index):
        if not index:return None
        if index>0:return z.exports[index-1]['name']
        value=z.imports[-index-1]
        if value>>62==2:return z.imported_packages[(value>>32)&0x3fffffff]
        return self.global_name(value)
    def text_value(self,text):
        if text.get('history')!='string_table':return text.get('source')
        package=text['table'].split('.',1)[0]
        if package not in self.string_tables:
            z,_,_=self.extract(package);b=z.content(0);fields,start=unversioned(b)
            if fields:raise ValueError('Original StringTable has unexpected reflected fields')
            r=Reader(z,b);r.p=start
            if r.u32()!=0:raise ValueError('Original StringTable has unexpected object GUID marker')
            namespace=r.string();rows={}
            for _ in range(r.count()):key=r.string();rows[key]=r.string()
            self.string_tables[package]={'namespace':namespace,'rows':rows}
        return self.string_tables[package]['rows'].get(text['key'])
    def ui(self,path,kind,seen=None):
        package=path.split('.',1)[0];key=(package.casefold(),kind)
        if key in self.ui_cache:return self.ui_cache[key]
        seen=set() if seen is None else seen
        if key in seen or len(seen)>16:raise ValueError('Original UIData inheritance cycle or bound exceeded')
        seen.add(key)
        # The weapon analyzer owns the shared, original FText/native-struct
        # decoder. Skin/level/chroma property orders below come from this task's
        # separate pinned native UIData property metadata, not weapon UI fields.
        from analyze_all_weapons import UIReader,Metadata,chinese_strings
        if self.chinese is None:self.chinese,self.chinese_source=chinese_strings(self.paks,self.output)
        class SkinUIReader(UIReader):
            def text(self):
                value=super().text()
                if value['history']=='none':
                    # Native FTextHistory_None preserves an optional original
                    # culture-invariant FString. These are used by several
                    # authored "Standard" chromas; it is not an asset label.
                    present=self.i32()
                    if present not in (0,1):raise ValueError('Original FText invariant-string marker outside bool range')
                    value['culture_invariant_string_present']=bool(present)
                    if present:value['source']=self.string()
                return value
        if self.ui_metadata is None:self.ui_metadata=Metadata()
        z,_,_=self.extract(package)
        cls=next(e for e in z.exports if self.global_name(int(e['class_index'],16))=='/Script/Engine.BlueprintGeneratedClass')
        cdo=next(e for e in z.exports if e['name']=='Default__'+cls['name']);b=z.content(cdo);fields,start=unversioned(b)
        parent=int(cls['super_index'],16);result={}
        if parent>>62==2:
            parent_package=z.imported_packages[(parent>>32)&0x3fffffff]
            result.update(self.ui(parent_package,kind,seen))
        own={'skin':6,'level':1,'chroma':3,'weapon':0}[kind]
        r=SkinUIReader(z,b,self.ui_metadata);r.p=start
        result.update({'source_path':path,'sha256':hashlib.sha256(z.data).hexdigest(),'serialized_fields':[i for i,_ in fields]})
        for index,zero in fields:
            if zero:continue
            if index<own:
                if kind=='skin':
                    if index==0:result['use_model_viewer_overrides']=bool(r.take(1)[0])
                    elif index in (1,5):r.take(24)
                    else:r.take(4)
                elif kind=='level':result['level_item_value']=r.take(1)[0]
                elif kind=='chroma':result[['full_render','full_render_override','swatch'][index]]=self.object_path(z,r.i32())
                continue
            base=index-own
            if base in (0,1):
                text=r.text()
                if base==0:
                    result['display_text']=text;result['display_name']=self.text_value(text)
                    namespace=(self.string_tables[text['table'].split('.',1)[0]]['namespace']
                        if text.get('history')=='string_table' else text.get('namespace'))
                    result['display_namespace']=namespace
                    result['display_name_zh']=self.chinese.get((namespace,text.get('key')))
            elif base==2:result['icon']=self.object_path(z,r.i32())
            elif base==3:
                # Only present transforms need a schema. Read it from the pinned
                # native property descriptor; no texture relation is guessed.
                native=json.loads((ROOT/'work/all-skins/native/ui-data-properties.json').read_text())
                base_type=next(x for x in native if x['name']=='BaseContentUIData')
                field=next(x for x in base_type['properties'] if x['name']=='IconTransform')
                getter=int.from_bytes(bytes.fromhex(field['raw_hex'])[0x38:0x40],'little')-0x140000000
                schema=self.ui_metadata.structure(getter);members,used=unversioned(r.b[r.p:]);r.p+=used
                transform={}
                for slot,zflag in members:
                    member=schema['fields'][slot]
                    if zflag:transform[member['name']]=None;continue
                    if member['kind']==25 and self.ui_metadata.structure(member['structure_getter'])['name']=='Vector2D':
                        transform[member['name']]=list(struct.unpack('<2d',r.take(16)))
                    else:transform[member['name']]=r.native_value(member)
                result['icon_transform']=transform
            elif base==4:r.i32()
            else:raise ValueError(f'Original {kind} UIData property {index} outside verified schema')
        if len(b)-r.p!=4 or r.take(4)!=b'\0'*4:raise ValueError('Original UIData did not consume the exact export')
        if not result.get('icon') and result.get('full_render'):
            result['icon']=result['full_render'];result['icon_source_field']='FullRender'
        elif result.get('icon'):result['icon_source_field']='DisplayIcon'
        self.ui_cache[key]=result;return result
    def run(self,limit=0,include_ui=True):
        targets=[]
        for path in self.entries:
            if path.casefold().endswith('.uasset') and 'primaryasset' in path.rsplit('/',1)[-1].casefold() and any('/equippables/'+p+'/' in path.casefold() for p in ('guns','melee')):
                targets.append('/Game/'+path.split('/Content/',1)[1][:-7])
        targets.sort()
        if limit:targets=targets[:limit]
        for index,package in enumerate(targets):
            try:
                row=self.decode(package)
                if row:self.rows.append(row)
            except Exception as exc:self.errors.append({'package':package,'error':str(exc)})
            if index%100==0:print(json.dumps({'processed':index,'total':len(targets),'decoded':len(self.rows),'errors':len(self.errors)}),flush=True)
        if include_ui:
            for index,row in enumerate(self.rows):
                if row['kind']=='weapon':continue
                path=row['original_fields'].get('ui_data')
                if not path:continue
                try:
                    ui=self.ui(path,row['kind']);row['ui']=ui
                    row['display_name']=ui.get('display_name');row['icon']=ui.get('icon')
                    row['display_name_zh']=ui.get('display_name_zh')
                    row['display_name_status']=('original_string_table_source' if ui.get('display_text',{}).get('history')=='string_table'
                        else 'original_text_source') if row['display_name'] else 'original_text_has_no_source'
                    row['icon_status']='original_'+ui.get('icon_source_field','texture_reference_missing')
                except Exception as exc:self.ui_errors.append({'package':row['package'],'ui_data':path,'error':str(exc)})
                if index%100==0:print(json.dumps({'ui_processed':index,'total':len(self.rows),'ui_errors':len(self.ui_errors)}),flush=True)
        by_path={r['class_path'].casefold():r for r in self.rows};skins=[]
        for r in self.rows:
            if r['kind']!='skin':continue
            f=r['original_fields'];weapon=by_path.get((f.get('equippable') or '').casefold());levels=[];chromas=[]
            for ordinal,path in enumerate(f.get('levels',[]),1):
                item=by_path.get((path or '').casefold());levels.append({'ordinal':ordinal,'class_path':path,'path':path,'guid':item['guid'] if item else None,'words':item['words'] if item else None,'name':item.get('display_name') if item else None,'selectable':item is not None})
            for path in f.get('chromas',[]):
                item=by_path.get((path or '').casefold());chromas.append({'class_path':path,'path':path,'guid':item['guid'] if item else None,'words':item['words'] if item else None,'name':item.get('display_name') if item else None,'selectable':item is not None,'is_default':(path or '').casefold()==(f.get('default_chroma') or '').casefold()})
            skins.append({'guid':r['guid'],'words':r['words'],'class_path':r['class_path'],
                'equippable_class_path':f.get('equippable'),'equippable_guid':weapon['guid'] if weapon else None,
                'weapon_guid':weapon['guid'] if weapon else None,'weapon_words':weapon['words'] if weapon else None,
                'equippable_words':weapon['words'] if weapon else None,'levels':levels,'chromas':chromas,
                'default_chroma_class_path':f.get('default_chroma'),'ui_data':f.get('ui_data'),
                'display_name':r['display_name'],'display_name_status':r['display_name_status'],
                'display_name_zh':r.get('display_name_zh'),'display_name_zh_locale':'zh-TW' if r.get('display_name_zh') else None,
                'label':r['display_name'] or r['class_path'].rsplit('.',1)[-1],'label_source':'original_display_name' if r['display_name'] else 'original_asset_name',
                'icon':r['icon'],'icon_status':r['icon_status'],
                'relationships_resolved':bool(weapon and levels and all(x['guid'] for x in levels) and all(x['guid'] for x in chromas))})
            for item in levels+chromas:
                original=by_path.get((item.get('class_path') or '').casefold())
                if original:
                    item['name']=original.get('display_name');item['label']=item['name'] or original['class_path'].rsplit('.',1)[-1]
                    item['name_zh']=original.get('display_name_zh');item['display_name_zh']=item['name_zh']
                    item['icon']=original.get('icon');item['ui_data']=original['original_fields'].get('ui_data')
                    item['name_status']=original.get('display_name_status')
            skins[-1]['default_level_guid']=next((x['guid'] for x in levels if x['selectable']),None)
            skins[-1]['default_chroma_guid']=next((x['guid'] for x in chromas if x['is_default']),None)
            skins[-1]['selectable']=bool(weapon and any(x['selectable'] for x in levels)
                and skins[-1]['default_chroma_guid'])
            skins[-1]['authored_null_reference_count']=sum(not x.get('class_path') for x in levels+chromas)
            skins[-1]['unresolved_references']=[{'kind':kind,'class_path':x['class_path'],
                'reason':'authored_null' if not x['class_path'] else 'not_found_as_typed_dataasset_in_current_containers'}
                for kind,items in (('level',levels),('chroma',chromas)) for x in items if not x['selectable']]
            skins[-1]['base_content']=bool(f.get('base_content',False))
            skins[-1]['status']='local_assets_relationships_resolved' if skins[-1]['relationships_resolved'] else 'local_reference_unresolved'
            skins[-1]['runtime_supported']=None
            skins[-1]['runtime_status']='not_exercised_requires_native_content_index_validation'
        counts={k:sum(r['kind']==k for r in self.rows) for k in SCHEMAS}
        result={'schema_version':2,'source':'Original read-only local IoStore Zen CDOs','inventory_sha256':self.inventory_sha256,
            'inventory_source':'Current original .utoc directory indices; no historical inventory input','source_tocs':self.source_tocs,
            'input_installation':str(self.paks),'output_directory':str(self.output),'network_used':False,'target_executed':False,
            'guid_encoding':'Original FGuid four little-endian uint32 words; formatted AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE from words, not UUID byte guessing.',
            'complete_candidate_scan':not bool(limit),'candidate_packages':len(targets),'counts':counts,
            'resolved_skin_relationships':sum(s['relationships_resolved'] for s in skins),'skins':skins,'assets':self.rows,
            'errors':self.errors,'duplicate_input_paths':self.duplicates,
            'ui_decoded_count':len(self.ui_cache),'ui_errors':self.ui_errors,
            'chinese_localization':self.chinese_source,
            'limits':['Local serialized assets establish identity and authored relationships. Native ContentIndex inclusion and runtime apply are separate checks.',
                'Unknown display names/icons remain null, with the actual original UIData class path preserved. No labels or texture relationships are invented.',
                'DataAsset fields including Uuid, bHideIfNotOwned and bBaseContent are decoded with pinned native schemas; exact export consumption is checked.',
                'The static selectable flag means a complete local default choice exists, not that a native runtime application has been exercised.']}
        return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--paks',type=Path,default=Path('F:/Riot Games/VALORANT/live/ShooterGame/Content/Paks'))
    parser.add_argument('--output',type=Path,default=ROOT/'work/all-skins')
    parser.add_argument('--report',type=Path,default=ROOT/'reports/all-skins.json')
    parser.add_argument('--limit',type=int,default=0)
    parser.add_argument('--skip-ui',action='store_true')
    args=parser.parse_args()
    if args.report.resolve().drive.upper()!='F:':raise ValueError('Catalog report must remain on F:')
    catalog=Catalog(args.paks,args.output);result=catalog.run(args.limit,not args.skip_ui)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    temporary=args.report.with_name(args.report.name+'.tmp')
    temporary.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');temporary.replace(args.report)
    print(json.dumps({'report':str(args.report),'counts':result['counts'],'errors':len(result['errors']),'resolved':result['resolved_skin_relationships']}))


if __name__=='__main__':main()
