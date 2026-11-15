"""Plan original resource blocks for the current complete trainer catalog."""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict, deque
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'work/extract'))
from iostore import IoStore
from analyze_share_resources import package_name, parse_header, merged_ranges, consecutive
from zen import names_at


def lite_scope():
    skins = json.loads((ROOT/'reports/all-skins.json').read_text(encoding='utf-8'))
    weapons = json.loads((ROOT/'reports/all-weapons.json').read_text(encoding='utf-8'))['weapons']
    heroes = json.loads((ROOT/'profiles/playable-heroes.json').read_text(encoding='utf-8'))['heroes']
    tiers = {row['guid'].replace('-',''): row.get('original_fields',{}).get('content_tier')
             for row in skins['assets'] if row['kind']=='skin'}
    defaults = {row['default_skin_class_path'] for row in weapons if row.get('default_skin_class_path')}
    accepted = {'/Game/ContentTiers/Premium_PrimaryAsset.Premium_PrimaryAsset_C': 'Premium',
                '/Game/ContentTiers/Exclusive_PrimaryAsset.Exclusive_PrimaryAsset_C': 'Exclusive'}
    rows = list(skins['skins'].values()) if isinstance(skins['skins'],dict) else skins['skins']
    selected = [row for row in rows if row['class_path'] in defaults or tiers.get(row['guid'].replace('-','')) in accepted]
    counts = Counter(accepted.get(tiers.get(row['guid'].replace('-','')),'base') for row in selected)
    families = sorted({row['class_path'].split('.',1)[0].rsplit('/',1)[0]+'/' for row in selected})
    bases = sorted({row['weapon_class_path'].split('.',1)[0].rsplit('/',1)[0]+'/' for row in weapons})
    codes = {row['code'] for row in heroes}
    return {'edition':'duel1-lotus-premium-exclusive','maps':['duel1','jam'],
            'skin_ids':[row['guid'].replace('-','') for row in selected],
            'skin_count':len(selected),'skin_tier_counts':dict(counts),'hero_count':len(heroes),
            'families':families,'weapon_bases':bases,'hero_codes':sorted(codes),
            'content_tier_classes':list(accepted),
            'tier_source':'Original captured EquippableSkinDataAsset.content_tier; Premium and Exclusive plus base weapon skins.'}


def plan(destination, source, edition='full', runtime_logs=()):
    previous = json.loads((ROOT/'reports/portable-resource-plan.json').read_text(encoding='utf-8'))
    maps = json.loads((ROOT/'reports/training-maps.json').read_text(encoding='utf-8'))['maps']
    scope = lite_scope() if edition=='lite-rare' else None
    map_prefixes = [row['world_path'].rsplit('/',1)[0]+'/' for row in maps if row['kind']=='standard_5v5'
                    and (scope is None or row['id'] in scope['maps'])]
    map_prefixes.append('/Game/Maps/Duel/Duel_1/')
    prefixes = ('/Engine/', '/Game/Characters/', '/Game/Equippables/', '/Game/UI/',
                '/Game/Globals/', '/Game/Settings/', '/Game/GameModes/', '/Game/Gameplay/',
                '/Game/Audio/', '/Game/WwiseAudio/', '/Game/FX/', '/Game/VFX/',
                '/Game/Animation/', '/Game/Animations/', *map_prefixes)
    if scope:
        prefixes = ('/Engine/','/Game/Characters/_Core/','/Game/Equippables/_Core/',
                    '/Game/Equippables/Guns/_Core/','/Game/Equippables/Guns/Attachments/',
                    '/Game/UI/InGame/','/Game/UI/Screens/InGame/','/Game/UI/Screens/Shared/',
                    '/Game/UI/Settings/','/Game/Globals/','/Game/Settings/','/Game/ContentTiers/',
                    '/Game/Equippables/Finishers/','/Game/ContextAwareModule/',
                    '/Game/GameModes/','/Game/Maps/_Core/',*map_prefixes,*scope['families'])

    def family_for(package,path):
        family = next((prefix for prefix in prefixes if package.startswith(prefix)),None)
        if family is None and '/Plugins/' in path:
            family = 'all engine/project plugins'
        if scope and family is None:
            for code in scope['hero_codes']:
                prefix='/Game/Characters/'+code+'/'
                if package.startswith(prefix) and ('/' not in package[len(prefix):] or package.startswith(prefix+'S0/')):
                    return 'playable heroes default models and S0'
            for base in scope['weapon_bases']:
                if package.startswith(base):
                    tail=package[len(base):]
                    child=tail.split('/',1)[0]
                    if '/' not in tail or child in ('_Core','Attachments','Meshes','Models','Materials','Textures',
                                                    'Audio','Sounds','Animations','Animation','VFX','FX','Projectiles'):
                        return 'all base weapon classes and shared components'
        return family
    stores, paths, path_ids, dependencies = {}, {}, defaultdict(set), defaultdict(set)
    package_groups, groups = {}, Counter()
    paks = source/'ShooterGame/Content/Paks'
    for toc in sorted(paks.glob('*.utoc')):
        io = IoStore(toc)
        expected = previous['container_evidence'].get(toc.name, {}).get('toc_sha256')
        actual = hashlib.sha256(io.data).hexdigest()
        if actual != expected:
            raise RuntimeError('The known-good base plan belongs to other resource indices: '+toc.name)
        stores[toc.name] = io
        for path, index in io.files.items():
            chunk = bytes.fromhex(io.ids[index])
            if chunk[-1] != 1:
                continue
            package = package_name(path)
            if package is None:
                continue
            pid = int.from_bytes(chunk[:8], 'little')
            paths[pid] = package
            path_ids[package.casefold()].add(pid)
            family = family_for(package,path)
            if family:
                package_groups[pid] = family
                groups[family] += 1
        for index, cid in enumerate(io.ids):
            if bytes.fromhex(cid)[-1] != 6:
                continue
            io.files['__planner_header__'] = index
            graph, _ = parse_header(io.read('__planner_header__'))
            for pid, children in graph.items():
                dependencies[pid].update(children)
        print(json.dumps({'indexed':toc.name,'packages':len(paths)}), flush=True)
    # Keep the previously exercised startup/catalog paths and exact blocks,
    # then extend them for all current maps, heroes, skins, and original UI.
    prior = previous['profiles']['observed_catalog_closure']
    seeds = {int(pid,16) for pid in prior['selected_package_ids']} | set(package_groups)
    literals = set()
    for path in (ROOT/'native').iterdir():
        if path.suffix not in ('.h','.hpp','.inl','.cpp'):
            continue
        for value in re.findall(r'"(/(?:Game|Engine)/[^"\s\\]+)"',path.read_text(encoding='utf-8',errors='replace')):
            if scope and value.startswith('/Game/Maps/') and not value.startswith(('/Game/Maps/_Core/',*map_prefixes)):
                continue
            literals.update(path_ids.get(value.split('.',1)[0].casefold(),()))
    seeds.update(literals)
    requested_blocks=defaultdict(set)
    runtime_evidence=[]
    for path in runtime_logs:
        text=path.read_text(encoding='utf-8',errors='replace')
        before=len(seeds)
        for package in re.findall(r'StartBundleIoRequests: FailedRead:\s+(/(?:Game|Engine)/[^\s(]+)',text):
            seeds.update(path_ids.get(package.split('.',1)[0].casefold(),()))
        found=0
        for container,block in re.findall(r"Signature error detected in container '([^']+)' at block index '(\d+)'",text):
            toc=container+'.utoc'
            index=int(block)
            if toc not in stores or index>=len(stores[toc].blocks):
                raise RuntimeError('Runtime requested block does not belong to this pinned source')
            requested_blocks[toc].add(index)
            found+=1
        runtime_evidence.append({'log_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
                                 'named_roots_added':len(seeds)-before,'block_error_receipts':found})
    soft_metadata = {'verified_asset_files':0,'path_references':0,'additional_roots':0}
    if scope:
        catalog=json.loads((ROOT/'reports/all-skins.json').read_text(encoding='utf-8'))
        rows=list(catalog['skins'].values()) if isinstance(catalog['skins'],dict) else catalog['skins']
        wanted=set(scope['skin_ids'])
        selected_rows=[row for row in rows if row['guid'].replace('-','') in wanted]
        for row in selected_rows:
            wanted.update(item['guid'].replace('-','') for item in (*row['levels'],*row['chromas']) if isinstance(item.get('guid'),str))
        paths_from_metadata=set()
        def strings(value):
            if isinstance(value,str):
                if value.startswith(('/Game/','/Engine/')):
                    paths_from_metadata.add(value.split('.',1)[0])
            elif isinstance(value,dict):
                for item in value.values():strings(item)
            elif isinstance(value,list):
                for item in value:strings(item)
        strings(selected_rows)
        strings(json.loads((ROOT/'reports/all-weapons.json').read_text(encoding='utf-8'))['weapons'])
        for asset in catalog['assets']:
            if asset['guid'].replace('-','') not in wanted:
                continue
            strings(asset)
            path=ROOT/asset['extracted_path']
            data=path.read_bytes()
            if hashlib.sha256(data).hexdigest()!=asset['sha256']:
                raise RuntimeError('Captured selected skin metadata changed: '+asset['package'])
            names,_=names_at(data,52)
            strings(names)
            soft_metadata['verified_asset_files']+=1
        extra=set()
        for package in paths_from_metadata:
            extra.update(path_ids.get(package.casefold(),()))
        soft_metadata.update(path_references=len(paths_from_metadata),additional_roots=len(extra-seeds))
        seeds.update(extra)
    selected = set(seeds)
    queue = deque(seeds)
    while queue:
        for child in dependencies.get(queue.popleft(),()):
            if child not in selected:
                selected.add(child)
                queue.append(child)
    old_blocks = {}
    for row in prior['offset_preserving_sparse_candidate']['manifest']:
        old_blocks[row['toc']] = {n for first,last in row['block_index_intervals_inclusive'] for n in range(first,last+1)}
    manifest, chunk_types = [], Counter()
    for name, io in stores.items():
        blocks = set(old_blocks.get(name,()))
        blocks.update(requested_blocks.get(name,()))
        companions = set()
        for path,index in io.files.items():
            if bytes.fromhex(io.ids[index])[-1] in (2,3,4):
                if path_ids.get((package_name(path) or '').casefold(),set()) & selected:
                    companions.add(index)
        kept_chunks = 0
        for index,cid in enumerate(io.ids):
            chunk = bytes.fromhex(cid)
            kind,pid = chunk[-1],int.from_bytes(chunk[:8],'little')
            if kind in (1,2,3,4) and pid not in selected and index not in companions:
                continue
            offset,length = io.ranges[index]
            blocks.update(range(offset//io.block_size,(offset+length+io.block_size-1)//io.block_size))
            kept_chunks += 1
            chunk_types[kind] += 1
        if io.partition_count != 1 or not io.block_hashes:
            raise RuntimeError('Standalone block copy requires the known signed single-partition source')
        ranges = merged_ranges((io.blocks[n][0],(io.blocks[n][1]+15)//16*16) for n in sorted(blocks))
        payload = sum(size for _,size in ranges)
        allocated = merged_ranges((offset//65536*65536,((offset+size+65535)//65536-offset//65536)*65536) for offset,size in ranges)
        manifest.append({'file':io.path.with_suffix('.ucas').name,'toc':name,'toc_sha256':hashlib.sha256(io.data).hexdigest(),
                         'logical_bytes':io.path.with_suffix('.ucas').stat().st_size,'payload_bytes':payload,
                         'estimated_allocated_bytes':sum(n for _,n in allocated),'selected_blocks':len(blocks),
                         'total_blocks':len(io.blocks),'kept_chunks':kept_chunks,
                         'block_intervals':consecutive(blocks),'physical_ranges':ranges})
        print(json.dumps({'container':name,'payload_bytes':payload,'selected_blocks':len(blocks),'total_blocks':len(io.blocks)}),flush=True)
    passthrough = [{'file':p.name,'bytes':p.stat().st_size} for p in sorted(paks.iterdir()) if p.is_file() and p.suffix in ('.utoc','.pak','.sig')]
    report = {'schema_version':1,'kind':'trainer-full-catalog-original-block-plan-v1',
              'created_utc':datetime.now(timezone.utc).isoformat(),'source_root':str(source),
              'image_sha256':json.loads((ROOT/'project.json').read_text())['expected_snapshot_sha256'],
              'all_indexed_packages':len(paths),'root_packages':len(seeds),'selected_packages':len(selected),
              'selected_package_ids':[f'{pid:016x}' for pid in sorted(selected)],
              'root_groups':dict(groups),'literal_native_roots':len(literals),
              'edition':edition,'scope':scope,
              'selected_skin_soft_metadata':soft_metadata,
              'runtime_dependency_evidence':runtime_evidence,
              'chunk_types':dict(chunk_types),'containers':manifest,'passthrough':passthrough,
              'payload_bytes':sum(x['payload_bytes'] for x in manifest),
              'passthrough_bytes':sum(x['bytes'] for x in passthrough),
              'estimated_allocated_ucas_bytes':sum(x['estimated_allocated_bytes'] for x in manifest),
              'source_resource_bytes':sum(p.stat().st_size for p in paks.iterdir() if p.is_file()),
              'validation':'candidate_only_not_runtime_verified',
              'qualification':'Broad complete feature families plus hard imports, known-good base blocks, and all shared header/shader chunks. Dynamic soft dependencies still require native validation.'}
    report['resource_bundle_bytes'] = report['payload_bytes']+report['passthrough_bytes']
    destination.parent.mkdir(parents=True,exist_ok=True)
    destination.write_text(json.dumps(report,separators=(',',':')),encoding='utf-8')
    print(json.dumps({k:report[k] for k in ('all_indexed_packages','selected_packages','payload_bytes','passthrough_bytes','resource_bundle_bytes','estimated_allocated_ucas_bytes','source_resource_bytes')}),flush=True)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--source',type=Path,default=Path('F:/Riot Games/VALORANT/live'))
    parser.add_argument('--edition',choices=('full','lite-rare'),default='full')
    parser.add_argument('--runtime-log',type=Path,action='append',default=[])
    args=parser.parse_args()
    plan(args.output.resolve(),args.source.resolve(strict=True),args.edition,args.runtime_log)
