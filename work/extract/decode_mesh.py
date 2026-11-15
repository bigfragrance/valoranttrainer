"""Decode observed inline LOD0 buffers, with structural checks. No approximate geometry."""
from pathlib import Path
import struct,json,hashlib
import numpy as np
from zen import Zen

def u32(b,p):return struct.unpack_from('<I',b,p)[0]
def decode(path):
    z=Zen(path)
    export=next(e for e in z.exports if e['class_index']=='0x407cde1a593e47cf')
    b=z.content(export); candidates=[]
    for p in range(len(b)-16):
        if b[p:p+4]!=b'\x0c\0\0\0':continue
        n=u32(b,p+4)
        if not 3<=n<=2000000 or b[p:p+8]!=b[p+8:p+16] or p+16+n*12+26>len(b):continue
        q=p+16+n*12
        # FStaticMeshVertexBuffer header and tangent bulk array
        uvcount=u32(b,q+2); nv=u32(b,q+6); full=u32(b,q+10); high=u32(b,q+14)
        if nv!=n or not 1<=uvcount<=8 or full not in (0,1) or high not in (0,1):continue
        tangent_stride=u32(b,q+18); tangent_count=u32(b,q+22)
        if tangent_stride not in (8,16) or tangent_count!=n:continue
        q+=26+tangent_stride*n
        uvstride,uvn=struct.unpack_from('<II',b,q)
        if uvstride!=(8 if full else 4) or uvn!=n*uvcount:continue
        uvstart=q+8;q=uvstart+uvstride*uvn
        # FColorVertexBuffer strip flags, stride and vertex count
        colorstride,colors=struct.unpack_from('<II',b,q+2);q+=10
        if colors:
            elem,count=struct.unpack_from('<II',b,q);q+=8
            if elem!=4 or count!=n or colorstride!=4 or colors!=n:continue
            q+=count*4
        # FRawStaticIndexBuffer bool32bit then TArray<uint8>
        is32,elem,nbytes=struct.unpack_from('<III',b,q)
        if is32 not in (0,1) or elem!=1 or nbytes%(12 if is32 else 6) or q+12+nbytes>len(b):continue
        indices=np.frombuffer(b,dtype='<u4' if is32 else '<u2',count=nbytes//(4 if is32 else 2),offset=q+12)
        if len(indices)<3 or indices.max()>=n:continue
        verts=np.frombuffer(b,dtype='<f4',count=n*3,offset=p+16).reshape(-1,3)
        if not np.isfinite(verts).all() or np.abs(verts).max()>10000000:continue
        uv=np.frombuffer(b,dtype='<f4' if full else '<f2',count=uvn*2,offset=uvstart).reshape(n,uvcount,2)[:,0,:].astype(float)
        sections=[]
        for ns in range(1,64):
            sp=p-14-40*ns-4
            if sp<0 or u32(b,sp)!=ns:continue
            records=[struct.unpack_from('<10I',b,sp+4+40*j) for j in range(ns)]
            if records[0][1]!=0 or sum(s[2]*3 for s in records)!=len(indices):continue
            if any(s[0]>64 or s[1]+s[2]*3>len(indices) or s[4]>=n for s in records):continue
            sections=[dict(material=s[0],first=s[1],count=s[2]*3) for s in records];break
        materials=[]
        for nm in range(1,64):
            mp=len(b)-4-36*nm
            if mp<0 or u32(b,mp)!=nm:continue
            found=[]
            for j in range(nm):
                ref=struct.unpack_from('<i',b,mp+4+36*j)[0]
                if not -len(z.imports)<=ref<0:break
                obj=z.imports[-ref-1]
                if obj>>62!=2:break
                pi=(obj>>32)&0x3fffffff
                if pi>=len(z.imported_packages):break
                found.append(z.imported_packages[pi])
            if len(found)==nm:materials=found;break
        candidates.append(dict(name=export['name'],vertices=verts.astype(float).tolist(),indices=indices.astype(int).tolist(),uvs=uv.tolist(),sections=sections,materials=materials,source=str(path),source_sha256=hashlib.sha256(z.data).hexdigest(),position_buffer_offset=export['start']+p,index_buffer_offset=export['start']+q,imports=z.imported_packages,bounds=[verts.min(axis=0).tolist(),verts.max(axis=0).tolist()]))
    if not candidates:raise ValueError('No verified inline LOD')
    return candidates[0]

if __name__=='__main__':
    root=Path(__file__).parent; out=root/'meshes';out.mkdir(exist_ok=True)
    report=[]
    for source in ['related','dependencies','mesh-dependencies']:
        for p in (root/source).rglob('*.uasset'):
            try:
                z=Zen(p)
                if not any(e['class_index']=='0x407cde1a593e47cf' for e in z.exports):continue
                m=decode(p);dest=out/(m['name']+'.json');dest.write_text(json.dumps(m,separators=(',',':')),encoding='utf8')
                report.append(dict(name=m['name'],vertices=len(m['vertices']),triangles=len(m['indices'])//3,bounds=m['bounds'],path=str(dest)))
            except Exception as e:report.append(dict(source=str(p),error=str(e)))
    (out/'report.json').write_text(json.dumps(report,indent=2),encoding='utf8')
    print(json.dumps(report,indent=2))

