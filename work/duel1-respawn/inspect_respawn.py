from pathlib import Path
import sys,json,struct,hashlib
sys.path.insert(0,str(Path(__file__).resolve().parents[1]));import platform_core as p
from capstone.x86 import X86_OP_MEM,X86_REG_RIP,X86_OP_REG,X86_OP_IMM
OUT=Path(__file__).parent
classes=json.loads((p.OUT/'continue-world/base-bootstrap/class-catalog.json').read_text(encoding='utf-8'))['classes']
def insns(rva):
 fr=p.fn(rva)
 if not fr:return []
 o=p.pe.get_offset_from_rva(fr[0]);return list(p.md.disasm(p.b[o:o+fr[1]-fr[0]],p.BASE+fr[0]))
def rip_target(i):
 for op in i.operands:
  if op.type==X86_OP_MEM and op.mem.base==X86_REG_RIP:return i.address+i.size+op.mem.disp-p.BASE

def table(name):
 c=next(c for c in classes if c['name']==name)
 regs=insns(int(c['getter_rva'],16));registrar=next((rip_target(i) for i in regs if i.mnemonic=='lea' and i.op_str.startswith('r9,')),None)
 lines=insns(registrar) if registrar else []
 count=next((i.operands[1].imm for i in lines if i.mnemonic=='mov' and i.op_str.startswith('r8d,')),0)
 address=next((rip_target(i) for i in lines if i.mnemonic=='lea' and i.op_str.startswith('rdx,')),None)
 if address is None:
  address=next((rip_target(i) for i in lines if i.mnemonic=='lea' and i.op_str.startswith('rsi,')),None)
  count=next((i.operands[1].imm for i in lines if i.mnemonic=='mov' and i.op_str.startswith('r14d,')),0)
 methods=[]
 if address and count:
  o=p.pe.get_offset_from_rva(address)
  for k in range(count):
   n,f=struct.unpack_from('<QQ',p.b,o+k*16)
   fr=p.fn(f-p.BASE)
   methods.append({'name':p.string(n-p.BASE),'name_rva':hex(n-p.BASE),'exec_rva':hex(f-p.BASE),'range':[hex(x) for x in fr] if fr else None})
 return {'class':c,'registrar_rva':hex(registrar) if registrar else None,'table_rva':hex(address) if address else None,'method_count':count,'methods':methods}

def dump(name):
 t=table(name);(OUT/(name+'-methods.json')).write_text(json.dumps(t,indent=2),encoding='utf-8');return t
if __name__=='__main__':
 for name in sys.argv[1:]:
  t=dump(name);print(name,t['registrar_rva'],t['table_rva'],t['method_count']);print('\n'.join(str(x) for x in t['methods']))
