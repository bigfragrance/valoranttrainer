"""Bounded read-only UE IoStore v5 inventory/extractor, written for this task.

Source files are opened rb only; exports are written only to a caller-selected
output directory. Does not start any game process or use the network.
"""
from pathlib import Path
import argparse, ctypes, hashlib, json, struct, zlib

MAGIC = b'-==--==--==--==-'
INVALID = 0xffffffff

class Cursor:
    def __init__(self, data, pos=0): self.data, self.pos = data, pos
    def read(self,n):
        if n<0 or self.pos+n>len(self.data): raise ValueError('Out of bounds')
        x=self.data[self.pos:self.pos+n]; self.pos+=n; return x
    def u32(self): return struct.unpack('<I',self.read(4))[0]
    def string(self):
        n=struct.unpack('<i', self.read(4))[0]
        if abs(n)>1000000: raise ValueError('Invalid FString')
        return self.read(n if n>=0 else -n*2).decode('utf-8' if n>=0 else 'utf-16-le').rstrip('\x00')

class IoStore:
    def __init__(self, path, oodle_dll=None):
        self.path=Path(path).resolve()
        self.data=self.path.read_bytes()
        b=self.data
        if b[:16]!=MAGIC: raise ValueError('Invalid IoStore magic')
        self.version=b[16]
        if self.version!=5: raise ValueError('Only observed v5 supported')
        (self.header_size,self.entry_count,self.block_count,self.block_entry_size,
         self.method_count,self.method_name_length,self.block_size,
         self.directory_size,self.partition_count)=struct.unpack_from('<9I',b,20)
        self.flags=b[80]
        self.partition_size=struct.unpack_from('<Q',b,88)[0]
        seed_count=struct.unpack_from('<I',b,84)[0]
        overflow_count=struct.unpack_from('<I',b,96)[0]
        if self.flags&2: raise ValueError('Encrypted container not supported')
        if self.block_entry_size!=12: raise ValueError('Unknown block entry size')
        c=Cursor(b,self.header_size)
        self.ids=[c.read(12).hex() for _ in range(self.entry_count)]
        self.ranges=[(int.from_bytes(c.read(5),'big'),int.from_bytes(c.read(5),'big')) for _ in range(self.entry_count)]
        c.read(4*(seed_count+overflow_count))
        self.blocks=[]
        for _ in range(self.block_count):
            record=c.read(12)
            self.blocks.append((int.from_bytes(record[:5],'little'),int.from_bytes(record[5:8],'little'),int.from_bytes(record[8:11],'little'),record[11]))
        self.methods=['None']+[c.read(self.method_name_length).split(b'\0')[0].decode('ascii') for _ in range(self.method_count)]
        self.block_hashes=None
        if self.flags&4:
            sig_size=c.u32(); c.read(sig_size*2)
            self.block_hashes=[c.read(20) for _ in range(self.block_count)]
        self.directory_offset=c.pos
        self.files={}
        if self.directory_size:
            self._directory(c.read(self.directory_size))
        self.oodle_dll=oodle_dll
        self._decompress=None

    def _directory(self,data):
        c=Cursor(data)
        self.mount=c.string()
        dirs=[struct.unpack('<4I',c.read(16)) for _ in range(c.u32())]
        files=[struct.unpack('<3I',c.read(12)) for _ in range(c.u32())]
        names=[c.string() for _ in range(c.u32())]
        visited=set()
        def walk(di,parent):
            if di in visited or di>=len(dirs): raise ValueError('Invalid directory traversal')
            visited.add(di)
            name,child,sibling,fi=dirs[di]
            here=parent+(names[name]+'/' if name!=INVALID else '')
            seenfiles=set()
            while fi!=INVALID:
                if fi in seenfiles or fi>=len(files): raise ValueError('Invalid file traversal')
                seenfiles.add(fi)
                ni,next_fi,entry=files[fi]
                if entry>=self.entry_count: raise ValueError('Invalid chunk index')
                self.files[here+names[ni]]=entry
                fi=next_fi
            if child!=INVALID: walk(child,here)
            if sibling!=INVALID: walk(sibling,parent)
        walk(0,self.mount)
        if c.pos!=len(data): raise ValueError('Unexpected directory index tail')

    def metadata(self,name):
        idx=self.files[name]; offset,length=self.ranges[idx]
        blocks=[]
        for bi in range(offset//self.block_size,(offset+length+self.block_size-1)//self.block_size):
            physical,compressed,uncompressed,method=self.blocks[bi]
            blocks.append(dict(index=bi,offset=physical,compressed=compressed,uncompressed=uncompressed,method=self.methods[method]))
        return dict(path=name,container=self.path.name,index=idx,chunk_id=self.ids[idx],logical_offset=offset,size=length,blocks=blocks)

    def read(self,name):
        meta=self.metadata(name)
        output=[]
        for block in meta['blocks']:
            physical=block['offset']
            partition=physical//self.partition_size
            p=self.path.with_suffix('.ucas') if not partition else self.path.with_name(self.path.stem+'_s'+str(partition)+'.ucas')
            with p.open('rb') as f:
                f.seek(physical%self.partition_size)
                physical_length=(block['compressed']+15)//16*16 if self.block_hashes else block['compressed']
                stored=f.read(physical_length)
            if self.block_hashes and hashlib.sha1(stored).digest()!=self.block_hashes[block['index']]:
                raise ValueError('TOC physical block SHA1 mismatch')
            packed=stored[:block['compressed']]
            if len(packed)!=block['compressed']: raise ValueError('Short physical read')
            method=block['method']
            if method=='None': unpacked=packed
            elif method=='Zlib': unpacked=zlib.decompress(packed)
            elif method=='Oodle': unpacked=self._oodle(packed,block['uncompressed'])
            else: raise ValueError('Unsupported compression '+method)
            if len(unpacked)!=block['uncompressed']: raise ValueError('Decompressed length mismatch')
            output.append(unpacked)
        content=b''.join(output)
        begin=meta['logical_offset']%self.block_size
        content=content[begin:begin+meta['size']]
        if len(content)!=meta['size']: raise ValueError('Chunk length mismatch')
        return content

    def _oodle(self,packed,size):
        if not self.oodle_dll:
            from native_oodle_directory import decompress_block
            return decompress_block(packed,size)
        if self._decompress is None:
            if not self.oodle_dll: raise ValueError('Oodle decompression requires an explicitly supplied local DLL')
            self._dll=ctypes.CDLL(str(Path(self.oodle_dll).resolve()))
            self._decompress=self._dll.OodleLZ_Decompress
            self._decompress.restype=ctypes.c_int64
            self._decompress.argtypes=[ctypes.c_void_p,ctypes.c_int64,ctypes.c_void_p,ctypes.c_int64,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_void_p,ctypes.c_int64,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int64,ctypes.c_int]
        src=ctypes.create_string_buffer(packed)
        dst=ctypes.create_string_buffer(size)
        result=self._decompress(src,len(packed),dst,size,1,0,0,None,0,None,None,None,0,3)
        if result!=size: raise ValueError('Oodle failure '+str(result))
        return dst.raw

    def export(self,name,destination):
        dest=Path(destination).resolve()
        # Explicitly prevent exporting anywhere in the source installation.
        install=self.path.parents[3]
        if dest==install or install in dest.parents: raise ValueError('Destination is inside source installation')
        content=self.read(name)
        dest.parent.mkdir(parents=True,exist_ok=True)
        dest.write_bytes(content)
        meta=self.metadata(name)
        meta.update(output=str(dest),sha256=hashlib.sha256(content).hexdigest(),physical_block_sha1_verified_against_toc=bool(self.block_hashes),toc_signature_authentication='not performed')
        dest.with_name(dest.name+'.source.json').write_text(json.dumps(meta,indent=2),encoding='utf-8')
        return meta

def inventory(source,destination):
    result=[]
    for p in sorted(Path(source).glob('*.utoc')):
        io=IoStore(p)
        entry=dict(container=p.name,version=io.version,flags=io.flags,methods=io.methods,sha256=hashlib.sha256(io.data).hexdigest(),entry_count=io.entry_count,indexed_files=len(io.files),directory_offset=io.directory_offset,directory_size=io.directory_size,files=[dict(path=name,index=i,size=io.ranges[i][1]) for name,i in io.files.items()])
        result.append(entry)
        print(p.name,io.entry_count,len(io.files),io.methods)
    Path(destination).write_text(json.dumps(result,indent=2),encoding='utf-8')

if __name__=='__main__':
    a=argparse.ArgumentParser(); a.add_argument('source'); a.add_argument('destination'); args=a.parse_args()
    inventory(args.source,args.destination)
