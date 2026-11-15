"""Copy selected, signature-verified original blocks into one dependency file."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import sys
import zipfile

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'work/extract'))
from iostore import IoStore


def dump(path,value):
    path.write_text(json.dumps(value,ensure_ascii=False,separators=(',',':'))+'\n',encoding='utf-8')


def export(plan_path,target):
    plan=json.loads(plan_path.read_text(encoding='utf-8'))
    if target.exists():
        raise RuntimeError('Dependency output already exists')
    source=Path(plan['source_root'])/'ShooterGame/Content/Paks'
    manifest={'schema_version':1,'kind':'original-signed-ranges-v1',
              'created_utc':datetime.now(timezone.utc).isoformat(),'scope':plan['scope'],
              'image_sha256':plan['image_sha256'],'containers':[],'passthrough':[],
              'original_indices_and_signatures_unchanged':True,
              'original_selected_block_sha1_verified':True,
              'source_installation_modified':False}
    target.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(target,'x',zipfile.ZIP_STORED,allowZip64=True) as bundle:
        for item in plan['containers']:
            io=IoStore(source/item['toc'])
            if hashlib.sha256(io.data).hexdigest()!=item['toc_sha256'] or io.partition_count!=1 or not io.block_hashes:
                raise RuntimeError('Original container identity changed')
            member='payload/'+item['file']+'.bin'
            record={key:item[key] for key in ('file','toc','toc_sha256','logical_bytes','payload_bytes','selected_blocks','total_blocks')}
            record.update(member=member,ranges=[])
            selected=[n for first,last in item['block_intervals'] for n in range(first,last+1)]
            cursor=0
            with (source/item['file']).open('rb',buffering=16*1024*1024) as incoming,bundle.open(member,'w',force_zip64=True) as outgoing:
                for offset,size in item['physical_ranges']:
                    incoming.seek(offset)
                    at=offset
                    end=offset+size
                    sha=hashlib.sha256()
                    while at<end:
                        block=selected[cursor]
                        physical,compressed,_,_=io.blocks[block]
                        length=(compressed+15)//16*16
                        if physical!=at or at+length>end:
                            raise RuntimeError('Resource range is not an exact original block union')
                        data=incoming.read(length)
                        if len(data)!=length or hashlib.sha1(data).digest()!=io.block_hashes[block]:
                            raise RuntimeError('Original signed block differs from its untouched UTOC')
                        outgoing.write(data)
                        sha.update(data)
                        at+=length
                        cursor+=1
                    record['ranges'].append([offset,size,sha.hexdigest()])
            if cursor!=len(selected):
                raise RuntimeError('Selected block coverage is incomplete')
            manifest['containers'].append(record)
            print(json.dumps({'copied':item['file'],'bytes':item['payload_bytes'],'blocks_verified':len(selected)}),flush=True)
        for item in plan['passthrough']:
            path=source/item['file']
            member='paks/'+item['file']
            sha=hashlib.sha256()
            with path.open('rb',buffering=16*1024*1024) as incoming,bundle.open(member,'w',force_zip64=True) as outgoing:
                while data:=incoming.read(1024*1024):
                    sha.update(data)
                    outgoing.write(data)
            manifest['passthrough'].append({**item,'member':member,'sha256':sha.hexdigest()})
        payload=(json.dumps(manifest,ensure_ascii=False,separators=(',',':'))+'\n').encode('utf-8')
        bundle.writestr('dependency-manifest.json',payload,compress_type=zipfile.ZIP_DEFLATED)
    with target.open('rb') as stream:
        digest=hashlib.file_digest(stream,'sha256').hexdigest()
    record={'file':target.name,'bytes':target.stat().st_size,'sha256':digest,
            'manifest_sha256':hashlib.sha256(payload).hexdigest(),
            'resource_payload_bytes':sum(row['payload_bytes'] for row in manifest['containers']),
            'passthrough_bytes':sum(row['bytes'] for row in manifest['passthrough'])}
    target.with_name(target.name+'.manifest.json').write_bytes(payload)
    dump(target.with_name(target.name+'.json'),record)
    target.with_name(target.name+'.sha256.txt').write_text(digest+'  '+target.name+'\n',encoding='utf-8')
    print(json.dumps(record),flush=True)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plan',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    export(args.plan.resolve(strict=True),args.output.resolve())
