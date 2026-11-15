from pathlib import Path
import json,hashlib,struct
import inspect_respawn as w
p=w.p
EXPECTED='147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f'
assert hashlib.sha256(p.b).hexdigest()==EXPECTED

def code_record(label,lo,hi):
 off=p.pe.get_offset_from_rva(lo);raw=p.b[off:off+hi-lo]
 lines=[{'rva':hex(i.address-p.BASE),'instruction':i.mnemonic+' '+i.op_str} for i in p.md.disasm(raw,p.BASE+lo)]
 destination=w.OUT/(label+'.json');destination.write_text(json.dumps(lines,indent=2),encoding='utf-8')
 return {'label':label,'rva_start':hex(lo),'rva_end_exclusive':hex(hi),'bytes':hi-lo,'sha256':hashlib.sha256(raw).hexdigest(),'assembly':destination.relative_to(Path.cwd()).as_posix(),'first_instructions':lines[:5],'last_instructions':lines[-12:]}

records=[code_record('AuthRespawnPlayer',0x6504dc0,0x6504f3b),code_record('AuthCreateAndAddEquippable',0x64a1f00,0x64a207a),code_record('AuthCreateAndAddItem',0x64a19b0,0x64a1c41),code_record('AuthEquipFirstEquippedWeapon',0x649e890,0x649ea8f),code_record('SharedNoArgumentAuthWrapper',0x3b866f0,0x3b86704),code_record('ForceActivateEquippableInput',0x649f9d0,0x649fd2d),code_record('ShooterCharacterGetInventory',0x6c69d60,0x6c69d80),code_record('ShooterCharacterHasInitialized',0x6c6ba70,0x6c6ba90),code_record('AresInventoryGetCurrentEquippable',0x649bcd0,0x649bcf0),code_record('AresInventoryGetDesiredEquippable',0x649e560,0x649e580),code_record('CreateDynamicLocationSpawner',0x71abab0,0x71abb4a),code_record('CreateSpawnPointSpawner',0x7902030,0x79020ca)]

parameters=[]
for name,rva,offset,kind in [('State',0xb0d3f50,0,'ShooterPlayerState'),('PlayerSpawner',0xb0d3aa0,8,'PlayerSpawner'),('ReturnValue',0xb0db140,16,'Pawn')]:
 o=p.pe.get_offset_from_rva(rva);raw=p.b[o:o+64];assert struct.unpack_from('<I',raw,0x32)[0]==offset
 parameters.append({'name':name,'descriptor_rva':hex(rva),'parameter_offset':offset,'native_type':kind,'raw_hex':raw.hex(),'sha256':hashlib.sha256(raw).hexdigest()})
report={
 'schema_version':1,'scope':'Read-only fixed-image analysis, no game execution, injection, process write, installed-file change, credential or account operation.',
 'image':{'path':'work/client-analysis/Shipping-analysis-copy.exe','sha256':EXPECTED},
 'summary':{'original_respawn_entry_callable_body':'AuthRespawnPlayer exports a parameter-reader wrapper that unconditionally returns nullptr; it does not perform respawn.','inventory_creation_entries':'AuthCreateAndAddEquippable and AuthCreateAndAddItem similarly return nullptr. AuthEquipFirstEquippedWeapon only consumes parameters.','existing_client_functionality':'Inventory getters and ForceActivateEquippableInput retain real object reads / downstream calls; this does not supply missing creation/state assembly.','completion':'Original character, weapon and bot running acceptance remains unresolved.'},
 'respawn_manager':{
  'class':w.table('AresRespawnManager')['class'],'native_registration':'work/duel1-respawn/AresRespawnManager-methods.json','registered_method_count':1,'table_rva':'0xb0d9518','exec_rva':'0x6504dc0','ufunction_params_rva':'0xd52e7f0','parameter_array_rva':'0xc2adb10','parameter_struct_size':24,'parameters':parameters,
  'null_return_proof':['0x6504dd2 xor ebp,ebp','0x6504dd4 mov rsi,r8 preserves return buffer','Two object values are read into stack locals; all calls before the final return are VM/property readers.','0x6504f2a mov qword ptr [rsi],rbp writes zero','0x6504f3a ret; no call to SpawnActor, possession, inventory creation or a method on this'],
  'constructor':{'entry_rva':'0x650be40','object_size':'0x468','base':'Engine.Info','vtable_rva':'0xa9021c0','observation':'Constructor calls base actor construction and writes actor flags/vtable. The native class adds no registered native respawn implementation beyond its one wrapper.'}},
 'inventory_creation':[
  {'name':'AresInventory.AuthCreateAndAddEquippable','exec_rva':'0x64a1f00','null_store_rva':'0x64a2064','null_register':'rbp','result':'null return after parameter parsing'},
  {'name':'AresInventory.AuthCreateAndAddItem','exec_rva':'0x64a19b0','null_store_rva':'0x64a1c33','null_register':'rbp','result':'null return after parameter parsing'},
  {'name':'AresInventory.AuthEquipFirstEquippedWeapon','exec_rva':'0x649e890','result':'parameter parsing then return; no equip implementation call'},
  {'name':'AresInventory.AuthDestroyInventory / AuthEquipLastEquippedWeapon','exec_rva':'0x3b866f0','result':'shared 20-byte wrapper only advances FFrame bytecode pointer then returns'}],
 'retained_client_equippable_processing':{'name':'AresInventory.ForceActivateEquippableInput','exec_rva':'0x649f9d0','real_callee_rva':'0x70a4d30','sites':['0x649fcef','0x649fd0b'],'qualification':'The wrapper actually calls downstream input/state logic, unlike the factory wrappers. Preconditions and complete callee lifecycle are not established.'},
 'read_only_acceptance_fields':[
  {'owner':'ShooterCharacter','field':'Inventory','offset':'0xc08','getter_exec_rva':'0x6c69d60','field_type':'AresInventory pointer','descriptor_rva':'0xb4dfb20'},
  {'owner':'ShooterCharacter','field':'HasInitialized','offset':'0xa90','width':1,'getter_exec_rva':'0x6c6ba70'},
  {'owner':'ShooterCharacter','field':'StartingEquippableClasses','offset':'0xbd8','field_type':'TArray','getter_exec_rva':'0x6c63b80','qualification':'Only getter field access inspected; not a grant or initialization mechanism.'},
  {'owner':'AresInventory','field':'CurrentEquippable','offset':'0x280','getter_exec_rva':'0x649bcd0','descriptor_rva':'0xb09d920'},
  {'owner':'AresInventory','field':'DesiredEquippable','offset':'0x268','getter_exec_rva':'0x649e560','descriptor_rva':'0xb09cf40'}],
 'spawner_objects':{
  'base':next(c for c in w.classes if c['name']=='PlayerSpawner'),
  'concrete_native_classes':[c for c in w.classes if c['name'] in ('DynamicLocationPlayerSpawner','SpawnPointPlayerSpawner','ResurrectPlayerSpawner')],
  'real_factory_paths':[{'wrapper':'0x6728e80','factory':'0x71abab0','class_getter':'0x6728dd0','construct_object_call':'0x1d166c0'},{'wrapper':'0x6d0c590','factory':'0x7902030','class_getter':'0x6d0c4e0','construct_object_call':'0x1d166c0'}],
  'qualification':'These factories construct UObject spawn-description objects. They do not replace the absent AuthRespawnPlayer implementation and do not themselves prove Pawn creation.'},
 'dependency_checklist':[
  'Original GameState / fixed-time-step initialization must be established before ServerMovementTimingComponent; see native-map-init-crash.json.',
  'A concrete ShooterCharacter-derived Blueprint class is needed; native ShooterCharacter is abstract. BasePawn_C metadata evidence is in duel1-player-static.json.',
  'AuthRespawnPlayer signature requires a ShooterPlayerState and a PlayerSpawner; its local body does not consume them to spawn a Pawn.',
  'After any separately established Pawn spawning path, validate actual class, Controller/Pawn association, Pawn.HasInitialized and Pawn.Inventory before attempting equipping.',
  'Weapon actor construction alone is insufficient: validate inventory CurrentEquippable/DesiredEquippable, attachment, animation/state and actual client fire behavior.',
  'Known inventory grant wrappers and known bot factories cannot be treated as usable server-side factories simply by invoking reflection.'
 ],
 'next_experiment_boundary':{'useful':'Keep working native GameModeBase navigation as a control. Any candidate original-Pawn creation path must demonstrate a non-empty implementation and then be observed in the owned local scene.','not_supported':'Blindly calling AuthRespawnPlayer, AuthCreateAndAddEquippable or AuthCreateAndAddItem cannot implement the requested behavior on this fixed image.','unresolved':'A supported local object-creation and state-initialization sequence for BasePawn_C and original weapons; no full alternative route is established here.'},
 'raw_code_evidence':records,
 'limits':['A bounded set of generated native wrappers and relevant getters was inspected. This is not a proof that all generation logic is absent everywhere in the image.','Function-pointer registration and static byte contents do not establish successful runtime construction.','AuthCreateAndAddPatchedEquippable, TryEquipSpecificEquippable and other chained functions were inventoried but not classified as empty because complete discontiguous control flow was not assessed.','No bot behavior, damage, weapon firing or character possession was runtime-tested.']
}
Path('reports/duel1-respawn-static.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print('Wrote reports/duel1-respawn-static.json; raw function evidence',len(records))
