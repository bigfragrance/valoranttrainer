"""Assemble extracted triangle buffers using structurally decoded original placements."""
from pathlib import Path
import json, math, shutil
import numpy as np

ROOT=Path(__file__).parent
OUT=ROOT.parent/'trainer'/'scenes';OUT.mkdir(parents=True,exist_ok=True)

def transform(vertices,c):
    pitch,yaw,roll=map(math.radians,c['rotation'])
    cp,sp,cy,sy,cr,sr=math.cos(pitch),math.sin(pitch),math.cos(yaw),math.sin(yaw),math.cos(roll),math.sin(roll)
    mat=np.array([[cp*cy,cp*sy,sp],[sr*sp*cy-cr*sy,sr*sp*sy+cr*cy,-sr*cp],[-(cr*sp*cy+sr*sy),cy*sr-cr*sp*sy,cr*cp]])
    v=(np.array(vertices)*c['scale'])@mat+np.array(c['location'])
    return (v*np.array([.01,-.01,.01])).round(6).tolist()

def main():
    layers=json.loads((ROOT/'scene-transforms.json').read_text())
    bindings={k.lower():v for k,v in json.loads((ROOT/'textures'/'materials.json').read_text()).items()}
    mesh_cache={}
    report=[]
    for number,folder in enumerate(['Duel_1','Duel_2','Duel_3','Duel_Platform','Duel_Heady'],1):
        scene=dict(name=f'斗牛 {number} · Skirmish {"ABCDE"[number-1]}',units='m',spawn=[13.8,0,3.65],look_at=[0,0,3.65],meshes=[],provenance=[])
        missing=[]
        selected=[x for x in layers if ('\\'+folder+'\\') in x['source'] or (folder=='Duel_Platform' and x['source']=='raw\\Duel_Layout_Platform.umap')]
        for layer in selected:
            scene['provenance'].append(dict(source=layer['source'],sha256=layer['source_sha256']))
            collision_only='BV_Pawn' in layer['source']
            for c in layer['components']:
                name=c['mesh'].split('/')[-1]
                path=ROOT/'meshes'/(name+'.json')
                if not path.exists():missing.append(c['mesh']);continue
                if name not in mesh_cache:mesh_cache[name]=json.loads(path.read_text())
                m=mesh_cache[name]
                vertices=transform(m['vertices'],c)
                sections=m.get('sections') or [dict(material=0,first=0,count=len(m['indices']))]
                for section in sections:
                    indices=np.array(m['indices'][section['first']:section['first']+section['count']]).reshape(-1,3)
                    # Unreal to Panda reflection reverses winding; negative scales reverse it once more.
                    if np.prod(c['scale'])>0:indices=indices[:,[0,2,1]]
                    # Keep only section vertices to limit memory and JSON size.
                    used=np.unique(indices); remap={int(old):j for j,old in enumerate(used)}
                    indices=[remap[int(i)] for i in indices.reshape(-1)]
                    mat_index=section['material']; mats=c.get('materials') or m.get('materials') or m['imports']
                    material=mats[mat_index] if mat_index<len(mats) else ''
                    if not material and mat_index<len(m.get('materials',[])):material=m['materials'][mat_index]
                    binding=bindings.get((material or '').lower())
                    sm=dict(name=c['name']+'/'+name+f'/material{mat_index}',vertices=[vertices[i] for i in used],indices=indices,color=[.76,.78,.79,1],visible=not collision_only,collision=True,source_mesh=m['source_sha256'],source_export=c['source_export'])
                    if collision_only:sm['collision_type']='pawn'
                    if not collision_only and binding:
                        src=Path(binding['texture']);dst=OUT/'textures'/src.name;dst.parent.mkdir(exist_ok=True)
                        if not dst.exists():shutil.copy2(src,dst)
                        sm['texture']='textures/'+src.name;sm['uvs']=[[m['uvs'][i][0],1-m['uvs'][i][1]] for i in used];sm['color']=[1,1,1,1]
                    scene['meshes'].append(sm)
        scene['limitations']=['Original map meshes and placements; reconstructed rendering and physics.','Layered Unreal materials, lighting and VFX are not reproduced.','Map numbering inferred from Skirmish A–E and Duel folder structure.']
        scene['missing_meshes']=sorted(set(missing))
        dest=OUT/f'duel-{number}.json';dest.write_text(json.dumps(scene,separators=(',',':'),ensure_ascii=False),encoding='utf8')
        report.append(dict(scene=dest.name,meshes=len(scene['meshes']),triangles=sum(len(m['indices'])//3 for m in scene['meshes']),textured=sum('texture'in m for m in scene['meshes']),missing=scene['missing_meshes']))
    (ROOT/'scene-build-report.json').write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding='utf8');print(json.dumps(report,indent=2,ensure_ascii=False))

if __name__=='__main__':main()
