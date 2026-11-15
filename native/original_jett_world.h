#pragma once
#include "original_jett_native.h"
#include <array>

// Passive, source-bound world evidence. This adapter never spawns, advances or
// damages anything. Native dagger damage events remain in a separate typed ring
// and are never represented as a rifle FireShot observation.
namespace original_jett_world {
inline constexpr size_t MaxTracked=256,EventCapacity=32,NativePointBytes=0x130,NativeHitBytes=0xf0;
inline constexpr const wchar_t* SmokeZonePath=L"/Game/Characters/Wushu/S0/Ability_4/GameObject_Wushu_4_SmokeZone.GameObject_Wushu_4_SmokeZone_C";
inline constexpr const wchar_t* SmokeLoopPath=L"/Game/Characters/Wushu/S0/Ability_4/FXC_Wushu_4_SmokeIntroLoop.FXC_Wushu_4_SmokeIntroLoop_C";
struct WeakIdentity{int32_t index=-1,serial=0;};
struct Tracked {
    bool occupied=false;
    WeakIdentity identity{};
    uint32_t initializedBits=0;
    int32_t nextEvent=0;
};
struct DaggerHit {
    uint64_t sequence=0,observationTickMilliseconds=0;
    void *projectile=nullptr,*component=nullptr,*sourcePawn=nullptr,*sourceEquippable=nullptr,*firingPlayerState=nullptr,*damageType=nullptr;
    WeakIdentity identity{};
    float initializedAt=0,damage=0,falloff=0;
    int32_t nativeEventIndex=0;
    uint8_t region=255;
    bool throughWall=false,finiteOriginalGeometry=false;
    engine::Vec start{},direction{};
    std::array<uint8_t,NativeHitBytes> nativeHit{};
};
struct Group {
    void *type=nullptr,*lastActor=nullptr,*lastOwner=nullptr,*lastDamageComponent=nullptr;
    bool classLoaded=false,queryValid=false,lastBegunPlay=false,lastDamageArrayValid=false;
    int32_t worldCount=0,localCount=0,lastNativeDamageCount=0;
    uint64_t localInitializations=0,trackingOverflow=0,damageArrayResets=0;
    engine::Vec lastLocation{};
};
struct Diagnostics {
    bool initialized=false;
    unsigned error=0;
    uint64_t samples=0,daggerNativeEvents=0,daggerInvalidGeometry=0,daggerDamageComponents=0;
    uint64_t smokeLoopActors=0,smokeLoopActorsMaximum=0;
    void *world=nullptr,*pawn=nullptr,*playerState=nullptr,*smokeAbility=nullptr,*daggerAbility=nullptr,*smokeLoopClass=nullptr;
    const char* stage="original_jett_world_not_initialized";
    const wchar_t* failedEvent=nullptr;
    uint16_t expectedEventSize=0,actualEventSize=0;
    uintptr_t exceptionRva=0;
    bool restartRequired=false;
};
inline Diagnostics state;
inline std::array<Group,2> groups{};
inline std::array<std::array<Tracked,MaxTracked>,2> tracked{};
inline std::array<DaggerHit,EventCapacity> daggerEvents{};
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;return false;}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* parameters){
    auto fn=engine::Find(name);const auto actual=Read<uint16_t>(fn,0xbe);
    if(!object||!fn||actual!=size){state.failedEvent=name;state.expectedEventSize=size;state.actualEventSize=actual;return false;}
    auto table=Read<void**>(object,0);auto process=Read<void*>(table,0x298);
    if(!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,fn,parameters);return true;
}
inline bool CaptureIdentity(void* actor,WeakIdentity& identity){
    identity.index=Read<int32_t>(actor,0xc);if(identity.index<0)return false;
    identity.serial=engine::function<int32_t(*)(void*,int32_t)>(0x1cf35b0)
        (reinterpret_cast<void*>(engine::image+0xd7b87c0),identity.index);
    return identity.serial>0;
}
inline bool OwnerChainIsLocal(void* actor){
    for(unsigned depth=0;actor&&depth<8;++depth){
        if(actor==state.smokeAbility||actor==state.pawn)return true;
        auto next=Read<void*>(actor,0x210);if(next==actor)return false;actor=next;
    }
    return false;
}
inline Tracked* Track(size_t kind,void* actor,bool& fresh){
    fresh=false;WeakIdentity identity{};if(!CaptureIdentity(actor,identity))return nullptr;
    Tracked* empty=nullptr;
    for(auto& item:tracked[kind]){
        if(item.occupied&&engine::function<void*(*)(const WeakIdentity*)>(0x1d4cfa0)(&item.identity)==nullptr)item={};
        if(item.occupied&&item.identity.index==identity.index&&item.identity.serial==identity.serial)return &item;
        if(!item.occupied&&!empty)empty=&item;
    }
    if(!empty){++groups[kind].trackingOverflow;return nullptr;}
    empty->occupied=true;empty->identity=identity;fresh=true;return empty;
}
inline bool InitializeImpl(void* world,void* pawn){
    if(!original_players::Thread()||!original_jett::State().loadoutGranted
       ||original_jett::State().world!=world||original_jett::State().pawn!=pawn)
        return Fail(2901,"original_jett_world_requires_granted_original_local_context");
    if(state.initialized)return state.world==world&&state.pawn==pawn;
    state.world=world;state.pawn=pawn;state.playerState=Read<void*>(pawn,0x488);
    state.smokeAbility=original_jett::skills[0].actor;state.daggerAbility=original_jett::skills[3].actor;
    state.initialized=true;state.stage="original_jett_world_passive_observer_ready";return true;
}
inline bool ObserveGroup(size_t kind){
    auto& group=groups[kind];group.worldCount=group.localCount=0;group.queryValid=false;
    group.type=kind?original_jett::projectileObservations[1].type:engine::Find(SmokeZonePath);
    group.classLoaded=group.type&&original_players::Derived(group.type,engine::Class(kind?0x644d990:0x6480c50));
    if(!group.classLoaded)return true;
    struct Query{void* world;void* type;void** actors=nullptr;int32_t count=0,capacity=0;} p{state.world,group.type};
    static_assert(sizeof(Query)==32);
    if(!Event(engine::DefaultObject(engine::Class(0x43a9670)),L"/Script/Engine.GameplayStatics.GetAllActorsOfClass",32,&p))
        return Fail(2902,"original_jett_world_native_actor_query_failed");
    struct Owner{void* allocation;~Owner(){if(allocation)engine::function<void(*)(void*)>(0x1915720)(allocation);}} owner{p.actors};
    group.queryValid=p.count>=0&&p.count<=65536&&p.capacity>=p.count
        &&(!p.count||original_players::Readable(p.actors,static_cast<size_t>(p.count)*8));
    if(!group.queryValid)return Fail(2903,"original_jett_world_native_actor_array_invalid");
    group.worldCount=p.count;
    for(int32_t n=0;n<p.count;++n){
        auto actor=p.actors[n];void* firingState=nullptr;
        const bool local=kind?Event(actor,L"/Script/ShooterGame.Projectile.GetFiringPlayerState",8,&firingState)
            &&firingState==state.playerState:OwnerChainIsLocal(actor);
        if(!local)continue;
        ++group.localCount;group.lastActor=actor;group.lastOwner=Read<void*>(actor,0x210);
        uint8_t begun=0;group.lastBegunPlay=Event(actor,L"/Script/Engine.Actor.HasActorBegunPlay",1,&begun)&&begun!=0;
        Event(actor,L"/Script/Engine.Actor.K2_GetActorLocation",24,&group.lastLocation);
        bool fresh=false;auto item=Track(kind,actor,fresh);if(!item)continue;
        if(!kind){if(fresh)++group.localInitializations;continue;}
        const auto initialized=Read<float>(actor,0x518);if(!std::isfinite(initialized))continue;
        uint32_t bits=0;std::memcpy(&bits,&initialized,4);
        if(fresh||item->initializedBits!=bits){item->initializedBits=bits;item->nextEvent=0;++group.localInitializations;}
        auto component=original_weapon::Component(actor,0x6708820);
        if(!component||Read<void*>(component,0xc8)!=actor)continue;
        ++state.daggerDamageComponents;group.lastDamageComponent=component;
        auto data=Read<uint8_t*>(component,0x370);const auto count=Read<int32_t>(component,0x378),capacity=Read<int32_t>(component,0x37c);
        group.lastNativeDamageCount=count;
        group.lastDamageArrayValid=count>=0&&count<=8192&&capacity>=count
            &&(!count||original_players::Readable(data,static_cast<size_t>(count)*NativePointBytes));
        if(!group.lastDamageArrayValid)continue;
        if(count<item->nextEvent){item->nextEvent=0;++group.damageArrayResets;}
        for(int32_t index=item->nextEvent;index<count;++index){
            auto native=data+static_cast<size_t>(index)*NativePointBytes;
            auto& result=daggerEvents[state.daggerNativeEvents%EventCapacity];result={};
            result.sequence=++state.daggerNativeEvents;result.observationTickMilliseconds=GetTickCount64();
            result.projectile=actor;result.component=component;result.sourcePawn=state.pawn;result.sourceEquippable=state.daggerAbility;
            result.firingPlayerState=firingState;result.identity=item->identity;result.initializedAt=initialized;result.nativeEventIndex=index;
            result.damageType=Read<void*>(native,8);result.damage=Read<float>(native,0x10);result.direction=Read<engine::Vec>(native,0x18);
            result.start=Read<engine::Vec>(actor,0x570);std::memcpy(result.nativeHit.data(),native+0x30,NativeHitBytes);
            result.region=Read<uint8_t>(native,0x120);result.falloff=Read<float>(native,0x124);result.throughWall=Read<uint8_t>(native,0x128)!=0;
            const auto norm=result.direction.x*result.direction.x+result.direction.y*result.direction.y+result.direction.z*result.direction.z;
            result.finiteOriginalGeometry=original_players::Finite(result.start)&&original_players::Finite(result.direction)
                &&std::isfinite(result.damage)&&result.damage>=0&&std::isfinite(result.falloff)&&norm>.9&&norm<1.1;
            if(!result.finiteOriginalGeometry)++state.daggerInvalidGeometry;
        }
        item->nextEvent=count;
    }
    return true;
}
inline bool ObserveSmokeLoops(){
    state.smokeLoopActors=0;state.smokeLoopClass=engine::Find(SmokeLoopPath);
    if(!state.smokeLoopClass||!groups[0].classLoaded)return true;
    struct Query{void* world;void* type;void** actors=nullptr;int32_t count=0,capacity=0;} p{state.world,state.smokeLoopClass};
    if(!Event(engine::DefaultObject(engine::Class(0x43a9670)),L"/Script/Engine.GameplayStatics.GetAllActorsOfClass",32,&p))
        return Fail(2904,"original_jett_world_original_smoke_loop_query_failed");
    struct Owner{void* allocation;~Owner(){if(allocation)engine::function<void(*)(void*)>(0x1915720)(allocation);}} owner{p.actors};
    if(p.count<0||p.count>65536||p.capacity<p.count||(p.count&&!original_players::Readable(p.actors,static_cast<size_t>(p.count)*8)))
        return Fail(2905,"original_jett_world_original_smoke_loop_array_invalid");
    for(int32_t n=0;n<p.count;++n){
        auto actor=p.actors[n];auto target=Read<void*>(actor,0x620);
        if(target&&original_players::Derived(Read<void*>(target,0x10),groups[0].type)
           &&OwnerChainIsLocal(target)&&!Read<uint8_t>(actor,0x5fc))++state.smokeLoopActors;
    }
    state.smokeLoopActorsMaximum=(std::max)(state.smokeLoopActorsMaximum,state.smokeLoopActors);return true;
}
inline bool PollImpl(){
    if(!state.initialized||!original_players::Thread()||state.restartRequired)return false;
    if(original_jett::State().pawn!=state.pawn||Read<void*>(state.pawn,0x488)!=state.playerState)
        return Fail(2906,"original_jett_world_original_local_identity_changed");
    if(!ObserveGroup(0)||!ObserveGroup(1)||!ObserveSmokeLoops())return false;
    ++state.samples;state.error=0;state.stage="original_jett_world_original_actors_and_events_sampled";return true;
}
inline const DaggerHit* EventAt(uint64_t sequence){
    if(!sequence||sequence>state.daggerNativeEvents||state.daggerNativeEvents-sequence>=EventCapacity)return nullptr;
    auto& event=daggerEvents[(sequence-1)%EventCapacity];return event.sequence==sequence?&event:nullptr;
}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0002900U;
    const auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;state.restartRequired=true;
    state.stage="original_jett_world_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* world,void* pawn){__try{return InitializeImpl(world,pawn);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Poll(){__try{return PollImpl();}__except(Capture(GetExceptionInformation())){return false;}}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;std::wstring temporary(path);temporary+=L".tmp";
    FILE* f=nullptr;if(_wfopen_s(&f,temporary.c_str(),L"wb")||!f)return false;
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"pid\":%lu,\"stage\":",GetCurrentProcessId());original_players::Text(f,state.stage);
    std::fprintf(f,",\"error\":%u,\"initialized\":%s,\"samples\":%llu,\"dagger_native_events\":%llu,\"dagger_invalid_geometry\":%llu,\"dagger_damage_component_samples\":%llu,\"smoke_loop_actors\":%llu,\"smoke_loop_actors_maximum\":%llu,\"smoke_loop_class\":\"%p\",\"pawn\":\"%p\",\"smoke_ability\":\"%p\",\"dagger_ability\":\"%p\",\"restart_required\":%s,\"exception_rva\":%llu",state.error,b(state.initialized),static_cast<unsigned long long>(state.samples),static_cast<unsigned long long>(state.daggerNativeEvents),static_cast<unsigned long long>(state.daggerInvalidGeometry),static_cast<unsigned long long>(state.daggerDamageComponents),static_cast<unsigned long long>(state.smokeLoopActors),static_cast<unsigned long long>(state.smokeLoopActorsMaximum),state.smokeLoopClass,state.pawn,state.smokeAbility,state.daggerAbility,b(state.restartRequired),static_cast<unsigned long long>(state.exceptionRva));
    for(size_t i=0;i<groups.size();++i){
        const auto& g=groups[i];
        std::fprintf(f,",\"%s\":{\"class\":\"%p\",\"class_loaded\":%s,\"query_valid\":%s,\"world_count\":%d,\"local_count\":%d,\"local_initializations\":%llu,\"tracking_overflow\":%llu,\"last_actor\":\"%p\",\"last_owner\":\"%p\",\"last_begun_play\":%s,\"last_location\":[%.9g,%.9g,%.9g],\"last_damage_component\":\"%p\",\"last_native_damage_count\":%d,\"last_damage_array_valid\":%s,\"damage_array_resets\":%llu}",i?"dagger":"smoke_zone",g.type,b(g.classLoaded),b(g.queryValid),g.worldCount,g.localCount,static_cast<unsigned long long>(g.localInitializations),static_cast<unsigned long long>(g.trackingOverflow),g.lastActor,g.lastOwner,b(g.lastBegunPlay),g.lastLocation.x,g.lastLocation.y,g.lastLocation.z,g.lastDamageComponent,g.lastNativeDamageCount,b(g.lastDamageArrayValid),static_cast<unsigned long long>(g.damageArrayResets));
    }
    std::fputs(",\"dagger_events\":[",f);
    const auto first=state.daggerNativeEvents>EventCapacity?state.daggerNativeEvents-EventCapacity+1:1;
    for(uint64_t sequence=first;sequence<=state.daggerNativeEvents;++sequence){
        const auto event=EventAt(sequence);if(!event)continue;if(sequence!=first)std::fputc(',',f);
        std::fprintf(f,"{\"sequence\":%llu,\"observation_tick_milliseconds\":%llu,\"projectile\":\"%p\",\"component\":\"%p\",\"source_equippable\":\"%p\",\"firing_player_state\":\"%p\",\"native_event_index\":%d,\"initialized_at\":%.9g,\"damage_type\":\"%p\",\"damage\":%.9g,\"falloff\":%.9g,\"region\":%u,\"through_wall\":%s,\"finite_original_geometry\":%s,\"start\":[%.9g,%.9g,%.9g],\"direction\":[%.9g,%.9g,%.9g],\"native_hit_hex\":\"",static_cast<unsigned long long>(event->sequence),static_cast<unsigned long long>(event->observationTickMilliseconds),event->projectile,event->component,event->sourceEquippable,event->firingPlayerState,event->nativeEventIndex,event->initializedAt,event->damageType,event->damage,event->falloff,static_cast<unsigned>(event->region),b(event->throughWall),b(event->finiteOriginalGeometry),event->start.x,event->start.y,event->start.z,event->direction.x,event->direction.y,event->direction.z);
        for(auto byte:event->nativeHit)std::fprintf(f,"%02x",static_cast<unsigned>(byte));std::fputs("\"}",f);
    }
    std::fputs("],\"applies_damage\":false,\"shares_rifle_shot_ring\":false,\"spawns_or_ticks_skill_actors\":false}\n",f);
    const bool ok=std::ferror(f)==0,closed=std::fclose(f)==0;
    return ok&&closed&&MoveFileExW(temporary.c_str(),path,MOVEFILE_REPLACE_EXISTING);
}
}
