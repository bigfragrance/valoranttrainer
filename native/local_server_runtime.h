#pragma once
#include "local_server_control.h"
#include "local_server_equipment.h"
#include "local_server_abilities.h"
#include "local_server_match.h"
#include "local_server_spectator.h"
#include "local_server_client.h"
#include "local_server_names.h"
#include "local_server_character_stream.h"
#include "local_server_npc_input.h"
#include "local_server_input.h"
#include "local_server_game_start.h"
#include "local_server_combat.h"
#include "local_server_pickups.h"
#include "local_server_cosmetics.h"
#include "local_server_match_end.h"
#include "local_server_starting_equipment.h"
#include "local_server_hero_abilities.h"
#include "local_server_hero_authority.h"
#include "local_server_ability_damage_integration.h"
#include "local_server_hero_resource_init.h"
#include "local_server_hero_ultimate_authority.h"
#include "local_server_hero_event_authority.h"
#include "local_server_hero_skill_readback.h"
#include "local_server_phoenix_c_native_observer.h"
#include "local_server_phoenix_c_result_observer.h"
#include "local_server_phoenix_c_throw_result.h"
#include "local_server_phoenix_c_actor_gate.h"
#include "local_server_phoenix_c_fire_gate_readback.h"
#include "local_server_phoenix_c_state_counter.h"
#include "local_server_effect_authority.h"
#include "local_server_effect_schema_probe.h"
#include "local_server_authority_clock_probe.h"
#include "local_server_hero_selection.h"
#include "local_server_ui_vm_probe.h"
#include "local_server_heal_observer.h"
#include "local_server_armor.h"
#include "armor_poll_performance_json.h"
#include "local_server_humanlike_bots.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

// Session assembly for the separate native loopback host and actual client.
// Root owns login/travel/listen, ActorChannels and the human authority input
// queue. This module never creates player zero or transfers a LocalPlayer;
// TickHost does not call local_server_input::Poll or simulate human physics.
// Integration and evidence boundaries: work/local-server-playable/runtime-assembly.
namespace local_server_runtime {
using Command=training_control::Command;
using Result=training_control::Result;
using Guid=local_server_equipment::Guid;
using Weak=original_inventory_slots::WeakIdentity;
using Vec=engine::Vec;
using Request=local_server_equipment::Request;
inline constexpr size_t MaxPlayers=original_players::MaxPlayers,RequestCapacity=32;
inline constexpr size_t ControllerTableEntries=623,PurchaseSlot=0x1330/8;
inline constexpr uintptr_t ControllerTableRva=0xb50b540,PurchaseLeafRva=0x18a3a00;
inline constexpr uintptr_t ActorClassGetter=0x3683900,AresItemClassGetter=0x645c570;
inline constexpr size_t MaxRegistryActors=MaxPlayers*(local_server_equipment::MaxReferences+8)+1024;
static_assert(PurchaseSlot<ControllerTableEntries&&sizeof(Request)==72);

struct Hooks {
    void* (*spawnHuman)(void* pc,void* ps,const Vec& feet,double yaw)=nullptr;
    void (*requestStop)()=nullptr;
    bool (*beforeRetire)(size_t index,void* pawn,uint32_t epoch)=nullptr;
    void (*actorSpawned)(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch,bool human)=nullptr;
    void (*matchEnded)(int32_t winner,local_server_match::EndReason reason)=nullptr;
    bool (*canDamage)(size_t source,size_t victim)=nullptr;
    // Root currently initializes/polls presentation after BindClientHero. Keep
    // false in that integration; true explicitly transfers those calls here.
    bool manageClientPresentation=false;
    bool includeProjectileWorldActors=false;
    uint32_t port=0;
    bool (*isObserver)(size_t index,void* pc,void* ps)=nullptr;
};
struct Loadout {
    bool used=false,human=false,complete=false,restoreSelection=false;
    void* pawn=nullptr;uint32_t epoch=0,waitingSequence=0;
    unsigned step=0;Guid held{};
};
struct ControllerHook {
    void* controller=nullptr;Weak identity{};
    void **parent=nullptr,**allocation=nullptr,**table=nullptr;
    bool installed=false,profileVerified=false,otherSlotsPreserved=false,restored=false;
    uint64_t purchases=0,lastAudit=0;
};
struct RetiredLife {
    bool used=false,heroDamageRetired=false,effectsRetired=false,combatRetired=false,npcRetired=false,senderRetired=false,parentRetired=false,heroAuthorityRetired=false,cosmeticsRetired=false,abilitiesRetired=false,heroAbilitiesRetired=false,startingRetired=false,equipmentCaptured=false,armorRetired=false,equipmentRetired=false,complete=false;
    void* pawn=nullptr;uint32_t epoch=0;Weak identity{};
};
struct RegistryReference {
    bool observed=false,fatal=false;int32_t player=-1,offset=-1,slot=-1,element=-1;
    void *object=nullptr,*type=nullptr;uint32_t className[3]{};uint8_t role=0;
    const char* source="none";const char* reason="none";
};
struct NativeChannelObservation {
    void *actor=nullptr,*channel=nullptr;
    uint8_t flags=0,openFlags=0;
    bool found=false,openAck=false;
};
struct NativeChannelSnapshot {
    bool valid=false;size_t count=0;void* connection=nullptr;
    std::array<NativeChannelObservation,512> actors{};
};
struct InventoryDependencyObservation {
    void *inventory=nullptr,*previous=nullptr,*replicatedCurrent=nullptr,*correctionCurrent=nullptr;
    uint8_t correctionState=0;bool correctionPublished=false,initialCorrectionProcessed=false;
    local_server_equipment::NetworkedMovementTimestamp changeTimestamp{};
    NativeChannelObservation current{},desired{},candidate{};
};
struct EquipmentGraphObservation {
    bool used=false,complete=false,ownership=false;
    size_t slots=0,actors=0,accepted=0;unsigned phase=0,error=0;
    const char* stage="not_collected";
    InventoryDependencyObservation dependencies{};
};
enum class AdmittedConnectionObservation:uint8_t {NotApplicable,Open,Closed,Failed};
struct ConnectionObservation {
    uint64_t calls=0,skips=0,open=0,closed=0,failures=0,sampledAtMs=0;
    uint64_t nativeStateCloses=0,retiredPeerObservations=0;
    int32_t nativeState=-1,clientCount=-1;
    bool weakVerified=false,stateReadable=false,driverVerified=false,openOwnersVerified=false;
    bool worldDriverVerified=false,peerListReadable=false,peerListShapeVerified=false,retiredWeakEmptyPeers=false,capturedTokenKnown=false;
    const char* stage="local_server_runtime_connection_not_observed";
};
struct State {
    bool loaded=false,host=false,enabled=false,factoryConfigured=false,initialized=false;
    bool humanAdopted=false,ready=false,matchStarted=false,gameStartSubmitted=false;
    bool equipmentReady=false,abilitiesReady=false,combatReady=false,nativeGameplayActive=false,startingEquipmentReady=false;
    bool namesReady=true,graphComplete=true,stopping=false,stopNotified=false,closed=false,disconnected=false;
    bool configurationPending=false,clientEquipmentObserved=false,deathPreprocessPending=false;
    bool heroDamageBoundaryPending=false,closePending=false,closePairUnverified=false;
    bool bootstrapGraphReady=false,bootstrapGraphOnly=false;
    bool nativeEndPending=false,nativeEndSubmitted=false,nativeEndHookForwarded=false;
    int32_t endWinner=-1;local_server_match::EndReason endReason=local_server_match::EndReason::None;
    unsigned error=0;DWORD gameThread=0;
    uint32_t appliedRevision=0,pendingRevision=0,humanEpoch=0;
    uint64_t hostTicks=0,clientTicks=0,acceptedCommands=0,completedRequests=0,retirements=0;
    uint64_t graphVisits=0,foreignControllersSkipped=0,nameDeferrals=0;
    uint64_t deathPreprocessCalls=0,deathRetirementDeferrals=0;
    uint64_t graphDependencyDeferrals=0,graphInvalidReferences=0;
    uint64_t npcUnarmedPollSkips=0,heroDamageBoundaryDeferrals=0,npcObservationOnlyFrames=0,closeAttempts=0;
    size_t graphActors=0,bootstrapGraphActors=0,pendingRequests=0;
    void *world=nullptr,*driver=nullptr,*controller=nullptr,*playerState=nullptr,*pawn=nullptr,*connection=nullptr;
    Weak worldIdentity{},driverIdentity{},controllerIdentity{},stateIdentity{},pawnIdentity{},connectionIdentity{};
    ConnectionObservation connectionObservation{};
    uintptr_t exceptionRva=0;
    const char* stage="local_server_runtime_not_loaded";
    const char* commandStage="none";
    const char* registryStage="local_server_runtime_registry_not_visited";
    const char* nativeEndStage="local_server_runtime_native_match_end_not_requested";
};
inline State state;
inline local_server_player_state_authority::Source heroPlayerStateSource{};
inline bool heroPlayerStateSourceBound=false;
struct InitialHumanSpawn {
    uint64_t requests=0,mapDeferrals=0;
    bool selected=false,mapReady=false,originalMapAnchor=false,preservedBootstrap=false;
    Vec bootstrapCenter{},feet{},center{},rotation{};
    void* pawnClass=nullptr;
    const char* stage="local_server_runtime_initial_human_spawn_not_requested";
};
inline InitialHumanSpawn initialHumanSpawn{};
inline Hooks hooks;
inline ControllerHook controllerHook;
inline std::array<Loadout,MaxPlayers> loadouts{};
inline std::array<RetiredLife,MaxPlayers> retiredLives{};
inline std::array<Request,RequestCapacity> requests{};
inline size_t requestHead=0,requestCount=0;
inline bool requestInFlight=false;
inline uint32_t requestSequence=0;
inline Request activeRequest{};
inline Guid preferredHumanWeapon{};
inline bool humanWeaponSelected=false;
inline std::array<void*,MaxRegistryActors> actorScratch{};
inline RegistryReference firstRegistryReference{};
inline std::array<EquipmentGraphObservation,MaxPlayers> equipmentGraphObservations{};
inline local_server_equipment::ClientObservation clientEquipment{};
inline NativeChannelSnapshot channelSnapshot{};
inline InventoryDependencyObservation clientDependencies{};
inline SRWLOCK npcSnapshotLock=SRWLOCK_INIT;
inline std::string npcSnapshotJson="{\"schema_version\":1,\"initialized\":false,\"runtime_acceptance_verified\":false,\"stage\":\"npc_authority_not_initialized\"}";
inline SRWLOCK featureSnapshotLock=SRWLOCK_INIT;
inline std::string effectsSnapshotJson="[]",cosmeticsSnapshotJson="{\"initialized\":false}",endSnapshotJson="{\"accepted\":false,\"native_complete\":false}";
inline std::string armorPerformanceSnapshotJson="{\"sampled\":false}";
inline std::string weaponSnapshotJson="{\"ready\":false,\"stage\":\"local_server_runtime_waiting_original_current_equipment\"}";
inline uint64_t nextNpcSnapshotAt=0,nextFeatureSnapshotAt=0;
inline constexpr uint64_t PairEpoch=1; // One runtime/module owns one pair; respawns do not change it.
inline std::array<local_server_match_end::PlayerTotal,MaxPlayers> endTotals{};
inline all_skins::CatalogValidation skinCatalogValidation{};
inline std::wstring outputDirectory;
inline void* playerControllerClass=nullptr;
inline const State& GetState(){return state;}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Capture(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline bool Thread(){return state.initialized&&!state.closed&&state.gameThread==GetCurrentThreadId()&&original_players::Thread();}
inline bool Fail(unsigned error,const char* why){state.error=error?error:4800;state.stage=why;state.ready=false;return false;}
inline void CaptureNpcSnapshot();
inline void CaptureFeatureSnapshot();
inline bool CosmeticsResult(bool result);
inline bool Practice(){return !state.enabled||local_server_control::Initial().config.mode==static_cast<uint32_t>(training_control::Mode::HoldAngle);}
inline bool JettPractice(){return Practice()&&local_server_hero_selection::SelectionIndex()==0;}
inline bool HeroPractice(){return state.enabled&&Practice()&&local_server_hero_selection::SelectionIndex()!=0;}
inline bool ValidateHeroPlayerStateSource(const local_server_player_state_authority::Source& source){
    if(!Thread()||state.error||!state.host||!HeroPractice()||state.stopping||state.world!=source.world||local_server_match::GetState().ended)return false;
    local_server_hero_abilities::SourceView current{};
    return local_server_hero_authority::ResolveHostSource(source.pawn,current)
        &&local_server_hero_authority::ValidateHostSource(current)&&!current.ability&&!current.pendingGrant
        &&current.world==source.world&&current.controller==source.controller&&current.playerState==source.playerState
        &&current.pawn==source.pawn&&current.sourceEpoch==source.sourceEpoch;
}
inline bool HeroAuthorityFailure(){
    const auto& events=local_server_hero_event_authority::GetState();
    if(events.error)return Fail(events.error,events.stage);
    const auto& ultimate=local_server_hero_ultimate_authority::GetState();
    if(ultimate.error)return Fail(ultimate.error,ultimate.stage);
    const auto& resources=local_server_hero_resource_init::GetState();
    if(resources.error)return Fail(resources.error,resources.stage);
    const auto& publisher=local_server_player_state_authority::GetState();
    if(publisher.error)return Fail(publisher.error,publisher.stage);
    const auto& authority=local_server_hero_authority::GetState();
    return Fail(authority.error?authority.error:4861,authority.stage);
}
inline bool RetireHeroTypedResources(void* pawn,uint32_t epoch){
    local_server_phoenix_c_state_counter::Retire(pawn,epoch);
    local_server_phoenix_c_native_observer::Retire(pawn,epoch);
    local_server_hero_skill_readback::Retire(pawn,epoch);
    if(local_server_hero_event_authority::GetState().initialized&&!local_server_hero_event_authority::GetState().closed
       &&!local_server_hero_event_authority::Retire(pawn,epoch))return false;
    if(local_server_hero_ultimate_authority::GetState().initialized&&!local_server_hero_ultimate_authority::GetState().closed
       &&!local_server_hero_ultimate_authority::Retire(pawn,epoch))return false;
    if(local_server_hero_resource_init::GetState().initialized&&!local_server_hero_resource_init::GetState().closed
       &&!local_server_hero_resource_init::RetirePlayer(pawn,epoch))return false;
    // Keep the exact tuple captured before registration. Admission has already
    // been fenced here; resolving a new source would incorrectly reopen it.
    if(heroPlayerStateSourceBound&&heroPlayerStateSource.pawn==pawn&&heroPlayerStateSource.sourceEpoch==epoch){
        if(!local_server_player_state_authority::Retire(heroPlayerStateSource))return false;
        heroPlayerStateSourceBound=false;heroPlayerStateSource={};
    }
    return true;
}
inline bool CloseHeroTypedResources(){
    if(local_server_hero_event_authority::GetState().initialized&&!local_server_hero_event_authority::GetState().closed
       &&!local_server_hero_event_authority::Close())return false;
    if(local_server_hero_ultimate_authority::GetState().initialized&&!local_server_hero_ultimate_authority::GetState().closed){
        if(heroPlayerStateSourceBound&&!local_server_hero_ultimate_authority::Retire(heroPlayerStateSource.pawn,heroPlayerStateSource.sourceEpoch))return false;
        if(!local_server_hero_ultimate_authority::Close())return false;
    }
    if(local_server_hero_resource_init::GetState().initialized&&!local_server_hero_resource_init::GetState().closed
       &&!local_server_hero_resource_init::Close())return false;
    if(heroPlayerStateSourceBound){
        if(!local_server_player_state_authority::Retire(heroPlayerStateSource))return false;
        heroPlayerStateSourceBound=false;heroPlayerStateSource={};
    }
    return local_server_player_state_authority::Close();
}
// Passive statistics from the existing native ActorChannel snapshot only.
// All native queries run on the owned GT; file/UI writers copy the locked JSON.
struct ItemChannelRow {
    void *actor=nullptr,*channel=nullptr,*owner=nullptr,*pawn=nullptr,*mesh1p=nullptr,*mesh3p=nullptr;
    Weak actorId{},ownerId{},pawnId{},mesh1pId{},mesh3pId{};
    bool actorWeak=false,ownerWeak=false,pawnWeak=false,mesh1pWeak=false,mesh3pWeak=false;
    bool ownerKnown=false,destroyingKnown=false,destroying=false,nameKnown=false,nameMatch=false,closing=false;
    bool mesh1pRegisteredKnown=false,mesh3pRegisteredKnown=false,mesh1pRegistered=false,mesh3pRegistered=false;
    uint64_t actorName=0,meshCacheAtMs=0;
};
struct ItemChannelProbe {
    bool valid=false,profiles=false,metadata=false,driverFields=false;
    void *connection=nullptr,*driver=nullptr,*serverConnection=nullptr,*replicationBackend=nullptr;
    uint64_t sampledAtMs=0,samples=0,driverNameKey=0;
    uint32_t channels=0,items=0,duplicates=0,nonItems=0,nullActors=0;
    uint32_t liveOwners=0,nullOwners=0,invalidOwners=0,destroyingOwners=0;
    uint32_t beingDestroyed=0,invalidActors=0,queries=0,queryFailures=0;
    uint32_t nameMatches=0,nameMismatches=0,nameUnknown=0,closingItems=0;
    uint32_t rowCount=0,truncatedRows=0;bool itemFields=false,meshFields=false;
    std::array<ItemChannelRow,32> rows{};
    uint32_t exceptionCode=0;uintptr_t exceptionRva=0;
    const char* stage="item_channels_not_observed";
};
inline ItemChannelProbe itemChannelProbe{};
inline uint64_t nextItemChannelProbeAt=0;
inline SRWLOCK itemChannelProbeLock=SRWLOCK_INIT;
inline std::string itemChannelProbeJson="{\"stage\":\"item_channels_not_observed\",\"valid\":false}";
inline bool ItemChannelMethod(void*& function,const wchar_t* path,uint16_t bytes,uint32_t flags,uintptr_t exec,void* owner){
    function=engine::Find(path);
    return function&&owner&&Read<void*>(function,0x28)==owner&&Read<uint16_t>(function,0xbe)==bytes
        &&Read<uint32_t>(function,0xb8)==flags&&Read<void*>(function,0xe0)==reinterpret_cast<void*>(engine::image+exec);
}
inline bool ItemChannelQuery(void* object,void* function,void* params){
    const auto dispatch=Read<void*>(Read<void**>(object),0x298);
    if(!object||!function||!dispatch)return false;
    ++itemChannelProbe.queries;
    reinterpret_cast<void(*)(void*,void*,void*)>(dispatch)(object,function,params);return true;
}
// Resolve only already-captured generation tokens; never allocate a serial.
inline bool ItemChannelKnownWeak(void* object,Weak& id,const local_server_remote_render_probe::Snapshot& render){
    if(!object)return false;
    const auto take=[&](void* source,int32_t index,int32_t serial){Weak known{index,serial};
        if(source!=object||index<0||serial<=0||original_inventory_slots::ResolveIdentity(known)!=object)return false;id=known;return true;};
    for(const auto& p:local_server_equipment::players){
        if(take(p.pawn,p.pawnIdentity.index,p.pawnIdentity.serial)
           ||take(p.transaction.candidate,p.transaction.candidateIdentity.index,p.transaction.candidateIdentity.serial)
           ||take(p.transaction.replaced,p.transaction.replacedIdentity.index,p.transaction.replacedIdentity.serial))return true;
        for(const auto& r:p.retired)if(take(r.actor,r.identity.index,r.identity.serial))return true;
    }
    for(const auto& r:retiredLives)if(take(r.pawn,r.identity.index,r.identity.serial))return true;
    for(const auto& p:local_server_match::players)if(take(p.pawn,p.pawnIdentity.index,p.pawnIdentity.serial))return true;
    if(render.world!=state.world||render.connection!=state.connection)return false;
    for(const auto& r:render.rows){
        if(take(r.pawn,r.pawnId.index,r.pawnId.serial))return true;
        for(const auto& m:r.meshes)if(take(m.object,m.identity.index,m.identity.serial))return true;
    }return false;
}
inline void CacheItemChannelProbe(){
    const auto& p=itemChannelProbe;const auto b=[](bool value){return value?"true":"false";};
    local_server_match::Json out;out.Add("{\"stage\":");out.String(p.stage);
    out.Add(",\"scope\":\"existing_native_actor_channels\",\"valid\":%s,\"profiles_verified\":%s,\"metadata_verified\":%s,\"driver_fields_known\":%s,\"sampled_at_ms\":%llu,\"samples\":%llu,\"minimum_interval_ms\":100",
        b(p.valid),b(p.profiles),b(p.metadata),b(p.driverFields),static_cast<unsigned long long>(p.sampledAtMs),static_cast<unsigned long long>(p.samples));
    out.Add(",\"connection\":\"%p\",\"driver\":\"%p\",\"server_connection\":\"%p\",\"replication_backend\":\"%p\",\"ordinary_server_delete_context\":%s,\"driver_native_name_key64\":\"0x%016llx\"",
        p.connection,p.driver,p.serverConnection,p.replicationBackend,b(p.driverFields&&!p.serverConnection&&!p.replicationBackend),static_cast<unsigned long long>(p.driverNameKey));
    const struct {const char* name;uint32_t value;} values[]{
        {"native_channels",p.channels},{"ares_items",p.items},{"duplicate_item_channels",p.duplicates},{"non_item_actors",p.nonItems},{"null_actor_channels",p.nullActors},
        {"live_owner_items",p.liveOwners},{"null_owner_items",p.nullOwners},{"invalid_owner_items",p.invalidOwners},{"destroying_owner_items",p.destroyingOwners},
        {"being_destroyed_items",p.beingDestroyed},{"invalid_channel_actors",p.invalidActors},{"native_query_calls",p.queries},{"query_failures",p.queryFailures},
        {"driver_name_matches",p.nameMatches},{"driver_name_mismatches",p.nameMismatches},{"driver_name_unknown",p.nameUnknown},{"closing_item_channels",p.closingItems},{"exception_code",p.exceptionCode}
    };for(const auto& v:values)out.Add(",\"%s\":%u",v.name,v.value);
    out.Add(",\"exception_rva\":%llu,\"live_owner_semantics\":\"native_IsValid_and_not_IsActorBeingDestroyed\",\"world_scan_calls\":0,\"weak_serial_allocations\":0,\"actor_destroy_calls\":0,\"channel_flag_writes\":0,\"client_delete_verified\":false,\"peer_delete_verified\":false,\"item_field_metadata\":%s,\"mesh_field_metadata\":%s,\"row_limit\":32,\"truncated_rows\":%u,\"rows\":[",static_cast<unsigned long long>(p.exceptionRva),b(p.itemFields),b(p.meshFields),p.truncatedRows);
    for(uint32_t n=0;n<p.rowCount;++n){const auto& r=p.rows[n];
        out.Add("%s{\"actor\":\"%p\",\"channel\":\"%p\",\"owner\":\"%p\",\"my_pawn\":\"%p\",\"mesh_1p\":\"%p\",\"mesh_3p\":\"%p\",\"closing\":%s,\"owner_known\":%s,\"destroying_known\":%s,\"being_destroyed\":%s,\"name_known\":%s,\"driver_name_match\":%s,\"actor_native_name_key64\":\"0x%016llx\"",n?",":"",r.actor,r.channel,r.owner,r.pawn,r.mesh1p,r.mesh3p,b(r.closing),b(r.ownerKnown),b(r.destroyingKnown),b(r.destroying),b(r.nameKnown),b(r.nameMatch),static_cast<unsigned long long>(r.actorName));
        const struct {const char* name;bool known;Weak weak;} ids[]{
            {"actor",r.actorWeak,r.actorId},{"owner",r.ownerWeak,r.ownerId},{"pawn",r.pawnWeak,r.pawnId},{"mesh_1p",r.mesh1pWeak,r.mesh1pId},{"mesh_3p",r.mesh3pWeak,r.mesh3pId}
        };for(const auto& id:ids)out.Add(",\"%s_weak_known\":%s,\"%s_weak\":[%d,%d]",id.name,b(id.known),id.name,id.weak.index,id.weak.serial);
        out.Add(",\"mesh_1p_registered_known\":%s,\"mesh_1p_registered\":%s,\"mesh_3p_registered_known\":%s,\"mesh_3p_registered\":%s,\"mesh_registered_cache_at_ms\":%llu}",b(r.mesh1pRegisteredKnown),b(r.mesh1pRegistered),b(r.mesh3pRegisteredKnown),b(r.mesh3pRegistered),static_cast<unsigned long long>(r.meshCacheAtMs));
    }out.Add("]}");
    AcquireSRWLockExclusive(&itemChannelProbeLock);itemChannelProbeJson.swap(out.value);ReleaseSRWLockExclusive(&itemChannelProbeLock);
}
inline void ObserveItemChannelProbeImpl(){
    const uint64_t previousSamples=itemChannelProbe.samples;itemChannelProbe={};auto& p=itemChannelProbe;
    p.sampledAtMs=GetTickCount64();p.samples=previousSamples+1;p.connection=channelSnapshot.connection;p.channels=static_cast<uint32_t>(channelSnapshot.count);
    if(!channelSnapshot.valid){p.stage="item_channels_native_snapshot_invalid";CacheItemChannelProbe();return;}
    const struct {uintptr_t rva;size_t bytes;uint64_t fnv;} profiles[]{
        {0x3eea190,37,0x31e9421eb0765366ULL},{0x3ee99c0,32,0xe01e3ed93850b113ULL},{0x45d72b0,258,0xfc87a27a1c1e4f64ULL},
        {0x47465b0,61,0x1c7d9331dcdac053ULL},{0x47387b0,12,0x94d91abb077040c3ULL}
    };
    for(const auto& profile:profiles){const auto code=reinterpret_cast<const uint8_t*>(engine::image+profile.rva);uint64_t h=14695981039346656037ULL;
        if(!original_players::Readable(code,profile.bytes)){p.stage="item_channels_original_query_profile_unreadable";CacheItemChannelProbe();return;}
        for(size_t n=0;n<profile.bytes;++n)h=(h^code[n])*1099511628211ULL;
        if(h!=profile.fnv){p.stage="item_channels_original_query_profile_changed";CacheItemChannelProbe();return;}
    }p.profiles=true;
    p.driver=state.driver;
    p.driverFields=p.driver&&p.connection&&Read<void*>(p.connection,0x60)==p.driver
        &&Read<void*>(state.world,0x40)==p.driver&&Read<void*>(p.driver,0x178)==state.world
        &&original_players::Read(p.driver,0xc0,p.serverConnection)&&original_players::Read(p.driver,0x7d0,p.replicationBackend)
        &&original_players::Read(p.driver,0x1d0,p.driverNameKey);
    void *beingDestroyed=nullptr,*getOwner=nullptr,*isValid=nullptr;
    const auto actorClass=engine::Find(L"/Script/Engine.Actor"),libraryClass=engine::Find(L"/Script/Engine.KismetSystemLibrary");
    const auto library=Read<void*>(libraryClass,0x120);
    if(!library||Read<void*>(library,0x10)!=libraryClass||!(Read<uint32_t>(library,8)&0x10)
       ||!ItemChannelMethod(beingDestroyed,L"/Script/Engine.Actor.IsActorBeingDestroyed",1,0x54020401,0x3eea190,actorClass)
       ||!ItemChannelMethod(getOwner,L"/Script/Engine.Actor.GetOwner",8,0x54020403,0x3ee99c0,actorClass)
       ||!ItemChannelMethod(isValid,L"/Script/Engine.KismetSystemLibrary.IsValid",9,0x14022403,0x45d72b0,libraryClass)){
        p.stage="item_channels_original_query_metadata_or_cdo_unavailable";CacheItemChannelProbe();return;
    }
    namespace field=local_server_remote_timeline_probe;
    if(!field::Field(beingDestroyed,L"ReturnValue",0,1)||!field::Field(getOwner,L"ReturnValue",0,8)
       ||!field::Field(isValid,L"Object",0,8)||!field::Field(isValid,L"ReturnValue",8,1)){
        p.stage="item_channels_original_query_parameter_layout_unknown";CacheItemChannelProbe();return;
    }p.metadata=true;
    const auto itemClass=engine::Find(L"/Script/ShooterGame.AresItem"),equippableClass=engine::Find(L"/Script/ShooterGame.AresEquippable");
    p.itemFields=field::StrongField(itemClass,L"MyPawn",0x4a8);
    p.meshFields=field::StrongField(equippableClass,L"Mesh1P",0xde0)&&field::StrongField(equippableClass,L"Mesh3P",0xdf0);
    const auto render=local_server_remote_render_probe::GetSnapshot();
    struct ValidParams {void* object=nullptr;uint8_t value=0;uint8_t padding[7]{};};static_assert(sizeof(ValidParams)==16&&offsetof(ValidParams,value)==8);
    std::array<void*,512> seen{};size_t seenCount=0;
    for(size_t n=0;n<channelSnapshot.count;++n){const auto& entry=channelSnapshot.actors[n];auto actor=entry.actor;
        if(!actor){++p.nullActors;continue;}
        ValidParams itemValid{actor,0,{}};
        if(!ItemChannelQuery(library,isValid,&itemValid)||itemValid.value>1){++p.queryFailures;continue;}
        if(!itemValid.value){++p.invalidActors;continue;}
        if(!original_players::IsA(actor,AresItemClassGetter)){++p.nonItems;continue;}
        bool duplicate=false;for(size_t k=0;k<seenCount;++k)if(seen[k]==actor){duplicate=true;break;}
        if(duplicate){++p.duplicates;continue;}seen[seenCount++]=actor;++p.items;
        ItemChannelRow* row=nullptr;
        if(p.rowCount<p.rows.size()){
            row=&p.rows[p.rowCount++];row->actor=actor;row->channel=entry.channel;row->closing=(entry.flags&2)!=0;
            row->actorWeak=ItemChannelKnownWeak(actor,row->actorId,render);
            if(p.itemFields){row->pawn=Read<void*>(actor,0x4a8);row->pawnWeak=ItemChannelKnownWeak(row->pawn,row->pawnId,render);}
            if(p.meshFields&&original_players::IsA(actor,0x645c660)){
                row->mesh1p=Read<void*>(actor,0xde0);row->mesh3p=Read<void*>(actor,0xdf0);
                row->mesh1pWeak=ItemChannelKnownWeak(row->mesh1p,row->mesh1pId,render);
                row->mesh3pWeak=ItemChannelKnownWeak(row->mesh3p,row->mesh3pId,render);
                if(render.world==state.world&&render.connection==state.connection){
                    for(const auto& r:render.rows)for(const auto& m:r.meshes)if(m.owner==actor&&m.typed&&m.live){
                        if(row->mesh1pWeak&&m.object==row->mesh1p&&m.identity.index==row->mesh1pId.index&&m.identity.serial==row->mesh1pId.serial){row->mesh1pRegisteredKnown=true;row->mesh1pRegistered=m.registered;row->meshCacheAtMs=render.atMs;}
                        if(row->mesh3pWeak&&m.object==row->mesh3p&&m.identity.index==row->mesh3pId.index&&m.identity.serial==row->mesh3pId.serial){row->mesh3pRegisteredKnown=true;row->mesh3pRegistered=m.registered;row->meshCacheAtMs=render.atMs;}
                    }
                }
            }
        }else ++p.truncatedRows;
        if(entry.flags&2)++p.closingItems;
        uint64_t actorName=0;if(p.driverFields&&original_players::Read(actor,0x218,actorName)){
            if(actorName==p.driverNameKey)++p.nameMatches;else ++p.nameMismatches;
            if(row){row->nameKnown=true;row->nameMatch=actorName==p.driverNameKey;row->actorName=actorName;}
        }else ++p.nameUnknown;
        uint8_t destroying=255;
        if(ItemChannelQuery(actor,beingDestroyed,&destroying)&&destroying<=1){if(destroying)++p.beingDestroyed;if(row){row->destroyingKnown=true;row->destroying=destroying!=0;}}else ++p.queryFailures;
        void* owner=nullptr;if(!ItemChannelQuery(actor,getOwner,&owner)){++p.queryFailures;continue;}
        if(row){row->ownerKnown=true;row->owner=owner;row->ownerWeak=ItemChannelKnownWeak(owner,row->ownerId,render);}
        if(!owner){++p.nullOwners;continue;}
        ValidParams ownerValid{owner,0,{}};
        if(!ItemChannelQuery(library,isValid,&ownerValid)||ownerValid.value>1){++p.queryFailures;continue;}
        if(!ownerValid.value||!original_players::IsA(owner,ActorClassGetter)){++p.invalidOwners;continue;}
        uint8_t ownerDestroying=255;
        if(!ItemChannelQuery(owner,beingDestroyed,&ownerDestroying)||ownerDestroying>1){++p.queryFailures;continue;}
        if(ownerDestroying)++p.destroyingOwners;else ++p.liveOwners;
    }
    p.valid=p.queryFailures==0;p.stage=p.valid?"item_channels_native_queries_cached_on_game_thread":"item_channels_native_queries_partially_unknown";CacheItemChannelProbe();
}
inline int ItemChannelProbeException(EXCEPTION_POINTERS* exception){
    itemChannelProbe.valid=false;
    if(exception&&exception->ExceptionRecord){
        itemChannelProbe.exceptionCode=exception->ExceptionRecord->ExceptionCode;
        const auto at=reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress);
        itemChannelProbe.exceptionRva=at>=engine::image?at-engine::image:at;
    }
    itemChannelProbe.stage="item_channels_query_exception_cached_no_gameplay_change";return EXCEPTION_EXECUTE_HANDLER;
}
inline void ObserveItemChannelProbe(){
    if(!Thread())return;const auto now=GetTickCount64();if(now<nextItemChannelProbeAt)return;nextItemChannelProbeAt=now+100;
    __try{ObserveItemChannelProbeImpl();}
    __except(ItemChannelProbeException(GetExceptionInformation())){CacheItemChannelProbe();}
}
inline std::string ItemChannelProbeJson(){
        struct ReadLock {SRWLOCK* lock;explicit ReadLock(SRWLOCK* p):lock(p){AcquireSRWLockShared(lock);}~ReadLock(){ReleaseSRWLockShared(lock);}};
    ReadLock guard(&itemChannelProbeLock);return itemChannelProbeJson;
}
inline void ObserveNativeChannels(){
    channelSnapshot={};auto connection=state.connection;if(!connection)return;
    struct Channels {void** data;int32_t count,capacity;};
    const auto channels=Read<Channels>(connection,0x78);
    if(channels.count<0||channels.count>512||channels.capacity<channels.count
       ||(channels.count&&(!channels.data||!original_players::Readable(channels.data,static_cast<size_t>(channels.count)*8))))return;
    channelSnapshot.connection=connection;
    for(int32_t n=0;n<channels.count;++n){const auto channel=Read<void*>(channels.data,static_cast<size_t>(n)*8);
        if(!channel||Read<void*>(channel,0x30)!=connection||!original_players::IsA(channel,0x3dd3d40))continue;
        auto& observed=channelSnapshot.actors[channelSnapshot.count++];
        observed.actor=Read<void*>(channel,0x78);observed.channel=channel;observed.flags=Read<uint8_t>(channel,0x38);
        observed.openFlags=Read<uint8_t>(channel,0xa0);observed.found=true;observed.openAck=(observed.openFlags&1)!=0;
    }
    const auto after=Read<Channels>(connection,0x78);
    channelSnapshot.valid=after.data==channels.data&&after.count==channels.count&&after.capacity==channels.capacity;
    if(!channelSnapshot.valid)channelSnapshot.count=0;
    ObserveItemChannelProbe();
}
inline NativeChannelObservation ObserveItemChannel(void* actor){
    NativeChannelObservation result{};result.actor=actor;if(!actor||!channelSnapshot.valid)return result;
    for(size_t n=0;n<channelSnapshot.count;++n)if(channelSnapshot.actors[n].actor==actor)return channelSnapshot.actors[n];
    return result;
}
inline InventoryDependencyObservation ObserveInventoryDependencies(void* inventory,void* current,void* desired,void* candidate=nullptr){
    InventoryDependencyObservation result{};result.inventory=inventory;
    result.previous=Read<void*>(inventory,0x278);result.replicatedCurrent=Read<void*>(inventory,0x2a0);
    result.correctionCurrent=Read<void*>(inventory,0x320);result.correctionState=Read<uint8_t>(inventory,0x2fc);
    result.correctionPublished=Read<uint8_t>(inventory,0x30c)!=0;result.initialCorrectionProcessed=Read<uint8_t>(inventory,0x30d)!=0;
    result.changeTimestamp=Read<local_server_equipment::NetworkedMovementTimestamp>(inventory,0x2a8);
    result.current=ObserveItemChannel(current);result.desired=ObserveItemChannel(desired);result.candidate=ObserveItemChannel(candidate);
    return result;
}
inline bool StopRequested(){return state.stopping;}
inline bool SetHooks(const Hooks& callbacks){
    if(state.initialized||state.closed)return false;hooks=callbacks;return true;
}
inline bool PrepareInitialHumanSpawn(void* world,void* pc,void* ps,void* pawnClass,
                                    engine::Transform& transform,Vec& rotation,bool& ready){
    ready=false;auto& observation=initialHumanSpawn;++observation.requests;
    observation.bootstrapCenter=transform.translation;observation.pawnClass=pawnClass;
    if(!state.loaded||!state.host||state.closed||state.error)
        return Fail(4870,"local_server_runtime_initial_human_spawn_requires_loaded_host");
    // A configured practice session needs the same supported initial human
    // anchor as FFA. Its bootstrap DefaultPawn may also be at unsupported zero.
    // Preserve the separate unconfigured lifecycle probe only.
    if(!state.enabled){
        observation.preservedBootstrap=true;observation.center=transform.translation;
        observation.rotation=rotation;observation.stage="local_server_runtime_initial_human_bootstrap_pose_preserved";
        ready=true;return true;
    }
    const auto connection=Read<void*>(pc,0x510);
    if(!Thread()||world!=state.world||!state.matchStarted||state.humanAdopted
       ||!original_players::IsA(pc,original_players::ShooterControllerClass)
       ||!original_players::IsA(ps,original_players::ShooterStateClass)
       ||Read<uint8_t>(pc,0x224)!=3||Read<void*>(pc,0x470)!=ps||Read<void*>(ps,0x210)!=pc||Read<void*>(pc,0x4b0)
       ||!connection||Read<void*>(connection,0x60)!=state.driver
       ||Read<void*>(connection,0x38)!=pc||Read<void*>(connection,0xa0)!=pc||Read<int32_t>(connection,0x13c)!=3)
        return Fail(4871,"local_server_runtime_initial_human_spawn_admitted_context_changed");
    const auto& match=local_server_match::GetState();
    if(!match.started||match.world!=world||match.humanRegistered||local_server_match::Player(0))
        return Fail(4872,"local_server_runtime_initial_human_spawn_requires_unbound_match_slot");
    if(!training_maps::Poll())return Fail(training_maps::State().error,training_maps::State().stage);
    observation.mapReady=training_maps::Ready();
    if(!observation.mapReady){
        ++observation.mapDeferrals;observation.stage="local_server_runtime_initial_human_waiting_original_map_layers";
        return true;
    }
    original_players::CharacterDefaults defaults;
    if(!original_players::ProbeDefaults(local_server_hero_selection::SelectionPath(),defaults)||defaults.type!=pawnClass||defaults.abstractClass)
        return Fail(4873,"local_server_runtime_initial_human_original_hero_dimensions_unavailable");
    deathmatch_rules::SpawnSelection spawn;
    if(!local_server_match::SelectSpawn(0,1,spawn))
        return Fail(local_server_match::GetState().error,local_server_match::GetState().stage);
    if(spawn.basis!=deathmatch_rules::PositionBasis::Feet||!spawn.originalMapAnchor
       ||!original_players::Finite(spawn.position)||!std::isfinite(spawn.yaw))
        return Fail(4874,"local_server_runtime_initial_human_spawn_requires_supported_original_anchor");
    // Use the same authored anchor, native ground/capsule queries and exact
    // root-from-feet convention as the original-class NPC/respawn factories.
    // This is an initial authority spawn, before BeginPlay/possession and the
    // first ActorChannel. No client transform, movement clock or ACK is set.
    transform.translation=spawn.position;transform.translation.z+=defaults.capsuleHalfHeight+2.;
    rotation={0,spawn.yaw,0};
    observation.selected=true;observation.originalMapAnchor=true;observation.feet=spawn.position;
    observation.center=transform.translation;observation.rotation=rotation;
    observation.stage="local_server_runtime_initial_human_supported_original_map_anchor_selected";
    ready=true;return true;
}
inline bool Load(const std::wstring& directory,bool host){
    if(state.closed||state.error)return false;
    if(!local_server_control::LoadInitial(directory,host))
        return Fail(local_server_control::GetState().error,local_server_control::GetState().stage);
    if(state.loaded)return state.host==host&&outputDirectory==local_server_control::outputDirectory;
    outputDirectory=local_server_control::outputDirectory;state.loaded=true;state.host=host;
    state.enabled=local_server_control::Enabled();
    if(!local_server_feedback_ledger::Configure(outputDirectory,host,state.enabled&&!Practice()))
        local_server_feedback_ledger::RejectCommittedRecord("feedback_ledger_session_configuration_rejected");
    state.stage=state.enabled?"local_server_runtime_waiting_native_world":"local_server_runtime_basic_lifecycle_probe";
    return true;
}
inline bool ConfigureFactory(original_players::Options& options){
    if(!state.loaded||state.closed||state.error)return false;
    if(!state.factoryConfigured&&original_players::createdGameState)
        return Fail(4801,"local_server_runtime_factory_must_precede_game_state_creation");
    const bool ffa=state.enabled&&!Practice();
    options.createGameState=state.host;options.createPlayers=false;options.transferLocalPlayer=false;
    options.freeForAll=ffa;
    options.playerStateClassPath=ffa?deathmatch_rules::StateClassPath:original_players::Options{}.playerStateClassPath;
    options.controllerClassPath=ffa?deathmatch_rules::ControllerClassPath:nullptr;
    // Configured practice needs the original shared Blueprint service graph.
    // This selects a native-client candidate; runtime acceptance stays false
    // until Root verifies that actual class/replica/UI lifecycle independently.
    options.gameStateClassPath=ffa?deathmatch_rules::GameStateClassPath
        :state.enabled?L"/Game/BaseGameState.BaseGameState_C":nullptr;
    state.factoryConfigured=true;return true;
}

inline bool HumanOwner(void* pc,void* ps,void* pawn,void* connection){
    return state.host&&original_players::IsA(pc,original_players::ShooterControllerClass)
        &&original_players::IsA(ps,original_players::ShooterStateClass)
        &&original_players::IsA(pawn,original_players::ShooterCharacterClass)
        &&original_players::IsA(connection,0x3dea9c0)
        &&Read<uint8_t>(pc,0x224)==3&&Read<uint8_t>(pawn,0x224)==3
        &&Read<void*>(pc,0x470)==ps&&Read<void*>(ps,0x210)==pc
        &&Read<void*>(pc,0x4b0)==pawn&&Read<void*>(pawn,0x4a0)==pc&&Read<void*>(pawn,0x488)==ps
        &&Read<void*>(pc,0x510)==connection&&Read<void*>(connection,0x38)==pc&&Read<void*>(connection,0xa0)==pc
        &&Read<void*>(connection,0x60)==state.driver&&Read<uint8_t>(connection,0x13c)==3
        &&Read<uint8_t>(connection,0x1d8)==3;
}
inline void Purchase(void* pc,void* weaponClass,uint8_t targetSlot,uint8_t purchaseType){
    // Exact original RCX/RDX/R8b/R9b authority ABI; queue-only. The original
    // client ServerPurchase RPC and all of its callspace stay unmodified.
    if(!Thread()||!state.host||state.stopping||!controllerHook.installed
       ||pc!=controllerHook.controller||!Live(pc,controllerHook.identity)
       ||Read<void**>(pc)!=controllerHook.table)return;
    ++controllerHook.purchases;
    const auto p=local_server_equipment::State(0);const auto previous=p?p->nativePurchases:0;
    local_server_equipment::NativePurchase(pc,weaponClass,targetSlot,purchaseType);
    if(p&&p->nativePurchases!=previous&&p->nativePurchasePending){preferredHumanWeapon=p->nativePurchase.weapon;humanWeaponSelected=true;}
}
inline bool AuditControllerHook(){
    auto& h=controllerHook;
    if(!h.installed||!Live(h.controller,h.identity)||Read<void**>(h.controller)!=h.table)
        return Fail(4802,"local_server_runtime_authority_controller_table_owner_changed");
    for(size_t n=0;n<ControllerTableEntries;++n){const auto expected=n==PurchaseSlot?reinterpret_cast<void*>(&Purchase):Read<void*>(h.parent,n*8);
        if(Read<void*>(h.table,n*8)!=expected)return Fail(4803,"local_server_runtime_unrelated_controller_virtual_changed");}
    if(Read<void*>(h.table-1)!=Read<void*>(h.parent-1))return Fail(4803,"local_server_runtime_controller_rtti_changed");
    h.otherSlotsPreserved=true;h.lastAudit=GetTickCount64();return true;
}
inline bool InstallControllerHook(void* pc){
    auto& h=controllerHook;
    if(h.installed)return h.controller==pc&&AuditControllerHook();
    const auto native=reinterpret_cast<void**>(engine::image+ControllerTableRva);
    const auto leaf=reinterpret_cast<void*>(engine::image+PurchaseLeafRva);
    auto actual=Read<void**>(pc);auto rpc=engine::Find(L"/Script/ShooterGame.ShooterPlayerController.ServerPurchase");
    if(actual!=native||!original_players::Readable(actual-1,(ControllerTableEntries+1)*8)
       ||Read<void*>(actual,PurchaseSlot*8)!=leaf
       ||Read<uint8_t>(leaf)!=0xc2||Read<uint16_t>(leaf,1)!=0
       ||!rpc||Read<uint16_t>(rpc,0xbe)!=10||(Read<uint32_t>(rpc,0xb8)&0x200040U)!=0x200040U)
        return Fail(4804,"local_server_runtime_original_purchase_leaf_or_full_table_profile_mismatch");
    if(!Capture(pc,h.identity))return Fail(4805,"local_server_runtime_controller_weak_identity_missing");
    auto storage=static_cast<void**>(HeapAlloc(GetProcessHeap(),0,(ControllerTableEntries+1)*8));
    if(!storage)return Fail(4806,"local_server_runtime_controller_table_allocation_failed");
    std::memcpy(storage,actual-1,(ControllerTableEntries+1)*8);
    h.controller=pc;h.parent=actual;h.allocation=storage;h.table=storage+1;
    h.table[PurchaseSlot]=reinterpret_cast<void*>(&Purchase);h.profileVerified=true;
    if(!original_players::Write(pc,0,h.table)||Read<void**>(pc)!=h.table){
        // Retain allocation if an unexpected partial ownership change occurred.
        return Fail(4807,"local_server_runtime_controller_table_install_readback_failed");
    }
    h.installed=true;return AuditControllerHook();
}
inline bool RestoreControllerHook(){
    auto& h=controllerHook;if(!h.installed)return true;
    if(Live(h.controller,h.identity)){
        if(Read<void**>(h.controller)!=h.table)return Fail(4802,"local_server_runtime_authority_controller_table_owner_changed");
        if(!original_players::Write(h.controller,0,h.parent)||Read<void**>(h.controller)!=h.parent)
            return Fail(4808,"local_server_runtime_controller_table_restore_failed");
    }
    h.installed=false;h.restored=true;
    // Keep backing alive until the owned process exits. A late original native
    // callback cannot observe reclaimed table memory during shutdown.
    return true;
}

inline const all_weapons::Definition* Weapon(const char* label){
    for(const auto& d:all_weapons::definitions)if(d.label&&std::strcmp(d.label,label)==0&&d.identityVerified)return &d;
    return nullptr;
}
inline Request Preference(size_t index,const all_weapons::Definition& definition){
    Request request=local_server_equipment::preferences[index][static_cast<size_t>(&definition-all_weapons::definitions)];
    request.weapon=definition.id;request.sequence=0;request.mode=local_server_equipment::Mode::Equip;
    request.reserved[0]=request.reserved[1]=request.reserved[2]=0;return request;
}
inline bool ProbeHumanPublicationPending(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch,bool human){
    namespace probe=local_server_empty_bootstrap_probe;
    if(!probe::armed)return false;
    const auto p=local_server_equipment::State(index);
    probe::core::Scope source{};source.enabled=state.enabled;source.host=state.host;source.freeForAll=local_server_match::FreeForAll()&&!Practice();
    source.human=human;source.index=static_cast<uint32_t>(index);source.epoch=epoch;
    source.world=reinterpret_cast<uintptr_t>(state.world);source.pc=reinterpret_cast<uintptr_t>(pc);source.ps=reinterpret_cast<uintptr_t>(ps);
    source.pawn=reinterpret_cast<uintptr_t>(pawn);source.connection=reinterpret_cast<uintptr_t>(state.connection);source.inventory=p?reinterpret_cast<uintptr_t>(p->inventory):0;
    const auto admitted=local_server_match::Player(0);
    source.linked=human&&index==0&&state.controller==pc&&state.playerState==ps&&state.pawn==pawn&&state.humanEpoch==epoch
        &&admitted&&admitted->present&&admitted->alive&&admitted->human&&admitted->pc==pc&&admitted->ps==ps&&admitted->pawn==pawn&&admitted->epoch==epoch
        &&Read<uint8_t>(pc,0x224)==3&&Read<uint8_t>(pawn,0x224)==3&&Read<uint8_t>(pawn,0xa90)!=0
        &&Read<void*>(pc,0x510)==state.connection&&Read<void*>(state.connection,0x38)==pc&&Read<void*>(state.connection,0xa0)==pc
        &&Read<void*>(pc,0x4b0)==pawn&&Read<void*>(pawn,0x4a0)==pc&&Read<void*>(pc,0x470)==ps&&Read<void*>(pawn,0x488)==ps;
    return probe::PendingHuman(source,p);
}
inline bool RegisterEquipment(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch,bool human){
    if(index>=MaxPlayers||!epoch||human!=(index==0))return Fail(4809,"local_server_runtime_equipment_roster_identity_invalid");
    // Set the original NPC clock origin before any force/equipment can publish
    // native absolute timestamps. Human private input/ACK mapping stays intact.
    if(state.enabled&&state.host&&!human&&!local_server_npc_input::PrepareFreshClock(index,pc,ps,pawn,epoch,human))
        return Fail(local_server_npc_input::GetState().error,local_server_npc_input::GetState().stage);
    // The match has already bound this exact real tuple. Register its effect
    // source before original skill BeginPlay can issue a continuous effect.
    if(state.enabled&&!local_server_effect_authority::RegisterPlayer(index,pc,ps,pawn,epoch)){
        const auto& effects=local_server_effect_authority::GetState();return Fail(effects.error,effects.stage);
    }
    if(!local_server_equipment::RegisterPlayer(index,pc,ps,pawn,epoch)){
        const auto p=local_server_equipment::State(index);
        return Fail(p&&p->error?p->error:local_server_equipment::State().error,p&&p->error?p->stage:local_server_equipment::State().stage);
    }
    // Exact-marker life-one human only. Keep RegisterPlayer/EnsureForPawn baseline intact.
    const bool emptyBootstrapPending=ProbeHumanPublicationPending(index,pc,ps,pawn,epoch,human);
    if(!emptyBootstrapPending&&state.enabled&&!Practice()&&!local_server_armor::RegisterOrEnsure(index,pc,ps,pawn,epoch)){
        const auto& armor=local_server_armor::GetState();return Fail(armor.error?armor.error:5524,armor.stage);
    }
    auto& plan=loadouts[index];
    if(plan.used)return plan.pawn==pawn&&plan.epoch==epoch;
    plan={};plan.used=true;plan.human=human;plan.pawn=pawn;plan.epoch=epoch;
    if(human&&humanWeaponSelected){
        const auto selected=all_weapons::Find(preferredHumanWeapon),vandal=Weapon("Vandal");
        if(selected&&selected->identityVerified&&vandal&&!all_weapons::Same(selected->id,vandal->id)){
            plan.restoreSelection=true;plan.held=selected->id;
        }
    }
    if(!local_server_equipment::SetPurchasesEnabled(index,false))return Fail(4810,"local_server_runtime_initial_loadout_purchase_policy_failed");
    // Skills are a practice-only grant to the actual admitted human. Jett
    // retains its proven priming/dagger path; other selected heroes bind their
    // own original CDO loadout. No skill AI is introduced for NPCs or FFA.
    // NPC decisions use the requested real weapon, without a second skill AI.
    if(!emptyBootstrapPending&&human&&JettPractice()&&!local_server_abilities::RegisterPlayer(pc,ps,pawn,epoch))
        return Fail(local_server_abilities::GetState().error,local_server_abilities::GetState().stage);
    if(!emptyBootstrapPending&&human&&state.enabled&&JettPractice()&&!local_server_starting_equipment::RegisterOrEnsure(state.world,pc,ps,pawn,epoch))
        return Fail(local_server_starting_equipment::GetState().error,local_server_starting_equipment::GetState().stage);
    if(!emptyBootstrapPending&&human&&state.host&&HeroPractice()&&!local_server_hero_authority::PollHost(pawn,true,false)){
        return HeroAuthorityFailure();
    }
    if(!emptyBootstrapPending&&human&&state.host&&HeroPractice()){
        heroPlayerStateSource={state.world,pc,ps,pawn,epoch};heroPlayerStateSourceBound=true;
        // The original skill factories can issue RegisterEvent during FinishSpawn,
        // before the framework has published a slot. Retain only requests from
        // this already match-bound human and its actual original starter CDOs.
        if(!local_server_hero_event_authority::RegisterHuman(pc,ps,pawn,local_server_hero_selection::SelectionIndex(),epoch))
            return HeroAuthorityFailure();
    }
    if(!emptyBootstrapPending&&human&&HeroPractice()&&!local_server_hero_abilities::RegisterPlayer(pc,ps,pawn,local_server_hero_selection::SelectionIndex(),epoch)){
        if(local_server_hero_event_authority::GetState().error)return HeroAuthorityFailure();
        return Fail(local_server_hero_abilities::GetState().error,local_server_hero_abilities::GetState().stage);
    }
    return true;
}
inline bool EquipmentReady(size_t index,void* pawn,uint32_t epoch){
    return index<MaxPlayers&&loadouts[index].used&&loadouts[index].pawn==pawn&&loadouts[index].epoch==epoch
        &&loadouts[index].complete&&local_server_equipment::Ready(index)
        &&(Practice()||local_server_armor::Ready(index,pawn,epoch));
}
inline bool ExternalNpcProducer(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch,void* inputState){
    if(!state.host||!state.enabled||!Thread()||!index||index>=local_server_npc_input::entries.size())return false;
    auto& entry=local_server_npc_input::entries[index];
    return entry.present&&!entry.error&&entry.pc==pc&&entry.ps==ps&&entry.pawn==pawn&&entry.input==inputState
        &&entry.epoch==epoch&&local_server_npc_input::Links(entry);
}
inline bool ExternalNpcClock(const local_server_combat::ExternalNPCClockSource& source){
    if(!ExternalNpcProducer(source.index,source.pc,source.ps,source.pawn,source.epoch,source.inputState))return false;
    // Sample the real producer, without ticking/funding it, before checking
    // its exact current source identity. A Mode tick may have advanced it.
    if(!local_server_npc_time_domain::Observe())return false;
    const auto domain=local_server_npc_time_domain::Player(source.index);
    const auto entry=local_server_npc_input::Player(source.index);
    return domain&&entry&&domain->mapped&&domain->pc==source.pc&&domain->ps==source.ps
        &&domain->pawn==source.pawn&&domain->epoch==source.epoch&&domain->movement==source.movement
        &&domain->prediction==source.prediction&&domain->clock==source.clock
        &&entry->pc==source.pc&&entry->ps==source.ps&&entry->pawn==source.pawn&&entry->epoch==source.epoch
        &&entry->input==source.inputState&&entry->movement==source.movement
        &&entry->prediction==source.prediction&&entry->clock==source.clock
        &&source.step==1&&source.baseOffset>=0&&domain->baseOffset==static_cast<uint32_t>(source.baseOffset)
        &&source.offset==domain->expectedOffset
        &&local_server_npc_time_domain::Validate(source.index,source.pawn,source.movement,source.epoch);
}
inline bool HeroDamageActive(){
    return state.host&&HeroPractice()&&local_server_ability_damage_integration::GetState().initialized;
}
inline bool HeroDamageFault(){
    const auto& composed=local_server_ability_damage_integration::GetState();
    if(composed.error||composed.restartRequired){Fail(composed.error?composed.error:4867,composed.stage);return true;}
    const auto& observer=local_server_ability_damage::GetState();
    if(observer.error||observer.restartRequired){Fail(observer.error?observer.error:4867,observer.stage);return true;}
    return false;
}
inline bool HeroDamageWait(const char* why){
    state.ready=false;state.deathPreprocessPending=true;state.heroDamageBoundaryPending=true;
    ++state.heroDamageBoundaryDeferrals;state.stage=why;return false;
}
inline bool FenceHeroDamageAdmission(){
    const auto& authority=local_server_hero_authority::GetState();
    if(!HeroDamageActive()||!authority.initialized||authority.closed||authority.closing)return true;
    return local_server_hero_authority::PollHost(state.pawn,false,false)||HeroAuthorityFailure();
}
inline bool ExtraHostCloseScopesIdle(){
    const auto& input=local_server_input::GetState();
    const bool active=input.inPoll||input.inReceive
        ||local_server_combat::firearms::observerDepth||local_server_combat::melee::observerDepth
        ||local_server_ability_damage::WrapperCount()||local_server_ability_damage::GetState().processing;
    // A constructed scratch with no active capture is deliberately retained:
    // the existing sender Retire owns its original native destructor. Waiting
    // on that flag here would prevent the very cleanup that clears it.
    if(active){state.stage="local_server_runtime_close_waiting_all_native_capture_and_input_scopes";return false;}
    return true;
}
inline bool HostCloseBoundaryIdle(){
    if(!ExtraHostCloseScopesIdle())return false;
    if(!local_server_npc_input::CleanupBoundaryReady()){
        state.stage="local_server_runtime_close_waiting_native_capture_transaction_cleanup";return false;
    }
    return true;
}
// Complete a real already-armed dispatch/world/flush pair without submitting
// any adapter input, budget, clock step or physics. The source module retains
// all unserved real debt/queues and records only original progress that occurred.
inline bool ObservePendingWorldFrame(double delta){
    if(!state.host||!state.enabled)return true;
    const auto& npc=local_server_npc_input::GetState();
    const auto& sender=local_server_character_stream::GetState();
    if(npc.error||npc.unsupported||sender.error||sender.unsupported){
        if(state.closePending&&npc.initialized&&ExtraHostCloseScopesIdle()
           &&local_server_npc_input::CleanupExecutionScopesIdle()){
            // Only this proven real post-world boundary may abandon an actual
            // fatal transaction after every real native/builder scope left.
            // Its original scratch destructor is owned/profiled by the sender;
            // no completed move, clock step or capture receipt is fabricated.
            if(!local_server_npc_input::CleanupBoundaryReady()
               &&local_server_npc_input::AbortFailedManualTransactions(state.world,state.driver,static_cast<float>(delta)))
                state.closePairUnverified=true;
            if(local_server_npc_input::CleanupBoundaryReady()){
                if(!npc.error&&!npc.unsupported&&npc.frameArmed){
                    // A sender error is not fabricated into an NPC error.
                    // Try its real observation contract. If that actually
                    // fails the NPC context, the same real post-world pair
                    // then qualifies for its explicit unverified abandon.
                    local_server_npc_input::ObserveWorldTickOnly(static_cast<float>(delta));
                }
                if((npc.error||npc.unsupported)
                   &&local_server_npc_input::AbandonFailedWorldFrame(state.world,state.driver,static_cast<float>(delta)))
                    state.closePairUnverified=true;
            }
        }
        CaptureNpcSnapshot();return Fail(npc.error?npc.error:sender.error,npc.error?npc.stage:sender.stage);
    }
    if(!npc.frameArmed)return true;
    const bool observed=local_server_npc_input::ObserveWorldTickOnly(static_cast<float>(delta));
    CaptureNpcSnapshot();if(!observed)return Fail(local_server_npc_input::GetState().error,local_server_npc_input::GetState().stage);
    ++state.npcObservationOnlyFrames;return true;
}
inline bool PollPendingControl(){
    return local_server_control::Poll()||Fail(local_server_control::GetState().error,local_server_control::GetState().stage);
}
inline bool HoldPendingWorldFrame(double delta){
    return ObservePendingWorldFrame(delta)&&PollPendingControl();
}
inline bool PrepareHeroDamageRetirement(size_t index,void* pawn,uint32_t epoch){
    if(!HeroDamageActive())return true;
    if(index>=MaxPlayers||!pawn||!epoch)return Fail(4868,"local_server_runtime_damage_retirement_invalid_tuple");
    auto& retired=retiredLives[index];
    const bool same=retired.used&&retired.pawn==pawn&&retired.epoch==epoch;
    if(same&&retired.heroDamageRetired)return Live(pawn,retired.identity)
        ||Fail(4834,"local_server_runtime_damage_retired_life_identity_expired");
    if(!local_server_ability_damage_integration::ReadyForPlayerRetirement(pawn,epoch)){
        if(HeroDamageFault())return false;
        return HeroDamageWait("local_server_runtime_waiting_real_damage_observer_retirement_boundary");
    }
    if(!same){
        if((retired.used&&!retired.complete)||!loadouts[index].used||loadouts[index].pawn!=pawn||loadouts[index].epoch!=epoch)
            return Fail(4811,"local_server_runtime_damage_retirement_epoch_mismatch");
        retired={};retired.used=true;retired.pawn=pawn;retired.epoch=epoch;
        if(!Capture(pawn,retired.identity))return Fail(4834,"local_server_runtime_damage_retiring_weak_identity_missing");
    }
    retired.heroDamageRetired=true;return true;
}
// Native health is sampled for every bound roster member BEFORE any callback
// can retire parent input or let match Poll reuse an NPC slot. The ready proof
// is cached in that exact RetiredLife so the later bool match hook only confirms
// already completed damage retirement; no pending bool is passed to coordinator.
inline bool PreflightHeroDamageRetirements(){
    if(!HeroDamageActive())return true;
    if(!local_server_ability_damage_integration::FlushRetirements()){
        if(HeroDamageFault())return false;
        return HeroDamageWait("local_server_runtime_waiting_early_damage_retirement_flush");
    }
    std::array<bool,MaxPlayers> needs{};
    for(size_t n=0;n<MaxPlayers;++n){const auto& plan=loadouts[n];if(!plan.used)continue;
        const auto player=local_server_match::Player(n);
        const bool current=player&&player->present&&player->pawn==plan.pawn&&player->epoch==plan.epoch;
        auto& target=original_damage::targets[n];
        if(current&&target.bound&&target.pawn==plan.pawn){
            if(!original_damage::Sample(n))return Fail(original_damage::State().error,original_damage::State().stage);
            needs[n]=!target.alive;
        }
        needs[n]=needs[n]||!current||(player&&(!player->alive||player->phase==local_server_match::Phase::DeadWaiting));
        const auto& old=retiredLives[n];
        needs[n]=needs[n]||(old.used&&!old.complete&&old.pawn==plan.pawn&&old.epoch==plan.epoch);
    }
    bool ready=true;
    for(size_t n=0;n<MaxPlayers;++n)if(needs[n]){
        const auto& plan=loadouts[n];
        if(!PrepareHeroDamageRetirement(n,plan.pawn,plan.epoch)){
            if(state.error)return false;ready=false;
        }
    }
    return ready;
}

inline bool RetireLife(size_t index,void* pawn,uint32_t epoch,bool allowBoundaryWait){
    if(index>=MaxPlayers||!pawn||!epoch)return Fail(4811,"local_server_runtime_retirement_epoch_mismatch");
    if(!PrepareHeroDamageRetirement(index,pawn,epoch))return false;
    auto& retired=retiredLives[index];
    const bool same=retired.used&&retired.pawn==pawn&&retired.epoch==epoch;
    if(same){
        if(!Live(pawn,retired.identity))return Fail(4834,"local_server_runtime_retired_life_identity_expired");
        if(retired.complete)return true;
    }else{
        if((retired.used&&!retired.complete)||!loadouts[index].used||loadouts[index].pawn!=pawn||loadouts[index].epoch!=epoch)
            return Fail(4811,"local_server_runtime_retirement_epoch_mismatch");
        retired={};retired.used=true;retired.pawn=pawn;retired.epoch=epoch;
        if(!Capture(pawn,retired.identity))return Fail(4834,"local_server_runtime_retiring_life_weak_identity_missing");
    }
    // Snapshot this exact old life before any callback can clear its slots
    // or presentation references. Mutating destruction happens only after all
    // damage/input/cosmetic/ability observers below have retired.
    if(state.enabled&&!retired.equipmentCaptured){
        if(!local_server_equipment::CaptureRetiringLife(index,pawn,epoch)){
            const auto equipment=local_server_equipment::State(index);
            return Fail(equipment&&equipment->error?equipment->error:local_server_equipment::State().error,
                equipment&&equipment->error?equipment->stage:local_server_equipment::State().stage);
        }
        retired.equipmentCaptured=true;
    }
    if(!retired.effectsRetired){
        if(state.enabled&&!local_server_effect_authority::RetirePlayer(index,pawn,epoch)){
            const auto& effects=local_server_effect_authority::GetState();return Fail(effects.error,effects.stage);
        }
        retired.effectsRetired=true;
    }
    if(!retired.combatRetired){
        if(state.enabled&&!local_server_humanlike_bots::BeforeRetire(index,pawn,epoch))
            return Fail(7758,"humanlike_owned_weapon_cache_retirement_failed");
        if(state.enabled&&!local_server_combat::BeforeRetire(index,pawn,epoch))
            return Fail(local_server_combat::GetState().error,local_server_combat::GetState().stage);
        retired.combatRetired=true;
    }
    if(!retired.npcRetired){
        const bool retiredNpc=!state.enabled||local_server_npc_input::BeforeRetire(index,pawn,epoch);
        CaptureNpcSnapshot();
        if(!retiredNpc)return Fail(local_server_npc_input::GetState().error,local_server_npc_input::GetState().stage);
        if(!local_server_humanlike_bots::Retired(index,pawn,epoch))return Fail(7762,"humanlike_retired_input_consumer_failed");
        retired.npcRetired=true;
    }
    if(!retired.senderRetired){
        if(state.enabled&&!local_server_character_stream::Retire(pawn))
            return Fail(local_server_character_stream::GetState().error,local_server_character_stream::GetState().stage);
        retired.senderRetired=true;
    }
    // Combat releases its composed view observer before Root restores the
    // authority input receiver's sole Pawn table for this life epoch.
    if(!retired.parentRetired){
        if(index==0&&!hooks.beforeRetire&&local_server_input::GetState().installed&&local_server_input::GetState().pawn==pawn)
            return Fail(4835,"local_server_runtime_authority_input_retirement_hook_required");
        if(hooks.beforeRetire&&!hooks.beforeRetire(index,pawn,epoch)){
            const auto& input=local_server_input::GetState();
            if(allowBoundaryWait&&index==0&&!input.error){
                state.ready=false;state.deathPreprocessPending=true;++state.deathRetirementDeferrals;
                state.stage="local_server_runtime_waiting_nonreentrant_input_retirement_boundary";return false;
            }
            return Fail(index==0&&input.error?input.error:4812,index==0&&input.error?input.stage:"local_server_runtime_parent_retirement_failed");
        }
        retired.parentRetired=true;
    }
    if(!retired.heroAuthorityRetired){
        if(state.host&&index==0&&HeroPractice()&&!local_server_hero_authority::RetirePlayer(pawn,epoch)){
            return HeroAuthorityFailure();
        }
        retired.heroAuthorityRetired=true;
    }
    if(!retired.cosmeticsRetired){
        if(state.enabled&&!CosmeticsResult(local_server_cosmetics::Retire(index,pawn,epoch)))return false;
        retired.cosmeticsRetired=true;
    }
    if(!retired.abilitiesRetired){
        if(!local_server_abilities::RetirePlayer(pawn))return Fail(local_server_abilities::GetState().error,local_server_abilities::GetState().stage);
        retired.abilitiesRetired=true;
    }
    if(!retired.heroAbilitiesRetired){
        if(index==0&&HeroPractice()&&!local_server_hero_abilities::RetirePlayer(pawn,epoch))
            return Fail(local_server_hero_abilities::GetState().error,local_server_hero_abilities::GetState().stage);
        retired.heroAbilitiesRetired=true;
    }
    if(!retired.startingRetired){
        if(index==0&&state.enabled&&JettPractice()&&!local_server_starting_equipment::RetirePlayer(pawn,epoch))
            return Fail(local_server_starting_equipment::GetState().error,local_server_starting_equipment::GetState().stage);
        retired.startingRetired=true;
    }
    if(!retired.armorRetired){
        if(state.enabled&&!Practice()&&!local_server_armor::BeforeRetire(index,pawn,epoch)){
            const auto& armor=local_server_armor::GetState();return Fail(armor.error?armor.error:5527,armor.stage);
        }
        retired.armorRetired=true;
    }
    if(!retired.equipmentRetired){
        if(state.enabled&&!local_server_equipment::RetireCapturedLife(index,pawn,epoch)){
            const auto equipment=local_server_equipment::State(index);
            return Fail(equipment&&equipment->error?equipment->error:local_server_equipment::State().error,
                equipment&&equipment->error?equipment->stage:local_server_equipment::State().stage);
        }
        if(!local_server_equipment::UnregisterPlayer(index))return Fail(4813,"local_server_runtime_equipment_retirement_failed");
        retired.equipmentRetired=true;
    }
    // An accepted configuration request retains canonical IDs across the pawn
    // epoch. Its old native transaction is not treated as completed; it is
    // submitted anew after the replacement's default loadout is established.
    if(index==0&&requestInFlight)requestSequence=0;
    loadouts[index]={};retired.complete=true;++state.retirements;return true;
}
inline bool BeforeRetire(size_t index,void* pawn,uint32_t epoch){return RetireLife(index,pawn,epoch,false);}
inline void Stop();
inline void RegisterName(size_t index,void* ps,uint32_t epoch){
    if(!local_server_names::GetState().initialized)return;
    wchar_t subject[256]{};
    if(!local_server_services::ReadPlayerSubjectKey(ps,subject,256)){
        if(local_server_services::State().error)Fail(local_server_services::State().error,local_server_services::State().stage);
        return;
    }
    // AI with an empty actual account subject stays empty. Engine.PlayerId is
    // the match agent's separate integer identity, never a fabricated login.
    if(!subject[0])return;
    // The admitted connection is cached as Player before the hero is spawned.
    // Keep that presentation name across Pawn epochs; the hero label is separate.
    const auto label=index==0?L"Player":original_players::players[index].requestedName;
    if(!local_server_names::Register(index,epoch,ps,label[0]?label:L"Player")&&local_server_names::GetState().error)
        Fail(local_server_names::GetState().error,local_server_names::GetState().stage);
}
inline void ActorSpawned(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch,bool human){
    if(state.error||index>=MaxPlayers)return;
    if(human){
        if(state.controller!=pc||state.playerState!=ps||Read<void*>(pc,0x510)!=state.connection
           ||!HumanOwner(pc,ps,pawn,state.connection)||!Capture(pawn,state.pawnIdentity)){
            Fail(4814,"local_server_runtime_human_respawn_changed_admitted_identity");return;
        }
        state.pawn=pawn;state.humanEpoch=epoch;
    }
    RegisterName(index,ps,epoch);
    if(state.enabled&&!local_server_combat::RegisterPlayer(index,pc,ps,pawn,epoch)){
        Fail(local_server_combat::GetState().error,local_server_combat::GetState().stage);return;
    }
    if(state.enabled){
        const bool registered=local_server_npc_input::Register(index,pc,ps,pawn,epoch,human);CaptureNpcSnapshot();
        if(!registered){Fail(local_server_npc_input::GetState().error,local_server_npc_input::GetState().stage);return;}
        if(!CosmeticsResult(local_server_cosmetics::Register(index,pc,ps,pawn,epoch,human)))return;
    }
    if(hooks.actorSpawned)hooks.actorSpawned(index,pc,ps,pawn,epoch,human);
}
inline void AfterScoreCommit(uint64_t token,size_t source,size_t victim){
    if(!Practice()&&!local_server_kill_ammo::OnCommittedKill(token,source,victim)){
        const auto& ammo=local_server_kill_ammo::GetState();Fail(ammo.error?ammo.error:5482,ammo.stage);
    }
}
inline bool AfterDeath(size_t index,void* pawn,void* ps,uint32_t epoch){
    if(Practice())return true;
    if(local_server_pickups::OnDeath(index,pawn,ps,epoch))return true;
    const auto& pickups=local_server_pickups::GetState();return Fail(pickups.error?pickups.error:7906,pickups.stage);
}
inline bool StopDeathmatchExtras(){
    if(!state.host||Practice())return true;
    if(local_server_kill_ammo::GetState().initialized&&!local_server_kill_ammo::Stop()){const auto& ammo=local_server_kill_ammo::GetState();return Fail(ammo.error?ammo.error:5490,ammo.stage);}
    if(!local_server_pickups::Stop()){const auto& pickups=local_server_pickups::GetState();return Fail(pickups.error?pickups.error:7917,pickups.stage);}
    return true;
}
inline bool WeaponInput(size_t index,bool fire,bool reload){
    if(index>=MaxPlayers||!loadouts[index].used)return false;
    if(!loadouts[index].complete||!local_server_equipment::Ready(index)||!local_server_combat::Ready(index))return true;
    // A native inactive match forbids new presses, but its last held NPC
    // input still needs the original producer's ordinary neutral release.
    return local_server_combat::WeaponInput(index,state.nativeGameplayActive&&fire,state.nativeGameplayActive&&reload);
}
inline bool PollNpcAuthority(double delta){
    if(!state.enabled)return true;
    armor_poll_performance::Scope measurement(armor_poll_performance::Slot::NpcAuthorityPoll);
    const auto& npc=local_server_npc_input::GetState();
    if(npc.error||npc.unsupported){CaptureNpcSnapshot();return Fail(npc.error,npc.stage);}
    if(!npc.frameArmed){
        // The driver table can first be installed between original dispatch
        // and flush. Let ordinary match/equipment work proceed; never invent
        // the missing pre-world observation or an authority physics step.
        ++state.npcUnarmedPollSkips;CaptureNpcSnapshot();return true;
    }
    const bool polled=local_server_npc_input::Poll(static_cast<float>(delta));CaptureNpcSnapshot();
    return polled||Fail(local_server_npc_input::GetState().error,local_server_npc_input::GetState().stage);
}
inline bool BeforeWorldTick(float delta){
    if(!state.initialized||state.closed||!state.host||!state.enabled||state.stopping)return true;
    if(state.error)return false;
    if(!Thread())return Fail(4848,"local_server_runtime_npc_world_tick_requires_game_thread");
    const bool observed=local_server_npc_input::BeforeWorldTick(delta,!state.heroDamageBoundaryPending&&!state.closePending);CaptureNpcSnapshot();
    return observed||Fail(local_server_npc_input::GetState().error,local_server_npc_input::GetState().stage);
}
inline bool CanDamage(size_t source,size_t victim){
    return !state.stopping&&state.nativeGameplayActive&&source<MaxPlayers&&victim<MaxPlayers
        &&loadouts[source].complete&&loadouts[victim].complete
        &&local_server_equipment::Ready(source)&&local_server_equipment::Ready(victim)
        &&(!hooks.canDamage||hooks.canDamage(source,victim));
}
inline void MatchEnded(int32_t winner,local_server_match::EndReason reason){
    if(!StopDeathmatchExtras())return;
    for(size_t n=0;n<MaxPlayers;++n)if(loadouts[n].used)local_server_equipment::SetPurchasesEnabled(n,false);
    state.ready=false;state.endWinner=winner;state.endReason=reason;
    if(!local_server_match::FreeForAll()){
        state.nativeEndStage="local_server_runtime_native_match_end_unavailable_for_practice";
        if(!state.nativeEndHookForwarded){state.nativeEndHookForwarded=true;if(hooks.matchEnded)hooks.matchEnded(winner,reason);}
    }else if(reason!=local_server_match::EndReason::KillLimit&&reason!=local_server_match::EndReason::TimeLimit){
        Fail(4851,"local_server_runtime_native_match_end_reason_not_supported");
    }else{
        // A kill-limit callback can run inside the original damage commit.
        // Capture/publish only after that outer transaction has completed.
        state.nativeEndPending=true;state.nativeEndStage="local_server_runtime_waiting_stable_native_match_end_boundary";
    }
    CaptureFeatureSnapshot();
}
inline bool PollNativeEnd(){
    if(!state.enabled||!state.host)return true;
    if(state.nativeEndPending){
        if(local_server_match::pending.active){state.nativeEndStage="local_server_runtime_waiting_native_damage_commit_before_match_end";CaptureFeatureSnapshot();return true;}
        local_server_match_end::ActualTotals totals{};
        if(!local_server_match_end::CaptureTotalsFromMatch(endTotals,totals,PairEpoch))return Fail(4852,"local_server_runtime_actual_completed_match_totals_unavailable");
        const auto winner=state.endWinner>=0?local_server_match::Player(static_cast<size_t>(state.endWinner)):nullptr;
        if(state.endWinner>=0&&(!winner||!winner->present))return Fail(4853,"local_server_runtime_actual_match_winner_not_current_participant");
        const bool submitted=local_server_match_end::EndAuthorityMatch(state.world,winner?winner->ps:nullptr,state.endReason,totals);
        CaptureFeatureSnapshot();
        if(!submitted)return Fail(local_server_match_end::GetState().error,local_server_match_end::GetState().stage);
        state.nativeEndPending=false;state.nativeEndSubmitted=true;
    }
    if(state.nativeEndSubmitted){
        const bool polled=local_server_match_end::PollAuthority();CaptureFeatureSnapshot();
        const auto& ended=local_server_match_end::GetState();if(!polled)return Fail(ended.error,ended.stage);
        state.nativeGameplayActive=ended.nativeActive;state.nativeEndStage=ended.stage;
        if(ended.complete&&!state.nativeEndHookForwarded){state.nativeEndHookForwarded=true;if(hooks.matchEnded)hooks.matchEnded(state.endWinner,state.endReason);}
    }
    return true;
}
inline bool CosmeticsResult(bool result){
    CaptureFeatureSnapshot();if(result)return true;
    for(const auto& player:local_server_cosmetics::players)if(player.registered&&player.error)return Fail(player.error,player.stage);
    return Fail(local_server_cosmetics::GetState().error?local_server_cosmetics::GetState().error:4854,local_server_cosmetics::GetState().stage);
}
inline void Stop(){
    state.stopping=true;state.ready=false;state.stage="local_server_runtime_stop_requested";
    const auto& events=local_server_hero_event_authority::GetState();
    if(state.host&&HeroPractice()&&events.initialized&&events.installed&&!events.closed&&!events.closing&&!events.error
       &&!local_server_hero_event_authority::SetPolicy(true,false))HeroAuthorityFailure();
    FenceHeroDamageAdmission();
    if(!state.stopNotified){state.stopNotified=true;if(hooks.requestStop)hooks.requestStop();}
}
inline std::string StateJson();
inline std::string ControlStateJson();
inline Result Dispatch(Command,const uint8_t*,uint32_t,char*,size_t);
inline bool RequestStop(){return Thread()?local_server_control::RequestStop():false;}
inline bool StopDisconnectedWorldFrame(double delta){
    // Disconnect is discovered inside the real post-world callback. Retire
    // that already armed pair here; a later frame has a different delta.
    state.disconnected=true;
    return ObservePendingWorldFrame(delta)&&RequestStop();
}

inline bool ReadFundedBoundarySource(void* expectedWorld,void* expectedDriver,local_server_human_funded_boundary_probe::PublicSource& out){
    namespace probe=local_server_human_funded_boundary_probe;
    const auto& native=local_server_npc_time_domain::GetState();const auto& cached=native.source;
    out={};out.reason="original_mode_source_context_not_current";
    if(!local_server_empty_bootstrap_probe::armed||!state.enabled||!state.host||Practice()
       ||GetCurrentThreadId()!=state.gameThread||GetCurrentThreadId()!=original_players::gameThread
       ||expectedWorld!=state.world||expectedDriver!=state.driver||!native.initialized||native.closed||native.unsupported||native.error
       ||!native.profiles||!native.sourceEpoch||native.world!=expectedWorld||native.driver!=expectedDriver)return false;
    out.gameStartSubmitted=state.gameStartSubmitted;out.nativeStartScheduleSubmitted=local_server_game_start::GetState().scheduleSubmitted;
    out.world=native.world;out.driver=native.driver;out.mode=cached.mode;out.gameState=cached.gameState;
    out.timing=cached.timing;out.fixed=cached.fixed;out.clock=cached.clock;out.sourceEpoch=native.sourceEpoch;
    out.worldId={native.worldId.index,native.worldId.serial};out.driverId={native.driverId.index,native.driverId.serial};
    out.modeId={native.modeId.index,native.modeId.serial};out.gameStateId={native.gameStateId.index,native.gameStateId.serial};
    out.timingId={native.timingId.index,native.timingId.serial};out.fixedId={native.fixedId.index,native.fixedId.serial};
    // Reuse existing typed/profile/Weak proof, but read C directly NOW. Calling
    // ReadSource/Observe would mutate their caches; 789DF70 can lazily write.
    if(!probe::Live(out.world,out.worldId)||!probe::Live(out.driver,out.driverId)||!probe::Live(out.mode,out.modeId)
       ||!probe::Live(out.gameState,out.gameStateId)||!probe::Live(out.timing,out.timingId)||!probe::Live(out.fixed,out.fixedId)){
        out.reason="original_mode_source_existing_weak_generation_changed";return false;
    }
    if(Read<void*>(out.world,0x40)!=out.driver||Read<void*>(out.driver,0x178)!=out.world||Read<void*>(out.driver,0xc0)
       ||Read<void*>(out.world,0x170)!=out.mode||Read<void*>(out.world,0x178)!=out.gameState
       ||Read<void*>(out.mode,0x868)!=out.timing||Read<void*>(out.timing,0xf8)!=out.clock||Read<void*>(out.timing,0x108)!=out.fixed
       ||Read<void*>(out.gameState,0xcc0)!=out.fixed||Read<void*>(out.timing,0xc8)!=out.mode||Read<void*>(out.timing,0x28)!=out.mode
       ||Read<void*>(out.fixed,0xc8)!=out.gameState||Read<void*>(out.fixed,0x28)!=out.gameState||!(Read<uint8_t>(out.timing,0xa0)&1)
       ||Read<uint8_t>(out.fixed,0xd9)!=128){out.reason="original_mode_source_live_owner_or_manager_link_changed";return false;}
    const auto table=Read<void*>(out.timing);const auto vb=Read<void*>(out.clock,0x10);
    if(Read<void*>(table,0x3e0)!=reinterpret_cast<void*>(engine::image+0x7868330)
       ||Read<void*>(table,0x3e8)!=reinterpret_cast<void*>(engine::image+0x7868830)
       ||Read<void*>(table,0x3f8)!=reinterpret_cast<void*>(engine::image+0x78680e0)
       ||Read<void*>(table,0x1b8)!=reinterpret_cast<void*>(engine::image+0x2a64ee0)
       ||Read<void*>(out.clock)!=reinterpret_cast<void*>(engine::image+0xb600a60)
       ||vb!=reinterpret_cast<void*>(engine::image+0xc6b3e90)||Read<int32_t>(vb,4)!=0x990){out.reason="original_mode_source_retained_native_shape_changed";return false;}
    const auto schedule=Read<local_server_npc_time_domain::Array>(out.fixed,0xe0);
    if(schedule.count!=0||schedule.capacity<0||schedule.capacity>4096){out.reason="original_mode_source_pending_schedule_not_empty";return false;}
    out.c=Read<uint32_t>(out.clock,0x9a8);out.step=Read<uint32_t>(out.clock,0x9ac);out.offset=Read<int32_t>(out.clock,0x24);
    out.accumulated=Read<float>(out.clock,0x20); // Mode A_C: context only, NEVER the human A_H used for Q.
    if(out.c>local_server_npc_time_domain::MaxExactIndex||out.step!=1||out.offset||!std::isfinite(out.accumulated)||out.accumulated<0){
        out.reason="original_mode_source_counter_or_neutral_conversion_changed";return false;
    }
    out.verified=true;out.reason="cached_original_typed_source_reverified_and_counter_read_directly_same_GT";return true;
}
inline bool InitializeWorldImpl(void* world,void* driver,bool host){
    if(state.initialized)return !state.error&&Thread()&&state.world==world&&state.driver==driver&&state.host==host;
    if(!state.loaded||state.closed||state.error||host!=state.host||(host&&!state.factoryConfigured)
       ||!world||!driver||world!=original_players::world||!original_players::Thread())
        return Fail(4815,"local_server_runtime_original_world_or_factory_context_missing");
    if(Read<void*>(world,0x40)!=driver||Read<void*>(driver,0x178)!=world
       ||(host?Read<void*>(driver,0xc0)!=nullptr:Read<void*>(driver,0xc0)==nullptr))
        return Fail(4816,"local_server_runtime_actual_native_driver_role_mismatch");
    if(state.enabled&&(!hooks.requestStop||(host&&!hooks.spawnHuman)))
        return Fail(4817,"local_server_runtime_owned_stop_and_human_respawn_hooks_required");
    if(state.enabled&&(hooks.port<1024||hooks.port>65535))return Fail(4855,"local_server_runtime_actual_loopback_port_required_for_presentation_pair");
    state.world=world;state.driver=driver;state.gameThread=GetCurrentThreadId();state.initialized=true;
    (void)armor_poll_performance::BindGameThread(state.gameThread);
    if(!Capture(world,state.worldIdentity)||!Capture(driver,state.driverIdentity))return Fail(4818,"local_server_runtime_world_driver_weak_identity_missing");
    if(host)local_server_authority_clock_probe::Poll(world,driver);
    playerControllerClass=engine::Find(L"/Script/Engine.PlayerController");
    if(!playerControllerClass)return Fail(4833,"local_server_runtime_original_player_controller_class_missing");
    local_server_control::Hooks controlHooks;controlHooks.dispatch=Dispatch;controlHooks.stateJson=ControlStateJson;controlHooks.requestStop=Stop;
    if(!local_server_control::Initialize(controlHooks))return Fail(local_server_control::GetState().error,local_server_control::GetState().stage);
    if(host){
        if(!local_server_equipment::Initialize(world))return Fail(local_server_equipment::State().error,local_server_equipment::State().stage);
    }else if(!local_server_equipment::PrepareClientAppearance())return Fail(local_server_equipment::State().error,local_server_equipment::State().stage);
    if(!local_server_abilities::Init(world,host,JettPractice()))return Fail(local_server_abilities::GetState().error,local_server_abilities::GetState().stage);
    if(HeroPractice()&&!local_server_hero_abilities::Init(world,host))
        return Fail(local_server_hero_abilities::GetState().error,local_server_hero_abilities::GetState().stage);
    if(host&&HeroPractice()){
        local_server_hero_authority::LifecycleHooks lifecycle{};
        lifecycle.retirePlayer=RetireHeroTypedResources;lifecycle.close=CloseHeroTypedResources;
        if(!local_server_hero_authority::SetLifecycleHooks(lifecycle))return Fail(4862,"local_server_runtime_hero_lifecycle_binding_conflict");
        if(!local_server_hero_authority::InitHost(world))return HeroAuthorityFailure();
        local_server_player_state_authority::Hooks publisher{};publisher.validate=ValidateHeroPlayerStateSource;
        if(!local_server_player_state_authority::Init(world,publisher))return HeroAuthorityFailure();
        local_server_hero_ultimate_authority::Hooks ultimate{};
        ultimate.resolve=local_server_hero_authority::ResolveHostSource;ultimate.validate=local_server_hero_authority::ValidateHostSource;
        if(!local_server_hero_ultimate_authority::Init(world,ultimate)
           ||!local_server_hero_ultimate_authority::SetPolicy(true,true))return HeroAuthorityFailure();
    }
    if(HeroPractice()){
        local_server_hero_resource_init::Hooks resources{};
        if(host)resources.validate=local_server_hero_authority::ValidateHostSource;
        if(!local_server_hero_resource_init::Init(world,host,resources))return HeroAuthorityFailure();
    }
    if(HeroPractice())local_server_hero_skill_readback::Init(world,host);
    if(state.enabled&&!CosmeticsResult(local_server_cosmetics::Initialize(host,world,outputDirectory,hooks.port)))return false;
    if(host&&state.enabled){
        original_scoreboard::playerInfoClass=nullptr;
        if(!Practice()){
            auto infoClass=all_weapons::LoadClass(0x64f4540,deathmatch_rules::PlayerInfoClassPath);
            if(!infoClass||!original_players::Derived(infoClass,engine::Class(0x64f4540)))
                return Fail(4857,"local_server_runtime_original_deathmatch_player_info_class_unavailable");
            original_scoreboard::playerInfoClass=infoClass;
        }
        local_server_match::Hooks matchHooks;matchHooks.registerEquipment=RegisterEquipment;matchHooks.beforeRetire=BeforeRetire;
        matchHooks.equipmentReady=EquipmentReady;
        matchHooks.submitBot=local_server_npc_input::SubmitBot;matchHooks.npcAuthorityReady=local_server_npc_input::Ready;
        matchHooks.sensorPass=local_server_humanlike_bots::SensorPass;matchHooks.decideBot=local_server_humanlike_bots::Decide;
        matchHooks.botSubmitted=local_server_humanlike_bots::Submitted;
        matchHooks.weaponInput=WeaponInput;matchHooks.canDamage=CanDamage;matchHooks.actorSpawned=ActorSpawned;
        matchHooks.matchEnded=MatchEnded;matchHooks.spawnHuman=hooks.spawnHuman;
        matchHooks.afterScoreCommit=AfterScoreCommit;matchHooks.afterDeath=AfterDeath;
        matchHooks.isObserver=hooks.isObserver;
        // NPC decisions cache intent in the separate actual authority driver.
        // Human input/physics remains owned solely by Root's native receiver.
        if(!local_server_match::Initialize(world,matchHooks))return Fail(local_server_match::GetState().error,local_server_match::GetState().stage);
        if(HeroPractice()){
            local_server_hero_event_authority::Hooks events{};
            events.resolve=local_server_hero_authority::ResolveHostSource;events.validate=local_server_hero_authority::ValidateHostSource;
            if(!local_server_hero_event_authority::Init(world,events)
               ||!local_server_hero_event_authority::SetPolicy(true,true))return HeroAuthorityFailure();
        }
        local_server_effect_schema_probe::Capture();
        if(!local_server_effect_authority::Initialize(world)){
            const auto& effects=local_server_effect_authority::GetState();return Fail(effects.error,effects.stage);
        }
        local_server_effect_schema_probe::Capture();
        if(!local_server_combat::Initialize(world))return Fail(local_server_combat::GetState().error,local_server_combat::GetState().stage);
        if(!local_server_humanlike_bots::Initialize())return Fail(7759,"humanlike_single_scheduler_adapter_initialization_failed");
        if(HeroPractice()){
            // Explicitly keep fast=false until real Base/Radial and paired
            // client receipt verification. Existing PS/resource/Ult hooks stay.
            local_server_ability_damage_integration::Settings damageSettings{};
            if(!local_server_ability_damage_integration::Initialize(world,damageSettings)){
                if(HeroDamageFault())return false;
                return Fail(4867,"local_server_runtime_damage_compositor_initialization_failed");
            }
        }
        if(!local_server_character_stream::Initialize(world,driver))return Fail(local_server_character_stream::GetState().error,local_server_character_stream::GetState().stage);
        local_server_npc_input::Hooks npcHooks;npcHooks.weaponInput=WeaponInput;
        const bool npcInitialized=local_server_npc_input::Initialize(world,driver,npcHooks);CaptureNpcSnapshot();
        if(!npcInitialized)return Fail(local_server_npc_input::GetState().error,local_server_npc_input::GetState().stage);
        if(local_server_empty_bootstrap_probe::armed)local_server_human_funded_boundary_probe::SetSourceReader(ReadFundedBoundarySource);
        local_server_combat::SetExternalNPCProducer(ExternalNpcProducer);
        if(!local_server_combat::SetExternalNPCClockValidator(ExternalNpcClock))
            return Fail(4557,"local_server_runtime_external_npc_clock_validator_owner_conflict");
        if(!Practice()&&!local_server_armor::Initialize(world)){
            const auto& armor=local_server_armor::GetState();return Fail(armor.error?armor.error:5500,armor.stage);
        }
        if(!local_server_match::Start(local_server_control::Initial()))return Fail(local_server_match::GetState().error,local_server_match::GetState().stage);
        state.matchStarted=true;state.pendingRevision=local_server_match::Configuration().config.revision;state.configurationPending=true;
        if(!Practice()&&!local_server_kill_ammo::Initialize(world)){
            const auto& ammo=local_server_kill_ammo::GetState();return Fail(ammo.error?ammo.error:7920,ammo.stage);
        }
        if(!Practice()&&!local_server_pickups::Initialize(world,driver)){
            const auto& pickups=local_server_pickups::GetState();return Fail(pickups.error?pickups.error:7920,pickups.stage);
        }
    }
    state.stage=state.enabled?"local_server_runtime_waiting_actual_admitted_player":"local_server_runtime_basic_lifecycle_probe";
    return true;
}
inline LONG CaptureException(EXCEPTION_POINTERS* exception){
    const auto code=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0004800U;
    const auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;
    if(local_server_hero_event_authority::GetState().error)HeroAuthorityFailure();
    else Fail(code,"local_server_runtime_native_exception_requires_new_session");
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool InitializeWorld(void* world,void* driver,bool host){
    __try{return InitializeWorldImpl(world,driver,host);}__except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool AdoptHumanImpl(void* pc,void* ps,void* pawn,void* connection){
    if(!Thread()||state.error||state.stopping||!HumanOwner(pc,ps,pawn,connection))return Fail(4819,"local_server_runtime_requires_actual_admitted_authority_human");
    if(state.humanAdopted)return state.controller==pc&&state.playerState==ps&&state.pawn==pawn&&state.connection==connection;
    if(!Capture(pc,state.controllerIdentity)||!Capture(ps,state.stateIdentity)||!Capture(pawn,state.pawnIdentity)||!Capture(connection,state.connectionIdentity))
        return Fail(4820,"local_server_runtime_admitted_player_weak_identity_missing");
    state.controller=pc;state.playerState=ps;state.pawn=pawn;state.connection=connection;state.humanEpoch=1;
    if(!InstallControllerHook(pc))return false;
    if(state.enabled){
        if(!local_server_match::RegisterHuman(pc,ps,pawn,1))return Fail(local_server_match::GetState().error,local_server_match::GetState().stage);
        if(state.error)return false;
    }else if(!RegisterEquipment(0,pc,ps,pawn,1,true))return false;
    state.humanAdopted=true;
    state.stage=state.enabled?"local_server_runtime_building_native_authority_graph":"local_server_runtime_probe_loadout_pending";return true;
}
inline bool AdoptHuman(void* pc,void* ps,void* pawn,void* connection){
    __try{return AdoptHumanImpl(pc,ps,pawn,connection);}__except(CaptureException(GetExceptionInformation())){return false;}
}

inline bool AdvanceLoadouts(){
    for(size_t n=0;n<MaxPlayers;++n){auto& plan=loadouts[n];if(!plan.used||plan.complete)continue;
        const auto p=local_server_equipment::State(n);
        if(!p||!p->registered||p->pawn!=plan.pawn||p->error)return Fail(p&&p->error?p->error:4821,p&&p->error?p->stage:"local_server_runtime_loadout_player_changed");
        if(local_server_empty_bootstrap_probe::HoldBound(n,plan.pawn,plan.epoch))continue;
        if(plan.waitingSequence){
            if(p->lastCompletedSequence!=plan.waitingSequence)continue;
            plan.waitingSequence=0;++plan.step;
        }
        const unsigned count=plan.human?(plan.restoreSelection?4U:3U):1U;
        if(plan.step==count){
            plan.complete=true;
            const bool allow=!state.enabled||!local_server_match::GetState().ended;
            if(!local_server_equipment::SetPurchasesEnabled(n,allow))return Fail(4822,"local_server_runtime_loadout_purchase_policy_failed");
            continue;
        }
        if(local_server_equipment::Busy(n))continue;
        const auto definition=plan.human&&plan.step==3?all_weapons::Find(plan.held)
            :Weapon(plan.human&&plan.step==0?"Classic":plan.human&&plan.step==1?"Melee":"Vandal");
        if(!definition)return Fail(4823,"local_server_runtime_default_weapon_not_in_verified_catalog");
        auto request=Preference(n,*definition);
        request.mode=plan.human&&plan.step<2?local_server_equipment::Mode::Grant:local_server_equipment::Mode::Equip;
        if(!local_server_equipment::Equip(n,request))return Fail(p->error?p->error:4824,p->error?p->stage:p->lastRejection);
        plan.waitingSequence=p->lastAcceptedSequence;
    }
    return true;
}
inline bool Queue(const Request& request){
    if(requestCount+(requestInFlight?1U:0U)==RequestCapacity)return false;
    requests[(requestHead+requestCount)%RequestCapacity]=request;++requestCount;
    state.pendingRequests=requestCount+(requestInFlight?1U:0U);return true;
}
inline bool HumanLifeWaiting();
inline bool AdvanceRequests(){
    if(!loadouts[0].complete||HumanLifeWaiting())return true;
    const auto p=local_server_equipment::State(0);
    if(requestInFlight&&requestSequence){
        if(!p||p->lastCompletedSequence!=requestSequence)return true;
        requestInFlight=false;requestSequence=0;++state.completedRequests;
    }
    if(local_server_equipment::Busy(0))return true;
    if(!requestInFlight){
        if(!requestCount){state.pendingRequests=0;return true;}
        activeRequest=requests[requestHead];requestHead=(requestHead+1)%RequestCapacity;--requestCount;requestInFlight=true;
    }
    if(!local_server_equipment::Equip(0,activeRequest))return Fail(p&&p->error?p->error:4825,p&&p->error?p->stage:p?p->lastRejection:"local_server_runtime_equipment_request_player_missing");
    requestSequence=p->lastAcceptedSequence;state.pendingRequests=requestCount+1;return true;
}
inline bool MaintainCurrentSkinPreference(){
    if(!loadouts[0].complete||HumanLifeWaiting()||requestCount||requestInFlight||!local_server_equipment::Ready(0))return true;
    const auto p=local_server_equipment::State(0);
    const auto definition=p&&p->current?all_weapons::FindByClass(Read<void*>(p->current,0x10)):nullptr;
    if(!definition)return true;const auto request=Preference(0,*definition);
    if(!original_content::Valid(request.skin)||!original_content::Valid(request.level)||!original_content::Valid(request.chroma))return true;
    if(all_weapons::Same(Read<Guid>(p->current,0xfe8),request.skin)&&all_weapons::Same(Read<Guid>(p->current,0xff8),request.level)
       &&all_weapons::Same(Read<Guid>(p->current,0x1038),request.chroma))return true;
    // A saved non-held skin becomes applicable after an actual native 1/2/3
    // switch. Reuse the same authority transaction, never rewrite client state.
    return Queue(request)||Fail(4832,"local_server_runtime_current_skin_preference_queue_failed");
}
inline bool AbilityReady(){
    if(!Practice())return true;
    if(HeroPractice())return local_server_hero_abilities::AssemblyReady(state.pawn,state.humanEpoch);
    for(const auto& p:local_server_abilities::GetPlayers())if(p.used&&p.pawn==state.pawn)return p.ready;
    return false;
}
inline bool HumanLifeWaiting(){
    const auto human=state.enabled&&state.host?local_server_match::Player(0):nullptr;
    return human&&(!human->alive||human->phase!=local_server_match::Phase::Alive);
}
inline bool ClientEquipmentPredictionReady(){
    if(!HeroPractice())return clientEquipment.ownership.allNativePredictionReady;
    const auto& owner=clientEquipment.ownership;
    // Preserve complete ownership/Role2 validation across every real slot.
    // A native inactive passive graph is judged by the separate original hero
    // assembly receipt, rather than requiring every passive machine enabled.
    if(!clientEquipment.ownershipObserved||!owner.complete||owner.items!=owner.autonomousItems
       ||owner.authorityItems||owner.invalidRoles||!clientEquipment.current)return false;
    local_server_item_ownership::ClientObservation current{};
    if(!local_server_item_ownership::ObserveClientItem(state.pawn,clientEquipment.current,current))return false;
    if(current.items!=1||current.ownedItems!=1||current.autonomousItems!=1||current.authorityItems||current.invalidRoles
       ||!current.machines||current.enabledMachines!=current.machines
       ||current.predictedStates!=current.machines||current.roleSelectedStates!=current.machines)return false;
    // Ordinary current equipment keeps the same native prediction gate. Also
    // validate its primary state's current real identity and owner links.
    auto actor=clientEquipment.current,machine=clientEquipment.machine,selected=clientEquipment.predictedState;
    Weak actorId{},machineId{},selectedId{};
    return Capture(actor,actorId)&&Capture(machine,machineId)&&Capture(selected,selectedId)
        &&Read<void*>(machine,0xc8)==actor&&Read<void*>(selected,0xc8)==actor&&Read<void*>(selected,0xe0)==machine;
}
inline void RefreshReady(){
    state.namesReady=!local_server_names::GetState().initialized||!local_server_names::roster.count||local_server_names::GetState().ready;
    state.abilitiesReady=AbilityReady();
    state.startingEquipmentReady=!state.enabled||!Practice();
    if(state.host){
        if(state.enabled&&Practice())state.startingEquipmentReady=state.humanAdopted
            &&(HeroPractice()?local_server_hero_abilities::AssemblyReady(state.pawn,state.humanEpoch)
                :local_server_starting_equipment::Ready(state.pawn,state.humanEpoch));
        state.equipmentReady=state.humanAdopted;
        for(const auto& plan:loadouts)if(plan.used)state.equipmentReady=state.equipmentReady&&plan.complete&&local_server_equipment::Ready(static_cast<size_t>(&plan-loadouts.data()));
        state.combatReady=state.enabled&&local_server_combat::GetState().ready;
        if(state.enabled)for(const auto& p:local_server_match::AuthorityPlayers())if(p.present&&p.alive)
            state.combatReady=state.combatReady&&local_server_combat::Ready(p.index);
        state.nativeGameplayActive=state.nativeEndSubmitted?local_server_match_end::GetState().nativeActive
            :state.gameStartSubmitted&&local_server_game_start::GetState().gameplayActive;
        state.ready=state.enabled&&state.matchStarted&&state.humanAdopted&&local_server_match::GetState().ready
            &&state.equipmentReady&&state.abilitiesReady&&state.startingEquipmentReady&&state.combatReady&&state.nativeGameplayActive&&state.namesReady
            &&state.graphComplete&&!state.configurationPending;
    }else{
        if(HeroPractice())state.startingEquipmentReady=local_server_hero_abilities::AssemblyReady(state.pawn,state.humanEpoch);
        else if(state.enabled&&JettPractice()){
            const auto& path=local_server_client::shop.path;
            state.startingEquipmentReady=path.samples&&path.worldMatches&&path.playerStateMatches&&path.spawnedMatches
                &&path.inventoryOwned&&path.startingArrayValid&&path.startingClassesComplete;
        }
        state.equipmentReady=state.clientEquipmentObserved&&clientEquipment.current&&clientEquipment.current==clientEquipment.desired
            &&clientEquipment.initialCorrectionProcessed&&clientEquipment.ownerMatches&&clientEquipment.nativeComponentsPresent&&clientEquipment.appearanceFieldsPresent
            &&clientEquipment.ownershipObserved&&ClientEquipmentPredictionReady();
        state.ready=state.enabled&&state.humanAdopted&&state.equipmentReady&&state.abilitiesReady&&state.startingEquipmentReady&&local_server_client::GetState().ready;
    }
    if(state.host&&state.matchStarted&&local_server_match::GetState().ended&&!state.error&&!state.stopping){
        state.ready=false;state.stage=state.nativeEndSubmitted?local_server_match_end::GetState().stage:state.nativeEndStage;return;
    }
    if(state.error||state.stopping||state.closed)state.ready=false;
    if(state.ready)state.stage="local_server_runtime_native_graph_ready_acceptance_unverified";
    else if(!state.error&&!state.stopping)state.stage=!state.enabled?"local_server_runtime_basic_lifecycle_probe"
        :!state.humanAdopted?"local_server_runtime_waiting_actual_admitted_player"
        :!state.namesReady?"local_server_runtime_waiting_both_original_name_caches"
        :!state.equipmentReady?"local_server_runtime_waiting_original_equipment"
        :!state.abilitiesReady?"local_server_runtime_waiting_original_ability_graph"
        :!state.startingEquipmentReady?"local_server_runtime_waiting_original_authored_starting_equipment"
        :state.host&&!state.nativeGameplayActive?"local_server_runtime_waiting_original_gameplay_maturation"
        :state.host&&!state.combatReady?"local_server_runtime_waiting_authority_combat_bindings"
        :state.host&&state.configurationPending?"local_server_runtime_waiting_native_ammo_rule_readback"
        :state.host?local_server_match::GetState().stage:local_server_client::GetState().stage;
}
inline bool CurrentWorld(){
    return Live(state.world,state.worldIdentity)&&Live(state.driver,state.driverIdentity)
        &&Read<void*>(state.world,0x40)==state.driver&&Read<void*>(state.driver,0x178)==state.world;
}
inline AdmittedConnectionObservation ObserveAdmittedConnection(){
    auto& d=state.connectionObservation;++d.calls;
    if(!state.enabled||!state.humanAdopted||state.stopping||state.closed){
        ++d.skips;d.stage=state.disconnected&&d.closed
            ?(d.retiredWeakEmptyPeers?"local_server_runtime_connection_stopping_after_observed_native_peer_retirement"
                :"local_server_runtime_connection_stopping_after_observed_native_close")
            :"local_server_runtime_connection_not_applicable";
        return AdmittedConnectionObservation::NotApplicable;
    }
    d.sampledAtMs=GetTickCount64();
    d.nativeState=-1;d.clientCount=-1;d.weakVerified=d.stateReadable=d.driverVerified=d.openOwnersVerified=false;
    d.worldDriverVerified=d.peerListReadable=d.peerListShapeVerified=d.retiredWeakEmptyPeers=d.capturedTokenKnown=false;
    const auto failed=[&](unsigned code,const char* message){
        ++d.failures;d.stage=message;if(!state.error)Fail(code,message);
        return AdmittedConnectionObservation::Failed;
    };
    if(state.error)return failed(state.error,state.stage);
    d.worldDriverVerified=Thread()&&state.host&&CurrentWorld();
    if(!d.worldDriverVerified)
        return failed(4885,"local_server_runtime_connection_observation_world_or_thread_changed");
    struct Connections {void** data=nullptr;int32_t count=0,capacity=0;};
    Connections clients{};
    d.peerListReadable=original_players::Read(state.driver,0xc8,clients);
    if(d.peerListReadable)d.clientCount=clients.count;
    d.peerListShapeVerified=d.peerListReadable&&clients.count>=0&&clients.capacity>=clients.count
        &&clients.capacity<=1024&&(clients.capacity==0||clients.data);
    d.capturedTokenKnown=state.connection&&state.connectionIdentity.index>=0&&state.connectionIdentity.serial>0;
    d.weakVerified=d.capturedTokenKnown&&Live(state.connection,state.connectionIdentity);
    // The original driver's cleanup may retire its captured peer before this
    // callback. A previously observed complete one-peer Open tuple plus the
    // same live World/driver and its actual empty peer list proves removal.
    // Do not query the retired pointer's class, driver, state13C or owner.
    if(!d.weakVerified&&d.capturedTokenKnown&&d.open>0&&d.peerListShapeVerified&&clients.count==0){
        d.retiredWeakEmptyPeers=true;++d.closed;++d.retiredPeerObservations;
        d.stage="local_server_runtime_captured_peer_weak_retired_empty_native_peer_list";
        return AdmittedConnectionObservation::Closed;
    }
    d.driverVerified=d.weakVerified&&original_players::IsA(state.connection,0x3dea9c0)
        &&Read<void*>(state.connection,0x60)==state.driver;
    if(!d.driverVerified)
        return failed(4886,"local_server_runtime_connection_observation_original_weak_or_driver_changed");
    d.stateReadable=original_players::Read(state.connection,0x13c,d.nativeState);
    if(!d.stateReadable)
        return failed(4887,"local_server_runtime_connection_observation_native_state_unreadable");
    // A native close legitimately tears down Controller/Pawn links. Observe it
    // before death retirement or a pending spawn, independently of whether the
    // old input epoch is installed. No connection/lifecycle field is written.
    if(d.nativeState!=3){
        ++d.closed;++d.nativeStateCloses;d.stage="local_server_runtime_connection_closed_before_death_processing";
        return AdmittedConnectionObservation::Closed;
    }
    if(state.disconnected)
        return failed(4888,"local_server_runtime_connection_observation_closed_epoch_reopened");
    d.openOwnersVerified=d.peerListShapeVerified&&clients.count==1&&clients.capacity>=1&&clients.capacity<=1024
        &&clients.data&&Read<void*>(clients.data,0)==state.connection
        &&Live(state.controller,state.controllerIdentity)&&Live(state.playerState,state.stateIdentity)
        &&original_players::IsA(state.controller,original_players::ShooterControllerClass)
        &&original_players::IsA(state.playerState,original_players::ShooterStateClass)
        &&Read<uint8_t>(state.controller,0x224)==3&&Read<uint8_t>(state.playerState,0x224)==3
        &&Read<uint8_t>(state.connection,0x1d8)==3
        &&Read<void*>(state.connection,0x38)==state.controller&&Read<void*>(state.connection,0xa0)==state.controller
        &&Read<void*>(state.controller,0x510)==state.connection
        &&Read<void*>(state.controller,0x470)==state.playerState&&Read<void*>(state.playerState,0x210)==state.controller;
    if(!d.openOwnersVerified)
        return failed(4889,"local_server_runtime_connection_observation_open_native_owner_changed");
    ++d.open;d.stage="local_server_runtime_connection_open_before_death_processing";
    return AdmittedConnectionObservation::Open;
}
inline bool ShouldPollAuthorityInput(){
    // Root may install a fresh receiver when this is true. Installed state is
    // deliberately not a prerequisite; a new pawn still starts at clock zero.
    return state.initialized&&!state.closed&&state.host&&state.humanAdopted&&!state.error&&!state.stopping
        &&!state.deathPreprocessPending&&!HumanLifeWaiting();
}
inline bool PreprocessDeathsImpl(){
    if(!Thread()||!state.host)return Fail(4836,"local_server_runtime_death_preprocess_requires_host_game_thread");
    if(state.error)return false;
    if(state.stopping||!state.enabled||!state.matchStarted||!state.humanAdopted)return true;
    if(!CurrentWorld())return Fail(4827,"local_server_runtime_world_driver_changed_requires_new_pair");
    ++state.deathPreprocessCalls;state.deathPreprocessPending=false;state.heroDamageBoundaryPending=false;
    // Retire completed typed deaths before any old input/NPC slot can move.
    if(HeroDamageActive()&&!local_server_ability_damage_integration::FlushRetirements()){
        if(HeroDamageFault())return false;
        HeroDamageWait("local_server_runtime_waiting_early_damage_retirement_flush");return true;
    }
    // This helper only samples native life and commits an unaccounted native
    // death once. It does not call Poll, advance match elapsed time, TickBots,
    // map streaming, physical movement, input or the game-start getter.
    if(!local_server_match::PreprocessDeaths())return Fail(local_server_match::GetState().error,local_server_match::GetState().stage);
    if(local_server_match::pending.active){
        state.ready=false;state.deathPreprocessPending=true;
        state.stage="local_server_runtime_waiting_native_damage_dispatch_boundary";
        if(HeroDamageActive())state.heroDamageBoundaryPending=true;
        return true;
    }
    if(!PreflightHeroDamageRetirements())return state.error==0;
    const auto human=local_server_match::Player(0);
    if(!human||human->alive||human->phase!=local_server_match::Phase::DeadWaiting)return true;
    state.ready=false;
    if(!RetireLife(0,human->pawn,human->epoch,true))return state.error==0;
    // Full retirement is cached by actual pawn/epoch/weak identity. The due
    // spawn's existing BeforeRetire callback therefore becomes an idempotent
    // confirmation, preserving the ordinary same-PC/PS native factory path.
    if(!local_server_match::RespawnHuman())return Fail(local_server_match::GetState().error,local_server_match::GetState().stage);
    return state.error==0;
}
inline bool PreprocessDeaths(){
    __try{return PreprocessDeathsImpl();}__except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool TickHostImpl(double delta){
    if(!Thread()||!state.host||!std::isfinite(delta)||delta<0)return Fail(4826,"local_server_runtime_host_tick_context_invalid");
    armor_poll_performance::Scope measurementHost(armor_poll_performance::Slot::HostTick);
    if(state.error)return false;
    if(state.stopping){
        return HoldPendingWorldFrame(delta);
    }
    if(!CurrentWorld())return Fail(4827,"local_server_runtime_world_driver_changed_requires_new_pair");
    ++state.hostTicks;local_server_feedback_ledger::PollHost();
    if(state.heroDamageBoundaryPending){
        return HoldPendingWorldFrame(delta);
    }
    // Root's original input Poll can have produced native life transitions
    // after the early preprocess. Recheck before an adapter physics step.
    if(!PreflightHeroDamageRetirements()){
        if(state.error)return false;
        return HoldPendingWorldFrame(delta);
    }
    if(state.deathPreprocessPending){
        if(!PollNpcAuthority(delta))return false;return local_server_control::Poll();
    }
    const bool humanLifeWaiting=HumanLifeWaiting();
    const bool matchEnded=state.enabled&&state.matchStarted&&local_server_match::GetState().ended;
    if(state.enabled&&state.humanAdopted&&!humanLifeWaiting&&!matchEnded){
        const auto& input=local_server_input::GetState();
        if(!input.installed||input.pc!=state.controller||input.pawn!=state.pawn||input.connection!=state.connection){
            state.ready=false;state.stage="local_server_runtime_waiting_root_authority_input_epoch";
            if(!PollNpcAuthority(delta))return false;
            return local_server_control::Poll();
        }
    }
    const bool startAlreadySubmitted=state.gameStartSubmitted;
    const bool adoptStartPawn=state.gameStartSubmitted&&state.humanAdopted&&local_server_game_start::GetState().pawn!=state.pawn;
    // Root installs/polls its single authority input receiver before TickHost.
    // Adoption itself never forces the game-start dependency graph early.
    if(state.enabled&&state.humanAdopted&&!state.gameStartSubmitted&&!matchEnded){
        if(!local_server_game_start::Start(state.world,state.pawn)){
            const auto& start=local_server_game_start::GetState();
            if(start.disconnected)return StopDisconnectedWorldFrame(delta);
            return Fail(start.error,start.stage);
        }
        state.gameStartSubmitted=true;
    }
    if(adoptStartPawn&&!humanLifeWaiting&&!matchEnded&&!local_server_game_start::AdoptPawn(state.world,state.pawn)){
        const auto& start=local_server_game_start::GetState();
        if(start.disconnected)return StopDisconnectedWorldFrame(delta);
        return Fail(start.error,start.stage);
    }
    if(startAlreadySubmitted&&!adoptStartPawn&&!humanLifeWaiting&&!matchEnded&&!local_server_game_start::Poll()){
        const auto& start=local_server_game_start::GetState();
        if(start.disconnected)return StopDisconnectedWorldFrame(delta);
        return Fail(start.error,start.stage);
    }
    if(state.humanAdopted&&controllerHook.installed&&GetTickCount64()-controllerHook.lastAudit>=1000&&!AuditControllerHook())return false;
    state.nativeGameplayActive=state.nativeEndSubmitted?local_server_match_end::GetState().nativeActive
        :state.gameStartSubmitted&&local_server_game_start::GetState().gameplayActive;
    if(state.enabled){
        // Root pairs BeforeWorldTick with the actual driver's original
        // dispatch/flush delta. Consume that pair exactly once before match
        // readiness and asynchronous equipment are sampled this frame.
        if(!PollNpcAuthority(delta))return false;
    }
    // Any original callback during NPC work can also change life.
    // Check all old tuples before match Poll can call its fatal bool hook.
    if(!PreflightHeroDamageRetirements())return state.error==0&&PollPendingControl();
    if(state.enabled){
        armor_poll_performance::Scope measurementMatchPoll(armor_poll_performance::Slot::HostMatchPoll);
        if(!local_server_match::Poll(delta))return Fail(local_server_match::GetState().error,local_server_match::GetState().stage);
    }
    if(state.error)return false;
    if(state.stopping)return RequestStop();
    if(state.enabled)for(const auto& p:local_server_match::AuthorityPlayers())if(p.present&&!p.alive&&loadouts[p.index].used)
        local_server_equipment::SetPurchasesEnabled(p.index,false);
    {
    armor_poll_performance::Scope measurementEquipment(armor_poll_performance::Slot::HostEquipmentPoll);
    if(!local_server_equipment::Poll(static_cast<float>((std::min)(delta,1.)))){
        for(const auto& p:local_server_equipment::players)if(p.registered&&p.error)return Fail(p.error,p.stage);
        return Fail(local_server_equipment::State().error,local_server_equipment::State().stage);
    }
    }
    if(state.enabled&&!Practice()&&!local_server_armor::Poll()){
        const auto& armor=local_server_armor::GetState();return Fail(armor.error?armor.error:5504,armor.stage);
    }
    {
    armor_poll_performance::Scope measurementLoadoutMaintenance(armor_poll_performance::Slot::HostLoadoutMaintenance);
    if(!AdvanceLoadouts()||!MaintainCurrentSkinPreference()||!AdvanceRequests())return false;
    }
    if(state.enabled&&local_server_effect_authority::GetState().error){
        const auto& effects=local_server_effect_authority::GetState();return Fail(effects.error,effects.stage);
    }
    if(state.enabled&&JettPractice()&&!local_server_starting_equipment::Poll())
        return Fail(local_server_starting_equipment::GetState().error,local_server_starting_equipment::GetState().stage);
    if(!local_server_abilities::Poll())return Fail(local_server_abilities::GetState().error,local_server_abilities::GetState().stage);
    if(HeroPractice()&&!local_server_hero_abilities::Poll()){
        if(local_server_hero_event_authority::GetState().error)return HeroAuthorityFailure();
        return Fail(local_server_hero_abilities::GetState().error,local_server_hero_abilities::GetState().stage);
    }
    if(HeroPractice()&&!local_server_hero_authority::PollHost(state.pawn,!state.stopping&&!local_server_match::GetState().ended,false)){
        return HeroAuthorityFailure();
    }
    if(HeroPractice()&&(!local_server_hero_event_authority::SetPolicy(true,!state.stopping&&!local_server_match::GetState().ended)
       ||!local_server_hero_event_authority::Poll()))return HeroAuthorityFailure();
    if(HeroPractice()&&!local_server_hero_resource_init::Poll())return HeroAuthorityFailure();
    if(HeroPractice())local_server_hero_skill_readback::Poll(state.pawn,state.humanEpoch);
    if(state.host&&HeroPractice()&&local_server_hero_selection::SelectionIndex()==1){
        if(!local_server_phoenix_c_native_observer::GetState().initialized)local_server_phoenix_c_native_observer::InitHost(state.world);
        local_server_phoenix_c_native_observer::Poll(state.pawn,state.humanEpoch);
        if(!local_server_phoenix_c_throw_result::GetState().initialized)local_server_phoenix_c_throw_result::InitHost(state.world);
        if(!local_server_phoenix_c_actor_gate::GetState().initialized)local_server_phoenix_c_actor_gate::InitHost(state.world);
        local_server_phoenix_c_actor_gate::Poll(state.pawn,state.humanEpoch);
        if(!local_server_phoenix_c_result_observer::GetState().initialized)local_server_phoenix_c_result_observer::InitHost(state.world);
        if(!local_server_phoenix_c_fire_gate_readback::GetState().initialized)local_server_phoenix_c_fire_gate_readback::InitHost(state.world);
        local_server_phoenix_c_fire_gate_readback::Poll(state.pawn,state.humanEpoch);
        local_server_phoenix_c_state_counter::Poll(state.pawn,state.humanEpoch);
    }
    if(state.enabled){
        if(HeroPractice()){
            if(!local_server_hero_ultimate_authority::SetPolicy(!state.stopping&&!local_server_match::GetState().ended,true))return HeroAuthorityFailure();
            for(const auto& registered:local_server_hero_abilities::GetPlayers())
                if(registered.used&&!registered.retired&&registered.authority&&registered.context.sourceReady
                   &&registered.context.pawn==state.pawn&&registered.epoch==state.humanEpoch)
                    if(!local_server_hero_ultimate_authority::RegisterHuman(state.pawn,state.humanEpoch))return HeroAuthorityFailure();
            if(!local_server_hero_ultimate_authority::Poll())return HeroAuthorityFailure();
        }
        {
        armor_poll_performance::Scope measurementCurrentWeaponBind(armor_poll_performance::Slot::HostWeaponBindLoop);
        bool bindDiagnosticsPending=false;
        for(size_t n=0;n<MaxPlayers;++n)if(loadouts[n].used&&loadouts[n].complete){const auto p=local_server_equipment::State(n);
            if(p&&p->current&&!local_server_combat::BindWeaponForHostTick(n,p->current,bindDiagnosticsPending))return Fail(local_server_combat::GetState().error,local_server_combat::GetState().stage);
        }
        if(!local_server_combat::FinishHostTickBindDiagnostics(bindDiagnosticsPending))return Fail(local_server_combat::GetState().error,local_server_combat::GetState().stage);
        }
        {
        // Includes the existing Poll wrapper's CacheDiagnostics, not just PollImpl.
        armor_poll_performance::Scope measurementCombat(armor_poll_performance::Slot::HostCombatPollAndCache);
        if(!local_server_combat::Poll((std::min)(delta,1.)))return Fail(local_server_combat::GetState().error,local_server_combat::GetState().stage);
        }
        {
        armor_poll_performance::Scope measurementPostCombatTail(armor_poll_performance::Slot::HostPostCombatTail);
        if(!Practice()&&!local_server_kill_ammo::Poll()){const auto& ammo=local_server_kill_ammo::GetState();return Fail(ammo.error?ammo.error:5486,ammo.stage);}
        if(!Practice()&&!local_server_pickups::Poll()){const auto& pickups=local_server_pickups::GetState();return Fail(pickups.error?pickups.error:7918,pickups.stage);}
        if(HeroDamageActive()&&!local_server_ability_damage_integration::Poll(local_server_match::Now())){
            if(HeroDamageFault())return false;
            return Fail(4867,"local_server_runtime_damage_compositor_poll_failed");
        }
        const auto& combat=local_server_combat::GetState();
        if(state.configurationPending&&combat.ammoPolicyKnown
           &&combat.infiniteAmmo==((local_server_match::Configuration().config.flags&training_control::InfiniteAmmo)!=0)){
            state.appliedRevision=state.pendingRevision;state.configurationPending=false;
        }
        // Root's real actor-channel writer runs independently. Defer the first
        // RPC until its owned PC channel exists; NPC channel/GUID deferrals are
        // handled by Send. There is no own-human capture/physics pairing here.
        if(state.humanAdopted&&local_server_character_stream::ActorChannelExists(state.connection,state.controller)){
            std::array<void*,MaxPlayers> pawns{};size_t count=0;
            for(const auto& p:local_server_match::AuthorityPlayers())if(p.present&&p.pawn)pawns[count++]=p.pawn;
            if(!local_server_character_stream::Send(state.connection,state.controller,pawns.data(),count))
                return Fail(local_server_character_stream::GetState().error,local_server_character_stream::GetState().stage);
        }
        if(!PollNativeEnd())return false;
        if(!HumanLifeWaiting()&&!CosmeticsResult(local_server_cosmetics::Poll(state.connection)))return false;
        }
    }
    {
    armor_poll_performance::Scope measurementReadyRefresh(armor_poll_performance::Slot::HostReadyRefresh);
    RefreshReady();
    }
    CaptureFeatureSnapshot();
    return local_server_control::Poll()?true:Fail(local_server_control::GetState().error,local_server_control::GetState().stage);
}
inline bool TickHost(double delta){__try{return TickHostImpl(delta);}__except(CaptureException(GetExceptionInformation())){return false;}}

inline bool TickClientImpl(double delta,void* pc,void* ps,void* pawn){
    if(!Thread()||state.host||!std::isfinite(delta)||delta<0)return Fail(4828,"local_server_runtime_client_tick_context_invalid");
    if(state.error)return false;if(state.stopping)return local_server_control::Poll();
    if(!CurrentWorld())return Fail(4827,"local_server_runtime_world_driver_changed_requires_new_pair");
    ++state.clientTicks;
    if(!pc||!ps||!pawn){
        if(pc&&ps)local_server_spectator_view::MaybeObserve(state.world,pc,nullptr);
        state.ready=false;state.stage="local_server_runtime_waiting_received_autonomous_character";return local_server_control::Poll();
    }
    if(Read<uint8_t>(pawn,0x224)!=2||Read<uint8_t>(pc,0x224)!=2||Read<void*>(pc,0x4b0)!=pawn
       ||Read<void*>(pc,0x470)!=ps||Read<void*>(ps,0x210)!=pc||Read<void*>(pawn,0x488)!=ps||Read<void*>(pawn,0x4a0)!=pc){
        // These native actor references have independent replicated arrival
        // order. A partial client graph is a waiting stage, never an authority
        // association to repair by assigning another object's owner locally.
        local_server_spectator_view::MaybeObserve(state.world,pc,nullptr);
        state.ready=false;state.stage="local_server_runtime_waiting_received_native_owner_links";
        return local_server_control::Poll();
    }
    if(state.humanAdopted&&(state.controller!=pc||state.playerState!=ps))return Fail(4830,"local_server_runtime_client_admitted_identity_changed");
    const bool sameLife=state.pawn==pawn&&Live(state.pawn,state.pawnIdentity);
    if(state.pawn&&!sameLife){
        if(state.enabled&&!CosmeticsResult(local_server_cosmetics::Retire(0,state.pawn)))return false;
        if(!local_server_abilities::RetirePlayer(state.pawn))return Fail(local_server_abilities::GetState().error,local_server_abilities::GetState().stage);
        if(HeroPractice())local_server_hero_skill_readback::Retire(state.pawn,state.humanEpoch);
        if(HeroPractice()&&!local_server_hero_resource_init::RetirePlayer(state.pawn,state.humanEpoch))return HeroAuthorityFailure();
        if(HeroPractice()&&!local_server_hero_abilities::RetirePlayer(state.pawn,state.humanEpoch))
            return Fail(local_server_hero_abilities::GetState().error,local_server_hero_abilities::GetState().stage);
        if(hooks.manageClientPresentation&&!local_server_client::Retire(state.pawn))return Fail(local_server_client::GetState().error,local_server_client::GetState().stage);
        ++state.retirements;
    }
    if(!sameLife){
        if(!Capture(pc,state.controllerIdentity)||!Capture(ps,state.stateIdentity)||!Capture(pawn,state.pawnIdentity))return Fail(4831,"local_server_runtime_client_owner_weak_identity_missing");
        state.controller=pc;state.playerState=ps;state.pawn=pawn;state.connection=Read<void*>(state.driver,0xc0);++state.humanEpoch;
        if(JettPractice()&&!local_server_abilities::RegisterPlayer(pc,ps,pawn,state.humanEpoch))return Fail(local_server_abilities::GetState().error,local_server_abilities::GetState().stage);
        if(HeroPractice()&&!local_server_hero_abilities::RegisterPlayer(pc,ps,pawn,local_server_hero_selection::SelectionIndex(),state.humanEpoch))
            return Fail(local_server_hero_abilities::GetState().error,local_server_hero_abilities::GetState().stage);
        if(state.enabled&&!CosmeticsResult(local_server_cosmetics::Register(0,pc,ps,pawn,0,true)))return false;
        state.humanAdopted=true;
    }
    if(!local_server_equipment::PollClient(pawn))return Fail(local_server_equipment::State().error,local_server_equipment::State().stage);
    if(state.enabled&&!Practice())local_server_armor::ObserveClient(state.world,pc,ps,pawn,state.humanEpoch);
    state.clientEquipmentObserved=local_server_equipment::ObserveClient(pawn,clientEquipment);
    local_server_remote_timeline_probe::RememberBorrowedOwnLife(state.world,pawn,state.pawnIdentity.index,state.pawnIdentity.serial,
        ps,state.stateIdentity.index,state.stateIdentity.serial);
    ObserveNativeChannels();clientDependencies=ObserveInventoryDependencies(clientEquipment.inventory,clientEquipment.current,clientEquipment.desired);
    if(!local_server_abilities::Poll())return Fail(local_server_abilities::GetState().error,local_server_abilities::GetState().stage);
    if(HeroPractice()&&!local_server_hero_abilities::Poll())
        return Fail(local_server_hero_abilities::GetState().error,local_server_hero_abilities::GetState().stage);
    if(HeroPractice()&&!local_server_hero_resource_init::Poll())return HeroAuthorityFailure();
    if(HeroPractice())local_server_hero_skill_readback::Poll(state.pawn,state.humanEpoch);
    if(hooks.manageClientPresentation){
        if((!local_server_client::GetState().initialized||local_server_client::GetState().retired||local_server_client::GetState().pawn!=pawn)
           &&!local_server_client::Initialize(state.world,pc,ps,pawn))
            return Fail(local_server_client::GetState().error,local_server_client::GetState().stage);
        if(!local_server_client::Poll(static_cast<float>((std::min)(delta,1.))))return Fail(local_server_client::GetState().error,local_server_client::GetState().stage);
        local_server_ui_vm_probe::Capture(state.world);
        if(!Practice()){
            static uint64_t nextHealObserverAttempt=0;
            if(!local_server_heal_observer::GetState().initialized&&GetTickCount64()>=nextHealObserverAttempt){
                nextHealObserverAttempt=GetTickCount64()+1000;
                local_server_heal_observer::InitClient(state.world);
            }
            local_server_pickups::PollClient(state.world,state.connection);
        }
    }
    if(local_server_client::GetState().error)return Fail(local_server_client::GetState().error,local_server_client::GetState().stage);
    if(state.enabled&&!CosmeticsResult(local_server_cosmetics::Poll(state.connection)))return false;
    RefreshReady();CaptureFeatureSnapshot();return local_server_control::Poll()?true:Fail(local_server_control::GetState().error,local_server_control::GetState().stage);
}
inline bool TickClient(double delta,void* pc,void* ps,void* pawn){
    __try{return TickClientImpl(delta,pc,ps,pawn);}__except(CaptureException(GetExceptionInformation())){return false;}
}

inline void RegistryReferenceAt(void* object,const char* source,const char* reason,bool fatal,int32_t player=-1,int32_t offset=-1,int32_t slot=-1,int32_t element=-1){
    if(!firstRegistryReference.observed||(fatal&&!firstRegistryReference.fatal)
       ||(!fatal&&!firstRegistryReference.fatal&&firstRegistryReference.offset<0&&offset>=0)){
        auto& ref=firstRegistryReference;ref={};ref.observed=true;ref.fatal=fatal;ref.object=object;
        ref.type=Read<void*>(object,0x10);ref.role=Read<uint8_t>(object,0x224);ref.player=player;
        ref.offset=offset;ref.slot=slot;ref.element=element;ref.source=source;ref.reason=reason;
        for(size_t n=0;n<3;++n)ref.className[n]=Read<uint32_t>(ref.type,0x18+n*4);
    }
    if(fatal)++state.graphInvalidReferences;
}
inline bool RegistryFailure(unsigned code,const char* reason,void* object=nullptr,const char* source="actor_registry",int32_t player=-1,int32_t offset=-1,int32_t slot=-1,int32_t element=-1){
    state.graphComplete=false;state.registryStage=reason;
    RegistryReferenceAt(object,source,reason,true,player,offset,slot,element);return Fail(code,reason);
}
inline void RegistryWaiting(const char* reason,void* object=nullptr,const char* source="dependency",int32_t player=-1,int32_t offset=-1,int32_t slot=-1,int32_t element=-1){
    state.graphComplete=false;state.ready=false;++state.graphDependencyDeferrals;
    if(!state.error)state.registryStage=reason;
    RegistryReferenceAt(object,source,reason,false,player,offset,slot,element);
}
inline bool AddActor(void* actor,const char* source="actor_registry",bool required=false){
    if(!actor)return required?RegistryFailure(4840,"local_server_runtime_required_actor_missing",actor,source):true;
    // Engine.Actor is3683900.645c570 is AresItem and is deliberately confined
    // to inventory references below; it is not a general actor-class guard.
    if(!original_players::IsA(actor,ActorClassGetter))return RegistryFailure(4840,"local_server_runtime_registry_reference_is_not_actor",actor,source);
    if(original_players::Derived(Read<void*>(actor,0x10),playerControllerClass)&&actor!=state.controller){++state.foreignControllersSkipped;return true;}
    if(Read<uint8_t>(actor,0x224)!=3)return RegistryFailure(4841,"local_server_runtime_registry_actor_not_authority",actor,source);
    Weak identity{};
    if(!Capture(actor,identity)){
        if(required)return RegistryFailure(4842,"local_server_runtime_required_actor_identity_invalid",actor,source);
        RegistryWaiting("local_server_runtime_waiting_native_actor_identity",actor,source);return true;
    }
    auto getWorld=Read<void*(*)(void*)>(Read<void**>(actor),0x1b8);
    if(!getWorld||getWorld(actor)!=state.world)return RegistryFailure(4843,"local_server_runtime_registry_actor_belongs_to_other_world",actor,source);
    for(size_t n=0;n<state.graphActors;++n)if(actorScratch[n]==actor)return true;
    if(state.graphActors==actorScratch.size())return RegistryFailure(4844,"local_server_runtime_actor_registry_capacity_exceeded",actor,source);
    actorScratch[state.graphActors++]=actor;return true;
}
inline bool BeginRegistry(bool bootstrapOnly){
    if(!Thread()||!state.host||state.error||state.stopping)return false;
    state.graphActors=0;state.bootstrapGraphActors=0;state.bootstrapGraphReady=false;state.bootstrapGraphOnly=bootstrapOnly;
    state.graphComplete=true;++state.graphVisits;firstRegistryReference={};equipmentGraphObservations={};
    state.registryStage="local_server_runtime_validating_native_actor_graph";
    if(!CurrentWorld())return RegistryFailure(4843,"local_server_runtime_registry_world_driver_changed");
    // Every entry point keeps the actual nonempty-subject name-cache barrier.
    // An empty bootstrap list while it is pending is intentional and nonfatal.
    state.namesReady=!local_server_names::GetState().initialized||!local_server_names::roster.count||local_server_names::GetState().ready;
    if(!state.namesReady){++state.nameDeferrals;RegistryWaiting("local_server_runtime_registry_waiting_both_name_caches");}
    return true;
}
inline bool BootstrapActors(){
    if(!AddActor(Read<void*>(state.world,0x178),"bootstrap_game_state",true))return false;
    if(state.humanAdopted){
        if(!Live(state.controller,state.controllerIdentity)||!Live(state.playerState,state.stateIdentity)
           ||Read<void*>(state.controller,0x470)!=state.playerState||Read<void*>(state.playerState,0x210)!=state.controller
           ||Read<void*>(state.controller,0x510)!=state.connection||Read<void*>(state.connection,0x38)!=state.controller
           ||Read<void*>(state.connection,0xa0)!=state.controller)
            return RegistryFailure(4845,"local_server_runtime_bootstrap_admitted_pc_ps_connection_changed",state.controller,"bootstrap_owner");
        if(!AddActor(state.controller,"bootstrap_owner_pc",true)||!AddActor(state.playerState,"bootstrap_owner_ps",true))return false;
        // The actual live owned pawn is indispensable for native inventory
        // correction. During a dead life, the match separately owns its corpse.
        if(!HumanLifeWaiting()){
            if(!Live(state.pawn,state.pawnIdentity)||Read<void*>(state.controller,0x4b0)!=state.pawn
               ||Read<void*>(state.pawn,0x4a0)!=state.controller||Read<void*>(state.pawn,0x488)!=state.playerState)
                return RegistryFailure(4845,"local_server_runtime_bootstrap_live_pawn_owner_changed",state.pawn,"bootstrap_owner_pawn");
            if(!AddActor(state.pawn,"bootstrap_owner_pawn",true))return false;
        }
    }
    state.bootstrapGraphActors=state.graphActors;state.bootstrapGraphReady=true;return true;
}
inline bool CheckItemReference(void* actor,const char* source,int32_t player,int32_t offset,int32_t slot=-1,int32_t element=-1){
    return !actor||original_players::IsA(actor,AresItemClassGetter)
        ||RegistryFailure(4846,"local_server_runtime_equipment_reference_is_not_ares_item",actor,source,player,offset,slot,element);
}
inline bool DiagnoseEquipmentGraph(size_t index,const local_server_equipment::Graph& graph,bool complete){
    auto& observation=equipmentGraphObservations[index];observation.used=true;observation.complete=complete;
    observation.slots=graph.slotCount;observation.actors=graph.actorCount;
    const auto p=local_server_equipment::State(index);
    if(!p)return RegistryFailure(4845,"local_server_runtime_equipment_player_record_missing",nullptr,"equipment_player",static_cast<int32_t>(index));
    observation.phase=static_cast<unsigned>(p->phase);observation.error=p->error;observation.ownership=local_server_equipment::Ownership(*p);
    if(observation.ownership)observation.dependencies=ObserveInventoryDependencies(p->inventory,p->current,p->desired,
        p->phase==local_server_equipment::Phase::Idle?nullptr:p->transaction.candidate);
    if(p->error)return RegistryFailure(p->error,p->stage,p->pawn,"equipment_player",static_cast<int32_t>(index));
    if(!complete){
        observation.stage="local_server_runtime_waiting_equipment_dependency_graph";
        RegistryWaiting(observation.stage,p->inventory,"equipment_graph",static_cast<int32_t>(index));
        if(!observation.ownership)return true;
        for(size_t n=0;n<local_server_equipment::SlotCount;++n){
            const auto slot=Read<void*>(p->inventory,0x1b8+n*8);const auto player=static_cast<int32_t>(index),slotIndex=static_cast<int32_t>(n);
            if(!slot){RegistryWaiting("local_server_runtime_waiting_native_inventory_slot",nullptr,"inventory_slot",player,static_cast<int32_t>(0x1b8+n*8),slotIndex);continue;}
            if(!original_players::IsA(slot,0x68f63c0)||Read<void*>(slot,0x40)!=p->inventory)
                return RegistryFailure(4846,"local_server_runtime_inventory_slot_type_or_owner_invalid",slot,"inventory_slot",player,static_cast<int32_t>(0x1b8+n*8),slotIndex);
            if(original_players::IsA(slot,0x69cf3d0)){
                const auto contents=Read<local_server_equipment::NativeArray<void*>>(slot,0x50);
                if(contents.count<0||contents.count>256||contents.capacity<contents.count||(contents.count&&!contents.data)
                   ||(contents.count&&!original_players::Readable(contents.data,static_cast<size_t>(contents.count)*8)))
                    return RegistryFailure(4844,"local_server_runtime_inventory_multi_slot_bounds_invalid",slot,"multi_slot",player,0x50,slotIndex);
                for(int32_t nItem=0;nItem<contents.count;++nItem)if(!CheckItemReference(Read<void*>(contents.data,static_cast<size_t>(nItem)*8),"multi_slot_content",player,0x50,slotIndex,nItem))return false;
            }else if(!CheckItemReference(Read<void*>(slot,0x38),"slot_content",player,0x38,slotIndex))return false;
        }
        for(size_t offset:{size_t(0x268),size_t(0x278),size_t(0x280),size_t(0x2a0),size_t(0x320)})
            if(!CheckItemReference(Read<void*>(p->inventory,offset),"inventory_reference",static_cast<int32_t>(index),static_cast<int32_t>(offset)))return false;
    }else observation.stage="local_server_runtime_equipment_dependency_graph_collected";
    return true;
}
inline bool AddEquipmentActor(size_t index,void* actor){
    const auto p=local_server_equipment::State(index);
    if(!CheckItemReference(actor,"equipment_actor",static_cast<int32_t>(index),-1))return false;
    if(!actor)return true;
    if(!p||!equipmentGraphObservations[index].ownership){RegistryWaiting("local_server_runtime_waiting_equipment_graph_owner",actor,"equipment_actor",static_cast<int32_t>(index));return true;}
    const auto owner=Read<void*>(actor,0x210),typedOwner=Read<void*>(actor,0x4a8);
    if(!owner||!typedOwner){RegistryWaiting("local_server_runtime_waiting_native_item_owner",actor,"equipment_actor",static_cast<int32_t>(index));return true;}
    if(owner!=p->pawn||typedOwner!=p->pawn)return RegistryFailure(4847,"local_server_runtime_equipment_actor_cross_player_owner",actor,"equipment_actor",static_cast<int32_t>(index));
    const auto before=state.graphActors;
    if(!AddActor(actor,"equipment_actor"))return false;
    if(state.graphActors>before)++equipmentGraphObservations[index].accepted;return true;
}
template<class Visit> inline bool GetBootstrapActors(Visit&& visit){
    if(!BeginRegistry(true))return false;if(!state.namesReady)return true;
    if(!BootstrapActors())return false;
    state.graphComplete=false;state.ready=false;state.registryStage="local_server_runtime_validated_bootstrap_actor_graph_only";
    for(size_t n=0;n<state.graphActors;++n)visit(actorScratch[n]);return true;
}
template<class Visit> inline bool GetActors(Visit&& visit){
    if(!BeginRegistry(false))return false;if(!state.namesReady)return true;
    if(!BootstrapActors())return false;
    // Snapshot existing channels only. Root creates/replicates after this
    // visitor, so first-frame channel absence is an observation, never a gate.
    ObserveNativeChannels();
    if(state.enabled){
        std::array<void*,MaxPlayers*6+129> actors{};
        const auto count=local_server_match::RegistryActors(actors.data(),actors.size());
        if(count>actors.size())return RegistryFailure(4844,"local_server_runtime_match_actor_registry_capacity_exceeded");
        for(size_t n=0;n<count;++n)if(!AddActor(actors[n],"match_actor"))return false;
        std::array<void*,MaxPlayers*2> combatActors{};
        const auto combatCount=local_server_combat::RegistryActors(combatActors.data(),combatActors.size());
        if(combatCount>combatActors.size())return RegistryFailure(4844,"local_server_runtime_combat_actor_registry_capacity_exceeded");
        for(size_t n=0;n<combatCount;++n)if(!AddActor(combatActors[n],"combat_actor"))return false;
        if(!Practice()){
            bool accepted=true;
            const bool visited=local_server_pickups::VisitActors([&](void* actor){if(accepted)accepted=AddActor(actor,"deathmatch_health_pack");});
            if(!visited||!accepted){if(state.error)return false;return RegistryFailure(7921,"local_server_runtime_pickup_actor_registry_invalid");}
        }
    }
    for(size_t n=0;n<MaxPlayers;++n)if(loadouts[n].used){
        local_server_equipment::Graph graph;
        // Slots/components stay in original Inventory v318. A pending item
        // graph cannot withhold the validated PC/pawn channels it depends on.
        const bool complete=local_server_equipment::CollectGraph(n,graph);
        if(!DiagnoseEquipmentGraph(n,graph,complete))return false;
        if(graph.actorCount>local_server_equipment::MaxReferences)return RegistryFailure(4844,"local_server_runtime_equipment_actor_registry_capacity_exceeded");
        for(size_t i=0;i<graph.actorCount;++i)if(!AddEquipmentActor(n,graph.actors[i]))return false;
    }
    if(!local_server_abilities::VisitReplicationGraph([](void* root,void* object){if(root==object)AddActor(root,"ability_actor");})){
        const auto& ability=local_server_abilities::GetState();
        if(ability.error)return RegistryFailure(ability.error,ability.stage);RegistryWaiting("local_server_runtime_waiting_ability_actor_graph");
    }
    if(state.enabled&&JettPractice()&&!local_server_starting_equipment::VisitReplicationGraph([](void* root,void*){AddActor(root,"authored_starting_equipment");})){
        const auto& starting=local_server_starting_equipment::GetState();
        if(starting.error)return RegistryFailure(starting.error,starting.stage);
        RegistryWaiting("local_server_runtime_waiting_authored_starting_equipment_graph");
    }
    if(HeroPractice()&&!local_server_hero_abilities::VisitReplicationGraph([](void* root,void*){AddActor(root,"authored_hero_ability_actor");})){
        const auto& abilities=local_server_hero_abilities::GetState();
        if(abilities.error)return RegistryFailure(abilities.error,abilities.stage);
        RegistryWaiting("local_server_runtime_waiting_original_hero_ability_graph");
    }
    if(state.host&&HeroPractice()&&!local_server_hero_authority::VisitReplicationGraph([](void* root,void*){AddActor(root,"authored_hero_authority_actor");})){
        const auto& authority=local_server_hero_authority::GetState();
        if(authority.error)return RegistryFailure(authority.error,authority.stage);
        RegistryWaiting("local_server_runtime_waiting_original_hero_authority_graph");
    }
    if(state.enabled&&!local_server_effect_authority::VisitReplicationGraph([](void* root,void*){AddActor(root,"native_continuous_effect_owner");})){
        const auto& effects=local_server_effect_authority::GetState();
        if(effects.error)return RegistryFailure(effects.error,effects.stage);
        RegistryWaiting("local_server_runtime_waiting_native_continuous_effect_graph");
    }
    if(!local_server_abilities::VisitWorldActors([](void* actor,const local_server_abilities::WorldActorInfo&){AddActor(actor,"ability_world_actor");},hooks.includeProjectileWorldActors)){
        const auto& ability=local_server_abilities::GetState();
        if(ability.error)return RegistryFailure(ability.error,ability.stage);RegistryWaiting("local_server_runtime_waiting_ability_world_actor_graph");
    }
    if(state.enabled&&!local_server_cosmetics::VisitReplicationGraph([](void* root,void*){AddActor(root,"cosmetic_native_component_owner");})){
        const auto& cosmetic=local_server_cosmetics::GetState();
        if(cosmetic.error)return RegistryFailure(cosmetic.error,cosmetic.stage);
        RegistryWaiting("local_server_runtime_waiting_native_cosmetic_component_graph");
    }
    if(state.error)return false;
    if(state.graphComplete)state.registryStage="local_server_runtime_validated_native_actor_graph";
    // Each yielded actor was validated independently. Completeness is a
    // readiness observation, not permission to suppress native bootstrap work.
    for(size_t n=0;n<state.graphActors;++n)visit(actorScratch[n]);return true;
}

inline Guid Id(const training_control::Guid& id){Guid out{};std::memcpy(&out,&id,sizeof(out));return out;}
inline Result Reply(Result result,const char* text,char* message,size_t capacity){
    state.commandStage=text;if(message&&capacity)strncpy_s(message,capacity,text,_TRUNCATE);return result;
}
inline Result DispatchImpl(Command command,const uint8_t* data,uint32_t count,char* message,size_t capacity){
    const auto reply=[&](Result result,const char* text){return Reply(result,text,message,capacity);};
    if(!Thread()||state.error||state.stopping||!state.enabled)return reply(Result::Unavailable,"local_server_runtime_session_not_accepting_commands");
    if(!local_server_control::ValidPayload(command,data,count))return reply(Result::Invalid,"local_server_runtime_invalid_command_payload");
    if(state.host==local_server_control::ClientUI(command))return reply(Result::Unavailable,"local_server_runtime_command_requires_other_role");
    if(!state.host){
        if(!state.humanAdopted||!local_server_client::GetState().initialized)return reply(Result::Unavailable,"local_server_runtime_client_original_ui_not_ready");
        const bool done=command==Command::ToggleBuyMenu?local_server_client::ToggleShop():command==Command::ToggleOptions?local_server_client::ToggleOptions():local_server_client::CloseUI();
        return reply(done?Result::Applied:Result::Unavailable,done?"local_server_runtime_original_client_ui_applied":"local_server_runtime_original_client_ui_not_available");
    }
    if(!state.matchStarted||!state.humanAdopted)return reply(Result::Unavailable,"local_server_runtime_waiting_authority_human_and_match");
    ++state.acceptedCommands;
    switch(command){
    case Command::SpectateBot:{
        bot_spectator_contract::Request request{};
        if(!bot_spectator_contract::Decode(data,count,request))return reply(Result::Invalid,"native_spectator_invalid_request");
        const auto result=local_server_spectator::Handle(request);
        return reply(result,local_server_spectator::GetState().stage);
    }
    case Command::HumanBots:{
        const auto outcome=human_bot_store::Handle(data,count);
        return reply(static_cast<Result>(outcome.outcome),outcome.message);
    }
    case Command::Configure:{
        training_control::Configuration configuration{};std::memcpy(&configuration,data,sizeof(configuration));
        const auto& previous=local_server_match::Configuration().config;
        if(configuration.mode!=previous.mode||configuration.totalPlayers!=previous.totalPlayers)
            return reply(Result::Unavailable,"local_server_runtime_mode_or_population_requires_new_owned_pair");
        if(configuration.revision<state.appliedRevision||configuration.revision<state.pendingRevision)
            return reply(Result::Invalid,"local_server_runtime_configuration_revision_is_stale");
        if(!local_server_match::Configure(configuration))return reply(Result::Failed,"local_server_runtime_native_match_configuration_rejected");
        state.pendingRevision=configuration.revision;state.configurationPending=true;
        return reply(Result::Pending,"local_server_runtime_configuration_waiting_native_ammo_rule_readback");
    }
    case Command::Routes:{
        training_control::RouteSet routes{};std::memcpy(&routes,data,count);
        if(!local_server_match::SetRoutes(routes))return reply(Result::Failed,"local_server_runtime_original_route_configuration_rejected");
        return reply(Result::Applied,"local_server_runtime_routes_saved_original_validation_pending");
    }
    case Command::Weapon:case Command::BuyWeapon:{
        training_control::Guid wire{};std::memcpy(&wire,data,sizeof(wire));const auto definition=all_weapons::Find(Id(wire));
        if(!definition||!definition->identityVerified)return reply(Result::Invalid,"local_server_runtime_weapon_not_in_original_catalog");
        if(!Queue(Preference(0,*definition)))return reply(Result::Unavailable,"local_server_runtime_equipment_request_queue_full");
        preferredHumanWeapon=definition->id;humanWeaponSelected=true;
        return reply(Result::Pending,"local_server_runtime_authority_weapon_request_queued");
    }
    case Command::Skin:case Command::SaveSkinSelection:{
        training_control::Skin wire{};std::memcpy(&wire,data,sizeof(wire));const auto definition=all_weapons::Find(Id(wire.weapon));
        if(!definition||!definition->identityVerified)return reply(Result::Invalid,"local_server_runtime_skin_weapon_not_in_original_catalog");
        all_skins::Selection selection{};
        if(!all_skins::Resolve(*definition,Id(wire.skin),Id(wire.level),Id(wire.chroma),selection))return reply(Result::Invalid,"local_server_runtime_skin_relationship_not_in_original_catalog");
        if(command==Command::Skin&&requestCount+(requestInFlight?1U:0U)==RequestCapacity)return reply(Result::Unavailable,"local_server_runtime_equipment_request_queue_full");
        auto& preferred=local_server_equipment::preferences[0][static_cast<size_t>(definition-all_weapons::definitions)];
        preferred={};preferred.weapon=definition->id;preferred.skin=selection.skin;preferred.level=selection.level;preferred.chroma=selection.chroma;
        if(command==Command::SaveSkinSelection)return reply(Result::Applied,"local_server_runtime_canonical_skin_preference_saved");
        if(!Queue(Preference(0,*definition)))return reply(Result::Unavailable,"local_server_runtime_equipment_request_queue_full");
        preferredHumanWeapon=definition->id;humanWeaponSelected=true;
        return reply(Result::Pending,"local_server_runtime_authority_skin_request_queued");
    }
    case Command::ValidateRoutes:
        return training_bot_controller::ValidateRoutes(local_server_match::Now())?reply(Result::Pending,"local_server_runtime_original_route_validation_requested")
            :reply(Result::Unavailable,"local_server_runtime_original_route_validator_not_ready");
    case Command::ValidateSkinCatalog:
        return all_skins::ValidateCatalog(skinCatalogValidation)?reply(Result::Applied,"local_server_runtime_original_skin_catalog_validated")
            :reply(Result::Failed,"local_server_runtime_original_skin_catalog_validation_failed");
    case Command::ResolveMapPoint:{
        training_control::Point point{};std::memcpy(&point,data,sizeof(point));Vec feet{};original_players::CharacterDefaults dimensions;
        if(!original_players::ProbeDefaults(local_server_hero_selection::SelectionPath(),dimensions)
           ||!local_server_match::Supported(local_server_match::Vector(point),dimensions,feet))
            return reply(Result::Unavailable,"local_server_runtime_map_point_has_no_original_ground_or_capsule_clearance");
        if(message&&capacity)snprintf(message,capacity,"{\"position\":[%.5f,%.5f,%.5f],\"ground\":true,\"capsule\":true}",feet.x,feet.y,feet.z);
        state.commandStage="local_server_runtime_map_point_ground_and_capsule_verified";return Result::Applied;
    }
    case Command::RestartMatch:
        // Existing Python pair ownership must create a new initialized pair.
        // A stop acknowledgement would not constitute a match restart.
        return reply(Result::Unavailable,"local_server_runtime_restart_requires_outer_owned_pair_restart");
    case Command::Effect:{
        const auto result=local_server_cosmetics::DispatchEffect(data,count,message,capacity);CaptureFeatureSnapshot();
        if(local_server_cosmetics::GetState().error||local_server_cosmetics::GetState().restartRequired)CosmeticsResult(false);
        state.commandStage=local_server_cosmetics::GetState().commandStage;return result;
    }
    case Command::TestPlayerNearBot:case Command::TestAudioRegistration:
        return reply(Result::Unavailable,"local_server_runtime_network_test_fixture_not_implemented");
    default:return reply(Result::Invalid,"local_server_runtime_unknown_command");
    }
}
inline Result Dispatch(Command command,const uint8_t* data,uint32_t count,char* message,size_t capacity){
    __try{return DispatchImpl(command,data,count,message,capacity);}__except(CaptureException(GetExceptionInformation())){
        return Reply(Result::Failed,"local_server_runtime_command_exception_requires_new_pair",message,capacity);
    }
}

inline const char* Phase(local_server_match::Phase phase){
    return phase==local_server_match::Phase::Alive?"alive":phase==local_server_match::Phase::DeadWaiting?"dead_waiting_respawn"
        :phase==local_server_match::Phase::Rebuilding?"rebuilding":"absent";
}
inline void JsonGuid(local_server_match::Json& out,const Guid& id){out.Add("\"%08x%08x%08x%08x\"",id.words[0],id.words[1],id.words[2],id.words[3]);}
inline void CaptureNpcSnapshot(){
    const auto& s=local_server_npc_input::GetState();
    if(s.gameThread&&s.gameThread!=GetCurrentThreadId())return;
    const auto now=GetTickCount64();
    if(!s.error&&!s.unsupported&&(s.frameArmed||now<nextNpcSnapshotAt))return;
    nextNpcSnapshotAt=now+100;
    local_server_match::Json out;const auto b=[](bool value){return value?"true":"false";};
    const auto number=[&](const char* name,double value){out.Add(",\"%s\":",name);if(std::isfinite(value))out.Add("%.9g",value);else out.Add("null");};
    const auto vector=[&](const char* name,const Vec& value){out.Add(",\"%s\":",name);if(original_players::Finite(value))out.Vector(value);else out.Add("null");};
    out.Add("{\"schema_version\":1,\"cached_native_fields_only\":true,\"runtime_acceptance_verified\":false,\"stage\":");out.String(s.stage);
    const struct {const char* name;bool value;} flags[]{
        {"initialized",s.initialized},{"profiles_verified",s.profilesVerified},{"unsupported",s.unsupported},
        {"frame_armed",s.frameArmed},{"frame_adapter_allowed",s.frameAdapterAllowed},{"in_poll",s.inPoll},{"last_dispatch_matched",s.lastDispatchMatched},
        {"last_frame_aborted_unverified",s.lastFrameAbortedUnverified},{"cleanup_snapshot_pending",s.cleanupSnapshotPending},{"cleanup_delta_differs",s.cleanupDeltaDiffers}
    };for(const auto& flag:flags)out.Add(",\"%s\":%s",flag.name,b(flag.value));
    const struct {const char* name;uint64_t value;} numbers[]{
        {"error",s.error},{"exception_rva",s.exceptionRva},{"game_thread",s.gameThread},{"bound",s.bound},{"last_index",s.lastIndex},
        {"profile_failure_rva",s.profileFailureRva},{"frames",s.frames},{"polls",s.polls},{"manual_frames",s.manualFrames},
        {"neutral_frames",s.neutralFrames},{"native_yields",s.nativeYields},{"retirements",s.retirements},
        {"snapshots",s.snapshots},{"unpaired_skips",s.unpairedSkips},{"physics_completed",s.physicsCompleted},
        {"runtime_unarmed_poll_skips",state.npcUnarmedPollSkips},{"dispatch_calls",s.dispatchCalls},{"dispatch_rejected",s.dispatchRejected},
        {"unverified_frame_aborts",s.unverifiedFrameAborts},{"failed_manual_aborts",s.failedManualAborts},
        {"failed_manual_abort_dtor_calls",s.failedManualAbortDtorCalls},{"failed_manual_abort_error",s.failedManualAbortError},
        {"failed_manual_abort_exception_rva",s.failedManualAbortExceptionRva},
        {"frame_snapshot_at_ms",s.frameSnapshotAtMs},{"cleanup_boundary_at_ms",s.cleanupBoundaryAtMs},
        {"cleanup_boundary_calls",s.cleanupBoundaryCalls},{"cleanup_snapshot_frame",s.cleanupSnapshotFrame},
        {"sender_native_capture_scopes",local_server_character_stream::GetState().inNativeCaptures},
        {"sender_manual_capture_scopes",local_server_character_stream::GetState().manualCaptureScopes},
        {"sender_saved_move_dtor_scopes",local_server_character_stream::GetState().savedMoveDtorScopes}
    };for(const auto& item:numbers)out.Add(",\"%s\":%llu",item.name,static_cast<unsigned long long>(item.value));
    out.Add(",\"function_checks_run\":%u,\"failed_function_index\":%u,\"failed_function\":",s.functionChecksRun,s.failedFunctionIndex);out.String(s.failedFunction);
    out.Add(",\"function_checks\":[");
    for(size_t n=0;n<s.functionChecks.size();++n){const auto& check=s.functionChecks[n];
        out.Add("%s{\"name\":",n?",":"");out.String(check.name);
        out.Add(",\"checked\":%s,\"found\":%s,\"logical_bytes\":%u,\"storage_bytes\":%u,\"actual_bytes\":%u,\"expected_flags\":%u,\"actual_flags\":%u,\"expected_exec\":%llu,\"actual_exec\":%llu,\"expected_owner\":\"%p\",\"actual_owner\":\"%p\",\"bytes_match\":%s,\"flags_match\":%s,\"exec_match\":%s,\"owner_match\":%s}",
            b(check.checked),b(check.found),static_cast<unsigned>(check.expectedBytes),static_cast<unsigned>(check.storageBytes),static_cast<unsigned>(check.actualBytes),
            check.expectedFlags,check.actualFlags,static_cast<unsigned long long>(check.expectedExec),static_cast<unsigned long long>(check.actualExec),check.expectedOwner,check.actualOwner,
            b(check.bytesMatch),b(check.flagsMatch),b(check.execMatch),b(check.ownerMatch));
    }out.Add("]");
    out.Add(",\"last_dispatch_object\":\"%p\",\"last_dispatch_table\":\"%p\",\"last_dispatch_target\":\"%p\",\"last_dispatch_target_rva\":%llu",
        s.lastDispatchObject,s.lastDispatchTable,s.lastDispatchTarget,static_cast<unsigned long long>(s.lastDispatchTargetRva));
    number("frame_delta",s.frameDelta);number("elapsed",s.elapsed);
    number("cleanup_snapshot_delta",s.cleanupSnapshotDelta);number("cleanup_current_post_world_delta",s.cleanupCurrentPostWorldDelta);
    out.Add(",\"world\":\"%p\",\"driver\":\"%p\",\"game_state\":\"%p\",\"fixed_step\":\"%p\",\"entries\":[",s.world,s.driver,s.gameState,s.fixedStep);
    bool comma=false;
    for(size_t n=1;n<local_server_npc_input::entries.size();++n){const auto& e=local_server_npc_input::entries[n];if(!e.present)continue;
        out.Add("%s{\"index\":%zu,\"epoch\":%u,\"error\":%u,\"stage\":",comma?",":"",n,e.epoch,e.error);out.String(e.stage);
        out.Add(",\"owner\":");out.String(e.owner==local_server_npc_input::Owner::Native?"native":e.owner==local_server_npc_input::Owner::Adapter?"adapter":"unknown");
        const struct {const char* name;bool value;} entryFlags[]{
            {"present",e.present},{"alive",e.alive},{"ready",e.ready},{"native_locally_controlled",e.nativeLocal},
            {"snapshot",e.snapshot},{"has_intent",e.hasIntent},{"crouch_metadata",e.crouchMetadata},{"last_jump",e.lastJump},
            {"root_physics_class_valid",e.rootPhysicsClassValid},{"root_physics_owner_matches",e.rootPhysicsOwnerMatches},
            {"root_physics_target_matched",e.rootPhysicsTargetMatched},{"root_physics_dispatched",e.rootPhysicsDispatched}
        };for(const auto& flag:entryFlags)out.Add(",\"%s\":%s",flag.name,b(flag.value));
        const struct {const char* name;uint64_t value;} entryNumbers[]{
            {"clock_index",e.index},{"clock_base_offset",e.baseOffset},{"frame_steps",e.frameSteps},{"raw_before",e.rawBefore},
            {"public_wire_index",e.commonWireIndex},{"public_target_index",e.commonTargetIndex},{"public_unserved_steps",e.commonUnservedSteps},
            {"steps_before",e.stepsBefore},{"total_moves_before",e.totalMovesBefore},{"total_moves_after",e.totalMovesAfter},
            {"capture_before",e.captureBefore},{"snapshot_frame",e.snapshotFrame},{"submissions",e.submissions},{"capture_generation",e.captureGeneration},
            {"manual_frames",e.manualFrames},{"neutral_frames",e.neutralFrames},{"native_observations",e.nativeObservations},
            {"native_advances",e.nativeAdvances},{"native_capture_observations",e.nativeCaptureObservations},
            {"clock_advances",e.clockAdvances},{"before_frames",e.beforeFrames},{"physics_calls",e.physicsCalls},
            {"physics_completed",e.physicsCompleted},{"captures",e.captures},{"frame_scopes",e.frameScopes},{"delegates",e.delegates},
            {"walk_calls",e.walkCalls},{"crouch_writes",e.crouchWrites},{"crouch_calls",e.crouchCalls},
            {"jump_calls",e.jumpCalls},{"face_calls",e.faceCalls},{"input_calls",e.inputCalls},{"root_physics_queries",e.rootPhysicsQueries},
            {"root_physics_result",e.rootPhysicsResult},{"root_physics_process_event_rva",e.rootPhysicsProcessEventRva},
            {"root_physics_native_predicate_rva",e.rootPhysicsNativePredicateRva}
        };for(const auto& item:entryNumbers)out.Add(",\"%s\":%llu",item.name,static_cast<unsigned long long>(item.value));
        const auto* capture=local_server_character_stream::Existing(e.movement);
        out.Add(",\"manual_capture\":{\"binding_present\":%s,\"pending\":%s,\"saved_move_constructed\":%s,\"dtor_attempted\":%s,\"dtor_completed\":%s,\"abort_attempted\":%s,\"aborted_unverified\":%s,\"pending_index\":%u}",
            b(capture!=nullptr),b(capture&&capture->manualPending),b(capture&&capture->savedMoveConstructed),
            b(capture&&capture->savedMoveDtorAttempted),b(capture&&capture->savedMoveDtorCompleted),
            b(capture&&capture->manualAbortAttempted),b(capture&&capture->manualAbortedUnverified),capture?capture->pendingIndex:0);
        number("accumulated_before",e.accumulatedBefore);number("budget",e.budget);number("dropped_seconds",e.droppedSeconds);number("submitted_at",e.submittedAt);
        number("public_budget_supplied_seconds",e.commonBudgetSuppliedSeconds);
        out.Add(",\"pc\":\"%p\",\"ps\":\"%p\",\"pawn\":\"%p\",\"movement\":\"%p\",\"root\":\"%p\",\"input\":\"%p\",\"inventory\":\"%p\",\"stream\":\"%p\",\"prediction\":\"%p\",\"clock\":\"%p\",\"time_shift\":\"%p\"",
            e.pc,e.ps,e.pawn,e.movement,e.root,e.input,e.inventory,e.stream,e.prediction,e.clock,e.timeShift);
        out.Add(",\"root_physics_class\":\"%p\",\"root_physics_table\":\"%p\",\"root_physics_process_event\":\"%p\",\"root_physics_native_predicate\":\"%p\"",
            e.rootPhysicsClass,e.rootPhysicsTable,e.rootPhysicsProcessEvent,e.rootPhysicsNativePredicate);
        vector("intent",e.latest.worldDirection);vector("intent_rotation",e.latest.rotation);vector("consumed_input",e.consumedInput);
        vector("position_before",e.positionBefore);vector("position_after",e.positionAfter);vector("velocity_after",e.velocityAfter);
        const auto& movement=e.movementObservation;
        out.Add(",\"movement_observation\":{\"world_frame\":%llu,\"physical_index\":%u,\"phase_mask\":%u,\"settings_known\":%s,\"plane_flags\":%u,\"constrain_to_plane\":%s,\"snap_to_plane_at_start\":%s,\"plane_axis\":%u,\"movement_mode\":%u,\"custom_movement_mode\":%u,\"ground_movement_mode\":%u,\"updated_primitive_is_root\":%s,\"capsule_is_root\":%s",
            static_cast<unsigned long long>(movement.worldFrame),movement.physicalIndex,unsigned(movement.stages),b(movement.settingsKnown),
            unsigned(movement.planeFlags),b((movement.planeFlags&0x10)!=0),b((movement.planeFlags&0x20)!=0),unsigned(movement.planeAxis),
            unsigned(movement.movementMode),unsigned(movement.customMode),unsigned(movement.groundMode),b(movement.primitiveIsRoot),b(movement.capsuleIsRoot));
        vector("plane_normal",movement.planeNormal);vector("plane_origin",movement.planeOrigin);
        vector("before_world",movement.beforeWorld);vector("after_world",movement.afterWorld);
        vector("after_capture",movement.afterCapture);vector("after_frame_scope",movement.afterScope);vector("after_delegate",movement.afterDelegate);
        out.Add(",\"phase_mask_meaning\":\"1 before_world;2 after_world;4 after_capture;8 after_frame_scope;16 after_delegate\",\"source\":\"same_weak_root_component_world_translation\",\"read_only\":true}");
        out.Add("}");comma=true;
    }
    out.Add("],\"public_time_domain\":");local_server_npc_time_domain::AddJson(out);out.Add("}");
    AcquireSRWLockExclusive(&npcSnapshotLock);npcSnapshotJson.swap(out.value);ReleaseSRWLockExclusive(&npcSnapshotLock);
}
inline std::string NpcStateJson(){
    struct SharedLock {SRWLOCK* lock;explicit SharedLock(SRWLOCK* value):lock(value){AcquireSRWLockShared(lock);}~SharedLock(){ReleaseSRWLockShared(lock);}};
    SharedLock lock(&npcSnapshotLock);return npcSnapshotJson;
}
inline void WriteNpcState(FILE* file){
    if(!file)return;const auto json=NpcStateJson();std::fwrite(json.data(),1,json.size(),file);
}
inline bool WriteNpcDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    const auto json=NpcStateJson();const auto written=std::fwrite(json.data(),1,json.size(),file);
    const bool okay=written==json.size()&&!std::ferror(file);std::fclose(file);return okay;
}
inline void ChannelJson(local_server_match::Json& out,const NativeChannelObservation& observation){
    out.Add("{\"actor\":\"%p\",\"channel\":\"%p\",\"found\":%s,\"closing\":%s,\"open_ack\":%s,\"flags\":%u,\"open_flags\":%u}",
        observation.actor,observation.channel,observation.found?"true":"false",(observation.flags&2)?"true":"false",
        observation.openAck?"true":"false",static_cast<unsigned>(observation.flags),static_cast<unsigned>(observation.openFlags));
}
inline void InventoryDependenciesJson(local_server_match::Json& out,const InventoryDependencyObservation& observation){
    const auto& timestamp=observation.changeTimestamp;const bool finite=std::isfinite(timestamp.netTimestamp);
    out.Add("{\"inventory\":\"%p\",\"previous\":\"%p\",\"replicated_current\":\"%p\",\"correction_current\":\"%p\",\"correction_state\":%u,\"correction_published\":%s,\"initial_correction_processed\":%s,\"change_character\":[%d,%d],\"change_timestamp_finite\":%s,\"change_net_timestamp\":%.9g,\"change_respawn\":%d,\"current\":",
        observation.inventory,observation.previous,observation.replicatedCurrent,observation.correctionCurrent,
        static_cast<unsigned>(observation.correctionState),observation.correctionPublished?"true":"false",observation.initialCorrectionProcessed?"true":"false",
        timestamp.character.index,timestamp.character.serial,finite?"true":"false",finite?static_cast<double>(timestamp.netTimestamp):0.,timestamp.respawnNumber);
    ChannelJson(out,observation.current);out.Add(",\"desired\":");ChannelJson(out,observation.desired);
    out.Add(",\"candidate\":");ChannelJson(out,observation.candidate);out.Add("}");
}
inline void ClientEquipmentJson(local_server_match::Json& out){
    const auto b=[](bool value){return value?"true":"false";};const auto& observed=clientEquipment;
    out.Add("{\"observed\":%s,\"original_inventory\":%s,\"slots\":%d,\"owner_matches\":%s,\"components_present\":%s,\"appearance_fields_present\":%s,\"pawn\":\"%p\",\"owner\":\"%p\",\"typed_owner\":\"%p\",\"primary_data\":\"%p\",\"skin_data\":\"%p\",\"level_data\":\"%p\",\"chroma_data\":\"%p\",\"mesh_1p\":\"%p\",\"mesh_3p\":\"%p\",\"machine\":\"%p\",\"machine_predicted_state\":\"%p\",\"visual_or_fire_verified\":false,\"dependencies\":",
        b(state.clientEquipmentObserved),b(observed.originalInventory),observed.slotCount,b(observed.ownerMatches),b(observed.nativeComponentsPresent),b(observed.appearanceFieldsPresent),
        observed.pawn,observed.owner,observed.typedOwner,observed.primaryData,observed.skinData,observed.levelData,observed.chromaData,
        observed.mesh1P,observed.mesh3P,observed.machine,observed.predictedState);
    InventoryDependenciesJson(out,clientDependencies);
    out.Add(",\"native_correction_indices\":{\"server_publication\":%d,\"client_processed\":%d,\"correction_input\":%d,\"client_input\":%d}",
        observed.serverCorrectionSequence,observed.processedCorrectionSequence,
        observed.correctionInputSequence,observed.clientInputCorrectionSequence);
    const auto& owner=observed.ownership;
    out.Add(",\"native_prediction\":{\"observed\":%s,\"slots\":%u,\"items\":%u,\"owned_items\":%u,\"autonomous_items\":%u,\"simulated_items\":%u,\"authority_items\":%u,\"invalid_roles\":%u,\"machines\":%u,\"enabled_machines\":%u,\"predicted_states\":%u,\"authority_states\":%u,\"role_selected_states\":%u,\"complete\":%s,\"all_native_prediction_ready\":%s}}",
        b(observed.ownershipObserved),owner.validSlots,owner.items,owner.ownedItems,owner.autonomousItems,owner.simulatedItems,owner.authorityItems,owner.invalidRoles,
        owner.machines,owner.enabledMachines,owner.predictedStates,owner.authorityStates,owner.roleSelectedStates,b(owner.complete),b(owner.allNativePredictionReady));
}
inline void ObserveWeaponJson(local_server_match::Json& out){
    void* actor=nullptr;bool ready=false;
    if(state.host){const auto p=local_server_equipment::State(0);actor=p?p->current:nullptr;ready=p&&local_server_equipment::Ready(0);}
    else {actor=clientEquipment.current;ready=state.equipmentReady;}
    const auto definition=actor?all_weapons::FindByClass(Read<void*>(actor,0x10)):nullptr;
    ready=ready&&definition;
    out.Add("{\"ready\":%s,\"stage\":",ready?"true":"false");
    out.String(ready?"local_server_runtime_native_equipment_observed_visual_unverified":"local_server_runtime_waiting_original_current_equipment");
    if(ready){out.Add(",\"id\":");JsonGuid(out,definition->id);out.Add(",\"skin_id\":");JsonGuid(out,Read<Guid>(actor,0xfe8));
        out.Add(",\"level_id\":");JsonGuid(out,Read<Guid>(actor,0xff8));out.Add(",\"chroma_id\":");JsonGuid(out,Read<Guid>(actor,0x1038));
        out.Add(",\"name\":");out.String(definition->label);}
    out.Add("}");
}
inline void CaptureFeatureSnapshot(){
    if(state.gameThread&&state.gameThread!=GetCurrentThreadId())return;
    const auto now=GetTickCount64();
    if(!state.error&&!state.nativeEndPending&&now<nextFeatureSnapshotAt)return;
    nextFeatureSnapshotAt=now+100;
    armor_poll_performance::Scope measurementFeature(armor_poll_performance::Slot::FeatureSnapshotBuild);
    if(state.host&&state.initialized&&!state.closed)local_server_match_end::PollOriginalModePreflight(state.world);
    local_server_match::Json effects,cosmetics,weapon;
    local_server_cosmetics::WriteEffects(effects);ObserveWeaponJson(weapon);
    const auto& observed=local_server_cosmetics::GetState();const auto b=[](bool value){return value?"true":"false";};
    cosmetics.Add("{\"initialized\":%s,\"host\":%s,\"profiles_verified\":%s,\"world_bound\":%s,\"peer_bound\":%s,\"metadata_ready\":%s,\"error\":%u,\"restart_required\":%s,\"stage\":",
        b(observed.initialized),b(observed.host),b(observed.profilesVerified),b(observed.worldBound),b(observed.peerBound),b(observed.metadataReady),observed.error,b(observed.restartRequired));cosmetics.String(observed.stage);
    cosmetics.Add(",\"commands\":%llu,\"publications\":%llu,\"receipts\":%llu,\"native_map_writes\":%llu,\"material_graph_calls\":%llu,\"rendered_pixels_verified\":false,\"players\":[",
        static_cast<unsigned long long>(observed.commands),static_cast<unsigned long long>(observed.publications),static_cast<unsigned long long>(observed.receipts),
        static_cast<unsigned long long>(observed.nativeMapWrites),static_cast<unsigned long long>(observed.materialGraphCalls));
    bool comma=false;for(const auto& player:local_server_cosmetics::players)if(player.registered){
        cosmetics.Add("%s{\"index\":%u,\"epoch\":%u,\"source_ready\":%s,\"snapshot_ready\":%s,\"assets_ready\":%s,\"peer_scope\":%s,\"controls\":%zu,\"error\":%u,\"stage\":",
            comma?",":"",player.index,player.epoch,b(player.sourceReady),b(player.snapshotReady),b(player.assetsReady),b(player.peerScope),player.controlCount,player.error);
        cosmetics.String(player.stage);cosmetics.Add("}");comma=true;
    }cosmetics.Add("],\"gt_timing\":");
    local_server_cosmetics_timing::WriteJson(cosmetics);cosmetics.Add("}");auto ended=local_server_match_end::StateJson();
    // Both ControlStateJson (GT) and full diagnostics (observer thread) read
    // this cache. Only the bound GT may format mutable QPC aggregates.
    std::string performance;
    if(state.gameThread&&state.gameThread==GetCurrentThreadId())performance=armor_poll_performance::Json();
    AcquireSRWLockExclusive(&featureSnapshotLock);
    effectsSnapshotJson.swap(effects.value);cosmeticsSnapshotJson.swap(cosmetics.value);weaponSnapshotJson.swap(weapon.value);endSnapshotJson.swap(ended);
    if(!performance.empty())armorPerformanceSnapshotJson.swap(performance);
    ReleaseSRWLockExclusive(&featureSnapshotLock);
}
inline std::string FeatureSnapshotCopy(const std::string& snapshot){
    struct SharedLock {SRWLOCK* lock;explicit SharedLock(SRWLOCK* value):lock(value){AcquireSRWLockShared(lock);}~SharedLock(){ReleaseSRWLockShared(lock);}};
    SharedLock lock(&featureSnapshotLock);return snapshot;
}
inline void WeaponJson(local_server_match::Json& out){out.value+=FeatureSnapshotCopy(weaponSnapshotJson);}
inline std::string StateJsonImpl(bool compact){
    local_server_match::Json out;const auto b=[](bool value){return value?"true":"false";};
    out.Add("{\"schema_version\":1,\"ready\":%s,\"applied_revision\":%u,\"error\":",b(state.ready&&!state.error&&!state.stopping),state.appliedRevision);
    if(state.error)out.String(state.stage);else out.Add("null");
    out.Add(",\"stage\":");out.String(state.stage);out.Add(",\"map_id\":");out.String(state.enabled?local_server_control::Initial().mapId:"");
    out.Add(",\"host\":%s,\"configured_session\":%s,\"playable_verified\":false,\"error_code\":%u,\"command_stage\":",b(state.host),b(state.enabled),state.error);out.String(state.commandStage);
    out.Add(",\"pending_equipment_requests\":%zu,\"configuration_pending\":%s,\"native_gameplay_active\":%s,\"equipment_ready\":%s,\"abilities_ready\":%s,\"combat_ready\":%s,\"names_ready\":%s,\"graph_complete\":%s",
        state.pendingRequests,b(state.configurationPending),b(state.nativeGameplayActive),b(state.equipmentReady),b(state.abilitiesReady),b(state.combatReady),b(state.namesReady),b(state.graphComplete));
    out.Add(",\"death_preprocess_pending\":%s,\"death_preprocess_calls\":%llu,\"death_retirement_deferrals\":%llu",
        b(state.deathPreprocessPending),static_cast<unsigned long long>(state.deathPreprocessCalls),static_cast<unsigned long long>(state.deathRetirementDeferrals));
    out.Add(",\"starting_equipment_ready\":%s",b(state.startingEquipmentReady));
    if(state.host){out.Add(",\"configured_bot_roster\":");local_server_bot_roster_initial::WriteJson(out,[](size_t slot){const auto*p=local_server_match::Player(slot);return p&&p->playerIdAssigned&&p->playerId>0?p->playerId:0;});}
    if(state.host){out.Add(",\"human_bots\":");out.value+=local_server_humanlike_bots::SummaryJson();}
    if(state.host){out.Add(",\"spectator_authority\":");out.value+=local_server_spectator::Json();}
    if(state.host){
        const auto& spawn=initialHumanSpawn;
        out.Add(",\"initial_human_spawn\":{\"stage\":");out.String(spawn.stage);
        out.Add(",\"requests\":%llu,\"map_deferrals\":%llu,\"selected\":%s,\"map_ready\":%s,\"original_map_anchor\":%s,\"preserved_bootstrap\":%s,\"pawn_class\":\"%p\",\"bootstrap_center\":",
            static_cast<unsigned long long>(spawn.requests),static_cast<unsigned long long>(spawn.mapDeferrals),
            b(spawn.selected),b(spawn.mapReady),b(spawn.originalMapAnchor),b(spawn.preservedBootstrap),spawn.pawnClass);
        out.Vector(spawn.bootstrapCenter);out.Add(",\"feet\":");out.Vector(spawn.feet);
        out.Add(",\"center\":");out.Vector(spawn.center);out.Add(",\"rotation\":");out.Vector(spawn.rotation);out.Add("}");
    }
    if(state.host&&state.enabled&&local_server_combat::GetState().ammoPolicyKnown)out.Add(",\"infinite_ammo\":%s",b(local_server_combat::GetState().infiniteAmmo));
    const auto& match=local_server_match::GetState();
    out.Add(",\"ended\":%s,\"winner\":%d,\"end_reason\":%u,\"time_remaining\":%.4f,\"players\":[",b(state.host&&state.matchStarted&&match.ended),state.matchStarted?match.winner:-1,static_cast<unsigned>(state.matchStarted?match.endReason:local_server_match::EndReason::None),state.matchStarted?match.remaining:0.);
    bool comma=false;
    if(state.host&&state.matchStarted)for(const auto& p:local_server_match::AuthorityPlayers())if(p.present){
        const auto& ai=training_bot_controller::actors[p.index];
        out.Add("%s{\"index\":%zu,\"bot\":%s,\"alive\":%s,\"health\":%.4f,\"epoch\":%u,\"hero_index\":%u,\"kills\":%u,\"deaths\":%u,\"streak\":%u,\"feet\":",comma?",":"",p.index,b(!p.human),b(p.alive),p.health,p.epoch,p.hero,p.kills,p.deaths,p.streak);out.Vector(p.feet);
        local_server_bot_roster_initial::PlayerFields(out,p.index,p.human,p.playerId,p.playerIdAssigned);
        out.Add(",\"name\":");const auto label=original_players::Utf8(original_players::players[p.index].requestedName);out.String(label.c_str());
        out.Add(",\"phase\":");out.String(Phase(p.phase));out.Add(",\"stage\":");out.String(p.stage);
        out.Add(",\"profile\":{\"reaction_ms\":%.3f,\"aim_speed\":%.3f,\"accuracy\":%.5f,\"movement\":%.5f}}",ai.profile.reaction*1000,ai.profile.turnSpeed,ai.profile.accuracy,ai.profile.movement);comma=true;
    }
    out.Add("],\"routes\":[");comma=false;
    if(state.host&&state.matchStarted)for(uint32_t n=0;n<local_server_match::Configuration().routes.count;++n){const auto& route=local_server_match::Configuration().routes.items[n];const auto result=training_bot_controller::RouteValidationById(route.id);
        out.Add("%s{\"id\":%u,\"status\":",comma?",":"",route.id);out.String(result?training_bot_controller::RouteCheckStatusName(result->status):"pending");
        out.Add(",\"message\":");out.String(result?result->reason:"local_server_runtime_original_route_not_validated");out.Add("}");comma=true;}
    out.Add("],\"effects\":");out.value+=FeatureSnapshotCopy(effectsSnapshotJson);out.Add(",\"weapon\":");WeaponJson(out);
    out.Add(",\"cosmetics\":");out.value+=FeatureSnapshotCopy(cosmeticsSnapshotJson);
    if(state.host&&state.enabled){
        out.Add(",\"native_end_pending\":%s,\"native_end_submitted\":%s,\"native_end_stage\":",b(state.nativeEndPending),b(state.nativeEndSubmitted));out.String(state.nativeEndStage);
        out.Add(",\"native_match_end\":");out.value+=FeatureSnapshotCopy(endSnapshotJson);
    }
    if(!state.host){
        out.Add(",\"spectator\":");out.value+=local_server_spectator_view::Json();
        out.Add(",\"client_equipment\":");ClientEquipmentJson(out);
        out.Add(",\"ui_vm_probe\":");out.value+=local_server_ui_vm_probe::Json();
    }
    // The fixed V2 mailbox carries product state, not per-actor diagnostic
    // graphs. Full evidence remains in bridge-state.json at the same sampling
    // boundary. This keeps larger supported populations within StateCapacity.
    if(compact){out.Add(",\"detailed_diagnostics_in_bridge_state\":true}");return out.value;}
    out.Add(",\"registry\":{\"stage\":");out.String(state.registryStage);
    out.Add(",\"bootstrap_ready\":%s,\"bootstrap_only\":%s,\"bootstrap_actors\":%zu,\"validated_actors\":%zu,\"dependencies_complete\":%s,\"dependency_deferrals\":%llu,\"invalid_references\":%llu,\"first_reference\":",
        b(state.bootstrapGraphReady),b(state.bootstrapGraphOnly),state.bootstrapGraphActors,state.graphActors,b(state.graphComplete),
        static_cast<unsigned long long>(state.graphDependencyDeferrals),static_cast<unsigned long long>(state.graphInvalidReferences));
    if(firstRegistryReference.observed){const auto& ref=firstRegistryReference;
        out.Add("{\"fatal\":%s,\"player\":%d,\"offset\":%d,\"slot\":%d,\"element\":%d,\"object\":\"%p\",\"class\":\"%p\",\"class_name_words\":[%u,%u,%u],\"role\":%u,\"source\":",
            b(ref.fatal),ref.player,ref.offset,ref.slot,ref.element,ref.object,ref.type,ref.className[0],ref.className[1],ref.className[2],static_cast<unsigned>(ref.role));
        out.String(ref.source);out.Add(",\"reason\":");out.String(ref.reason);out.Add("}");
    }else out.Add("null");
    out.Add(",\"channel_snapshot_valid\":%s,\"channel_snapshot_connection\":\"%p\",\"channel_snapshot_actor_count\":%zu,\"channel_snapshot_precedes_root_replication\":%s",
        b(channelSnapshot.valid),channelSnapshot.connection,channelSnapshot.count,b(state.host));
    out.Add(",\"kill_display\":");out.value+=local_server_feedback_view::Json();
    out.Add(",\"death_pickups\":");out.value+=local_server_pickups::Json();
    out.Add(",\"kill_ammo\":");out.value+=local_server_kill_ammo::Json();
    out.Add(",\"pickup_recovery\":");out.value+=local_server_recovery::Json();
    out.Add(",\"client_heal_receipts\":");out.value+=local_server_heal_observer::Json();
    out.Add(",\"armor\":");out.value+=local_server_armor::Json();
    out.Add(",\"armor_qpc\":");out.value+=FeatureSnapshotCopy(armorPerformanceSnapshotJson);
    out.Add(",\"item_actor_channels\":");out.value+=ItemChannelProbeJson();
    out.Add(",\"equipment\":[");bool graphComma=false;
    for(size_t n=0;n<equipmentGraphObservations.size();++n){const auto& graph=equipmentGraphObservations[n];if(!graph.used)continue;
        out.Add("%s{\"index\":%zu,\"complete\":%s,\"ownership\":%s,\"slots\":%zu,\"actors\":%zu,\"accepted\":%zu,\"phase\":%u,\"error\":%u,\"stage\":",
            graphComma?",":"",n,b(graph.complete),b(graph.ownership),graph.slots,graph.actors,graph.accepted,graph.phase,graph.error);out.String(graph.stage);
        out.Add(",\"dependencies\":");InventoryDependenciesJson(out,graph.dependencies);out.Add("}");graphComma=true;
    }
    out.Add("]}");
    out.Add(",\"damage_lifecycle\":{\"boundary_pending\":%s,\"boundary_deferrals\":%llu,\"observation_only_frames\":%llu,\"close_pending\":%s,\"close_attempts\":%llu,\"close_pair_unverified\":%s,\"immediate_enabled\":false}",
        b(state.heroDamageBoundaryPending),static_cast<unsigned long long>(state.heroDamageBoundaryDeferrals),
        static_cast<unsigned long long>(state.npcObservationOnlyFrames),b(state.closePending),static_cast<unsigned long long>(state.closeAttempts),b(state.closePairUnverified));
    out.Add(",\"runtime\":{\"host_ticks\":%llu,\"client_ticks\":%llu,\"human_epoch\":%u,\"graph_actors\":%zu,\"name_deferrals\":%llu,\"foreign_controllers_skipped\":%llu,\"purchase_table_installed\":%s,\"purchase_profile_verified\":%s,\"other_pc_virtuals_preserved\":%s,\"purchase_requests\":%llu,\"own_human_physics_submitted\":false,\"client_presentation_managed\":%s,\"stopping\":%s,\"disconnected\":%s,\"exception_rva\":%llu}",
        static_cast<unsigned long long>(state.hostTicks),static_cast<unsigned long long>(state.clientTicks),state.humanEpoch,state.graphActors,static_cast<unsigned long long>(state.nameDeferrals),static_cast<unsigned long long>(state.foreignControllersSkipped),b(controllerHook.installed),b(controllerHook.profileVerified),b(controllerHook.otherSlotsPreserved),static_cast<unsigned long long>(controllerHook.purchases),b(hooks.manageClientPresentation),b(state.stopping),b(state.disconnected),static_cast<unsigned long long>(state.exceptionRva));
    const auto& connection=state.connectionObservation;
    out.Add(",\"admitted_connection\":{\"stage\":");out.String(connection.stage);
    out.Add(",\"calls\":%llu,\"skips\":%llu,\"open\":%llu,\"closed\":%llu,\"failures\":%llu,\"sampled_at_ms\":%llu,\"native_state13c\":%d,\"driver_client_count\":%d,\"weak_verified\":%s,\"state_readable\":%s,\"driver_verified\":%s,\"open_owners_verified\":%s,\"before_death_processing\":true,\"input_epoch_not_required\":true",
        static_cast<unsigned long long>(connection.calls),static_cast<unsigned long long>(connection.skips),
        static_cast<unsigned long long>(connection.open),static_cast<unsigned long long>(connection.closed),
        static_cast<unsigned long long>(connection.failures),static_cast<unsigned long long>(connection.sampledAtMs),
        connection.nativeState,connection.clientCount,b(connection.weakVerified),b(connection.stateReadable),
        b(connection.driverVerified),b(connection.openOwnersVerified));
    out.Add(",\"world_driver_verified\":%s,\"peer_list_readable\":%s,\"peer_list_shape_verified\":%s,\"retired_weak_empty_native_peers\":%s,\"native_state_closes\":%llu,\"retired_peer_observations\":%llu,\"captured_weak_token_present\":%s}",
        b(connection.worldDriverVerified),b(connection.peerListReadable),b(connection.peerListShapeVerified),
        b(connection.retiredWeakEmptyPeers),static_cast<unsigned long long>(connection.nativeStateCloses),
        static_cast<unsigned long long>(connection.retiredPeerObservations),b(connection.capturedTokenKnown));
    if(state.host&&state.matchStarted){out.Add(",\"match\":");out.value+=local_server_match::StateJson();}
    out.Add("}");
    return out.value;
}
inline std::string StateJson(){return StateJsonImpl(false);}
inline std::string ControlStateJson(){
    auto value=StateJsonImpl(true);
    if(value.size()>=training_control::StateCapacity)
        return local_server_control::WaitingJson("local_server_runtime_state_capacity_exceeded",true);
    return value;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    const auto json=StateJson();const auto written=std::fwrite(json.data(),1,json.size(),file);const bool okay=written==json.size()&&!std::ferror(file);std::fclose(file);return okay;
}
// A close request is persistent. Bridge continues calling this on
// later actual post-world GT boundaries even after stopping or an old error.
inline void RequestClose(){state.stopping=true;state.ready=false;state.closePending=true;}
inline bool PrepareClose(){
    if(state.closed)return true;
    if(state.initialized&&!Thread())return false;
    RequestClose();++state.closeAttempts;
    if(state.host&&!HostCloseBoundaryIdle())return false;
    FenceHeroDamageAdmission(); // Preserve any error, but independently restore safe owned hooks.
    if(state.host&&state.enabled&&local_server_npc_input::GetState().frameArmed){
        const auto& npc=local_server_npc_input::GetState();
        if(!npc.error&&!npc.unsupported){
            state.stage="local_server_runtime_close_waiting_real_post_world_pair_consumption";return false;
        }
        // A failed producer cannot be polled as a healthy paired frame. Keep
        // its actual fatal error and unverified pair; do not invent a receipt
        // or reset its queue/budget. Still restore our independent upper hooks.
        state.closePairUnverified=true;
    }
    if(HeroDamageActive()&&!local_server_ability_damage_integration::Close()){
        if(HeroDamageFault())return false;
        state.stage="local_server_runtime_close_waiting_outermost_damage_return";return false;
    }
    return true;
}
inline bool Close(){
    local_server_phoenix_c_actor_gate::Close();
    if(!local_server_phoenix_c_state_counter::Close())return false;
    local_server_phoenix_c_fire_gate_readback::Close();
    if(!local_server_phoenix_c_throw_result::Close())return false;
    if(!local_server_phoenix_c_result_observer::Close())return false;
    if(!local_server_phoenix_c_native_observer::Close())return false;
    local_server_hero_skill_readback::Close();
    if(state.closed)return true;
    if(!PrepareClose())return false;
    if(!StopDeathmatchExtras())return false;
    if(!local_server_heal_observer::Close())return false;
    const auto& events=local_server_hero_event_authority::GetState();
    if(state.host&&HeroPractice()&&events.initialized&&events.installed&&!events.closed&&!events.closing&&!events.error
       &&!local_server_hero_event_authority::SetPolicy(true,false))return HeroAuthorityFailure();
    if(state.host&&!RestoreControllerHook())return false;
    if(state.host&&local_server_effect_authority::GetState().initialized){
        // Also fence a source registered before a later loadout failure. Its
        // native handles retain cleanup permission; no new effect is admitted.
        for(const auto& source:local_server_effect_authority::sources)if(source.used)
            if(!local_server_effect_authority::RetirePlayer(source.index,source.pawn,source.epoch)){
                const auto& effects=local_server_effect_authority::GetState();return Fail(effects.error,effects.stage);
            }
        for(size_t n=0;n<original_timed_streak::entries.size();++n)if(original_timed_streak::entries[n].bound)
            if(!original_timed_streak::BeforeRetire(n))return Fail(original_timed_streak::State().error,original_timed_streak::State().stage);
    }
    if(state.host)for(size_t n=0;n<MaxPlayers;++n)if(loadouts[n].used){
        const auto& plan=loadouts[n];
        if(state.enabled&&local_server_combat::GetState().initialized&&!local_server_combat::BeforeRetire(n,plan.pawn,plan.epoch))return false;
        if(state.enabled&&local_server_npc_input::GetState().initialized){
            const bool retired=local_server_npc_input::BeforeRetire(n,plan.pawn,plan.epoch);CaptureNpcSnapshot();
            if(!retired)return Fail(local_server_npc_input::GetState().error,local_server_npc_input::GetState().stage);
        }
        if(state.enabled&&local_server_character_stream::GetState().initialized&&!local_server_character_stream::Retire(plan.pawn))
            return Fail(local_server_character_stream::GetState().error,local_server_character_stream::GetState().stage);
    }
    // Keep the source graph and original slots alive until all composed host
    // producers have fenced this human life and restored their owned entries.
    if(state.host&&HeroPractice()){
        const auto& human=loadouts[0];
        if(human.used){
            if(local_server_input::GetState().installed&&!hooks.beforeRetire)
                return Fail(4835,"local_server_runtime_authority_input_retirement_hook_required");
            if(hooks.beforeRetire&&!hooks.beforeRetire(0,human.pawn,human.epoch)){
                const auto& input=local_server_input::GetState();
                if(input.error)return Fail(input.error,input.stage);
                state.stage="local_server_runtime_close_waiting_idle_input_boundary";return false;
            }
            if(!local_server_hero_authority::RetirePlayer(human.pawn,human.epoch)){
                return HeroAuthorityFailure();
            }
        }
        if(!local_server_hero_authority::Close()){
            return HeroAuthorityFailure();
        }
    }
    if(state.host)for(size_t n=0;n<MaxPlayers;++n)if(loadouts[n].used){
        const auto& plan=loadouts[n];
        if(state.enabled&&local_server_cosmetics::GetState().initialized&&!CosmeticsResult(local_server_cosmetics::Retire(n,plan.pawn,plan.epoch)))return false;
        if(local_server_abilities::GetState().initialized&&!local_server_abilities::RetirePlayer(plan.pawn))return false;
        if(n==0&&state.enabled&&JettPractice()&&!local_server_starting_equipment::RetirePlayer(plan.pawn,plan.epoch))return false;
        if(n==0&&HeroPractice()&&!local_server_hero_abilities::RetirePlayer(plan.pawn,plan.epoch))return false;
        if(state.enabled&&!Practice()&&local_server_armor::GetState().initialized
           &&!local_server_armor::BeforeRetire(n,plan.pawn,plan.epoch)){
            const auto& armor=local_server_armor::GetState();return Fail(armor.error?armor.error:5527,armor.stage);
        }
        if(!local_server_equipment::UnregisterPlayer(n))return false;
    }
    if(!state.host&&HeroPractice()&&local_server_hero_resource_init::GetState().initialized
       &&!local_server_hero_resource_init::Close())return HeroAuthorityFailure();
    if(!state.host&&state.pawn&&local_server_cosmetics::GetState().initialized&&!CosmeticsResult(local_server_cosmetics::Retire(0,state.pawn)))return false;
    if(!state.host&&hooks.manageClientPresentation&&state.pawn&&local_server_client::GetState().initialized&&!local_server_client::Retire(state.pawn))return false;
    if(!state.host&&local_server_abilities::GetState().initialized&&state.pawn&&!local_server_abilities::RetirePlayer(state.pawn))return false;
    if(!state.host&&HeroPractice()&&state.pawn&&!local_server_hero_abilities::RetirePlayer(state.pawn,state.humanEpoch))return false;
    if(state.host&&!local_server_effect_authority::Close()){
        const auto& effects=local_server_effect_authority::GetState();return Fail(effects.error?effects.error:4860,effects.stage);
    }
    local_server_feedback_view::Close();local_server_feedback_ledger::Close();
    if(state.host&&!local_server_humanlike_bots::Close())return false;
    local_server_control::Close();state.closePending=false;state.closed=true;state.initialized=false;state.stage="local_server_runtime_closed";return true;
}
} // namespace local_server_runtime
