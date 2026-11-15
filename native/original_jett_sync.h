#pragma once
#include "original_jett_native.h"
#include <array>

// Observe both shipped E state machines and their authored synchronization.
// Poll is read-only. The separate opt-in initializer can only initialize an
// enabled, cached machine with its own authored StartingState when BOTH native
// current-state fields are empty; it never selects Primed or applies motion.
namespace original_jett_sync {
inline constexpr size_t MaxMachines=16,MaxTriggers=8;
inline constexpr uintptr_t MachineClass=0x67722c0,SyncClass=0x6d6e830;
struct NamedObject {void* object=nullptr;char name[128]{};};
struct Trigger {
    NamedObject state{},machine{},auth{},client{};
    void* owner=nullptr;
    uint8_t ownerRole=255;
    bool machineValid=false,enabled=false,isOriginalCurrent=false;
};
struct Sync {
    NamedObject state{};
    bool typed=false,arrayValid=false,mustBeInState=false,mustObserveTransition=false;
    bool hasEverFailed=false,originalCondition=false;
    float originalLastTickWorldTime=0;
    int32_t count=0;
    size_t captured=0;
    std::array<Trigger,MaxTriggers> triggers{};
};
struct Machine {
    NamedObject object{},starting{},auth{},client{},current{};
    void* owner=nullptr;
    int32_t objectIndex=-1,weakSerial=0,cacheIndex=-1;
    uint8_t ownerRole=255;
    bool ownerValid=false,enabled=false,currentQueryValid=false;
    bool startingMachineValid=false,missingStartingInitialization=false;
    uint64_t transitions=0,originalInitializeCalls=0;
    float originalCurrentSubframeTime=0;
    Sync startingSync{},currentSync{};
};
struct Diagnostics {
    bool initialized=false,queryValid=false,cacheValid=false,actorHasBegunPlay=false,restartRequired=false;
    unsigned error=0;
    uint64_t samples=0,originalInitializeCalls=0;
    int32_t componentCount=0,cacheCount=0;
    size_t captured=0,missingInitializations=0;
    double originalWorldTime=0,originalWorldUnpausedTime=0;
    void *pawn=nullptr,*ability=nullptr,*primary=nullptr;
    const char* stage="original_jett_sync_not_initialized";
    const wchar_t* failedEvent=nullptr;
    uint16_t expectedEventSize=0,actualEventSize=0;
    uintptr_t exceptionRva=0;
};
inline Diagnostics state;
inline std::array<Machine,MaxMachines> machines{};
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;return false;}
inline void Name(NamedObject& observation,void* object){
    if(observation.object!=object)original_weapon::OriginalObjectName(object,observation.name);
    observation.object=object;
}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* parameters){
    auto fn=engine::Find(name);const auto actual=Read<uint16_t>(fn,0xbe);
    if(!object||!fn||actual!=size){state.failedEvent=name;state.expectedEventSize=size;state.actualEventSize=actual;return false;}
    auto table=Read<void**>(object,0);auto process=Read<void*>(table,0x298);
    if(!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,fn,parameters);return true;
}
inline bool CaptureSync(void* object,Sync& result){
    Name(result.state,object);result.typed=original_players::IsA(object,SyncClass);
    result.captured=0;result.count=0;result.arrayValid=false;
    if(!result.typed)return true;
    result.mustBeInState=Read<uint8_t>(object,0x3e0)!=0;
    result.mustObserveTransition=Read<uint8_t>(object,0x3e1)!=0;
    result.hasEverFailed=Read<uint8_t>(object,0x3f8)!=0;
    result.originalLastTickWorldTime=Read<float>(object,0x384);
    auto entries=Read<void**>(object,0x3e8);result.count=Read<int32_t>(object,0x3f0);
    const auto capacity=Read<int32_t>(object,0x3f4);
    result.arrayValid=result.count>=0&&result.count<=128&&capacity>=result.count
        &&(!result.count||original_players::Readable(entries,static_cast<size_t>(result.count)*8));
    if(!result.arrayValid)return Fail(3001,"original_jett_sync_authored_trigger_array_invalid");
    for(int32_t i=0;i<result.count&&result.captured<MaxTriggers;++i){
        auto& item=result.triggers[result.captured++];Name(item.state,entries[i]);
        auto machine=Read<void*>(entries[i],0xe0);Name(item.machine,machine);
        item.machineValid=original_players::IsA(machine,MachineClass);
        if(!original_players::IsA(entries[i],0x676b8c0)||!item.machineValid)
            return Fail(3010,"original_jett_sync_authored_trigger_owner_invalid");
        item.owner=Read<void*>(machine,0xc8);item.ownerRole=Read<uint8_t>(item.owner,0x224);
        item.enabled=item.machineValid&&Read<uint8_t>(machine,0x348)!=0;
        Name(item.auth,Read<void*>(machine,0x1c8));Name(item.client,Read<void*>(machine,0x1b0));
        item.isOriginalCurrent=(item.ownerRole==3?item.auth.object:item.client.object)==entries[i];
    }
    // Preserved native7147cd0 is a pure comparison of TriggeringStates against
    // each source machine's role-selected current state; it performs no writes.
    result.originalCondition=engine::function<bool(*)(void*)>(0x7147cd0)(object);
    return true;
}
inline Machine* Record(void* object){
    const auto index=Read<int32_t>(object,0xc);
    const auto serial=index<0?0:engine::function<int32_t(*)(void*,int32_t)>(0x1cf35b0)
        (reinterpret_cast<void*>(engine::image+0xd7b87c0),index);
    if(index<0||serial<=0)return nullptr;
    for(size_t i=0;i<state.captured;++i)if(machines[i].objectIndex==index&&machines[i].weakSerial==serial)return &machines[i];
    if(state.captured==MaxMachines)return nullptr;
    auto& result=machines[state.captured++];result={};result.objectIndex=index;result.weakSerial=serial;
    Name(result.object,object);return &result;
}
inline bool PollImpl(){
    if(!state.initialized||state.restartRequired||!original_players::Thread())return false;
    if(original_jett::skills[2].actor!=state.ability||original_jett::State().pawn!=state.pawn
       ||Read<void*>(state.ability,0x4a8)!=state.pawn)
        return Fail(3002,"original_jett_sync_actual_ability_owner_changed");
    uint8_t begun=0;state.actorHasBegunPlay=Event(state.ability,L"/Script/Engine.Actor.HasActorBegunPlay",1,&begun)&&begun!=0;
    state.originalWorldTime=Read<double>(original_jett::State().world,0x740);
    state.originalWorldUnpausedTime=Read<double>(original_jett::State().world,0x748);
    auto cached=Read<void**>(state.ability,0xe30);state.cacheCount=Read<int32_t>(state.ability,0xe38);
    const auto capacity=Read<int32_t>(state.ability,0xe3c);
    state.cacheValid=state.cacheCount>=0&&state.cacheCount<=128&&capacity>=state.cacheCount
        &&(!state.cacheCount||original_players::Readable(cached,static_cast<size_t>(state.cacheCount)*8));
    if(!state.cacheValid)return Fail(3003,"original_jett_sync_original_tick_cache_invalid");
    struct Query {void* type;void** data=nullptr;int32_t count=0,capacity=0;} query{engine::Class(MachineClass)};
    static_assert(sizeof(Query)==24);
    state.queryValid=Event(state.ability,L"/Script/Engine.Actor.K2_GetComponentsByClass",24,&query);
    struct Owner{void* allocation;~Owner(){if(allocation)engine::function<void(*)(void*)>(0x1915720)(allocation);}} owner{query.data};
    if(!state.queryValid||query.count<0||query.count>static_cast<int32_t>(MaxMachines)||query.capacity<query.count
       ||(query.count&&!original_players::Readable(query.data,static_cast<size_t>(query.count)*8)))
        return Fail(3004,"original_jett_sync_original_component_query_invalid");
    state.componentCount=query.count;state.primary=Read<void*>(state.ability,0xe00);state.missingInitializations=0;
    for(int32_t i=0;i<query.count;++i){
        auto component=query.data[i];if(!original_players::IsA(component,MachineClass))return Fail(3005,"original_jett_sync_component_class_mismatch");
        auto item=Record(component);if(!item)return Fail(3006,"original_jett_sync_component_identity_capacity");
        item->owner=Read<void*>(component,0xc8);item->ownerValid=item->owner==state.ability;
        item->ownerRole=Read<uint8_t>(item->owner,0x224);item->enabled=Read<uint8_t>(component,0x348)!=0;
        item->originalCurrentSubframeTime=Read<float>(component,0x1e0);
        item->cacheIndex=-1;for(int32_t k=0;k<state.cacheCount;++k)if(cached[k]==component)item->cacheIndex=k;
        Name(item->starting,Read<void*>(component,0x1e8));Name(item->auth,Read<void*>(component,0x1c8));Name(item->client,Read<void*>(component,0x1b0));
        void* current=nullptr;item->currentQueryValid=Event(component,L"/Script/ShooterGame.StateMachineComponent.GetCurrentState",8,&current);
        if(!item->currentQueryValid)return Fail(3007,"original_jett_sync_original_current_query_failed");
        if(item->current.object&&item->current.object!=current)++item->transitions;
        Name(item->current,current);
        item->startingMachineValid=item->starting.object&&Read<void*>(item->starting.object,0xe0)==component
            &&Read<void*>(item->starting.object,0xc8)==state.ability;
        item->missingStartingInitialization=state.actorHasBegunPlay&&item->ownerValid&&item->enabled
            &&item->cacheIndex>=0&&item->startingMachineValid&&!item->auth.object&&!item->client.object&&!current;
        if(item->missingStartingInitialization)++state.missingInitializations;
        if(!CaptureSync(item->starting.object,item->startingSync)||!CaptureSync(current,item->currentSync))return false;
    }
    ++state.samples;state.error=0;state.stage=state.missingInitializations?"original_jett_sync_missing_native_start_observed":"original_jett_sync_native_machines_observed";
    return true;
}
inline bool InitializeImpl(void* pawn){
    if(!original_players::Thread()||!original_jett::State().loadoutGranted||pawn!=original_jett::State().pawn)
        return Fail(3008,"original_jett_sync_requires_granted_original_local_player");
    if(!state.initialized){state.pawn=pawn;state.ability=original_jett::skills[2].actor;state.initialized=true;}
    if(state.pawn!=pawn||!state.ability)return Fail(3002,"original_jett_sync_actual_ability_owner_changed");
    return PollImpl();
}
inline bool InitializeMissingOriginalStartingStateImpl(){
    if(!PollImpl())return false;
    for(size_t i=0;i<state.captured;++i){
        auto& item=machines[i];if(!item.missingStartingInitialization)continue;
        // Reflection metadata calls this 8B parameter Owner, NOT State. The
        // original Equippable override7116e60 reads its own StartingState and
        // invokes its own transition lifecycle. Never choose a desired state.
        void* owner=state.ability;
        if(!Event(item.object.object,L"/Script/ShooterGame.StateMachineComponent.InitializeStartingState",8,&owner))
            return Fail(3009,"original_jett_sync_original_start_initializer_signature_missing");
        ++item.originalInitializeCalls;++state.originalInitializeCalls;
    }
    return PollImpl();
}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0003000U;
    const auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;state.restartRequired=true;
    state.stage="original_jett_sync_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* pawn){__try{return InitializeImpl(pawn);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Poll(){__try{return PollImpl();}__except(Capture(GetExceptionInformation())){return false;}}
inline bool InitializeMissingOriginalStartingState(){__try{return InitializeMissingOriginalStartingStateImpl();}__except(Capture(GetExceptionInformation())){return false;}}
inline void WriteNamed(FILE* f,const char* key,const NamedObject& item){
    std::fprintf(f,"\"%s\":{\"object\":\"%p\",\"name\":",key,item.object);original_players::Text(f,item.name);std::fputc('}',f);
}
inline void WriteSync(FILE* f,const Sync& item){
    const auto b=[](bool value){return value?"true":"false";};
    std::fputc('{',f);WriteNamed(f,"state",item.state);
    std::fprintf(f,",\"typed\":%s,\"array_valid\":%s,\"must_be_in_state\":%s,\"must_observe_transition\":%s,\"has_ever_failed\":%s,\"original_condition\":%s,\"original_last_tick_world_time\":%.9g,\"trigger_count\":%d,\"captured\":%zu,\"triggers\":[",b(item.typed),b(item.arrayValid),b(item.mustBeInState),b(item.mustObserveTransition),b(item.hasEverFailed),b(item.originalCondition),item.originalLastTickWorldTime,item.count,item.captured);
    for(size_t i=0;i<item.captured;++i){
        const auto& t=item.triggers[i];if(i)std::fputc(',',f);std::fputc('{',f);WriteNamed(f,"state",t.state);
        std::fputc(',',f);WriteNamed(f,"machine",t.machine);std::fputc(',',f);WriteNamed(f,"auth",t.auth);std::fputc(',',f);WriteNamed(f,"client",t.client);
        std::fprintf(f,",\"owner\":\"%p\",\"owner_role\":%u,\"machine_valid\":%s,\"enabled\":%s,\"is_original_current\":%s}",t.owner,static_cast<unsigned>(t.ownerRole),b(t.machineValid),b(t.enabled),b(t.isOriginalCurrent));
    }
    std::fputs("]}",f);
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;std::wstring temporary(path);temporary+=L".tmp";FILE* f=nullptr;
    if(_wfopen_s(&f,temporary.c_str(),L"wb")||!f)return false;const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"pid\":%lu,\"stage\":",GetCurrentProcessId());original_players::Text(f,state.stage);
    std::fprintf(f,",\"error\":%u,\"initialized\":%s,\"query_valid\":%s,\"cache_valid\":%s,\"actor_has_begun_play\":%s,\"samples\":%llu,\"original_initialize_calls\":%llu,\"component_count\":%d,\"cache_count\":%d,\"captured\":%zu,\"missing_initializations\":%zu,\"original_world_time\":%.9g,\"original_world_unpaused_time\":%.9g,\"pawn\":\"%p\",\"ability\":\"%p\",\"primary\":\"%p\",\"restart_required\":%s,\"exception_rva\":%llu,\"machines\":[",state.error,b(state.initialized),b(state.queryValid),b(state.cacheValid),b(state.actorHasBegunPlay),static_cast<unsigned long long>(state.samples),static_cast<unsigned long long>(state.originalInitializeCalls),state.componentCount,state.cacheCount,state.captured,state.missingInitializations,state.originalWorldTime,state.originalWorldUnpausedTime,state.pawn,state.ability,state.primary,b(state.restartRequired),static_cast<unsigned long long>(state.exceptionRva));
    for(size_t i=0;i<state.captured;++i){
        const auto& m=machines[i];if(i)std::fputc(',',f);std::fputc('{',f);WriteNamed(f,"machine",m.object);
        std::fputc(',',f);WriteNamed(f,"starting",m.starting);std::fputc(',',f);WriteNamed(f,"auth",m.auth);std::fputc(',',f);WriteNamed(f,"client",m.client);std::fputc(',',f);WriteNamed(f,"current",m.current);
        std::fprintf(f,",\"owner\":\"%p\",\"owner_valid\":%s,\"owner_role\":%u,\"enabled\":%s,\"cache_index\":%d,\"object_index\":%d,\"weak_serial\":%d,\"current_query_valid\":%s,\"starting_machine_valid\":%s,\"missing_starting_initialization\":%s,\"transitions\":%llu,\"original_initialize_calls\":%llu,\"original_current_subframe_time\":%.9g,\"starting_sync\":",m.owner,b(m.ownerValid),static_cast<unsigned>(m.ownerRole),b(m.enabled),m.cacheIndex,m.objectIndex,m.weakSerial,b(m.currentQueryValid),b(m.startingMachineValid),b(m.missingStartingInitialization),static_cast<unsigned long long>(m.transitions),static_cast<unsigned long long>(m.originalInitializeCalls),m.originalCurrentSubframeTime);
        WriteSync(f,m.startingSync);std::fputs(",\"current_sync\":",f);WriteSync(f,m.currentSync);std::fputc('}',f);
    }
    std::fputs("],\"poll_changes_gameplay\":false,\"forces_target_state_or_motion\":false,\"failed_event\":",f);
    const auto event=original_players::Utf8(state.failedEvent);original_players::Text(f,event.c_str());
    std::fprintf(f,",\"expected_event_size\":%u,\"actual_event_size\":%u}\n",state.expectedEventSize,state.actualEventSize);
    const bool ok=std::ferror(f)==0,closed=std::fclose(f)==0;
    return ok&&closed&&MoveFileExW(temporary.c_str(),path,MOVEFILE_REPLACE_EXISTING);
}
}
