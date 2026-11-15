"""Recover observed blocking-volume component transforms, without guessing BSP.

The five BV_Pawn maps use BP_BlockingVolume static-mesh components. Positions
are UE centimeters and rotations UE pitch/yaw/roll degrees. Component defaults
resolve through the exported BP_BlockingVolume's Cube template to Box_For_Volumes.
"""
import json,struct,hashlib
from pathlib import Path
from zen import Zen

def unversioned(b):
    p=0; idx=0; fragments=[]; zeros=0
    while True:
        v=struct.unpack_from('<H',b,p)[0];p+=2
        idx+=v&127; count=v>>9
        fragments.append((idx,count,bool(v&128)))
        idx+=count
        if v&128: zeros+=count
        if v&256: break
    size=(1 if zeros<=8 else 2 if zeros<=16 else ((zeros+31)//32)*4) if zeros else 0
    mask=int.from_bytes(b[p:p+size],'little');p+=size
    bit=0;props=[]
    for idx,count,haszero in fragments:
        for i in range(idx,idx+count):
            zero=bool(mask&(1<<bit)) if haszero else False
            bit+=haszero
            props.append((i,zero))
    return props,p

def mesh_reference(z,ref):
    if ref==0:return None
    if ref>0:return {'export':ref-1}
    value=z.imports[-ref-1]
    if value>>62==2:
        package_index=(value>>32)&0x3fffffff
        return z.imported_packages[package_index]
    return hex(value)

def component(z,e):
    b=z.content(e);props,p=unversioned(b)
    result=dict(source_export=e['index'],source_byte_offset=e['start'],name=z.exports[int(e['outer'],16)]['name'],mesh='/Game/Blueprints/Utilities/Box_For_Volumes',location=[0.,0.,0.],rotation=[0.,0.,0.],scale=[1.,1.,1.],property_offsets={})
    for idx,zero in props:
        if idx>169:break
        if zero:
            if idx in (167,168,169):result[{167:'location',168:'rotation',169:'scale'}[idx]]=[0.,0.,0.]
            continue
        result['property_offsets'][str(idx)]=e['start']+p
        if idx==5:
            result['mesh']=mesh_reference(z,struct.unpack_from('<i',b,p)[0]);p+=4
        elif idx==35:
            n=struct.unpack_from('<I',b,p)[0]
            assert n<1000
            p+=4
            result['materials']=[mesh_reference(z,struct.unpack_from('<i',b,p+j*4)[0]) for j in range(n)]
            p+=4*n
        elif idx==123:p+=1
        elif idx in (167,168,169):
            result[{167:'location',168:'rotation',169:'scale'}[idx]]=list(struct.unpack_from('<3d',b,p));p+=24
        else:raise ValueError('Unknown nonzero property '+str(idx))
    return result

def main():
    output=[]
    for path in Path('raw').rglob('*_BV_Pawn.umap'):
        z=Zen(path);parts=[];errors=[]
        for e in z.exports:
            if e['class_index']!='0x57defc7857c66b65':continue
            try:parts.append(component(z,e))
            except Exception as ex:errors.append(dict(index=e['index'],name=e['name'],error=str(ex)))
        result=dict(source=str(path),source_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),units='UE centimeters',rotation='UE pitch,yaw,roll degrees',components=parts,errors=errors,coverage={'decoded':len(parts),'undecoded':len(errors)},limitations=['BP default static mesh inferred through dependency package and Cube template; default mesh and defaults require referenced package to remain identical.','Includes designated pawn-blocking volume layer only; art and layout collision are separate.','Transform values are serialized bytes, not runtime collision verification.'])
        output.append(result)
        print(path,len(parts),len(errors),errors[:4])
    Path('collision-transforms.json').write_text(json.dumps(output,indent=2),encoding='utf-8')

if __name__=='__main__':main()
