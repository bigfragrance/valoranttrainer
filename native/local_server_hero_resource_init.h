#pragma once
#include "local_server_hero_abilities.h"
#include "local_server_player_state_authority.h"
#include <array>
#include <cstdio>

// Missing typed HOST initialization only. Original resource classes, cost
// getters, Replenish, ownership/delegate callbacks and native publication own
// their respective operations. No UFunction replacement, Blueprint parser,
// map listener sweep, client factory, client write or repeated practice refill.
// Source: work/local-server-playable/hero-abilities/group3-resource-audit/ and
// authority-init/ult-review/. Signature progress is deliberately separate from
// its initial ordinary charges; its stripped later producer is NOT supplied.
namespace local_server_hero_resource_init {
namespace framework=local_server_hero_abilities;
namespace resource=local_server_resource_authority;
namespace ps=local_server_player_state_authority;
using Source=framework::SourceView;
using Weak=original_inventory_slots::WeakIdentity;
inline constexpr uintptr_t UltPointsClass=0x68d2fe0;
inline constexpr size_t MaxRecords=framework::MaxPlayers*framework::MaxItems;
enum class Kind:uint8_t {EquipmentCharges,SignatureCharges,UltimatePoints,TuningOnly};
struct Hooks {
    // Required for HOST: Root composes the actual session/practice policy and
    // current/retiring-epoch admission fence. Never called by a Role2 client.
    bool (*validate)(const Source&)=nullptr;
};
struct Record {
    bool used=false,retired=false,host=false,inFlight=false,metadataVerified=false;
    bool originalOwnerReady=false,pawnOwnerAttempted=false,psOwnerAttempted=false;
    bool initialAttempted=false,initialReady=false,initialObserved=false,ready=false;
    bool psRegistered=false,tuningPrepared=false,signature=false,ultimateCostKnown=false;
    void *component=nullptr,*slot=nullptr,*tuning=nullptr;Weak componentId{},slotId{},tuningId{};
    Source source{};Kind kind=Kind::EquipmentCharges;uint8_t slotType=255;
    uint64_t registration=0,samples=0,nativeReplenishes=0,ownerCallbacks=0;
    int32_t initialBefore=0,initialAfter=0,initialMaximum=0,nativeResult=0;
    int32_t current=0,authority=0,maximum=0,required=0,points=0;
    int32_t signaturePerUse=0,signatureProgress=0,maximumConsumable=0;
    bool canConsume=false,pointRecordWritten=false,pointNotified=false,pointNativePublished=false,pointShadowMatches=false;
    resource::ComponentRoute route{};
    // UltPoints has no scalar amount producer. Only its actual Tuning map/NET
    // array may reuse this source-bound existing helper; PublishResource is
    // never called for this local proof record.
    resource::Record tuningProof{};
    const char* stage="hero_typed_resource_not_initialized";
};
struct State {
    bool initialized=false,host=false,closed=false,restartRequired=false,profilesVerified=false,schemaVerified=false;
    void* world=nullptr;Weak worldId{};DWORD gameThread=0;unsigned error=0;
    uint32_t active=0,records=0,readyRecords=0;
    uint64_t registrations=0,polls=0,samples=0,retired=0,closes=0;
    uint64_t nativeReplenishes=0,ownerCallbacks=0,ultimateInitializations=0,chargeInitializations=0,sourceDeferrals=0;
    uintptr_t exceptionRva=0;const char* stage="hero_typed_resources_not_initialized";
};
struct Diagnostic {
    bool used=false,retired=false,host=false,ready=false,initialAttempted=false,initialReady=false,initialObserved=false;
    bool metadataVerified=false,ownerReady=false,signature=false,tuningPrepared=false,ultimateCostKnown=false;
    bool pointRecordWritten=false,pointNotified=false,pointNativePublished=false,pointShadowMatches=false;
    void *pawn=nullptr,*item=nullptr,*component=nullptr;uint32_t epoch=0;uint8_t slotType=255;Kind kind=Kind::EquipmentCharges;
    uint64_t registration=0,samples=0,nativeReplenishes=0,ownerCallbacks=0;
    int32_t before=0,after=0,initialMaximum=0,nativeResult=0,current=0,authority=0,maximum=0,required=0,points=0;
    int32_t signaturePerUse=0,signatureProgress=0,maximumConsumable=0;bool canConsume=false;
    const char* stage="hero_typed_resource_not_initialized";
};
struct Diagnostics {State summary{};std::array<Diagnostic,MaxRecords> records{};uint64_t at=0;};
struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr Profile Profiles[]{
    {0x6f7d130,2303,0x72d121dccd7bff55ULL},{0x6f7da30,2263,0xea9c8e5c56243a3fULL},
    {0x6dc73c0,238,0xb795e1fc2a795ca8ULL},{0x7195ec0,128,0x52eeaf8cddca7aacULL},
    {0x6cc61e0,43,0x0aeba717cbc6cadcULL},{0x6f7e3e0,71,0x5636022b717c0729ULL},
    {0x6f7e430,70,0xe366246d57c069a8ULL},{0x653b9b0,54,0x1d78983e12a69fc8ULL},
    {0x6dd3330,88,0x2c59c2cd2fc037fbULL},
    {0x6f2bf30,764,0xc8ef1093793c4943ULL},{0x6f2b7d0,8,0x98b17e07e43a87d9ULL},
    {0x6f2b960,359,0x839afd93f3073dc3ULL},{0x6f617e0,262,0xddaafee5be1bb2b8ULL},
    {0x6f619c0,367,0xa91dadeec9752c52ULL},{0x6f2bd60,322,0x43fcb3d0c574fd46ULL},
    {0x637fea0,31,0x39e4fedf016a5ebeULL},{0x64d6850,31,0x86a3993d13dab51eULL},
    {0x631dc80,31,0x63e778e8e540991aULL},{0x6b95970,258,0xb03cb98ad507ee92ULL}
};
inline State state{};inline Hooks hooks{};inline std::array<Record,MaxRecords> records{};
inline Diagnostics cached{};inline SRWLOCK cacheLock=SRWLOCK_INIT;
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool Capture(void* p,Weak& id){return original_inventory_slots::CaptureIdentity(p,id)&&Live(p,id);}
inline bool Thread(){return state.initialized&&original_players::Thread()&&GetCurrentThreadId()==state.gameThread;}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;return false;}
inline const State& GetState(){return state;}
inline void Cache(){
    state.records=state.readyRecords=0;for(const auto& r:records)if(r.used&&!r.retired){++state.records;if(r.ready)++state.readyRecords;}
    AcquireSRWLockExclusive(&cacheLock);cached.summary=state;cached.at=GetTickCount64();
    for(size_t i=0;i<records.size();++i){const auto& r=records[i];auto& d=cached.records[i];d={};
        d.used=r.used;d.retired=r.retired;d.host=r.host;d.ready=r.ready;d.initialAttempted=r.initialAttempted;
        d.initialReady=r.initialReady;d.initialObserved=r.initialObserved;d.metadataVerified=r.metadataVerified;
        d.ownerReady=r.originalOwnerReady;d.signature=r.signature;d.tuningPrepared=r.tuningPrepared;d.ultimateCostKnown=r.ultimateCostKnown;
        d.pointRecordWritten=r.pointRecordWritten;d.pointNotified=r.pointNotified;d.pointNativePublished=r.pointNativePublished;d.pointShadowMatches=r.pointShadowMatches;
        d.pawn=r.source.pawn;d.item=r.source.ability;d.component=r.component;d.epoch=r.source.sourceEpoch;d.slotType=r.slotType;d.kind=r.kind;
        d.registration=r.registration;d.samples=r.samples;d.nativeReplenishes=r.nativeReplenishes;d.ownerCallbacks=r.ownerCallbacks;
        d.before=r.initialBefore;d.after=r.initialAfter;d.initialMaximum=r.initialMaximum;d.nativeResult=r.nativeResult;
        d.current=r.current;d.authority=r.authority;d.maximum=r.maximum;d.required=r.required;d.points=r.points;
        d.signaturePerUse=r.signaturePerUse;d.signatureProgress=r.signatureProgress;d.maximumConsumable=r.maximumConsumable;d.canConsume=r.canConsume;d.stage=r.stage;}
    ReleaseSRWLockExclusive(&cacheLock);
}
inline Diagnostics Snapshot(){AcquireSRWLockShared(&cacheLock);const auto d=cached;ReleaseSRWLockShared(&cacheLock);return d;}
inline bool Method(const wchar_t* name,uint16_t bytes,uintptr_t exec,uintptr_t owner){
    auto fn=engine::Find(name);return fn&&Read<uint16_t>(fn,0xbe)==bytes
        &&Read<void*>(fn,0xe0)==reinterpret_cast<void*>(engine::image+exec)&&Read<void*>(fn,0x28)==engine::Class(owner);
}
inline bool Verify(){
    if(state.profilesVerified&&state.schemaVerified)return true;
    for(const auto& p:Profiles){auto code=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(code,p.bytes))return Fail(7330,"hero_typed_resource_original_profile_unreadable");
        uint64_t hash=0xcbf29ce484222325ULL;for(size_t i=0;i<p.bytes;++i)hash=(hash^code[i])*0x100000001b3ULL;
        if(hash!=p.fnv)return Fail(7331,"hero_typed_resource_original_profile_mismatch");}
    if(!Method(L"/Script/ShooterGame.EquipmentChargeComponent.GetMaxCharges",4,0x653b9b0,resource::EquipmentChargeClass)
       ||!Method(L"/Script/ShooterGame.EquipmentChargeComponent.GetCurrentCharges",4,0x637fea0,resource::EquipmentChargeClass)
       ||!Method(L"/Script/ShooterGame.ResourceComponent.Replenish",8,0x6b95970,resource::ResourceClass)
       ||!Method(L"/Script/ShooterGame.UltPointsComponent.OnOwningStateChanged",8,0x6dc73c0,UltPointsClass)
       ||!Method(L"/Script/ShooterGame.ShooterPlayerState.GetNumPointsNeededForUltimate",4,0x6cc61e0,0x6cc4040)
       ||!Method(L"/Script/ShooterGame.SignatureAbilityResourceComponent.GetSignatureChargeAmount",4,0x64d6850,resource::SignatureClass)
       ||!Method(L"/Script/ShooterGame.SignatureAbilityResourceComponent.GetNumSignatureChargesPerUse",4,0x631dc80,resource::SignatureClass))
        return Fail(7332,"hero_typed_resource_original_method_metadata_mismatch");
    if(!framework::loadout::Field(engine::Class(UltPointsClass),L"OwningCharacter",0x148,8,1,0)
       ||!framework::loadout::Field(engine::Class(UltPointsClass),L"OwningPlayerState",0x150,8,1,0)
       ||!framework::loadout::Field(engine::Class(resource::EquipmentChargeClass),L"MaxCharges",0x188,4,1,0)
       ||!framework::loadout::Field(engine::Class(resource::SignatureClass),L"NumSignatureChargesPerUse",0x1a8,4,1,0)
       ||!framework::loadout::Field(engine::Class(resource::SignatureClass),L"AuthSignatureChargeAmount",0x1ac,4,1,0))
        return Fail(7333,"hero_typed_resource_original_field_metadata_mismatch");
    state.profilesVerified=state.schemaVerified=true;return true;
}
inline bool SameSource(const Source& a,const Source& b){
    if(a.contextObject!=b.contextObject||a.world!=b.world||a.controller!=b.controller||a.playerState!=b.playerState
       ||a.pawn!=b.pawn||a.inventory!=b.inventory||a.ability!=b.ability||a.heroIndex!=b.heroIndex
       ||a.sourceEpoch!=b.sourceEpoch||a.registration!=b.registration||a.pendingGrant!=b.pendingGrant)return false;
    for(size_t i=0;i<a.identities.size();++i)if(!original_inventory_slots::SameIdentity(a.identities[i],b.identities[i]))return false;
    return true;
}
inline bool ClientView(framework::Player& p,framework::Item& item,Source& out){
    out={};auto& c=p.context;auto& n=item.native;
    if(state.host||p.authority||!p.used||p.retired||p.error||!p.registration||!p.epoch||c.epoch!=p.epoch
       ||!c.sourceReady||!item.assembled||!n.published||n.slotType>=17
       ||!Live(n.actor,n.actorId)||!Live(n.slot,n.slotId)||!Live(c.world,c.worldId)||!Live(c.pawn,c.pawnId)
       ||!Live(c.controller,c.controllerId)||!Live(c.playerState,c.playerStateId)||!Live(c.inventory,c.inventoryId)
       ||!Live(p.heroClass,p.heroClassId)||!Live(p.heroDefaults,p.heroDefaultsId)
       ||!Live(n.type,n.typeId)||!original_players::Derived(Read<void*>(c.pawn,0x10),p.heroClass)
       ||!original_players::Derived(Read<void*>(n.actor,0x10),n.type)
       ||Read<void*>(c.controller,0x470)!=c.playerState||Read<void*>(c.controller,0x4b0)!=c.pawn
       ||Read<void*>(c.pawn,0x488)!=c.playerState||Read<void*>(c.pawn,0x4a0)!=c.controller
       ||Read<void*>(c.pawn,0xc08)!=c.inventory||Read<void*>(c.inventory,0xc8)!=c.pawn||Read<void*>(c.inventory,0x2f0)!=c.pawn
       ||Read<uint8_t>(c.pawn,0x224)!=2||Read<uint8_t>(c.controller,0x224)!=2
       ||original_players::activePawn!=c.pawn||original_players::activeController!=c.controller||!original_players::localPlayer
       ||Read<void*>(c.controller,0x510)!=original_players::localPlayer||Read<void*>(original_players::localPlayer,0x38)!=c.controller
       ||Read<void*>(c.inventory,0x1b8+size_t(n.slotType)*8)!=n.slot
       ||!local_server_item_ownership::ValidSlot(c.inventory,n.slot,n.slotType)||!framework::loadout::Contains(n)
       ||Read<uint8_t>(n.actor,0x224)!=2||Read<void*>(n.actor,0x210)!=c.pawn||Read<void*>(n.actor,0x4a8)!=c.pawn)return false;
    out.contextObject=n.actor;out.world=c.world;out.controller=c.controller;out.playerState=c.playerState;
    out.pawn=c.pawn;out.inventory=c.inventory;out.ability=n.actor;out.heroIndex=p.hero;out.sourceEpoch=p.epoch;out.registration=p.registration;
    out.identities={n.actorId,c.worldId,c.controllerId,c.playerStateId,c.pawnId,c.inventoryId,n.actorId};return true;
}
inline bool SourceValid(const Record& r){
    if(!Thread()||state.closed||state.restartRequired||state.error||r.retired||r.host!=state.host)return false;
    const std::array<void*,7> objects{r.source.contextObject,r.source.world,r.source.controller,r.source.playerState,r.source.pawn,r.source.inventory,r.source.ability};
    for(size_t i=0;i<objects.size();++i)if(!Live(objects[i],r.source.identities[i]))return false;
    if(r.source.world!=state.world||r.source.pendingGrant||!Live(state.world,state.worldId)
       ||!Live(r.component,r.componentId)||!Live(r.slot,r.slotId)||Read<void*>(r.component,0xc8)!=r.source.ability
       ||r.slotType>=17||Read<void*>(r.source.inventory,0x1b8+size_t(r.slotType)*8)!=r.slot
       ||!local_server_item_ownership::ValidSlot(r.source.inventory,r.slot,r.slotType)
       ||!local_server_item_ownership::Contains(r.slot,r.source.ability)
       ||(r.kind==Kind::TuningOnly
          ?(r.component!=r.tuning||!original_players::IsA(r.tuning,original_ability_tuning::ComponentClass)
             ||original_weapon::Component(r.source.ability,resource::ResourceClass)!=nullptr
             ||original_weapon::Component(r.source.ability,original_ability_tuning::ComponentClass)!=r.tuning)
          :original_weapon::Component(r.source.ability,resource::ResourceClass)!=r.component)
       ||(r.tuning&&(!Live(r.tuning,r.tuningId)||Read<void*>(r.tuning,0xc8)!=r.source.ability)))return false;
    if(r.host)return hooks.validate&&framework::ValidateSource(r.source)&&hooks.validate(r.source);
    for(auto& p:framework::players)if(p.used&&!p.retired&&p.epoch==r.source.sourceEpoch&&p.context.pawn==r.source.pawn)
        for(size_t i=0;i<p.count;++i)if(p.items[i].native.actor==r.source.ability){Source current{};
            return ClientView(p,p.items[i],current)&&SameSource(r.source,current);}
    return false;
}
inline Record* Find(const Source& s,void* component){
    for(auto& r:records)if(r.used&&!r.retired&&r.component==component&&SameSource(r.source,s))return &r;return nullptr;
}
inline Record* Register(framework::Player& p,framework::Item& item,const Source& source){
    const bool tuningOnly=!item.resource&&item.resources==0&&item.tunings==1&&item.tuning;
    auto component=tuningOnly?item.tuning:item.resource;const auto componentId=tuningOnly?item.tuningId:item.resourceId;
    if(auto r=Find(source,component))return r;
    Record* free=nullptr;for(auto& r:records){
        if(r.used&&!r.retired&&(r.component==component||SameSource(r.source,source)))
            return Fail(7334,"hero_typed_resource_component_contract_changed_without_epoch_retirement"),nullptr;
        if(!free&&(!r.used||(r.retired&&!r.inFlight)))free=&r;}
    if(!free||state.registrations==UINT64_MAX)return Fail(7335,"hero_typed_resource_bounded_record_capacity_exhausted"),nullptr;
    *free={};auto& r=*free;r.used=true;r.host=p.authority;r.source=source;r.component=component;r.componentId=componentId;
    r.slot=item.native.slot;r.slotId=item.native.slotId;r.slotType=item.native.slotType;r.tuning=item.tuning;r.tuningId=item.tuningId;
    r.signature=original_players::IsA(r.component,resource::SignatureClass);
    r.kind=tuningOnly?Kind::TuningOnly:(original_players::IsA(r.component,UltPointsClass)?Kind::UltimatePoints:(r.signature?Kind::SignatureCharges:Kind::EquipmentCharges));
    r.registration=++state.registrations;r.stage="hero_typed_resource_registered_initialization_pending";return &r;
}
inline bool ChargeMetadata(Record& r){
    r.metadataVerified=false;
    auto table=Read<void*>(r.component);
    if(Read<void*>(table,0x4f8)!=reinterpret_cast<void*>(engine::image+0x6f2b7d0)
       ||Read<void*>(table,0x528)!=reinterpret_cast<void*>(engine::image+0x6f2bf30)
       ||Read<void*>(table,0x500)!=reinterpret_cast<void*>(engine::image+0x6f2bd60)){
        r.stage="hero_typed_charge_override_requires_separate_original_contract";return true;}
    if((Read<uint8_t>(r.component,0xc2)&0x40)==0){r.stage="hero_typed_charge_waiting_original_component_begin_play";return true;}
    r.metadataVerified=true;return true;
}
inline bool ChargeObservation(Record& r){
    if(!SourceValid(r)){r.stage="hero_typed_charge_waiting_current_source_fence";return false;}
    if(!original_ability_tuning::Event(r.component,L"/Script/ShooterGame.EquipmentChargeComponent.GetCurrentCharges",4,&r.current)
       ||!original_ability_tuning::Event(r.component,L"/Script/ShooterGame.EquipmentChargeComponent.GetMaxCharges",4,&r.maximum)
       ||r.current!=Read<int32_t>(r.component,0x10c)||r.current<0||r.maximum<0||r.maximum>65535)
        return Fail(7337,"hero_typed_charge_original_getter_or_limits_invalid");
    r.authority=Read<int32_t>(r.component,0x110);
    if(r.signature){
        if(!original_ability_tuning::Event(r.component,L"/Script/ShooterGame.SignatureAbilityResourceComponent.GetSignatureChargeAmount",4,&r.signatureProgress)
           ||!original_ability_tuning::Event(r.component,L"/Script/ShooterGame.SignatureAbilityResourceComponent.GetNumSignatureChargesPerUse",4,&r.signaturePerUse)
           ||r.signatureProgress!=Read<int32_t>(r.component,0x1ac)||r.signaturePerUse!=Read<int32_t>(r.component,0x1a8)
           ||r.signatureProgress<0||r.signaturePerUse<0)return Fail(7338,"hero_signature_original_progress_getter_invalid");}
    return SourceValid(r);
}
inline bool Charges(Record& r,bool genericReady){
    r.ready=false;if(!ChargeMetadata(r)||!r.metadataVerified)return !state.error;
    if(!ChargeObservation(r))return !state.error;
    r.originalOwnerReady=true;
    if(!r.host){
        // First real replicated charge is a receive-side receipt. Subsequent
        // original depletion to zero is legal and never causes a local refill.
        if(genericReady&&r.authority==r.current&&(r.current>0||r.maximum==0))r.initialObserved=true;
        r.initialReady=r.initialObserved;r.ready=genericReady&&r.initialReady;
        r.stage=r.ready?"client_original_initial_charges_observed_no_local_writes":"client_waiting_first_original_charge_delivery";return true;
    }
    if(!genericReady){r.stage="hero_typed_charge_waiting_original_resource_and_tuning_publication";return true;}
    if(!r.initialAttempted){
        r.initialBefore=r.current;r.initialMaximum=r.maximum;r.initialAfter=r.current;
        // Mark BEFORE original callbacks: re-entry cannot replenish twice.
        r.initialAttempted=true;
        if(r.current<r.maximum){
            struct Replenish {int32_t units,result=0;} call{r.maximum-r.current};
            r.inFlight=true;++state.active;
            const bool invoked=original_ability_tuning::Event(r.component,L"/Script/ShooterGame.ResourceComponent.Replenish",8,&call);
            --state.active;r.inFlight=false;
            if(!invoked)return Fail(7339,"hero_typed_charge_original_replenish_dispatch_failed");
            ++r.nativeReplenishes;++state.nativeReplenishes;r.nativeResult=call.result;
            // Native6F617E0 returns its granted units, saved BEFORE the
            // synchronous ResourceChanged/Replenished callbacks. Those real
            // listeners may consume immediately. Its return is not the final
            // net difference, and a legal zero after callbacks is preserved.
            if(call.result<1||call.result>call.units)
                return Fail(7340,"hero_typed_charge_initial_native_replenish_result_not_observed");
        }
        r.initialReady=true;++state.chargeInitializations;
        if(!SourceValid(r)){r.stage="hero_typed_charge_source_fenced_after_original_callback";return true;}
        if(!ChargeObservation(r))return !state.error;r.initialAfter=r.current;
    }
    if(!SourceValid(r)){r.stage="hero_typed_charge_waiting_current_source_fence";return true;}
    // Native Replenish already emitted local original events/delegates. Publish
    // its genuine result through the existing exact component channel tracker;
    // do not replay its local OnRep or write an invented Signature progress.
    if(!resource::PollItem(r.source.ability))return Fail(resource::GetState().error,resource::GetState().stage);
    r.ready=r.initialReady&&resource::ItemReady(r.source.ability);
    r.current=Read<int32_t>(r.component,0x10c);r.authority=Read<int32_t>(r.component,0x110);
    r.stage=r.ready?"host_initial_native_charges_published_later_signature_progress_unverified":"host_initial_native_charges_waiting_original_publication";
    return true;
}
inline ps::Source PlayerStateSource(const Record& r){return {r.source.world,r.source.controller,r.source.playerState,r.source.pawn,r.source.sourceEpoch};}
inline bool UltimateMetadata(Record& r){
    r.metadataVerified=false;
    auto table=Read<void*>(r.component);
    if(Read<void*>(table,0x4d0)!=reinterpret_cast<void*>(engine::image+0x6f7e3e0)
       ||Read<void*>(table,0x4e0)!=reinterpret_cast<void*>(engine::image+0x6f7e430)){
        r.stage="hero_ultimate_override_requires_separate_original_contract";return true;}
    if((Read<uint8_t>(r.component,0xc2)&0x40)==0){r.stage="hero_ultimate_waiting_original_component_begin_play";return true;}
    r.metadataVerified=true;return true;
}
inline bool OriginalUltimateOwner(Record& r){
    r.originalOwnerReady=false;auto pawn=Read<void*>(r.component,0x148),playerState=Read<void*>(r.component,0x150);
    if((pawn&&pawn!=r.source.pawn)||(playerState&&playerState!=r.source.playerState)){
        r.stage="hero_ultimate_waiting_nonnull_original_owner_conflict";return true;}
    if(!r.host){r.originalOwnerReady=pawn==r.source.pawn&&playerState==r.source.playerState;return true;}
    if(!SourceValid(r))return true;
    if(!pawn&&!r.pawnOwnerAttempted){
        r.pawnOwnerAttempted=true;r.inFlight=true;++state.active;
        engine::function<void(*)(void*,void*)>(0x6f7d130)(r.component,r.source.pawn);
        --state.active;r.inFlight=false;++r.ownerCallbacks;++state.ownerCallbacks;
        if(!SourceValid(r)){r.stage="hero_ultimate_source_fenced_after_original_pawn_callback";return true;}
    }
    pawn=Read<void*>(r.component,0x148);playerState=Read<void*>(r.component,0x150);
    if(pawn==r.source.pawn&&!playerState&&!r.psOwnerAttempted){
        r.psOwnerAttempted=true;r.inFlight=true;++state.active;void* actual=r.source.playerState;
        const bool invoked=original_ability_tuning::Event(r.component,L"/Script/ShooterGame.UltPointsComponent.OnOwningStateChanged",8,&actual);
        --state.active;r.inFlight=false;++r.ownerCallbacks;++state.ownerCallbacks;
        if(!invoked)return Fail(7342,"hero_ultimate_original_player_state_callback_dispatch_failed");
        if(!SourceValid(r)){r.stage="hero_ultimate_source_fenced_after_original_state_callback";return true;}
    }
    r.originalOwnerReady=Read<void*>(r.component,0x148)==r.source.pawn&&Read<void*>(r.component,0x150)==r.source.playerState;
    if(!r.originalOwnerReady)r.stage="hero_ultimate_waiting_original_owner_callback_readback";return true;
}
inline bool UltimateTuning(Record& r){
    if(r.tuningPrepared)return true;
    if(!r.host)return true; // Native receiving OnRep owns the client map.
    if(!resource::VerifyProfiles())return Fail(resource::GetState().error,resource::GetState().stage);
    if(!SourceValid(r)){r.stage="hero_ultimate_tuning_waiting_current_source_fence";return true;}
    auto& proof=r.tuningProof;
    if(!proof.used){
        void* connection=nullptr;
        if(!local_server_item_ownership::ActualHuman(r.source.world,r.source.controller,r.source.playerState,r.source.pawn,r.source.inventory,connection))return true;
        proof.world=r.source.world;proof.controller=r.source.controller;proof.playerState=r.source.playerState;proof.pawn=r.source.pawn;
        proof.inventory=r.source.inventory;proof.item=r.source.ability;proof.slot=r.slot;proof.connection=connection;
        proof.resource=r.component;proof.tuning=r.tuning;proof.sourceEpoch=r.source.sourceEpoch;proof.slotType=r.slotType;
        const std::array<void*,10> objects{proof.world,proof.controller,proof.playerState,proof.pawn,proof.inventory,proof.item,proof.slot,proof.connection,proof.resource,proof.tuning};
        for(size_t i=0;i<objects.size();++i)if(objects[i]&&!Capture(objects[i],proof.identities[i]))return Fail(7343,"hero_ultimate_tuning_actual_source_identity_unavailable");
        auto settings=engine::DefaultObject(engine::Class(0x282ff30));
        if(!settings)return Fail(7344,"hero_ultimate_tuning_actual_push_settings_unavailable");
        proof.tuningPushBased=engine::function<bool(*)(void*,void*)>(0x2839df0)(settings,engine::Class(original_ability_tuning::ComponentClass));proof.used=true;
    }
    if(!resource::Context(proof)||!SourceValid(r))return true;
    if(!resource::EnsureRoute(r.source.ability,r.component,r.route)||!resource::PrepareTuning(proof))
        return Fail(resource::GetState().error,resource::GetState().stage);
    if(!SourceValid(r))return true;r.tuningPrepared=proof.tuningReady;return true;
}
inline bool TuningOnlyProofContext(const Record& r){
    if(r.kind!=Kind::TuningOnly||!SourceValid(r))return false;
    const auto& proof=r.tuningProof;void* actualConnection=nullptr;
    const std::array<void*,10> objects{proof.world,proof.controller,proof.playerState,proof.pawn,proof.inventory,
        proof.item,proof.slot,proof.connection,proof.resource,proof.tuning};
    for(size_t n=0;n<objects.size();++n)if(objects[n]&&!Live(objects[n],proof.identities[n]))return false;
    return proof.used&&!proof.resource&&proof.world==r.source.world&&proof.controller==r.source.controller
        &&proof.playerState==r.source.playerState&&proof.pawn==r.source.pawn&&proof.inventory==r.source.inventory
        &&proof.item==r.source.ability&&proof.slot==r.slot&&proof.slotType==r.slotType
        &&proof.sourceEpoch==r.source.sourceEpoch&&proof.tuning==r.tuning
        &&local_server_item_ownership::ActualHuman(proof.world,proof.controller,proof.playerState,proof.pawn,proof.inventory,actualConnection)
        &&actualConnection==proof.connection;
}
inline bool TuningOnly(Record& r){
    r.ready=false;r.originalOwnerReady=false;
    if(!SourceValid(r)){r.stage="hero_tuning_only_waiting_current_source_fence";return true;}
    r.originalOwnerReady=true;
    if(!resource::VerifyProfiles())return Fail(resource::GetState().error,resource::GetState().stage);
    if(!r.host){
        void* property=nullptr;std::array<resource::RuntimeEntry,resource::MaxRows> entries{};int32_t count=0;
        // Observed map/NET-array agreement is a bounded receiving-side record,
        // not delivery or skill execution proof. Empty/unobserved stays unknown.
        r.metadataVerified=resource::NetProperty(r.tuning,L"ReplicatedAbilityTunings",0x138,16,property);
        const bool observed=r.metadataVerified&&resource::ReadMap(r.tuning,entries,count)&&count>0&&resource::ArrayMatches(r.tuning,entries,count);
        if(observed)r.initialObserved=true;
        r.initialReady=r.initialObserved;r.tuningPrepared=observed;r.ready=r.initialReady&&r.tuningPrepared;
        r.stage=r.ready?"client_tuning_only_original_map_and_net_array_observed_execution_unverified":"client_tuning_only_waiting_nonempty_original_map_and_net_array";
        return true;
    }
    if(!r.tuningPrepared&&original_ability_tuning::MapCount(static_cast<uint8_t*>(r.tuning)+0xe8)==0){
        r.stage="hero_tuning_only_waiting_nonempty_original_source_map";return true;
    }
    auto& proof=r.tuningProof;
    if(!proof.used){
        void* connection=nullptr;
        if(!local_server_item_ownership::ActualHuman(r.source.world,r.source.controller,r.source.playerState,r.source.pawn,r.source.inventory,connection))return true;
        proof.world=r.source.world;proof.controller=r.source.controller;proof.playerState=r.source.playerState;proof.pawn=r.source.pawn;
        proof.inventory=r.source.inventory;proof.item=r.source.ability;proof.slot=r.slot;proof.connection=connection;
        proof.resource=nullptr;proof.tuning=r.tuning;proof.sourceEpoch=r.source.sourceEpoch;proof.slotType=r.slotType;
        const std::array<void*,10> objects{proof.world,proof.controller,proof.playerState,proof.pawn,proof.inventory,
            proof.item,proof.slot,proof.connection,proof.resource,proof.tuning};
        for(size_t n=0;n<objects.size();++n)if(objects[n]&&!Capture(objects[n],proof.identities[n]))
            return Fail(7360,"hero_tuning_only_actual_source_identity_unavailable");
        auto settings=engine::DefaultObject(engine::Class(0x282ff30));
        if(!settings)return Fail(7361,"hero_tuning_only_actual_push_settings_unavailable");
        proof.tuningPushBased=engine::function<bool(*)(void*,void*)>(0x2839df0)(settings,engine::Class(original_ability_tuning::ComponentClass));proof.used=true;
    }
    if(!TuningOnlyProofContext(r)){r.stage="hero_tuning_only_waiting_actual_connection_and_source_fence";return true;}
    if(!r.tuningPrepared){
        r.initialAttempted=true;r.inFlight=true;++state.active;
        const bool prepared=resource::PrepareTuning(proof);
        --state.active;r.inFlight=false;
        // Latch the actual original protocol result before callback-induced
        // admission closure, preventing a second OnRep on the same live epoch.
        r.tuningPrepared=proof.tuningReady;r.metadataVerified=proof.tuningMetadata;
        r.initialReady=r.tuningPrepared;
        if(!prepared)return Fail(resource::GetState().error,resource::GetState().stage);
    }
    r.ready=r.initialReady&&TuningOnlyProofContext(r);
    r.stage=r.ready?"host_tuning_only_original_net_array_and_repnotify_prepared_delivery_unverified":"host_tuning_only_waiting_current_source_fence";
    return true;
}
inline bool ObserveOriginalUltimateCost(Record& r,int32_t required){
    r.ultimateCostKnown=false;
    if(required<0||required>65535)return Fail(7345,"hero_ultimate_original_required_cost_getter_invalid");
    // Original7195EC0 returns zero when its actual character/data source is
    // unresolved. Original UltPoints.CanConsume1 itself rejects cost<=0.
    // Keep that unknown pending; never publish an invented zero-point grant.
    if(required==0){r.stage="hero_ultimate_waiting_positive_original_cost_source";return false;}
    if(r.initialAttempted&&required!=r.required)return Fail(7346,"hero_ultimate_original_cost_changed_within_registered_epoch");
    r.ultimateCostKnown=true;r.required=required;return true;
}
inline bool UltimateObservation(Record& r){
    r.ultimateCostKnown=false;int32_t required=-1;
    if(!SourceValid(r)){r.stage="hero_ultimate_waiting_current_source_fence";return false;}
    if(!original_ability_tuning::Event(r.source.playerState,L"/Script/ShooterGame.ShooterPlayerState.GetNumPointsNeededForUltimate",4,&required)
       ||required<0||required>65535)return Fail(7345,"hero_ultimate_original_required_cost_getter_invalid");
    if(!ObserveOriginalUltimateCost(r,required))return false;
    r.points=Read<int32_t>(r.source.playerState,0xbdc);
    r.canConsume=engine::function<bool(*)(void*,int32_t)>(0x6f7e3e0)(r.component,1);
    r.maximumConsumable=engine::function<int32_t(*)(void*)>(0x6f7e430)(r.component);
    if(r.points<0||r.maximumConsumable<0||r.maximumConsumable>1)return Fail(7347,"hero_ultimate_original_availability_readback_invalid");
    return SourceValid(r);
}
inline bool Ultimate(Record& r){
    r.ready=false;
    if(!UltimateMetadata(r)||!r.metadataVerified||!OriginalUltimateOwner(r)||!r.originalOwnerReady)return !state.error;
    if(!UltimateObservation(r))return !state.error;
    if(!r.host){
        if(r.points>=r.required&&r.canConsume&&r.maximumConsumable==1)r.initialObserved=true;
        r.initialReady=r.initialObserved;r.ready=r.initialObserved;
        r.stage=r.ready?"client_original_ultimate_point_delivery_observed_no_local_writes":"client_waiting_first_original_ultimate_point_delivery";return true;
    }
    if(!UltimateTuning(r)||!r.tuningPrepared)return !state.error;
    if(!r.psRegistered){
        if(!ps::Register(PlayerStateSource(r))){const auto& failure=ps::GetState();
            return Fail(failure.error?failure.error:7348,failure.error?failure.stage:"hero_ultimate_actual_player_state_registration_failed");}
        r.psRegistered=true;
    }
    if(!r.initialReady){
        r.initialAttempted=true;r.inFlight=true;++state.active;
        const auto publication=ps::SetPoints(PlayerStateSource(r),r.required);
        --state.active;r.inFlight=false;
        r.pointRecordWritten=r.pointRecordWritten||publication.recordWritten;r.pointNotified=r.pointNotified||publication.notified;
        r.pointNativePublished=r.pointNativePublished||publication.nativePublished;r.pointShadowMatches=publication.nativeShadowMatches;
        if(publication.status==ps::Status::Failed)return Fail(publication.error?publication.error:7349,
            ps::GetState().error?ps::GetState().stage:"hero_ultimate_player_state_point_publication_failed");
        // Remember an actual committed grant even if an original callback
        // closed the policy gate. Closing admission must not erase this receipt
        // and cause a second grant when the same still-live epoch is enabled.
        if(publication.Ready()){r.initialReady=true;++state.ultimateInitializations;}
        if(!SourceValid(r)){r.stage="hero_ultimate_source_fenced_after_point_publication";return true;}
        if(!publication.Ready()){
            if(publication.recordWritten||publication.notified||publication.nativePublished)
                return Fail(7355,"hero_ultimate_waiting_player_state_api_reported_partial_point_publication");
            r.stage="hero_ultimate_waiting_native_ps_publication_no_point_write_this_attempt";return true;}
        if(!UltimateObservation(r))return !state.error;
        // Ready is the PS publisher's actual grant receipt. Original callbacks
        // may already have consumed that grant; requiring availability again
        // here would reject a legal zero or invite a second initialization.
    }
    // This epoch was granted once through the real PS publisher. A later zero
    // is original gameplay depletion, not permission to grant on every poll.
    r.ready=r.initialReady&&r.originalOwnerReady&&r.tuningPrepared;
    r.stage=r.ready?"host_initial_ultimate_points_published_original_consumption_unverified":"host_ultimate_initialization_pending";return true;
}
inline framework::ResourceExtensionResult ExtensionImpl(framework::Player& player,framework::Item& item){
    framework::ResourceExtensionResult out{};
    if(player.hero==0)return out; // Existing Jett producer owns hero0.
    const bool tuningOnly=!item.resource&&item.resources==0&&item.tunings==1&&item.tuning;
    if(!tuningOnly&&!original_players::IsA(item.resource,UltPointsClass)&&!original_players::IsA(item.resource,resource::EquipmentChargeClass))return out;
    auto component=tuningOnly?item.tuning:item.resource;const auto componentId=tuningOnly?item.tuningId:item.resourceId;
    out.handled=out.supported=true;
    if(!Thread()||state.closed||state.restartRequired||state.error){out.error=state.error;out.stage="hero_typed_resource_extension_unavailable";return out;}
    if(player.authority!=state.host){out.error=7351;out.stage="hero_typed_resource_role_context_mismatch";return out;}
    Source source{};
    const bool admitted=state.host
        ?framework::ResolveSource(item.native.actor,source)&&!source.pendingGrant&&hooks.validate&&hooks.validate(source)
        :ClientView(player,item,source);
    if(!admitted){
        // Rejected admission is also a current diagnostic, not a reason to
        // leave an earlier ready receipt exposed. Preserve the one-time grant
        // receipt so reopening this same epoch cannot initialize it twice.
        for(auto& previous:records)if(previous.used&&!previous.retired&&previous.source.pawn==player.context.pawn
            &&previous.source.sourceEpoch==player.epoch&&previous.source.registration==player.registration
            &&previous.source.ability==item.native.actor&&previous.component==component
            &&original_inventory_slots::SameIdentity(previous.componentId,componentId)
            &&original_inventory_slots::SameIdentity(previous.source.identities[6],item.native.actorId)){
                previous.ready=false;previous.stage="hero_typed_resource_waiting_actual_current_source_fence";}
        ++state.sourceDeferrals;out.stage="hero_typed_resource_waiting_actual_current_source_fence";return out;}
    auto r=Register(player,item,source);if(!r){out.error=state.error;out.stage=state.stage;return out;}
    if(r->inFlight){out.stage="hero_typed_resource_original_callback_in_flight";return out;}
    if(!SourceValid(*r)){++state.sourceDeferrals;out.stage="hero_typed_resource_waiting_registered_source_validation";return out;}
    ++r->samples;++state.samples;
    const bool result=r->kind==Kind::TuningOnly?TuningOnly(*r):(r->kind==Kind::UltimatePoints?Ultimate(*r):Charges(*r,item.resourcesReady));
    out.ready=result&&r->ready;out.error=result?0:state.error;out.stage=r->stage;
    if(state.error){out.error=state.error;out.stage=state.stage;}return out;
}
inline LONG Exception(EXCEPTION_POINTERS* info){
    state.error=info&&info->ExceptionRecord?info->ExceptionRecord->ExceptionCode:0xe0007352U;
    const auto at=info&&info->ExceptionRecord?reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.restartRequired=true;
    state.stage="hero_typed_resource_native_exception_requires_new_owned_pair";return EXCEPTION_EXECUTE_HANDLER;
}
inline framework::ResourceExtensionResult Extension(framework::Player& player,framework::Item& item){
    __try{const auto result=ExtensionImpl(player,item);if(result.error)Cache();return result;}__except(Exception(GetExceptionInformation())){
        framework::ResourceExtensionResult out{};out.handled=out.supported=true;out.error=state.error;out.stage=state.stage;Cache();return out;}
}
inline bool InitImpl(void* world,bool host,const Hooks& supplied){
    if(state.initialized)return !state.error&&!state.closed&&!state.restartRequired&&state.world==world&&state.host==host&&hooks.validate==supplied.validate;
    const auto& f=framework::GetState();
    if(!world||!original_players::Thread()||!f.initialized||f.world!=world||f.host!=host||(host&&!supplied.validate))
        return Fail(7353,"hero_typed_resource_init_requires_framework_world_and_explicit_host_fence");
    if(host){const auto& publisher=ps::GetState();
        if(!publisher.initialized||publisher.closed||publisher.error||publisher.restartRequired||publisher.world!=world||!ps::hooks.validate)
            return Fail(publisher.error?publisher.error:7356,publisher.error?publisher.stage:"hero_typed_resource_init_requires_original_player_state_publisher_first");}
    Weak identity{};if(!Capture(world,identity)||!Verify())return state.error?false:Fail(7354,"hero_typed_resource_world_identity_unavailable");
    state.world=world;state.worldId=identity;state.host=host;state.gameThread=GetCurrentThreadId();hooks=supplied;state.initialized=true;
    if(!framework::BindResourceExtension(Extension)){state.initialized=false;return Fail(framework::GetState().error,framework::GetState().stage);}
    state.stage=host?"host_typed_resource_extension_bound_before_registration":"client_typed_resource_readonly_extension_bound";return true;
}
inline bool PollImpl(){
    if(!Thread()||state.error||state.restartRequired||state.closed)return false;state.records=state.readyRecords=0;
    for(auto& r:records)if(r.used&&!r.retired){
        if(!Live(r.component,r.componentId)||!Live(r.source.pawn,r.source.identities[4])){
            r.retired=true;r.ready=false;r.stage="hero_typed_resource_expired_weak_receipt_retired";++state.retired;continue;}
        if(!SourceValid(r)){r.ready=false;r.stage="hero_typed_resource_waiting_actual_current_source_fence";}
        ++state.records;if(r.ready)++state.readyRecords;
    }
    ++state.polls;state.stage="hero_typed_resource_receipts_sampled_execution_and_delivery_unverified";return true;
}
inline bool RetireImpl(void* pawn,uint32_t epoch){
    if(!Thread()||!pawn||!epoch)return false;
    // Coordinator admission is intentionally NOT queried here: it fenced this
    // exact epoch before requesting cache retirement. No UObject is touched.
    for(auto& r:records)if(r.used&&!r.retired&&r.source.pawn==pawn&&r.source.sourceEpoch==epoch){
        r.retired=true;r.ready=false;r.stage="hero_typed_resource_epoch_fenced_receipt_retired";++state.retired;}
    return true;
}
inline bool CloseImpl(){
    if(state.closed)return true;if(!Thread())return false;state.closed=true;
    for(auto& r:records)if(r.used&&!r.retired){r.retired=true;r.ready=false;r.stage="hero_typed_resource_closed_receipt_retired";++state.retired;}
    if(framework::resourceExtension==Extension)framework::resourceExtension=nullptr;
    ++state.closes;state.stage="hero_typed_resource_extension_closed_no_native_teardown";return true;
}
inline bool Init(void* world,bool host,const Hooks& supplied){__try{const bool ok=InitImpl(world,host,supplied);Cache();return ok;}__except(Exception(GetExceptionInformation())){Cache();return false;}}
inline bool Poll(){__try{const bool ok=PollImpl();Cache();return ok;}__except(Exception(GetExceptionInformation())){Cache();return false;}}
inline bool RetirePlayer(void* pawn,uint32_t epoch){__try{const bool ok=RetireImpl(pawn,epoch);Cache();return ok;}__except(Exception(GetExceptionInformation())){Cache();return false;}}
inline bool Close(){__try{const bool ok=CloseImpl();Cache();return ok;}__except(Exception(GetExceptionInformation())){Cache();return false;}}
inline void WriteTuningOnlyDiagnostic(FILE* file,const Diagnostic& r){
    auto b=[](bool v){return v?"true":"false";};
    std::fprintf(file,"{\"pawn\":\"%p\",\"item\":\"%p\",\"component\":\"%p\",\"epoch\":%u,\"slot\":%u,\"kind\":3,\"kind_name\":\"tuning_only\",\"retired\":%s,\"ready\":%s,\"metadata_verified\":%s,\"original_owner_ready\":%s,\"initial_attempted\":%s,\"initial_ready\":%s,\"initial_observed_on_receiver\":%s,\"tuning_prepared\":%s,\"native_resource_present\":false,\"current_native\":null,\"current_authority\":null,\"current_maximum\":null,\"current_ultimate_points\":null,\"required_ultimate_points\":null,\"signature_progress\":null,\"native_replenishes\":0,\"stage\":",
        r.pawn,r.item,r.component,r.epoch,unsigned(r.slotType),b(r.retired),b(r.ready),b(r.metadataVerified),b(r.ownerReady),
        b(r.initialAttempted),b(r.initialReady),b(r.initialObserved),b(r.tuningPrepared));
    original_players::Text(file,r.stage);std::fputc('}',file);
}
inline void Write(FILE* file){
    if(!file)return;const auto d=Snapshot();const auto& s=d.summary;auto b=[](bool v){return v?"true":"false";};
    std::fprintf(file,"{\"stage\":");original_players::Text(file,s.stage);
    std::fprintf(file,",\"error\":%u,\"initialized\":%s,\"host\":%s,\"closed\":%s,\"profiles_verified\":%s,\"schema_verified\":%s,\"restart_required\":%s,\"records\":%u,\"ready_records\":%u,\"registrations\":%llu,\"native_replenishes\":%llu,\"original_owner_callbacks\":%llu,\"ultimate_initializations\":%llu,\"charge_initializations\":%llu,\"source_deferrals\":%llu,\"retired\":%llu,\"polls\":%llu,\"signature_progress_authority_supported\":false,\"automatic_regeneration_supported\":false,\"ultimate_consumption_authority_supported\":false,\"delivery_verified\":false,\"hero_execution_verified\":false,\"client_writes\":0,\"client_factory\":false,\"cached_snapshot\":true,\"sampled_at_ms\":%llu,\"entries\":[",
        s.error,b(s.initialized),b(s.host),b(s.closed),b(s.profilesVerified),b(s.schemaVerified),b(s.restartRequired),s.records,s.readyRecords,
        static_cast<unsigned long long>(s.registrations),static_cast<unsigned long long>(s.nativeReplenishes),static_cast<unsigned long long>(s.ownerCallbacks),
        static_cast<unsigned long long>(s.ultimateInitializations),static_cast<unsigned long long>(s.chargeInitializations),static_cast<unsigned long long>(s.sourceDeferrals),
        static_cast<unsigned long long>(s.retired),static_cast<unsigned long long>(s.polls),static_cast<unsigned long long>(d.at));
    bool first=true;for(const auto& r:d.records)if(r.used){if(!first)std::fputc(',',file);first=false;
        if(r.kind==Kind::TuningOnly){WriteTuningOnlyDiagnostic(file,r);continue;}
        char cost[24]{};const bool costKnown=r.kind==Kind::UltimatePoints&&r.ultimateCostKnown;
        if(costKnown)std::snprintf(cost,sizeof(cost),"%d",r.required);
        else std::snprintf(cost,sizeof(cost),"null");
        std::fprintf(file,"{\"pawn\":\"%p\",\"item\":\"%p\",\"component\":\"%p\",\"epoch\":%u,\"slot\":%u,\"kind\":%u,\"retired\":%s,\"ready\":%s,\"metadata_verified\":%s,\"original_owner_ready\":%s,\"initial_attempted\":%s,\"initial_ready\":%s,\"initial_observed_on_receiver\":%s,\"initial_before\":%d,\"initial_after\":%d,\"initial_maximum\":%d,\"native_replenish_result\":%d,\"native_replenishes\":%llu,\"current_native\":%d,\"current_authority\":%d,\"current_maximum\":%d,\"required_ultimate_points\":%s,\"required_ultimate_points_known\":%s,\"current_ultimate_points\":%d,\"can_consume_one\":%s,\"max_consumable\":%d,\"signature\":%s,\"signature_per_use\":%d,\"signature_progress\":%d,\"signature_progress_authority_supported\":false,\"point_record_written\":%s,\"point_native_notified\":%s,\"point_native_published\":%s,\"point_native_shadow_matches\":%s,\"tuning_prepared\":%s,\"stage\":",
            r.pawn,r.item,r.component,r.epoch,unsigned(r.slotType),unsigned(r.kind),b(r.retired),b(r.ready),b(r.metadataVerified),b(r.ownerReady),
            b(r.initialAttempted),b(r.initialReady),b(r.initialObserved),r.before,r.after,r.initialMaximum,r.nativeResult,
            static_cast<unsigned long long>(r.nativeReplenishes),r.current,r.authority,r.maximum,cost,b(costKnown),r.points,b(r.canConsume),r.maximumConsumable,
            b(r.signature),r.signaturePerUse,r.signatureProgress,b(r.pointRecordWritten),b(r.pointNotified),b(r.pointNativePublished),b(r.pointShadowMatches),b(r.tuningPrepared));
        original_players::Text(file,r.stage);std::fputc('}',file);}
    std::fputs("]}",file);
}
}
