"""Offline metadata reader for the observed UE5 Zen package layout."""
import struct, json
from pathlib import Path

def names_at(b, off):
    n, byte_count = struct.unpack_from('<II',b,off)
    assert n<100000 and byte_count<len(b)
    p=off+16+8*n
    lengths=[int.from_bytes(b[p+i*2:p+i*2+2],'big') for i in range(n)]
    p+=2*n
    names=[]
    start=p
    for v in lengths:
        wide=bool(v&0x8000); length=v&0x7fff
        size=length*(2 if wide else 1)
        names.append(b[p:p+size].decode('utf-16le' if wide else 'utf8'))
        p+=size
    assert p-start==byte_count,(p-start,byte_count)
    return names,p

class Zen:
    def __init__(self,path):
        self.path=Path(path); self.data=b=self.path.read_bytes()
        h=struct.unpack_from('<13I',b)
        self.header=dict(zip(['versioned','header_size','name_index','name_number','flags','cooked_header_size','hashes_offset','imports_offset','exports_offset','bundles_offset','dependency_headers_offset','dependency_entries_offset','imported_names_offset'],h))
        assert not h[0],'Versioned not supported'
        self.names,end=names_at(b,52)
        self.imports=[int.from_bytes(b[i:i+8],'little') for i in range(h[7],h[8],8)]
        self.hashes=[int.from_bytes(b[i:i+8],'little') for i in range(h[6],h[7],8)]
        try:
            self.imported_packages,np=names_at(b,h[12])
            numbers=struct.unpack_from('<'+str(len(self.imported_packages))+'I',b,np)
            self.imported_packages=[name+('_'+str(num-1) if num else '') for name,num in zip(self.imported_packages,numbers)]
        except Exception: self.imported_packages=[]
        self.exports=[]
        assert (h[9]-h[8])%72==0
        for i,p in enumerate(range(h[8],h[9],72)):
            offset,size,ni,nn,outer,cls,sup,template,public,flags=struct.unpack_from('<QQIIQQQQQI',b,p)
            name=self.names[ni&0x3fffffff]+('_'+str(nn-1) if nn else '')
            start=h[1]+offset
            assert start+size<=len(b),(start,size,len(b))
            self.exports.append(dict(index=i,name=name,serial_offset=offset,start=start,size=size,outer=hex(outer),class_index=hex(cls),super_index=hex(sup),template=hex(template),public=hex(public),flags=hex(flags)))
    def content(self,e):
        if isinstance(e,int): e=self.exports[e]
        return self.data[e['start']:e['start']+e['size']]
    def info(self): return dict(header=self.header,names=self.names,imported_packages=self.imported_packages,imports=[hex(x) for x in self.imports],hashes=[hex(x) for x in self.hashes],exports=self.exports)

if __name__=='__main__':
    import sys
    z=Zen(sys.argv[1]); print(json.dumps(z.info(),indent=2))
