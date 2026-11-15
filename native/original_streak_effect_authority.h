#pragma once
#include "original_timed_streak.h"
#include "original_effects.h"
#include <array>
#include <cmath>
#include <cstddef>

// Local delivery for this one shipped FFA continuous FXC. Ordinary offline
// Play(false) returns an ID without a client record. A real Play(localOnly=true)
// creates the native actor/record; its real handle is associated with the BP.
// Subsequent BP Start/Reset retain original Update/Stop and the original timer.
// Evidence: work/original-streak-continuous-authority. No replacement UI/clock.
namespace original_streak_effect_authority {
inline constexpr size_t MaxPlayers=32,MaxManagerRecords=8192,EffectInfoBytes=0x110;
inline constexpr const wchar_t* EffectPath=L"/Game/GameModes/FXC/FXC_GameMode_FFA_KillStreak.FXC_GameMode_FFA_KillStreak_C";
using WeakIdentity=original_inventory_slots::WeakIdentity;
using Name=original_timed_streak::Name;
struct Handle {uint8_t bytes[24]{};};
struct Data {alignas(8) uint8_t bytes[88]{};};
struct IntData {Data data;Name tag;int32_t value=0;};
struct PlayParams {
    void *type=nullptr,*actor=nullptr,*context=nullptr;
    Data data;
    void* worldContext=nullptr;
    engine::Vec location{},rotation{};
    Name socket{};
    uint8_t localOnly=1,alliance=3,cosmetic=0,replay=0;
    Handle result;
};
struct StopParams {void* actor=nullptr;void* worldContext=nullptr;Handle handle;uint8_t stopType=0,padding[7]{};};
static_assert(sizeof(Handle)==24&&sizeof(Data)==88&&sizeof(IntData)==104);
static_assert(sizeof(PlayParams)==208&&offsetof(PlayParams,data)==24&&offsetof(PlayParams,worldContext)==112);
static_assert(offsetof(PlayParams,localOnly)==180&&offsetof(PlayParams,result)==184);
static_assert(sizeof(StopParams)==48&&offsetof(StopParams,stopType)==40);
struct Entry {
    bool used=false,ready=false,pending=false,created=false,associated=false,retiring=false;
    void *component=nullptr,*pawn=nullptr,*manager=nullptr,*container=nullptr,*record=nullptr;
    WeakIdentity componentIdentity{},pawnIdentity{},managerIdentity{},containerIdentity{};
    Handle handle{},lastRejectedHandle{};
    uint64_t epoch=0,ensures=0,creates=0,nativeDelivered=0,updatesVerified=0,retireStops=0;
    int32_t oldCount=0,scheduledWindow=0,dataDuration=-1,dataKillCount=-1,clientActive=0,pendingEffects=0;
    bool containerTypeValid=false,containerManagerValid=false,containerTargetValid=false,containerStopped=false,dataValid=false;
};
struct Diagnostics {
    bool initialized=false,profileValid=false,restartRequired=false;
    unsigned error=0,tracked=0,ready=0,pending=0;
    uint64_t ensures=0,creates=0,nativeDelivered=0,updatesVerified=0,associations=0,pendingObservations=0;
    uint64_t retireCalls=0,retired=0,retireStops=0,expiredRetired=0,dataCopies=0,dataDestroys=0;
    uintptr_t exceptionRva=0;
    void *world=nullptr,*effectClass=nullptr,*library=nullptr,*tagManager=nullptr;
    void *make=nullptr,*setInt=nullptr,*getInt=nullptr,*play=nullptr,*stop=nullptr,*componentQuery=nullptr;
    void* handleProperty=nullptr;
    int32_t handleOffset=-1;
    Name durationTag{},killCountTag{};
    const char* stage="original_streak_effect_authority_not_initialized";
    const wchar_t* failedEvent=nullptr;
    uint16_t expectedEventSize=0,actualEventSize=0;
};
inline Diagnostics state;
inline std::array<Entry,MaxPlayers> entries{};
inline DWORD gameThread=0;
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;return false;}
inline bool Thread(){return state.initialized&&!state.restartRequired&&gameThread==GetCurrentThreadId()&&original_players::Thread();}
inline bool Live(const WeakIdentity& identity,void* object){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Nonzero(const Handle& handle){const auto low=Read<uint64_t>(&handle,0);return low!=0&&low!=1;}
inline bool Local(const Handle& handle){return Read<uint8_t>(&handle,0x14)!=0;}
inline bool SameID(const Handle& a,const Handle& b){return std::memcmp(a.bytes,b.bytes,16)==0;}
inline bool SameHandle(const Handle& a,const Handle& b){return std::memcmp(a.bytes,b.bytes,24)==0;}
inline bool Resolve(void*& result,const wchar_t* path,uint16_t bytes){
    result=engine::Find(path);const auto actual=Read<uint16_t>(result,0xbe);
    if(result&&actual==bytes)return true;
    state.failedEvent=path;state.expectedEventSize=bytes;state.actualEventSize=actual;return false;
}
inline bool Event(void* object,void* method,void* params){return original_timed_streak::Event(object,method,params);}
inline void CopyData(void* destination,const void* source){
    // Exact FEffectData COPY CONSTRUCTOR, used by original EffectInfo693efa0.
    // 6fe9cf0 is instead AddEffectContext; never use it as a data copier.
    engine::function<void*(*)(void*,const void*)>(0x64611d0)(destination,source);++state.dataCopies;
}
struct DataOwner {
    void* data=nullptr;bool constructed=false;
    ~DataOwner(){if(constructed){engine::function<void(*)(void*)>(0x6461100)(data);++state.dataDestroys;}}
};
inline bool Tag(const wchar_t* literal,Name& result){
    Name name{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&name,literal,1);
    // Original6fe9d1e..5b: the already-initialized GameplayTagsManager and its
    // RequestGameplayTag native API. No manager init, account or online flow.
    engine::function<void*(*)(void*,void*,const void*,bool)>(0x3d00fb0)(state.tagManager,&result,&name,false);
    return original_streak_effect_authority::Read<uint64_t>(&result,0)!=0
        &&original_streak_effect_authority::Read<uint64_t>(&result,0)==original_streak_effect_authority::Read<uint64_t>(&name,0);
}
inline bool InitializeImpl(void* world){
    if(state.initialized)return world==state.world&&Thread();
    const auto& timed=original_timed_streak::State();
    if(!world||world!=original_players::world||!timed.initialized||world!=timed.world||!original_players::Thread())
        return Fail(3750,"original_streak_effect_authority_original_world_or_timer_missing");
    state.world=world;gameThread=GetCurrentThreadId();
    state.effectClass=engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t,void*)>(0x1d0c6e0)
        (engine::Class(0x648ada0),nullptr,EffectPath,nullptr,0,nullptr);
    if(!original_players::Derived(state.effectClass,engine::Class(0x648ada0)))return Fail(3751,"original_streak_effect_authority_original_fxc_class_missing");
    const bool methods=Resolve(state.make,L"/Script/ShooterGame.EffectsBlueprintLibrary.MakeEffectData",88)
        &&Resolve(state.setInt,L"/Script/ShooterGame.EffectsBlueprintLibrary.SetEffectData_Int32",104)
        &&Resolve(state.getInt,L"/Script/ShooterGame.EffectsBlueprintLibrary.GetEffectData_Int32",104)
        &&Resolve(state.play,L"/Script/ShooterGame.EffectsBlueprintLibrary.PlayContinuousEffect",208)
        // Logical ParmsSize ends at the enum byte40; C++ storage remains48.
        &&Resolve(state.stop,L"/Script/ShooterGame.EffectsBlueprintLibrary.StopContinuousEffect",41)
        &&Resolve(state.componentQuery,L"/Script/Engine.Actor.GetComponentByClass",16);
    if(!methods)return Fail(3752,"original_streak_effect_authority_reflected_abi_mismatch");
    auto libraryClass=engine::Find(L"/Script/ShooterGame.EffectsBlueprintLibrary");
    if(!libraryClass||Read<void*>(state.make,0x28)!=libraryClass||Read<void*>(state.play,0x28)!=libraryClass)
        return Fail(3753,"original_streak_effect_authority_original_library_owner_mismatch");
    state.library=engine::DefaultObject(libraryClass);
    const auto handle=original_timed_streak::Field(timed.type,L"FXC_Handle",24);
    const auto output=original_timed_streak::Field(state.play,L"ReturnValue",24);
    const auto data=original_timed_streak::Field(state.make,L"ReturnValue",88);
    const auto input=original_timed_streak::Field(state.setInt,L"EffectData",88);
    state.handleProperty=handle.field;state.handleOffset=handle.offset;
    state.tagManager=Read<void*>(reinterpret_cast<void*>(engine::image+0xda01a80),0);
    state.profileValid=state.library&&handle.field&&output.field&&data.field&&input.field
        &&handle.fieldClass==output.fieldClass&&Read<void*>(handle.field,0x78)==Read<void*>(output.field,0x78)
        &&data.fieldClass==input.fieldClass&&Read<void*>(data.field,0x78)==Read<void*>(input.field,0x78)
        &&state.tagManager&&Tag(L"FXC.Duration",state.durationTag)&&Tag(L"FXC.KillStreak",state.killCountTag);
    if(!state.profileValid)return Fail(3754,"original_streak_effect_authority_native_struct_or_registered_tags_mismatch");
    state.initialized=true;state.error=0;state.stage="original_streak_effect_authority_original_local_factory_ready";return true;
}
inline bool Array(void* manager,size_t offset,void*& data,int32_t& count){
    data=Read<void*>(manager,offset);count=Read<int32_t>(manager,offset+8);const auto capacity=Read<int32_t>(manager,offset+12);
    return count>=0&&count<=static_cast<int32_t>(MaxManagerRecords)&&capacity>=count&&capacity<=65536&&(!count||data);
}
inline bool Locate(Entry& e,const Handle& handle){
    e.record=nullptr;e.container=nullptr;e.ready=false;e.pending=false;e.dataValid=false;
    e.containerTypeValid=e.containerManagerValid=e.containerTargetValid=e.containerStopped=false;
    void *active=nullptr,*pending=nullptr;
    if(!Array(e.manager,0x128,active,e.clientActive)||!Array(e.manager,0xe8,pending,e.pendingEffects))
        return Fail(3755,"original_streak_effect_authority_native_record_array_invalid");
    unsigned matches=0;
    for(int32_t i=0;i<e.clientActive;++i){auto record=static_cast<uint8_t*>(active)+static_cast<size_t>(i)*EffectInfoBytes;
        const auto id=Read<Handle>(record,0);if(!SameID(id,handle))continue;
        if(!SameHandle(id,handle)||Read<void*>(record,0x18)!=state.effectClass)return Fail(3756,"original_streak_effect_authority_active_record_identity_or_type_mismatch");
        e.record=record;e.container=Read<void*>(record,0xf0);++matches;
    }
    for(int32_t i=0;i<e.pendingEffects;++i){auto record=static_cast<uint8_t*>(pending)+static_cast<size_t>(i)*EffectInfoBytes;
        const auto id=Read<Handle>(record,0);if(!SameID(id,handle))continue;
        if(!SameHandle(id,handle)||Read<void*>(record,0x18)!=state.effectClass)return Fail(3757,"original_streak_effect_authority_pending_record_identity_or_type_mismatch");
        e.record=record;e.pending=true;++matches;
    }
    if(matches>1)return Fail(3758,"original_streak_effect_authority_duplicate_native_id");
    if(!matches)return true;
    if(e.pending){++state.pendingObservations;return true;}
    if(!e.container||Read<void*>(e.container,0x10)!=state.effectClass)return Fail(3759,"original_streak_effect_authority_native_container_type_missing");
    e.containerTypeValid=true;e.containerManagerValid=Read<void*>(e.container,0x618)==e.manager;
    e.containerTargetValid=Read<void*>(e.container,0x620)==e.pawn;e.containerStopped=Read<uint8_t>(e.container,0x5fc)!=0;
    if(!e.containerManagerValid||!e.containerTargetValid||e.containerStopped
       ||!original_inventory_slots::CaptureIdentity(e.container,e.containerIdentity))
        return Fail(3760,"original_streak_effect_authority_original_container_ownership_or_lifecycle_invalid");
    e.ready=true;return true;
}
inline bool ReadData(Entry& e,int32_t duration,int32_t oldCount){
    auto source=e.ready?static_cast<const uint8_t*>(e.container)+0x498
        :e.pending?static_cast<const uint8_t*>(e.record)+0x20:nullptr;
    if(!source)return false;
    IntData query;DataOwner owned{&query.data,false};CopyData(&query.data,source);owned.constructed=true;
    query.tag=state.durationTag;query.value=INT32_MIN;
    if(!Event(state.library,state.getInt,&query))return false;e.dataDuration=query.value;
    query.tag=state.killCountTag;query.value=INT32_MIN;
    if(!Event(state.library,state.getInt,&query))return false;e.dataKillCount=query.value;
    e.dataValid=e.dataDuration==duration&&e.dataKillCount==oldCount;return e.dataValid;
}
inline bool Create(Entry& e,int32_t oldCount,int32_t duration,Handle& result){
    IntData data;DataOwner dataOwner{&data.data,false};
    if(!Event(state.library,state.make,&data.data))return Fail(3761,"original_streak_effect_authority_original_make_data_failed");
    dataOwner.constructed=true;data.tag=state.durationTag;data.value=duration;
    if(!Event(state.library,state.setInt,&data))return Fail(3762,"original_streak_effect_authority_original_duration_data_failed");
    data.tag=state.killCountTag;data.value=oldCount;
    if(!Event(state.library,state.setInt,&data))return Fail(3763,"original_streak_effect_authority_original_streak_data_failed");
    PlayParams play;play.type=state.effectClass;play.actor=e.pawn;play.worldContext=e.component;
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&play.socket,L"None",1);
    DataOwner playOwner{&play.data,false};CopyData(&play.data,&data.data);playOwner.constructed=true;
    ++e.creates;++state.creates;
    // Exact original class/data/offsets/filter. localOnly is an INPUT to the
    // real native creator, not an edit to a returned handle or driver/role.
    if(!Event(state.library,state.play,&play)||!Nonzero(play.result)||!Local(play.result))
        return Fail(3764,"original_streak_effect_authority_native_local_creation_did_not_return_local_handle");
    result=play.result;return true;
}
inline void Recount(){
    state.tracked=state.ready=state.pending=0;
    for(const auto& e:entries)if(e.used){++state.tracked;if(e.ready)++state.ready;if(e.pending)++state.pending;}
}
inline bool EnsureImpl(size_t index,void* component,void* pawn,int32_t oldCount,double scheduledWindow){
    if(!state.initialized&&!InitializeImpl(original_timed_streak::State().world))return false;
    if(!Thread()||index>=entries.size()||!component||!pawn||oldCount<0||oldCount==INT32_MAX
       ||!std::isfinite(scheduledWindow)||scheduledWindow<=0||scheduledWindow>120
       ||std::abs(scheduledWindow-std::round(scheduledWindow))>.001)
        return Fail(3765,"original_streak_effect_authority_invalid_original_window_or_source");
    original_timed_streak::Snapshot snapshot;
    if(!original_timed_streak::Observe(index,snapshot)||snapshot.component!=component||snapshot.pawn!=pawn
       ||snapshot.count!=oldCount+1||!snapshot.timerExists||std::abs(snapshot.timerRate-scheduledWindow)>.01)
        return Fail(3766,"original_streak_effect_authority_requires_completed_original_start_and_timer");
    auto& e=entries[index];
    if(e.used&&(e.component!=component||e.pawn!=pawn||e.epoch!=snapshot.epoch))return Fail(3767,"original_streak_effect_authority_old_spawn_requires_retire");
    if(!e.used){
        e.component=component;e.pawn=pawn;e.epoch=snapshot.epoch;
        struct Query {void* type;void* result=nullptr;} query{engine::Class(original_effects::ManagerClass)};
        if(!Event(pawn,state.componentQuery,&query)||!original_players::IsA(query.result,original_effects::ManagerClass)
           ||Read<void*>(query.result,0xc8)!=pawn||(Read<uint8_t>(query.result,0xa0)&1)==0
           ||(Read<uint8_t>(query.result,0xc2)&0x40)==0
           ||!original_inventory_slots::CaptureIdentity(component,e.componentIdentity)
           ||!original_inventory_slots::CaptureIdentity(pawn,e.pawnIdentity)
           ||!original_inventory_slots::CaptureIdentity(query.result,e.managerIdentity))
            return Fail(3768,"original_streak_effect_authority_original_manager_or_source_identity_invalid");
        e.manager=query.result;e.used=true;
    }
    if(!Live(e.componentIdentity,e.component)||!Live(e.pawnIdentity,e.pawn)||!Live(e.managerIdentity,e.manager)
       ||Read<void*>(component,0xc8)!=pawn||Read<void*>(e.manager,0xc8)!=pawn)
        return Fail(3769,"original_streak_effect_authority_live_identity_changed");
    ++state.ensures;++e.ensures;e.oldCount=oldCount;e.scheduledWindow=static_cast<int32_t>(std::round(scheduledWindow));
    auto current=Read<Handle>(component,static_cast<size_t>(state.handleOffset));
    if(!Nonzero(current))return Fail(3770,"original_streak_effect_authority_original_start_returned_empty_or_one_shot_id");
    if(!Locate(e,current))return false;
    if(Local(current)){
        // The original creator (or our earlier successful delivery) already
        // owns the real local record. Original Update has already executed.
        if(!e.ready&&!e.pending)return Fail(3771,"original_streak_effect_authority_local_id_has_no_native_record");
        e.handle=current;
        if(!ReadData(e,e.scheduledWindow,oldCount))return Fail(3772,"original_streak_effect_authority_original_updated_data_not_observed");
        if(e.associated){++e.updatesVerified;++state.updatesVerified;}else{++e.nativeDelivered;++state.nativeDelivered;}
    }else{
        // This is the precise absent local creation boundary, not a request
        // to duplicate an already-received replicated effect.
        if(e.ready||e.pending||oldCount!=0)return Fail(3773,"original_streak_effect_authority_nonlocal_id_not_first_missing_creation");
        e.lastRejectedHandle=current;Handle actual;
        if(!Create(e,oldCount,e.scheduledWindow,actual)||!Locate(e,actual))return false;
        if((!e.ready&&!e.pending)||!ReadData(e,e.scheduledWindow,oldCount))
            return Fail(3774,"original_streak_effect_authority_real_local_record_or_data_missing");
        // Only the original returned POD handle is associated with the exact
        // original reflected BP field. No handle flag or manager table is made.
        if(!SameHandle(Read<Handle>(component,static_cast<size_t>(state.handleOffset)),current)
           ||!original_players::Write(component,static_cast<size_t>(state.handleOffset),actual)
           ||!SameHandle(Read<Handle>(component,static_cast<size_t>(state.handleOffset)),actual))
            return Fail(3775,"original_streak_effect_authority_native_handle_association_failed");
        e.handle=actual;e.created=true;e.associated=true;++state.associations;
    }
    Recount();state.error=0;state.stage=e.pending?"original_streak_effect_authority_waiting_for_original_deferred_flush"
        :"original_streak_effect_authority_real_local_fxc_verified";return true;
}
inline bool BeforeRetireImpl(size_t index){
    if(!state.initialized)return true;
    if(!Thread()||index>=entries.size())return false;++state.retireCalls;
    auto& e=entries[index];if(!e.used)return true;
    if(!Live(e.componentIdentity,e.component)||!Live(e.pawnIdentity,e.pawn)||!Live(e.managerIdentity,e.manager)){
        e={};++state.expiredRetired;Recount();return true;
    }
    e.retiring=true;
    // Usually native Reset already stopped this record. Do not send another
    // Stop just because the BP retains its historical handle bytes.
    if(!Locate(e,e.handle))return false;
    if(e.ready||e.pending){
        if(!Local(e.handle))return Fail(3776,"original_streak_effect_authority_retire_only_owns_local_handle");
        StopParams stop;stop.actor=e.pawn;stop.worldContext=e.component;stop.handle=e.handle;
        if(!Event(state.library,state.stop,&stop)||!Locate(e,e.handle))return Fail(3777,"original_streak_effect_authority_original_stop_failed");
        ++e.retireStops;++state.retireStops;
        if(e.ready||e.pending)return Fail(3778,"original_streak_effect_authority_original_record_not_retired");
    }
    e={};++state.retired;Recount();state.error=0;state.stage="original_streak_effect_authority_old_source_retired";return true;
}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0003750U;
    const auto at=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.restartRequired=true;
    state.stage="original_streak_effect_authority_native_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* world){__try{return InitializeImpl(world);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Ensure(size_t index,void* component,void* pawn,int32_t oldCount,double scheduledWindow){__try{return EnsureImpl(index,component,pawn,oldCount,scheduledWindow);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool BeforeRetire(size_t index){__try{return BeforeRetireImpl(index);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"initialized\":%s,\"profile_valid\":%s,\"error\":%u,\"stage\":",b(state.initialized),b(state.profileValid),state.error);
    original_players::Text(f,state.stage);
    std::fprintf(f,",\"tracked\":%u,\"ready\":%u,\"pending\":%u,\"ensures\":%llu,\"creates\":%llu,\"native_delivered\":%llu,\"updates_verified\":%llu,\"associations\":%llu,\"pending_observations\":%llu,\"retired\":%llu,\"retire_stops\":%llu,\"expired_retired\":%llu,\"data_copies\":%llu,\"data_destroys\":%llu,\"handle_offset\":%d,\"failed_event_expected_bytes\":%u,\"failed_event_actual_bytes\":%u,\"exception_rva\":\"0x%llx\",\"entries\":[",
        state.tracked,state.ready,state.pending,static_cast<unsigned long long>(state.ensures),static_cast<unsigned long long>(state.creates),static_cast<unsigned long long>(state.nativeDelivered),static_cast<unsigned long long>(state.updatesVerified),static_cast<unsigned long long>(state.associations),static_cast<unsigned long long>(state.pendingObservations),static_cast<unsigned long long>(state.retired),static_cast<unsigned long long>(state.retireStops),static_cast<unsigned long long>(state.expiredRetired),static_cast<unsigned long long>(state.dataCopies),static_cast<unsigned long long>(state.dataDestroys),state.handleOffset,state.expectedEventSize,state.actualEventSize,static_cast<unsigned long long>(state.exceptionRva));
    bool first=true;for(size_t i=0;i<entries.size();++i){const auto& e=entries[i];if(!e.used)continue;
        std::fprintf(f,"%s{\"index\":%zu,\"epoch\":%llu,\"component\":\"%p\",\"pawn\":\"%p\",\"manager\":\"%p\",\"container\":\"%p\",\"ready\":%s,\"pending\":%s,\"associated\":%s,\"handle_local\":%s,\"old_count\":%d,\"scheduled_window\":%d,\"effect_duration\":%d,\"effect_kill_count\":%d,\"data_valid\":%s,\"container_type_valid\":%s,\"container_manager_valid\":%s,\"container_target_valid\":%s,\"container_stopped\":%s,\"client_active_count\":%d,\"pending_effect_count\":%d,\"creates\":%llu,\"updates_verified\":%llu}",
            first?"":",",i,static_cast<unsigned long long>(e.epoch),e.component,e.pawn,e.manager,e.container,b(e.ready),b(e.pending),b(e.associated),b(Local(e.handle)),e.oldCount,e.scheduledWindow,e.dataDuration,e.dataKillCount,b(e.dataValid),b(e.containerTypeValid),b(e.containerManagerValid),b(e.containerTargetValid),b(e.containerStopped),e.clientActive,e.pendingEffects,static_cast<unsigned long long>(e.creates),static_cast<unsigned long long>(e.updatesVerified));first=false;
    }
    std::fputs("],\"entry_state_sampled_at\":\"last_ensure_or_retire\",\"only_original_ffa_fxc\":true,\"local_only_is_factory_input\":true,\"returned_handle_flags_edited\":false,\"native_record_tables_written\":false,\"new_timer\":false,\"replacement_hud\":false,\"driver_or_role_modified\":false,\"runtime_verified\":false}\n",f);
    const bool ok=std::ferror(f)==0;std::fclose(f);return ok;
}
}
