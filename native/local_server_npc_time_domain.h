#pragma once
#include "local_server_authority_clock_probe.h"
#include "original_inventory_slots.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

// Explicit missing-host policy over the ORIGINAL GameMode timing producer.
// The public producer itself is never initialized/ticked/funded here. Each
// registered NPC keeps its actual simulated birth-local raw clock. Original
// common-clock APIs establish its public time conversion before publication.
namespace local_server_npc_time_domain {
namespace observer=local_server_authority_clock_probe;
namespace field=local_server_remote_timeline_probe;
inline constexpr size_t MaxPlayers=32;
inline constexpr uint32_t MaxExactIndex=0x00fffffeU;
inline constexpr float Step=1.0f/128.0f;
struct Weak {int32_t index=-1,serial=0;};
struct Array {void* data=nullptr;int32_t count=0,capacity=0;};
struct Source {
    void *mode=nullptr,*timing=nullptr,*gameState=nullptr,*fixed=nullptr,*clock=nullptr;
    uint32_t index=0,step=0;float accumulated=0;double worldTime=0;
};
struct Binding {
    bool used=false,mapped=false,counted=false,seedTeleport=false,carryReserved=false,carrySubmitted=false;
    uint32_t epoch=0,origin=0,baseOffset=0,lastRaw=0,lastWire=0,lastSentWire=0;
    int32_t expectedOffset=0;
    void *pc=nullptr,*ps=nullptr,*pawn=nullptr,*movement=nullptr,*prediction=nullptr,*clock=nullptr;
    Weak pcId{},psId{},pawnId{},movementId{};
    uint32_t firstCaptureRaw=0,firstConvertedRaw=0,firstConvertedWire=0,firstSentWire=0,carryWire=0;
    uint64_t captures=0,conversions=0,submissions=0;
};
struct State {
    bool initialized=false,closed=false,unsupported=false,profiles=false;
    uint32_t error=0,bound=0,lastIndex=0,sourceEpoch=0,beforeIndex=0,seedScopes=0;
    const char* stage="npc_common_time_not_initialized";
    void *world=nullptr,*driver=nullptr;
    Weak worldId{},driverId{},modeId{},timingId{},gameStateId{},fixedId{};
    Source source{};
    uint64_t reads=0,binds=0,retirements=0,seeds=0,pops=0,deepCopies=0,sourceProgress=0;
    double lastProgressAt=0;
};
inline State state{};
inline std::array<Binding,MaxPlayers> bindings{};
inline std::array<uint32_t,MaxPlayers> retiredEpochs{};
inline const State& GetState(){return state;}
inline const Binding* Player(size_t i){return i<bindings.size()&&bindings[i].used?&bindings[i]:nullptr;}
template<class T> inline T Read(const void* p,size_t o=0){return original_players::Value<T>(p,o);}
template<class T> inline bool Put(void* p,size_t o,const T& value){return original_players::Write(p,o,value);}
inline bool Fail(uint32_t code,const char* why){if(!state.unsupported){state.error=code;state.stage=why;state.unsupported=true;}return false;}
inline bool Capture(void* p,Weak& id){
    id.index=Read<int32_t>(p,0xc);if(!p||id.index<0)return false;
    id.serial=engine::function<int32_t(*)(void*,int32_t)>(0x1cf35b0)(reinterpret_cast<void*>(engine::image+0xd7b87c0),id.index);
    return id.serial>0&&engine::function<void*(*)(const Weak*)>(0x1d4cfa0)(&id)==p;
}
inline bool Same(void* p,const Weak& id){return p&&id.index>=0&&id.serial>0&&engine::function<void*(*)(const Weak*)>(0x1d4cfa0)(&id)==p;}
inline void* WorldOf(void* actor){
    auto target=Read<void*>(Read<void**>(actor),0x1b8);
    return target==reinterpret_cast<void*>(engine::image+0x3ed83b0)?reinterpret_cast<void*(*)(void*)>(target)(actor):nullptr;
}
struct Profile {uint32_t rva,bytes;uint64_t fnv;};
// BEGIN GENERATED NPC COMMON MAP PROFILES
inline constexpr Profile Profiles[]{
    {0x1cf35b0,1571,0x40614eae57888396ULL},
    {0x1d4cfa0,1562,0xb050d7efb4cf1a25ULL},
    {0x6499330,174,0x2423cf21a48d620cULL},
    {0x685f510,174,0xf06863bc56d5eda8ULL},
    {0x68f63c0,167,0xd1c98ecc35052fb8ULL},
    {0x69cf3d0,174,0xd28ff1e1986a0d09ULL},
    {0x7890fa0,98,0x9b94cb0594ed7fa4ULL},
    {0x7891010,48,0xe8abc591b29ab71eULL},
    {0x7891280,238,0x862d6647c415794ULL},
    {0x78913a0,318,0x453ad149ae134966ULL},
    {0x7892e10,142,0x773b71bd06054ce5ULL},
    {0x78946d0,129,0x26d747ab5dd7ec59ULL},
    {0x789e910,656,0x166cc623dda3986ULL},
    {0x789ee40,26,0x9031464499c99ce7ULL},
    {0x789ee60,550,0x909a57b98d314de2ULL},
    {0x789edf0,35,0xb5911dd35a38dd26ULL},
    {0x789f090,25,0xe9040b5725abd43aULL},
    {0x789f0b0,29,0x212a448438acad98ULL},
};
// END GENERATED NPC COMMON MAP PROFILES
inline bool ProfilesMatch(){
    if(!observer::VerifyProfiles())return false;
    for(const auto& p:Profiles){const auto bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes))return false;uint64_t h=14695981039346656037ULL;
        for(uint32_t n=0;n<p.bytes;++n)h=(h^bytes[n])*1099511628211ULL;
        if(h!=p.fnv)return false;
    }return true;
}
inline bool ReadSource(Source& source){
    ++state.reads;
    if(!original_players::Thread()||state.closed||state.world!=original_players::world||!Same(state.world,state.worldId)||!Same(state.driver,state.driverId)
       ||Read<void*>(state.world,0x40)!=state.driver||Read<void*>(state.driver,0x178)!=state.world||Read<void*>(state.driver,0xc0))
        return Fail(3850,"npc_common_time_requires_current_owned_server_world");
    if(!observer::Schema(GetTickCount64()))return Fail(3851,"npc_common_time_original_source_schema_unknown");
    source.mode=Read<void*>(state.world,0x170);source.gameState=Read<void*>(state.world,0x178);
    if(!field::TypedInstance(source.mode,0x6c8ec60)||Read<uint8_t>(source.mode,0x224)!=3||WorldOf(source.mode)!=state.world)
        return Fail(3852,"npc_common_time_actual_native_mode_missing_restore_original_mode_lifecycle");
    source.timing=Read<void*>(source.mode,0x868);auto table=Read<void**>(source.timing);
    if(!field::TypedInstance(source.timing,0x6bdf750)||Read<void*>(source.timing,0xc8)!=source.mode
       ||Read<void*>(source.timing,0x28)!=source.mode||!(Read<uint8_t>(source.timing,0xa0)&1)
       ||Read<void*>(table,0x3e0)!=reinterpret_cast<void*>(engine::image+0x7868330)
       ||Read<void*>(table,0x3e8)!=reinterpret_cast<void*>(engine::image+0x7868830)
       ||Read<void*>(table,0x3f8)!=reinterpret_cast<void*>(engine::image+0x78680e0)
       ||Read<void*>(table,0x1b8)!=reinterpret_cast<void*>(engine::image+0x2a64ee0)
       ||engine::function<void*(*)(void*)>(0x2a64ee0)(source.timing)!=state.world)
        return Fail(3853,"npc_common_time_original_timing_component_lifecycle_missing");
    source.fixed=Read<void*>(source.gameState,0xcc0);
    if(!field::TypedInstance(source.gameState,0x6c9d280)||!field::TypedInstance(source.fixed,0x684d360)
       ||Read<void*>(source.timing,0x108)!=source.fixed||Read<void*>(source.fixed,0xc8)!=source.gameState
       ||Read<void*>(source.fixed,0x28)!=source.gameState)
        return Fail(3854,"npc_common_time_original_beginplay_fixed_manager_link_missing");
    const auto schedule=Read<Array>(source.fixed,0xe0);
    if(Read<uint8_t>(source.fixed,0xd9)!=128||schedule.count!=0||schedule.capacity<0||schedule.capacity>4096)
        return Fail(3855,"npc_common_time_rate_or_pending_global_schedule_unsupported");
    source.clock=Read<void*>(source.timing,0xf8);const auto vb=Read<void*>(source.clock,0x10);
    if(Read<void*>(source.clock)!=reinterpret_cast<void*>(engine::image+0xb600a60)
       ||vb!=reinterpret_cast<void*>(engine::image+0xc6b3e90)||Read<int32_t>(vb,4)!=0x990
       ||Read<int32_t>(source.clock,0x24)!=0)
        return Fail(3856,"npc_common_time_original_public_clock_shape_or_origin_changed");
    source.index=Read<uint32_t>(source.clock,0x9a8);source.step=Read<uint32_t>(source.clock,0x9ac);
    source.accumulated=Read<float>(source.clock,0x20);source.worldTime=Read<double>(state.world,0x740);
    if(source.step!=1||source.index>MaxExactIndex||!std::isfinite(source.accumulated)||source.accumulated<0
       ||!std::isfinite(source.worldTime))return Fail(3857,"npc_common_time_original_index_step_or_accumulator_unsupported");
    return true;
}
inline bool CaptureSourceIdentities(const Source& s){return Capture(s.mode,state.modeId)&&Capture(s.timing,state.timingId)
    &&Capture(s.gameState,state.gameStateId)&&Capture(s.fixed,state.fixedId);}
inline bool Observe(){
    if(!state.initialized||state.unsupported||state.closed)return false;Source s{};if(!ReadSource(s))return false;
    const bool same=s.mode==state.source.mode&&s.timing==state.source.timing&&s.gameState==state.source.gameState
        &&s.fixed==state.source.fixed&&s.clock==state.source.clock;
    if(!same||s.index<state.source.index){
        if(state.bound)return Fail(3858,"npc_common_time_native_round_source_reset_requires_life_retirement");
        if(!CaptureSourceIdentities(s))return Fail(3859,"npc_common_time_source_identity_capture_failed");
        ++state.sourceEpoch;state.lastProgressAt=s.worldTime;
    }else if(!Same(s.mode,state.modeId)||!Same(s.timing,state.timingId)||!Same(s.gameState,state.gameStateId)||!Same(s.fixed,state.fixedId))
        return Fail(3860,"npc_common_time_native_source_weak_generation_changed");
    if(s.index>state.source.index){++state.sourceProgress;state.lastProgressAt=s.worldTime;}
    if(s.worldTime-state.lastProgressAt>.25)return Fail(3861,"npc_common_time_original_producer_not_ticking_restore_mode_lifecycle");
    state.source=s;state.stage="npc_common_time_original_public_producer_observed";return true;
}
inline bool Initialize(void* world,void* driver){
    if(state.initialized)return state.world==world&&state.driver==driver&&Observe();
    if(state.closed||state.unsupported||!original_players::Thread())return false;
    state.world=world;state.driver=driver;state.profiles=ProfilesMatch();
    if(!state.profiles)return Fail(3862,"npc_common_time_original_api_profiles_unknown");
    if(!Capture(world,state.worldId)||!Capture(driver,state.driverId))return Fail(3863,"npc_common_time_world_identity_unavailable");
    Source s{};if(!ReadSource(s)||!CaptureSourceIdentities(s))return false;
    state.source=s;state.sourceEpoch=1;state.lastProgressAt=s.worldTime;state.initialized=true;
    state.stage="npc_common_time_initialized_original_mode_source";return true;
}
inline bool RawActorProof(const Binding& b){
    return b.used&&b.epoch&&field::TypedInstance(b.pc,original_players::ShooterControllerClass)&&field::TypedInstance(b.ps,original_players::ShooterStateClass)
        &&field::TypedInstance(b.pawn,original_players::ShooterCharacterClass)&&field::TypedInstance(b.movement,0x6c7d380)
        &&Read<uint8_t>(b.pc,0x224)==3&&Read<uint8_t>(b.pawn,0x224)==3&&Read<uint8_t>(b.pawn,0x128)==2
        &&!Read<void*>(b.pc,0x510)&&!Read<void*>(b.pc,0x708)&&!Read<void*>(b.pc,0x710)
        &&Read<void*>(b.pc,0x4b0)==b.pawn&&Read<void*>(b.pawn,0x4a0)==b.pc&&Read<void*>(b.pc,0x470)==b.ps
        &&Read<void*>(b.pawn,0x488)==b.ps&&Read<void*>(b.ps,0x210)==b.pc&&Read<void*>(b.pawn,0x4f8)==b.movement
        &&Read<void*>(b.movement,0xc8)==b.pawn&&Read<void*>(b.movement,0x190)==b.pawn&&Read<void*>(b.movement,0x1a8)==b.pawn
        &&WorldOf(b.pc)==state.world&&WorldOf(b.ps)==state.world&&WorldOf(b.pawn)==state.world;
}
inline bool PrePredictionActorProof(const Binding& b){return RawActorProof(b)
    &&Same(b.pc,b.pcId)&&Same(b.ps,b.psId)&&Same(b.pawn,b.pawnId)&&Same(b.movement,b.movementId);}
inline bool ActorProof(const Binding& b){return PrePredictionActorProof(b)
    &&b.prediction&&b.clock&&Read<void*>(b.movement,0x7f0)==b.prediction&&b.clock==static_cast<uint8_t*>(b.prediction)+0xf0;
}
inline bool ClockProof(const Binding& b){
    const auto table=Read<void*>(b.clock),vb=Read<void*>(b.clock,0x10);
    return ActorProof(b)&&table==reinterpret_cast<void*>(engine::image+0xb600a60)&&vb
        &&Read<int32_t>(vb,4)==static_cast<int32_t>(b.baseOffset)
        &&Read<void*>(table,0)==reinterpret_cast<void*>(engine::image+0x789e910)
        &&Read<void*>(table,0x20)==reinterpret_cast<void*>(engine::image+0x789edf0)
        &&Read<void*>(table,0x28)==reinterpret_cast<void*>(engine::image+0x789f090)
        &&Read<void*>(table,0x30)==reinterpret_cast<void*>(engine::image+0x789f0b0)
        &&Read<uint32_t>(b.clock,size_t(b.baseOffset)+0x1c)==1
        &&Read<int32_t>(b.clock,0x24)==(b.mapped?b.expectedOffset:0);
}
inline bool ContextProof(){
    return state.initialized&&!state.closed&&!state.unsupported&&original_players::Thread()
        &&Same(state.world,state.worldId)&&Same(state.driver,state.driverId)&&Same(state.source.mode,state.modeId)
        &&Same(state.source.timing,state.timingId)&&Same(state.source.gameState,state.gameStateId)&&Same(state.source.fixed,state.fixedId)
        &&Read<void*>(state.world,0x170)==state.source.mode&&Read<void*>(state.source.mode,0x868)==state.source.timing
        &&Read<void*>(state.source.timing,0xf8)==state.source.clock&&Read<void*>(state.source.timing,0x108)==state.source.fixed
        &&Read<void*>(state.world,0x178)==state.source.gameState&&Read<void*>(state.source.gameState,0xcc0)==state.source.fixed
        &&Read<uint32_t>(state.source.clock,0x9a8)==state.source.index&&Read<uint32_t>(state.source.clock,0x9ac)==1;
}
inline Binding* Owned(void* pawn,void* movement){for(auto& b:bindings)if(b.used&&b.pawn==pawn&&b.movement==movement)return &b;return nullptr;}
inline bool Validate(size_t index,void* pawn,void* movement,uint32_t epoch){
    if(index==0||index>=bindings.size())return false;const auto& b=bindings[index];
    return ContextProof()&&b.mapped&&b.epoch==epoch&&b.pawn==pawn&&b.movement==movement&&ClockProof(b);
}
inline bool PublicIndex(const Binding& b,uint32_t raw,uint32_t& wire){
    if(!b.mapped||!ContextProof()||!ClockProof(b)||uint64_t(raw)+b.origin>MaxExactIndex)return false;
    wire=raw;engine::function<void(*)(void*,uint32_t*)>(0x789ee40)(b.clock,&wire);
    return wire==raw+b.origin;
}
inline bool InitializeOriginalMap(Binding& b){
    alignas(16) uint8_t seed[0x480]{},popped[0x480]{};bool seedBuilt=false,popBuilt=false,okay=false;
    const float originalBudget=Read<float>(b.clock,0x20);
    ++state.seedScopes;
    __try {
        engine::function<void*(*)(void*)>(0x7890fa0)(seed);seedBuilt=true;
        engine::function<void*(*)(void*)>(0x7890fa0)(popped);popBuilt=true;
        auto vb=Read<void*>(b.prediction,8);const int32_t disp=Read<int32_t>(vb,4);
        if(!vb||disp<0||disp>0x1000)return false;
        auto clockBase=static_cast<uint8_t*>(b.prediction)+8+disp;
        if(Read<uint32_t>(clockBase,8)!=0)return false;
        engine::Vec neutral{};
        engine::function<void(*)(void*,void*,float,const engine::Vec*,void*)>(0x78913a0)(seed,b.pawn,Step,&neutral,clockBase);
        engine::function<void(*)(void*,void*,uint32_t)>(0x7891280)(seed,b.pawn,0);
        b.seedTeleport=Read<uint8_t>(seed,0x468)!=0;
        if(!Put(seed,0x1c,b.origin+1))return false;
        engine::function<void(*)(void*,void*,int32_t)>(0x789e910)(b.clock,seed,0);++state.seeds;
        const bool didPop=engine::function<bool(*)(void*,void*)>(0x789ee60)(b.clock,popped);if(didPop)++state.pops;
        b.expectedOffset=-static_cast<int32_t>(b.origin);b.mapped=true;
        uint32_t raw=1;engine::function<void(*)(void*,uint32_t*)>(0x789ee40)(b.clock,&raw);
        float seconds=Step;engine::function<void(*)(void*,float*)>(0x789f0b0)(b.clock,&seconds);
        const float expectedSeconds=static_cast<float>(b.origin+1)*Step;
        const bool originalStatePreserved=Read<uint32_t>(b.clock,size_t(b.baseOffset)+0x18)==0
            &&Read<float>(b.clock,0x20)==originalBudget&&Read<int32_t>(b.clock,0x4d0)==0&&Read<int32_t>(b.clock,0x96c)==0;
        okay=didPop&&Read<uint32_t>(seed,0x1c)==1&&Read<uint32_t>(popped,0x1c)==1&&originalStatePreserved
            &&Read<int32_t>(b.clock,0x24)==b.expectedOffset&&raw==b.origin+1&&seconds==expectedSeconds;
        engine::function<void(*)(void*,float*)>(0x789f090)(b.clock,&seconds);
        okay=okay&&seconds==Step&&ClockProof(b);
    }__finally {
        if(popBuilt)engine::function<void*(*)(void*,uint32_t)>(0x7891010)(popped,0);
        if(seedBuilt)engine::function<void*(*)(void*,uint32_t)>(0x7891010)(seed,0);
        --state.seedScopes;
    }
    return okay;
}
inline bool UnpublishedNativeEvents(const Binding& b){
    auto forces=Read<void*>(b.pawn,0x1100),inventory=Read<void*>(b.pawn,0xc08);
    const auto forceClass=engine::Class(0x685f510);
    if(!Read<uint8_t>(b.pawn,0xa90)||!field::TypedInstance(forces,0x685f510)
       ||Read<void*>(forces,0xc8)!=b.pawn||Read<void*>(forces,0x28)!=b.pawn
       ||!field::Field(forceClass,L"ForceModules",0x150,16)
       ||!field::Field(forceClass,L"AuthActivePredictedForceModules",0x160,16))return false;
    for(const size_t offset:{size_t(0x150),size_t(0x160)}){const auto array=Read<Array>(forces,offset);
        if(array.count!=0||array.capacity<0||array.capacity>4096)return false;}
    if(!field::TypedInstance(inventory,0x6499330)||Read<void*>(inventory,0xc8)!=b.pawn||Read<void*>(inventory,0x2f0)!=b.pawn)return false;
    const original_inventory_slots::Inventory* verified=nullptr;
    for(const auto& cached:original_inventory_slots::inventories)if(cached.ready&&cached.pawn==b.pawn&&cached.object==inventory
        &&original_inventory_slots::ResolveIdentity(cached.pawnIdentity)==b.pawn
        &&original_inventory_slots::ResolveIdentity(cached.inventoryIdentity)==inventory){verified=&cached;break;}
    if(!verified)return false;
    for(const size_t offset:{size_t(0x268),size_t(0x278),size_t(0x280),size_t(0x2a0),size_t(0x320)})
        if(Read<void*>(inventory,offset))return false;
    for(size_t i=0;i<original_inventory_slots::SlotCount;++i){auto slot=Read<void*>(inventory,0x1b8+i*8);
        if(slot!=verified->slots[i].object||!field::TypedInstance(slot,0x68f63c0)
           ||Read<void*>(slot,0x28)!=inventory||Read<void*>(slot,0x40)!=inventory||Read<uint8_t>(slot,0x30)!=i
           ||Read<void*>(slot,0x38))return false;
        if(original_players::IsA(slot,0x69cf3d0)){const auto contents=Read<Array>(slot,0x50);
            if(contents.count!=0||contents.capacity<0||contents.capacity>4096)return false;}
    }
    return true;
}
inline bool BindFresh(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch){
    if(index==0)return true;
    if(!Observe()||index>=bindings.size()||!epoch||epoch<=retiredEpochs[index])return Fail(3864,"npc_common_time_fresh_bind_context_or_epoch_invalid");
    auto& b=bindings[index];state.lastIndex=static_cast<uint32_t>(index);
    if(b.used)return b.pc==pc&&b.ps==ps&&b.pawn==pawn&&Validate(index,pawn,b.movement,epoch)
        ||Fail(3865,"npc_common_time_requires_prior_real_life_retirement");
    b={};b.used=true;b.pc=pc;b.ps=ps;b.pawn=pawn;b.epoch=epoch;b.movement=Read<void*>(pawn,0x4f8);
    const auto& roster=original_players::players[index];
    if(!roster.ready||!roster.registered||roster.character!=pawn||roster.controller!=pc||roster.state!=ps
       ||roster.movement!=b.movement||!original_players::Registered(ps))
        return Fail(3866,"npc_common_time_fresh_bind_is_not_current_registered_npc_roster_tuple");
    // No prediction factory or weak-serial allocation may touch a different
    // controller/player/world tuple before the actual NPC-only links pass.
    if(!RawActorProof(b))return Fail(3866,"npc_common_time_original_actor_tuple_invalid_before_prediction_factory");
    if(!Capture(pc,b.pcId)||!Capture(ps,b.psId)||!Capture(pawn,b.pawnId)||!Capture(b.movement,b.movementId))
        return Fail(3866,"npc_common_time_fresh_actor_identities_unavailable");
    if(!PrePredictionActorProof(b))return Fail(3866,"npc_common_time_original_actor_weak_tuple_changed_before_prediction_factory");
    b.clock=engine::function<void*(*)(void*)>(0x7892e10)(b.movement);b.prediction=Read<void*>(b.movement,0x7f0);
    const auto vb=Read<void*>(b.clock,0x10);const int32_t disp=Read<int32_t>(vb,4);
    if(disp<0||disp>0x1000)return Fail(3867,"npc_common_time_actual_clock_virtual_base_unknown");b.baseOffset=static_cast<uint32_t>(disp);
    if(!ClockProof(b)||Read<uint32_t>(b.clock,size_t(disp)+0x18)||Read<float>(b.clock,0x20)!=0
       ||Read<uint8_t>(b.clock,0x4d8)||Read<int32_t>(b.clock,0x4d0)||Read<int32_t>(b.clock,0x96c)
       ||!Read<void*>(b.clock,0x4b8)||Read<int32_t>(b.clock,0x4cc)!=32)
        return Fail(3868,"npc_common_time_requires_fresh_unpublished_original_clock");
    if(!UnpublishedNativeEvents(b))
        return Fail(3869,"npc_common_time_origin_requires_no_existing_force_or_equipment_timestamp_events");
    b.origin=state.source.index;
    if(b.origin>=MaxExactIndex||!InitializeOriginalMap(b))return Fail(3870,"npc_common_time_original_seed_pop_roundtrip_failed");
    b.counted=true;++state.bound;++state.binds;state.stage="npc_common_time_fresh_original_clock_mapped";return true;
}
inline bool CapturePhysical(void* pawn,void* movement,uint32_t raw){
    auto b=Owned(pawn,movement);if(!b)return true;
    if(!Observe()||!raw||!ContextProof()||!ClockProof(*b)||Read<uint32_t>(b->clock,size_t(b->baseOffset)+0x18)!=raw
       ||(b->lastRaw&&raw<=b->lastRaw))return Fail(3871,"npc_common_time_real_capture_identity_or_raw_mismatch");
    if(!b->firstCaptureRaw)b->firstCaptureRaw=raw;++b->captures;return true;
}
inline bool MapCopiedMove(void* pawn,void* movement,const void* original,void* copy,bool& carry,uint32_t& wire){
    carry=false;auto b=Owned(pawn,movement);if(!b)return false;
    const uint32_t raw=Read<uint32_t>(original,0x1c);
    if(!ContextProof()||!ClockProof(*b)||!raw||Read<void*>(copy)!=reinterpret_cast<void*>(engine::image+0xb600b78)
       ||Read<uint32_t>(copy,0x1c)!=raw||Read<uint32_t>(b->clock,size_t(b->baseOffset)+0x18)!=raw
       ||uint64_t(raw)+b->origin>MaxExactIndex)return Fail(3872,"npc_common_time_private_copy_origin_or_class_invalid");
    engine::function<void(*)(void*,void*)>(0x789edf0)(b->clock,copy);
    wire=Read<uint32_t>(copy,0x1c);if(wire!=raw+b->origin||wire>state.source.index)
        return Fail(3873,"npc_common_time_copy_public_timestamp_exceeds_actual_source");
    carry=b->seedTeleport&&!b->carryReserved&&!b->carrySubmitted;
    if(carry&&!Put(copy,0x468,uint8_t(1)))return Fail(3874,"npc_common_time_genuine_seed_marker_copy_failed");
    ++state.deepCopies;return true;
}
inline bool CommitConverted(void* pawn,void* movement,uint32_t raw,uint32_t wire,bool carry){
    auto b=Owned(pawn,movement);if(!b)return true;
    uint32_t expected=0;if(!PublicIndex(*b,raw,expected)||wire!=expected||!raw||wire<=b->lastWire||raw<=b->lastRaw)
        return Fail(3875,"npc_common_time_converted_record_receipt_not_monotonic");
    if(carry&&(b->carryReserved||b->carrySubmitted||!b->seedTeleport))return Fail(3876,"npc_common_time_seed_marker_duplicate_reservation");
    b->lastRaw=raw;b->lastWire=wire;++b->conversions;
    if(!b->firstConvertedRaw){b->firstConvertedRaw=raw;b->firstConvertedWire=wire;}
    if(carry){b->carryReserved=true;b->carryWire=wire;}
    return true;
}
inline bool Submitted(void* pawn,void* movement,uint32_t firstWire,uint32_t lastWire,bool containsCarry){
    auto b=Owned(pawn,movement);if(!b)return true;
    if(!ContextProof()||!ClockProof(*b)||!firstWire||lastWire<firstWire||lastWire>b->lastWire||firstWire<=b->lastSentWire)
        return Fail(3877,"npc_common_time_original_rpc_submission_receipt_invalid");
    if(containsCarry&&(!b->carryReserved||b->carrySubmitted||b->carryWire<firstWire||b->carryWire>lastWire))
        return Fail(3878,"npc_common_time_submitted_seed_marker_receipt_invalid");
    if(!b->firstSentWire)b->firstSentWire=firstWire;b->lastSentWire=lastWire;++b->submissions;
    if(containsCarry){b->carrySubmitted=true;b->seedTeleport=false;}
    return true;
}
inline bool Retire(size_t index,void* pawn,uint32_t epoch){
    if(state.seedScopes)return Fail(3879,"npc_common_time_retire_during_native_seed_scope");
    if(index==0)return true;if(index>=bindings.size())return false;auto& b=bindings[index];
    if(!b.used)return retiredEpochs[index]==epoch;
    // Source reset/closed policy must not prevent legitimate owned-life cleanup.
    if(b.pawn!=pawn||b.epoch!=epoch)return Fail(3879,"npc_common_time_retire_actual_life_mismatch");
    // A committed early phase has the same real actor/clock proof, without
    // claiming that Combat/Crouch/stream Register has already completed.
    if(b.counted&&(!PrePredictionActorProof(b)||!ClockProof(b)))return Fail(3879,"npc_common_time_retire_prepared_actor_or_clock_identity_mismatch");
    const bool counted=b.counted;b={};retiredEpochs[index]=epoch;if(counted&&state.bound)--state.bound;++state.retirements;return true;
}
template<class Json> inline void AddJson(Json& out){
    out.Add("{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"source_epoch\":%u,\"native_common_index\":%u,\"native_common_step\":%u,\"bound\":%u,\"native_source_writes\":0,\"client_clock_writes\":0,\"binds\":%llu,\"seed_calls\":%llu,\"pops\":%llu,\"deep_copies\":%llu,\"entries\":[",
        state.stage,state.error,state.initialized?"true":"false",state.sourceEpoch,state.source.index,state.source.step,state.bound,
        static_cast<unsigned long long>(state.binds),static_cast<unsigned long long>(state.seeds),static_cast<unsigned long long>(state.pops),static_cast<unsigned long long>(state.deepCopies));
    bool comma=false;for(size_t i=1;i<bindings.size();++i){const auto& b=bindings[i];if(!b.used)continue;
        out.Add("%s{\"index\":%zu,\"epoch\":%u,\"origin\":%u,\"offset\":%d,\"last_raw\":%u,\"last_wire\":%u,\"first_capture_raw\":%u,\"first_converted_raw\":%u,\"first_converted_wire\":%u,\"first_sent_wire\":%u,\"seed_marker_pending\":%s,\"carry_reserved\":%s,\"carry_submitted\":%s}",
            comma?",":"",i,b.epoch,b.origin,b.expectedOffset,b.lastRaw,b.lastWire,b.firstCaptureRaw,b.firstConvertedRaw,b.firstConvertedWire,b.firstSentWire,
            b.seedTeleport?"true":"false",b.carryReserved?"true":"false",b.carrySubmitted?"true":"false");comma=true;
    }out.Add("]}");
}
} // namespace local_server_npc_time_domain
