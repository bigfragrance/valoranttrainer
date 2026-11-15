#pragma once
#include "original_content_services.h"
#include "original_inventory_slots.h"
#include "original_jett_priming.h"
#include "training_world_geometry.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <type_traits>

// Sweep provides current-component capsule collision for movement, cover and
// route validation. Observe records the additional named-profile metadata.
// Both use the original query lifetime and source-identity checks.
namespace humanlike_capsule_profile_comparison {
struct Comparison {
    const char* stage="capsule_profile_not_observed";
    bool abiKnown=false,sourceKnown=false,profileKnown=false,objectTypeKnown=false;
    bool profileResolved=false,sweepAttempted=false,sweepKnown=false,sweepClear=false;
    bool componentResponsesKnown=false;
    uint32_t responseReadCount=0;
    std::array<uint8_t,32> responses{};
    original_content_services::Name profileName{};
    uint8_t objectType=255,resolvedObjectType=255;
    bool hitKnown=false,hitBlocking=false;
    float hitTime=0,hitDistance=0;
    engine::Vec hitNormal{};
    uintptr_t hitActor=0,hitActorClass=0,hitComponent=0,hitComponentClass=0;
    bool hitOwnerBlockingVolumeKnown=false,hitOwnerBlockingVolume=false;
};
static_assert(std::is_trivially_copyable_v<Comparison>);
static_assert(sizeof(original_content_services::Name)==12);

struct Parameter {const wchar_t* name;int offset;size_t bytes;bool boolean=false;};
inline bool Fields(void* function,const Parameter* parameters,size_t count){
    for(size_t index=0;index<count;++index){
        const auto& parameter=parameters[index];
        original_jett_priming::PropertyInfo info{};
        if(original_jett_priming::FieldOffset(function,parameter.name,parameter.bytes,&info)!=parameter.offset
           ||info.arrayDim!=1||info.elementSize!=static_cast<int32_t>(parameter.bytes))return false;
        if(parameter.boolean&&(info.boolFieldSize!=1||info.boolByteOffset!=0
           ||!info.boolByteMask||!info.boolFieldMask))return false;
    }
    return true;
}
inline bool Function(void* function,uint16_t bytes,uintptr_t exec){
    return function&&original_players::Value<uint16_t>(function,0xbe)==bytes
        &&original_players::Value<uintptr_t>(function,0xe0)==engine::image+exec;
}
inline bool HitLayout(){
    auto type=engine::Find(L"/Script/Engine.HitResult");
    if(!type||original_players::Value<int32_t>(type,0x60)!=240)return false;
    constexpr Parameter parameters[]{
        {L"Time",4,4},{L"Distance",8,4},{L"ImpactNormal",0x58,24},
        {L"bBlockingHit",0xad,1,true},{L"Component",0xd0,8}
    };
    if(!Fields(type,parameters,std::size(parameters)))return false;
    original_jett_priming::PropertyInfo blocking{};
    return original_jett_priming::FieldOffset(type,L"bBlockingHit",1,&blocking)==0xad
        &&blocking.boolByteMask==1&&blocking.boolFieldMask==1;
}
inline bool Body(uintptr_t rva,size_t count,uint64_t expected){
    const auto* bytes=reinterpret_cast<const uint8_t*>(engine::image+rva);
    if(!original_players::Readable(bytes,count))return false;
    uint64_t hash=14695981039346656037ULL;
    for(size_t index=0;index<count;++index)hash=(hash^bytes[index])*1099511628211ULL;
    return hash==expected;
}
inline bool Abi(){
    auto name=engine::Find(L"/Script/Engine.PrimitiveComponent.GetCollisionProfileName");
    auto object=engine::Find(L"/Script/Engine.PrimitiveComponent.GetCollisionObjectType");
    auto response=engine::Find(L"/Script/Engine.PrimitiveComponent.GetCollisionResponseToChannel");
    if(!Function(name,12,0x3e832e0)||!Function(object,1,0x3e805a0)
       ||!Function(response,2,0x3e805e0)
       ||!Body(0x41153b0,889,0x48e64e4d2f3f853aULL)
       ||!Body(0x411f100,299,0x04044204a2a69b24ULL)
       ||!Body(0x4604560,418,0x2dd1b5c4f6484482ULL)
       ||!Body(0x3690190,195,0xede3f42112341e3cULL))return false;
    constexpr Parameter nameReturn[]{ {L"ReturnValue",0,12} };
    constexpr Parameter objectReturn[]{ {L"ReturnValue",0,1} };
    constexpr Parameter responseParameters[]{ {L"Channel",0,1},{L"ReturnValue",1,1} };
    return Fields(name,nameReturn,std::size(nameReturn))
        &&Fields(object,objectReturn,std::size(objectReturn))
        &&Fields(response,responseParameters,std::size(responseParameters));
}
struct Shape {int32_t kind=3;float radius=0,halfHeight=0,padding=0;};
static_assert(sizeof(Shape)==16);
struct QueryParameters {
    alignas(16) std::array<uint8_t,192> bytes{};
    bool constructed=false;
    ~QueryParameters(){
        if(constructed)engine::function<void(*)(void*)>(0x3690190)(bytes.data());
    }
};

struct Source {
    size_t index=0;
    void* world=nullptr;
    void* physicsScene=nullptr;
    void* pawn=nullptr;
    void* capsule=nullptr;
    original_inventory_slots::WeakIdentity worldIdentity{},pawnIdentity{},capsuleIdentity{};
};
inline bool Current(const Source& source,double radius,double half){
    if(!original_players::Thread()||!source.index||source.index>=original_players::MaxPlayers
       ||source.world!=training_world_geometry::world||source.world!=original_players::world
       ||!source.physicsScene||original_players::Value<void*>(source.world,0x260)!=source.physicsScene
       ||original_inventory_slots::ResolveIdentity(source.worldIdentity)!=source.world
       ||original_inventory_slots::ResolveIdentity(source.pawnIdentity)!=source.pawn
       ||original_inventory_slots::ResolveIdentity(source.capsuleIdentity)!=source.capsule)return false;
    const auto& player=original_players::players[source.index];
    return player.ready&&player.characterInitialized&&player.character==source.pawn
        &&original_players::Value<void*>(source.pawn,0x500)==source.capsule
        &&original_players::Value<void*>(source.capsule,0xc8)==source.pawn
        &&original_players::Value<void*>(player.movement,0xd8)==source.capsule
        &&std::abs(original_players::Value<float>(source.capsule,0x6dc)-radius)<.001
        &&std::abs(original_players::Value<float>(source.capsule,0x6d8)-half)<.001;
}
struct FixedAbi {
    uintptr_t image=0;
    bool known=false;
    void* blockingVolumeClass=nullptr;
};
inline FixedAbi fixedAbi{};
inline void ObserveHit(Comparison& result,const uint8_t* nativeHit,bool layoutKnown=false){
    if(!result.sweepKnown||(!layoutKnown&&!HitLayout()))return;
    result.hitBlocking=(nativeHit[0xad]&1)!=0;
    result.hitTime=original_players::Value<float>(nativeHit,4);
    result.hitDistance=original_players::Value<float>(nativeHit,8);
    result.hitNormal=original_players::Value<engine::Vec>(nativeHit,0x58);
    result.hitKnown=std::isfinite(result.hitTime)&&result.hitTime>=0&&result.hitTime<=1
        &&std::isfinite(result.hitDistance)&&result.hitDistance>=0
        &&original_players::Finite(result.hitNormal)&&result.hitBlocking!=result.sweepClear;
    if(!result.hitKnown||!result.hitBlocking)return;
    const auto componentIdentity=original_players::Value<original_inventory_slots::WeakIdentity>(nativeHit,0xd0);
    auto component=original_inventory_slots::ResolveIdentity(componentIdentity);
    if(!component)return;
    auto componentClass=original_players::Value<void*>(component,0x10);
    auto owner=original_players::Value<void*>(component,0xc8);
    if(original_inventory_slots::ResolveIdentity(componentIdentity)!=component)return;
    result.hitComponent=reinterpret_cast<uintptr_t>(component);
    result.hitComponentClass=reinterpret_cast<uintptr_t>(componentClass);
    original_inventory_slots::WeakIdentity ownerIdentity{};
    if(!original_inventory_slots::CaptureIdentity(owner,ownerIdentity)
       ||original_inventory_slots::ResolveIdentity(ownerIdentity)!=owner)return;
    auto ownerClass=original_players::Value<void*>(owner,0x10);
    if(original_inventory_slots::ResolveIdentity(componentIdentity)!=component
       ||original_inventory_slots::ResolveIdentity(ownerIdentity)!=owner
       ||original_players::Value<void*>(component,0xc8)!=owner)return;
    // Actor evidence is the original weak-resolved hit component's owner.
    // HitObjectHandle has a separate representation and remains uninterpreted.
    result.hitActor=reinterpret_cast<uintptr_t>(owner);
    result.hitActorClass=reinterpret_cast<uintptr_t>(ownerClass);
    auto blockingVolume=layoutKnown?fixedAbi.blockingVolumeClass:engine::Find(L"/Script/Engine.BlockingVolume");
    result.hitOwnerBlockingVolumeKnown=ownerClass&&blockingVolume;
    if(result.hitOwnerBlockingVolumeKnown)
        result.hitOwnerBlockingVolume=original_players::Derived(ownerClass,blockingVolume);
}
// Fixed original code and HitResult metadata are confirmed once per loaded
// engine image. Source identities and actual component settings stay per-query.
inline bool SweepAbi(){
    if(fixedAbi.image==engine::image)return fixedAbi.known;
    fixedAbi={};fixedAbi.image=engine::image;
    fixedAbi.known=engine::image&&Body(0x4965f90,8,0xd3e4a86e7fd5333fULL)
        &&Body(0x41ecc90,8,0xc3659ac4ad5dca2fULL)
        &&Body(0x411f100,299,0x04044204a2a69b24ULL)
        &&Body(0x4604560,418,0x2dd1b5c4f6484482ULL)
        &&Body(0x3690190,195,0xede3f42112341e3cULL)&&HitLayout();
    if(fixedAbi.known)fixedAbi.blockingVolumeClass=engine::Find(L"/Script/Engine.BlockingVolume");
    return fixedAbi.known;
}
inline bool ComponentSettings(const Source& source,Comparison& result){
    auto table=original_players::Value<void*>(source.capsule,0);
    if(original_players::Value<uintptr_t>(table,0x5a0)!=engine::image+0x4965f90
       ||original_players::Value<uintptr_t>(table,0x5c8)!=engine::image+0x41ecc90)return false;
    auto responses=engine::function<const uint8_t*(*)(const void*)>(0x4965f90)(source.capsule);
    result.objectType=engine::function<uint8_t(*)(const void*)>(0x41ecc90)(source.capsule);
    result.objectTypeKnown=result.objectType<32;
    if(!result.objectTypeKnown||responses!=static_cast<const uint8_t*>(source.capsule)+0x570
       ||!original_players::Read(responses,0,result.responses))return false;
    result.responseReadCount=static_cast<uint32_t>(result.responses.size());
    for(const auto response:result.responses)if(response>2)return false;
    result.componentResponsesKnown=true;
    return true;
}
inline Comparison RunSweep(const Source& source,const engine::Vec& from,
                           const engine::Vec& to,double radius,double half,
                           Comparison result,bool cachedHitLayout){
    auto start=from,end=to;start.z+=half+2;end.z+=half+2;
    if(start.x==end.x&&start.y==end.y&&start.z==end.z)end.z+=.1;
    std::array<void*,128> ignoredStorage{};
    const auto ignored=training_world_geometry::Ignore(ignoredStorage);
    original_content_services::Name traceTag{};
    QueryParameters query;
    using Configure=void*(*)(void*,const original_content_services::Name*,bool,
                            const training_world_geometry::Array*,bool,void*);
    auto constructed=engine::function<Configure>(0x4604560)
        (query.bytes.data(),&traceTag,false,&ignored,true,source.world);
    query.constructed=true;
    if(constructed!=query.bytes.data()||!Current(source,radius,half)){
        result.sourceKnown=Current(source,radius,half);
        result.componentResponsesKnown=result.sourceKnown;
        result.stage="component_query_parameters_unconfirmed";return result;
    }
    Shape shape{};shape.radius=static_cast<float>(radius);shape.halfHeight=static_cast<float>(half);
    alignas(16) const std::array<double,4> rotation{{0,0,0,1}};
    alignas(16) std::array<uint8_t,240> nativeHit{};
    result.sweepAttempted=true;
    using Sweep=bool(*)(void*,uint8_t*,const engine::Vec*,const engine::Vec*,const double*,
                       int32_t,const Shape*,const void*,const uint8_t*);
    const bool blocking=engine::function<Sweep>(0x411f100)
        (source.world,nativeHit.data(),&start,&end,rotation.data(),result.objectType,
         &shape,query.bytes.data(),result.responses.data());
    result.sourceKnown=Current(source,radius,half);
    result.sweepKnown=result.sourceKnown;
    result.sweepClear=result.sweepKnown&&!blocking;
    ObserveHit(result,nativeHit.data(),cachedHitLayout);
    if(!Current(source,radius,half)){
        result.sourceKnown=false;result.sweepKnown=false;result.sweepClear=false;result.hitKnown=false;
        result.componentResponsesKnown=false;
        result.hitActor=result.hitActorClass=result.hitComponent=result.hitComponentClass=0;
        result.hitOwnerBlockingVolumeKnown=result.hitOwnerBlockingVolume=false;
    }
    result.stage=result.sweepKnown?"component_response_simple_sweep_observed":"component_response_sweep_unconfirmed";
    return result;
}
inline Comparison Sweep(size_t index,void* pawn,const engine::Vec& from,
                        const engine::Vec& to,double radius,double half){
    Comparison result{};
    if(!engine::image||!original_players::Thread()||!index||index>=original_players::MaxPlayers
       ||!original_players::Finite(from)||!original_players::Finite(to)
       ||!std::isfinite(radius)||!std::isfinite(half)||radius<=0||half<radius){
        result.stage="component_sweep_invalid_context";return result;
    }
    Source source{};source.index=index;source.pawn=pawn;source.world=training_world_geometry::world;
    source.physicsScene=original_players::Value<void*>(source.world,0x260);
    if(!original_inventory_slots::CaptureIdentity(source.world,source.worldIdentity)
       ||!original_inventory_slots::CaptureIdentity(pawn,source.pawnIdentity)){
        result.stage="component_sweep_source_identity_unavailable";return result;
    }
    source.capsule=original_players::Value<void*>(pawn,0x500);
    if(!original_inventory_slots::CaptureIdentity(source.capsule,source.capsuleIdentity)
       ||!original_players::IsA(source.capsule,0x3685a30)||!Current(source,radius,half)){
        result.stage="component_sweep_source_mismatch";return result;
    }
    result.sourceKnown=true;
    result.abiKnown=SweepAbi();
    if(!result.abiKnown){result.stage="component_sweep_abi_unconfirmed";return result;}
    if(!ComponentSettings(source,result)||!Current(source,radius,half)){
        result.sourceKnown=Current(source,radius,half);result.componentResponsesKnown=false;
        result.stage="component_sweep_settings_unconfirmed";return result;
    }
    result=RunSweep(source,from,to,radius,half,result,true);
    Comparison current{};
    if(!Current(source,radius,half)||!ComponentSettings(source,current)
       ||current.objectType!=result.objectType||current.responses!=result.responses
       ||!Current(source,radius,half)){
        result.sourceKnown=false;result.componentResponsesKnown=false;
        result.sweepKnown=false;result.sweepClear=false;result.hitKnown=false;
        result.hitActor=result.hitActorClass=result.hitComponent=result.hitComponentClass=0;
        result.hitOwnerBlockingVolumeKnown=result.hitOwnerBlockingVolume=false;
        result.stage="component_sweep_source_or_settings_changed";
    }
    return result;
}
inline Comparison Observe(size_t index,void* pawn,const engine::Vec& from,
                          const engine::Vec& to,double radius,double half){
    Comparison result{};
    if(!engine::image||!original_players::Thread()||!index||index>=original_players::MaxPlayers
       ||!original_players::Finite(from)||!original_players::Finite(to)
       ||!std::isfinite(radius)||!std::isfinite(half)||radius<=0||half<radius){
        result.stage="capsule_profile_invalid_context";return result;
    }
    Source source{};source.index=index;source.pawn=pawn;source.world=training_world_geometry::world;
    source.physicsScene=original_players::Value<void*>(source.world,0x260);
    if(!original_inventory_slots::CaptureIdentity(source.world,source.worldIdentity)
       ||!original_inventory_slots::CaptureIdentity(pawn,source.pawnIdentity)){
        result.stage="capsule_profile_source_identity_unavailable";return result;
    }
    source.capsule=original_players::Value<void*>(pawn,0x500);
    if(!original_inventory_slots::CaptureIdentity(source.capsule,source.capsuleIdentity)
       ||!original_players::IsA(source.capsule,0x3685a30)||!Current(source,radius,half)){
        result.stage="capsule_profile_source_mismatch";return result;
    }
    result.sourceKnown=true;
    if(!Abi()){result.stage="capsule_profile_abi_unconfirmed";return result;}
    result.abiKnown=true;
    result.profileKnown=engine::Event(source.capsule,
        L"/Script/Engine.PrimitiveComponent.GetCollisionProfileName",&result.profileName);
    if(!result.profileKnown||!Current(source,radius,half)){
        result.sourceKnown=Current(source,radius,half);
        result.stage="capsule_profile_name_query_incomplete";return result;
    }
    result.objectTypeKnown=engine::Event(source.capsule,
        L"/Script/Engine.PrimitiveComponent.GetCollisionObjectType",&result.objectType);
    if(!result.objectTypeKnown||result.objectType>=32||!Current(source,radius,half)){
        result.objectTypeKnown=false;result.sourceKnown=Current(source,radius,half);
        result.stage="capsule_profile_object_type_unconfirmed";return result;
    }
    // Retain named-profile resolution as a separate observation. Custom
    // component responses are read independently through the original getter.
    int32_t channel=-1;std::array<uint8_t,32> registeredResponses{};
    result.profileResolved=engine::function<bool(*)(const original_content_services::Name*,int32_t*,uint8_t*)>(0x41153b0)
        (&result.profileName,&channel,registeredResponses.data());
    if(result.profileResolved&&channel>=0&&channel<32)result.resolvedObjectType=static_cast<uint8_t>(channel);
    else result.profileResolved=false;
    if(!Current(source,radius,half)){
        result.sourceKnown=false;result.stage="component_response_source_changed";return result;
    }
    for(uint8_t queryChannel=0;queryChannel<32;++queryChannel){
        std::array<uint8_t,2> query{{queryChannel,255}};
        ++result.responseReadCount;
        if(!engine::Event(source.capsule,L"/Script/Engine.PrimitiveComponent.GetCollisionResponseToChannel",query.data())
           ||query[1]>2){
            result.sourceKnown=Current(source,radius,half);
            result.stage="component_response_query_unconfirmed";return result;
        }
        result.responses[queryChannel]=query[1];
    }
    result.sourceKnown=Current(source,radius,half);
    result.componentResponsesKnown=result.sourceKnown;
    if(!result.componentResponsesKnown){result.stage="component_response_source_changed";return result;}
    return RunSweep(source,from,to,radius,half,result,false);
}
} // namespace humanlike_capsule_profile_comparison
