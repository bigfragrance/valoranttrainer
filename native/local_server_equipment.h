#pragma once
#include "original_content_services.h"
#include "original_inventory_slots.h"
#include "all_skins.h"
#include "local_server_item_ownership.h"
#include "local_server_resource_authority.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>

// Authority equipment assembly for an actual native network session. Catalog
// and authored skin resolution are shared; the standalone all_weapons player,
// input hooks, local weapon observers and all_skins replacement loop are not.
// Static evidence/ABI limits: work/local-server-playable/equipment/report.md.
namespace local_server_equipment {
using Guid=original_content::Guid;
using Weak=original_inventory_slots::WeakIdentity;
inline constexpr size_t MaxPlayers=32,SlotCount=17,MaxReferences=384;
enum class Mode:uint8_t { Equip=0, Grant=1 };
enum class Phase:uint8_t { Idle, Spawn, Publish, RequestEquip, WaitCurrent, Failed };
// Configuration/equipment requests only. The host derives the player index
// from the admitted connection; no process pointers or player index go on wire.
struct Request {
    Guid weapon{},skin{},level{},chroma{};
    uint32_t sequence=0;
    Mode mode=Mode::Equip;
    uint8_t reserved[3]{};
};
static_assert(sizeof(Request)==72);
struct NetworkedMovementTimestamp {Weak character{};float netTimestamp=0;int32_t respawnNumber=-1;};
struct PendingEquippableChange {void* current=nullptr;NetworkedMovementTimestamp timestamp{};};
static_assert(sizeof(NetworkedMovementTimestamp)==16&&alignof(NetworkedMovementTimestamp)==4);
static_assert(sizeof(PendingEquippableChange)==24&&offsetof(PendingEquippableChange,timestamp)==8);
struct Retired {void* actor=nullptr;Weak identity{};};
struct Transaction {
    Request request{};
    const all_weapons::Definition* definition=nullptr;
    all_skins::Selection selection{};
    all_skins::Binding skinBinding{};
    void *slot=nullptr,*candidate=nullptr,*replaced=nullptr;
    Weak candidateIdentity{},replacedIdentity{};
    uint64_t beganAt=0;
    bool reused=false,published=false,equipAccepted=false,ownershipPolicyReady=false,ammoPrepared=false;
    local_server_item_ownership::Status ownershipStatus=local_server_item_ownership::Status::NotObserved;
    original_weapon::EquipEventRegistration equipRegistration{};
};
struct LifeItem {
    void *actor=nullptr,*pawn=nullptr;Weak identity{},pawnIdentity{};uint32_t epoch=0;
    bool factoryBirth=false,observedOwned=false;
};
struct Player {
    bool registered=false,restartRequired=false,nativeEquipped=false;
    bool correctionPublished=false,nativeComponentsPresent=false;
    bool initialCorrectionPublicationObserved=false;
    int32_t initialCorrectionSequence=-1;
    uint64_t postBootstrapCorrectionSkips=0;
    unsigned error=0,accepted=0,completed=0,rejected=0,generation=0;
    uint32_t sourceEpoch=0;
    uint32_t lastAcceptedSequence=0,lastCompletedSequence=0;
    uint64_t correctionRequests=0,retiredActors=0,replicatedChanges=0,nativePurchases=0;
    bool replicatedChangePublished=false,purchasesEnabled=true,nativePurchasePending=false,moneyReady=false;
    int32_t money=0;
    uint8_t nativePurchaseTargetSlot=254,nativePurchaseType=255;
    Request nativePurchase{};
    NetworkedMovementTimestamp lastChangeTimestamp{};
    const char* stage="network_equipment_player_not_registered";
    const char* lastRejection="none";
    Phase phase=Phase::Idle;
    void *controller=nullptr,*playerState=nullptr,*pawn=nullptr,*inventory=nullptr;
    void *current=nullptr,*desired=nullptr,*lastCorrection=nullptr,*lastReplicatedCurrent=nullptr;
    Weak controllerIdentity{},stateIdentity{},pawnIdentity{},inventoryIdentity{};
    Transaction transaction{};
    std::array<Retired,16> retired{};
    std::array<LifeItem,MaxReferences> lifeItems{};
    bool retiring=false;
};
struct Diagnostics {
    bool initialized=false,profilesVerified=false,schemaVerified=false,catalogReady=false;
    bool appearanceServicesPrepared=false,clientAppearancePrepared=false,restartRequired=false,shoppingEnabled=false;
    unsigned error=0,verifiedDefinitions=0,registeredPlayers=0,activeIndex=0;
    uint64_t polls=0,clientOwnerNotifications=0,clientSlotNotifications=0;
    uintptr_t exceptionRva=0;
    const char* stage="network_equipment_not_initialized";
    const char* exceptionOperation="none";
    void* world=nullptr;
    DWORD gameThread=0;
};
enum class DeathItemStatus:uint8_t {Pending,AlreadyGone,WeakDestroyed,BeingDestroyed,Transferred,ForeignOwnerProtected};
struct DeathItem {
    LifeItem source;DeathItemStatus status=DeathItemStatus::Pending;
    bool destroyAttempted=false,observersRetired=false,orphan=false;
    void *ownerAtCapture=nullptr,*typedOwnerAtCapture=nullptr,*lastOwner=nullptr,*lastTypedOwner=nullptr;
    uint8_t inInventoryAtCapture=255,lastInInventory=255;uint32_t ownedComponents=0;
    void *driver=nullptr,*serverConnection=nullptr,*replicationDriver=nullptr;
    uint64_t actorDriverName=0,driverName=0;bool driverNameMatches=false,driverContextKnown=false;
};
struct DeathLife {
    bool captured=false,complete=false,inProgress=false;
    void *pawn=nullptr,*controller=nullptr,*playerState=nullptr,*inventory=nullptr;
    Weak pawnIdentity{},controllerIdentity{},stateIdentity{},inventoryIdentity{};
    uint32_t epoch=0;size_t count=0;
    uint64_t alreadyGone=0,destroyCalls=0,weakDestroyed=0,beingDestroyed=0,transferred=0,foreignProtected=0,orphansDestroyed=0;
    std::array<DeathItem,MaxReferences> items{};
    const char* stage="network_equipment_no_retired_life";
};
struct DeathStatistics {
    uint64_t lifeCaptures=0,lifeCompleted=0,itemReceipts=0,factoryBirths=0,itemsCaptured=0;
    uint64_t destroyCalls=0,weakDestroyed=0,beingDestroyed=0,alreadyGone=0,orphansDestroyed=0;
    uint64_t transferred=0,foreignProtected=0,ownedComponents=0;
};
struct DeathLifecycleMethods {
    bool ready=false;void *destroy=nullptr,*beingDestroyed=nullptr;Weak destroyId{},beingId{};
    struct Check {
        bool checked=false,found=false;uint16_t expectedBytes=0,actualBytes=0;
        uint32_t expectedFlags=0,actualFlags=0;uintptr_t expectedExec=0,actualExec=0;
        void *expectedOwner=nullptr,*actualOwner=nullptr;
    };
    std::array<Check,2> checks{};
};
struct Graph {
    void* pawn=nullptr;
    void* inventory=nullptr;
    void* slots[SlotCount]{};
    void* actors[MaxReferences]{};
    size_t slotCount=0,actorCount=0;
    bool complete=true;
};
struct ClientObservation {
    void *pawn=nullptr,*inventory=nullptr,*current=nullptr,*desired=nullptr;
    void *owner=nullptr,*typedOwner=nullptr,*primaryData=nullptr;
    void *skinData=nullptr,*levelData=nullptr,*chromaData=nullptr;
    void *mesh1P=nullptr,*mesh3P=nullptr,*machine=nullptr,*predictedState=nullptr;
    Guid weapon{},skin{},level{},chroma{};
    int slotCount=0;
    bool originalInventory=false,ownerMatches=false,initialCorrectionProcessed=false;
    int32_t serverCorrectionSequence=-1,processedCorrectionSequence=-1;
    int32_t correctionInputSequence=-1,clientInputCorrectionSequence=-1;
    bool nativeComponentsPresent=false,appearanceFieldsPresent=false;
    local_server_item_ownership::ClientObservation ownership{};
    bool ownershipObserved=false;
    // This is native field/identity observation, not visual or firing proof.
};
inline Diagnostics state;
inline std::array<Player,MaxPlayers> players{};
inline std::array<DeathLife,MaxPlayers> deathLives{};
inline DeathStatistics deathStats;
inline DeathLifecycleMethods deathMethods;
inline bool TrackLifeInventory(Player& p);
// Canonical GUID preferences survive a pawn epoch; they contain no native
// actor, inventory, state-machine or input pointers.
inline std::array<std::array<Request,all_weapons::Count>,MaxPlayers> preferences{};
inline const Diagnostics& State(){return state;}
inline const Player* State(size_t index){return index<MaxPlayers?&players[index]:nullptr;}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;return false;}
inline bool Fail(Player& p,unsigned code,const char* stage){p.error=code;p.stage=stage;p.phase=Phase::Failed;p.nativeEquipped=false;return false;}
inline bool Reject(Player& p,const char* reason){++p.rejected;p.lastRejection=reason;return false;}
inline bool Thread(){return engine::image&&original_players::Thread()&&(!state.gameThread||state.gameThread==GetCurrentThreadId())&&!state.restartRequired;}
inline bool Event(void* object,const wchar_t* path,uint16_t size,void* parameters){
    auto fn=engine::Find(path);
    return object&&fn&&Read<uint16_t>(fn,0xbe)==size&&engine::Event(object,path,parameters);
}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Capture(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
struct CodeProfile {uintptr_t rva;size_t bytes;uint64_t fnv;};
// BEGIN PINNED EQUIPMENT PROFILES
inline constexpr CodeProfile Profiles[]{
    {0x70ec480,636,0x2b872f1c0627e25aULL},
    {0x6ff5f40,101,0xeb154c7c237fd429ULL},
    {0x6ff6000,20,0x54a3b75986931278ULL},
    {0x6ff6014,4,0xdbdaadda7244606aULL},
    {0x6ff6018,784,0x83a108b8d3ee771cULL},
    {0x6ff6328,441,0x180238b5ad5f5d6bULL},
    {0x6ff64e1,30,0xc4f7623038ff7438ULL},
    {0x6ff64ff,322,0x1b8cc2abfd8f54a1ULL},
    {0x6ff6641,8,0x2d61151501c50057ULL},
    {0x6ff68d0,77,0x8971fb2c88a16baULL},
    {0x713e820,1539,0x4fee8a9f382431c0ULL},
    {0x7141820,1217,0x51ebeedd1d5cdd1eULL},
    {0x70eda70,93,0x576784a71302c564ULL},
    {0x70edacd,279,0xcd37c8a44794a560ULL},
    {0x70edbe4,112,0xf2e657a2dbf877cfULL},
    {0x70f8930,346,0x6f7dfe2f24f68206ULL},
    {0x70f8a90,332,0xb64bfbb41a07add5ULL},
    {0x70f9de0,466,0x2f2ff432cde5fe07ULL},
    {0x70fa670,469,0xb09df98847fe568dULL},
    {0x70fa910,539,0x72500757cacd5f9fULL},
    {0x70fb800,168,0x84496bd4102b3e97ULL},
    {0x7129f90,258,0x4cf4f86fc8db05c8ULL},
    {0x70fb8b0,1032,0x2851268283855f8bULL},
    {0x70eecd0,3723,0xdd6430bbfdbfd156ULL},
    {0x68f7050,69,0x68af81ea47454eaULL},
    {0x649e0a0,104,0xe21d8313b69cf533ULL},
    {0x71ce540,171,0xe9fe60e4f61a7275ULL},
    {0x71d1300,171,0xafbc982e19e31189ULL},
    {0x71d13b0,169,0x69f13da801580934ULL},
    {0x71d1460,173,0xc25678bc79afbfc3ULL},
    {0x78a2a90,43,0xb41d3616d0a16c56ULL},
    {0x78a2abb,101,0xc78c0522ee280eedULL},
    {0x78a2b20,27,0x4a7f24f45c8cb99eULL},
};
// END PINNED EQUIPMENT PROFILES
inline bool VerifyProfiles(){
    if(state.profilesVerified)return true;
    for(const auto& profile:Profiles){
        auto code=reinterpret_cast<const uint8_t*>(engine::image+profile.rva);
        if(!original_players::Readable(code,profile.bytes))return Fail(3501,"network_equipment_profile_memory_unavailable");
        uint64_t digest=0xcbf29ce484222325ULL;
        for(size_t i=0;i<profile.bytes;++i)digest=(digest^code[i])*0x100000001b3ULL;
        if(digest!=profile.fnv)return Fail(3502,"network_equipment_original_code_profile_mismatch");
    }
    state.profilesVerified=true;return true;
}
inline bool Field(void* type,const wchar_t* name,int32_t offset,int32_t bytes,int32_t dimensions=1,uint64_t flags=0){
    uint32_t nativeName[3]{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(nativeName,name,1);
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,nativeName,3);
    return field&&Read<int32_t>(field,0x44)==offset&&Read<int32_t>(field,0x34)==bytes
        &&Read<int32_t>(field,0x30)==dimensions&&(Read<uint64_t>(field,0x38)&flags)==flags;
}
inline bool VerifySchema(){
    if(state.schemaVerified)return true;
    auto inventory=engine::Class(0x6499330),slot=engine::Class(0x68f63c0),weapon=engine::Class(0x645c660);
    if(!Field(inventory,L"ItemSlots",0x1b8,8,17,0x100000020ULL)
       ||!Field(inventory,L"CurrentEquippable",0x280,8)
       ||!Field(inventory,L"DesiredEquippable",0x268,8)
       ||!Field(inventory,L"EquippableChange",0x2a0,24,1,0x100000020ULL)
       ||!Field(inventory,L"AuthServerCorrectRepVariables",0x320,16,1,0x100000020ULL)
       ||!Field(slot,L"Contents",0x38,8,1,0x100000020ULL)
       ||!Field(slot,L"Inventory",0x40,8)||!Field(slot,L"SlotType",0x30,1)
       ||!Field(weapon,L"Owner",0x210,8,1,0x20)
       ||!Field(weapon,L"OwningPrimaryDataAsset",0xfa8,8,1,0x20)
       ||!Field(weapon,L"SkinDataAsset",0xfb0,8,1,0x20)
       ||!Field(weapon,L"SkinLevelDataAsset",0xfb8,8,1,0x20)
       ||!Field(weapon,L"AttachmentDataAssets",0xfc0,16,1,0x20)
       ||!Field(weapon,L"ChromaDataAsset",0xfd0,8,1,0x20)
       ||!Field(weapon,L"SkinDataAssetId",0xfe8,16,1,0x20)
       ||!Field(weapon,L"SkinLevelDataAssetId",0xff8,16,1,0x20)
       ||!Field(weapon,L"AttachmentDataAssetIds",0x1028,16,1,0x20)
       ||!Field(weapon,L"ChromaDataAssetId",0x1038,16,1,0x20))
        return Fail(3503,"network_equipment_original_replication_schema_mismatch");
    auto change=engine::Class(0x6499c10),timestamp=engine::Class(0x6cd57a0);
    if(!Field(change,L"NewCurrentEquippable",0,8)||!Field(change,L"Timestamp",8,16)
       ||!Field(timestamp,L"Character",0,8)||!Field(timestamp,L"NetTimestamp",8,4)
       ||!Field(timestamp,L"RespawnNumber",12,4))
        return Fail(3544,"network_equipment_original_movement_timestamp_schema_mismatch");
    state.schemaVerified=true;return true;
}
inline bool PrepareCatalog(){
    if(state.catalogReady)return true;
    if(!Thread()||!original_content::State().initialized||!original_content::State().index)
        return Fail(3504,"network_equipment_original_content_not_ready");
    if(!VerifyProfiles()||!VerifySchema())return false;
    state.verifiedDefinitions=0;
    for(auto& definition:all_weapons::definitions)if(all_weapons::ResolveDefinition(definition))++state.verifiedDefinitions;
    if(state.verifiedDefinitions!=all_weapons::Count)return Fail(3505,"network_equipment_original_weapon_catalog_incomplete");
    state.catalogReady=true;return true;
}
inline bool PrepareHostShopping(){
    auto gameState=Read<void*>(state.world,0x178);
    if(!original_players::IsA(gameState,original_players::ShooterGameStateClass)||Read<uint8_t>(gameState,0x224)!=3)
        return Fail(3549,"network_equipment_native_shopping_game_state_missing");
    auto type=Read<void*>(gameState,0x10);
    if(!Field(type,L"bIsShoppingEnabled",0xa08,1,1,0x20)
       ||!Field(type,L"bAllowCharacterSpecificEquippablesInShop",0x6f3,1))
        return Fail(3550,"network_equipment_native_shopping_rule_schema_mismatch");
    uint8_t enabled=1;
    if(!Event(gameState,L"/Script/ShooterGame.ShooterGameState.SetShoppingEnabled",1,&enabled)
       ||!Read<uint8_t>(gameState,0xa08))return Fail(3551,"network_equipment_original_shopping_rule_not_observed");
    // Free practice skills use their original loadout rather than the gun
    // store. This is an explicit local match policy, not a decoded server rule.
    if(!original_players::Write<uint8_t>(gameState,0x6f3,0))return Fail(3552,"network_equipment_authority_skill_shop_policy_failed");
    state.shoppingEnabled=true;return true;
}
inline bool PrepareMoney(Player& p){
    auto component=Read<void*>(p.playerState,0x8a0);
    if(!original_players::IsA(component,0x69cd280)||Read<void*>(component,0xc8)!=p.playerState
       ||!Field(Read<void*>(component,0x10),L"Money",0xf8,4,1,0x100000020ULL))
        return Fail(p,3553,"network_equipment_original_money_component_or_schema_missing");
    uint8_t replicated=1;
    if(!Event(component,L"/Script/Engine.ActorComponent.SetIsReplicated",1,&replicated))
        return Fail(p,3554,"network_equipment_original_money_replication_enable_failed");
    int32_t previous=Read<int32_t>(component,0xf8);
    if(previous<9000){
        // AuthSetMoney is an absent authority leaf in this client build. Supply
        // only this host's free-practice budget; client Money remains native
        // replication plus the original OnRep, with no fabricated owned flags.
        if(!original_players::Write<int32_t>(component,0xf8,9000)
           ||!Event(component,L"/Script/ShooterGame.MoneyManagementComponent.OnRep_Money",4,&previous))
            return Fail(p,3555,"network_equipment_authority_practice_money_failed");
    }
    p.money=Read<int32_t>(component,0xf8);p.moneyReady=p.money>=9000;
    return p.moneyReady||Fail(p,3556,"network_equipment_authority_practice_money_read_back_failed");
}
inline bool InitializeImpl(void* world){
    if(!Thread()||!world||!Read<void*>(world,0x170))return Fail(3506,"network_equipment_requires_native_authority_world");
    if(state.initialized)return state.world==world||Fail(3507,"network_equipment_world_changed_unregister_first");
    state.gameThread=GetCurrentThreadId();state.world=world;
    if(!PrepareCatalog())return false;
    // Both images need their real original preload/config context before a
    // native weapon BeginPlay can visit skin resources. This initializes the
    // shared resource layer only, without a player-bound weapon factory.
    if(!all_skins::Initialize())return Fail(all_skins::State().error,all_skins::State().stage);
    state.appearanceServicesPrepared=true;
    if(!PrepareHostShopping())return false;
    state.initialized=true;state.stage="network_equipment_authority_catalog_ready";return true;
}
// Resource context only; safe before client net travel once its original local
// services exist. This never spawns/equips a client weapon or grants inventory.
inline bool PrepareClientAppearanceImpl(){
    if(!Thread()||!PrepareCatalog())return false;
    if(!all_skins::Initialize())return Fail(all_skins::State().error,all_skins::State().stage);
    state.appearanceServicesPrepared=true;state.clientAppearancePrepared=true;return true;
}
inline bool PlayerLive(const Player& p){
    return p.registered&&Live(p.controller,p.controllerIdentity)&&Live(p.playerState,p.stateIdentity)
        &&Live(p.pawn,p.pawnIdentity)&&Live(p.inventory,p.inventoryIdentity);
}
inline bool Ownership(const Player& p){
    return PlayerLive(p)&&Read<uint8_t>(p.pawn,0x224)==3&&Read<uint8_t>(p.controller,0x224)==3
        &&Read<void*>(p.controller,0x470)==p.playerState&&Read<void*>(p.controller,0x4b0)==p.pawn
        &&Read<void*>(p.playerState,0x210)==p.controller&&Read<void*>(p.pawn,0x488)==p.playerState
        &&Read<void*>(p.pawn,0x4a0)==p.controller&&Read<void*>(p.pawn,0xc08)==p.inventory
        &&Read<void*>(p.inventory,0xc8)==p.pawn&&Read<void*>(p.inventory,0x2f0)==p.pawn;
}
inline bool RegisterPlayerImpl(size_t index,void* controller,void* playerState,void* pawn,uint32_t epoch){
    if(!Thread()||!state.initialized||index>=MaxPlayers||!epoch)return Fail(3508,"network_equipment_registration_context_invalid");
    auto& p=players[index];state.activeIndex=static_cast<unsigned>(index);
    if(p.registered)return p.controller==controller&&p.playerState==playerState&&p.pawn==pawn&&p.sourceEpoch==epoch&&Ownership(p);
    if(!original_players::IsA(pawn,original_players::ShooterCharacterClass)||!Read<uint8_t>(pawn,0xa90)
       ||!original_players::IsA(playerState,original_players::ShooterStateClass)
       ||!controller||Read<uint8_t>(controller,0x224)!=3||Read<uint8_t>(pawn,0x224)!=3)
        return Fail(p,3509,"network_equipment_native_authority_player_missing");
    p.controller=controller;p.playerState=playerState;p.pawn=pawn;p.inventory=Read<void*>(pawn,0xc08);p.sourceEpoch=epoch;
    if(!Capture(controller,p.controllerIdentity)||!Capture(playerState,p.stateIdentity)
       ||!Capture(pawn,p.pawnIdentity)||!Capture(p.inventory,p.inventoryIdentity))
        return Fail(p,3510,"network_equipment_native_player_weak_identity_missing");
    p.registered=true;++state.registeredPlayers;
    if(!Ownership(p)||!original_inventory_slots::EnsureForPawn(pawn))return Fail(p,3511,"network_equipment_original_inventory_ownership_invalid");
    auto table=Read<void*>(p.inventory);
    if(Read<void*>(table,0x318)!=reinterpret_cast<void*>(engine::image+0x70fb800))
        return Fail(p,3512,"network_equipment_original_slot_replication_virtual_mismatch");
    if(!PrepareMoney(p)||!TrackLifeInventory(p))return false;
    p.stage="network_equipment_authority_player_registered";return true;
}
inline bool Busy(size_t index){return index<MaxPlayers&&players[index].registered&&players[index].phase!=Phase::Idle;}
inline bool Ready(size_t index){return index<MaxPlayers&&Ownership(players[index])&&!players[index].retiring&&!players[index].error
    &&players[index].phase==Phase::Idle&&players[index].nativeEquipped;}
inline bool SkinMatches(void* actor,const all_weapons::Definition& definition,const all_skins::Selection& selected){
    return Read<void*>(actor,0x10)==definition.runtimeClass&&Read<void*>(actor,0xfa8)==definition.dataAsset
        &&Read<void*>(actor,0xfb0)==selected.skinData&&Read<void*>(actor,0xfb8)==selected.levelData
        &&Read<void*>(actor,0xfd0)==selected.chromaData
        &&all_weapons::Same(Read<Guid>(actor,0xfe8),selected.skin)
        &&all_weapons::Same(Read<Guid>(actor,0xff8),selected.level)
        &&all_weapons::Same(Read<Guid>(actor,0x1038),selected.chroma);
}
inline bool EquipImpl(size_t index,const Request& request){
    if(!Thread()||!state.initialized||index>=MaxPlayers)return false;
    auto& p=players[index];state.activeIndex=static_cast<unsigned>(index);
    if(p.error||p.restartRequired||!Ownership(p))return false;
    if(p.retiring)return Reject(p,"network_equipment_old_life_retirement_fenced");
    if(p.phase!=Phase::Idle)return Reject(p,"network_equipment_request_busy");
    if((request.mode!=Mode::Equip&&request.mode!=Mode::Grant)||request.reserved[0]||request.reserved[1]||request.reserved[2])
        return Reject(p,"network_equipment_request_schema_invalid");
    if(request.sequence&&request.sequence<=p.lastAcceptedSequence)return Reject(p,"network_equipment_request_sequence_not_new");
    auto definition=all_weapons::Find(request.weapon);
    if(!definition||!definition->identityVerified||definition->equipmentSlot<0||definition->equipmentSlot>2)
        return Reject(p,"network_equipment_request_unknown_original_weapon");
    all_skins::Selection selected;
    if(!all_skins::Resolve(*definition,request.skin,request.level,request.chroma,selected))
        return Reject(p,"network_equipment_request_invalid_skin_weapon_relationship");
    auto slot=Read<void*>(p.inventory,0x1b8+static_cast<size_t>(definition->equipmentSlot)*8);
    if(!original_players::IsA(slot,0x68f63c0)||original_players::IsA(slot,0x69cf3d0)
       ||Read<void*>(slot,0x40)!=p.inventory||Read<uint8_t>(slot,0x30)!=definition->equipmentSlot)
        return Fail(p,3513,"network_equipment_native_target_slot_invalid");
    auto old=Read<void*>(slot,0x38);
    const bool reuse=old&&SkinMatches(old,*definition,selected);
    if(request.mode==Mode::Grant&&!reuse&&old&&(old==Read<void*>(p.inventory,0x280)||old==Read<void*>(p.inventory,0x268)))
        return Reject(p,"network_equipment_grant_cannot_replace_current_use_equip");
    if(!TrackLifeInventory(p))return false;
    p.transaction={};auto& t=p.transaction;t.request=request;t.definition=definition;t.selection=selected;
    if(!t.request.sequence)t.request.sequence=p.lastAcceptedSequence+1;
    t.beganAt=GetTickCount64();t.slot=slot;t.reused=reuse;t.replaced=reuse?nullptr:old;t.candidate=reuse?old:nullptr;
    if(old&&!Capture(old,reuse?t.candidateIdentity:t.replacedIdentity))return Fail(p,3514,"network_equipment_existing_item_identity_expired");
    if(reuse&&(Read<void*>(old,0x210)!=p.pawn||Read<void*>(old,0x4a8)!=p.pawn||Read<uint8_t>(old,0x224)!=3))
        return Fail(p,3515,"network_equipment_existing_item_owner_invalid");
    p.phase=reuse?(request.mode==Mode::Grant?Phase::Publish:Phase::RequestEquip):Phase::Spawn;
    p.lastAcceptedSequence=t.request.sequence;++p.accepted;p.nativeEquipped=false;
    auto& preferred=preferences[index][static_cast<size_t>(definition-all_weapons::definitions)];
    preferred={};preferred.weapon=definition->id;preferred.skin=selected.skin;preferred.level=selected.level;preferred.chroma=selected.chroma;
    p.stage="network_equipment_authority_request_accepted";return true;
}
inline void NativePurchaseImpl(void* controller,void* weaponClass,uint8_t targetSlot,uint8_t purchaseType){
    if(!Thread()||!state.initialized||!controller||!weaponClass)return;
    const auto definition=all_weapons::FindByClass(weaponClass);
    if(!definition||!definition->identityVerified)return;
    for(size_t i=0;i<MaxPlayers;++i){auto& p=players[i];
        if(!p.registered||p.controller!=controller||!Ownership(p))continue;
        if(!p.purchasesEnabled||p.error){Reject(p,"network_equipment_native_shop_policy_disabled");return;}
        // This local trainer's free weapon-selection policy chooses the real
        // definition's native slot and retains all original TryEquip checks.
        // TargetSlot/Type are observed; storage/economy server logic is not
        // reconstructed by interpreting unknown enum values.
        p.nativePurchase=preferences[i][static_cast<size_t>(definition-all_weapons::definitions)];
        p.nativePurchase.weapon=definition->id;p.nativePurchase.sequence=0;p.nativePurchase.mode=Mode::Equip;
        p.nativePurchaseTargetSlot=targetSlot;p.nativePurchaseType=purchaseType;
        p.nativePurchasePending=true;++p.nativePurchases;return;
    }
}
inline bool SetPurchasesEnabled(size_t index,bool enabled){
    if(!Thread()||index>=MaxPlayers||!players[index].registered)return false;
    players[index].purchasesEnabled=enabled;if(!enabled)players[index].nativePurchasePending=false;return true;
}
template<class T> struct NativeArray {T* data=nullptr;int32_t count=0,capacity=0;};
// Source receipts are restricted to this player's real inventory and this
// module's actual deferred-spawn return. No world actor enumeration occurs.
inline bool EquipmentActorWorld(void* actor){
    auto table=Read<void**>(actor);auto get=Read<void*(*)(void*)>(table,0x1b8);
    return get&&get(actor)==state.world;
}
inline bool RememberLifeItem(Player& p,void* actor,const Weak& identity,bool factoryBirth){
    if(!Live(actor,identity)||!original_players::IsA(actor,0x645c660)||Read<uint8_t>(actor,0x224)!=3
       ||!EquipmentActorWorld(actor))return Fail(p,3552,"network_equipment_life_item_native_identity_invalid");
    const bool owned=Read<void*>(actor,0x210)==p.pawn&&Read<void*>(actor,0x4a8)==p.pawn;
    if(factoryBirth){const auto& t=p.transaction;
        if(t.reused||t.candidate!=actor||!original_inventory_slots::SameIdentity(t.candidateIdentity,identity)
           ||!t.definition||!t.definition->identityVerified||Read<void*>(actor,0x10)!=t.definition->runtimeClass)
            return Fail(p,3553,"network_equipment_life_item_factory_receipt_invalid");
    }else if(!owned)return true;
    LifeItem* empty=nullptr;
    for(auto& r:p.lifeItems){
        if(r.actor&&original_inventory_slots::SameIdentity(r.identity,identity)){
            if(r.actor!=actor||r.pawn!=p.pawn||r.epoch!=p.sourceEpoch
               ||!original_inventory_slots::SameIdentity(r.pawnIdentity,p.pawnIdentity))
                return Fail(p,3554,"network_equipment_life_item_generation_changed");
            r.factoryBirth=r.factoryBirth||factoryBirth;r.observedOwned=r.observedOwned||owned;return true;
        }
        if(!empty&&(!r.actor||!Live(r.actor,r.identity)))empty=&r;
    }
    if(!empty)return Fail(p,3555,"network_equipment_life_item_receipt_capacity_exceeded");
    *empty={actor,p.pawn,identity,p.pawnIdentity,p.sourceEpoch,factoryBirth,owned};
    ++deathStats.itemReceipts;if(factoryBirth)++deathStats.factoryBirths;return true;
}
inline bool ObserveLifeItem(Player& p,void* actor){
    if(!actor)return true;Weak identity{};
    if(!Capture(actor,identity))return true; // No old raw read after weak failure.
    if(!original_players::IsA(actor,0x645c660))return true;
    if(Read<void*>(actor,0x210)!=p.pawn||Read<void*>(actor,0x4a8)!=p.pawn)return true;
    return RememberLifeItem(p,actor,identity,false);
}
inline bool TrackLifeInventory(Player& p){
    if(!Ownership(p))return Fail(p,3556,"network_equipment_life_tracking_requires_current_native_owner");
    for(size_t i=0;i<SlotCount;++i){const auto slot=Read<void*>(p.inventory,0x1b8+i*8);
        if(!slot)continue;
        if(!original_players::IsA(slot,0x68f63c0)||Read<void*>(slot,0x40)!=p.inventory)
            return Fail(p,3557,"network_equipment_life_tracking_slot_owner_invalid");
        if(original_players::IsA(slot,0x69cf3d0)){
            const auto a=Read<NativeArray<void*>>(slot,0x50);
            if(a.count<0||a.count>256||a.capacity<a.count
               ||(a.count&&!original_players::Readable(a.data,static_cast<size_t>(a.count)*8)))
                return Fail(p,3558,"network_equipment_life_tracking_multi_slot_invalid");
            for(int32_t n=0;n<a.count;++n)if(!ObserveLifeItem(p,a.data[n]))return false;
        }else if(!ObserveLifeItem(p,Read<void*>(slot,0x38)))return false;
    }
    for(size_t offset:{size_t(0x268),size_t(0x278),size_t(0x280),size_t(0x2a0),size_t(0x320)})
        if(!ObserveLifeItem(p,Read<void*>(p.inventory,offset)))return false;
    const auto& t=p.transaction;
    if(Live(t.candidate,t.candidateIdentity)&&!ObserveLifeItem(p,t.candidate))return false;
    if(Live(t.replaced,t.replacedIdentity)&&!ObserveLifeItem(p,t.replaced))return false;
    for(const auto& r:p.retired)if(Live(r.actor,r.identity)&&!ObserveLifeItem(p,r.actor))return false;
    return true;
}

inline bool DeathMethods(){
    if(deathMethods.ready)return Live(deathMethods.destroy,deathMethods.destroyId)
        &&Live(deathMethods.beingDestroyed,deathMethods.beingId);
    const auto actor=engine::Class(0x3683900);
    deathMethods.destroy=engine::Find(L"/Script/Engine.Actor.K2_DestroyActor");
    deathMethods.beingDestroyed=engine::Find(L"/Script/Engine.Actor.IsActorBeingDestroyed");
    const auto check=[&](size_t index,void* f,uint16_t bytes,uint32_t flags,uintptr_t exec){
        auto& observed=deathMethods.checks[index];observed.checked=true;observed.found=f!=nullptr;
        observed.expectedBytes=bytes;observed.actualBytes=Read<uint16_t>(f,0xbe);
        observed.expectedFlags=flags;observed.actualFlags=Read<uint32_t>(f,0xb8);
        observed.expectedOwner=actor;observed.actualOwner=Read<void*>(f,0x28);
        observed.expectedExec=exec;const auto address=Read<uintptr_t>(f,0xe0);
        observed.actualExec=address>=engine::image?address-engine::image:address;
        return f&&observed.actualOwner==actor&&observed.actualBytes==bytes
            &&observed.actualFlags==flags&&address==engine::image+exec;};
    if(!check(0,deathMethods.destroy,0,0x04020402,0x3eece00)
       ||!check(1,deathMethods.beingDestroyed,1,0x54020401,0x3eea190)
       ||!Capture(deathMethods.destroy,deathMethods.destroyId)||!Capture(deathMethods.beingDestroyed,deathMethods.beingId))
        return Fail(3559,"network_equipment_original_death_lifecycle_metadata_changed");
    deathMethods.ready=true;return true;
}
inline bool DeathEvent(void* actor,void* function,void* parameters){
    auto table=Read<void**>(actor);auto event=Read<void(*)(void*,void*,void*)>(table,0x298);
    if(!event||!function)return false;event(actor,function,parameters);return true;
}
inline bool SameRetiringLife(const DeathLife& life,const Player& p,void* pawn,uint32_t epoch){
    return life.captured&&life.pawn==pawn&&life.epoch==epoch&&p.registered&&p.pawn==pawn&&p.sourceEpoch==epoch
        &&p.controller==life.controller&&p.playerState==life.playerState&&p.inventory==life.inventory
        &&Live(pawn,life.pawnIdentity)&&Live(life.controller,life.controllerIdentity)
        &&Live(life.playerState,life.stateIdentity)&&Live(life.inventory,life.inventoryIdentity)
        &&original_inventory_slots::SameIdentity(p.pawnIdentity,life.pawnIdentity)
        &&original_inventory_slots::SameIdentity(p.inventoryIdentity,life.inventoryIdentity)&&Ownership(p);
}
inline bool CaptureRetiringLifeImpl(size_t index,void* pawn,uint32_t epoch){
    if(!Thread()||!state.initialized||index>=MaxPlayers||!pawn||!epoch)return Fail(3560,"network_equipment_death_capture_context_invalid");
    auto& p=players[index];auto& life=deathLives[index];state.activeIndex=static_cast<unsigned>(index);
    if(life.captured&&life.pawn==pawn&&life.epoch==epoch)
        return SameRetiringLife(life,p,pawn,epoch)||Fail(p,3561,"network_equipment_death_capture_source_changed");
    if((life.captured&&!life.complete)||!p.registered||p.pawn!=pawn||p.sourceEpoch!=epoch||!Ownership(p))
        return Fail(p,3561,"network_equipment_death_capture_source_changed");
    if(!DeathMethods()||!TrackLifeInventory(p))return false;
    life={};life.captured=true;life.pawn=p.pawn;life.controller=p.controller;life.playerState=p.playerState;
    life.inventory=p.inventory;life.epoch=epoch;life.pawnIdentity=p.pawnIdentity;
    life.controllerIdentity=p.controllerIdentity;life.stateIdentity=p.stateIdentity;life.inventoryIdentity=p.inventoryIdentity;
    life.stage="network_equipment_old_life_items_captured_before_observer_retirement";
    for(const auto& source:p.lifeItems)if(source.actor){
        if(!Live(source.actor,source.identity)){++life.alreadyGone;++deathStats.alreadyGone;continue;}
        if(source.epoch!=epoch||source.pawn!=pawn
           ||!original_inventory_slots::SameIdentity(source.pawnIdentity,p.pawnIdentity)
           ||(!source.factoryBirth&&!source.observedOwned))return Fail(p,3562,"network_equipment_death_item_receipt_wrong_life");
        auto& item=life.items[life.count++];item.source=source;
        item.ownerAtCapture=Read<void*>(source.actor,0x210);item.typedOwnerAtCapture=Read<void*>(source.actor,0x4a8);
        item.inInventoryAtCapture=Read<uint8_t>(source.actor,0x4b0);
        for(size_t offset:{size_t(0xdd8),size_t(0xde0),size_t(0xdf0)}){
            const auto component=Read<void*>(source.actor,offset);Weak identity{};
            if(component&&Capture(component,identity)&&original_players::IsA(component,0x36837a0)
               &&Read<void*>(component,0xc8)==source.actor){++item.ownedComponents;++deathStats.ownedComponents;}
        }
    }
    if(index&&!local_server_resource_authority::RetireNpcWeaponAmmoLife(index,pawn,epoch,p.pawnIdentity))
        return Fail(p,3567,"network_equipment_old_npc_ammo_life_retire_failed");
    p.retiring=true;p.purchasesEnabled=false;p.nativePurchasePending=false;
    ++deathStats.lifeCaptures;deathStats.itemsCaptured+=life.count;return true;
}
inline bool ProvedOtherInventory(const Player& dying,void* actor,void* owner,void* typed){
    if(!owner||owner!=typed||owner==dying.pawn)return false;
    for(const auto& other:players){
        if(&other==&dying||!other.registered||other.pawn!=owner||!Ownership(other))continue;
        for(size_t offset:{size_t(0x268),size_t(0x280)})if(Read<void*>(other.inventory,offset)==actor)return true;
        for(size_t n=0;n<SlotCount;++n){const auto slot=Read<void*>(other.inventory,0x1b8+n*8);
            if(!slot||!original_players::IsA(slot,0x68f63c0)||Read<void*>(slot,0x40)!=other.inventory)continue;
            if(original_players::IsA(slot,0x69cf3d0)){
                const auto a=Read<NativeArray<void*>>(slot,0x50);
                if(a.count<0||a.count>256||a.capacity<a.count
                   ||(a.count&&!original_players::Readable(a.data,static_cast<size_t>(a.count)*8)))continue;
                for(int32_t k=0;k<a.count;++k)if(a.data[k]==actor)return true;
            }else if(Read<void*>(slot,0x38)==actor)return true;
        }
    }return false;
}
inline bool RetireCapturedLifeBody(size_t index,void* pawn,uint32_t epoch){
    auto& p=players[index];auto& life=deathLives[index];
    if(!SameRetiringLife(life,p,pawn,epoch))return Fail(p,3563,"network_equipment_death_retire_wrong_life");
    if(life.complete)return true;if(!DeathMethods())return false;
    life.stage="network_equipment_destroying_exact_old_life_owned_equipment";
    for(size_t n=0;n<life.count;++n){auto& item=life.items[n];if(item.status!=DeathItemStatus::Pending)continue;
        if(!SameRetiringLife(life,p,pawn,epoch))return Fail(p,3563,"network_equipment_death_retire_wrong_life");
        const auto actor=item.source.actor;
        if(!Live(actor,item.source.identity)){
            item.status=DeathItemStatus::AlreadyGone;++life.alreadyGone;++deathStats.alreadyGone;continue;}
        if(!original_players::IsA(actor,0x645c660)||Read<uint8_t>(actor,0x224)!=3||!EquipmentActorWorld(actor))
            return Fail(p,3564,"network_equipment_death_item_native_context_changed");
        item.lastOwner=Read<void*>(actor,0x210);item.lastTypedOwner=Read<void*>(actor,0x4a8);
        item.lastInInventory=Read<uint8_t>(actor,0x4b0);
        item.driver=Read<void*>(state.world,0x40);
        item.driverContextKnown=item.driver&&original_players::Readable(item.driver,0x7d8)
            &&Read<void*>(item.driver,0x178)==state.world;
        item.serverConnection=Read<void*>(item.driver,0xc0);item.replicationDriver=Read<void*>(item.driver,0x7d0);
        // Original47465B0 compares exactly these two native64-bit name keys.
        item.actorDriverName=Read<uint64_t>(actor,0x218);item.driverName=Read<uint64_t>(item.driver,0x1d0);
        item.driverNameMatches=item.driverContextKnown&&item.actorDriverName==item.driverName;
        const bool foreign=(item.lastOwner&&item.lastOwner!=pawn)||(item.lastTypedOwner&&item.lastTypedOwner!=pawn);
        if(foreign){
            const bool transferred=ProvedOtherInventory(p,actor,item.lastOwner,item.lastTypedOwner);
            item.status=transferred?DeathItemStatus::Transferred:DeathItemStatus::ForeignOwnerProtected;
            if(transferred){++life.transferred;++deathStats.transferred;}
            else {++life.foreignProtected;++deathStats.foreignProtected;}
            continue; // Actual foreign ownership is protected, never called a successful drop.
        }
        // Empty owner fields are not a native drop receipt. This exact item
        // was really born for or previously held by this old life.
        item.orphan=!item.lastOwner&&!item.lastTypedOwner;
        if(!item.observersRetired){
            const auto resource=local_server_resource_authority::FindCached(actor);
            if(resource&&resource->weaponAmmo&&!local_server_resource_authority::RetireWeaponAmmo(actor))
                return Fail(p,3567,"network_equipment_old_weapon_ammo_observer_retire_failed");
            if(!original_effects::RetireActor(actor))return Fail(p,3568,"network_equipment_old_weapon_effect_observer_retire_failed");
            item.observersRetired=true;
        }
        uint8_t being=255;
        if(!DeathEvent(actor,deathMethods.beingDestroyed,&being)||being>1)
            return Fail(p,3565,"network_equipment_original_destroy_state_query_failed");
        if(being){item.status=DeathItemStatus::BeingDestroyed;++life.beingDestroyed;++deathStats.beingDestroyed;continue;}
        if(item.destroyAttempted)return Fail(p,3566,"network_equipment_native_destroy_unconfirmed_no_replay");
        // Reserve before entering original callbacks. A retry cannot dispatch
        // a second Destroy to an old raw pointer or a replacement object.
        item.destroyAttempted=true;++life.destroyCalls;++deathStats.destroyCalls;
        if(!DeathEvent(actor,deathMethods.destroy,nullptr))return Fail(p,3569,"network_equipment_original_old_weapon_destroy_dispatch_failed");
        if(!Live(actor,item.source.identity)){
            item.status=DeathItemStatus::WeakDestroyed;++life.weakDestroyed;++deathStats.weakDestroyed;
        }else {
            being=255;
            if(!DeathEvent(actor,deathMethods.beingDestroyed,&being)||being!=1)
                return Fail(p,3566,"network_equipment_native_destroy_unconfirmed_no_replay");
            item.status=DeathItemStatus::BeingDestroyed;++life.beingDestroyed;++deathStats.beingDestroyed;
        }
        if(item.orphan){++life.orphansDestroyed;++deathStats.orphansDestroyed;}
    }
    life.complete=true;life.stage="network_equipment_original_old_life_destroy_observed_peer_delete_unverified";
    ++deathStats.lifeCompleted;return true;
}
inline bool RetireCapturedLifeImpl(size_t index,void* pawn,uint32_t epoch){
    if(!Thread()||index>=MaxPlayers||!pawn||!epoch)return Fail(3570,"network_equipment_death_retire_context_invalid");
    auto& life=deathLives[index];if(life.inProgress)return Fail(3571,"network_equipment_death_retire_reentrant_callback");
    state.activeIndex=static_cast<unsigned>(index);life.inProgress=true;bool result=false;
    __try{result=RetireCapturedLifeBody(index,pawn,epoch);}__finally{life.inProgress=false;}
    return result;
}

inline void WriteDeathRetirements(FILE* file){
    const auto u=[](uint64_t value){return static_cast<unsigned long long>(value);};
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,",\"death_weapon_retirement\":{\"life_captures\":%llu,\"life_completed\":%llu,\"item_receipts\":%llu,\"factory_births\":%llu,\"items_captured\":%llu,\"destroy_calls\":%llu,\"weak_destroyed\":%llu,\"being_destroyed\":%llu,\"already_gone\":%llu,\"null_owner_orphans_destroyed\":%llu,\"transfers_proved\":%llu,\"foreign_owner_protected\":%llu,\"owned_mesh_components\":%llu,\"world_actor_scans\":0,\"mesh_destroy_calls\":0,\"peer_delete_verified\":false,\"lives\":[",
        u(deathStats.lifeCaptures),u(deathStats.lifeCompleted),u(deathStats.itemReceipts),u(deathStats.factoryBirths),u(deathStats.itemsCaptured),u(deathStats.destroyCalls),u(deathStats.weakDestroyed),u(deathStats.beingDestroyed),u(deathStats.alreadyGone),u(deathStats.orphansDestroyed),u(deathStats.transferred),u(deathStats.foreignProtected),u(deathStats.ownedComponents));
    bool comma=false;for(size_t n=0;n<deathLives.size();++n){const auto& life=deathLives[n];if(!life.captured)continue;
        std::fprintf(file,"%s{\"index\":%zu,\"epoch\":%u,\"pawn\":\"%p\",\"pawn_weak\":[%d,%d],\"stage\":\"%s\",\"complete\":%s,\"items\":[",comma?",":"",n,life.epoch,life.pawn,life.pawnIdentity.index,life.pawnIdentity.serial,life.stage,b(life.complete));
        for(size_t k=0;k<life.count;++k){const auto& q=life.items[k];
            std::fprintf(file,"%s{\"actor\":\"%p\",\"weak\":[%d,%d],\"factory_birth\":%s,\"observed_owned\":%s,\"status\":%u,\"destroy_attempted\":%s,\"observers_retired\":%s,\"owner_at_capture\":\"%p\",\"typed_owner_at_capture\":\"%p\",\"in_inventory_at_capture\":%u,\"last_owner\":\"%p\",\"last_typed_owner\":\"%p\",\"last_in_inventory\":%u,\"null_owner_orphan\":%s,\"owned_mesh_components\":%u,\"driver\":\"%p\",\"driver_context_known\":%s,\"driver_server_connection\":\"%p\",\"driver_replication_driver_7d0\":\"%p\",\"actor_net_driver_name64\":%llu,\"driver_net_driver_name64\":%llu,\"driver_name_matches\":%s}",
                k?",":"",q.source.actor,q.source.identity.index,q.source.identity.serial,b(q.source.factoryBirth),b(q.source.observedOwned),unsigned(q.status),b(q.destroyAttempted),b(q.observersRetired),q.ownerAtCapture,q.typedOwnerAtCapture,unsigned(q.inInventoryAtCapture),q.lastOwner,q.lastTypedOwner,unsigned(q.lastInInventory),b(q.orphan),q.ownedComponents,q.driver,b(q.driverContextKnown),q.serverConnection,q.replicationDriver,u(q.actorDriverName),u(q.driverName),b(q.driverNameMatches));
        }
        std::fputs("]}",file);comma=true;
    }
    std::fputs("],\"status_values\":{\"pending\":0,\"already_gone\":1,\"weak_destroyed\":2,\"being_destroyed\":3,\"transferred\":4,\"foreign_owner_protected\":5},\"death_methods\":{\"ready\":",file);
    std::fputs(b(deathMethods.ready),file);std::fputs(",\"owner_field_offset\":40,\"owner_class_getter_rva\":57161984,\"checks\":[",file);
    for(size_t n=0;n<deathMethods.checks.size();++n){const auto& q=deathMethods.checks[n];
        std::fprintf(file,"%s{\"method\":\"%s\",\"checked\":%s,\"found\":%s,\"expected_parms_size\":%u,\"actual_parms_size\":%u,\"expected_flags\":%u,\"actual_flags\":%u,\"expected_exec_rva\":%llu,\"actual_exec_rva\":%llu,\"expected_owner\":\"%p\",\"actual_owner\":\"%p\"}",
            n?",":"",n?"Actor.IsActorBeingDestroyed":"Actor.K2_DestroyActor",b(q.checked),b(q.found),unsigned(q.expectedBytes),unsigned(q.actualBytes),q.expectedFlags,q.actualFlags,u(q.expectedExec),u(q.actualExec),q.expectedOwner,q.actualOwner);
    }
    std::fputs("]}}",file);
}

inline bool PublishAttachmentIds(Transaction& t){
    auto& b=t.skinBinding;Guid ids[64]{};
    if(b.forcedCount>64)return false;
    for(unsigned i=0;i<b.forcedCount;++i){ids[i]=Read<Guid>(b.forced[i],0x38);if(!original_content::Valid(ids[i]))return false;}
    const auto existing=Read<NativeArray<Guid>>(t.candidate,0x1028);
    if(existing.data||existing.count||existing.capacity)return false;
    if(!b.forcedCount)return true;
    auto allocator=Read<void*>(reinterpret_cast<void*>(engine::image+0xd978ce8));
    auto allocate=Read<void*(*)(void*,size_t,uint32_t)>(Read<void*>(allocator),0x28);
    if(!allocator||!allocate)return false;
    auto data=static_cast<Guid*>(allocate(allocator,b.forcedCount*sizeof(Guid),8));
    if(!data)return false;
    std::memcpy(data,ids,b.forcedCount*sizeof(Guid));
    NativeArray<Guid> value{data,static_cast<int32_t>(b.forcedCount),static_cast<int32_t>(b.forcedCount)};
    if(!original_players::Write(t.candidate,0x1028,value)){engine::function<void(*)(void*)>(0x1915720)(data);return false;}
    return true;
}
inline bool PrepareNativeEquipEvents(Player& p,Transaction& t){
    auto actor=t.candidate;auto& r=t.equipRegistration;
    auto component=Read<void*>(actor,0xe18),registry=Read<void*>(component,0x198),type=engine::Class(0x6767a30);
    if(!original_players::IsA(component,0x6768f30)||Read<void*>(component,0xc8)!=actor
       ||!Read<uint8_t>(component,0x189)||!registry||!type
       ||engine::function<void*(*)(void*)>(0x6ff6000)(actor)!=registry)
        return Fail(p,3516,"network_equipment_original_event_registry_not_ready");
    r.component=reinterpret_cast<uintptr_t>(component);r.registry=reinterpret_cast<uintptr_t>(registry);
    r.type=reinterpret_cast<uintptr_t>(type);r.component_enabled=true;
    r.registered_before=engine::function<bool(*)(void*,void*)>(0x6ff68d0)(registry,type);
    // This shipped image omits RegisterEvent's ServerOnly=1 branch even in a
    // listen host. Supply the missing authority registration through its real
    // ServerAndClient=0 path, without changing NetMode/Role or event consumers.
    if(!r.registered_before)engine::function<void(*)(void*,void*,uint8_t)>(0x6ff5f40)(component,type,0);
    r.registered_after=engine::function<bool(*)(void*,void*)>(0x6ff68d0)(registry,type);r.valid=r.registered_after;
    if(!r.valid||!original_weapon::SupplyOriginalLocalEquipResponse(actor,r))
        return Fail(p,3517,"network_equipment_original_authored_equip_response_unavailable");
    return true;
}
inline bool BindNativeOwner(Player& p,Transaction& t){
    auto actor=t.candidate;
    if(!Event(actor,L"/Script/Engine.Actor.SetOwner",8,&p.pawn)||Read<void*>(actor,0x210)!=p.pawn)
        return Fail(p,3518,"network_equipment_native_owner_assignment_failed");
    if(Read<void*>(actor,0x4a8)!=p.pawn){
        auto previous=Read<void*>(actor,0x4a8);auto callback=Read<void*>(Read<void*>(actor),0x7d8);
        if(previous||callback!=reinterpret_cast<void*>(engine::image+0x70ec480))
            return Fail(p,3519,"network_equipment_native_owner_callback_profile_invalid");
        reinterpret_cast<void(*)(void*,void*,void*)>(callback)(actor,nullptr,p.pawn);
    }
    return Read<void*>(actor,0x4a8)==p.pawn||Fail(p,3520,"network_equipment_native_typed_owner_not_observed");
}
inline bool PrepareNativeOwnerSimulation(Player& p,Transaction& t){
    if(t.ownershipPolicyReady)return true;
    // NPC authority scheduling is independent. Do not add policy/API calls or
    // change NPC roles/ownership as part of the admitted human's grant.
    if(!Read<void*>(p.controller,0x510)){t.ownershipStatus=local_server_item_ownership::Status::NonHumanUnchanged;
        t.ownershipPolicyReady=true;return true;}
    const auto source=t.reused?local_server_item_ownership::Source::PublishedSlot:
        local_server_item_ownership::Source::PendingAuthorityGrant;
    if(!local_server_item_ownership::Prepare(state.world,p.controller,p.playerState,p.pawn,p.inventory,t.candidate,t.slot,source)){
        const auto& ownership=local_server_item_ownership::GetState();
        return Fail(p,ownership.error?ownership.error:3549,ownership.stage);
    }
    t.ownershipStatus=local_server_item_ownership::GetState().status;
    t.ownershipPolicyReady=t.ownershipStatus==local_server_item_ownership::Status::Published
        ||t.ownershipStatus==local_server_item_ownership::Status::AlreadyPublished;
    if(!t.ownershipPolicyReady)p.stage="network_equipment_waiting_actual_native_owner_simulation_policy";
    return true;
}
inline bool PrepareAuthorityAmmo(Player& p,Transaction& t){
    if(t.ammoPrepared||!t.ownershipPolicyReady)return true;
    const auto source=t.reused?local_server_item_ownership::Source::PublishedSlot:
        local_server_item_ownership::Source::PendingAuthorityGrant;
    const auto index=static_cast<size_t>(&p-players.data());
    const bool prepared=index==0
        ?local_server_resource_authority::PrepareWeaponAmmo(state.world,p.controller,p.playerState,p.pawn,p.inventory,t.candidate,p.sourceEpoch,t.slot,source)
        :local_server_resource_authority::PrepareNpcWeaponAmmo(index,state.world,p.controller,p.playerState,p.pawn,p.inventory,t.candidate,p.sourceEpoch,t.slot,source);
    if(!prepared){
        const auto& resource=local_server_resource_authority::GetState();
        return Fail(p,resource.error?resource.error:3550,resource.stage);
    }
    t.ammoPrepared=true;return true;
}
inline bool Spawn(Player& p){
    auto& t=p.transaction;engine::Transform transform;
    if(!Event(p.pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&transform.translation))
        return Fail(p,3521,"network_equipment_authority_pawn_transform_unavailable");
    t.candidate=engine::function<void*(*)(void*,void*,const engine::Transform*,uint8_t,void*,uint8_t)>(0x43fada0)
        (p.pawn,t.definition->runtimeClass,&transform,1,p.pawn,0);
    if(!t.candidate||Read<uint8_t>(t.candidate,0x224)!=3||!Capture(t.candidate,t.candidateIdentity))
        return Fail(p,3522,"network_equipment_native_authority_spawn_failed");
    if(!RememberLifeItem(p,t.candidate,t.candidateIdentity,true))return false;
    uint8_t begun=0;
    if(!Event(t.candidate,L"/Script/Engine.Actor.HasActorBegunPlay",1,&begun)||begun)
        return Fail(p,3523,"network_equipment_skin_input_must_precede_native_begin_play");
    auto& b=t.skinBinding;b.world=state.world;b.pawn=p.pawn;b.weapon=t.candidate;
    b.weaponId=t.definition->id;b.skinId=t.selection.skin;b.levelId=t.selection.level;b.chromaId=t.selection.chroma;
    b.skinData=t.selection.skinData;b.levelData=t.selection.levelData;b.chromaData=t.selection.chromaData;
    if(!original_players::Write(t.candidate,0xfa8,t.definition->dataAsset)
       ||!all_skins::ForcedAttachments(b,*t.definition)||!PublishAttachmentIds(t)
       ||!original_players::Write(t.candidate,0xfb0,b.skinData)||!original_players::Write(t.candidate,0xfb8,b.levelData)
       ||!original_players::Write(t.candidate,0xfd0,b.chromaData)
       ||!original_players::Write(t.candidate,0xfe8,b.skinId)||!original_players::Write(t.candidate,0xff8,b.levelId)
       ||!original_players::Write(t.candidate,0x1038,b.chromaId))
        return Fail(p,3524,"network_equipment_original_prefinish_replicated_skin_inputs_failed");
    engine::function<void(*)(void*,const engine::Transform*,bool,void*,uint8_t)>(0x3ef6cb0)
        (t.candidate,&transform,false,nullptr,0);
    if(!original_effects::EnsureActor(t.candidate,"network_authority_equipment"))
        return Fail(p,3537,"network_equipment_original_effect_context_not_ready");
    if(!BindNativeOwner(p,t))return false;
    uint8_t replicate=1;
    if(!Event(t.candidate,L"/Script/Engine.Actor.SetReplicates",1,&replicate)||Read<uint8_t>(t.candidate,0x224)!=3)
        return Fail(p,3525,"network_equipment_original_replication_enable_failed");
    if(!PrepareNativeOwnerSimulation(p,t)||!PrepareAuthorityAmmo(p,t))return false;
    if(!SkinMatches(t.candidate,*t.definition,t.selection))return Fail(p,3538,"network_equipment_native_skin_fields_not_read_back");
    if(!PrepareNativeEquipEvents(p,t))return false;
    p.phase=Phase::Publish;
    if(t.ownershipPolicyReady)p.stage="network_equipment_authority_native_actor_constructed";return true;
}
inline bool AddRetired(Player& p,void* actor,const Weak& identity){
    if(!actor||!Live(actor,identity))return true;
    for(const auto& r:p.retired)if(r.actor==actor)return true;
    for(auto& r:p.retired)if(!r.actor){r={actor,identity};return true;}
    return Fail(p,3526,"network_equipment_authority_retirement_queue_full");
}
inline bool SlotContains(void* slot,void* actor){
    if(!slot||!actor)return false;
    if(!original_players::IsA(slot,0x69cf3d0))return Read<void*>(slot,0x38)==actor;
    const auto array=Read<NativeArray<void*>>(slot,0x50);
    if(array.count<0||array.count>256||array.capacity<array.count||(!array.data&&array.count))return true;
    for(int32_t i=0;i<array.count;++i)if(Read<void*>(array.data,static_cast<size_t>(i)*8)==actor)return true;
    return false;
}
inline bool Retire(Player& p){
    for(auto& r:p.retired){
        if(!r.actor)continue;
        if(!Live(r.actor,r.identity)){r={};continue;}
        if(r.actor==Read<void*>(p.inventory,0x280)||r.actor==Read<void*>(p.inventory,0x268)
           ||r.actor==Read<void*>(p.inventory,0x320)||r.actor==Read<void*>(p.inventory,0x2a0))continue;
        bool retained=false;
        for(size_t i=0;i<SlotCount;++i)retained=retained||SlotContains(Read<void*>(p.inventory,0x1b8+i*8),r.actor);
        if(retained)continue;
        if(!local_server_resource_authority::RetireWeaponAmmo(r.actor)||!original_effects::RetireActor(r.actor)||!Event(r.actor,L"/Script/Engine.Actor.K2_DestroyActor",0,nullptr))
            return Fail(p,3527,"network_equipment_original_replaced_actor_retirement_failed");
        r={};++p.retiredActors;
    }
    return true;
}
inline bool Publish(Player& p){
    auto& t=p.transaction;
    // Retry only the existing candidate's policy while native ownership/slot
    // dependencies settle. Never respawn it or call SetReplicates again.
    if(!PrepareNativeOwnerSimulation(p,t)||!PrepareAuthorityAmmo(p,t))return false;
    if(!t.ownershipPolicyReady)return true;
    if(!t.reused){
        if(Read<void*>(t.slot,0x38)!=t.replaced)return Fail(p,3528,"network_equipment_original_slot_changed_during_transaction");
        // AuthAddItem is an absent authority leaf in this client build. This
        // host supplies grant data in the original replicated slot; the client
        // receives native net fields and runs original RepNotifies itself.
        if(!original_players::Write(t.slot,0x38,t.candidate)||!original_players::Write<uint8_t>(t.candidate,0x4b0,1)
           ||!Event(t.slot,L"/Script/ShooterGame.ItemSlot.OnRep_Contents",0,nullptr))
            return Fail(p,3529,"network_equipment_original_authority_slot_publication_failed");
        if(t.replaced&&Live(t.replaced,t.replacedIdentity)&&!original_players::Write<uint8_t>(t.replaced,0x4b0,0))
            return Fail(p,3530,"network_equipment_original_replaced_slot_state_failed");
        if(!AddRetired(p,t.replaced,t.replacedIdentity))return false;
    }
    t.published=true;
    if(t.request.mode==Mode::Grant){
        ++p.completed;++p.generation;p.lastCompletedSequence=t.request.sequence;p.phase=Phase::Idle;
        p.stage="network_equipment_original_authority_slot_granted";return true;
    }
    p.phase=Phase::RequestEquip;return true;
}
inline void Sample(Player& p){
    p.current=Read<void*>(p.inventory,0x280);p.desired=Read<void*>(p.inventory,0x268);
    p.nativeEquipped=p.current&&p.current==p.desired&&Read<void*>(p.current,0x210)==p.pawn
        &&Read<void*>(p.current,0x4a8)==p.pawn;
    p.correctionPublished=Read<uint8_t>(p.inventory,0x30c)&&Read<void*>(p.inventory,0x320)==p.current;
}
inline bool PublishReplicatedCurrent(Player& p){
    if(p.replicatedChangePublished&&p.lastReplicatedCurrent==p.current
       &&Read<void*>(p.inventory,0x2a0)==p.current)return true;
    if(p.current&&(!original_players::IsA(p.current,0x645c660)
       ||Read<void*>(p.current,0x210)!=p.pawn||Read<void*>(p.current,0x4a8)!=p.pawn))
        return Fail(p,3545,"network_equipment_native_current_owner_invalid_for_replication");
    PendingEquippableChange change;change.current=p.current;
    // Exact retained timestamp constructor: weak identity, original character
    // timekeeper, net-clock conversion and actual RespawnNumber. Do not invent
    // world seconds, an epoch, a serial or a client-side equip state.
    auto built=engine::function<NetworkedMovementTimestamp*(*)(NetworkedMovementTimestamp*,void*)>(0x78a2a90)
        (&change.timestamp,p.pawn);
    if(built!=&change.timestamp||!Live(p.pawn,change.timestamp.character)
       ||!std::isfinite(change.timestamp.netTimestamp)||change.timestamp.respawnNumber!=Read<int32_t>(p.pawn,0x143c))
        return Fail(p,3546,"network_equipment_original_native_movement_timestamp_not_read_back");
    // The official authority producer is absent from this client image. This
    // host supplies that producer's original reflected 24-byte net property;
    // original remote OnRep70ffce0 keeps its mapping/time/epoch checks and owns
    // all resulting simulated Current/Desired and equip notifications.
    if(!original_players::Write(p.inventory,0x2a0,change))
        return Fail(p,3547,"network_equipment_native_authority_change_publication_failed");
    auto observed=Read<PendingEquippableChange>(p.inventory,0x2a0);
    if(observed.current!=p.current||std::memcmp(&observed.timestamp,&change.timestamp,sizeof(change.timestamp)))
        return Fail(p,3548,"network_equipment_native_authority_change_read_back_mismatch");
    p.lastReplicatedCurrent=p.current;p.lastChangeTimestamp=change.timestamp;
    p.replicatedChangePublished=true;++p.replicatedChanges;return true;
}
inline bool RequestNativeCorrection(Player& p){
    if(Read<uint8_t>(p.pawn,0x128)!=2)return true;
    // Only a receiving UPlayer needs an owner correction. The original
    // PC.GetNetConnection (4929A80) returns null immediately for a null Player.
    // NPC equipment still uses simulated PendingEquippableChange. The host
    // does not request owner deferrals for a character without a recipient.
    // Real players keep the original connection/channel/open-ack retry path.
    if(!Read<void*>(p.controller,0x510))return true;
    if(!Read<void*>(p.pawn,0xba0))return Fail(p,3531,"network_equipment_native_input_state_missing_for_correction");
    // This adapter supplies the missing first owner-correction producer only.
    // A new native correction directly changes Current/Desired, resets weapon
    // state and resets the owned InputState in70FA910. Requesting one merely
    // because Current changed replays an ordinary already-predicted equip.
    // Actual native30C is a publication receipt, never a client-delivery ACK.
    // After that receipt, original70F7B70 keeps its state2 deferral/retry path
    // and original70A78A0 Tag7 requests correction for a real input/current
    // mismatch. Neither native path nor its counter/ACK rules is replaced.
    if(!p.initialCorrectionPublicationObserved&&Read<uint8_t>(p.inventory,0x30c)){
        p.initialCorrectionPublicationObserved=true;
        p.initialCorrectionSequence=Read<int32_t>(p.inventory,0x328);
        p.lastCorrection=Read<void*>(p.inventory,0x320);
    }
    if(p.initialCorrectionPublicationObserved){++p.postBootstrapCorrectionSkips;return true;}
    // Exact original callsite70F7BCA: RCX=inventory, R8b=0. Preserve actual PC,
    // channel/open-ACK checks and native deferral. First publication may be an
    // empty inventory; subsequent native mismatch handles the real loadout.
    engine::function<void(*)(void*,uintptr_t,uint8_t)>(0x70fa670)(p.inventory,0,0);
    ++p.correctionRequests;Sample(p);
    if(Read<uint8_t>(p.inventory,0x30c)){
        p.initialCorrectionPublicationObserved=true;
        p.initialCorrectionSequence=Read<int32_t>(p.inventory,0x328);
        p.lastCorrection=Read<void*>(p.inventory,0x320);
    }
    return true;
}
inline bool PollPlayer(Player& p){
    if(p.retiring)return true;
    if(p.error||!Ownership(p))return Fail(p,p.error?p.error:3532,p.error?p.stage:"network_equipment_player_identity_or_possession_changed");
    if(!TrackLifeInventory(p))return false;
    Sample(p);
    if(!PublishReplicatedCurrent(p)||!RequestNativeCorrection(p)||!Retire(p))return false;
    if(p.phase==Phase::Idle)return true;
    auto& t=p.transaction;
    if(GetTickCount64()-t.beganAt>20000)return Fail(p,3533,"network_equipment_native_transaction_timeout_no_state_forced");
    switch(p.phase){
    case Phase::Spawn:return Spawn(p);
    case Phase::Publish:return Publish(p);
    case Phase::RequestEquip:{
        if(!Live(t.candidate,t.candidateIdentity)||!BindNativeOwner(p,t)||!PrepareNativeOwnerSimulation(p,t)||!PrepareAuthorityAmmo(p,t))return false;
        if(!t.ownershipPolicyReady)return true;
        if(!PrepareNativeEquipEvents(p,t))return false;
        struct Params{uint8_t source=9,skipRestrictions=0,result=0;} params;
        if(!Event(t.candidate,L"/Script/ShooterGame.AresEquippable.TryEquip",3,&params)||!params.result)
            return Fail(p,3534,"network_equipment_original_try_equip_rejected");
        t.equipAccepted=true;p.phase=Phase::WaitCurrent;p.stage="network_equipment_waiting_original_authority_equip_state";return true;
    }
    case Phase::WaitCurrent:
        if(!Live(t.candidate,t.candidateIdentity))return Fail(p,3535,"network_equipment_native_candidate_expired");
        if(p.current!=t.candidate||p.desired!=t.candidate)return true;
        p.nativeComponentsPresent=Read<void*>(t.candidate,0xe00)&&Read<void*>(t.candidate,0xde0)&&Read<void*>(t.candidate,0xdf0);
        if(t.definition->categoryValue!=7)p.nativeComponentsPresent=p.nativeComponentsPresent
            &&original_weapon::Component(t.candidate,0x684d4c0)&&original_weapon::Component(t.candidate,0x63c7e00)
            &&original_weapon::Component(t.candidate,0x6d1e460);
        if(!p.nativeComponentsPresent)return Fail(p,3536,"network_equipment_original_native_weapon_components_missing");
        ++p.completed;++p.generation;p.lastCompletedSequence=t.request.sequence;p.phase=Phase::Idle;
        p.stage="network_equipment_original_authority_equipped_client_unverified";return RequestNativeCorrection(p);
    default:return false;
    }
}
inline bool PollImpl(float delta){
    if(!Thread()||!state.initialized||!std::isfinite(delta)||delta<0||delta>1)return false;
    ++state.polls;bool ok=true;
    for(size_t i=0;i<MaxPlayers;++i){auto& p=players[i];if(!p.registered)continue;state.activeIndex=static_cast<unsigned>(i);
        if(!p.error&&p.phase==Phase::Idle&&p.nativePurchasePending){
            p.nativePurchasePending=false;
            if(!EquipImpl(i,p.nativePurchase)&&p.error){ok=false;continue;}
        }
        if(!PollPlayer(p)){ok=false;continue;}
        if(!local_server_resource_authority::PollPlayerWeaponAmmo(p.pawn)){
            const auto& resource=local_server_resource_authority::GetState();
            Fail(p,resource.error?resource.error:3551,resource.stage);ok=false;
        }
    }
    return ok;
}
inline bool AddReference(Graph& graph,void* actor){
    if(!actor)return true;
    if(!original_players::IsA(actor,0x645c570))return false;
    for(size_t i=0;i<graph.actorCount;++i)if(graph.actors[i]==actor)return true;
    if(graph.actorCount==MaxReferences){graph.complete=false;return false;}
    graph.actors[graph.actorCount++]=actor;return true;
}
inline bool CollectGraph(size_t index,Graph& graph){
    graph={};
    if(!Thread()||index>=MaxPlayers||!Ownership(players[index]))return false;
    const auto& p=players[index];graph.pawn=p.pawn;graph.inventory=p.inventory;
    for(size_t i=0;i<SlotCount;++i){
        auto slot=Read<void*>(p.inventory,0x1b8+i*8);graph.slots[i]=slot;
        if(!original_players::IsA(slot,0x68f63c0)||Read<void*>(slot,0x40)!=p.inventory){graph.complete=false;continue;}
        ++graph.slotCount;
        if(original_players::IsA(slot,0x69cf3d0)){
            const auto array=Read<NativeArray<void*>>(slot,0x50);
            if(array.count<0||array.count>256||array.capacity<array.count||(!array.data&&array.count)){graph.complete=false;continue;}
            for(int32_t n=0;n<array.count;++n)graph.complete=AddReference(graph,Read<void*>(array.data,static_cast<size_t>(n)*8))&&graph.complete;
        }else graph.complete=AddReference(graph,Read<void*>(slot,0x38))&&graph.complete;
    }
    for(size_t offset:{size_t(0x268),size_t(0x278),size_t(0x280),size_t(0x2a0),size_t(0x320)})
        graph.complete=AddReference(graph,Read<void*>(p.inventory,offset))&&graph.complete;
    if(p.phase!=Phase::Idle&&p.transaction.ownershipPolicyReady&&p.transaction.ammoPrepared&&Live(p.transaction.candidate,p.transaction.candidateIdentity))
        graph.complete=AddReference(graph,p.transaction.candidate)&&graph.complete;
    for(const auto& r:p.retired)if(Live(r.actor,r.identity))graph.complete=AddReference(graph,r.actor)&&graph.complete;
    return graph.complete;
}
inline bool ObserveClient(void* pawn,ClientObservation& observation){
    observation={};
    if(!Thread()||!original_players::IsA(pawn,original_players::ShooterCharacterClass)||Read<uint8_t>(pawn,0x224)==3)return false;
    observation.pawn=pawn;observation.inventory=Read<void*>(pawn,0xc08);
    observation.originalInventory=original_players::IsA(observation.inventory,0x6499330)&&Read<void*>(observation.inventory,0xc8)==pawn;
    if(!observation.originalInventory)return false;
    observation.current=Read<void*>(observation.inventory,0x280);observation.desired=Read<void*>(observation.inventory,0x268);
    observation.initialCorrectionProcessed=Read<uint8_t>(observation.inventory,0x30d)!=0;
    observation.serverCorrectionSequence=Read<int32_t>(observation.inventory,0x328);
    observation.processedCorrectionSequence=Read<int32_t>(observation.inventory,0x2f8);
    observation.correctionInputSequence=Read<int32_t>(observation.inventory,0x32c);
    const auto input=Read<void*>(pawn,0xba0);
    if(input)observation.clientInputCorrectionSequence=Read<int32_t>(input,0x260);
    // Game-thread native reads only. Runtime/JSON readers may copy these POD
    // counts without invoking a UObject/native function from another thread.
    if(Read<uint8_t>(pawn,0x224)==2){
        observation.ownershipObserved=local_server_item_ownership::ObserveClient(pawn,observation.ownership);
        if(!observation.ownershipObserved&&local_server_item_ownership::GetState().error)return false;
    }
    for(size_t i=0;i<SlotCount;++i){auto slot=Read<void*>(observation.inventory,0x1b8+i*8);
        if(original_players::IsA(slot,0x68f63c0)&&Read<void*>(slot,0x40)==observation.inventory&&Read<uint8_t>(slot,0x30)==i)++observation.slotCount;
    }
    auto actor=observation.current;if(!actor)return true;
    auto definition=all_weapons::FindByClass(Read<void*>(actor,0x10));
    if(definition)observation.weapon=definition->id;
    observation.owner=Read<void*>(actor,0x210);observation.typedOwner=Read<void*>(actor,0x4a8);
    observation.ownerMatches=observation.owner==pawn&&observation.typedOwner==pawn;
    observation.primaryData=Read<void*>(actor,0xfa8);observation.skinData=Read<void*>(actor,0xfb0);
    observation.levelData=Read<void*>(actor,0xfb8);observation.chromaData=Read<void*>(actor,0xfd0);
    observation.skin=Read<Guid>(actor,0xfe8);observation.level=Read<Guid>(actor,0xff8);observation.chroma=Read<Guid>(actor,0x1038);
    observation.mesh1P=Read<void*>(actor,0xde0);observation.mesh3P=Read<void*>(actor,0xdf0);
    observation.machine=Read<void*>(actor,0xe00);observation.predictedState=Read<void*>(observation.machine,0x1b0);
    observation.nativeComponentsPresent=observation.mesh1P&&observation.mesh3P&&observation.machine;
    observation.appearanceFieldsPresent=observation.primaryData&&observation.skinData&&observation.levelData&&observation.chromaData;
    return true;
}
inline bool ClientOwnerNotification(void* pawn,void* actor){
    if(!actor||!original_players::IsA(actor,0x645c660)||Read<void*>(actor,0x210)!=pawn)return true;
    if(Read<uint8_t>(actor,0x224)==3)return Fail(3539,"network_equipment_client_received_local_authority_weapon");
    auto typedOwner=Read<void*>(actor,0x4a8);
    if(typedOwner==pawn)return true;
    if(typedOwner)return true; // Previous owner can await the native owner edge.
    auto callback=Read<void*>(Read<void*>(actor),0x7d8);
    if(callback!=reinterpret_cast<void*>(engine::image+0x70ec480))
        return Fail(3540,"network_equipment_client_original_owner_callback_mismatch");
    // Engine.Owner has already arrived from the host and the actual replicated
    // slot references this item. Complete its original Ares ownership callback
    // if that independent native notification has not bound MyPawn yet. No
    // client Owner/slot/skin/state/role/inventory-authority field is assigned.
    reinterpret_cast<void(*)(void*,void*,void*)>(callback)(actor,nullptr,pawn);
    ++state.clientOwnerNotifications;
    return Read<void*>(actor,0x4a8)==pawn||Fail(3541,"network_equipment_client_native_owner_notification_failed");
}
inline bool PollClientImpl(void* pawn){
    if(!Thread()||!state.catalogReady)return false;
    if(!original_players::IsA(pawn,original_players::ShooterCharacterClass)||Read<uint8_t>(pawn,0x224)==3)return false;
    auto inventory=Read<void*>(pawn,0xc08);
    if(!original_players::IsA(inventory,0x6499330)||Read<void*>(inventory,0xc8)!=pawn)return true;
    bool notify=false;
    for(size_t i=0;i<SlotCount;++i){
        auto slot=Read<void*>(inventory,0x1b8+i*8);
        if(!slot)continue;
        if(!original_players::IsA(slot,0x68f63c0)||Read<void*>(slot,0x28)!=inventory)
            return Fail(3542,"network_equipment_client_replicated_slot_outer_mismatch");
        notify=notify||Read<void*>(slot,0x40)!=inventory||Read<uint8_t>(slot,0x30)!=i;
    }
    if(notify){
        // Original RepNotify assembly only, for a resolved native replicated
        // slot graph whose per-slot backrefs have not caught up. It performs
        // the same native type/backref binding and OnItemsChanged broadcast.
        if(!Event(inventory,L"/Script/ShooterGame.AresInventory.OnRep_ItemSlots",0,nullptr))
            return Fail(3543,"network_equipment_client_original_slot_repnotify_unavailable");
        ++state.clientSlotNotifications;
    }
    for(size_t i=0;i<SlotCount;++i){
        auto slot=Read<void*>(inventory,0x1b8+i*8);if(!slot)continue;
        if(original_players::IsA(slot,0x69cf3d0)){
            const auto array=Read<NativeArray<void*>>(slot,0x50);
            if(array.count<0||array.count>256||array.capacity<array.count||(!array.data&&array.count))return false;
            for(int32_t n=0;n<array.count;++n)if(!ClientOwnerNotification(pawn,Read<void*>(array.data,static_cast<size_t>(n)*8)))return false;
        }else if(!ClientOwnerNotification(pawn,Read<void*>(slot,0x38)))return false;
    }
    return true;
}
inline bool UnregisterPlayer(size_t index){
    if(!Thread()||index>=MaxPlayers)return false;
    auto& p=players[index];if(p.registered&&state.registeredPlayers)--state.registeredPlayers;
    // Cached records only. Match lifecycle owns pawn/equipment destruction.
    p={};return true;
}
inline LONG CaptureException(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0003500U;
    auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;state.restartRequired=true;
    state.exceptionOperation=state.stage;state.stage="network_equipment_exception_requires_new_session";
    if(state.activeIndex<MaxPlayers){auto& p=players[state.activeIndex];p.restartRequired=true;Fail(p,state.error,state.stage);}
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool CaptureRetiringLife(size_t index,void* pawn,uint32_t epoch){
    __try{return CaptureRetiringLifeImpl(index,pawn,epoch);}__except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool RetireCapturedLife(size_t index,void* pawn,uint32_t epoch){
    __try{return RetireCapturedLifeImpl(index,pawn,epoch);}__except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool Initialize(void* world){__try{return InitializeImpl(world);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool PrepareClientAppearance(){__try{return PrepareClientAppearanceImpl();}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool RegisterPlayer(size_t index,void* controller,void* playerState,void* pawn,uint32_t epoch=1){
    __try{return RegisterPlayerImpl(index,controller,playerState,pawn,epoch);}__except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool Equip(size_t index,const Request& request){__try{return EquipImpl(index,request);}__except(CaptureException(GetExceptionInformation())){return false;}}
// Actual host-PC instance v1330 thunk. Root installs it in the existing private
// controller table owner; this module never competes with the native input
// module by copying/replacing the controller table a second time.
inline void NativePurchase(void* controller,void* weaponClass,uint8_t targetSlot,uint8_t purchaseType){
    __try{NativePurchaseImpl(controller,weaponClass,targetSlot,purchaseType);}__except(CaptureException(GetExceptionInformation())){}
}
inline bool Poll(float delta){__try{return PollImpl(delta);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool PollClient(void* pawn){__try{return PollClientImpl(pawn);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    std::fprintf(file,"{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"profiles_verified\":%s,\"schema_verified\":%s,\"catalog_ready\":%s,\"verified_definitions\":%u,\"client_appearance_prepared\":%s,\"restart_required\":%s,\"exception_rva\":%llu,\"polls\":%llu,\"client_visual_or_fire_proven\":false,\"players\":[",
        state.stage,state.error,state.profilesVerified?"true":"false",state.schemaVerified?"true":"false",state.catalogReady?"true":"false",
        state.verifiedDefinitions,state.clientAppearancePrepared?"true":"false",state.restartRequired?"true":"false",
        static_cast<unsigned long long>(state.exceptionRva),static_cast<unsigned long long>(state.polls));
    bool comma=false;
    for(size_t i=0;i<MaxPlayers;++i){const auto& p=players[i];if(!p.registered)continue;
        std::fprintf(file,"%s{\"index\":%zu,\"stage\":\"%s\",\"error\":%u,\"phase\":%u,\"accepted\":%u,\"completed\":%u,\"rejected\":%u,\"last_rejection\":\"%s\",\"generation\":%u,\"native_equipped\":%s,\"native_components_present\":%s,\"native_correction_published\":%s,\"correction_requests\":%llu,\"retired_actors\":%llu,\"pawn\":\"%p\",\"inventory\":\"%p\",\"current\":\"%p\",\"desired\":\"%p\",\"candidate\":\"%p\",\"replicated_changes\":%llu,\"change_net_timestamp\":%.9g,\"change_respawn\":%d,\"native_purchases\":%llu,\"native_purchase_pending\":%s,\"native_purchase_target_slot\":%u,\"native_purchase_type\":%u,\"money\":%d,\"money_ready\":%s",
            comma?",":"",i,p.stage,p.error,static_cast<unsigned>(p.phase),p.accepted,p.completed,p.rejected,p.lastRejection,p.generation,
            p.nativeEquipped?"true":"false",p.nativeComponentsPresent?"true":"false",p.correctionPublished?"true":"false",
            static_cast<unsigned long long>(p.correctionRequests),static_cast<unsigned long long>(p.retiredActors),p.pawn,p.inventory,p.current,p.desired,p.transaction.candidate,
            static_cast<unsigned long long>(p.replicatedChanges),static_cast<double>(p.lastChangeTimestamp.netTimestamp),p.lastChangeTimestamp.respawnNumber,
            static_cast<unsigned long long>(p.nativePurchases),p.nativePurchasePending?"true":"false",static_cast<unsigned>(p.nativePurchaseTargetSlot),
            static_cast<unsigned>(p.nativePurchaseType),p.money,p.moneyReady?"true":"false");
        std::fprintf(file,",\"initial_correction_publication_observed\":%s,\"initial_correction_sequence\":%d,\"post_bootstrap_correction_skips\":%llu}",
            p.initialCorrectionPublicationObserved?"true":"false",p.initialCorrectionSequence,
            static_cast<unsigned long long>(p.postBootstrapCorrectionSkips));
        comma=true;
    }
    std::fputs("]",file);WriteDeathRetirements(file);
    std::fputs("}\n",file);const bool ok=std::ferror(file)==0;std::fclose(file);return ok;
}
} // namespace local_server_equipment
