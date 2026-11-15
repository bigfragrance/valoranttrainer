from pathlib import Path
import json
from iostore import IoStore

inv=json.loads(Path('inventory.json').read_text())
refs=json.loads(Path('mesh-references.json').read_text())
lookup={f['path'].casefold():(bucket['container'],f['path']) for bucket in inv for f in bucket['files']}
ios={};results={};missing=[]
for ref in refs:
    if ref.startswith('/Game/'):stem='../../../ShooterGame/Content/'+ref[6:]
    elif ref.startswith('/Engine/'):stem='../../../Engine/Content/'+ref[8:]
    else:missing.append(dict(ref=ref,error='not a package'));continue
    pair=lookup.get((stem+'.uasset').casefold())
    if pair is None:missing.append(dict(ref=ref,error='not indexed'));continue
    container,p=pair
    if container not in ios:ios[container]=IoStore(Path(r'F:\Riot Games\VALORANT\live\ShooterGame\Content\Paks')/container)
    io=ios[container]
    out=Path('mesh-dependencies')/(ref[1:]+'.uasset')
    try:
        meta=io.export(p,out)
        results[ref]=dict(path=str(out.resolve()),sha256=meta['sha256'],size=meta['size'])
    except Exception as ex:
        record=dict(ref=ref,error=str(ex),metadata=io.metadata(p))
        missing.append(record)
        print('error',ref,str(ex))
Path('mesh-files.json').write_text(json.dumps(results,indent=2))
Path('mesh-extraction-errors.json').write_text(json.dumps(missing,indent=2))
print('exported',len(results),'missing',len(missing),'bytes',sum(x['size'] for x in results.values()))
