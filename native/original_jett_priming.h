#pragma once
#include "original_jett_native.h"
#include <cmath>
#include <cstring>

// Standalone authority tuning arrives after the original E BeginPlay queried
// PrimeDuration. Catch up only the original query/setter block, before inputs.
// No synthetic FFrame, timer write, state transition, AddStates or BeginPlay.
// Evidence: reports/duel1-jett-e-x-gate-audit.json.
namespace original_jett_priming {
inline constexpr const wchar_t* TypePath=L"/Game/Characters/Wushu/S0/Ability_E/Ability_Wushu_E_Dash.Ability_Wushu_E_Dash_C";
inline constexpr const wchar_t* GraphPath=L"/Game/Characters/Wushu/S0/Ability_E/Ability_Wushu_E_Dash.Ability_Wushu_E_Dash_C.ExecuteUbergraph_Ability_Wushu_E_Dash";
inline constexpr const wchar_t* DurationTag=L"AbilityTuning.Wushu.E.PrimeDuration";
inline constexpr int32_t OriginalEntry=1170;
struct PropertyInfo {
    void *field=nullptr,*fieldClass=nullptr,*firstProperty=nullptr;
    uint64_t castFlags=0,actualName=0,expectedName=0;
    int32_t offset=-1,propertiesSize=0,arrayDim=0,elementSize=0;
    uint8_t boolFieldSize=0,boolByteOffset=0,boolByteMask=0,boolFieldMask=0;
    bool genericProperty=false,nameMatches=false,boundsValid=false;
};
struct Diagnostics {
    bool initialized=false,ready=false,attempted=false,unchanged=false,restartRequired=false;
    bool ownerValid=false,sourceValid=false,profileValid=false,frameValid=false,initialStates=false;
    bool noInput=false,noActiveForce=false,timerQueryValid=false,postconditions=false;
    uint32_t error=0,generation=0;
    uint64_t originalCalls=0,completedCalls=0;
    uintptr_t exceptionRva=0;
    const char* stage="original_jett_priming_not_initialized";
    const wchar_t* failedEvent=nullptr;
    uint16_t expectedEventSize=0,actualEventSize=0;
    void *pawn=nullptr,*ability=nullptr,*type=nullptr,*graph=nullptr,*tuning=nullptr,*sourceTable=nullptr;
    void *primed=nullptr,*primingMachine=nullptr,*primaryMachine=nullptr,*primeIdle=nullptr,*inactive=nullptr;
    void *frame=nullptr,*nativeFrame=nullptr,*sourceRow=nullptr,*runtimeValue=nullptr;
    original_ability_tuning::WeakIdentity identity{};
    int32_t frameOffset=-1,outValueOffset=-1,successOffset=-1,activeForceCount=-1;
    uint32_t inputEdges=0;
    uint8_t activationInput=255,originalQuerySucceeded=0;
    float before=0,source=0,after=0,originalQueryValue=0;
    original_ability_tuning::Value sourceBytes{};
    PropertyInfo frameProperty{},outProperty{},successProperty{};
    void *graphOuter=nullptr,*classGraph=nullptr,*classFrameField=nullptr,*frameGetter=nullptr,*script=nullptr;
    void *floatFieldClass=nullptr,*boolFieldClass=nullptr;
    uint32_t functionFlags=0,profileFailures=0,entryReturn=0;
    uint16_t functionParamsSize=0;
    int32_t scriptSize=0,scriptCapacity=0,graphPropertiesSize=0,classFrameOffset=-1;
    bool scriptReadable=false,anchorsMatch=false,primitiveLayoutsMatch=false;
    std::array<uint8_t,10> anchorOpcodes{};
};
inline Diagnostics state;
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(uint32_t code,const char* stage){state.error=code;state.stage=stage;return false;}
inline bool Event(void* object,const wchar_t* path,uint16_t size,void* params){
    auto function=engine::Find(path);const auto actual=Read<uint16_t>(function,0xbe);
    if(!object||!function||actual!=size){state.failedEvent=path;state.expectedEventSize=size;state.actualEventSize=actual;return false;}
    auto table=Read<void**>(object,0);auto process=Read<void*>(table,0x298);
    if(!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,function,params);return true;
}
inline int FieldOffset(void* type,const wchar_t* name,size_t bytes,PropertyInfo* observation=nullptr){
    if(!type||!name)return -1;
    PropertyInfo local{};auto& info=observation?*observation:local;
    original_ability_tuning::NativeName nativeName{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&nativeName,name,1);
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&nativeName,3);
    auto fieldClass=Read<void*>(field,8);
    // Exact native1c63870 starts at UStruct.ChildProperties+58; iterator
    // 1b9efc0 uses this same generic8001 mask (including float/bool), Next+18.
    info.field=field;info.fieldClass=fieldClass;info.firstProperty=Read<void*>(type,0x58);
    info.castFlags=Read<uint64_t>(fieldClass,0x18);info.actualName=Read<uint64_t>(field,0x20);
    info.expectedName=original_jett_priming::Read<uint64_t>(&nativeName,0);
    info.offset=field?Read<int32_t>(field,0x44):-1;info.propertiesSize=Read<int32_t>(type,0x60);
    info.arrayDim=Read<int32_t>(field,0x30);info.elementSize=Read<int32_t>(field,0x34);
    info.genericProperty=field&&(info.castFlags&0x8001)==0x8001;
    info.nameMatches=field&&info.actualName==info.expectedName;
    info.boundsValid=info.offset>=0&&info.propertiesSize>=0
        &&static_cast<size_t>(info.offset)+bytes<=static_cast<size_t>(info.propertiesSize);
    if((info.castFlags&0x28001)==0x28001){
        info.boolFieldSize=Read<uint8_t>(field,0x78);info.boolByteOffset=Read<uint8_t>(field,0x79);
        info.boolByteMask=Read<uint8_t>(field,0x7a);info.boolFieldMask=Read<uint8_t>(field,0x7b);
    }
    return info.genericProperty&&info.nameMatches&&info.boundsValid?info.offset:-1;
}
inline void* ObjectField(const wchar_t* name){
    const int offset=FieldOffset(state.type,name,8);
    return offset<0?nullptr:Read<void*>(state.ability,static_cast<size_t>(offset));
}
inline bool Source(){
    if(!original_players::IsA(state.tuning,original_ability_tuning::ComponentClass)
        ||Read<void*>(state.tuning,0xc8)!=state.ability)
        return Fail(3351,"original_jett_priming_actual_tuning_component_missing");
    const original_ability_tuning::Record* imported=nullptr;
    for(const auto& record:original_ability_tuning::records){
        if(record.component==state.tuning&&record.actor==state.ability&&record.ready&&record.sourceRowsPreserved
            &&engine::function<void*(*)(const original_ability_tuning::WeakIdentity*)>(0x1d4cfa0)(&record.identity)==state.tuning){
            imported=&record;break;
        }
    }
    if(!imported||!Event(state.tuning,L"/Script/ShooterGame.AbilityTuningComponent.GetAbilityTuningTable",8,&state.sourceTable)
        ||state.sourceTable!=imported->table||!original_players::IsA(state.sourceTable,original_ability_tuning::DataTableClass)
        ||Read<void*>(state.sourceTable,0x30)!=engine::Find(L"/Script/ShooterGame.AbilityTuningDataTableRow"))
        return Fail(3352,"original_jett_priming_requires_completed_original_tuning_import");
    original_ability_tuning::NativeName tag{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&tag,DurationTag,1);
    auto map=static_cast<uint8_t*>(state.tuning)+0xe8;
    state.runtimeValue=original_ability_tuning::FindValue(map,&tag);
    if(!original_players::Readable(state.runtimeValue,24)||Read<uint8_t>(state.runtimeValue,0)!=2)
        return Fail(3353,"original_jett_priming_original_duration_tag_not_float");
    struct Names {void* table;original_ability_tuning::NativeName* data=nullptr;int32_t count=0,capacity=0;} names{state.sourceTable};
    static_assert(sizeof(Names)==24);
    const bool queried=Event(engine::DefaultObject(engine::Class(0x42f1030)),L"/Script/Engine.DataTableFunctionLibrary.GetDataTableRowNames",24,&names);
    struct Owner {void* data;~Owner(){if(data)engine::function<void(*)(void*)>(0x1915720)(data);}} owner{names.data};
    if(!queried||names.count<=0||names.count>static_cast<int32_t>(original_ability_tuning::MaxRows)
        ||names.capacity<names.count||!original_players::Readable(names.data,static_cast<size_t>(names.count)*12))
        return Fail(3354,"original_jett_priming_original_source_rows_unavailable");
    int matches=0;
    for(int32_t i=0;i<names.count;++i){
        auto row=engine::function<uint8_t*(*)(void*,const original_ability_tuning::NativeName*)>(0x42ee820)(state.sourceTable,names.data+i);
        if(!original_players::Readable(row,48))return Fail(3355,"original_jett_priming_original_source_row_invalid");
        if(std::memcmp(row+8,&tag,8)==0){++matches;state.sourceRow=row;std::memcpy(&state.sourceBytes,row+24,24);}
    }
    state.source=original_jett_priming::Read<float>(&state.sourceBytes,8);
    state.sourceValid=matches==1&&original_jett_priming::Read<uint8_t>(&state.sourceBytes,0)==2&&std::isfinite(state.source)&&state.source>0
        &&std::memcmp(state.runtimeValue,&state.sourceBytes,24)==0;
    return state.sourceValid||Fail(3356,"original_jett_priming_duration_source_or_runtime_override_conflict");
}
inline bool Profile(){
    state.graph=engine::Find(GraphPath);
    state.frameOffset=FieldOffset(state.type,L"UberGraphFrame",8,&state.frameProperty);
    state.outValueOffset=FieldOffset(state.graph,L"CallFunc_GetAbilityTuningValue_OutValue",4,&state.outProperty);
    state.successOffset=FieldOffset(state.graph,L"CallFunc_GetAbilityTuningValue_ReturnValue",1,&state.successProperty);
    auto script=Read<uint8_t*>(state.graph,0x68);
    auto classTable=Read<void*>(state.type,0);
    auto frameField=Read<void*>(state.type,0x290);
    state.script=script;state.graphOuter=Read<void*>(state.graph,0x28);state.classGraph=Read<void*>(state.type,0x298);
    state.classFrameField=frameField;state.classFrameOffset=frameField?Read<int32_t>(frameField,0x44):-1;
    state.frameGetter=Read<void*>(classTable,0x3c0);state.functionFlags=Read<uint32_t>(state.graph,0xb8);
    state.functionParamsSize=Read<uint16_t>(state.graph,0xbe);state.scriptSize=Read<int32_t>(state.graph,0x70);
    state.scriptCapacity=Read<int32_t>(state.graph,0x74);state.graphPropertiesSize=Read<int32_t>(state.graph,0x60);
    state.scriptReadable=state.scriptSize==3821&&state.scriptCapacity>=state.scriptSize&&original_players::Readable(script,3821);
    constexpr std::array<size_t,10> offsets{0,5,1170,1248,1289,1330,1371,1412,3818,3820};
    constexpr std::array<uint8_t,10> expected{0x4c,0x4e,0x14,0x19,0x19,0x19,0x19,0x4d,4,0x53};
    state.anchorOpcodes.fill(0xff);
    if(state.scriptSize>0&&state.scriptSize<=1048576&&state.scriptCapacity>=state.scriptSize){
        for(size_t i=0;i<offsets.size();++i)if(offsets[i]<static_cast<size_t>(state.scriptSize))
            original_players::Read(script,offsets[i],state.anchorOpcodes[i]);
        if(state.scriptSize>=5)original_players::Read(script,1,state.entryReturn);
    }
    state.anchorsMatch=state.anchorOpcodes==expected&&state.entryReturn==3818;
    // Exact pinned FFieldClass constructors: Float1c9de40 cast1008101,
    // Bool1c77e20 cast28001. Keep primitive layout evidence distinct from the
    // generic FindFProperty filter; never infer a primitive from byte count.
    state.floatFieldClass=engine::function<void*(*)()>(0x1c9de40)();
    state.boolFieldClass=engine::function<void*(*)()>(0x1c77e20)();
    state.primitiveLayoutsMatch=state.outProperty.fieldClass==state.floatFieldClass
        &&state.outProperty.castFlags==0x1008101&&state.outProperty.arrayDim==1&&state.outProperty.elementSize==4
        &&state.successProperty.fieldClass==state.boolFieldClass&&state.successProperty.castFlags==0x28001
        &&state.successProperty.arrayDim==1&&state.successProperty.elementSize==1&&state.successProperty.boolFieldSize==1
        &&state.successProperty.boolByteOffset==0&&state.successProperty.boolByteMask!=0&&state.successProperty.boolFieldMask!=0;
    state.profileFailures=0;
    const auto require=[](bool valid,uint32_t bit){if(!valid)state.profileFailures|=bit;};
    require(state.graph&&state.graphOuter==state.type,1);
    require(state.classGraph==state.graph,2);require(state.frameOffset>=0,4);
    require(state.outValueOffset>=0,8);require(state.successOffset>=0,16);
    require(frameField&&state.classFrameOffset==state.frameOffset,32);
    // Ordinary BlueprintGeneratedClass uses40c0c80 (AnimBlueprintGeneratedClass
    // has3f41d30). Both select class+290/+298 and return instance[Offset44];
    // this E actor's exact original ordinary-class virtual is required here.
    require(state.frameGetter==reinterpret_cast<void*>(engine::image+0x40c0c80),64);
    require((state.functionFlags&0x8000)!=0&&(state.functionFlags&0x400)==0,128);
    require(state.functionParamsSize==4,256);require(state.scriptSize==3821,512);
    require(state.scriptReadable,1024);require(state.anchorsMatch,2048);require(state.primitiveLayoutsMatch,4096);
    state.profileValid=state.profileFailures==0;
    if(!state.profileValid)return Fail(3357,"original_jett_priming_original_1170_graph_profile_mismatch");
    state.frame=Read<void*>(state.ability,static_cast<size_t>(state.frameOffset));
    state.nativeFrame=reinterpret_cast<void*(*)(void*,void*,void*)>(Read<void*>(classTable,0x3c0))(state.type,state.ability,state.graph);
    state.frameValid=state.frame&&state.frame==state.nativeFrame
        &&original_players::Readable(state.frame,static_cast<size_t>(Read<int32_t>(state.graph,0x60)));
    return state.frameValid||Fail(3358,"original_jett_priming_original_persistent_frame_unavailable");
}
inline bool InitialConditions(){
    const auto& skill=original_jett::skills[2];
    state.inputEdges=skill.inputEdges;
    struct Activation {uint8_t slot=5,value=255;} activation;
    if(!Event(state.pawn,L"/Script/ShooterGame.ShooterCharacter.GetCurrentActivationInputState",2,&activation))
        return Fail(3359,"original_jett_priming_original_activation_query_failed");
    state.activationInput=activation.value;
    state.noInput=state.inputEdges==0&&activation.value==0&&(original_jett::previousInputs&original_jett::InputBits[2])==0
        &&skill.stateTransitions==0&&skill.forceStateEntries==0;
    auto force=Read<void*>(state.pawn,0x1100);
    state.activeForceCount=-1;
    state.noActiveForce=original_players::IsA(force,0x685f510)
        &&Event(force,L"/Script/ShooterGame.ForceModuleManagerComponent.GetActiveForceModuleCount",4,&state.activeForceCount)
        &&state.activeForceCount==0;
    void *primaryCurrent=nullptr,*primingCurrent=nullptr;
    state.initialStates=Event(state.primaryMachine,L"/Script/ShooterGame.StateMachineComponent.GetCurrentState",8,&primaryCurrent)
        &&Event(state.primingMachine,L"/Script/ShooterGame.StateMachineComponent.GetCurrentState",8,&primingCurrent)
        &&primaryCurrent==state.inactive&&primingCurrent==state.primeIdle
        &&Read<void*>(state.primaryMachine,0x1e8)==state.inactive&&Read<void*>(state.primingMachine,0x1e8)==state.primeIdle
        &&Read<uint8_t>(state.primaryMachine,0x348)!=0&&Read<uint8_t>(state.primingMachine,0x348)!=0;
    return (state.noInput&&state.noActiveForce&&state.initialStates)
        ||Fail(3360,"original_jett_priming_requires_unactivated_original_initial_states");
}
inline bool InitializeImpl(void* pawn,void* ability){
    if(!original_players::Thread()||state.restartRequired)return Fail(3361,"original_jett_priming_wrong_thread_or_restart_required");
    const auto& jett=original_jett::State();const auto& skill=original_jett::skills[2];
    if(!jett.loadoutGranted||jett.pawn!=pawn||skill.actor!=ability||!skill.grantReady)
        return Fail(3362,"original_jett_priming_requires_actual_completed_E_grant");
    if(state.initialized&&state.pawn==pawn&&state.ability==ability
        &&engine::function<void*(*)(const original_ability_tuning::WeakIdentity*)>(0x1d4cfa0)(&state.identity)==ability){
        if(state.ready)return true;
        if(state.error)return false;
        return Fail(3363,"original_jett_priming_prior_attempt_not_repeated");
    }
    const auto generation=state.generation+1;const auto calls=state.originalCalls,completed=state.completedCalls;
    state={};state.generation=generation;state.originalCalls=calls;state.completedCalls=completed;
    state.initialized=true;state.pawn=pawn;state.ability=ability;state.type=engine::Find(TypePath);
    state.identity.index=Read<int32_t>(ability,0xc);
    if(state.identity.index<0)return Fail(3364,"original_jett_priming_ability_identity_invalid");
    state.identity.serial=engine::function<int32_t(*)(void*,int32_t)>(0x1cf35b0)
        (reinterpret_cast<void*>(engine::image+0xd7b87c0),state.identity.index);
    state.ownerValid=state.type&&Read<void*>(ability,0x10)==state.type&&skill.type==state.type
        &&Read<void*>(ability,0x210)==pawn&&Read<void*>(ability,0x4a8)==pawn&&Read<uint8_t>(ability,0x224)==3
        &&Read<void*>(skill.slot,0x38)==ability&&state.identity.serial>0&&engine::function<int(*)(void*)>(0x3efc820)(pawn)==0;
    if(!state.ownerValid)return Fail(3365,"original_jett_priming_original_owned_standalone_E_mismatch");
    state.primed=ObjectField(L"Prime_Primed");state.primeIdle=ObjectField(L"Prime_Idle");
    state.tuning=ObjectField(L"AbilityTuning");state.primingMachine=ObjectField(L"StateMachine_Priming");
    state.primaryMachine=Read<void*>(ability,0xe00);state.inactive=Read<void*>(state.primaryMachine,0x350);
    if(!original_players::IsA(state.primed,0x63d9390)||!original_players::IsA(state.primeIdle,0x6d6e830)
        ||!original_players::IsA(state.primingMachine,0x67722c0)||!original_players::IsA(state.primaryMachine,0x67722c0)
        ||Read<void*>(state.primed,0xc8)!=ability||Read<void*>(state.primeIdle,0xc8)!=ability
        ||Read<void*>(state.primed,0xe0)!=state.primingMachine||Read<void*>(state.primeIdle,0xe0)!=state.primingMachine
        ||Read<void*>(state.primingMachine,0xc8)!=ability||Read<void*>(state.primaryMachine,0xc8)!=ability)
        return Fail(3366,"original_jett_priming_original_component_relationships_invalid");
    if(!Source())return false;
    state.timerQueryValid=Event(state.primed,L"/Script/ShooterGame.TimedStateComponent.GetTimerLength",4,&state.before);
    state.after=state.before;
    if(!state.timerQueryValid||!std::isfinite(state.before))return Fail(3367,"original_jett_priming_original_timer_query_invalid");
    if(state.before==state.source){state.unchanged=true;state.ready=true;state.error=0;state.stage="original_jett_priming_original_duration_already_correct";return true;}
    if(state.before!=0)return Fail(3368,"original_jett_priming_nonzero_original_timer_conflict_preserved");
    if(Read<uint64_t>(state.primed,0x48c)!=0)return Fail(3369,"original_jett_priming_original_timer_tuning_tag_conflict");
    if(!InitialConditions()||!Profile())return false;
    int32_t entry=OriginalEntry;state.attempted=true;++state.originalCalls;
    if(!Event(ability,GraphPath,4,&entry))return Fail(3370,"original_jett_priming_original_graph_dispatch_failed");
    ++state.completedCalls;
    state.originalQueryValue=Read<float>(state.frame,static_cast<size_t>(state.outValueOffset));
    state.originalQuerySucceeded=static_cast<uint8_t>(Read<uint8_t>(state.frame,static_cast<size_t>(state.successOffset))&state.successProperty.boolFieldMask);
    state.timerQueryValid=Event(state.primed,L"/Script/ShooterGame.TimedStateComponent.GetTimerLength",4,&state.after);
    state.postconditions=state.timerQueryValid&&state.after==state.source&&state.originalQueryValue==state.source
        &&state.originalQuerySucceeded!=0&&Read<void*>(ability,static_cast<size_t>(state.frameOffset))==state.frame
        &&Read<void*>(ability,0x210)==pawn&&Read<void*>(ability,0x4a8)==pawn
        &&state.sourceRow&&std::memcmp(static_cast<uint8_t*>(state.sourceRow)+24,&state.sourceBytes,24)==0
        &&std::memcmp(state.runtimeValue,&state.sourceBytes,24)==0&&InitialConditions();
    if(!state.postconditions){state.restartRequired=true;return Fail(3371,"original_jett_priming_native_readback_failed_restart_required");}
    state.ready=true;state.error=0;state.stage="original_jett_priming_original_duration_initialized";return true;
}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0003350U;
    const auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;state.restartRequired=true;
    state.stage="original_jett_priming_exception_requires_restart";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* pawn,void* ability){__try{return InitializeImpl(pawn,ability);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool CatchUp(void* pawn,void* ability){return Initialize(pawn,ability);}
inline void WriteProperty(FILE* f,const PropertyInfo& p){
    const auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"field\":\"%p\",\"field_class\":\"%p\",\"first_property\":\"%p\",\"cast_flags\":%llu,\"actual_name\":%llu,\"expected_name\":%llu,\"offset\":%d,\"properties_size\":%d,\"array_dim\":%d,\"element_size\":%d,\"generic_property\":%s,\"name_matches\":%s,\"bounds_valid\":%s,\"bool_field_size\":%u,\"bool_byte_offset\":%u,\"bool_byte_mask\":%u,\"bool_field_mask\":%u}",p.field,p.fieldClass,p.firstProperty,static_cast<unsigned long long>(p.castFlags),static_cast<unsigned long long>(p.actualName),static_cast<unsigned long long>(p.expectedName),p.offset,p.propertiesSize,p.arrayDim,p.elementSize,b(p.genericProperty),b(p.nameMatches),b(p.boundsValid),static_cast<unsigned>(p.boolFieldSize),static_cast<unsigned>(p.boolByteOffset),static_cast<unsigned>(p.boolByteMask),static_cast<unsigned>(p.boolFieldMask));
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;std::wstring temporary(path);temporary+=L".tmp";
    FILE* f=nullptr;if(_wfopen_s(&f,temporary.c_str(),L"wb")||!f)return false;
    const auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"schema_version\":2,\"pid\":%lu,\"stage\":",GetCurrentProcessId());original_players::Text(f,state.stage);
    std::fprintf(f,",\"error\":%u,\"generation\":%u,\"initialized\":%s,\"ready\":%s,\"attempted\":%s,\"unchanged\":%s,\"restart_required\":%s,\"exception_rva\":%llu,\"original_calls\":%llu,\"completed_calls\":%llu",state.error,state.generation,b(state.initialized),b(state.ready),b(state.attempted),b(state.unchanged),b(state.restartRequired),static_cast<unsigned long long>(state.exceptionRva),static_cast<unsigned long long>(state.originalCalls),static_cast<unsigned long long>(state.completedCalls));
    std::fprintf(f,",\"owner_valid\":%s,\"source_valid\":%s,\"profile_valid\":%s,\"frame_valid\":%s,\"initial_states\":%s,\"no_input\":%s,\"no_active_force\":%s,\"timer_query_valid\":%s,\"postconditions\":%s,\"active_force_count\":%d,\"input_edges\":%u,\"activation_input\":%u",b(state.ownerValid),b(state.sourceValid),b(state.profileValid),b(state.frameValid),b(state.initialStates),b(state.noInput),b(state.noActiveForce),b(state.timerQueryValid),b(state.postconditions),state.activeForceCount,state.inputEdges,static_cast<unsigned>(state.activationInput));
    std::fprintf(f,",\"duration_before\":%.9g,\"original_source_duration\":%.9g,\"duration_after\":%.9g,\"original_query_value\":%.9g,\"original_query_succeeded\":%u,\"pawn\":\"%p\",\"ability\":\"%p\",\"primed_state\":\"%p\",\"primary_machine\":\"%p\",\"priming_machine\":\"%p\",\"tuning\":\"%p\",\"source_table\":\"%p\",\"graph\":\"%p\",\"frame\":\"%p\",\"original_frame\":\"%p\",\"original_entry\":1170",state.before,state.source,state.after,state.originalQueryValue,static_cast<unsigned>(state.originalQuerySucceeded),state.pawn,state.ability,state.primed,state.primaryMachine,state.primingMachine,state.tuning,state.sourceTable,state.graph,state.frame,state.nativeFrame);
    std::fprintf(f,",\"profile\":{\"failures\":%u,\"type\":\"%p\",\"graph_outer\":\"%p\",\"class_graph\":\"%p\",\"class_frame_field\":\"%p\",\"class_frame_offset\":%d,\"frame_getter\":\"%p\",\"frame_getter_rva\":%llu,\"function_flags\":%u,\"params_size\":%u,\"script\":\"%p\",\"script_size\":%d,\"script_capacity\":%d,\"graph_properties_size\":%d,\"script_readable\":%s,\"anchors_match\":%s,\"entry_return\":%u,\"primitive_layouts_match\":%s,\"float_field_class\":\"%p\",\"bool_field_class\":\"%p\",\"anchor_offsets\":[0,5,1170,1248,1289,1330,1371,1412,3818,3820],\"anchor_expected\":[76,78,20,25,25,25,25,77,4,83],\"anchor_actual\":[",state.profileFailures,state.type,state.graphOuter,state.classGraph,state.classFrameField,state.classFrameOffset,state.frameGetter,static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(state.frameGetter)>=engine::image?reinterpret_cast<uintptr_t>(state.frameGetter)-engine::image:reinterpret_cast<uintptr_t>(state.frameGetter)),state.functionFlags,static_cast<unsigned>(state.functionParamsSize),state.script,state.scriptSize,state.scriptCapacity,state.graphPropertiesSize,b(state.scriptReadable),b(state.anchorsMatch),state.entryReturn,b(state.primitiveLayoutsMatch),state.floatFieldClass,state.boolFieldClass);
    for(size_t i=0;i<state.anchorOpcodes.size();++i){if(i)std::fputc(',',f);std::fprintf(f,"%u",static_cast<unsigned>(state.anchorOpcodes[i]));}
    std::fputs("],\"frame_property\":",f);WriteProperty(f,state.frameProperty);
    std::fputs(",\"out_value_property\":",f);WriteProperty(f,state.outProperty);
    std::fputs(",\"success_property\":",f);WriteProperty(f,state.successProperty);
    std::fputs(",\"failure_bits\":{\"graph_owner\":1,\"class_graph\":2,\"frame_offset\":4,\"out_offset\":8,\"success_offset\":16,\"class_frame_field\":32,\"frame_getter\":64,\"function_flags\":128,\"params\":256,\"script_size\":512,\"script_readable\":1024,\"anchors\":2048,\"primitive_layouts\":4096}}",f);
    std::fputs(",\"failed_event\":",f);const auto failed=original_players::Utf8(state.failedEvent);original_players::Text(f,failed.c_str());
    std::fprintf(f,",\"expected_event_size\":%u,\"actual_event_size\":%u,\"manual_state_or_timer_writes\":false,\"replays_begin_play\":false,\"replays_add_states\":false}\n",state.expectedEventSize,state.actualEventSize);
    const bool ok=std::ferror(f)==0,closed=std::fclose(f)==0;
    return ok&&closed&&MoveFileExW(temporary.c_str(),path,MOVEFILE_REPLACE_EXISTING);
}
}
