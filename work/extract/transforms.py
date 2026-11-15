"""Source-bound static mesh component transform extraction for selected map layers."""
from pathlib import Path
import struct,json,hashlib,math
from zen import Zen
from decode_collision import component,unversioned,mesh_reference

def skip_struct(b,p,schema):
    fields,n=unversioned(b[p:]);p+=n
    for i,zero in fields:
        if zero:continue
        if i not in schema:raise ValueError('Unknown nested property '+str(i))
        kind=schema[i]
        if isinstance(kind,int):p+=kind
        elif isinstance(kind,dict):p=skip_struct(b,p,kind)
        elif isinstance(kind,list):
            count=struct.unpack_from('<I',b,p)[0];p+=4
            if count>10000:raise ValueError('Invalid nested count')
            for _ in range(count):p=skip_struct(b,p,kind[0])
        else:raise ValueError('Invalid schema')
    if p>len(b):raise ValueError('Out of bounds struct')
    return p

def parse_component(z,e):
    b=z.content(e);props,p=unversioned(b)
    indexes=[i for i,zero in props]
    if e['name']=='Cube':return component(z,e)
    if any(i>169 and i!=193 and not zero for i,zero in props):
        raise ValueError('Unknown properties after transform '+str([x for x in props if x[0]>169]))
    transform=[(i,zero) for i,zero in props if i in (167,168,169)]
    if not transform:raise ValueError('No serialized transform')
    result=dict(source_export=e['index'],source_byte_offset=e['start'],name=z.exports[int(e['outer'],16)]['name'],mesh=None,location=[0.,0.,0.],rotation=[0.,0.,0.],scale=[1.,1.,1.],property_offsets={},method='parsed observed property slots and nested structures')
    for i,zero in props:
        if i>=167:break
        if zero:continue
        result['property_offsets'][str(i)]=e['start']+p
        if i in (0,28,134):p+=4
        elif i==163:
            ref=struct.unpack_from('<i',b,p)[0];p+=4
            result['attach_parent_ref']=ref
            if ref>0:
                parent=z.exports[ref-1]
                parentprops,_=unversioned(z.content(parent))
                result['attach_parent_name']=parent['name']
                result['attach_parent_payload_bytes']=parent['size']
                result['attach_parent_transform_status']='identity inferred from default SceneComponent and no room for serialized vector' if parent['class_index']=='0x6173746e69679569' and parent['size']<24 else 'unresolved'
        elif i==5:
            result['mesh']=mesh_reference(z,struct.unpack_from('<i',b,p)[0]);p+=4
        elif i in (21,25,54,72,78,82,119,120,121,123):p+=1
        elif i==34:p=skip_struct(b,p,{0:1,9:1})
        elif i==33:
            count=struct.unpack_from('<I',b,p)[0];p+=4
            if count>10000:raise ValueError('Invalid streaming info count')
            for _ in range(count):p=skip_struct(b,p,{0:4,1:4,2:4})
        elif i==35:
            count=struct.unpack_from('<I',b,p)[0];p+=4
            result['materials']=[mesh_reference(z,struct.unpack_from('<i',b,p+j*4)[0]) for j in range(count)]
            p+=4*count
        elif i==143:p=skip_struct(b,p,{21:8,24:{1:[{0:8,1:1}]}})
        else:raise ValueError('Unknown property '+str(i))
    if not result['mesh']:raise ValueError('Missing mesh')
    start=p
    for i,zero in transform:
        value=[0.,0.,0.] if zero else list(struct.unpack_from('<3d',b,start))
        if not all(math.isfinite(v) and abs(v)<1e8 for v in value):raise ValueError('Invalid transform value')
        result[{167:'location',168:'rotation',169:'scale'}[i]]=value
        if not zero:result['property_offsets'][str(i)]=e['start']+start;start+=24
    for i,zero in props:
        if i==193 and not zero:
            result['slot193_byte']=b[start];start+=1
    if b[start:start+8]!=bytes(8):raise ValueError('Native suffix boundary did not validate')
    result['native_suffix_offset']=e['start']+start
    return result

def collect():
    layers=[]
    for path in Path('raw').rglob('*.umap'):
        if not any(x in path.stem for x in ('_Layout','_Art_FloorA','_Art_FrameA','_Art_ActivationsA','_Art_RoofA','_Art_FoliageA','_BV_Pawn')):continue
        z=Zen(path);parts=[];errors=[]
        for e in z.exports:
            if e['class_index']!='0x57defc7857c66b65':continue
            try:parts.append(parse_component(z,e))
            except Exception as ex:errors.append(dict(export=e['index'],name=e['name'],error=str(ex)))
        layer=dict(source=str(path),source_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),units='UE centimeters',rotation='UE pitch,yaw,roll degrees',components=parts,errors=errors)
        layers.append(layer)
        print(str(path),len(parts),'decoded',len(errors),'unknown',str(errors[:2]))
    Path('scene-transforms.json').write_text(json.dumps(layers,indent=2),encoding='utf8')
    Path('mesh-references.json').write_text(json.dumps(sorted(set(c['mesh'] for l in layers for c in l['components'] if isinstance(c['mesh'],str))),indent=2),encoding='utf8')
    return layers

if __name__=='__main__':collect()
