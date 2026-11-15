#pragma once
#include "original_ability_tuning.h"
#include "original_inventory_slots.h"
#include "original_jett_priming.h"
#include "local_server_item_ownership.h"
#include "local_server_npc_identity.h"
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>

// Missing HOST records only. The retained resource simulation changes +10c;
// its original NET/RepNotify authority record is +110. Publish that actual host
// result after original simulation/refill, never a client supplied amount.
// Tuning uses the retained, separately declared NET array of native runtime
// entries. Native FArrayProperty allocation/copy and the original OnRep own its
// lifecycle. No client import, custom serializer, state/role write or formula.
// Evidence: work/local-server-playable/abilities/case20-authority-publication.md.
namespace local_server_resource_authority {
using Weak=original_inventory_slots::WeakIdentity;
// Preserve existing human/ability capacity and allow two firearms per NPC.
inline constexpr size_t MaxItems=64+original_players::MaxPlayers*2,MaxRows=original_ability_tuning::MaxRows;
inline constexpr uintptr_t ResourceClass=0x637ed10,EquipmentChargeClass=0x675ea00;
inline constexpr uintptr_t SignatureClass=0x6cde0f0,AmmoClass=0x63c7e00,BlueprintResourceClass=0x65e3cd0;
struct Array {uint8_t* data=nullptr;int32_t count=0,capacity=0;};
struct alignas(8) RuntimeEntry {uint8_t tag[12]{},padding[4]{};original_ability_tuning::Value value{};};
static_assert(sizeof(Array)==16&&sizeof(RuntimeEntry)==40&&alignof(RuntimeEntry)==8&&offsetof(RuntimeEntry,value)==16);
struct ComponentRoute {
    void *component=nullptr,*owner=nullptr,*lifetime=nullptr;
    uintptr_t lifetimeRva=0;
    uint8_t flagsA0=0;bool replicates=false,usesRegisteredList=false,arraysValid=false;
    bool lifetimeVerified=false,registeredIdentityValid=false,conditionSupported=false,ready=false;
    int32_t replicatedCount=0,registeredCount=0,replicatedMatches=0,registeredMatches=0,condition=-1;
};
struct NativeScalarTracker {
    void *component=nullptr,*channel=nullptr,*replicator=nullptr,*layout=nullptr,*sendState=nullptr,*manager=nullptr;
    void *property=nullptr,*parent=nullptr,*command=nullptr,*shadow=nullptr;
    Weak componentId{},channelId{};
    uint32_t nativeFlags=0,layoutFlags=0,replicatorFlags=0,compareBefore=0,compareAfter=0;
    uint32_t historyStart=0,historyEndBefore=0,historyEndAfter=0,sendHistoryBefore=0,sendCompareBefore=0;
    uint16_t repIndex=2,commandStart=0,commandEnd=0,handle=0;
    int32_t shadowBefore=0,shadowAfter=0,result=-1,historyHandles=0;
    uint8_t connectionMode=255;
    uint32_t replicationFrame=0;
    bool prepared=false,deferred=false,published=false,handleObserved=false,terminatorObserved=false;
    const char* stage="native_scalar_tracker_not_requested";
};
struct AmmoObservation {
    void* component=nullptr;Weak identity{};ComponentRoute route{};
    int32_t authoredInitial=0,maximum=0,before=0,after=0,authorityBefore=0,authorityAfter=0,nativeReplenishResult=0;
    bool metadataVerified=false,nativeMethodsVerified=false,replenished=false,published=false;
    uint8_t flagsC2=0,inNativeBeginPlay=0;
    int32_t currentNative=0,currentMaximum=0,currentAuthorityBefore=0,currentAuthorityAfter=0;
    uint64_t ongoingSamples=0,nativeChanges=0,ongoingPublications=0;
    bool ongoingObserved=false,ongoingSynchronized=false;
    NativeScalarTracker tracker{};
};
// Additional, explicitly bounded host proof for authored child firearms. The
// normal primary/secondary API does not accept this route or weaken its guard.
// The configured class and real component reference must already be published
// in the original Backpack slot13; no pending/nonmember stand-in is accepted.
struct AuthoredSubequippableProof {
    void *component=nullptr,*ability=nullptr,*configuredClass=nullptr,*defaults=nullptr;
    std::array<Weak,4> identities{};
};
struct Record {
    bool used=false,ready=false,resourceReady=false,tuningReady=false,blueprintResource=false;
    bool resourceMetadata=false,tuningMetadata=false,nativeArrayAdded=false,nativeOnRep=false,mapPreserved=false;
    bool resourcePushBased=false,tuningPushBased=false,channelSnapshotValid=false,channelObserved=false;
    bool weaponAmmo=false,pendingGrant=false,ammoInitialAttempted=false,ammoInitialPrepared=false;
    bool ammoOngoingReady=false,currentResourcePushBased=false;
    bool scalarRetryWaiting=false;uint32_t scalarRetryFrame=0;
    void *world=nullptr,*controller=nullptr,*playerState=nullptr,*pawn=nullptr,*inventory=nullptr,*item=nullptr;
    void *slot=nullptr,*connection=nullptr,*resource=nullptr,*tuning=nullptr,*arrayProperty=nullptr,*innerProperty=nullptr;
    std::array<Weak,10> identities{};
    uint32_t sourceEpoch=0;uint64_t registration=0,polls=0,resourcePublications=0,arrayAdds=0;
    uint64_t ammoOngoingPolls=0,ammoOngoingDeferrals=0,ammoOngoingPolicySamples=0;
    int32_t nativeAmount=0,authorityBefore=0,authorityAfter=0,mapCount=0,arrayBefore=0,arrayAfter=0;
    uint8_t slotType=255;
    ComponentRoute resourceRoute{},tuningRoute{};
    AmmoObservation magazineAmmo{},reserveAmmo{};
    NativeScalarTracker resourceTracker{};
    size_t npcIndex=0; // Zero retains the existing admitted-human contract.
    uint8_t npcRemoteRole=255;
    bool authoredSubequippable=false;
    AuthoredSubequippableProof subequippableProof{};
    const char* stage="host_resource_item_not_prepared";
};
struct State {
    bool profilesVerified=false,ammoProfilesVerified=false,ammoOngoingProfilesVerified=false,scalarTrackerProfilesVerified=false,subequippableProfilesVerified=false,restartRequired=false;unsigned error=0;DWORD gameThread=0;
    uint64_t registrations=0,resourcePublications=0,nativeArrayAdds=0,retired=0,componentRegistrations=0;
    uint64_t ammoInitializations=0,ammoNativeReplenishes=0,ammoAuthorityPublications=0,ammoSkippedNonhuman=0,ammoSkippedNonballistic=0,ammoRetired=0;
    uint64_t ammoOngoingPolls=0,ammoOngoingDeferrals=0,ammoOngoingMagazinePublications=0,ammoOngoingReservePublications=0;
    uint64_t scalarTrackerPreparations=0,scalarTrackerCalls=0,scalarTrackerHistoryWitnesses=0,scalarTrackerFailures=0;
    uint64_t scalarTrackerDeferrals=0;
    uintptr_t exceptionRva=0;
    const char* stage="host_resource_authority_not_initialized";
};
struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr Profile Profiles[]{
    {0x4516300,497,0x36959e979faea5d0ULL},{0x6f11400,664,0x8d1b780a82108cbeULL},
    {0x4131070,258,0x35f44fe41db49e55ULL},{0x6f612e0,272,0x31837151013b592eULL},
    {0x6f2b3a0,1064,0x0a0d5c7067feacd3ULL},{0x6f6e870,489,0x8cc376b33c62bb03ULL},
    {0x6b95900,100,0x976ea463a882a8e7ULL},{0x6f619c0,367,0xa91dadeec9752c52ULL},
    {0x6f11040,272,0x5af1778133592241ULL},{0x1a7b720,196,0x4982523f72f33036ULL},
    {0x1c63870,269,0xde792136130ef719ULL},{0x6382bc0,47,0x07c9684295d78060ULL},
    {0x6ef0c50,489,0x4edac3c84500fe54ULL},{0x6ef0e80,129,0x9a241410bc7b5007ULL},
    {0x6ef0f60,24,0xc7b53c02abb7879eULL},{0x282ff30,167,0x0e6532940d7d2945ULL},
    {0x2839df0,173,0x99d93fcb48429ad2ULL}
};
inline constexpr Profile AmmoInitialProfiles[]{
    {0x6ef0e40,59,0x7b433d7abaf5edbfULL},{0x6b95970,258,0xb03cb98ad507ee92ULL},
    {0x6f617e0,262,0xddaafee5be1bb2b8ULL},{0x637fea0,31,0x39e4fedf016a5ebeULL},
    {0x63c84c0,31,0x63b8e5b702dfe552ULL},{0x37e4230,32,0x4e58980e57e48ac6ULL}
};
inline constexpr Profile AmmoOngoingProfiles[]{
    {0x6ef0ff0,220,0x2840c356801ad49cULL},{0x63c80a0,367,0x5d868c7e06d44dc9ULL},
    {0x45aef00,204,0x671df2d7325bff35ULL},{0x6f2bef0,7,0x9a4abacbff73e3b3ULL}
};
inline constexpr Profile ScalarTrackerProfiles[]{
    {0x42e59d0,269,0xbd6ca8afaf09231eULL},{0x42e9630,708,0xf90ebeac29d8dfadULL},
    {0x49caeb0,185,0xcf71c1fc22f16d28ULL},{0x49d63f0,464,0x2c8d7233fd4b3313ULL},
    {0x49d65c0,4056,0x064a88fe59dc7170ULL},{0x42ed380,1096,0xcf9b3a9f40d298d3ULL},
    {0x49cbe90,877,0xe60b4a938100e242ULL},{0x49cb180,374,0x18ab310b6b23642dULL},
    {0x49cb300,1772,0xa989d1a6de500590ULL},{0x49cb9f0,1179,0x96467aa66c5031a1ULL},
    {0x49cc2b0,3075,0x52ee77046fadf84bULL},{0x49d5d40,402,0x687104f03fce79ccULL},
    {0x49d9c70,680,0x86361c58daae3338ULL},{0x49d9a70,506,0x98ff3e4f3f57f856ULL},
    {0x49d98a0,452,0x82fb6a080e47ee14ULL}
};
inline State state{};
inline std::array<Record,MaxItems> records{};
inline const State& GetState(){return state;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Fail(unsigned code,const char* why){state.error=code;state.stage=why;return false;}
inline bool Thread(){return engine::image&&original_players::Thread()&&!state.restartRequired
    &&(!state.gameThread||state.gameThread==GetCurrentThreadId());}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool ValidArray(const Array& a,size_t stride,int32_t limit){
    return a.count>=0&&a.count<=limit&&a.capacity>=a.count&&a.capacity<=8192
        &&(!a.count||(a.data&&original_players::Readable(a.data,size_t(a.count)*stride)));
}
inline bool VerifyProfiles(){
    if(state.profilesVerified)return true;
    for(const auto& p:Profiles){auto code=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(code,p.bytes))return Fail(4520,"host_resource_pinned_profile_unreadable");
        uint64_t digest=14695981039346656037ULL;
        for(size_t i=0;i<p.bytes;++i)digest=(digest^code[i])*1099511628211ULL;
        if(digest!=p.fnv)return Fail(4521,"host_resource_pinned_profile_mismatch");}
    state.profilesVerified=true;state.gameThread=GetCurrentThreadId();return true;
}
inline bool VerifyAmmoProfiles(){
    if(state.ammoProfilesVerified)return true;
    for(const auto& p:AmmoInitialProfiles){auto code=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(code,p.bytes))return Fail(4620,"host_initial_ammo_native_profile_unreadable");
        uint64_t digest=14695981039346656037ULL;
        for(size_t i=0;i<p.bytes;++i)digest=(digest^code[i])*1099511628211ULL;
        if(digest!=p.fnv)return Fail(4621,"host_initial_ammo_native_profile_mismatch");}
    state.ammoProfilesVerified=true;return true;
}
inline bool VerifyAmmoOngoingProfiles(){
    if(state.ammoOngoingProfilesVerified)return true;
    for(const auto& p:AmmoOngoingProfiles){auto code=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(code,p.bytes))return Fail(4650,"host_ongoing_ammo_native_profile_unreadable");
        uint64_t digest=14695981039346656037ULL;
        for(size_t i=0;i<p.bytes;++i)digest=(digest^code[i])*1099511628211ULL;
        if(digest!=p.fnv)return Fail(4651,"host_ongoing_ammo_native_profile_mismatch");}
    state.ammoOngoingProfilesVerified=true;return true;
}
inline bool VerifyScalarTrackerProfiles(){
    if(state.scalarTrackerProfilesVerified)return true;
    for(const auto& p:ScalarTrackerProfiles){auto code=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(code,p.bytes))return Fail(4670,"host_native_scalar_tracker_profile_unreadable");
        uint64_t digest=14695981039346656037ULL;
        for(size_t i=0;i<p.bytes;++i)digest=(digest^code[i])*1099511628211ULL;
        if(digest!=p.fnv)return Fail(4671,"host_native_scalar_tracker_profile_mismatch");}
    state.scalarTrackerProfilesVerified=true;return true;
}
inline uintptr_t ExpectedLifetime(void* component){
    if(original_players::IsA(component,original_ability_tuning::ComponentClass))return 0x6f11040;
    if(original_players::IsA(component,SignatureClass))return 0x6f6e870;
    if(original_players::IsA(component,EquipmentChargeClass))return 0x6f2b3a0;
    if(original_players::IsA(component,AmmoClass))return 0x6ef0c50;
    if(original_players::IsA(component,ResourceClass))return 0x6f612e0;
    return 0;
}
// Read-only on either process. Readiness here describes a native component
// property route, not receipt of an ability or successful skill execution.
inline bool ObserveRoute(void* item,void* component,ComponentRoute& d){
    d={};d.component=component;d.owner=Read<void*>(component,0xc8);
    if(!item||!component||d.owner!=item)return false;
    auto table=Read<void*>(component);d.lifetime=Read<void*>(table,0x220);
    const auto address=reinterpret_cast<uintptr_t>(d.lifetime);d.lifetimeRva=address>=engine::image?address-engine::image:address;
    const auto expected=ExpectedLifetime(component);
    d.lifetimeVerified=expected&&d.lifetime==reinterpret_cast<void*>(engine::image+expected);
    d.flagsA0=Read<uint8_t>(component,0xa0);d.replicates=(d.flagsA0&0x20)!=0;
    d.usesRegisteredList=(Read<uint8_t>(item,0x11b)&1)!=0;
    const auto replicated=Read<Array>(item,0x330),registered=Read<Array>(item,0x320);
    d.replicatedCount=replicated.count;d.registeredCount=registered.count;
    d.arraysValid=ValidArray(replicated,8,512)&&ValidArray(registered,0x28,512);
    if(!d.arraysValid)return false;
    for(int32_t n=0;n<replicated.count;++n)if(Read<void*>(replicated.data,size_t(n)*8)==component)++d.replicatedMatches;
    for(int32_t n=0;n<registered.count;++n){auto row=registered.data+size_t(n)*0x28;
        if(Read<void*>(row)!=component)continue;
        ++d.registeredMatches;d.condition=Read<int32_t>(row,0x10);
        d.registeredIdentityValid=Live(component,Read<Weak>(row,8));
    }
    d.conditionSupported=d.condition==0||d.condition==2||d.condition==7||d.condition==13;
    const bool membership=d.usesRegisteredList
        ?d.registeredMatches==1&&d.registeredIdentityValid&&d.conditionSupported:d.replicatedMatches==1;
    d.ready=d.replicates&&d.lifetimeVerified&&membership
        &&Read<void*>(table,0x318)==reinterpret_cast<void*>(engine::image+0x4131210)
        &&Read<void*>(table,0x320)==reinterpret_cast<void*>(engine::image+0x2a64f70);
    return d.ready;
}
inline bool EnsureRoute(void* item,void* component,ComponentRoute& d){
    if(ObserveRoute(item,component,d))return true;
    // Retained EquipmentCharge, BlueprintResource, Ammo and AbilityTuning
    // constructors all request this native policy. Never alter A0 directly.
    if(!d.replicates){engine::function<void(*)(void*,uint8_t)>(0x4131070)(component,1);++state.componentRegistrations;}
    return ObserveRoute(item,component,d)||Fail(4522,"host_resource_original_component_replication_route_missing");
}
inline bool NetProperty(void* component,const wchar_t* name,int32_t offset,int32_t size,void*& field){
    original_jett_priming::PropertyInfo d{};
    if(original_jett_priming::FieldOffset(Read<void*>(component,0x10),name,size_t(size),&d)!=offset
       ||d.arrayDim!=1||d.elementSize!=size||(Read<uint64_t>(d.field,0x38)&0x100000020ULL)!=0x100000020ULL)return false;
    field=d.field;return true;
}
inline Record* FindCached(void* item){for(auto& r:records)if(r.used&&r.item==item)return &r;return nullptr;}
inline bool ObserveChannel(Record& r){
    r.channelSnapshotValid=false;r.channelObserved=false;
    const auto channels=Read<Array>(r.connection,0x78);
    if(!ValidArray(channels,8,512))return false;
    r.channelSnapshotValid=true;
    for(int32_t n=0;n<channels.count;++n){auto channel=Read<void*>(channels.data,size_t(n)*8);
        if(channel&&Read<void*>(channel,0x30)==r.connection&&Read<void*>(channel,0x78)==r.item
           &&original_players::IsA(channel,0x3dd3d40)){r.channelObserved=true;break;}}
    return true;
}
inline bool Context(const Record& r,bool deferWeaponMembership);
inline bool NetProperty(void* component,const wchar_t* name,int32_t offset,int32_t size,void*& field);
inline bool ScalarTrackerGate(NativeScalarTracker& w,const char* reason){
    w.stage=reason;++state.scalarTrackerFailures;return Fail(4553,reason);
}
inline uint32_t NativeReplicationFrame(const Record& r){return Read<uint32_t>(Read<void*>(r.connection,0x60),0x4d8);}
inline bool DeferScalarTracker(Record& r,NativeScalarTracker& w,const char* reason){
    w.deferred=true;w.stage=reason;w.replicationFrame=NativeReplicationFrame(r);
    r.scalarRetryWaiting=true;r.scalarRetryFrame=w.replicationFrame;r.stage=reason;state.stage=reason;++state.scalarTrackerDeferrals;
    return true; // A proven temporary native prerequisite, with no Auth write.
}
inline bool WaitingScalarRetryFrame(Record& r){
    if(!r.scalarRetryWaiting)return false;
    if(NativeReplicationFrame(r)==r.scalarRetryFrame){++state.scalarTrackerDeferrals;return true;}
    r.scalarRetryWaiting=false;return false;
}
inline bool PrepareScalarTracker(Record& r,void* component,NativeScalarTracker& w){
    w={};w.component=component;
    if(!Thread()||!Context(r,true))return ScalarTrackerGate(w,"host_native_scalar_tracker_actual_authority_context_changed");
    if(!VerifyScalarTrackerProfiles())return false;
    if(component==r.resource)w.componentId=r.identities[8];
    else if(component&&component==r.reserveAmmo.component)w.componentId=r.reserveAmmo.identity;
    else return ScalarTrackerGate(w,"host_native_scalar_tracker_requires_registered_resource_source");
    if(!Live(component,w.componentId)||!original_players::IsA(component,ResourceClass)||Read<void*>(component,0xc8)!=r.item)
        return ScalarTrackerGate(w,"host_native_scalar_tracker_resource_identity_or_owner_changed");
    const auto channels=Read<Array>(r.connection,0x78);
    if(!ValidArray(channels,8,512))return ScalarTrackerGate(w,"host_native_scalar_tracker_channel_list_invalid");
    unsigned matches=0;
    for(int32_t n=0;n<channels.count;++n){auto channel=Read<void*>(channels.data,size_t(n)*8);
        if(channel&&Read<void*>(channel,0x30)==r.connection&&Read<void*>(channel,0x78)==r.item
           &&original_players::IsA(channel,0x3dd3d40)){w.channel=channel;++matches;}}
    w.connectionMode=Read<uint8_t>(r.connection,0x1730);
    if(matches!=1||w.connectionMode!=0||(Read<uint8_t>(w.channel,0x38)&2)
       ||!original_inventory_slots::CaptureIdentity(w.channel,w.channelId))
        return ScalarTrackerGate(w,"host_native_scalar_tracker_requires_live_acknowledged_original_channel");
    if(!(Read<uint8_t>(w.channel,0xa0)&1))return DeferScalarTracker(r,w,"host_native_scalar_tracker_waiting_original_channel_ack_next_frame");
    // Native42E59D0 borrows an existing shared-value address from this exact
    // ActorChannel's object map. It does not create a replicator or NetGUID.
    auto shared=engine::function<void**(*)(void*,void*)>(0x42e59d0)(w.channel,component);
    w.replicator=Read<void*>(shared);
    if(!shared||!w.replicator)return DeferScalarTracker(r,w,"host_native_scalar_tracker_waiting_original_component_replicator_next_frame");
    if(!original_players::Readable(shared,16)||!original_players::Readable(w.replicator,0xe0))
        return ScalarTrackerGate(w,"host_native_scalar_tracker_existing_component_replicator_invalid");
    if(Read<void*>(w.replicator,0x48)!=component||Read<void*>(w.replicator,0x40)!=Read<void*>(component,0x10)
       ||Read<void*>(w.replicator,0x58)!=w.channel||Read<void*>(w.replicator,0x50)!=r.connection
       ||!Live(component,Read<Weak>(w.replicator,0xd8)))
        return ScalarTrackerGate(w,"host_native_scalar_tracker_replicator_object_class_channel_or_connection_mismatch");
    w.layout=Read<void*>(w.replicator,0x20);w.manager=Read<void*>(w.replicator,0x10);
    auto wrapper=Read<void*>(w.replicator,0x30);w.sendState=Read<void*>(wrapper,8);
    if(!w.layout||!w.manager||!w.sendState)return DeferScalarTracker(r,w,"host_native_scalar_tracker_waiting_original_sending_state_next_frame");
    if(!original_players::Readable(w.layout,0xf8)
       ||!original_players::Readable(w.manager,0x840)||!original_players::Readable(w.sendState,0x28)
       ||Read<void*>(w.manager,0x830)!=w.layout)
        return ScalarTrackerGate(w,"host_native_scalar_tracker_native_layout_manager_or_sending_state_missing");
    w.layoutFlags=Read<uint8_t>(w.layout,0x20);w.replicatorFlags=Read<uint32_t>(w.replicator,0xc);
    // These are read-only predicates from42E9630's actual eligibility setup and
    //49CBE90's admissibility. Unknown effective skip/layout modes stay gated.
    // The retained class descriptor's policy byte is not an effective dirty ID.
    if((w.layoutFlags&0x34U)||(w.replicatorFlags&0x20U))
        return ScalarTrackerGate(w,"host_native_scalar_tracker_effective_skip_or_layout_mode_unproven");
    w.nativeFlags=Read<uint32_t>(w.sendState,0x1c);
    // The complete last original DWORD is retained unchanged. Native mask0x2
    // denotes initial; mask0x80 selects a roles-only comparator. Neither is a
    // supported scalar continuation. No owner/initial flags are fabricated.
    if(w.nativeFlags&0x80U)return ScalarTrackerGate(w,"host_native_scalar_tracker_roles_only_native_flags_unproven");
    if(w.nativeFlags&2U)return DeferScalarTracker(r,w,"host_native_scalar_tracker_waiting_noninitial_native_flags_next_frame");
    if(!NetProperty(component,L"AuthResourceAmount",0x110,4,w.property))
        return ScalarTrackerGate(w,"host_native_scalar_tracker_original_net_amount_schema_changed");
    if(Read<uint16_t>(w.property,0x40)!=2)return ScalarTrackerGate(w,"host_native_scalar_tracker_original_auth_rep_index_changed");
    const auto parents=Read<Array>(w.layout,0x28),commands=Read<Array>(w.layout,0x38);
    if(!ValidArray(parents,0x38,512)||parents.count<=2||!ValidArray(commands,0x20,8192))
        return ScalarTrackerGate(w,"host_native_scalar_tracker_native_parent_or_command_arrays_invalid");
    // Resource.GetLifetime6F612E0 declares actualRepIndex2. Parent/command
    // pointers, offsets, condition and handle are validated against the actual
    // reflected field; no hand-built replication descriptor is admitted.
    w.parent=parents.data+2*0x38;w.commandStart=Read<uint16_t>(w.parent,0x20);w.commandEnd=Read<uint16_t>(w.parent,0x22);
    if(Read<void*>(w.parent)!=w.property||Read<int32_t>(w.parent,0x18)!=0x110
       ||Read<int32_t>(w.parent,0x24)!=0||!(Read<uint32_t>(w.parent,0x30)&1U)
       ||uint32_t(w.commandEnd)!=uint32_t(w.commandStart)+1U||int32_t(w.commandEnd)>commands.count)
        return ScalarTrackerGate(w,"host_native_scalar_tracker_auth_parent_is_not_one_native_unconditional_scalar");
    w.command=commands.data+size_t(w.commandStart)*0x20;w.handle=Read<uint16_t>(w.command,0x14);
    const auto shadowOffset=Read<int32_t>(w.command,0x10);
    if(Read<void*>(w.command)!=w.property||Read<int32_t>(w.command,0xc)!=0x110
       ||Read<uint16_t>(w.command,0x16)!=2||Read<uint8_t>(w.command,0x1c)!=5||!w.handle
       ||Read<int32_t>(w.parent,0x1c)!=shadowOffset)
        return ScalarTrackerGate(w,"host_native_scalar_tracker_auth_command_or_integer_type_changed");
    auto shadowData=Read<uint8_t*>(w.manager,0x820);const auto shadowNum=Read<int32_t>(w.manager,0x828),shadowCap=Read<int32_t>(w.manager,0x82c);
    if(!shadowData||shadowOffset<0||shadowNum<4||shadowNum>1048576||shadowCap<shadowNum
       ||uint64_t(shadowOffset)+4U>uint64_t(shadowNum)||!original_players::Readable(shadowData+shadowOffset,4))
        return ScalarTrackerGate(w,"host_native_scalar_tracker_native_shadow_allocation_invalid");
    w.shadow=shadowData+shadowOffset;w.shadowBefore=Read<int32_t>(w.shadow);
    if(w.shadowBefore!=Read<int32_t>(component,0x110))
        return DeferScalarTracker(r,w,"host_native_scalar_tracker_waiting_current_authority_shadow_next_frame");
    if(auto active=Read<void*>(w.sendState,0x20)){
        auto bits=Read<const uint32_t*>(active,0x18);
        if(!bits)bits=reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(active)+8);
        if(!original_players::Readable(bits,4))return ScalarTrackerGate(w,"host_native_scalar_tracker_native_active_tracker_invalid");
        if((Read<uint32_t>(bits)&4U)==0)return DeferScalarTracker(r,w,"host_native_scalar_tracker_waiting_original_active_parent_next_frame");
    }
    w.historyStart=Read<uint32_t>(w.manager,0x810);w.historyEndBefore=Read<uint32_t>(w.manager,0x814);
    w.compareBefore=Read<uint32_t>(w.manager,0x818);
    w.sendHistoryBefore=Read<uint32_t>(w.sendState,0x10);w.sendCompareBefore=Read<uint32_t>(w.sendState,0x14);
    if(w.historyStart>w.historyEndBefore||w.historyEndBefore>=0x7fffffffU||w.compareBefore>=0x7fffffffU
       ||w.historyEndBefore-w.historyStart>=64U||w.sendHistoryBefore<w.historyStart
       ||w.sendHistoryBefore>w.historyEndBefore||w.sendCompareBefore>w.compareBefore)
        return ScalarTrackerGate(w,"host_native_scalar_tracker_native_history_capacity_not_available");
    if(w.historyEndBefore-w.historyStart==63U&&w.sendHistoryBefore==w.historyStart)
        return DeferScalarTracker(r,w,"host_native_scalar_tracker_waiting_original_history_consumer_next_frame");
    w.replicationFrame=NativeReplicationFrame(r);w.prepared=true;
    w.stage="native_scalar_tracker_prepared_original_shadow_and_history";++state.scalarTrackerPreparations;return true;
}
inline bool PublishScalarTracker(Record& r,NativeScalarTracker& w,int32_t actualAuthority){
    if(w.deferred){state.restartRequired=true;return Fail(4674,"host_native_scalar_tracker_commit_attempted_before_native_retry_frame");}
    if(!w.prepared)return true; // Initial/no-channel path.
    if(!Thread()||!Context(r,true)||!Live(w.component,w.componentId)||!Live(w.channel,w.channelId)
       ||Read<void*>(w.replicator,0x48)!=w.component||Read<void*>(w.replicator,0x58)!=w.channel
       ||Read<void*>(w.replicator,0x50)!=r.connection||Read<void*>(w.replicator,0x20)!=w.layout
       ||Read<void*>(w.replicator,0x10)!=w.manager||Read<void*>(Read<void*>(w.replicator,0x30),8)!=w.sendState
       ||Read<void*>(w.manager,0x830)!=w.layout||Read<uint32_t>(w.sendState,0x1c)!=w.nativeFlags
       ||Read<uint8_t>(r.connection,0x1730)!=0||Read<uint32_t>(w.sendState,0x10)!=w.sendHistoryBefore
       ||Read<uint32_t>(w.sendState,0x14)!=w.sendCompareBefore
       ||Read<int32_t>(w.component,0x110)!=actualAuthority||Read<int32_t>(w.shadow)!=w.shadowBefore
       ||actualAuthority==w.shadowBefore||Read<uint32_t>(w.manager,0x814)!=w.historyEndBefore
       ||Read<uint32_t>(w.manager,0x818)!=w.compareBefore){
        state.restartRequired=true;++state.scalarTrackerFailures;w.stage="native_scalar_tracker_source_changed_after_authority_commit";
        return Fail(4672,w.stage);
    }
    ++state.scalarTrackerCalls;
    // Exact original42ED380 call ABI. False is the original sixth argument;
    // this build stores but does not consume it as a force flag. This invokes
    // the actual native comparator/history producer, not a synthetic serializer.
    w.result=engine::function<int32_t(*)(void*,void*,void*,const void*,const uint32_t*,bool)>(0x49cbe90)
        (w.layout,w.sendState,static_cast<uint8_t*>(w.manager)+8,w.component,&w.nativeFlags,false);
    w.shadowAfter=Read<int32_t>(w.shadow);w.compareAfter=Read<uint32_t>(w.manager,0x818);
    w.historyEndAfter=Read<uint32_t>(w.manager,0x814);
    auto changed=Read<Array>(static_cast<uint8_t*>(w.manager)+0x10+size_t(w.historyEndBefore&63U)*0x20);
    w.historyHandles=changed.count;
    if(ValidArray(changed,2,65536)&&changed.count>=2){
        w.terminatorObserved=Read<uint16_t>(changed.data,size_t(changed.count-1)*2)==0;
        for(int32_t n=0;n<changed.count-1;++n)if(Read<uint16_t>(changed.data,size_t(n)*2)==w.handle){w.handleObserved=true;break;}
    }
    if(w.result!=0||w.shadowAfter!=actualAuthority||w.compareAfter!=w.compareBefore+1U
       ||w.historyEndAfter!=w.historyEndBefore+1U||!w.handleObserved||!w.terminatorObserved){
        state.restartRequired=true;++state.scalarTrackerFailures;w.stage="native_scalar_tracker_target_shadow_or_history_witness_failed";
        return Fail(4673,w.stage);
    }
    w.published=true;w.stage="native_scalar_tracker_actual_handle_and_shadow_observed_delivery_unverified";
    ++state.scalarTrackerHistoryWitnesses;return true;
}
inline bool CanPublish(Record& r,bool pushBased,void* component=nullptr,NativeScalarTracker* tracker=nullptr){
    if(!ObserveChannel(r))return Fail(4552,"host_resource_native_actor_channel_snapshot_invalid");
    if(tracker)*tracker={};
    // Scalar updates on an existing channel always inspect the actual layout
    // and tracker. Descriptor policy=false cannot bypass effective-mode checks.
    if(r.channelObserved&&component&&tracker)return PrepareScalarTracker(r,component,*tracker);
    if(pushBased&&r.channelObserved){
        return Fail(4553,"host_resource_post_initial_push_dirty_publication_unproven");
    }
    return true;
}
inline bool SubEquippableContext(void* pawn,void* inventory,void* item,const AuthoredSubequippableProof& proof){
    const std::array<void*,4> objects{proof.component,proof.ability,proof.configuredClass,proof.defaults};
    for(size_t n=0;n<objects.size();++n)if(!Live(objects[n],proof.identities[n]))return false;
    return Read<void*>(proof.component,0x10)==engine::Class(0x63813f0)
        &&Read<void*>(proof.component,0xc8)==proof.ability
        &&Read<void*>(proof.component,0xe0)==proof.configuredClass&&Read<void*>(proof.component,0xf8)==item
        &&Read<void*>(Read<void*>(proof.component),0x220)==reinterpret_cast<void*>(engine::image+0x6eef150)
        &&Read<void*>(proof.ability,0x210)==pawn&&Read<void*>(proof.ability,0x4a8)==pawn
        &&Read<uint8_t>(proof.ability,0x224)==3&&Read<void*>(proof.configuredClass,0x120)==proof.defaults
        &&Read<uint8_t>(proof.defaults,0x4a0)==13&&Read<uint8_t>(item,0x4a0)==13
        &&original_players::Derived(Read<void*>(item,0x10),proof.configuredClass)
        &&local_server_item_ownership::Contains(Read<void*>(inventory,0x1b8+13*8),item);
}
// This route observes an NPC already admitted by the native match registry.
// Its one real client connection is only the replication audience. The NPC
// retains its original null Player, controller, actor ownership and roles.
inline bool NpcWeaponAmmoContext(void* world,void* pc,void* ps,void* pawn,void* inventory,
                                 uint32_t epoch,size_t index,void*& connection){
    connection=nullptr;
    const auto& registry=local_server_npc_identity::GetState();
    if(!index||index>=registry.entries.size()||!registry.contextBound||!registry.schemaVerified
       ||registry.error||registry.closed||registry.restartRequired||registry.world!=world
       ||!local_server_npc_identity::validator||!local_server_npc_identity::HostWorld())return false;
    const local_server_npc_identity::Tuple npc{index,pc,ps,pawn,epoch,false};
    const auto& entry=registry.entries[index];
    std::array<Weak,4> identities{};
    if(!entry.verified||!entry.marked||!entry.originalAfter
       ||!local_server_npc_identity::ReceiptSource(npc,entry,false)
       ||!local_server_npc_identity::Source(npc,identities,false)
       ||!(Read<uint8_t>(ps,local_server_npc_identity::BotOffset)&local_server_npc_identity::BotMask)
       ||Read<void*>(pc,0x510)||!original_players::IsA(inventory,0x6499330)
       ||Read<void*>(pawn,0xc08)!=inventory||Read<void*>(inventory,0xc8)!=pawn)return false;
    const auto& human=registry.entries[0];
    // PC, PlayerState and the admitted connection survive the human's death.
    // A replication audience has no dependency on its pawn or possession fence.
    if(!human.used||!human.human||!human.verified||human.originalAfter
       ||!Live(human.pc,human.pcId)||!Live(human.ps,human.psId)
       ||!Live(human.actualClass,human.classId)||Read<void*>(human.ps,0x10)!=human.actualClass
       ||!original_players::IsA(human.pc,original_players::ShooterControllerClass)
       ||!original_players::IsA(human.ps,original_players::ShooterStateClass)
       ||Read<uint8_t>(human.pc,0x224)!=3||Read<uint8_t>(human.ps,0x224)!=3
       ||Read<void*>(human.pc,0x470)!=human.ps||Read<void*>(human.ps,0x210)!=human.pc
       ||(Read<uint8_t>(human.ps,local_server_npc_identity::BotOffset)&local_server_npc_identity::BotMask)
       ||local_server_item_ownership::ActorWorld(human.pc)!=world
       ||local_server_item_ownership::ActorWorld(human.ps)!=world)return false;
    connection=Read<void*>(human.pc,0x510);
    const auto peers=Read<Array>(registry.driver,0xc8);
    return connection&&ValidArray(peers,8,32)&&peers.count==1&&Read<void*>(peers.data)==connection
        &&Read<void*>(connection,0x60)==registry.driver&&Read<void*>(connection,0x38)==human.pc
        &&Read<void*>(connection,0xa0)==human.pc&&Read<int32_t>(connection,0x13c)==3
        &&Read<uint8_t>(connection,0x1d8)==3;
}
inline bool Context(const Record& r,bool deferWeaponMembership=false){
    const std::array<void*,10> objects{r.world,r.controller,r.playerState,r.pawn,r.inventory,r.item,r.slot,r.connection,r.resource,r.tuning};
    for(size_t n=0;n<objects.size();++n)if(objects[n]&&!Live(objects[n],r.identities[n]))return false;
    void* connection=nullptr;
    const bool sourceValid=r.npcIndex
        ?r.weaponAmmo&&!r.authoredSubequippable&&NpcWeaponAmmoContext(r.world,r.controller,r.playerState,r.pawn,r.inventory,r.sourceEpoch,r.npcIndex,connection)
        :local_server_item_ownership::ActualHuman(r.world,r.controller,r.playerState,r.pawn,r.inventory,connection);
    return r.sourceEpoch&&sourceValid&&(!r.npcIndex||Read<uint8_t>(r.item,0x128)==r.npcRemoteRole)
        &&connection==r.connection&&Read<uint8_t>(r.item,0x224)==3&&Read<void*>(r.item,0x210)==r.pawn
        &&local_server_item_ownership::ActorWorld(r.item)==r.world
        &&Read<void*>(r.item,0x4a8)==r.pawn
        &&local_server_item_ownership::ValidSlot(r.inventory,r.slot,r.slotType)
        &&(local_server_item_ownership::Contains(r.slot,r.item)||(r.weaponAmmo&&r.pendingGrant)
            ||(deferWeaponMembership&&r.weaponAmmo))
        &&Read<void*>(r.resource,0xc8)==r.item&&(!r.tuning||Read<void*>(r.tuning,0xc8)==r.item)
        &&(!r.reserveAmmo.component||(Live(r.reserveAmmo.component,r.reserveAmmo.identity)
            &&Read<void*>(r.reserveAmmo.component,0xc8)==r.item&&Read<void*>(r.resource,0x138)==r.reserveAmmo.component))
        &&(!r.authoredSubequippable||(!r.pendingGrant&&r.slotType==13&&SubEquippableContext(r.pawn,r.inventory,r.item,r.subequippableProof)));
}
inline bool ReadMap(void* tuning,std::array<RuntimeEntry,MaxRows>& entries,int32_t& count){
    auto map=static_cast<uint8_t*>(tuning)+0xe8;count=original_ability_tuning::MapCount(map);
    if(count<0||count>int32_t(MaxRows))return false;
    const auto sparse=Read<Array>(map);auto bits=Read<const uint32_t*>(map,0x20);
    if(!bits)bits=reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(map)+0x10);
    if(!ValidArray(sparse,48,8192)||(sparse.count&&!original_players::Readable(bits,size_t((sparse.count+31)/32)*4)))return false;
    int32_t selected=0;
    for(int32_t n=0;n<sparse.count;++n){if(!(bits[n/32]&(1U<<(n%32))))continue;
        if(selected>=count)return false;auto source=sparse.data+size_t(n)*48;auto& e=entries[selected];
        if(!Read<uint64_t>(source)||Read<uint8_t>(source,0x10)>3)return false;
        for(int32_t prior=0;prior<selected;++prior)if(std::memcmp(entries[prior].tag,source,8)==0)return false;
        std::memcpy(e.tag,source,12);std::memcpy(&e.value,source+0x10,24);++selected;
    }
    return selected==count;
}
inline bool SameEntry(const RuntimeEntry& a,const RuntimeEntry& b){
    return std::memcmp(a.tag,b.tag,12)==0&&std::memcmp(&a.value,&b.value,24)==0;
}
inline bool ArrayMatches(void* tuning,const std::array<RuntimeEntry,MaxRows>& entries,int32_t count){
    const auto a=Read<Array>(tuning,0x138);
    if(!ValidArray(a,sizeof(RuntimeEntry),int32_t(MaxRows))||a.count!=count)return false;
    for(int32_t n=0;n<a.count;++n){const auto e=reinterpret_cast<const RuntimeEntry*>(a.data+size_t(n)*sizeof(RuntimeEntry));
        if(!SameEntry(*e,entries[n]))return false;}
    return true;
}
inline bool PrepareTuning(Record& r){
    if(!r.tuning){r.tuningReady=true;return true;}
    if(!original_players::IsA(r.tuning,original_ability_tuning::ComponentClass)||Read<void*>(r.tuning,0xc8)!=r.item)return Fail(4523,"host_resource_tuning_default_subobject_mismatch");
    if(!EnsureRoute(r.item,r.tuning,r.tuningRoute))return false;
    void* property=nullptr;
    if(!NetProperty(r.tuning,L"ReplicatedAbilityTunings",0x138,16,property))return Fail(4524,"host_resource_tuning_net_array_metadata_mismatch");
    r.arrayProperty=property;r.innerProperty=Read<void*>(property,0x80);
    auto runtimeType=engine::function<void*(*)()>(0x6382bc0)();
    if(!runtimeType||runtimeType!=engine::Find(L"/Script/ShooterGame.AbilityTuningRuntimeEntry")
       ||!r.innerProperty||Read<int32_t>(r.innerProperty,0x30)!=1||Read<int32_t>(r.innerProperty,0x34)!=40
       ||Read<void*>(r.innerProperty,0x78)!=runtimeType||Read<int32_t>(runtimeType,0x60)!=40)
        return Fail(4525,"host_resource_tuning_native_runtime_entry_metadata_mismatch");
    original_jett_priming::PropertyInfo tag{},value{};
    if(original_jett_priming::FieldOffset(runtimeType,L"AbilityTuningTag",12,&tag)!=0||tag.elementSize!=12
       ||original_jett_priming::FieldOffset(runtimeType,L"AbilityTuningValue",24,&value)!=16||value.elementSize!=24)
        return Fail(4526,"host_resource_tuning_native_tag_value_layout_mismatch");
    auto onRep=engine::Find(L"/Script/ShooterGame.AbilityTuningComponent.OnRep_ReplicatedAbilityTunings");
    if(!onRep||Read<uint16_t>(onRep,0xbe)!=0||Read<void*>(onRep,0xe0)!=reinterpret_cast<void*>(engine::image+0x6382bf0))
        return Fail(4527,"host_resource_tuning_original_repnotify_mismatch");
    r.tuningMetadata=true;
    std::array<RuntimeEntry,MaxRows> entries{};int32_t count=0;
    if(!ReadMap(r.tuning,entries,count))return Fail(4528,"host_resource_tuning_original_source_map_invalid");
    r.mapCount=count;const auto before=Read<Array>(r.tuning,0x138);r.arrayBefore=before.count;
    if(!ValidArray(before,sizeof(RuntimeEntry),int32_t(MaxRows)))return Fail(4529,"host_resource_tuning_native_destination_array_invalid");
    if(before.count){
        if(!ArrayMatches(r.tuning,entries,count))return Fail(4530,"host_resource_tuning_existing_native_array_conflicts_with_source_map");
    }else{
        if(count&&!CanPublish(r,r.tuningPushBased))return false;
        // Exact Array_Add wrapper45125f0 ->4516300 ABI: ArrayAddress,
        // actual FArrayProperty, actual inner-type item. The original routine
        // grows its own FScriptArray, initializes and FProperty-copies each item.
        for(int32_t n=0;n<count;++n){
            const auto index=engine::function<int32_t(*)(void*,void*,const void*)>(0x4516300)
                (static_cast<uint8_t*>(r.tuning)+0x138,r.arrayProperty,&entries[n]);
            if(index!=n){state.restartRequired=true;return Fail(4531,"host_resource_tuning_original_array_add_failed_requires_new_pair");}
            ++r.arrayAdds;++state.nativeArrayAdds;
        }
        r.nativeArrayAdded=count>0;
    }
    if(!ArrayMatches(r.tuning,entries,count)){state.restartRequired=true;return Fail(4532,"host_resource_tuning_native_array_readback_failed");}
    if(!original_ability_tuning::Event(r.tuning,L"/Script/ShooterGame.AbilityTuningComponent.OnRep_ReplicatedAbilityTunings",0,nullptr))
        return Fail(4533,"host_resource_tuning_original_repnotify_dispatch_failed");
    r.nativeOnRep=true;r.arrayAfter=Read<int32_t>(r.tuning,0x140);
    if(original_ability_tuning::MapCount(static_cast<uint8_t*>(r.tuning)+0xe8)!=count)return Fail(4534,"host_resource_tuning_native_map_roundtrip_count_changed");
    r.mapPreserved=true;
    for(int32_t n=0;n<count;++n){auto actual=original_ability_tuning::FindValue(static_cast<uint8_t*>(r.tuning)+0xe8,entries[n].tag);
        if(!actual||std::memcmp(actual,&entries[n].value,24)!=0){r.mapPreserved=false;break;}}
    if(!r.mapPreserved)return Fail(4535,"host_resource_tuning_native_map_roundtrip_changed_source_value");
    r.tuningReady=true;return true;
}
inline bool PublishResource(Record& r){
    r.resourceReady=false;
    if(!Thread()||!Context(r)){r.ready=false;return Fail(4536,"host_resource_publication_source_or_epoch_retired");}
    r.nativeAmount=Read<int32_t>(r.resource,0x10c);r.authorityBefore=Read<int32_t>(r.resource,0x110);
    r.authorityAfter=r.authorityBefore;
    // BlueprintResource overrides Consume/Replenish with authored Blueprint
    // events. X DaggersRemaining is not this generic integer; preserve its
    // original multicast mechanism and do not invent an equivalence.
    if(r.blueprintResource){r.resourceReady=true;return true;}
    if(!r.resourceMetadata||!r.resourceRoute.ready)return Fail(4537,"host_resource_publication_requires_native_net_property_route");
    if(WaitingScalarRetryFrame(r))return true;
    if(r.nativeAmount!=r.authorityBefore){
        if(!CanPublish(r,r.resourcePushBased,r.resource,&r.resourceTracker)){r.ready=false;return false;}
        if(r.resourceTracker.deferred)return true;
        if(!original_players::Write(r.resource,0x110,r.nativeAmount))return Fail(4538,"host_resource_authority_record_write_failed");
        r.authorityAfter=Read<int32_t>(r.resource,0x110);
        if(r.authorityAfter!=r.nativeAmount)return Fail(4539,"host_resource_authority_record_readback_failed");
        if(!PublishScalarTracker(r,r.resourceTracker,r.authorityAfter))return false;
        ++r.resourcePublications;++state.resourcePublications;
    }
    // Original host Notify already ran for its local resource change; calling
    // it again would duplicate delegates. Original receiving OnRep6b95900 owns
    // prediction reconciliation on the real client when this NET field arrives.
    r.resourceReady=true;return true;
}
inline bool PrepareImpl(void* world,void* pc,void* ps,void* pawn,void* inventory,void* item,uint32_t epoch){
    if(!Thread()||state.error)return false;
    if(!epoch)return Fail(4540,"host_resource_publication_requires_explicit_spawn_epoch");
    if(!VerifyProfiles())return false;
    if(!local_server_item_ownership::VerifyProfiles())
        return Fail(local_server_item_ownership::GetState().error,local_server_item_ownership::GetState().stage);
    void* connection=nullptr;
    if(!local_server_item_ownership::ActualHuman(world,pc,ps,pawn,inventory,connection)
       ||Read<uint8_t>(item,0x224)!=3||Read<void*>(item,0x210)!=pawn||Read<void*>(item,0x4a8)!=pawn)
        return Fail(4541,"host_resource_publication_requires_actual_owned_admitted_authority_item");
    if(auto existing=FindCached(item)){
        if(existing->world!=world||existing->controller!=pc||existing->playerState!=ps||existing->pawn!=pawn
           ||existing->inventory!=inventory||existing->sourceEpoch!=epoch||!Context(*existing))return Fail(4542,"host_resource_item_context_changed_without_retirement");
        if(existing->ready)return true;
        if(!existing->resourceMetadata||!existing->resourceRoute.ready||!existing->tuningReady||!existing->scalarRetryWaiting)
            return Fail(4542,"host_resource_initial_binding_incomplete_without_native_retry");
        if(!PublishResource(*existing))return false;
        existing->ready=existing->resourceReady&&existing->tuningReady;
        if(existing->ready)existing->stage="host_native_resource_and_tuning_published_delivery_unverified";
        state.stage=existing->stage;return true;
    }
    Record* target=nullptr;for(auto& r:records)if(!r.used){target=&r;break;}
    if(!target)return Fail(4543,"host_resource_item_registration_capacity");
    auto& r=*target;r.world=world;r.controller=pc;r.playerState=ps;r.pawn=pawn;r.inventory=inventory;r.item=item;
    r.connection=connection;r.sourceEpoch=epoch;r.slotType=Read<uint8_t>(item,0x4a0);
    r.slot=r.slotType<17?Read<void*>(inventory,0x1b8+size_t(r.slotType)*8):nullptr;
    if(!local_server_item_ownership::ValidSlot(inventory,r.slot,r.slotType)||!local_server_item_ownership::Contains(r.slot,item))
        return Fail(4544,"host_resource_publication_requires_original_published_slot_membership");
    r.resource=original_weapon::Component(item,ResourceClass);r.tuning=original_weapon::Component(item,original_ability_tuning::ComponentClass);
    if(!original_players::IsA(r.resource,ResourceClass)||Read<void*>(r.resource,0xc8)!=item)return Fail(4545,"host_resource_original_resource_default_subobject_missing");
    const std::array<void*,10> objects{world,pc,ps,pawn,inventory,item,r.slot,connection,r.resource,r.tuning};
    for(size_t n=0;n<objects.size();++n)if(objects[n]&&!original_inventory_slots::CaptureIdentity(objects[n],r.identities[n]))
        return Fail(4546,"host_resource_original_source_weak_identity_missing");
    if(state.registrations==UINT64_MAX)return Fail(4551,"host_resource_registration_generation_exhausted");
    r.used=true;r.registration=++state.registrations;
    if(!Context(r))return Fail(4547,"host_resource_original_source_context_invalid");
    if(!EnsureRoute(item,r.resource,r.resourceRoute))return false;
    void* amountProperty=nullptr;
    auto onRep=engine::Find(L"/Script/ShooterGame.ResourceComponent.OnRep_AuthResourceAmount");
    r.resourceMetadata=NetProperty(r.resource,L"AuthResourceAmount",0x110,4,amountProperty)
        &&onRep&&Read<uint16_t>(onRep,0xbe)==0&&Read<void*>(onRep,0xe0)==reinterpret_cast<void*>(engine::image+0x6b95900);
    if(!r.resourceMetadata)return Fail(4548,"host_resource_native_auth_amount_repnotify_metadata_mismatch");
    r.blueprintResource=original_players::IsA(r.resource,BlueprintResourceClass);
    if(!r.blueprintResource&&!original_players::IsA(r.resource,EquipmentChargeClass)&&!original_players::IsA(r.resource,AmmoClass))
        return Fail(4549,"host_resource_native_simulation_class_not_profiled");
    auto pushSettings=engine::DefaultObject(engine::Class(0x282ff30));
    if(!pushSettings)return Fail(4554,"host_resource_original_push_model_policy_unavailable");
    r.resourcePushBased=engine::function<bool(*)(void*,void*)>(0x2839df0)(pushSettings,engine::Class(ResourceClass));
    r.tuningPushBased=engine::function<bool(*)(void*,void*)>(0x2839df0)(pushSettings,engine::Class(original_ability_tuning::ComponentClass));
    if(!PrepareTuning(r)||!PublishResource(r))return false;
    r.ready=r.resourceReady&&r.tuningReady;
    if(r.ready)r.stage="host_native_resource_and_tuning_published_delivery_unverified";
    // A registered initial binding can wait for an original noninitial channel
    // pass. Success admits retry; ItemReady/WriteItem retain ready=false until
    // the actual authority value and required native history are published.
    state.stage=r.stage;return true;
}
inline bool ItemReady(void* item){
    const auto r=FindCached(item);return r&&r->ready&&r->resourceReady&&r->tuningReady&&!r->scalarRetryWaiting;
}
inline bool PollWeaponAmmoImpl(void* item);
inline bool PollItemImpl(void* item){
    if(!Thread()||state.error)return false;auto r=FindCached(item);
    if(!r||(!r->ready&&!(r->scalarRetryWaiting&&r->resourceMetadata&&r->resourceRoute.ready&&r->tuningReady)))
        return Fail(4550,"host_resource_poll_requires_prepared_item");
    if(r->weaponAmmo)return PollWeaponAmmoImpl(item);
    ++r->polls;
    if(!PublishResource(*r))return false;
    // The initial binding remains valid while a later native publication is
    // waiting. resource_ready=false and retry diagnostics expose that wait;
    // do not erase its registered binding or relabel it as published.
    if(r->scalarRetryWaiting){state.stage=r->stage;return true;}
    r->ready=r->resourceReady&&r->tuningReady;
    r->stage="host_native_resource_after_original_step_published_delivery_unverified";state.stage=r->stage;return true;
}
inline bool AmmoMethod(const wchar_t* path,uint16_t bytes,uintptr_t exec){
    auto fn=engine::Find(path);
    return fn&&Read<uint16_t>(fn,0xbe)==bytes&&Read<void*>(fn,0xe0)==reinterpret_cast<void*>(engine::image+exec);
}
inline bool InspectInitialAmmo(Record& r,AmmoObservation& d,void* component){
    d.component=component;
    if(!original_players::IsA(component,AmmoClass)||Read<void*>(component,0xc8)!=r.item
       ||!original_inventory_slots::CaptureIdentity(component,d.identity))return Fail(4622,"host_initial_ammo_actual_component_or_owner_invalid");
    auto table=Read<void*>(component);
    d.nativeMethodsVerified=Read<void*>(table,0x3e0)==reinterpret_cast<void*>(engine::image+0x6ef0e40)
        &&Read<void*>(table,0x4f8)==reinterpret_cast<void*>(engine::image+0x6ef0f60);
    if(!d.nativeMethodsVerified)return Fail(4623,"host_initial_ammo_native_startup_or_replenish_changed");
    d.flagsC2=Read<uint8_t>(component,0xc2);d.inNativeBeginPlay=Read<uint8_t>(component,0x140);
    if((d.flagsC2&0x40)==0||d.inNativeBeginPlay)
        return Fail(4624,"host_initial_ammo_requires_completed_original_component_begin_play");
    if(!EnsureRoute(r.item,component,d.route))return false;
    if(r.npcIndex&&d.route.usesRegisteredList&&d.route.condition!=0)
        return Fail(4688,"host_npc_ammo_requires_original_unconditional_component_route");
    original_jett_priming::PropertyInfo authored{};void *maximum=nullptr,*authority=nullptr;
    d.metadataVerified=original_jett_priming::FieldOffset(Read<void*>(component,0x10),L"Ammo",4,&authored)==0x12c
        &&authored.arrayDim==1&&authored.elementSize==4
        &&NetProperty(component,L"MaxAmmo",0x130,4,maximum)&&NetProperty(component,L"AuthResourceAmount",0x110,4,authority);
    if(!d.metadataVerified)return Fail(4625,"host_initial_ammo_native_authored_or_net_property_schema_changed");
    d.authoredInitial=Read<int32_t>(component,0x12c);d.authorityBefore=Read<int32_t>(component,0x110);
    if(!original_ability_tuning::Event(component,L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,&d.before)
       ||!original_ability_tuning::Event(component,L"/Script/ShooterGame.AmmoComponent.GetMaxAmmo",4,&d.maximum)
       ||d.before!=Read<int32_t>(component,0x10c)||d.maximum!=Read<int32_t>(component,0x130)
       ||d.before<0||d.maximum<0||d.maximum>65535||d.before>d.maximum||d.authoredInitial<0||d.authoredInitial>65535)
        return Fail(4626,"host_initial_ammo_original_amount_or_authored_limits_invalid");
    d.after=d.before;d.authorityAfter=d.authorityBefore;return true;
}
inline bool InitializeNativeAmmo(Record& r,AmmoObservation& d){
    if(!Context(r)||!Live(d.component,d.identity)||Read<void*>(d.component,0xc8)!=r.item)
        return Fail(4627,"host_initial_ammo_source_changed_before_native_initialization");
    // The retained Ammo BeginPlay6EF0E40 supplies this actual authored Ammo+12c
    // to the same native virtual Replenish. Preserve any existing native result.
    // Only a still-zero fresh grant can receive this missing initialization.
    if(d.before==0&&d.authoredInitial>0){
        struct Replenish {int32_t amount,result=0;} replenish{d.authoredInitial};
        if(!original_ability_tuning::Event(d.component,L"/Script/ShooterGame.ResourceComponent.Replenish",8,&replenish))
            return Fail(4628,"host_initial_ammo_original_replenish_dispatch_failed");
        d.replenished=true;d.nativeReplenishResult=replenish.result;++state.ammoNativeReplenishes;
    }
    if(!original_ability_tuning::Event(d.component,L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,&d.after)
       ||d.after!=Read<int32_t>(d.component,0x10c)||d.after<d.before||d.after>d.maximum
       ||(d.authoredInitial>0&&d.maximum>0&&d.before==0&&d.after==0))
        return Fail(4629,"host_initial_ammo_native_replenish_result_not_observed");
    return true;
}
inline bool PublishInitialAmmo(Record& r,AmmoObservation& d){
    if(!Thread()||!Context(r)||!Live(d.component,d.identity)||!d.metadataVerified||!d.route.ready
       ||(d.component!=r.resource&&d.component!=r.reserveAmmo.component))return Fail(4630,"host_initial_ammo_publication_source_or_native_route_invalid");
    const auto native=Read<int32_t>(d.component,0x10c);
    if(native!=d.after)return Fail(4631,"host_initial_ammo_native_amount_changed_before_publication");
    const auto previous=Read<int32_t>(d.component,0x110);d.authorityAfter=previous;
    if(previous!=native){
        if(!CanPublish(r,r.resourcePushBased,d.component,&d.tracker))return false;
        if(d.tracker.deferred)return Fail(4642,"host_initial_ammo_requires_new_native_grant_before_first_actor_channel");
        if(!original_players::Write(d.component,0x110,native)||Read<int32_t>(d.component,0x110)!=native)
            return Fail(4632,"host_initial_ammo_authority_record_readback_failed");
        if(!PublishScalarTracker(r,d.tracker,native))return false;
        d.authorityAfter=native;++r.resourcePublications;++state.resourcePublications;++state.ammoAuthorityPublications;
    }
    d.published=true;return true;
}
// Call from the validated native authority grant, after original owner callbacks
// and owned simulation policy, before any candidate ActorChannel. Pending grants
// pass their exact real target slot; published grants require actual containment.
// The epoch is the host match spawn epoch, never a weapon request sequence.
inline bool PrepareWeaponAmmoImpl(void* world,void* pc,void* ps,void* pawn,void* inventory,void* item,
                                  uint32_t epoch,void* targetSlot,local_server_item_ownership::Source source,
                                  const AuthoredSubequippableProof* subequippable=nullptr,size_t npcIndex=0){
    if(!Thread()||state.error)return false;
    // The human entry point keeps its existing ownership admission. NPCs
    // enter only through the separate match-registered firearm API below.
    if(!npcIndex&&(!pc||!Read<void*>(pc,0x510))){++state.ammoSkippedNonhuman;return true;}
    if(npcIndex&&subequippable)return Fail(4689,"host_npc_ammo_accepts_primary_or_secondary_only");
    if(!epoch||!VerifyProfiles()||!VerifyAmmoProfiles())return state.error?false:Fail(4633,"host_initial_ammo_requires_explicit_spawn_epoch");
    void* connection=nullptr;
    const bool sourceValid=npcIndex?NpcWeaponAmmoContext(world,pc,ps,pawn,inventory,epoch,npcIndex,connection)
        :local_server_item_ownership::ActualHuman(world,pc,ps,pawn,inventory,connection);
    const auto remoteRole=Read<uint8_t>(item,0x128);
    if(!sourceValid||!original_players::IsA(item,0x645c660)||Read<uint8_t>(item,0x224)!=3
       ||(npcIndex?(remoteRole!=1&&remoteRole!=2):remoteRole!=2)
       ||Read<void*>(item,0x210)!=pawn||Read<void*>(item,0x4a8)!=pawn
       ||local_server_item_ownership::ActorWorld(item)!=world)
        return Fail(4634,"host_initial_ammo_requires_actual_owned_admitted_native_authority_item");
    if(source!=local_server_item_ownership::Source::PublishedSlot&&source!=local_server_item_ownership::Source::PendingAuthorityGrant)
        return Fail(4635,"host_initial_ammo_grant_source_invalid");
    const auto slotType=Read<uint8_t>(item,0x4a0);
    auto slot=slotType<17?Read<void*>(inventory,0x1b8+size_t(slotType)*8):nullptr;
    if((targetSlot&&targetSlot!=slot)||!local_server_item_ownership::ValidSlot(inventory,slot,slotType)
       ||(source==local_server_item_ownership::Source::PublishedSlot&&!local_server_item_ownership::Contains(slot,item)))
        return Fail(4636,"host_initial_ammo_original_target_slot_or_published_membership_invalid");
    for(size_t n=0;n<17;++n)if(!local_server_item_ownership::ValidSlot(inventory,Read<void*>(inventory,0x1b8+n*8),n))
        return Fail(4637,"host_initial_ammo_requires_actual_seventeen_native_slots");
    const bool authoredChild=subequippable&&slotType==13&&source==local_server_item_ownership::Source::PublishedSlot
        &&SubEquippableContext(pawn,inventory,item,*subequippable);
    if(subequippable&&!authoredChild)return Fail(4680,"host_child_ammo_requires_real_configured_component_reference_and_published_backpack_membership");
    auto ammo=original_weapon::Component(item,AmmoClass);
    if(!ammo){++state.ammoSkippedNonballistic;return true;}
    auto firing=original_weapon::Component(item,0x684d4c0);
    if((slotType>1&&!authoredChild)||!original_players::IsA(firing,0x684d4c0)||Read<void*>(firing,0xc8)!=item)
        return Fail(4638,"host_initial_ammo_requires_original_primary_or_secondary_firearm");
    if(auto existing=FindCached(item)){
        if(!existing->weaponAmmo||!existing->ammoInitialPrepared||existing->world!=world||existing->controller!=pc
           ||existing->playerState!=ps||existing->pawn!=pawn||existing->inventory!=inventory||existing->slot!=slot
           ||existing->sourceEpoch!=epoch||existing->npcIndex!=npcIndex||existing->resource!=ammo
           ||existing->authoredSubequippable!=authoredChild||!Context(*existing))
            return Fail(4639,"host_initial_ammo_binding_changed_without_weapon_retirement");
        if(local_server_item_ownership::Contains(slot,item))existing->pendingGrant=false;
        return true; // No refill on repeated equip, polling, or an infinite-ammo flag.
    }
    if(!AmmoMethod(L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,0x637fea0)
       ||!AmmoMethod(L"/Script/ShooterGame.AmmoComponent.GetMaxAmmo",4,0x63c84c0)
       ||!AmmoMethod(L"/Script/ShooterGame.AmmoComponent.GetReserveComponent",8,0x37e4230)
       ||!AmmoMethod(L"/Script/ShooterGame.ResourceComponent.Replenish",8,0x6b95970)
       ||!AmmoMethod(L"/Script/ShooterGame.ResourceComponent.OnRep_AuthResourceAmount",0,0x6b95900))
        return Fail(4640,"host_initial_ammo_native_getter_replenish_or_repnotify_abi_changed");
    uint8_t begun=0;
    if(!original_ability_tuning::Event(item,L"/Script/Engine.Actor.HasActorBegunPlay",1,&begun)||!begun)
        return Fail(4641,"host_initial_ammo_requires_completed_original_weapon_begin_play");
    Record* target=nullptr;for(auto& r:records)if(!r.used){target=&r;break;}
    if(!target)return Fail(4543,"host_resource_item_registration_capacity");
    auto& r=*target;r.world=world;r.controller=pc;r.playerState=ps;r.pawn=pawn;r.inventory=inventory;r.item=item;
    r.slot=slot;r.slotType=slotType;r.connection=connection;r.sourceEpoch=epoch;r.resource=ammo;r.weaponAmmo=true;
    r.npcIndex=npcIndex;if(npcIndex)r.npcRemoteRole=remoteRole;
    r.authoredSubequippable=authoredChild;if(authoredChild)r.subequippableProof=*subequippable;
    r.pendingGrant=source==local_server_item_ownership::Source::PendingAuthorityGrant&&!local_server_item_ownership::Contains(slot,item);
    const std::array<void*,10> objects{world,pc,ps,pawn,inventory,item,slot,connection,ammo,nullptr};
    for(size_t n=0;n<objects.size();++n)if(objects[n]&&!original_inventory_slots::CaptureIdentity(objects[n],r.identities[n]))
        return Fail(4546,"host_resource_original_source_weak_identity_missing");
    if(state.registrations==UINT64_MAX)return Fail(4551,"host_resource_registration_generation_exhausted");
    r.used=true;r.registration=++state.registrations;
    if(!Context(r)||!ObserveChannel(r)||r.channelObserved)
        return Fail(4642,"host_initial_ammo_requires_new_native_grant_before_first_actor_channel");
    if(!InspectInitialAmmo(r,r.magazineAmmo,ammo)||r.magazineAmmo.maximum<=0)
        return state.error?false:Fail(4643,"host_initial_ammo_original_magazine_capacity_unavailable");
    void* reserve=nullptr;
    if(!original_ability_tuning::Event(ammo,L"/Script/ShooterGame.AmmoComponent.GetReserveComponent",8,&reserve)
       ||reserve!=Read<void*>(ammo,0x138)||reserve==ammo)return Fail(4644,"host_initial_ammo_original_reserve_reference_invalid");
    if(reserve&&!InspectInitialAmmo(r,r.reserveAmmo,reserve))return false;
    auto pushSettings=engine::DefaultObject(engine::Class(0x282ff30));
    if(!pushSettings)return Fail(4554,"host_resource_original_push_model_policy_unavailable");
    r.resourcePushBased=engine::function<bool(*)(void*,void*)>(0x2839df0)(pushSettings,engine::Class(ResourceClass));
    r.ammoInitialAttempted=true;
    if(!InitializeNativeAmmo(r,r.magazineAmmo)||(reserve&&!InitializeNativeAmmo(r,r.reserveAmmo)))return false;
    if(!PublishInitialAmmo(r,r.magazineAmmo)||(reserve&&!PublishInitialAmmo(r,r.reserveAmmo)))return false;
    r.resourceRoute=r.magazineAmmo.route;r.resourceMetadata=true;r.resourceReady=true;r.tuningReady=true;
    r.nativeAmount=r.magazineAmmo.after;r.authorityBefore=r.magazineAmmo.authorityBefore;r.authorityAfter=r.magazineAmmo.authorityAfter;
    r.ammoInitialPrepared=true;r.ready=true;++state.ammoInitializations;
    r.stage="host_original_finite_weapon_ammo_initial_records_published_delivery_unverified";state.stage=r.stage;return true;
}
inline bool ReadOngoingAmmo(Record& r,AmmoObservation& d){
    d.ongoingSynchronized=false;
    if(!d.metadataVerified||!d.nativeMethodsVerified||!Live(d.component,d.identity)
       ||!original_players::IsA(d.component,AmmoClass)||Read<void*>(d.component,0xc8)!=r.item
       ||!original_players::Readable(d.component,0x148))return Fail(4652,"host_ongoing_ammo_actual_native_component_or_metadata_changed");
    if(!ObserveRoute(r.item,d.component,d.route))return Fail(4653,"host_ongoing_ammo_original_component_replication_route_changed");
    if(r.npcIndex&&d.route.usesRegisteredList&&d.route.condition!=0)
        return Fail(4688,"host_npc_ammo_requires_original_unconditional_component_route");
    // Exact pinned GetAmmo637FEA0 and native resource getter6F2BEF0 both return
    // this original +10c int. Read it without an extra gameplay callback. This
    // path never calls Replenish, Consume, Reload or a client amount setter.
    const auto previous=d.ongoingObserved?d.currentNative:d.after;
    d.currentNative=Read<int32_t>(d.component,0x10c);d.currentMaximum=Read<int32_t>(d.component,0x130);
    d.currentAuthorityBefore=Read<int32_t>(d.component,0x110);d.currentAuthorityAfter=d.currentAuthorityBefore;
    d.flagsC2=Read<uint8_t>(d.component,0xc2);d.inNativeBeginPlay=Read<uint8_t>(d.component,0x140);
    if(d.currentNative!=previous)++d.nativeChanges;
    ++d.ongoingSamples;d.ongoingObserved=true;return true;
}
inline bool CommitOngoingAmmo(Record& r,AmmoObservation& d){
    if(Read<int32_t>(d.component,0x10c)!=d.currentNative
       ||Read<int32_t>(d.component,0x110)!=d.currentAuthorityBefore)
        return Fail(4654,"host_ongoing_ammo_native_counter_changed_during_publication");
    if(d.currentNative!=d.currentAuthorityBefore){
        if(!original_players::Write(d.component,0x110,d.currentNative)
           ||Read<int32_t>(d.component,0x110)!=d.currentNative){
            state.restartRequired=true;return Fail(4655,"host_ongoing_ammo_authority_record_readback_failed_requires_new_pair");}
        if(!PublishScalarTracker(r,d.tracker,d.currentNative))return false;
        d.currentAuthorityAfter=d.currentNative;++d.ongoingPublications;++r.resourcePublications;
        ++state.resourcePublications;++state.ammoAuthorityPublications;
        if(d.component==r.resource)++state.ammoOngoingMagazinePublications;else ++state.ammoOngoingReservePublications;
    }
    // Native firing/reload/replenish already owns host NotifyResourceChanged.
    // Do not repeat its delegates. The unchanged original receiving RepNotify
    // 6B95900 owns the real client's prediction reconciliation after delivery.
    d.ongoingSynchronized=d.currentAuthorityAfter==d.currentNative;return true;
}
inline bool PollWeaponAmmoImpl(void* item){
    if(!Thread()||state.error)return false;auto r=FindCached(item);
    if(!r||!r->weaponAmmo)return true; // No native work for an unregistered NPC, melee or skill.
    r->ammoOngoingReady=false;++r->ammoOngoingPolls;++r->polls;++state.ammoOngoingPolls;
    r->magazineAmmo.ongoingSynchronized=false;r->reserveAmmo.ongoingSynchronized=false;
    if(!r->ammoInitialPrepared||!r->ready||!VerifyAmmoOngoingProfiles())
        return state.error?false:Fail(4656,"host_ongoing_ammo_requires_completed_native_initial_binding");
    // Validate identities/ownership first. A normal pending grant or delayed
    // retirement can temporarily have no slot/current membership; defer those
    // records below without publishing from an unreferenced old item.
    if(!Context(*r,true)||r->magazineAmmo.component!=r->resource
       ||Read<uint8_t>(r->item,0x128)!=(r->npcIndex?r->npcRemoteRole:2))return Fail(4657,"host_ongoing_ammo_source_epoch_ownership_or_reserve_binding_changed");
    const bool contained=local_server_item_ownership::Contains(r->slot,r->item);
    const bool active=Read<void*>(r->inventory,0x280)==r->item||Read<void*>(r->inventory,0x268)==r->item;
    if(!contained&&!active){
        ++r->ammoOngoingDeferrals;++state.ammoOngoingDeferrals;
        r->stage="host_weapon_ammo_waiting_actual_inventory_publication_or_retirement";return true;
    }
    if(contained)r->pendingGrant=false;
    if(!ReadOngoingAmmo(*r,r->magazineAmmo)||(r->reserveAmmo.component&&!ReadOngoingAmmo(*r,r->reserveAmmo)))return false;
    const bool magazineChanged=r->magazineAmmo.currentNative!=r->magazineAmmo.currentAuthorityBefore;
    const bool reserveChanged=r->reserveAmmo.component&&r->reserveAmmo.currentNative!=r->reserveAmmo.currentAuthorityBefore;
    if(WaitingScalarRetryFrame(*r)){
        ++r->ammoOngoingDeferrals;++state.ammoOngoingDeferrals;state.stage=r->stage;return true;
    }
    if(magazineChanged||reserveChanged){
        auto settings=engine::DefaultObject(engine::Class(0x282ff30));
        if(!settings)return Fail(4554,"host_resource_original_push_model_policy_unavailable");
        r->currentResourcePushBased=engine::function<bool(*)(void*,void*)>(0x2839df0)(settings,engine::Class(ResourceClass));
        ++r->ammoOngoingPolicySamples;
        // Preserve both initial and live descriptor policy. Each changed
        // component must first resolve its own real native tracker route; the
        // pair is admitted before either Auth field changes. Unknown effective
        // layout/skip modes remain4553, without fabricated push flags/arrays.
        const bool descriptorPush=r->resourcePushBased||r->currentResourcePushBased;
        if(magazineChanged&&!CanPublish(*r,descriptorPush,r->magazineAmmo.component,&r->magazineAmmo.tracker))return false;
        if(reserveChanged&&!CanPublish(*r,descriptorPush,r->reserveAmmo.component,&r->reserveAmmo.tracker))return false;
        if((magazineChanged&&r->magazineAmmo.tracker.deferred)||(reserveChanged&&r->reserveAmmo.tracker.deferred)){
            ++r->ammoOngoingDeferrals;++state.ammoOngoingDeferrals;state.stage=r->stage;return true;
        }
        if(magazineChanged&&reserveChanged&&r->magazineAmmo.tracker.prepared&&r->reserveAmmo.tracker.prepared
           &&r->magazineAmmo.tracker.manager==r->reserveAmmo.tracker.manager)
            return Fail(4553,"host_native_scalar_tracker_distinct_ammo_objects_share_one_shadow_manager");
    }
    if(!CommitOngoingAmmo(*r,r->magazineAmmo))return false;
    if(r->reserveAmmo.component&&!CommitOngoingAmmo(*r,r->reserveAmmo)){
        state.restartRequired=true;
        r->stage=magazineChanged?"host_ongoing_ammo_pair_second_commit_failed_requires_new_pair":"host_ongoing_ammo_reserve_commit_failed_requires_new_pair";
        state.stage=r->stage;return false;
    }
    r->resourceRoute=r->magazineAmmo.route;r->nativeAmount=r->magazineAmmo.currentNative;
    r->authorityBefore=r->magazineAmmo.currentAuthorityBefore;r->authorityAfter=r->magazineAmmo.currentAuthorityAfter;
    r->ammoOngoingReady=r->magazineAmmo.ongoingSynchronized&&(!r->reserveAmmo.component||r->reserveAmmo.ongoingSynchronized);
    r->stage="host_original_weapon_ammo_current_records_synchronized_delivery_unverified";state.stage=r->stage;return true;
}
// Validate the current NPC source before testing native life. A dead life
// waits for the existing match retirement boundary without reading a dropped
// weapon's ownership. An invalid live/source tuple remains an explicit error.
inline bool NpcWeaponAmmoAlive(const Record& r,bool& alive){
    void* connection=nullptr;
    if(!r.npcIndex||!NpcWeaponAmmoContext(r.world,r.controller,r.playerState,r.pawn,r.inventory,
        r.sourceEpoch,r.npcIndex,connection)||connection!=r.connection)
        return Fail(4690,"host_npc_ammo_current_life_source_invalid");
    auto method=engine::Find(L"/Script/ShooterGame.ShooterCharacter.IsAlive");
    uint8_t result=255;
    if(!method||Read<uint16_t>(method,0xbe)!=1||Read<uint32_t>(method,0xb8)!=0x54020401
       ||Read<uintptr_t>(method,0xe0)!=engine::image+0x6c62390
       ||Read<void*>(method,0x28)!=engine::Class(original_players::ShooterCharacterClass)
       ||!original_ability_tuning::Event(r.pawn,L"/Script/ShooterGame.ShooterCharacter.IsAlive",1,&result)||result>1)
        return Fail(4691,"host_npc_ammo_original_life_query_unavailable");
    alive=result!=0;return true;
}
inline bool PollPlayerWeaponAmmoImpl(void* pawn){
    if(!Thread()||state.error)return false;
    for(const auto& r:records)if(r.used&&r.weaponAmmo&&r.pawn==pawn&&r.npcIndex){
        bool alive=false;if(!NpcWeaponAmmoAlive(r,alive))return false;
        if(!alive){
            ++state.ammoOngoingDeferrals;
            for(auto& waiting:records)if(waiting.used&&waiting.weaponAmmo&&waiting.pawn==pawn&&waiting.npcIndex){
                waiting.ammoOngoingReady=false;++waiting.ammoOngoingDeferrals;
                waiting.stage="host_npc_ammo_native_dead_waiting_existing_life_retirement";
            }
            return true;
        }
        break;
    }
    for(auto& r:records)if(r.used&&r.weaponAmmo&&r.pawn==pawn)if(!PollWeaponAmmoImpl(r.item))return false;
    return true;
}
inline bool RetireWeaponAmmoImpl(void* item){
    if(!Thread())return false;
    if(auto r=FindCached(item)){if(!r->weaponAmmo)return Fail(4645,"host_initial_ammo_retire_requires_weapon_record");
        *r={};++state.retired;++state.ammoRetired;}
    return true;
}
// Cached publication receipts belong to one exact NPC life. CaptureRetiringLife
// calls this before original actor destruction. Already-gone/transferred weapons
// need their old receipt retired too; none of their actor fields are accessed.
inline bool RetireNpcWeaponAmmoLife(size_t index,void* pawn,uint32_t epoch,const Weak& identity){
    if(!Thread()||!index||index>=local_server_npc_identity::MaxPlayers||!epoch||!Live(pawn,identity))
        return Fail(4692,"host_npc_ammo_retirement_requires_exact_existing_life");
    for(const auto& r:records)if(r.used&&r.weaponAmmo&&r.npcIndex==index){
        if(r.pawn!=pawn||r.sourceEpoch!=epoch||!original_inventory_slots::SameIdentity(r.identities[3],identity))
            return Fail(4692,"host_npc_ammo_retirement_receipt_life_mismatch");
    }
    for(auto& r:records)if(r.used&&r.weaponAmmo&&r.npcIndex==index){
        r={};++state.retired;++state.ammoRetired;
    }
    return true;
}
inline LONG CaptureException(EXCEPTION_POINTERS* e){
    state.error=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0004520U;
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.restartRequired=true;
    state.stage="host_resource_exception_requires_new_owned_pair";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Prepare(void* world,void* pc,void* ps,void* pawn,void* inventory,void* item,uint32_t epoch){
    __try{return PrepareImpl(world,pc,ps,pawn,inventory,item,epoch);}__except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool PollItem(void* item){__try{return PollItemImpl(item);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool PrepareWeaponAmmo(void* world,void* pc,void* ps,void* pawn,void* inventory,void* item,uint32_t epoch,
                              void* targetSlot=nullptr,local_server_item_ownership::Source source=local_server_item_ownership::Source::PublishedSlot){
    __try{return PrepareWeaponAmmoImpl(world,pc,ps,pawn,inventory,item,epoch,targetSlot,source);}
    __except(CaptureException(GetExceptionInformation())){return false;}
}
// Explicit NPC grant path. Its source tuple is proved against the existing
// match registry on every initial and ongoing publication. Existing native
// owner callbacks, resource methods, ActorChannels and scalar tracker own work.
inline bool PrepareNpcWeaponAmmo(size_t index,void* world,void* pc,void* ps,void* pawn,void* inventory,
                                 void* item,uint32_t epoch,void* targetSlot,local_server_item_ownership::Source source){
    if(!index||index>=local_server_npc_identity::MaxPlayers)
        return Fail(4689,"host_npc_ammo_requires_registered_nonhuman_index");
    __try{return PrepareWeaponAmmoImpl(world,pc,ps,pawn,inventory,item,epoch,targetSlot,source,nullptr,index);}
    __except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool PrepareAuthoredSubequippableAmmoImpl(void* world,void* pc,void* ps,void* pawn,void* inventory,
                                                void* item,uint32_t epoch,void* component,void* ability){
    if(!Thread()||state.error||!epoch)return false;
    if(!state.subequippableProfilesVerified){
        constexpr Profile native[]{
            {0x6388010,133,0x54d1ab3d0271fc2eULL},{0x6eef150,272,0x8f39b3dfd82f4374ULL},
            {0x485c950,32,0x4a42c757f42524ddULL}};
        for(const auto& p:native){const auto code=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
            if(!original_players::Readable(code,p.bytes))return Fail(4681,"host_child_ammo_original_component_profile_unreadable");
            uint64_t digest=0xcbf29ce484222325ULL;for(size_t n=0;n<p.bytes;++n)digest=(digest^code[n])*0x100000001b3ULL;
            if(digest!=p.fnv)return Fail(4682,"host_child_ammo_original_component_profile_mismatch");}
        state.subequippableProfilesVerified=true;
    }
    auto type=engine::Class(0x63813f0);original_jett_priming::PropertyInfo classField{},actorField{};
    if(!component||Read<void*>(component,0x10)!=type||Read<void*>(component,0xc8)!=ability
       ||original_jett_priming::FieldOffset(type,L"SubequippableClass",8,&classField)!=0xe0
       ||classField.arrayDim!=1||classField.elementSize!=8
       ||original_jett_priming::FieldOffset(type,L"Subequippable",8,&actorField)!=0xf8
       ||actorField.arrayDim!=1||actorField.elementSize!=8
       ||(Read<uint64_t>(actorField.field,0x38)&0x100000020ULL)!=0x100000020ULL
       ||(Read<uint64_t>(Read<void*>(actorField.field,8),0x18)&0x8001ULL)!=0x8001ULL
       ||Read<void*>(actorField.field,0x78)!=engine::Class(0x645c660))
        return Fail(4683,"host_child_ammo_exact_original_component_property_schema_mismatch");
    AuthoredSubequippableProof proof;proof.component=component;proof.ability=ability;
    proof.configuredClass=Read<void*>(component,0xe0);proof.defaults=Read<void*>(proof.configuredClass,0x120);
    if(!original_players::IsA(proof.configuredClass,0x1af0eb0)
       ||!original_players::Derived(proof.configuredClass,engine::Class(0x645c660))
       ||!original_players::IsA(proof.defaults,0x645c660)||Read<void*>(proof.defaults,0x10)!=proof.configuredClass)
        return Fail(4684,"host_child_ammo_original_configured_class_or_defaults_invalid");
    const std::array<void*,4> objects{proof.component,proof.ability,proof.configuredClass,proof.defaults};
    for(size_t n=0;n<objects.size();++n)if(!original_inventory_slots::CaptureIdentity(objects[n],proof.identities[n])
        ||!Live(objects[n],proof.identities[n]))return Fail(4685,"host_child_ammo_original_proof_weak_identity_missing");
    if(!SubEquippableContext(pawn,inventory,item,proof)
       ||local_server_item_ownership::ActorWorld(ability)!=world)
        return Fail(4686,"host_child_ammo_actual_source_component_or_slot_reference_mismatch");
    auto ammo=original_weapon::Component(item,AmmoClass),firing=original_weapon::Component(item,0x684d4c0);
    if(!original_players::IsA(ammo,AmmoClass)||Read<void*>(ammo,0xc8)!=item
       ||!original_players::IsA(firing,0x684d4c0)||Read<void*>(firing,0xc8)!=item)
        return Fail(4687,"host_child_ammo_requires_actual_original_ammo_and_firing_components_nonballistic_contract_unsupported");
    auto slot=Read<void*>(inventory,0x1b8+13*8);
    return PrepareWeaponAmmoImpl(world,pc,ps,pawn,inventory,item,epoch,slot,
        local_server_item_ownership::Source::PublishedSlot,&proof);
}
inline bool PrepareAuthoredSubequippableAmmo(void* world,void* pc,void* ps,void* pawn,void* inventory,
                                            void* item,uint32_t epoch,void* component,void* ability){
    __try{return PrepareAuthoredSubequippableAmmoImpl(world,pc,ps,pawn,inventory,item,epoch,component,ability);}
    __except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool RetireWeaponAmmo(void* item){__try{return RetireWeaponAmmoImpl(item);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool PollWeaponAmmo(void* item){__try{return PollWeaponAmmoImpl(item);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool PollPlayerWeaponAmmo(void* pawn){__try{return PollPlayerWeaponAmmoImpl(pawn);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline void RetirePlayer(void* pawn){for(auto& r:records)if(r.used&&r.pawn==pawn){r={};++state.retired;}}
inline void WriteRoute(FILE* f,const ComponentRoute& d){
    const auto b=[](bool x){return x?"true":"false";};
    std::fprintf(f,"{\"component\":\"%p\",\"owner\":\"%p\",\"flags_a0\":%u,\"replicates\":%s,\"uses_registered_list\":%s,\"arrays_valid\":%s,\"replicated_count\":%d,\"registered_count\":%d,\"replicated_matches\":%d,\"registered_matches\":%d,\"registered_condition\":%d,\"registered_identity_valid\":%s,\"condition_supported\":%s,\"native_lifetime_rva\":%llu,\"native_lifetime_verified\":%s,\"ready\":%s}",
        d.component,d.owner,unsigned(d.flagsA0),b(d.replicates),b(d.usesRegisteredList),b(d.arraysValid),d.replicatedCount,d.registeredCount,d.replicatedMatches,d.registeredMatches,d.condition,b(d.registeredIdentityValid),b(d.conditionSupported),static_cast<unsigned long long>(d.lifetimeRva),b(d.lifetimeVerified),b(d.ready));
}
inline void WriteScalarTracker(FILE* f,const NativeScalarTracker& w){
    const auto b=[](bool x){return x?"true":"false";};
    std::fprintf(f,"{\"mode\":\"original_native_scalar_compare_history\",\"stage\":\"%s\",\"prepared\":%s,\"deferred\":%s,\"observed_native_replication_frame\":%u,\"native_history_witnessed\":%s,\"component\":\"%p\",\"channel\":\"%p\",\"replicator\":\"%p\",\"layout\":\"%p\",\"send_state\":\"%p\",\"changelist_manager\":\"%p\"",
        w.stage,b(w.prepared),b(w.deferred),w.replicationFrame,b(w.published),w.component,w.channel,w.replicator,w.layout,w.sendState,w.manager);
    std::fprintf(f,",\"connection_mode\":%u,\"native_saved_flags\":%u,\"layout_flags\":%u,\"replicator_flags\":%u,\"rep_index\":%u,\"actual_handle\":%u,\"shadow_before\":%d,\"shadow_after\":%d,\"native_result\":%d,\"compare_before\":%u,\"compare_after\":%u,\"history_start\":%u,\"history_end_before\":%u,\"history_end_after\":%u,\"send_history_before\":%u,\"send_compare_before\":%u,\"history_handles\":%d,\"target_handle_observed\":%s,\"zero_terminator_observed\":%s,\"direct_push_dirty_api_proven\":false,\"network_delivery_verified\":false}",
        unsigned(w.connectionMode),w.nativeFlags,w.layoutFlags,w.replicatorFlags,unsigned(w.repIndex),unsigned(w.handle),w.shadowBefore,w.shadowAfter,w.result,w.compareBefore,w.compareAfter,w.historyStart,w.historyEndBefore,w.historyEndAfter,w.sendHistoryBefore,w.sendCompareBefore,w.historyHandles,b(w.handleObserved),b(w.terminatorObserved));
}
inline void WriteItem(FILE* f,void* item){
    const auto b=[](bool x){return x?"true":"false";};const auto r=FindCached(item);
    if(!r){std::fputs("null",f);return;}
    std::fprintf(f,"{\"stage\":\"%s\",\"registration\":%llu,\"source_epoch\":%u,\"ready\":%s,\"resource_ready\":%s,\"tuning_ready\":%s,\"blueprint_resource_uses_own_native_mechanism\":%s,\"native_amount\":%d,\"authority_before\":%d,\"authority_after\":%d,\"resource_publications\":%llu,\"map_count\":%d,\"array_before\":%d,\"array_after\":%d,\"native_array_adds\":%llu,\"native_repnotify_invoked\":%s,\"source_map_preserved\":%s,\"delivery_verified\":false,\"resource_route\":",
        r->stage,static_cast<unsigned long long>(r->registration),r->sourceEpoch,b(r->ready),b(r->resourceReady),b(r->tuningReady),b(r->blueprintResource),r->nativeAmount,r->authorityBefore,r->authorityAfter,static_cast<unsigned long long>(r->resourcePublications),r->mapCount,r->arrayBefore,r->arrayAfter,static_cast<unsigned long long>(r->arrayAdds),b(r->nativeOnRep),b(r->mapPreserved));
    WriteRoute(f,r->resourceRoute);std::fputs(",\"tuning_route\":",f);WriteRoute(f,r->tuningRoute);
    std::fprintf(f,",\"resource_native_push_policy\":%s,\"tuning_native_push_policy\":%s,\"native_channel_snapshot_valid\":%s,\"native_actor_channel_observed\":%s,\"native_scalar_retry_waiting\":%s,\"native_scalar_retry_after_frame\":%u,\"post_initial_push_dirty_publication_verified\":false,\"native_scalar_tracker\":",
        b(r->resourcePushBased),b(r->tuningPushBased),b(r->channelSnapshotValid),b(r->channelObserved),b(r->scalarRetryWaiting),r->scalarRetryFrame);
    WriteScalarTracker(f,r->resourceTracker);std::fputc('}',f);
}
inline void WriteAmmoObservation(FILE* f,const AmmoObservation& d){
    const auto b=[](bool x){return x?"true":"false";};
    std::fprintf(f,"{\"component\":\"%p\",\"authored_initial\":%d,\"native_maximum\":%d,\"native_before\":%d,\"native_after\":%d,\"authority_before\":%d,\"authority_after\":%d,\"metadata_verified\":%s,\"native_methods_verified\":%s,\"flags_c2\":%u,\"in_native_begin_play\":%u,\"original_replenish_called\":%s,\"original_replenish_result\":%d,\"authority_record_prepared\":%s,\"component_route\":",
        d.component,d.authoredInitial,d.maximum,d.before,d.after,d.authorityBefore,d.authorityAfter,b(d.metadataVerified),b(d.nativeMethodsVerified),unsigned(d.flagsC2),unsigned(d.inNativeBeginPlay),b(d.replenished),d.nativeReplenishResult,b(d.published));
    WriteRoute(f,d.route);
    std::fprintf(f,",\"ongoing\":{\"observed\":%s,\"samples\":%llu,\"native_changes\":%llu,\"authority_publications\":%llu,\"last_native_amount\":%d,\"last_native_maximum\":%d,\"last_authority_before\":%d,\"last_authority_after\":%d,\"host_fields_synchronized\":%s,\"delivery_verified\":false},\"native_scalar_tracker\":",
        b(d.ongoingObserved),static_cast<unsigned long long>(d.ongoingSamples),static_cast<unsigned long long>(d.nativeChanges),static_cast<unsigned long long>(d.ongoingPublications),d.currentNative,d.currentMaximum,d.currentAuthorityBefore,d.currentAuthorityAfter,b(d.ongoingSynchronized));
    WriteScalarTracker(f,d.tracker);std::fputc('}',f);
}
inline void WriteSummary(FILE* f){
    std::fprintf(f,"{\"stage\":\"%s\",\"error\":%u,\"profiles_verified\":%s,\"restart_required\":%s,\"registrations\":%llu,\"resource_publications\":%llu,\"native_array_adds\":%llu,\"component_native_registration_calls\":%llu,\"exception_rva\":%llu,\"delivery_verified\":false",
        state.stage,state.error,state.profilesVerified?"true":"false",state.restartRequired?"true":"false",static_cast<unsigned long long>(state.registrations),static_cast<unsigned long long>(state.resourcePublications),static_cast<unsigned long long>(state.nativeArrayAdds),static_cast<unsigned long long>(state.componentRegistrations),static_cast<unsigned long long>(state.exceptionRva));
    std::fprintf(f,",\"ammo_profiles_verified\":%s,\"weapon_ammo_initializations\":%llu,\"weapon_ammo_native_replenishes\":%llu,\"weapon_ammo_authority_publications\":%llu,\"weapon_ammo_nonhuman_unchanged\":%llu,\"weapon_ammo_nonballistic_unchanged\":%llu,\"weapon_ammo_retired\":%llu,\"weapon_ammo\":[",
        state.ammoProfilesVerified?"true":"false",static_cast<unsigned long long>(state.ammoInitializations),static_cast<unsigned long long>(state.ammoNativeReplenishes),static_cast<unsigned long long>(state.ammoAuthorityPublications),static_cast<unsigned long long>(state.ammoSkippedNonhuman),static_cast<unsigned long long>(state.ammoSkippedNonballistic),static_cast<unsigned long long>(state.ammoRetired));
    bool comma=false;for(const auto& r:records)if(r.used&&r.weaponAmmo){
        if(comma)std::fputc(',',f);comma=true;
        std::fprintf(f,"{\"weapon\":\"%p\",\"pawn\":\"%p\",\"source_epoch\":%u,\"registration\":%llu,\"slot_type\":%u,\"pending_grant_at_last_observation\":%s,\"initial_attempted\":%s,\"initial_records_prepared\":%s,\"delivery_verified\":false,\"ongoing_stage\":\"%s\",\"ongoing_ready\":%s,\"ongoing_polls\":%llu,\"ongoing_deferrals\":%llu,\"initial_resource_push_policy\":%s,\"current_resource_push_policy_known\":%s,\"last_resource_push_policy\":%s,\"ongoing_push_policy_samples\":%llu,\"post_initial_push_dirty_publication_verified\":false,\"magazine\":",
            r.item,r.pawn,r.sourceEpoch,static_cast<unsigned long long>(r.registration),unsigned(r.slotType),r.pendingGrant?"true":"false",r.ammoInitialAttempted?"true":"false",r.ammoInitialPrepared?"true":"false",r.stage,r.ammoOngoingReady?"true":"false",static_cast<unsigned long long>(r.ammoOngoingPolls),static_cast<unsigned long long>(r.ammoOngoingDeferrals),r.resourcePushBased?"true":"false",r.ammoOngoingPolicySamples?"true":"false",r.currentResourcePushBased?"true":"false",static_cast<unsigned long long>(r.ammoOngoingPolicySamples));
        WriteAmmoObservation(f,r.magazineAmmo);std::fputs(",\"reserve\":",f);WriteAmmoObservation(f,r.reserveAmmo);
        std::fprintf(f,",\"npc_index\":%zu,\"npc_authority_ammo\":%s,\"npc_original_remote_role\":%u",r.npcIndex,r.npcIndex?"true":"false",unsigned(r.npcRemoteRole));
        std::fprintf(f,",\"authored_subequippable\":%s,\"authored_component\":\"%p\",\"authored_ability\":\"%p\",\"native_scalar_retry_waiting\":%s,\"native_scalar_retry_after_frame\":%u}",
            r.authoredSubequippable?"true":"false",r.subequippableProof.component,r.subequippableProof.ability,r.scalarRetryWaiting?"true":"false",r.scalarRetryFrame);
    }
    std::fprintf(f,"],\"ammo_ongoing_profiles_verified\":%s,\"weapon_ammo_ongoing_polls\":%llu,\"weapon_ammo_ongoing_deferrals\":%llu,\"weapon_ammo_ongoing_magazine_publications\":%llu,\"weapon_ammo_ongoing_reserve_publications\":%llu,\"ongoing_delivery_verified\":false",
        state.ammoOngoingProfilesVerified?"true":"false",static_cast<unsigned long long>(state.ammoOngoingPolls),static_cast<unsigned long long>(state.ammoOngoingDeferrals),static_cast<unsigned long long>(state.ammoOngoingMagazinePublications),static_cast<unsigned long long>(state.ammoOngoingReservePublications));
    std::fprintf(f,",\"native_scalar_tracker_profiles_verified\":%s,\"native_scalar_tracker_preparations\":%llu,\"native_scalar_tracker_calls\":%llu,\"native_scalar_tracker_history_witnesses\":%llu,\"native_scalar_tracker_failures\":%llu,\"native_scalar_tracker_deferrals\":%llu,\"native_scalar_tracker_delivery_verified\":false}",
        state.scalarTrackerProfilesVerified?"true":"false",static_cast<unsigned long long>(state.scalarTrackerPreparations),static_cast<unsigned long long>(state.scalarTrackerCalls),static_cast<unsigned long long>(state.scalarTrackerHistoryWitnesses),static_cast<unsigned long long>(state.scalarTrackerFailures),static_cast<unsigned long long>(state.scalarTrackerDeferrals));
}
} // namespace local_server_resource_authority
