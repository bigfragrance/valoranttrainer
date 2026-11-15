#pragma once
#include "original_inventory_slots.h"
#include <array>
#include <cmath>
#include <cstddef>

// Ordinary FFA's shipped component owns the window and its native K2 timer.
// This adapter only supplies the absent local spawn/authority notification.
// It has no clock, window table, score write, timer-memory write or BP replay.
// Evidence: work/original-streak-runtime-policy and work/original-streak-policy.
namespace original_timed_streak {
inline constexpr size_t MaxPlayers=32;
inline constexpr const wchar_t* TypePath=L"/Game/GameModes/Components/Comp_TimedKillStreak.Comp_TimedKillStreak_C";
inline constexpr const wchar_t* GraphPath=L"/Game/GameModes/Components/Comp_TimedKillStreak.Comp_TimedKillStreak_C.ExecuteUbergraph_Comp_TimedKillStreak";
inline constexpr const wchar_t* StartPath=L"/Game/GameModes/Components/Comp_TimedKillStreak.Comp_TimedKillStreak_C.StartKillWindow";
inline constexpr const wchar_t* ResetPath=L"/Game/GameModes/Components/Comp_TimedKillStreak.Comp_TimedKillStreak_C.ResetKillStreak";
using WeakIdentity=original_inventory_slots::WeakIdentity;
struct Name {uint32_t comparison=0,number=0,display=0;};
struct Delegate {WeakIdentity object;Name function;};
struct TimerQuery {Delegate delegate;float result=-1;};
static_assert(sizeof(Name)==12&&sizeof(Delegate)==20&&sizeof(TimerQuery)==24);
static_assert(offsetof(TimerQuery,result)==20);
struct Snapshot {
    bool bound=false,identityValid=false,registered=false,begunPlay=false,ownerValid=false,playerInfoValid=false;
    bool frameValid=false,timerExists=false,timerRateDerivedFromNativeQueries=true;
    int32_t count=0,baseWindow=0,nextWindow=0,timeToAdd=0,nativeStreak=0,nativeCurrentLife=0;
    float timerRate=-1,timerRemaining=-1,timerElapsed=-1;
    uint64_t timerHandle=0,epoch=0;
    bool effectHandleCaptured=false;
    std::array<uint64_t,3> effectHandleWords{};
    void *pawn=nullptr,*playerState=nullptr,*playerInfo=nullptr,*component=nullptr,*nativeComponent=nullptr,*frame=nullptr;
};
struct Property {void* field=nullptr;void* fieldClass=nullptr;int32_t offset=-1,bytes=0;};
struct Layout {
    Property count{},base{},next{},add{},info{},frame{},timerHandle{},effectHandle{};
    bool verified=false;
};
struct LayoutFieldObservation {
    bool found=false;
    int32_t offset=0,elementSize=0,arrayDim=0;
    uint64_t castFlags=0;
};
struct Methods {
    void *add=nullptr,*query=nullptr,*start=nullptr,*reset=nullptr,*destroy=nullptr;
    void *remaining=nullptr,*elapsed=nullptr,*clear=nullptr,*getState=nullptr,*getInfo=nullptr;
    void *getStreakComponent=nullptr,*getStreak=nullptr,*getLife=nullptr,*isAlive=nullptr;
};
struct Entry {
    bool used=false,bound=false,created=false,retiring=false;
    void *pawn=nullptr,*playerState=nullptr,*playerInfo=nullptr,*component=nullptr,*nativeComponent=nullptr;
    WeakIdentity pawnIdentity{},stateIdentity{},infoIdentity{},componentIdentity{},nativeIdentity{};
    Delegate resetDelegate{};
    uint64_t epoch=0,observations=0,killNotifications=0,bridgeCalls=0,alreadyDelivered=0,resets=0,alreadyReset=0,clears=0;
    Snapshot last{};
};
struct EventObservation {
    uint64_t sequence=0;void *object=nullptr,*function=nullptr,*objectClass=nullptr,*functionOwner=nullptr;
    uintptr_t exec=0;uint32_t flags=0;uint16_t bytes=0;
    size_t index=MaxPlayers;uint64_t epoch=0;
    int32_t sourceCount=-1;bool effectHandleCaptured=false;
    std::array<uint64_t,3> effectHandleWords{};
    const char* role="none";
};
struct FaultFrame {
    uintptr_t pc=0,sp=0,rbx=0,rbp=0,rsi=0,rdi=0,r12=0,r13=0,r14=0,r15=0;
};
struct MetadataReference {
    void *object=nullptr,*owner=nullptr,*type=nullptr;WeakIdentity identity{};
    const char* role="none";uintptr_t exec=0;uint32_t flags=0;uint16_t bytes=0;bool function=false;
};
struct Diagnostics {
    bool initialized=false,profileValid=false,restartRequired=false;
    unsigned error=0,bound=0;
    uint64_t binds=0,created=0,reused=0,observations=0,bridgeCalls=0,alreadyDelivered=0;
    uint64_t resets=0,alreadyReset=0,clearCalls=0,destroyCalls=0,retired=0,staleRetired=0;
    uintptr_t exceptionRva=0;
    const char* stage="original_timed_streak_not_initialized";
    const wchar_t* failedEvent=nullptr;
    uint16_t expectedEventSize=0,actualEventSize=0;
    void *world=nullptr,*type=nullptr,*defaults=nullptr,*graph=nullptr,*library=nullptr,*timerLibrary=nullptr;
    int32_t cdoBaseWindow=0;
    uint64_t countObservations=0,diagnosticBatches=0,lastDiagnosticSampleMs=0,layoutProfileCaptures=0;
    bool layoutProfileCaptured=false;
    void *layoutProfileType=nullptr,*layoutProfileGraph=nullptr;
    std::array<LayoutFieldObservation,7> layoutFields{};
    const char* operation="none";size_t activeIndex=MaxPlayers;
    EventObservation activeEvent{},lastEvent{},faultEvent{};
    uint64_t eventSequence=0,metadataChecks=0;
    bool metadataCaptured=false;uint32_t expiredMetadataIndex=UINT32_MAX;
    const char* expiredMetadataRole="none";
    uintptr_t exceptionAddress=0,exceptionStack=0,exceptionRbp=0,exceptionAddressOperand=0;
    uint64_t exceptionAccessType=0;
    bool exceptionContextCaptured=false,faultStackStoppedByException=false;
    const char *faultOperation="none",*faultStackStop="not_captured";size_t faultIndex=MaxPlayers;
    CONTEXT exceptionContext{};
    std::array<FaultFrame,32> faultFrames{};size_t faultFrameCount=0;
};
inline Diagnostics state;
inline Methods methods;
inline Layout layout;
inline std::array<Entry,MaxPlayers> entries{};
inline std::array<MetadataReference,19> metadata{};
inline DWORD gameThread=0;
inline const Diagnostics& State(){return state;}
inline const Entry* Player(size_t index){return index<entries.size()?&entries[index]:nullptr;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;return false;}
inline bool Thread(){return state.initialized&&!state.restartRequired&&gameThread==GetCurrentThreadId()&&original_players::Thread();}
// These labels only describe the real call in progress. They never dispatch
// another original event, change a handle or decide a gameplay result.
inline void DiagnosticOperation(const char* operation,size_t index=MaxPlayers){
    if(gameThread&&gameThread!=GetCurrentThreadId())return;
    state.operation=operation;state.activeIndex=index;
}
inline bool ValidateMetadata(void* function=nullptr,void* object=nullptr,bool structuralOnly=false);
inline const char* EventRole(void* function){
    if(function==methods.add)return "component_factory";if(function==methods.query)return "component_query";
    if(function==methods.start)return "start_kill_window";if(function==methods.reset)return "reset_kill_streak";
    if(function==methods.destroy)return "destroy_component";if(function==methods.remaining)return "timer_remaining";
    if(function==methods.elapsed)return "timer_elapsed";if(function==methods.clear)return "timer_clear";
    if(function==methods.getState)return "pawn_player_state";if(function==methods.getInfo)return "owner_player_info";
    if(function==methods.getStreakComponent)return "native_streak_component";if(function==methods.getStreak)return "native_streak_count";
    if(function==methods.getLife)return "native_life_count";if(function==methods.isAlive)return "victim_alive";
    return "unknown";
}
inline bool Resolve(void*& result,const wchar_t* name,uint16_t bytes){
    result=engine::Find(name);const uint16_t actual=Read<uint16_t>(result,0xbe);
    if(result&&actual==bytes)return true;
    state.failedEvent=name;state.expectedEventSize=bytes;state.actualEventSize=actual;return false;
}
inline bool Event(void* object,void* method,void* params){
    if(state.metadataCaptured&&!ValidateMetadata(method,object))return false;
    auto process=Read<void*>(Read<void**>(object,0),0x298);
    if(!object||!method||!process)return false;
    const auto previous=state.activeEvent;EventObservation observed{};
    observed.sequence=++state.eventSequence;observed.object=object;observed.function=method;observed.objectClass=Read<void*>(object,0x10);
    observed.functionOwner=Read<void*>(method,0x28);observed.exec=Read<uintptr_t>(method,0xe0);
    observed.flags=Read<uint32_t>(method,0xb8);observed.bytes=Read<uint16_t>(method,0xbe);observed.role=EventRole(method);
    observed.index=state.activeIndex;
    for(size_t n=0;n<entries.size();++n){const auto& entry=entries[n];
        if(entry.used&&(object==entry.pawn||object==entry.playerState||object==entry.playerInfo
           ||object==entry.component||object==entry.nativeComponent)){observed.index=n;break;}}
    if(observed.index<entries.size()){
        const auto& entry=entries[observed.index];observed.epoch=entry.epoch;
        if(entry.bound&&entry.component){
            observed.sourceCount=Read<int32_t>(entry.component,static_cast<size_t>(layout.count.offset));
            if(layout.effectHandle.field)observed.effectHandleCaptured=original_players::Read(entry.component,
                static_cast<size_t>(layout.effectHandle.offset),observed.effectHandleWords);
        }
    }
    state.activeEvent=observed;
    // Preserve the original call once. If it faults, the outer native filter
    // observes this actual operation/entry before ordinary stack unwinding.
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,method,params);
    state.lastEvent=observed;state.activeEvent=previous;return true;
}
inline Property Field(void* structure,const wchar_t* name,int32_t bytes){
    Property result;if(!structure||!name||bytes<=0)return result;Name wanted{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&wanted,name,1);
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(structure,&wanted,3);
    auto fieldClass=Read<void*>(field,8);const uint64_t flags=Read<uint64_t>(fieldClass,0x18);
    const int32_t offset=Read<int32_t>(field,0x44),total=Read<int32_t>(structure,0x60);
    if(!field||(flags&0x8001)!=0x8001||Read<uint64_t>(field,0x20)!=Read<uint64_t>(&wanted,0)
       ||Read<int32_t>(field,0x30)!=1||Read<int32_t>(field,0x34)!=bytes||offset<0||total<bytes||offset>total-bytes)return result;
    result.field=field;result.fieldClass=fieldClass;result.offset=offset;result.bytes=bytes;return result;
}
inline bool Live(const WeakIdentity& identity,void* object){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool AllLive(const Entry& e){
    return Live(e.pawnIdentity,e.pawn)&&Live(e.stateIdentity,e.playerState)&&Live(e.infoIdentity,e.playerInfo)
        &&Live(e.componentIdentity,e.component)&&Live(e.nativeIdentity,e.nativeComponent);
}
inline void MetadataObjects(std::array<void*,19>& objects){
    objects={state.type,state.defaults,state.graph,state.library,state.timerLibrary,
        methods.add,methods.query,methods.start,methods.reset,methods.destroy,methods.remaining,methods.elapsed,
        methods.clear,methods.getState,methods.getInfo,methods.getStreakComponent,methods.getStreak,methods.getLife,methods.isAlive};
}
inline bool CaptureMetadata(){
    std::array<void*,19> objects{};MetadataObjects(objects);
    constexpr const char* roles[]{"component_class","component_cdo","ubergraph","factory_cdo","timer_cdo",
        "component_factory","component_query","start_kill_window","reset_kill_streak","destroy_component","timer_remaining",
        "timer_elapsed","timer_clear","pawn_player_state","owner_player_info","native_streak_component","native_streak_count","native_life_count","victim_alive"};
    for(size_t n=0;n<metadata.size();++n){auto& item=metadata[n];item={};item.object=objects[n];item.role=roles[n];item.function=n==2||n>=5;
        if(!item.object||!original_inventory_slots::CaptureIdentity(item.object,item.identity)||!Live(item.identity,item.object)){
            state.expiredMetadataIndex=static_cast<uint32_t>(n);state.expiredMetadataRole=item.role;
            return Fail(3734,"original_timed_streak_original_metadata_identity_capture_failed");}
        item.owner=Read<void*>(item.object,0x28);item.type=Read<void*>(item.object,0x10);
        if(item.function){item.exec=Read<uintptr_t>(item.object,0xe0);item.flags=Read<uint32_t>(item.object,0xb8);item.bytes=Read<uint16_t>(item.object,0xbe);}
    }
    state.metadataCaptured=true;return true;
}
inline bool ValidateMetadata(void* function,void* object,bool structuralOnly){
    ++state.metadataChecks;
    std::array<void*,19> objects{};MetadataObjects(objects);
    bool functionFound=function==nullptr;
    for(size_t n=0;n<metadata.size();++n){const auto& item=metadata[n];
        if(structuralOnly&&n>=5)continue;
        if(function&&item.object!=function&&item.object!=object)continue;
        if(item.object==function)functionFound=true;
        const bool same=objects[n]==item.object&&Live(item.identity,item.object);
        const bool shape=same&&Read<void*>(item.object,0x28)==item.owner&&Read<void*>(item.object,0x10)==item.type
            &&(!item.function||(Read<uintptr_t>(item.object,0xe0)==item.exec&&Read<uint32_t>(item.object,0xb8)==item.flags
                &&Read<uint16_t>(item.object,0xbe)==item.bytes));
        if(!shape){state.expiredMetadataIndex=static_cast<uint32_t>(n);state.expiredMetadataRole=item.role;state.restartRequired=true;
            return Fail(same?3736U:3735U,same?"original_timed_streak_original_metadata_shape_changed":"original_timed_streak_original_metadata_identity_expired");}
    }
    if(!functionFound)return Fail(3737,"original_timed_streak_event_function_not_in_captured_original_cache");
    return true;
}
inline void CaptureLayoutProfile(){
    if(state.layoutProfileCaptured&&state.layoutProfileType==state.type&&state.layoutProfileGraph==state.graph)return;
    if(!state.type||!state.graph)return;
    const wchar_t* names[]={L"KillCount",L"BaseKillWindow",L"KillWindow",L"TimeToAdd",L"PlayerInfo",L"UberGraphFrame",L"CallFunc_K2_SetTimerDelegate_ReturnValue"};
    for(size_t i=0;i<state.layoutFields.size();++i){
        Name name{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&name,names[i],1);
        auto structure=i==6?state.graph:state.type;
        auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(structure,&name,3);
        auto& value=state.layoutFields[i];value.found=field!=nullptr;
        value.offset=Read<int32_t>(field,0x44);value.elementSize=Read<int32_t>(field,0x34);value.arrayDim=Read<int32_t>(field,0x30);
        value.castFlags=Read<uint64_t>(Read<void*>(field,8),0x18);
    }
    state.layoutProfileType=state.type;state.layoutProfileGraph=state.graph;state.layoutProfileCaptured=true;++state.layoutProfileCaptures;
}
inline bool InitializeImpl(void* world){
    if(state.initialized)return state.world==world&&Thread()&&ValidateMetadata();
    if(!world||world!=original_players::world||!original_players::Thread())return Fail(3700,"original_timed_streak_world_or_thread_invalid");
    state.world=world;gameThread=GetCurrentThreadId();
    auto base=engine::Find(L"/Script/Engine.ActorComponent");
    state.type=base?engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t,void*)>(0x1d0c6e0)
        (base,nullptr,TypePath,nullptr,0,nullptr):nullptr;
    if(!original_players::Derived(state.type,base))return Fail(3701,"original_timed_streak_original_class_missing");
    state.defaults=engine::DefaultObject(state.type);state.graph=engine::Find(GraphPath);
    bool ok=Resolve(methods.add,L"/Script/ShooterGame.ShooterBlueprintLibrary.AddComponentByClass",24)
        &&Resolve(methods.query,L"/Script/Engine.Actor.GetComponentByClass",16)
        &&Resolve(methods.start,StartPath,24)&&Resolve(methods.reset,ResetPath,0)
        &&Resolve(methods.destroy,L"/Script/Engine.ActorComponent.K2_DestroyComponent",8)
        &&Resolve(methods.remaining,L"/Script/Engine.KismetSystemLibrary.K2_GetTimerRemainingTimeDelegate",24)
        &&Resolve(methods.elapsed,L"/Script/Engine.KismetSystemLibrary.K2_GetTimerElapsedTimeDelegate",24)
        &&Resolve(methods.clear,L"/Script/Engine.KismetSystemLibrary.K2_ClearTimerDelegate",20)
        &&Resolve(methods.getState,L"/Script/ShooterGame.ShooterCharacter.GetShooterPlayerState",8)
        &&Resolve(methods.getInfo,L"/Script/ShooterGame.ShooterPlayerState.GetOwnerExclusivePlayerInfo",8)
        &&Resolve(methods.getStreakComponent,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.GetKillStreakComponentMutable",8)
        &&Resolve(methods.getStreak,L"/Script/ShooterGame.KillStreakComponent.GetNumKillsStreak",4)
        &&Resolve(methods.getLife,L"/Script/ShooterGame.KillStreakComponent.GetNumKillsCurrentLife",4)
        &&Resolve(methods.isAlive,L"/Script/ShooterGame.ShooterCharacter.IsAlive",1);
    if(!ok)return Fail(3702,"original_timed_streak_function_abi_mismatch");
    auto libraryClass=engine::Find(L"/Script/ShooterGame.ShooterBlueprintLibrary");
    auto timerClass=engine::Class(0x45aa110);
    if(!libraryClass||Read<void*>(methods.add,0x28)!=libraryClass||Read<void*>(methods.remaining,0x28)!=timerClass)
        return Fail(3703,"original_timed_streak_library_owner_mismatch");
    state.library=engine::DefaultObject(libraryClass);state.timerLibrary=engine::DefaultObject(timerClass);
    layout.count=Field(state.type,L"KillCount",4);layout.base=Field(state.type,L"BaseKillWindow",4);
    layout.next=Field(state.type,L"KillWindow",4);layout.add=Field(state.type,L"TimeToAdd",4);
    layout.info=Field(state.type,L"PlayerInfo",8);layout.frame=Field(state.type,L"UberGraphFrame",8);
    layout.timerHandle=Field(state.graph,L"CallFunc_K2_SetTimerDelegate_ReturnValue",8);
    // Optional opaque observation only; no field/flag/handle is set by this
    // adapter. Original Play/Update/Stop consume this exact24-byte handle.
    layout.effectHandle=Field(state.type,L"FXC_Handle",24);
    // Retain raw failure diagnostics even if a strict field check below fails.
    // These pinned class properties do not change during the owned session.
    CaptureLayoutProfile();
    const auto integer=Field(methods.getStreak,L"ReturnValue",4),object=Field(methods.getInfo,L"ReturnValue",8);
    auto classTable=Read<void**>(state.type,0);
    layout.verified=layout.count.field&&layout.base.field&&layout.next.field&&layout.add.field&&layout.info.field
        &&layout.frame.field&&layout.timerHandle.field&&integer.field&&object.field&&state.defaults&&state.graph
        &&layout.count.fieldClass==integer.fieldClass&&layout.base.fieldClass==integer.fieldClass
        &&layout.next.fieldClass==integer.fieldClass&&layout.add.fieldClass==integer.fieldClass&&layout.info.fieldClass==object.fieldClass
        &&Read<void*>(state.graph,0x28)==state.type&&Read<void*>(methods.start,0x28)==state.type
        &&Read<void*>(methods.reset,0x28)==state.type&&Read<void*>(state.type,0x298)==state.graph
        &&Read<void*>(classTable,0x3c0)==reinterpret_cast<void*>(engine::image+0x40c0c80)
        &&Read<int32_t>(Read<void*>(state.type,0x290),0x44)==layout.frame.offset;
    if(!layout.verified||!state.library||!state.timerLibrary)return Fail(3704,"original_timed_streak_reflected_property_profile_mismatch");
    state.cdoBaseWindow=Read<int32_t>(state.defaults,static_cast<size_t>(layout.base.offset));
    if(state.cdoBaseWindow<=0||state.cdoBaseWindow>120)return Fail(3705,"original_timed_streak_cdo_window_invalid");
    if(!CaptureMetadata())return false;
    state.profileValid=true;state.initialized=true;state.error=0;state.stage="original_timed_streak_native_factory_ready";return true;
}
inline bool Timer(Entry& e,float& remaining,float& elapsed){
    TimerQuery rq{e.resetDelegate,-1},eq{e.resetDelegate,-1};
    if(!Event(state.timerLibrary,methods.remaining,&rq)||!Event(state.timerLibrary,methods.elapsed,&eq)
       ||!std::isfinite(rq.result)||!std::isfinite(eq.result))return false;
    remaining=rq.result;elapsed=eq.result;return true;
}
inline bool ObserveImpl(size_t index,Snapshot& result){
    result={};if(!Thread()||index>=entries.size())return false;
    state.activeIndex=index;
    if(!ValidateMetadata(nullptr,nullptr,true))return false;
    auto& e=entries[index];if(!e.bound)return Fail(3706,"original_timed_streak_source_not_bound");
    if(!AllLive(e))return Fail(3707,"original_timed_streak_weak_identity_expired");
    result.bound=true;result.identityValid=true;result.epoch=e.epoch;result.pawn=e.pawn;result.playerState=e.playerState;
    result.playerInfo=e.playerInfo;result.component=e.component;result.nativeComponent=e.nativeComponent;
    result.registered=(Read<uint8_t>(e.component,0xa0)&1)!=0;
    result.begunPlay=(Read<uint8_t>(e.component,0xc2)&0x40)!=0;
    result.ownerValid=Read<void*>(e.component,0xc8)==e.pawn&&Read<void*>(e.component,0x28)==e.pawn;
    result.playerInfoValid=Read<void*>(e.component,static_cast<size_t>(layout.info.offset))==e.playerInfo
        &&Read<void*>(e.playerState,0xa18)==e.playerInfo&&Read<void*>(e.pawn,0x488)==e.playerState;
    result.count=Read<int32_t>(e.component,static_cast<size_t>(layout.count.offset));
    result.baseWindow=Read<int32_t>(e.component,static_cast<size_t>(layout.base.offset));
    result.nextWindow=Read<int32_t>(e.component,static_cast<size_t>(layout.next.offset));
    result.timeToAdd=Read<int32_t>(e.component,static_cast<size_t>(layout.add.offset));
    if(layout.effectHandle.field)result.effectHandleCaptured=original_players::Read(e.component,
        static_cast<size_t>(layout.effectHandle.offset),result.effectHandleWords);
    if(!result.registered||!result.begunPlay||!result.ownerValid||!result.playerInfoValid
       ||result.count<0||result.baseWindow!=state.cdoBaseWindow||result.nextWindow<=0||result.timeToAdd<0
       ||!Event(e.nativeComponent,methods.getStreak,&result.nativeStreak)
       ||!Event(e.nativeComponent,methods.getLife,&result.nativeCurrentLife))
        return Fail(3708,"original_timed_streak_native_lifecycle_or_counter_readback_failed");
    auto frame=Read<void*>(e.component,static_cast<size_t>(layout.frame.offset));
    auto getter=Read<void*(*)(void*,void*,void*)>(Read<void**>(state.type,0),0x3c0);
    result.frameValid=frame&&getter&&getter(state.type,e.component,state.graph)==frame;
    result.frame=frame;
    if(!result.frameValid)return Fail(3709,"original_timed_streak_original_persistent_frame_missing");
    result.timerHandle=Read<uint64_t>(frame,static_cast<size_t>(layout.timerHandle.offset));
    if(!Timer(e,result.timerRemaining,result.timerElapsed))return Fail(3710,"original_timed_streak_original_timer_query_failed");
    result.timerExists=result.timerRemaining>=0&&result.timerElapsed>=0;
    // Same game-thread instant: both values are original TimerManager queries.
    // This is a derived rate observation, never a locally advanced clock.
    if(result.timerExists)result.timerRate=result.timerRemaining+result.timerElapsed;
    e.last=result;++e.observations;++state.observations;return true;
}
inline bool ObserveCountImpl(size_t index,int32_t& count){
    count=0;if(!Thread()||index>=entries.size())return false;
    state.activeIndex=index;
    if(!ValidateMetadata(nullptr,nullptr,true))return false;
    auto& e=entries[index];if(!e.bound)return Fail(3706,"original_timed_streak_source_not_bound");
    if(!AllLive(e))return Fail(3707,"original_timed_streak_weak_identity_expired");
    // Same original field and lifecycle guards as the full observation, read
    // afresh on EVERY call. No cached count, time estimate or delayed expiry.
    const bool registered=(Read<uint8_t>(e.component,0xa0)&1)!=0;
    const bool begun=(Read<uint8_t>(e.component,0xc2)&0x40)!=0;
    const bool owner=Read<void*>(e.component,0xc8)==e.pawn&&Read<void*>(e.component,0x28)==e.pawn;
    const bool info=Read<void*>(e.component,static_cast<size_t>(layout.info.offset))==e.playerInfo
        &&Read<void*>(e.playerState,0xa18)==e.playerInfo&&Read<void*>(e.pawn,0x488)==e.playerState;
    const auto actual=Read<int32_t>(e.component,static_cast<size_t>(layout.count.offset));
    if(!registered||!begun||!owner||!info||actual<0
       ||Read<int32_t>(e.component,static_cast<size_t>(layout.base.offset))!=state.cdoBaseWindow
       ||Read<int32_t>(e.component,static_cast<size_t>(layout.next.offset))<=0
       ||Read<int32_t>(e.component,static_cast<size_t>(layout.add.offset))<0)
        return Fail(3708,"original_timed_streak_native_lifecycle_or_counter_readback_failed");
    count=actual;++state.countObservations;return true;
}
inline bool SampleDiagnosticsImpl(bool force){
    if(!Thread())return false;
    const auto now=GetTickCount64();
    if(!force&&state.lastDiagnosticSampleMs&&now-state.lastDiagnosticSampleMs<1000)return true;
    state.lastDiagnosticSampleMs=now;
    for(size_t i=0;i<entries.size();++i)if(entries[i].bound){Snapshot sample;if(!ObserveImpl(i,sample))return false;}
    ++state.diagnosticBatches;return true;
}
inline bool BindImpl(size_t index,void* pawn,void* playerState,uint64_t epoch){
    if(!Thread()||index>=entries.size()||!original_players::IsA(pawn,original_players::ShooterCharacterClass)
       ||!original_players::IsA(playerState,original_players::ShooterStateClass)||!original_players::Registered(playerState)
       ||engine::function<int(*)(void*)>(0x3efc820)(pawn)!=0)return Fail(3711,"original_timed_streak_bind_requires_original_standalone_player");
    auto& e=entries[index];
    if(e.used){Snapshot copy;return e.bound&&e.pawn==pawn&&e.playerState==playerState&&e.epoch==epoch
        ?ObserveImpl(index,copy):Fail(3712,"original_timed_streak_old_spawn_requires_retire");}
    void *actualState=nullptr,*info=nullptr,*native=nullptr;
    const auto& roster=original_players::players[index];
    if(!roster.ready||!roster.registered||roster.character!=pawn||roster.state!=playerState
       ||!Event(pawn,methods.getState,&actualState)||actualState!=playerState
       ||!Event(playerState,methods.getInfo,&info)||info!=Read<void*>(playerState,0xa18)
       ||!Event(info,methods.getStreakComponent,&native)||!original_players::IsA(native,0x6904f00))
        return Fail(3713,"original_timed_streak_player_and_owner_info_identity_mismatch");
    struct Query {void* type;void* result=nullptr;} query{state.type};
    if(!Event(pawn,methods.query,&query))return Fail(3714,"original_timed_streak_original_component_query_failed");
    e.pawn=pawn;e.playerState=playerState;e.playerInfo=info;e.nativeComponent=native;e.epoch=epoch;
    if(!original_inventory_slots::CaptureIdentity(pawn,e.pawnIdentity)
       ||!original_inventory_slots::CaptureIdentity(playerState,e.stateIdentity)
       ||!original_inventory_slots::CaptureIdentity(info,e.infoIdentity)
       ||!original_inventory_slots::CaptureIdentity(native,e.nativeIdentity))return Fail(3715,"original_timed_streak_player_weak_identity_missing");
    e.used=true;e.component=query.result;
    if(!e.component){
        struct Add {void* actor;void* type;void* result=nullptr;} add{pawn,state.type};
        static_assert(sizeof(Add)==24);
        // The exact original Applicator factory. Registration calls BeginPlay
        // on the already-live pawn; never manually replay ReceiveBeginPlay.
        if(!Event(state.library,methods.add,&add)||!add.result)return Fail(3716,"original_timed_streak_original_add_component_failed");
        e.component=add.result;e.created=true;++state.created;
    }else ++state.reused;
    if(Read<void*>(e.component,0x10)!=state.type||!original_inventory_slots::CaptureIdentity(e.component,e.componentIdentity))
        return Fail(3717,"original_timed_streak_component_class_or_weak_identity_mismatch");
    query.result=nullptr;
    if(!Event(pawn,methods.query,&query)||query.result!=e.component)return Fail(3718,"original_timed_streak_original_component_query_did_not_return_new_component");
    e.resetDelegate.object=e.componentIdentity;
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&e.resetDelegate.function,L"ResetKillStreak",1);
    e.bound=true;++state.bound;++state.binds;
    Snapshot snapshot;if(!ObserveImpl(index,snapshot))return false;
    state.error=0;state.stage="original_timed_streak_component_beginplay_verified";return true;
}
inline bool Clear(Entry& e){
    if(!Event(state.timerLibrary,methods.clear,&e.resetDelegate))return Fail(3719,"original_timed_streak_original_clear_timer_failed");
    ++state.clearCalls;++e.clears;
    float remaining=-1,elapsed=-1;
    return Timer(e,remaining,elapsed)&&remaining<0&&elapsed<0
        ?true:Fail(3720,"original_timed_streak_original_timer_still_exists_after_clear");
}
inline bool AfterKillImpl(size_t index,void* victimPawn,void* response,int32_t expectedCount){
    if(!Thread()||index>=entries.size()||expectedCount<=0||expectedCount==INT32_MAX
       ||!original_players::IsA(victimPawn,original_players::ShooterCharacterClass)
       ||!original_players::IsA(response,0x64fdaa0))return Fail(3721,"original_timed_streak_requires_real_native_point_kill");
    auto& e=entries[index];Snapshot before;
    if(!ObserveImpl(index,before)||victimPawn==e.pawn)return Fail(3722,"original_timed_streak_kill_source_or_victim_invalid");
    uint8_t alive=1;
    if(!Event(victimPawn,methods.isAlive,&alive)||alive)return Fail(3723,"original_timed_streak_kill_victim_still_alive");
    ++e.killNotifications;
    if(before.count==expectedCount){++e.alreadyDelivered;++state.alreadyDelivered;state.stage="original_timed_streak_native_kill_already_delivered";return true;}
    if(before.count!=expectedCount-1)return Fail(3724,"original_timed_streak_expected_count_not_previous_native_count");
    struct Kill {void* killer;void* victim;void* response;} params{e.pawn,victimPawn,response};
    static_assert(sizeof(Kill)==24);
    ++e.bridgeCalls;++state.bridgeCalls;
    // Only this component's missing authority listener is supplied. Do not
    // broadcast OnAuthGotKill broadly or replay any unrelated kill consumer.
    if(!Event(e.component,methods.start,&params))return Fail(3725,"original_timed_streak_original_start_kill_window_failed");
    Snapshot after;
    if(!ObserveImpl(index,after)||after.count!=expectedCount||!after.timerExists
       ||after.timerRate<=0||std::abs(after.timerRate-static_cast<float>(before.nextWindow))>.01f)
        return Fail(3726,"original_timed_streak_original_kill_and_timer_not_observed");
    state.error=0;state.stage="original_timed_streak_original_kill_window_started";return true;
}
inline bool ResetImpl(size_t index){
    Snapshot before;if(!ObserveImpl(index,before))return false;
    auto& e=entries[index];if(!Clear(e))return false;
    if(before.count==0&&before.nextWindow==before.baseWindow&&before.timeToAdd==0
       &&before.nativeStreak==0&&before.nativeCurrentLife==0){
        ++e.alreadyReset;++state.alreadyReset;
    }else{
        ++e.resets;++state.resets;
        if(!Event(e.component,methods.reset,nullptr))return Fail(3727,"original_timed_streak_original_reset_failed");
    }
    Snapshot after;
    if(!ObserveImpl(index,after)||after.count||after.timeToAdd||after.nextWindow!=after.baseWindow
       ||after.nativeStreak||after.nativeCurrentLife||after.timerExists)
        return Fail(3728,"original_timed_streak_original_reset_readback_failed");
    state.error=0;state.stage="original_timed_streak_original_reset_verified";return true;
}
inline bool BeforeRetireImpl(size_t index){
    if(!Thread()||index>=entries.size())return false;
    auto& e=entries[index];if(!e.used)return true;
    if(!e.component){e={};return true;}
    if(!Live(e.componentIdentity,e.component)){
        // The native delegate stores this same weak identity. An expired
        // object cannot invoke ResetKillStreak on a later UObject at its address.
        if(e.bound&&state.bound)--state.bound;e={};++state.staleRetired;return true;
    }
    if(!AllLive(e)||Read<void*>(e.component,0xc8)!=e.pawn)return Fail(3729,"original_timed_streak_retire_requires_live_original_owner");
    e.retiring=true;
    if(!Clear(e))return false;
    // Retire precedes respawn: resetting an OLD component after the new life
    // starts could reset their shared OwnerExclusivePlayerInfo. Reject that.
    if(original_players::players[index].character!=e.pawn)return Fail(3730,"original_timed_streak_retire_must_precede_replacement_pawn");
    if(e.bound&&!ResetImpl(index))return false;
    auto component=e.component;
    if(!Event(component,methods.destroy,&e.pawn))return Fail(3731,"original_timed_streak_original_destroy_component_failed");
    ++state.destroyCalls;
    struct Query {void* type;void* result=nullptr;} query{state.type};
    if(!Event(e.pawn,methods.query,&query)||query.result==component
       ||(Live(e.componentIdentity,component)&&(Read<uint8_t>(component,0xa0)&1)!=0))
        return Fail(3732,"original_timed_streak_destroy_did_not_unregister_component");
    if(e.bound&&state.bound)--state.bound;e={};++state.retired;
    state.error=0;state.stage="original_timed_streak_old_timer_and_component_retired";return true;
}
inline bool FaultUnwindOne(CONTEXT& context){
    // Windows metadata unwinding only. No game event, handler execution,
    // speculative stack scan, allocation, file I/O or waiting in this filter.
    __try{
        DWORD64 image=0;
        if(auto function=RtlLookupFunctionEntry(context.Rip,&image,nullptr)){
            PVOID data=nullptr;DWORD64 frame=0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER,image,context.Rip,function,&context,&data,&frame,nullptr);
        }else{DWORD64 caller=0;
            if(!original_players::Read(reinterpret_cast<void*>(context.Rsp),0,caller))return false;
            context.Rip=caller;context.Rsp+=sizeof(DWORD64);}
        return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){state.faultStackStoppedByException=true;return false;}
}
inline void CaptureFaultStack(const CONTEXT& original){
    auto cursor=original;
    const auto tib=reinterpret_cast<const NT_TIB*>(NtCurrentTeb());
    const auto low=reinterpret_cast<uintptr_t>(tib->StackLimit),high=reinterpret_cast<uintptr_t>(tib->StackBase);
    state.faultFrameCount=0;state.faultStackStop="maximum_depth";state.faultStackStoppedByException=false;
    for(size_t depth=0;depth<state.faultFrames.size()&&cursor.Rip;++depth){
        auto& frame=state.faultFrames[state.faultFrameCount++];
        frame={cursor.Rip,cursor.Rsp,cursor.Rbx,cursor.Rbp,cursor.Rsi,cursor.Rdi,cursor.R12,cursor.R13,cursor.R14,cursor.R15};
        if(cursor.Rsp<low||cursor.Rsp>high||high-cursor.Rsp<sizeof(DWORD64)){
            state.faultStackStop="outside_actual_thread_stack";return;}
        const auto pc=cursor.Rip,sp=cursor.Rsp;
        if(!FaultUnwindOne(cursor)){state.faultStackStop=state.faultStackStoppedByException?"windows_unwind_fault":"leaf_return_unreadable";return;}
        if(cursor.Rsp<sp||(cursor.Rsp==sp&&cursor.Rip==pc)){state.faultStackStop="no_forward_unwind_progress";return;}
        if(!cursor.Rip){state.faultStackStop="end_of_stack";return;}
    }
}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0003700U;
    const auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionAddress=pc;state.exceptionRva=pc>=engine::image?pc-engine::image:pc;
    state.faultOperation=state.operation;state.faultIndex=state.activeIndex;state.faultEvent=state.activeEvent;
    if(exception&&exception->ExceptionRecord&&exception->ExceptionRecord->NumberParameters>=2){
        state.exceptionAccessType=exception->ExceptionRecord->ExceptionInformation[0];
        state.exceptionAddressOperand=exception->ExceptionRecord->ExceptionInformation[1];}
    if(exception&&exception->ContextRecord){const auto& context=*exception->ContextRecord;
        state.exceptionContext=context;state.exceptionContextCaptured=true;
        state.exceptionStack=context.Rsp;state.exceptionRbp=context.Rbp;CaptureFaultStack(context);}
    state.restartRequired=true;state.stage="original_timed_streak_native_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* world){DiagnosticOperation("initialize");__try{return InitializeImpl(world);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Bind(size_t index,void* pawn,void* playerState,uint64_t epoch){DiagnosticOperation("bind",index);__try{return BindImpl(index,pawn,playerState,epoch);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Observe(size_t index,Snapshot& snapshot){DiagnosticOperation("observe",index);__try{return ObserveImpl(index,snapshot);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool ObserveCount(size_t index,int32_t& count){DiagnosticOperation("observe_count",index);__try{return ObserveCountImpl(index,count);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool SampleDiagnostics(bool force=false){DiagnosticOperation("sample_diagnostics");__try{return SampleDiagnosticsImpl(force);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool AfterKill(size_t index,void* victimPawn,void* response,int32_t expectedCount){DiagnosticOperation("after_kill",index);__try{return AfterKillImpl(index,victimPawn,response,expectedCount);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Reset(size_t index){DiagnosticOperation("reset",index);__try{return ResetImpl(index);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool BeforeRetire(size_t index){DiagnosticOperation("before_retire",index);__try{return BeforeRetireImpl(index);}__except(Capture(GetExceptionInformation())){return false;}}
inline void WriteOpaqueHandle(FILE* f,bool captured,const std::array<uint64_t,3>& words){
    std::fprintf(f,"{\"captured\":%s,\"qwords\":[\"0x%016llx\",\"0x%016llx\",\"0x%016llx\"],\"written_by_adapter\":false}",
        captured?"true":"false",static_cast<unsigned long long>(words[0]),static_cast<unsigned long long>(words[1]),static_cast<unsigned long long>(words[2]));
}
inline void WriteEventObservation(FILE* f,const EventObservation& event){
    std::fprintf(f,"{\"sequence\":%llu,\"index\":%zu,\"epoch\":%llu,\"role\":",static_cast<unsigned long long>(event.sequence),event.index,static_cast<unsigned long long>(event.epoch));
    original_players::Text(f,event.role);
    std::fprintf(f,",\"object\":\"%p\",\"object_class\":\"%p\",\"function\":\"%p\",\"function_owner\":\"%p\",\"actual_exec\":\"0x%llx\",\"actual_function_flags\":%u,\"actual_function_parms_size\":%u,\"source_count_before_call\":%d,\"component_effect_handle_before_call\":",
        event.object,event.objectClass,event.function,event.functionOwner,static_cast<unsigned long long>(event.exec),event.flags,event.bytes,event.sourceCount);
    WriteOpaqueHandle(f,event.effectHandleCaptured,event.effectHandleWords);std::fputc('}',f);
}
inline void WriteCachedDiagnostics(FILE* f){
    std::fputs(",\"operation\":",f);original_players::Text(f,state.operation);
    std::fprintf(f,",\"active_index\":%zu,\"event_sequence\":%llu,\"metadata_captured\":%s,\"metadata_checks\":%llu,\"expired_metadata_index\":%u,\"expired_metadata_role\":",
        state.activeIndex,static_cast<unsigned long long>(state.eventSequence),state.metadataCaptured?"true":"false",static_cast<unsigned long long>(state.metadataChecks),state.expiredMetadataIndex);
    original_players::Text(f,state.expiredMetadataRole);
    std::fputs(",\"active_event\":",f);WriteEventObservation(f,state.activeEvent);
    std::fputs(",\"last_event\":",f);WriteEventObservation(f,state.lastEvent);
    std::fputs(",\"fault_event\":",f);WriteEventObservation(f,state.faultEvent);
    std::fputs(",\"fault_operation\":",f);original_players::Text(f,state.faultOperation);
    std::fprintf(f,",\"fault_index\":%zu,\"restart_required\":%s,\"original_image_base\":\"0x%llx\",\"exception_address\":\"0x%llx\",\"exception_context_captured\":%s,\"exception_access_type\":%llu,\"exception_address_operand\":\"0x%llx\",\"exception_context\":{",
        state.faultIndex,state.restartRequired?"true":"false",static_cast<unsigned long long>(engine::image),static_cast<unsigned long long>(state.exceptionAddress),state.exceptionContextCaptured?"true":"false",static_cast<unsigned long long>(state.exceptionAccessType),static_cast<unsigned long long>(state.exceptionAddressOperand));
    const auto& c=state.exceptionContext;
    std::fprintf(f,"\"rip\":\"0x%llx\",\"rsp\":\"0x%llx\",\"rax\":\"0x%llx\",\"rbx\":\"0x%llx\",\"rcx\":\"0x%llx\",\"rdx\":\"0x%llx\",\"rbp\":\"0x%llx\",\"rsi\":\"0x%llx\",\"rdi\":\"0x%llx\",\"r8\":\"0x%llx\",\"r9\":\"0x%llx\",\"r10\":\"0x%llx\",\"r11\":\"0x%llx\",\"r12\":\"0x%llx\",\"r13\":\"0x%llx\",\"r14\":\"0x%llx\",\"r15\":\"0x%llx\"},\"fault_stack_stop\":",
        static_cast<unsigned long long>(c.Rip),static_cast<unsigned long long>(c.Rsp),static_cast<unsigned long long>(c.Rax),static_cast<unsigned long long>(c.Rbx),static_cast<unsigned long long>(c.Rcx),static_cast<unsigned long long>(c.Rdx),static_cast<unsigned long long>(c.Rbp),static_cast<unsigned long long>(c.Rsi),static_cast<unsigned long long>(c.Rdi),static_cast<unsigned long long>(c.R8),static_cast<unsigned long long>(c.R9),static_cast<unsigned long long>(c.R10),static_cast<unsigned long long>(c.R11),static_cast<unsigned long long>(c.R12),static_cast<unsigned long long>(c.R13),static_cast<unsigned long long>(c.R14),static_cast<unsigned long long>(c.R15));
    original_players::Text(f,state.faultStackStop);
    std::fprintf(f,",\"fault_stack_stopped_by_exception\":%s,\"fault_frames\":[",state.faultStackStoppedByException?"true":"false");
    for(size_t n=0;n<state.faultFrameCount;++n){const auto& frame=state.faultFrames[n];
        std::fprintf(f,"%s{\"pc\":\"0x%llx\",\"sp\":\"0x%llx\",\"rbx\":\"0x%llx\",\"rbp\":\"0x%llx\",\"rsi\":\"0x%llx\",\"rdi\":\"0x%llx\",\"r12\":\"0x%llx\",\"r13\":\"0x%llx\",\"r14\":\"0x%llx\",\"r15\":\"0x%llx\"}",n?",":"",static_cast<unsigned long long>(frame.pc),static_cast<unsigned long long>(frame.sp),static_cast<unsigned long long>(frame.rbx),static_cast<unsigned long long>(frame.rbp),static_cast<unsigned long long>(frame.rsi),static_cast<unsigned long long>(frame.rdi),static_cast<unsigned long long>(frame.r12),static_cast<unsigned long long>(frame.r13),static_cast<unsigned long long>(frame.r14),static_cast<unsigned long long>(frame.r15));}
    std::fprintf(f,"],\"effect_handle_field\":{\"found\":%s,\"offset\":%d,\"bytes\":%d},\"metadata\":[",layout.effectHandle.field?"true":"false",layout.effectHandle.offset,layout.effectHandle.bytes);
    for(size_t n=0;n<metadata.size();++n){const auto& item=metadata[n];
        std::fprintf(f,"%s{\"index\":%zu,\"role\":",n?",":"",n);original_players::Text(f,item.role);
        std::fprintf(f,",\"object\":\"%p\",\"owner\":\"%p\",\"type\":\"%p\",\"weak_index\":%d,\"weak_serial\":%d,\"is_function\":%s,\"exec_at_capture\":\"0x%llx\",\"flags_at_capture\":%u,\"parms_size_at_capture\":%u}",
            item.object,item.owner,item.type,item.identity.index,item.identity.serial,item.function?"true":"false",static_cast<unsigned long long>(item.exec),item.flags,item.bytes);}
    std::fputs("],\"diagnostic_stack_scan\":false,\"diagnostic_original_event_replay\":false",f);
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"initialized\":%s,\"profile_valid\":%s,\"error\":%u,\"stage\":",b(state.initialized),b(state.profileValid),state.error);
    original_players::Text(f,state.stage);
    WriteCachedDiagnostics(f);
    if(state.type&&state.graph){
        std::fprintf(f,",\"layout_profile\":{\"graph_class_slot_matches\":%s,\"frame_getter_rva\":%llu,\"class_frame_property_offset\":%d,\"fields\":[",Read<void*>(state.type,0x298)==state.graph?"true":"false",static_cast<unsigned long long>(Read<uintptr_t>(Read<void**>(state.type,0),0x3c0)-engine::image),Read<int32_t>(Read<void*>(state.type,0x290),0x44));
        for(size_t i=0;i<state.layoutFields.size();++i){const auto& value=state.layoutFields[i];
            std::fprintf(f,"%s{\"index\":%zu,\"found\":%s,\"offset\":%d,\"element_size\":%d,\"array_dim\":%d,\"cast_flags\":%llu}",i?",":"",i,value.found?"true":"false",value.offset,value.elementSize,value.arrayDim,static_cast<unsigned long long>(value.castFlags));}
        std::fputs("]}",f);
    }
    std::fprintf(f,",\"cdo_base_window\":%d,\"bound\":%u,\"binds\":%llu,\"created\":%llu,\"reused\":%llu,\"observations\":%llu,\"bridge_calls\":%llu,\"already_delivered\":%llu,\"resets\":%llu,\"already_reset\":%llu,\"clear_calls\":%llu,\"destroy_calls\":%llu,\"retired\":%llu,\"stale_retired\":%llu,\"failed_event_expected_bytes\":%u,\"failed_event_actual_bytes\":%u,\"exception_rva\":\"0x%llx\",\"entries\":[",
        state.cdoBaseWindow,state.bound,static_cast<unsigned long long>(state.binds),static_cast<unsigned long long>(state.created),static_cast<unsigned long long>(state.reused),static_cast<unsigned long long>(state.observations),static_cast<unsigned long long>(state.bridgeCalls),static_cast<unsigned long long>(state.alreadyDelivered),static_cast<unsigned long long>(state.resets),static_cast<unsigned long long>(state.alreadyReset),static_cast<unsigned long long>(state.clearCalls),static_cast<unsigned long long>(state.destroyCalls),static_cast<unsigned long long>(state.retired),static_cast<unsigned long long>(state.staleRetired),state.expectedEventSize,state.actualEventSize,static_cast<unsigned long long>(state.exceptionRva));
    bool first=true;for(size_t i=0;i<entries.size();++i){const auto& e=entries[i];if(!e.used)continue;const auto& s=e.last;
        std::fprintf(f,"%s{\"index\":%zu,\"bound\":%s,\"created\":%s,\"epoch\":%llu,\"pawn\":\"%p\",\"player_state\":\"%p\",\"player_info\":\"%p\",\"component\":\"%p\",\"registered\":%s,\"begun_play\":%s,\"identity_valid\":%s,\"owner_valid\":%s,\"player_info_valid\":%s,\"frame_valid\":%s,\"count\":%d,\"base_window\":%d,\"next_window\":%d,\"time_to_add\":%d,\"native_streak\":%d,\"native_current_life\":%d,\"timer_exists\":%s,\"timer_handle\":%llu,\"timer_rate_seconds\":%.9g,\"timer_remaining_seconds\":%.9g,\"timer_elapsed_seconds\":%.9g,\"timer_rate_derived_from_native_queries\":true,\"bridge_calls\":%llu,\"already_delivered\":%llu,\"resets\":%llu,\"already_reset\":%llu,\"effect_handle\":",
            first?"":",",i,b(e.bound),b(e.created),static_cast<unsigned long long>(e.epoch),e.pawn,e.playerState,e.playerInfo,e.component,b(s.registered),b(s.begunPlay),b(s.identityValid),b(s.ownerValid),b(s.playerInfoValid),b(s.frameValid),s.count,s.baseWindow,s.nextWindow,s.timeToAdd,s.nativeStreak,s.nativeCurrentLife,b(s.timerExists),static_cast<unsigned long long>(s.timerHandle),static_cast<double>(s.timerRate),static_cast<double>(s.timerRemaining),static_cast<double>(s.timerElapsed),static_cast<unsigned long long>(e.bridgeCalls),static_cast<unsigned long long>(e.alreadyDelivered),static_cast<unsigned long long>(e.resets),static_cast<unsigned long long>(e.alreadyReset));
        WriteOpaqueHandle(f,s.effectHandleCaptured,s.effectHandleWords);std::fputc('}',f);first=false;
    }
    std::fprintf(f,"],\"count_observations\":%llu,\"diagnostic_batches\":%llu,\"diagnostic_last_sample_ms\":%llu,\"layout_profile_captures\":%llu,\"window_table_reimplemented\":false,\"local_timeout_clock\":false,\"timer_memory_written\":false,\"aggregate_score_written\":false,\"runtime_verified\":false}\n",
        static_cast<unsigned long long>(state.countObservations),static_cast<unsigned long long>(state.diagnosticBatches),static_cast<unsigned long long>(state.lastDiagnosticSampleMs),static_cast<unsigned long long>(state.layoutProfileCaptures));
    const bool ok=std::ferror(f)==0;std::fclose(f);return ok;
}
}
