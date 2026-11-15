"""Read local Zen texture mipmaps and convert diffuse textures without network access.

Material selection is an explicit preview approximation: it reuses a diffuse
texture; it does not reproduce Unreal layered shader graphs or scalar tuning.
"""
from pathlib import Path
import hashlib, io, json, re, struct
from PIL import Image
from zen import Zen, names_at
from iostore import IoStore

ROOT=Path(__file__).resolve().parent
PAKS=Path(r'F:\Riot Games\VALORANT\live\ShooterGame\Content\Paks')
TEXTURES=ROOT/'textures'
DEPS=ROOT/'texturedeps'

class Source:
    def __init__(self):
        inventory=json.loads((ROOT/'inventory.json').read_text())
        self.index={f['path'].lower():(c['container'],f) for c in inventory for f in c['files']}
        self.stores={}
        self.local={}
        for where in ('related','dependencies','mesh-dependencies','texturedeps'):
            for p in (ROOT/where).rglob('*.uasset.source.json'):
                d=json.loads(p.read_text())
                self.local[d['path'].lower()]=p.with_name(p.name.removesuffix('.source.json'))
        self.new_bytes=sum(p.stat().st_size for where in (TEXTURES,DEPS) for p in where.rglob('*') if p.is_file())
    def resource(self,package,extension='.uasset'):
        if package.startswith('/Game/'):
            name='../../../ShooterGame/Content/'+package[6:]+extension
        elif package.startswith('/Engine/'):
            name='../../../Engine/Content/'+package[8:]+extension
        else:raise ValueError('Unrecognized package prefix')
        return self.index[name.lower()]
    def store(self,container):
        if container not in self.stores:self.stores[container]=IoStore(PAKS/container)
        return self.stores[container]
    def export(self,package,extension='.uasset'):
        container,entry=self.resource(package,extension)
        key=entry['path'].lower()
        if key in self.local:return self.local[key]
        # All new copies stay inside the assigned F-drive workspace.
        dest=DEPS/(package.strip('/')+extension)
        if not dest.exists():
            if self.new_bytes+entry['size']>230*1024*1024:raise ValueError('Texture source budget reached')
            self.store(container).export(entry['path'],dest)
            self.new_bytes+=dest.stat().st_size
        self.local[key]=dest
        return dest
    def read_bulk(self,package):
        path=self.export(package,'.ubulk')
        return path.read_bytes(),path

def bulk_records(z):
    _,p=names_at(z.data,52)
    size=struct.unpack_from('<Q',z.data,p)[0]
    if size%32 or p+8+size!=z.header['hashes_offset']:raise ValueError('Unsupported bulk map')
    return [dict(zip(('offset','duplicate_offset','size','flags'),struct.unpack_from('<4Q',z.data,k))) for k in range(p+8,p+8+size,32)]

def mipmaps(z):
    e=next(e for e in z.exports if e['class_index']=='0x5b93bca796d1fa6f')
    b=z.content(e)
    records=bulk_records(z)
    formats=re.finditer(rb'PF_(?:DXT1|DXT3|DXT5|BC4|BC5|BC7|B8G8R8A8|G8)\x00',b)
    for match in formats:
        fmt=match.group()[:-1].decode()
        p=match.end()
        if p+8>len(b):continue
        first,count=struct.unpack_from('<II',b,p);p+=8
        if first>16 or not 1<=count<=16:continue
        out=[]
        try:
            for mip in range(count):
                ri=struct.unpack_from('<I',b,p)[0];p+=4
                r=records[ri].copy()
                if r['flags']&0x40:
                    if r['offset']!=p:raise ValueError('Inline texture offset mismatch')
                    r['data']=b[p:p+r['size']];p+=r['size']
                    if len(r['data'])!=r['size']:raise ValueError('Truncated mipmap')
                width,height,depth=struct.unpack_from('<III',b,p);p+=12
                if not (0<width<=32768 and 0<height<=32768 and depth==1):raise ValueError('Mipmap dimensions invalid')
                r.update(width=width,height=height,mip=mip+first)
                out.append(r)
            return fmt,out
        except (ValueError,IndexError,struct.error):continue
    raise ValueError('No supported texture mip chain')

def dds_bytes(fmt,width,height,data):
    fourcc={'PF_DXT1':b'DXT1','PF_DXT3':b'DXT3','PF_DXT5':b'DXT5','PF_BC4':b'ATI1','PF_BC5':b'ATI2','PF_BC7':b'DX10'}[fmt]
    block=8 if fmt in ('PF_DXT1','PF_BC4') else 16
    expected=((width+3)//4)*((height+3)//4)*block
    if len(data)!=expected:raise ValueError('Block-compressed texture size mismatch')
    header=bytearray(124)
    struct.pack_into('<7I',header,0,124,0x81007,height,width,len(data),0,1)
    struct.pack_into('<II4s5I',header,72,32,4,fourcc,0,0,0,0,0)
    struct.pack_into('<I',header,104,0x1000)
    extra=struct.pack('<5I',98,3,0,1,0) if fmt=='PF_BC7' else b''
    return b'DDS '+header+extra+data

def decode_texture(package,source=None,max_size=1024):
    source=source or Source()
    path=source.export(package)
    z=Zen(path)
    fmt,mips=mipmaps(z)
    # Prefer the highest mip within the requested export cap; retain exact
    # source pixels rather than upscaling a small resident mip.
    mip=next((x for x in mips if max(x['width'],x['height'])<=max_size),mips[-1])
    mode='inline'
    bulkpath=None
    if 'data' in mip:data=mip['data']
    else:
        bulk,bulkpath=source.read_bulk(package)
        data=bulk[mip['offset']:mip['offset']+mip['size']];mode='external'
        if len(data)!=mip['size']:raise ValueError('Truncated bulk mipmap')
    if fmt in ('PF_B8G8R8A8','PF_G8'):
        channels=4 if fmt=='PF_B8G8R8A8' else 1
        if len(data)!=mip['width']*mip['height']*channels:raise ValueError('Raw pixel size mismatch')
        im=Image.frombytes('RGBA' if channels==4 else 'L',(mip['width'],mip['height']),data,'raw','BGRA' if channels==4 else 'L')
    else:im=Image.open(io.BytesIO(dds_bytes(fmt,mip['width'],mip['height'],data))).convert('RGBA')
    # A path-derived suffix avoids colliding leaf names.
    key=hashlib.sha256(package.lower().encode()).hexdigest()[:8]
    dest=TEXTURES/(package.rsplit('/',1)[-1]+'_'+key+'.png')
    dest.parent.mkdir(parents=True,exist_ok=True);im.save(dest)
    record=dict(package=package,texture=str(dest),source=str(path),source_sha256=hashlib.sha256(z.data).hexdigest(),pixel_format=fmt,width=im.width,height=im.height,mip=mip['mip'],bulk_mode=mode,bulk_source=str(bulkpath) if bulkpath else None,mip_sha256=hashlib.sha256(data).hexdigest())
    dest.with_suffix('.source.json').write_text(json.dumps(record,indent=2),encoding='utf8')
    return record

def diffuse_candidate(p):
    name=p.rsplit('/',1)[-1].lower()
    return bool(re.search(r'(?:_df|_d|_albedo|_basecolor|_diffuse)(?:$|_)',name))

def material_texture(package,source,visited=None):
    visited=set() if visited is None else visited
    if package.lower() in visited or len(visited)>8:return [],[]
    visited.add(package.lower())
    z=Zen(source.export(package))
    candidates=[p for p in z.imported_packages if diffuse_candidate(p)]
    parents=[p for p in z.imported_packages if p.rsplit('/',1)[-1].lower().endswith(('_mi','_mi2'))]
    indirect=[]
    if not candidates:
        for p in parents:
            try:
                found,chain=material_texture(p,source,visited);candidates.extend(found);indirect.extend([p]+chain)
            except Exception:pass
    return list(dict.fromkeys(candidates)),indirect

def main():
    TEXTURES.mkdir(exist_ok=True);DEPS.mkdir(exist_ok=True)
    source=Source();materials=set()
    for p in (ROOT/'meshes').glob('*.json'):
        if p.name=='report.json':continue
        m=json.loads(p.read_text())
        materials.update(x for x in m.get('materials',m.get('imports',[])) if x)
    transforms=ROOT/'scene-transforms.json'
    if transforms.exists():
        for level in json.loads(transforms.read_text()):
            for c in level.get('components',[]):
                materials.update(x for x in c.get('materials',[]) if x)
    out={};errors=[];decoded={}
    for material in sorted(materials):
        try:
            candidates,parents=material_texture(material,source)
            if not candidates:
                errors.append(dict(material=material,error='No diffuse texture candidate found'));continue
            # Keep the distinction between exact pixel extraction and a
            # simplified material preview visible in the provenance.
            chosen=candidates[0]
            if chosen not in decoded:decoded[chosen]=decode_texture(chosen,source)
            out[material]=dict(texture=decoded[chosen]['texture'],diffuse_package=chosen,candidates=candidates,parent_materials=parents,selection='first diffuse texture import; layered shader graph is not reproduced')
            print(material,decoded[chosen]['width'],decoded[chosen]['height'],flush=True)
        except Exception as exc:errors.append(dict(material=material,error=str(exc)))
    (TEXTURES/'materials.json').write_text(json.dumps(out,indent=2),encoding='utf8')
    (TEXTURES/'report.json').write_text(json.dumps(dict(materials=len(out),unique_textures=len(decoded),errors=errors),indent=2),encoding='utf8')
    print(json.dumps(dict(materials=len(out),textures=len(decoded),errors=errors),indent=2))

if __name__=='__main__':main()
