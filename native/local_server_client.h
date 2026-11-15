#pragma once
#include "original_hud.h"
#include "original_players.h"
#include "original_inventory_slots.h"
#include "original_input_bindings.h"
#include "original_ui_input_mode.h"
#include "local_server_menu_input.h"
#include "original_camera.h"
#include "local_server_spectator_view.h"
#include "original_ads.h"
#include "original_render.h"
#include "original_audio_listeners.h"
#include "original_streak_hud.h"
#include "original_deathmatch_topbar.h"
#include "local_server_equipment.h"
#include "local_server_feedback_view.h"
#include "original_fault_observer.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

// Presentation for the ACTUAL autonomous native client of the loopback server.
// Initialize only after the engine has associated the sole ShooterLocalPlayer,
// the connection's ShooterPC/PS and its received Role2 character. The native
// client already owns prediction, action production, camera/recoil/ADS updates,
// physics, RPC callspace and viewport ownership. None is reimplemented here.
// Static evidence and call profiles: work/local-server-playable/client-presentation/.
namespace local_server_client {
using Weak=original_inventory_slots::WeakIdentity;
struct Array {void** data=nullptr;int32_t count=0,capacity=0;};
struct Name {uint32_t words[3]{};};
struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr Profile Profiles[]{
    {0x718e090,29,0x5739f4c310ec96ceULL},
    {0x7a8c590,1706,0xf667aab9bcd49243ULL},
    {0x3c514e0,2269,0x77024c9a5754a940ULL},
    {0x7ab5f30,819,0xf097bd64ba10a635ULL},
    {0x5ef6250,174,0x978bd1d0f4f83c30ULL},
    {0x7a8e180,2133,0xa3d1a1b4c04425e9ULL},
    {0x18bd090,164,0xc19c33bdf3acd586ULL},
    {0x1915720,59,0x9330f93033e42badULL},
    {0x3efc820,176,0x7f1113b3f79164b2ULL},
    {0x78b1f80,84,0x3617a51f2392299fULL},
    {0x7a8d5c0,110,0x7e4e386241fa2f71ULL},
    {0x3ed83b0,1609,0x15d2d4ecc2f49cf7ULL},
    {0x70f82b0,248,0x435698ca5f5cafc5ULL},
    {0x1af0eb0,174,0x9d5828d1e3b390e3ULL},
};
inline constexpr size_t ShopStartingClassLimit=64;
struct ShopStartingClassReadback {
    void *type=nullptr,*defaults=nullptr,*item=nullptr;
    Weak typeId{},defaultsId{},itemId{};
    bool typeLive=false,classObject=false,defaultsLive=false,classEquippable=false,defaultsDeferred=false;
    bool queried=false,itemLive=false,itemEquippable=false,itemOwned=false;
    uint8_t defaultSlot=0xff;
};
struct ShopOpenPathSnapshot {
    uint64_t samples=0,lastSampleMs=0,nativeLookupTotal=0;
    uintptr_t getWorldRva=0;
    bool coordinatorLive=false,controllerMatches=false,getWorldProfiled=false,worldQueried=false;
    bool worldLive=false,worldMatches=false,playerStateQueried=false,playerStateMatches=false;
    bool gameStateLive=false,gameStateShooter=false,entryArrayValid=false,entryLive=false;
    bool screenQueried=false,screenLive=false,screenMatches=false,stackLive=false,stackArrayValid=false,entryInOpenStack=false;
    bool characterRuleAllowed=false,characterShopAllowed=false,characterRuleRequired=false;
    bool spawnedLive=false,spawnedMatches=false,inventoryLive=false,inventoryOwned=false,slotsLive=false;
    bool startingArrayValid=false,startingClassesComplete=false,clientGameInstanceLive=false;
    bool flowManagerLive=false,flowStateLive=false,flowInGame=false,originalTickInputsObserved=false;
    int32_t entryCount=0,openEntryCount=0,startingClassCount=0,observedStartingClasses=0;
    int32_t nativeLookupCalls=0,deferredDefaults=0,missingItems=0,invalidStartingClasses=0;
    uint32_t screenFlags=0;
    uint8_t flowStateType=0xff;
    void *controller=nullptr,*world=nullptr,*playerState=nullptr,*gameState=nullptr,*screen=nullptr;
    void *entry=nullptr,*menuStack=nullptr,*gameInfoModel=nullptr,*playerModel=nullptr;
    void *spawnedCharacter=nullptr,*inventory=nullptr,*worldGameInstance=nullptr,*clientGameInstance=nullptr;
    void *flowManager=nullptr,*flowState=nullptr;
    Weak coordinatorId{},worldId{},playerStateId{},gameStateId{},entryId{},screenId{},stackId{};
    Weak spawnedId{},inventoryId{},clientGameInstanceId{},flowManagerId{},flowStateId{};
    std::array<ShopStartingClassReadback,ShopStartingClassLimit> startingClasses{};
    const char* stage="native_shop_readonly_path_not_sampled";
};
struct Shop {
    bool initialized=false,cacheSupplied=false,coordinatorSupplied=false,screenSupplied=false;
    bool nativePurchaseRPCVerified=false,shoppingEnabled=false,openKnown=false,open=false;
    bool ownerVerified=false,inventoryRefreshPending=true;
    bool canOpenABIValid=false,canOpenQueried=false,nativeCanOpen=false,coordinatorCanOpen=false,screenNativeOpen=false;
    int32_t money=0,storeCount=0,entryCount=0,modelCount=0;
    int32_t modelGroupCount=0,observedModelCount=0,storePurchasableCount=0;
    unsigned error=0;
    uint64_t toggles=0,refreshPasses=0,refreshCalls=0,inventorySignature=0;
    void *gameState=nullptr,*store=nullptr,*playerInfo=nullptr,*purchaseComponent=nullptr,*moneyComponent=nullptr;
    void *ui=nullptr,*uiModel=nullptr,*menuStack=nullptr,*coordinator=nullptr,*screen=nullptr,*entry=nullptr;
    void *notifiedPlayerState=nullptr;
    ShopOpenPathSnapshot path{};
    const char* stage="native_client_shop_not_started";
};
struct WeaponSnapshot {
    void *inventory=nullptr,*weapon=nullptr,*stability=nullptr,*ammo=nullptr;
    bool originalOwner=false,recoilQueried=false,ammoQueried=false;
    float pitchRecoil=0,yawRecoil=0,firingError=0;
    int32_t currentAmmo=-1,authorityAmmo=-1,predictedAmmo=-1;
};
enum class Probe:size_t {Equipment,HUD,ShopConstruction,Camera,ADS,Render,WeaponReadbacks,Audio,Streak,UI,ShopInventory,Topbar,Ownership,Initialization,AppearancePreparation,Count};
inline constexpr const char* ProbeNames[]{"equipment_replica","original_hud","shop_construction","native_camera_sample",
    "native_ads_sample","render_cache_sample","weapon_readbacks","audio_registry_sample","streak_hud_sample",
    "ui_input_mode","shop_inventory_models","replica_topbar","ownership_audit","local_method_resolution","appearance_preparation"};
static_assert(sizeof(ProbeNames)/sizeof(ProbeNames[0])==size_t(Probe::Count));
struct ProbeObservation {uint64_t calls=0,faultDelta=0,lastFaultDelta=0;};
struct AbilityObjectIdentity {
    void* object=nullptr;Weak identity{};
};
struct SpectatedAbilityRecord {
    bool valid=false;uint64_t targetGeneration=0;int32_t playerId=-1;
    AbilityObjectIdentity world{},controller{},hud{},panel{},actor{},playerState{},widget{},parent{},camera{},viewComponent{};
};
struct AbilityWidgetAudit {
    void *actor=nullptr,*widget=nullptr,*owner=nullptr,*parent=nullptr,*previousActor=nullptr;
    void *previousWidget=nullptr,*previousParent=nullptr;
    void *logicalTarget=nullptr,*localTarget=nullptr,*cameraTarget=nullptr,*pendingTarget=nullptr;
    Weak actorId{},widgetId{},ownerId{},parentId{},previousActorId{},previousWidgetId{},previousParentId{};
    bool previousRecorded=false,currentNativeSource=false,transitionAllowed=false;
    bool spectatorRefreshed=false;uint64_t spectatorSampleBefore=0,spectatorSampleAfter=0;
    Weak spectatorTargetBefore{},spectatorTargetAfter{};
    const char* transitionStage="not_requested";
};
struct State {
    bool initialized=false,retired=false,ready=false,restartRequired=false,profilesVerified=false;
    bool localPairVerified=false,nativeClient=false,viewportOwnerVerified=false;
    bool settingsReady=false,bindingsReady=false,nativeInputReady=false;
    bool hudAttempted=false,hudReady=false,hudUnique=false,firstPersonWidgetOwned=false;
    bool cameraBound=false,cameraReady=false,adsObserved=false,menuRoutingReady=false;
    bool audioStarted=false,audioObserved=false,audioOwnListenerVerified=false,audioWaiting=true;
    bool streakStarted=false,streakObserved=false,topbarStarted=false,topbarExpected=false,topbarReady=false;
    bool ownershipAuditComplete=false,noForeignUI=true,remoteRolesVerified=false;
    bool ownUIManagerPresent=false,ownUIDirectorPresent=false,uiManagerOwnershipVerified=false,uiManagerUnique=false;
    bool gameplayBlocked=false,cursorVisible=false;
    unsigned error=0,warning=0;
    const char* stage="native_client_presentation_not_started";
    const char* operation="none";
    const char* warningStage="none";
    DWORD gameThread=0;
    uintptr_t exceptionRva=0;
    uint64_t polls=0,epochs=0,retirements=0,viewSamples=0,lastSampleMs=0,lastAuditMs=0,lastTopbarMs=0;
    int32_t netMode=-1,localPlayerCount=0;
    uint32_t ownHUDCount=0,foreignHUDCount=0,remotePawnCount=0,remoteAbilityWidgetCount=0;
    uint32_t spectatedAbilityWidgetCount=0,cachedAbilityWidgetCount=0,transitionAbilityWidgetCount=0;
    AbilityWidgetAudit abilityWidgetAudit{},failedAbilityWidgetAudit{};
    void *auditRemoteWidget=nullptr,*auditRemoteWidgetOwner=nullptr,*auditRemoteWidgetParent=nullptr;
    const char* remoteWidgetAuditStage="no_remote_ability_widget";
    uint32_t foreignControllerCount=0,foreignUIManagerCount=0,foreignUIDirectorCount=0,foreignListenerCount=0;
    uint8_t pawnRole=0,pawnRemoteRole=0,controllerRole=0;
    uint32_t hudRequiredMask=0x1ff,hudPrerequisiteMask=0;
    bool hudInstalled=false,hudPlayerModelBound=false,hudRootObjectsPresent=false;
    bool hudFirstPersonPSBound=false,hudFirstPersonCharacterBound=false,hudModelGSBound=false,hudModelPCBound=false;
    bool cameraOwnerMatches=false,cameraViewTargetMatches=false,cameraLocallyControlled=false;
    unsigned hudError=0;
    const char* hudStage="original_hud_not_sampled";
    const char* cameraStage="native_camera_not_sampled";
    const char* lastFaultOperation="none";
    uint64_t lastFaultDelta=0,faultsBeforeInitialize=0,faultsObserved=0;
    bool streakMetadataBound=false;
    uint32_t streakMetadataGeneration=0,streakMetadataRefreshes=0,streakMetadataExpirations=0;
    uint64_t streakRetiredSamples=0,streakInnerExceptionRva=0;
    const char* streakExpiredMetadataRole="none";
    uintptr_t imageBase=0,moduleBase=0;
    int32_t hudElementCount=0;
    uint8_t viewTargetMode=0xff,viewTargetDefaultMode=0xff,viewTargetReady=0,viewTargetAllowedModes=0;
    void *world=nullptr,*controller=nullptr,*playerState=nullptr,*pawn=nullptr,*localPlayer=nullptr;
    void *gameInstance=nullptr,*driver=nullptr,*connection=nullptr,*viewportClient=nullptr,*sceneViewport=nullptr;
    void *uiManager=nullptr,*uiDirector=nullptr,*hud=nullptr,*observedCamera=nullptr;
    void *psSpawnedCharacter=nullptr,*psPossessedCharacter=nullptr,*hudModel=nullptr,*hudFirstPersonModel=nullptr;
    void *hudModelController=nullptr,*hudModelGameState=nullptr,*hudFirstPersonState=nullptr,*hudFirstPersonCharacter=nullptr;
    void *hudFirstPersonWidget=nullptr,*hudMinimapWidget=nullptr,*hudAbilityWidget=nullptr,*hudAbilityParent=nullptr;
    void *cameraOwner=nullptr,*cameraViewTarget=nullptr,*cameraPendingTarget=nullptr,*viewTargetComponent=nullptr;
    void *viewTargetInfoActor=nullptr,*viewTargetLocalActor=nullptr,*pcInputComponent=nullptr,*pawnInputComponent=nullptr,*playerInput=nullptr;
    std::array<ProbeObservation,size_t(Probe::Count)> probes{};
    Weak worldId{},controllerId{},playerStateId{},pawnId{},localPlayerId{};
    WeaponSnapshot weapon{};
};
inline State state{};
inline Shop shop{};
inline SpectatedAbilityRecord spectatedAbilityRecord{};
struct Methods {
    void *serverPurchase=nullptr,*toggleShop=nullptr,*setShopScreen=nullptr,*setScreenCoordinator=nullptr;
    void *setUIModel=nullptr,*shopPlayerStateChanged=nullptr,*setUICoordinator=nullptr;
    void *owningPlayer=nullptr,*actors=nullptr,*pitchRecoil=nullptr,*yawRecoil=nullptr,*firingError=nullptr,*ammo=nullptr;
    void* canOpenShop=nullptr;
};
inline Methods methods{};
struct StreakMetadataEntry {void* object=nullptr;Weak identity{};};
inline constexpr const char* StreakMetadataRoles[]{"effect_blueprint_class","component_blueprint_class","widget_blueprint_class",
    "gameplay_library_cdo","timer_library_cdo","text_library_cdo","enumerate_effects_function","widget_owner_function",
    "widget_in_viewport_function","widget_visible_function","widget_visibility_function","component_tick_function",
    "timer_remaining_function","text_get_function","text_to_string_function","panel_child_count_function","panel_child_at_function","widget_parent_function"};
inline constexpr size_t StreakMetadataCount=sizeof(StreakMetadataRoles)/sizeof(StreakMetadataRoles[0]);
inline std::array<StreakMetadataEntry,StreakMetadataCount> streakMetadata{};
struct ProbeScope {
    Probe probe;LONG64 before=0;const char* previous=nullptr;
    explicit ProbeScope(Probe value):probe(value),before(original_faults::Read64(&original_faults::observedCount)),previous(state.operation){
        state.operation=ProbeNames[size_t(probe)];
    }
    ~ProbeScope(){
        const LONG64 after=original_faults::Read64(&original_faults::observedCount);
        const uint64_t delta=after>=before?uint64_t(after-before):0;
        auto& row=state.probes[size_t(probe)];++row.calls;row.lastFaultDelta=delta;row.faultDelta+=delta;
        if(delta){state.lastFaultOperation=ProbeNames[size_t(probe)];state.lastFaultDelta=delta;}
        state.operation=previous;
    }
};
inline const State& GetState(){return state;}
inline const Shop& GetShopState(){return shop;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool IsA(void* p,uintptr_t getter){return original_players::IsA(p,getter);}
inline bool Fail(unsigned code,const char* reason){state.ready=false;state.error=code;state.stage=reason;return false;}
inline bool Warn(unsigned code,const char* reason){state.warning=code;state.warningStage=reason;return true;}
inline bool Waiting(const char* reason){state.ready=false;state.stage=reason;return true;}
inline bool Thread(){return engine::image&&state.gameThread==GetCurrentThreadId()
    &&original_players::gameThread==GetCurrentThreadId()&&!state.restartRequired;}
inline bool Live(void* object,const Weak& id){return object&&original_inventory_slots::ResolveIdentity(id)==object;}
inline bool Identity(void* object,Weak& id){return original_inventory_slots::CaptureIdentity(object,id)&&Live(object,id);}
inline bool ValidArray(const Array& a,int32_t maximum){return a.count>=0&&a.count<=maximum&&a.capacity>=a.count
    &&a.capacity<=4096&&(!a.count||(a.data&&original_players::Readable(a.data,size_t(a.count)*8)));}
inline void* Function(const wchar_t* name,uint16_t bytes){auto f=engine::Find(name);return f&&Read<uint16_t>(f,0xbe)==bytes?f:nullptr;}
inline bool Event(void* object,void* f,void* parameters){auto process=Read<void*>(Read<void*>(object),0x298);
    if(!object||!f||!process)return false;reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,f,parameters);return true;}
inline bool Event(void* object,const wchar_t* name,uint16_t bytes,void* parameters){return Event(object,Function(name,bytes),parameters);}
inline int32_t Field(void* type,const wchar_t* name,int32_t bytes,uint64_t* propertyFlags=nullptr){
    Name wanted{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&wanted,name,1);
    auto f=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&wanted,3);
    const auto flags=Read<uint64_t>(Read<void*>(f,8),0x18);const int32_t offset=Read<int32_t>(f,0x44),size=Read<int32_t>(type,0x60);
    if(!f||(flags&0x8001)!=0x8001||Read<uint64_t>(f,0x20)!=Read<uint64_t>(&wanted)
       ||Read<int32_t>(f,0x30)!=1||Read<int32_t>(f,0x34)!=bytes||offset<0||size<bytes||offset>size-bytes)return -1;
    if(propertyFlags)*propertyFlags=Read<uint64_t>(f,0x38);return offset;
}
inline bool VerifyProfiles(){
    for(const auto& p:Profiles){const auto* bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes))return false;uint64_t hash=14695981039346656037ULL;
        for(size_t i=0;i<p.bytes;++i)hash=(hash^bytes[i])*1099511628211ULL;
        if(hash!=p.fnv)return false;}
    return true;
}
inline bool AppendLocalCache(void* object,size_t offset,void* value){
    const auto before=Read<Array>(object,offset);
    // Only a newly empty native local resource/UI cache is adapted here.
    if(!value||before.count!=0||before.capacity<0||before.capacity>4096)return false;
    if(!original_players::Write(object,offset+8,int32_t(1)))return false;
    if(before.capacity<1)engine::function<void(*)(void*,int32_t)>(0x18bd090)(static_cast<uint8_t*>(object)+offset,0);
    auto data=Read<void**>(object,offset);
    return data&&original_players::Write(data,0,value)&&Read<void*>(data)==value;
}
inline bool WidgetOwned(void* widget){void* owner=nullptr;return widget&&methods.owningPlayer
    &&Event(widget,methods.owningPlayer,&owner)&&owner==state.controller;}

inline bool ValidateLocalPair(bool requirePawn=true){
    state.localPairVerified=false;state.viewportOwnerVerified=false;state.nativeClient=false;
    if(!Live(state.world,state.worldId)||!Live(state.controller,state.controllerId)
       ||!Live(state.playerState,state.playerStateId)||!Live(state.localPlayer,state.localPlayerId))
        return Fail(4501,"native_client_presentation_local_identity_expired");
    state.gameInstance=Read<void*>(state.world,0x1d8);state.driver=Read<void*>(state.world,0x40);
    state.connection=Read<void*>(state.driver,0xc0);state.localPlayerCount=Read<int32_t>(state.gameInstance,0x48);
    const auto locals=Read<Array>(state.gameInstance,0x40);
    state.controllerRole=Read<uint8_t>(state.controller,0x224);
    state.netMode=engine::function<int32_t(*)(void*)>(0x3efc820)(state.controller);
    state.nativeClient=state.netMode==3&&state.controllerRole==2&&state.driver&&state.connection
        &&Read<void*>(state.connection,0x38)==state.controller&&Read<void*>(state.connection,0xa0)==state.controller
        &&!Read<void*>(state.world,0x170);
    state.localPairVerified=state.nativeClient&&ValidArray(locals,32)&&locals.count==1
        &&Read<void*>(locals.data)==state.localPlayer&&IsA(state.localPlayer,0x6cb4740)
        &&Read<void*>(state.localPlayer,0x38)==state.controller&&Read<void*>(state.controller,0x510)==state.localPlayer
        &&Read<void*>(state.controller,0x470)==state.playerState;
    if(!state.localPairVerified)return Fail(4502,"native_client_presentation_requires_sole_actual_connection_local_player");
    // Exact retained native4921f21/4941082 ownership path, not an inferred
    // simulation-local predicate: actual LocalPlayer -> ViewportClient+80.
    state.viewportClient=Read<void*>(state.localPlayer,0x80);state.sceneViewport=Read<void*>(state.viewportClient,0xf0);
    auto viewportType=engine::Find(L"/Script/ShooterGame.AresGameViewportClient");
    state.viewportOwnerVerified=viewportType&&original_players::Derived(Read<void*>(state.viewportClient,0x10),viewportType)
        &&state.sceneViewport&&Read<void*>(state.controller,0x510)==state.localPlayer;
    if(!state.viewportOwnerVerified)return Fail(4503,"native_client_presentation_original_own_viewport_required");
    if(!requirePawn)return true;
    if(!Live(state.pawn,state.pawnId))return Fail(4504,"native_client_presentation_pawn_identity_expired_without_retire");
    state.pawnRole=Read<uint8_t>(state.pawn,0x224);state.pawnRemoteRole=Read<uint8_t>(state.pawn,0x128);
    return state.pawnRole==2&&Read<void*>(state.controller,0x4b0)==state.pawn
        &&Read<void*>(state.pawn,0x4a0)==state.controller&&Read<void*>(state.pawn,0x488)==state.playerState
        ?true:Fail(4505,"native_client_presentation_autonomous_possession_mismatch");
}
inline bool ResolveMethods(){
    ProbeScope probe(Probe::Initialization);
    // Register the original local shop types/CDOs before resolving their native
    // UFunctions. This constructs no viewport widget or gameplay actor.
    engine::DefaultObject(engine::Class(0x6cd95b0));engine::DefaultObject(engine::Class(0x6cd9450));
    methods.owningPlayer=Function(L"/Script/UMG.Widget.GetOwningPlayer",8);
    methods.actors=Function(L"/Script/Engine.GameplayStatics.GetAllActorsOfClass",32);
    methods.serverPurchase=Function(L"/Script/ShooterGame.ShooterPlayerController.ServerPurchase",10);
    methods.toggleShop=Function(L"/Script/ShooterGame.AresPlayerController.ToggleShop",4);
    methods.setUICoordinator=Function(L"/Script/ShooterGame.AresPlayerController.SetUICoordinator",8);
    methods.setShopScreen=Function(L"/Script/ShooterGame.ShopScreenCoordinator.SetShopScreen",8);
    methods.setScreenCoordinator=Function(L"/Script/ShooterGame.ShopScreen.SetShopScreenCoordinator",8);
    methods.setUIModel=Function(L"/Script/ShooterGame.ShopScreen.SetCoordinatedShooterUIModel",8);
    methods.shopPlayerStateChanged=Function(L"/Script/ShooterGame.ShopScreenCoordinator.HandleLocalPlayerStateChanged",8);
    methods.canOpenShop=Function(L"/Script/ShooterGame.OwnerExclusivePlayerInfo.CanOpenShop",1);
    shop.canOpenABIValid=methods.canOpenShop&&Read<void*>(methods.canOpenShop,0x28)==engine::Class(0x64f4540)
        &&Read<void*>(methods.canOpenShop,0xe0)==reinterpret_cast<void*>(engine::image+0x64a6650);
    methods.pitchRecoil=Function(L"/Script/ShooterGame.StabilityComponent.GetPitchRecoil",4);
    methods.yawRecoil=Function(L"/Script/ShooterGame.StabilityComponent.GetYawRecoil",4);
    methods.firingError=Function(L"/Script/ShooterGame.StabilityComponent.GetFiringError",4);
    methods.ammo=Function(L"/Script/ShooterGame.AmmoComponent.GetAmmo",4);
    const auto flags=Read<uint32_t>(methods.serverPurchase,0xb8);
    shop.nativePurchaseRPCVerified=methods.serverPurchase&&(flags&0x200040U)==0x200040U;
    return methods.owningPlayer&&methods.actors&&shop.nativePurchaseRPCVerified&&methods.toggleShop
        &&methods.setShopScreen&&methods.setScreenCoordinator&&methods.setUIModel&&methods.shopPlayerStateChanged;
}
inline bool EnsureStoreCache(){
    auto gs=Read<void*>(state.world,0x178);if(!IsA(gs,original_players::ShooterGameStateClass))return true;
    shop.gameState=gs;
    uint64_t flags=0;
    if(Field(Read<void*>(gs,0x10),L"CachedStores",16,&flags)!=0xaf8||(flags&0x20)!=0||(flags&0x2000)==0)
        return Fail(4507,"native_client_shop_cache_must_be_original_transient_nonreplicated_property");
    engine::function<void*(*)(void*)>(0x718e090)(gs);
    auto stores=Read<Array>(gs,0xaf8);
    if(!ValidArray(stores,16))return Fail(4508,"native_client_shop_original_store_cache_invalid");
    if(!stores.count){
        auto type=original_hud::LoadClass(0x6541090,L"/Game/GameModes/Deathmatch/DeathmatchGameStore.DeathmatchGameStore_C");
        auto store=type?original_hud::Construct(type,gs):nullptr;
        if(!IsA(store,0x6541090)||Read<int32_t>(store,0x40)<1||!AppendLocalCache(gs,0xaf8,store))
            return Fail(4509,"native_client_shop_original_resource_cache_creation_failed");
        shop.cacheSupplied=true;stores=Read<Array>(gs,0xaf8);
    }
    shop.storeCount=stores.count;shop.store=Read<void*>(stores.data);
    if(!IsA(shop.store,0x6541090))return Fail(4510,"native_client_shop_cached_store_type_mismatch");
    return true;
}
inline bool ReadyForHUD(){
    const auto& h=original_hud::State();state.settingsReady=h.settingsReady;
    auto inventory=Read<void*>(state.pawn,0xc08),asc=Read<void*>(state.pawn,0xa20),gs=Read<void*>(state.world,0x178);
    state.hudPrerequisiteMask=(state.settingsReady?1U:0U)|(h.uiServiceObjectsCreated?2U:0U)|(h.clientLinked?4U:0U)
        |(h.inGameOnlyShellSelected?8U:0U)|(Read<uint8_t>(state.pawn,0xa90)?16U:0U)|(IsA(inventory,0x6499330)?32U:0U)
        |(Read<void*>(inventory,0xc8)==state.pawn?64U:0U)|(IsA(asc,0x63d6b50)?128U:0U)|(IsA(gs,original_players::ShooterGameStateClass)?256U:0U);
    return state.hudPrerequisiteMask==state.hudRequiredMask;
}
inline void CaptureReadiness(){
    state.faultsObserved=uint64_t(original_faults::Read64(&original_faults::observedCount));
    const auto& h=original_hud::State();
    state.psSpawnedCharacter=Read<void*>(state.playerState,0xbc8);state.psPossessedCharacter=Read<void*>(state.playerState,0xbd0);
    state.hudInstalled=h.originalHUDInstalled;state.hudPlayerModelBound=h.playerModelBound;state.hudRootObjectsPresent=h.originalRootObjectsPresent;
    state.hudError=original_hud::error;state.hudStage=original_hud::stage;state.hudElementCount=h.hudElementCount;
    state.hudFirstPersonPSBound=h.firstPersonPlayerStateBound;state.hudFirstPersonCharacterBound=h.firstPersonCharacterBound;
    state.hudModelGSBound=h.modelGameStateBound;state.hudModelPCBound=h.modelControllerBound;
    state.hudModel=h.hudModel;state.hudFirstPersonModel=h.firstPersonModel;state.hudModelController=h.modelController;
    state.hudModelGameState=h.modelGameState;state.hudFirstPersonState=h.firstPersonPlayerState;state.hudFirstPersonCharacter=h.firstPersonSpawnedCharacter;
    state.hudFirstPersonWidget=h.firstPersonWidget;state.hudMinimapWidget=h.minimapWidget;
    state.hudAbilityWidget=h.abilityWidget;state.hudAbilityParent=h.abilityParent;
    const auto& c=original_camera::State();state.cameraStage=c.stage;state.cameraOwnerMatches=c.ownerMatches;
    state.cameraViewTargetMatches=c.viewTargetMatches;state.cameraLocallyControlled=c.locallyControlled;
    auto camera=Read<void*>(state.controller,0x528);state.cameraOwner=Read<void*>(camera,0x468);
    state.cameraViewTarget=Read<void*>(camera,0x580);state.cameraPendingTarget=Read<void*>(camera,0xe80);
    state.viewTargetComponent=Read<void*>(state.controller,0xc60);auto target=state.viewTargetComponent;
    state.viewTargetInfoActor=Read<void*>(target,0x170);state.viewTargetLocalActor=Read<void*>(target,0x180);
    state.viewTargetMode=Read<uint8_t>(target,0x178);state.viewTargetDefaultMode=Read<uint8_t>(target,0x16b);
    state.viewTargetReady=Read<uint8_t>(target,0x169);state.viewTargetAllowedModes=Read<uint8_t>(target,0x16a);
    state.playerInput=Read<void*>(state.controller,0x5e8);state.pcInputComponent=Read<void*>(state.controller,0x230);
    state.pawnInputComponent=Read<void*>(state.pawn,0x230);
}
inline bool BindHUD(){
    // Native observation changes only the expected viewed models; the local owner remains the actual connection PC.
    auto viewedPawn=local_server_spectator_view::ExpectedCharacter();
    auto viewedPS=local_server_spectator_view::ExpectedPlayerState();
    if(!viewedPawn||!viewedPS){viewedPawn=state.pawn;viewedPS=state.playerState;}
    if(!ReadyForHUD())return Waiting("native_client_presentation_waiting_replicated_character_and_original_services");
    if(!EnsureStoreCache())return false;
    auto hud=Read<void*>(state.controller,0x520);bool ok=false;
    if(!state.hudAttempted){
        state.hudAttempted=true;state.operation="adopt_or_construct_original_own_hud";
        if(IsA(hud,0x6cae7d0)){
            if(Read<void*>(hud,0x468)!=state.controller)return Fail(4511,"native_client_existing_hud_has_foreign_owner");
            // These are adapter observations only. Adopt an engine-created HUD
            // instead of invoking ClientSetHUD a second time on the same PC.
            auto& h=original_hud::diagnostics;h.originalHUDInstalled=true;h.readyForHUD=true;
            h.character=viewedPawn;h.playerState=viewedPS;h.inventory=Read<void*>(viewedPawn,0xc08);
            original_hud::controller=state.controller;ok=original_hud::InspectHUD();
        }else ok=original_hud::InstallOriginalHUD(state.controller,state.pawn);
    }else{
        auto& h=original_hud::diagnostics;h.character=viewedPawn;h.playerState=viewedPS;h.inventory=Read<void*>(viewedPawn,0xc08);
        ok=original_hud::InspectHUD();
    }
    state.hud=Read<void*>(state.controller,0x520);state.operation="none";
    if(original_hud::restartRequired)return Fail(original_hud::error?original_hud::error:4512,"native_client_original_hud_exception_requires_new_session");
    if(!IsA(state.hud,0x6cae7d0))return Fail(4513,"native_client_original_hud_factory_did_not_publish_hud");
    if(!ok){state.hudReady=false;return Warn(original_hud::error,original_hud::stage);}
    const auto& h=original_hud::State();state.firstPersonWidgetOwned=WidgetOwned(h.firstPersonWidget);
    state.hudReady=h.originalHUDInstalled&&h.playerModelBound&&h.originalRootObjectsPresent
        &&h.firstPersonViewportQueried&&h.firstPersonInViewport&&state.firstPersonWidgetOwned;
    return state.hudReady?true:Warn(0,"native_client_original_hud_waiting_replica_models_and_roots");
}
inline void BindShopMenuDelegate(void* entry,size_t offset,const wchar_t* function){
    Name name{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&name,function,1);
    engine::function<void(*)(void*,void*,void*,const Name*)>(0x5ef6250)
        (static_cast<uint8_t*>(entry)+offset,shop.coordinator,nullptr,&name);
}
inline bool EnsureShopScreen(){
    auto entries=Read<Array>(shop.coordinator,0x88);if(!ValidArray(entries,16))return Fail(4514,"native_client_shop_entry_cache_invalid");
    if(!entries.count){
        auto type=original_hud::LoadClass(0x6cd9450,L"/Game/UI/InGame/Shop/Shop_V2/WBP_Screen_PreRoundShopContainer.WBP_Screen_PreRoundShopContainer_C");
        Name none{};auto screen=type?engine::function<void*(*)(void*,void*,const Name*)>(0x3c514e0)(state.controller,type,&none):nullptr;
        if(!IsA(screen,0x6cd9450)||!WidgetOwned(screen))return Fail(4515,"native_client_shop_original_own_screen_factory_failed");
        auto model=shop.uiModel,coordinator=shop.coordinator;
        if(!Event(screen,methods.setUIModel,&model)||!Event(screen,methods.setScreenCoordinator,&coordinator)
           ||!Event(coordinator,methods.setShopScreen,&screen))return Fail(4516,"native_client_shop_screen_model_binding_failed");
        Array empty{};
        auto entry=engine::function<void*(*)(void*,void*,void*,void*,bool,bool,bool,bool,void*)>(0x7ab5f30)
            (shop.menuStack,screen,nullptr,nullptr,true,true,false,false,&empty);
        if(!IsA(entry,0x699c5b0)||!AppendLocalCache(shop.coordinator,0x88,entry))
            return Fail(4517,"native_client_shop_menu_entry_factory_failed");
        BindShopMenuDelegate(entry,0x48,L"OnOpenedShopScreen");BindShopMenuDelegate(entry,0x58,L"OnClosedShopScreen");
        shop.screenSupplied=true;entries=Read<Array>(shop.coordinator,0x88);
    }
    shop.entryCount=entries.count;shop.entry=Read<void*>(entries.data);shop.screen=Read<void*>(shop.entry,0x30);
    shop.ownerVerified=IsA(shop.entry,0x699c5b0)&&IsA(shop.screen,0x6cd9450)&&WidgetOwned(shop.screen);
    return shop.ownerVerified?true:Fail(4518,"native_client_shop_screen_owner_readback_failed");
}
inline bool ObserveShopOpen(){
    shop.openKnown=false;const auto opened=Read<Array>(shop.menuStack,0xb0);
    if(!shop.entry||!ValidArray(opened,128))return false;
    shop.open=false;for(int32_t i=0;i<opened.count;++i)if(opened.data[i]==shop.entry){shop.open=true;break;}
    shop.openKnown=true;return true;
}
inline void ObserveShopOpenPath(){
    // Current-state evidence only. Do not invoke preparation, model creation,
    // menu activation, a coordinator tick or a GameFlow transition to observe it.
    auto& x=shop.path;const auto now=GetTickCount64();
    if(x.lastSampleMs&&now-x.lastSampleMs<100)return;
    const auto samples=x.samples,lookups=x.nativeLookupTotal;x={};
    x.samples=samples+1;x.nativeLookupTotal=lookups;x.lastSampleMs=now;
    x.stage="native_shop_readonly_coordinator_identity_unavailable";
    if(!Thread()||!state.localPairVerified||!Identity(shop.coordinator,x.coordinatorId)
       ||!IsA(shop.coordinator,0x6cd95b0))return;
    x.coordinatorLive=true;x.controller=Read<void*>(shop.coordinator,0x80);
    x.controllerMatches=x.controller==state.controller&&Live(state.controller,state.controllerId);
    if(!x.controllerMatches){x.stage="native_shop_readonly_coordinator_controller_mismatch";return;}
    const auto getWorld=Read<uintptr_t>(Read<void*>(x.controller),0x1b8);
    x.getWorldRva=getWorld>=engine::image?getWorld-engine::image:getWorld;
    x.getWorldProfiled=getWorld==engine::image+0x3ed83b0;
    if(!x.getWorldProfiled){x.stage="native_shop_readonly_getworld_target_not_profiled";return;}
    x.worldQueried=true;x.world=engine::function<void*(*)(void*)>(0x3ed83b0)(x.controller);
    x.worldLive=Identity(x.world,x.worldId);x.worldMatches=x.worldLive&&x.world==state.world;
    x.playerStateQueried=true;x.playerState=engine::function<void*(*)(void*)>(0x78b1f80)(x.controller);
    x.playerStateMatches=Identity(x.playerState,x.playerStateId)&&x.playerState==state.playerState;
    if(x.worldLive){
        x.gameState=Read<void*>(x.world,0x178);x.gameStateLive=Identity(x.gameState,x.gameStateId);
        x.gameStateShooter=x.gameStateLive&&IsA(x.gameState,original_players::ShooterGameStateClass);
        x.worldGameInstance=Read<void*>(x.world,0x1d8);Weak giId{};
        // Exact original703F040 branches: real ClientGI, or AresGI's actual
        // ClientGI sidecar. Observe existing objects; never construct a flow.
        if(Identity(x.worldGameInstance,giId)){
            if(IsA(x.worldGameInstance,0x6429080))x.clientGameInstance=x.worldGameInstance;
            else if(IsA(x.worldGameInstance,0x647e8a0))x.clientGameInstance=Read<void*>(x.worldGameInstance,0x2b8);
        }
        x.clientGameInstanceLive=Identity(x.clientGameInstance,x.clientGameInstanceId)
            &&IsA(x.clientGameInstance,0x6429080);
        if(x.clientGameInstanceLive){
            x.flowManager=Read<void*>(x.clientGameInstance,0x3c0);
            x.flowManagerLive=Identity(x.flowManager,x.flowManagerId)&&IsA(x.flowManager,0x6881a80);
            if(x.flowManagerLive){
                x.flowState=Read<void*>(x.flowManager,0xb0);
                x.flowStateLive=Identity(x.flowState,x.flowStateId)&&IsA(x.flowState,0x68819d0);
                if(x.flowStateLive)x.flowStateType=Read<uint8_t>(x.flowState,0x80);
            }
        }
    }
    const auto entries=Read<Array>(shop.coordinator,0x88);x.entryCount=entries.count;
    x.entryArrayValid=ValidArray(entries,16);
    if(x.entryArrayValid&&entries.count>0){
        x.entry=Read<void*>(entries.data);x.entryLive=Identity(x.entry,x.entryId)&&IsA(x.entry,0x699c5b0);
        if(x.entryLive){
            auto candidate=Read<void*>(x.entry,0x30);Weak candidateId{};
            // The original getter reads the candidate's flags before class
            // validation. Keep its observer call behind actual native weak validity.
            if(!candidate||Identity(candidate,candidateId)){
                x.screenFlags=candidate?Read<uint32_t>(candidate,8):0;x.screenQueried=true;
                x.screen=engine::function<void*(*)(void*,int32_t)>(0x7a8d5c0)(shop.coordinator,0);
                x.screenLive=Identity(x.screen,x.screenId);x.screenMatches=x.screenLive&&x.screen==shop.screen;
            }
        }
    }
    x.menuStack=Read<void*>(shop.coordinator,0x78);
    x.stackLive=Identity(x.menuStack,x.stackId)&&IsA(x.menuStack,0x699c660);
    if(x.stackLive){const auto opened=Read<Array>(x.menuStack,0xb0);x.openEntryCount=opened.count;
        x.stackArrayValid=ValidArray(opened,128);
        if(x.stackArrayValid)for(int32_t i=0;i<opened.count;++i)
            if(Read<void*>(opened.data,size_t(i)*8)==x.entry&&x.entry)x.entryInOpenStack=true;
    }
    x.gameInfoModel=Read<void*>(shop.coordinator,0xc0);x.playerModel=Read<void*>(shop.coordinator,0xb8);
    x.flowInGame=x.flowStateLive&&x.flowStateType==6;
    x.originalTickInputsObserved=x.flowInGame&&x.screenLive;
    if(x.gameStateShooter){
        x.characterRuleAllowed=Read<uint8_t>(x.gameState,0x6f2)!=0;
        x.characterShopAllowed=Read<uint8_t>(x.gameState,0x6f3)!=0;
        x.characterRuleRequired=x.characterRuleAllowed&&x.characterShopAllowed;
    }
    if(x.playerStateMatches){
        x.spawnedCharacter=Read<void*>(x.playerState,0xbc8);x.spawnedLive=Identity(x.spawnedCharacter,x.spawnedId)
            &&IsA(x.spawnedCharacter,original_players::ShooterCharacterClass);
        x.spawnedMatches=x.spawnedLive&&x.spawnedCharacter==state.pawn;
    }
    if(x.spawnedLive){
        x.inventory=Read<void*>(x.spawnedCharacter,0xc08);x.inventoryLive=Identity(x.inventory,x.inventoryId)
            &&IsA(x.inventory,0x6499330);
        x.inventoryOwned=x.inventoryLive&&Read<void*>(x.inventory,0xc8)==x.spawnedCharacter
            &&Read<void*>(x.inventory,0x2f0)==x.spawnedCharacter;
        x.slotsLive=x.inventoryOwned;
        if(x.slotsLive)for(size_t i=0;i<17;++i){auto slot=Read<void*>(x.inventory,0x1b8+i*8);Weak slotId{};
            if(slot&&(!Identity(slot,slotId)||!IsA(slot,0x68f63c0))){x.slotsLive=false;break;}}
        const auto classes=Read<Array>(x.spawnedCharacter,0xbd8);x.startingClassCount=classes.count;
        x.startingArrayValid=ValidArray(classes,int32_t(ShopStartingClassLimit));
        if(x.startingArrayValid){
            x.observedStartingClasses=classes.count;
            for(int32_t i=0;i<classes.count;++i){auto& row=x.startingClasses[size_t(i)];
                row.type=Read<void*>(classes.data,size_t(i)*8);row.typeLive=Identity(row.type,row.typeId);
                // BD8 holds UClass objects, not Actor instances. Keep weak
                // liveness, metaclass identity and represented inheritance distinct.
                row.classObject=row.typeLive&&IsA(row.type,0x1af0eb0);
                row.classEquippable=row.classObject&&original_players::Derived(row.type,engine::Class(0x645c660));
                if(!row.classEquippable){++x.invalidStartingClasses;continue;}
                row.defaults=Read<void*>(row.type,0x120);row.defaultsDeferred=!row.defaults;
                if(row.defaultsDeferred){++x.deferredDefaults;continue;}
                row.defaultsLive=Identity(row.defaults,row.defaultsId)&&IsA(row.defaults,0x645c660);
                if(!row.defaultsLive){++x.invalidStartingClasses;continue;}
                row.defaultSlot=Read<uint8_t>(row.defaults,0x4a0);
                // Original70F82B0 creates a missing CDO via UClass.v420. The
                // existing-CDO gate above is mandatory for this read-only use.
                if(!x.slotsLive)continue;
                row.queried=true;++x.nativeLookupCalls;++x.nativeLookupTotal;
                row.item=engine::function<void*(*)(void*,void*,uint8_t)>(0x70f82b0)(x.inventory,row.type,0xfd);
                row.itemLive=Identity(row.item,row.itemId);row.itemEquippable=row.itemLive&&IsA(row.item,0x645c660);
                row.itemOwned=row.itemEquippable&&Read<void*>(row.item,0x210)==x.spawnedCharacter
                    &&Read<void*>(row.item,0x4a8)==x.spawnedCharacter;
                if(!row.itemEquippable)++x.missingItems;
            }
            x.startingClassesComplete=x.slotsLive&&x.nativeLookupCalls==classes.count
                &&x.deferredDefaults==0&&x.invalidStartingClasses==0&&x.missingItems==0;
        }
    }
    if(!x.worldMatches)x.stage="native_shop_readonly_original_controller_world_mismatch";
    else if(!x.playerStateMatches)x.stage="native_shop_readonly_original_playerstate_unavailable";
    else if(!x.gameStateShooter)x.stage="native_shop_readonly_original_shooter_gamestate_unavailable";
    else if(!x.screenLive)x.stage="native_shop_readonly_original_screen_unavailable";
    else if(!shop.canOpenQueried||!shop.nativeCanOpen)x.stage="native_shop_readonly_original_owner_eligibility_false";
    else if(x.characterRuleRequired&&!x.spawnedLive)x.stage="native_shop_readonly_required_spawned_character_unavailable";
    else if(x.characterRuleRequired&&!x.inventoryOwned)x.stage="native_shop_readonly_required_inventory_unavailable";
    else if(x.characterRuleRequired&&!x.startingArrayValid)x.stage="native_shop_readonly_required_starting_classes_invalid";
    else if(x.characterRuleRequired&&x.deferredDefaults)x.stage="native_shop_readonly_required_defaults_deferred";
    else if(x.characterRuleRequired&&!x.startingClassesComplete)x.stage="native_shop_readonly_required_equippables_unresolved";
    else if(x.entryInOpenStack)x.stage="native_shop_readonly_entry_in_original_open_stack";
    else x.stage="native_shop_readonly_open_inputs_observed_stack_open_unobserved";
}
inline void ObserveShopEligibility(){
    shop.canOpenQueried=false;shop.nativeCanOpen=false;
    if(shop.canOpenABIValid&&shop.playerInfo){uint8_t canOpen=0;
        shop.canOpenQueried=Event(shop.playerInfo,methods.canOpenShop,&canOpen);shop.nativeCanOpen=shop.canOpenQueried&&canOpen!=0;}
    shop.coordinatorCanOpen=Read<uint8_t>(shop.coordinator,0xc8)!=0;shop.screenNativeOpen=Read<uint8_t>(shop.screen,0x31c)!=0;
    const auto groups=Read<Array>(shop.coordinator,0x98);
    shop.modelGroupCount=groups.count;shop.observedModelCount=0;shop.storePurchasableCount=Read<int32_t>(shop.store,0x40);
    ObserveShopOpenPath();
    if(groups.count<0||groups.count>16||groups.capacity<groups.count||groups.capacity>4096
       ||(groups.count&&(!groups.data||!original_players::Readable(groups.data,size_t(groups.count)*16))))return;
    for(int32_t group=0;group<groups.count;++group){const auto models=Read<Array>(groups.data,size_t(group)*16);
        if(!ValidArray(models,256)){shop.observedModelCount=-1;return;}shop.observedModelCount+=models.count;}
}
inline bool PrepareShop(){
    if(!state.hudReady)return true;
    shop.playerInfo=Read<void*>(state.playerState,0xa18);shop.moneyComponent=Read<void*>(state.playerState,0x8a0);
    shop.purchaseComponent=Read<void*>(shop.playerInfo,0x550);shop.shoppingEnabled=Read<uint8_t>(shop.gameState,0xa08)!=0;
    shop.money=Read<int32_t>(shop.moneyComponent,0xf8);
    if(!IsA(shop.playerInfo,0x64f4540)||Read<void*>(shop.playerInfo,0x4e8)!=state.controller
       ||!IsA(shop.purchaseComponent,0x6a78fe0)||!IsA(shop.moneyComponent,0x69cd280)||!shop.shoppingEnabled){
        shop.stage="native_client_shop_waiting_replicated_own_player_info_money_and_shopping_rule";return true;
    }
    auto previousUI=shop.ui,previousCoordinator=shop.coordinator;
    shop.ui=Read<void*>(state.hud,0x760);shop.coordinator=Read<void*>(shop.ui,0x68);
    shop.uiModel=Read<void*>(shop.ui,0x80);shop.menuStack=Read<void*>(shop.ui,0x50);
    if(!IsA(shop.ui,0x6cd6490)||!IsA(shop.uiModel,0x6cd6540)||!IsA(shop.menuStack,0x699c660))
        return Warn(0,"native_client_shop_waiting_original_coordinator_graph");
    if(Read<void*>(state.controller,0xc30)!=shop.ui){
        if(Read<void*>(state.controller,0xc30)||Read<void*>(shop.ui,0xa0)!=state.controller||!methods.setUICoordinator)
            return Fail(4519,"native_client_shop_controller_coordinator_owner_conflict");
        auto ui=shop.ui;if(!Event(state.controller,methods.setUICoordinator,&ui)||Read<void*>(state.controller,0xc30)!=shop.ui)
            return Fail(4520,"native_client_shop_original_coordinator_setter_failed");
    }
    if(!shop.coordinator){
        shop.coordinator=engine::function<void*(*)(void*,void*,void*,void*)>(0x7a8c590)
            (shop.ui,state.controller,shop.uiModel,shop.menuStack);
        if(!IsA(shop.coordinator,0x6cd95b0)||!original_players::Write(shop.ui,0x68,shop.coordinator))
            return Fail(4521,"native_client_shop_original_coordinator_factory_failed");
        shop.coordinatorSupplied=true;
    }
    if(!IsA(shop.coordinator,0x6cd95b0)||Read<void*>(shop.coordinator,0x80)!=state.controller
       ||Read<void*>(shop.coordinator,0x78)!=shop.menuStack||Read<void*>(shop.coordinator,0x68)!=shop.uiModel)
        return Fail(4522,"native_client_shop_original_coordinator_bindings_mismatch");
    if(previousUI!=shop.ui||previousCoordinator!=shop.coordinator){
        shop.notifiedPlayerState=nullptr;shop.inventoryRefreshPending=true;shop.inventorySignature=0;
    }
    if(shop.notifiedPlayerState!=state.playerState){auto ps=state.playerState;
        if(!Event(shop.coordinator,methods.shopPlayerStateChanged,&ps))return Fail(4523,"native_client_shop_original_player_state_notification_failed");
        shop.notifiedPlayerState=state.playerState;
    }
    if(!EnsureShopScreen()||!ObserveShopOpen())return Fail(state.error?state.error:4524,"native_client_shop_original_open_state_unavailable");
    ObserveShopEligibility();
    shop.initialized=true;shop.error=0;shop.stage="native_client_original_shop_graph_ready_open_unverified_purchase_rpc_retained";return true;
}
inline uint64_t InventorySignature(void* inventory){
    uint64_t hash=14695981039346656037ULL;
    auto add=[&](uintptr_t value){hash=(hash^value)*1099511628211ULL;};
    add(reinterpret_cast<uintptr_t>(inventory));add(reinterpret_cast<uintptr_t>(Read<void*>(inventory,0x280)));
    add(reinterpret_cast<uintptr_t>(Read<void*>(inventory,0x268)));
    for(size_t i=0;i<17;++i){auto slot=Read<void*>(inventory,0x1b8+i*8);add(reinterpret_cast<uintptr_t>(slot));
        if(!slot)continue;add(reinterpret_cast<uintptr_t>(Read<void*>(slot,0x38)));
        if(IsA(slot,0x69cf3d0)){const auto contents=Read<Array>(slot,0x50);add(uintptr_t(contents.count));
            if(ValidArray(contents,64))for(int j=0;j<contents.count;++j)add(reinterpret_cast<uintptr_t>(contents.data[j]));}}
    return hash;
}
inline bool RefreshShopInventory(){
    if(!shop.initialized)return true;const bool wasOpen=shop.open;if(!ObserveShopOpen())return false;
    const auto signature=InventorySignature(Read<void*>(state.pawn,0xc08));
    if((shop.open&&!wasOpen)||signature!=shop.inventorySignature)shop.inventoryRefreshPending=true;
    if(!shop.open||!shop.inventoryRefreshPending)return true;
    const auto groups=Read<Array>(shop.coordinator,0x98);
    if(groups.count<1)return true;
    if(groups.count>16||groups.capacity<groups.count||!original_players::Readable(groups.data,size_t(groups.count)*16))
        return Fail(4525,"native_client_shop_original_model_groups_invalid");
    shop.modelCount=0;
    for(int g=0;g<groups.count;++g){const auto models=Read<Array>(groups.data,size_t(g)*16);
        if(!ValidArray(models,256))return Fail(4526,"native_client_shop_original_model_group_invalid");
        for(int i=0;i<models.count;++i){auto model=models.data[i];
            if(!IsA(model,0x6cd8d70)||Read<void*>(model,0x1c0)!=shop.playerInfo)
                return Fail(4527,"native_client_shop_model_has_foreign_player_info");
            // Re-read original owned-slot predicates after an actual replicated
            // inventory edge. This never selects or grants an item locally.
            engine::function<void(*)(void*)>(0x7a8e180)(model);++shop.refreshCalls;++shop.modelCount;}}
    shop.inventorySignature=signature;shop.inventoryRefreshPending=false;++shop.refreshPasses;return true;
}

inline void* Component(void* actor,uintptr_t getter){auto find=Read<void*>(Read<void*>(actor),0x740);
    return find?reinterpret_cast<void*(*)(void*,void*)>(find)(actor,engine::Class(getter)):nullptr;}
inline bool SampleNativeView(){
    auto camera=Read<void*>(state.controller,0x528),follow=Read<void*>(state.pawn,0x12a8);
    if(!IsA(camera,original_camera::CameraClass)||!IsA(follow,original_camera::CameraComponentClass))
        return Warn(0,"native_client_waiting_original_camera_manager_and_follow_component");
    {ProbeScope probe(Probe::Camera);
        if(!state.cameraBound||state.observedCamera!=camera){
            if(!original_camera::Initialize(state.world,state.controller,state.pawn))
                return Fail(original_camera::State().error,original_camera::State().stage);
            state.cameraBound=true;state.observedCamera=camera;
        }else if(!original_camera::Sample())return Fail(original_camera::State().error,original_camera::State().stage);
    }
    const auto& view=original_camera::State();
    const auto& spectator=local_server_spectator_view::GetState();
    state.cameraReady=view.realLocalPlayer&&view.ownerMatches&&view.localRole==2&&view.netMode==3
        &&(spectator.enabled?spectator.active:view.viewTargetMatches);
    // Initialize/Sample above are observation only. Never call AfterOriginal-
    // CharacterTick, RestoreWatchPossessedLifecycle or ReleaseBootstrapFovOverride
    // for this native client: its own scheduler already takes the Role2 branch.
    {ProbeScope probe(Probe::ADS);state.adsObserved=original_ads::Sample()&&original_ads::State().latest.localLinks;}
    {ProbeScope probe(Probe::Render);if(original_render::CaptureViewSample())++state.viewSamples;}
    ProbeScope probe(Probe::WeaponReadbacks);
    auto& w=state.weapon;w={};w.inventory=Read<void*>(state.pawn,0xc08);w.weapon=Read<void*>(w.inventory,0x280);
    if(!IsA(w.weapon,0x645c660))return true;
    w.originalOwner=Read<void*>(w.weapon,0x210)==state.pawn&&Read<void*>(w.weapon,0x4a8)==state.pawn;
    w.stability=Component(w.weapon,0x6d1e460);w.ammo=Component(w.weapon,0x63c7e00);
    if(IsA(w.stability,0x6d1e460)&&Read<void*>(w.stability,0xc8)==w.weapon){
        if(!methods.pitchRecoil)methods.pitchRecoil=Function(L"/Script/ShooterGame.StabilityComponent.GetPitchRecoil",4);
        if(!methods.yawRecoil)methods.yawRecoil=Function(L"/Script/ShooterGame.StabilityComponent.GetYawRecoil",4);
        if(!methods.firingError)methods.firingError=Function(L"/Script/ShooterGame.StabilityComponent.GetFiringError",4);
        w.recoilQueried=Event(w.stability,methods.pitchRecoil,&w.pitchRecoil)&&Event(w.stability,methods.yawRecoil,&w.yawRecoil)
            &&Event(w.stability,methods.firingError,&w.firingError);
    }
    if(IsA(w.ammo,0x63c7e00)&&Read<void*>(w.ammo,0xc8)==w.weapon){
        if(!methods.ammo)methods.ammo=Function(L"/Script/ShooterGame.AmmoComponent.GetAmmo",4);
        w.ammoQueried=Event(w.ammo,methods.ammo,&w.currentAmmo);
        w.predictedAmmo=Read<int32_t>(w.ammo,0x10c);w.authorityAmmo=Read<int32_t>(w.ammo,0x110);
    }
    return true;
}

// Reuse only the original topbar's UI factory, model binders and observations.
// Its Initialize/EnsureTeamStates/SynchronizePoints authority adapters are NEVER
// called here. Every score below is read from actual replicated native records.
inline bool PollReplicaTopbar(){
    namespace t=original_deathmatch_topbar;
    const auto now=GetTickCount64();if(state.lastTopbarMs&&now-state.lastTopbarMs<500)return true;state.lastTopbarMs=now;
    auto gs=Read<void*>(state.world,0x178),ownTeam=Read<void*>(state.playerState,0x6a8);
    state.topbarExpected=IsA(ownTeam,t::FFATeamClassGetter);
    if(!state.topbarExpected){state.topbarReady=false;return true;}
    if(!state.hudReady)return true;
    auto& out=t::state;
    if(state.topbarStarted&&out.hudModel!=original_hud::State().hudModel){
        // Native HUD replacement is a local UI lifecycle edge. Retire only the
        // observer caches; the engine owns the old HUD/widgets and their GC.
        state.topbarStarted=false;state.topbarReady=false;
    }
    if(!state.topbarStarted){
        out={};t::entries={};t::methods={};t::gameThread=GetCurrentThreadId();out.world=state.world;out.gameState=gs;
        out.controller=state.controller;out.hudModel=original_hud::State().hudModel;out.coordinator=original_hud::State().elementCoordinator;
        t::widgetClass=original_hud::LoadClass(0x63c3d90,t::WidgetClassPath);
        t::cardClass=original_hud::LoadClass(0x63c3d90,t::CardClassPath);t::masterClass=original_hud::LoadClass(0x63c3d90,t::MasterClassPath);
        if(!t::widgetClass||!t::cardClass||!t::masterClass||!t::BindMethods()||!t::Layouts())
            return Fail(out.error?out.error:4528,"native_client_original_ffa_topbar_ui_schema_changed");
        state.topbarStarted=true;
    }
    const auto playerStates=Read<Array>(gs,0x480),teams=Read<Array>(gs,0x720),models=Read<Array>(out.hudModel,0x50);
    if(!ValidArray(playerStates,32)||!ValidArray(teams,64)||!ValidArray(models,32))
        return Fail(4529,"native_client_topbar_replicated_arrays_invalid");
    if(playerStates.count<1||teams.count<playerStates.count||models.count!=playerStates.count)
        return Warn(0,"native_client_topbar_waiting_replicated_team_states_and_hud_roster");
    t::entries={};bool all=true;
    for(int i=0;i<playerStates.count;++i){auto ps=playerStates.data[i],team=Read<void*>(ps,0x6a8);void* native=nullptr;
        if(!IsA(ps,original_players::ShooterStateClass)||!IsA(team,t::FFATeamClassGetter)){all=false;break;}
        for(int j=0;j<teams.count;++j){auto candidate=teams.data[j];if(!IsA(candidate,t::TeamStateClassGetter)||Read<void*>(candidate,0x478)!=team)continue;
            const auto members=Read<Array>(candidate,0x468);if(!ValidArray(members,32))return Fail(4530,"native_client_topbar_replicated_team_members_invalid");
            for(int k=0;k<members.count;++k)if(members.data[k]==ps){if(native&&native!=candidate)return Fail(4531,"native_client_topbar_duplicate_replicated_team_state");native=candidate;}}
        if(!native){all=false;break;}
        auto& e=t::entries[size_t(i)];e.registered=true;e.index=uint32_t(i);e.playerState=ps;e.team=team;e.teamState=native;
        e.streak=Read<int32_t>(native,0x4b8);e.matchKills=-1;
        t::PointsQuery query{team,-1,0};t::TeamQuery identity{team,nullptr};
        if(e.streak<0||!t::Event(gs,t::methods.getPoints,&query)||!t::Event(gs,t::methods.getTeamState,&identity))return Fail(4532,"native_client_topbar_original_points_query_failed");
        e.nativePoints=query.result;e.nativePointsVerified=query.result==e.streak&&identity.result==native;all=all&&e.nativePointsVerified;
    }
    if(!all)return Warn(0,"native_client_topbar_waiting_consistent_native_team_replication");
    out.participants=uint32_t(playerStates.count);out.sourceRecordsVerified=true;out.originalPointsVerified=true;
    if(!t::RefreshModels())return Warn(out.error,out.stage);
    if(!out.widget){if(!t::EnsureWidget())return Warn(out.error,out.stage);}
    else {out.leftBox=Read<void*>(out.widget,size_t(t::leftOffset));out.rightBox=Read<void*>(out.widget,size_t(t::rightOffset));}
    if(!WidgetOwned(out.widget))return Fail(4533,"native_client_topbar_widget_owner_is_not_actual_local_controller");
    if(!t::ObserveCards())return Warn(out.error,out.stage);
    if(!t::CollapseOldMaster())return Fail(out.error,out.stage);
    out.initialized=true;out.ready=!out.pendingOriginalUpdate;out.error=0;
    out.stage=out.ready?"native_client_topbar_replicated_points_and_original_cards_observed":"native_client_topbar_waiting_original_card_update";
    state.topbarReady=out.ready;return true;
}
inline bool AbilityIdentityLive(const AbilityObjectIdentity& value){return Live(value.object,value.identity);}
inline void RememberSpectatedAbility(void* actor,void* widget,void* parent){
    const auto& view=local_server_spectator_view::GetState();
    SpectatedAbilityRecord next;next.targetGeneration=view.targetGeneration;next.playerId=view.nativePlayerId;
    next.world.object=state.world;next.controller.object=state.controller;next.hud.object=state.hud;
    next.panel.object=Read<void*>(state.hud,0xa58);next.actor.object=actor;
    next.playerState.object=view.targetState;next.widget.object=widget;next.parent.object=parent;
    next.camera.object=Read<void*>(state.controller,0x528);next.viewComponent.object=Read<void*>(state.controller,0xc60);
    for(auto* value:{&next.world,&next.controller,&next.hud,&next.panel,&next.actor,&next.playerState,
                    &next.widget,&next.parent,&next.camera,&next.viewComponent}){
        if(!Identity(value->object,value->identity))return;
    }
    if(Read<void*>(actor,0xb90)!=widget||Read<void*>(actor,0x488)!=view.targetState)return;
    next.valid=true;spectatedAbilityRecord=next;
}
// A replicated PlayerState can publish its new spawned character before the
// native spectator camera has left the same player's old body. Accept only
// that exact current character's locally-owned HUD widget during this native
// transition. This does not admit an old or otherwise unknown widget source.
inline bool NativeRespawnAbilityTransition(void* actor,void* widget,void* parent,bool inOwnAbilityParent){
    auto& audit=state.abilityWidgetAudit;const auto& old=spectatedAbilityRecord;
    const auto& view=local_server_spectator_view::GetState();
    audit.previousRecorded=old.valid;audit.previousActor=old.actor.object;audit.previousActorId=old.actor.identity;
    audit.previousWidget=old.widget.object;audit.previousWidgetId=old.widget.identity;
    audit.previousParent=old.parent.object;audit.previousParentId=old.parent.identity;
    audit.transitionStage="current_spawned_target_required";
    if(!local_server_spectator_view::Fresh()||!view.enabled||!view.nativeSpectatingKnown||!view.nativeSpectating
       ||view.mode!=4||!view.localOwnerVerified||!view.targetCurrentLife||actor!=view.targetPawn
       ||!inOwnAbilityParent||Read<void*>(actor,0xb90)!=widget||Read<void*>(actor,0x488)!=view.targetState
       ||!Live(actor,view.targetIdentity)||!Live(view.targetState,view.targetStateIdentity))return false;
    audit.transitionStage="previous_confirmed_same_player_life_required";
    if(!old.valid||old.actor.object==actor||old.playerState.object!=view.targetState||old.playerId!=view.nativePlayerId
       ||old.targetGeneration>=view.targetGeneration||old.world.object!=state.world||old.controller.object!=state.controller
       ||old.hud.object!=state.hud||old.panel.object!=Read<void*>(state.hud,0xa58))return false;
    // Historical widget/parent identities are diagnostic provenance. Only the
    // actual current widget receives this admission, so old-widget detachment
    // or visibility is not imposed as an additional native UI ordering rule.
    for(const auto* value:{&old.world,&old.controller,&old.hud,&old.panel,&old.actor,&old.playerState,&old.camera,&old.viewComponent})
        if(!AbilityIdentityLive(*value))return false;
    if(Read<uint8_t>(old.actor.object,0x224)!=1||Read<uint8_t>(actor,0x224)!=1
       ||Read<void*>(old.actor.object,0x488)!=view.targetState||Read<uint8_t>(view.targetState,0x224)!=1
       ||Read<void*>(state.controller,0x520)!=state.hud||Read<void*>(state.hud,0x468)!=state.controller
       ||Read<void*>(state.controller,0x528)!=old.camera.object||Read<void*>(state.controller,0xc60)!=old.viewComponent.object
       ||Read<void*>(old.camera.object,0x468)!=state.controller||Read<void*>(old.viewComponent.object,0xc8)!=state.controller
       ||Read<uint8_t>(old.viewComponent.object,0x178)!=4)return false;
    audit.logicalTarget=Read<void*>(old.viewComponent.object,0x170);audit.localTarget=Read<void*>(old.viewComponent.object,0x180);
    audit.cameraTarget=Read<void*>(old.camera.object,0x580);audit.pendingTarget=Read<void*>(old.camera.object,0xe80);
    audit.transitionStage="native_camera_must_still_reference_confirmed_previous_body";
    const auto nativeTransitionTarget=[&](void* target){return target==old.actor.object||target==actor;};
    if(audit.cameraTarget!=old.actor.object||!nativeTransitionTarget(audit.logicalTarget)||!nativeTransitionTarget(audit.localTarget)
       ||(audit.pendingTarget&&!nativeTransitionTarget(audit.pendingTarget)))return false;
    void* watched=nullptr;void* spawned=nullptr;void* owner=nullptr;void* currentParent=nullptr;
    const auto getParent=Function(L"/Script/UMG.Widget.GetParent",8);
    audit.currentNativeSource=local_server_spectator_view::Event(state.controller,
        L"/Script/ShooterGame.AresPlayerController.GetSpectatedSpawnedCharacter",8,&watched,0x64eaa00)
        &&watched==actor&&local_server_spectator_view::Event(view.targetState,
        L"/Script/ShooterGame.ShooterPlayerState.GetSpawnedCharacter",8,&spawned)&&spawned==actor;
    audit.transitionStage="current_native_subject_and_widget_identity_recheck_required";
    if(!audit.currentNativeSource||!Event(widget,methods.owningPlayer,&owner)||owner!=state.controller
       ||!getParent||!Event(widget,getParent,&currentParent)||currentParent!=parent
       ||!Live(actor,audit.actorId)||!Live(widget,audit.widgetId)||!Live(owner,audit.ownerId)||!Live(parent,audit.parentId)
       ||!Live(view.targetState,view.targetStateIdentity)||!AbilityIdentityLive(old.actor)||!AbilityIdentityLive(old.panel)
       ||Read<void*>(actor,0xb90)!=widget||Read<void*>(actor,0x488)!=view.targetState
       ||Read<void*>(state.hud,0xa58)!=old.panel.object||Read<void*>(old.camera.object,0x580)!=old.actor.object)return false;
    audit.transitionAllowed=true;audit.transitionStage="native_current_spawned_widget_during_same_player_camera_handoff";
    return true;
}
inline bool OwnedSpectatorAbilityWidget(void* actor,void* widget){
    state.abilityWidgetAudit={};auto& audit=state.abilityWidgetAudit;audit.actor=actor;audit.widget=widget;
    Identity(actor,audit.actorId);Identity(widget,audit.widgetId);
    state.auditRemoteWidget=widget;state.auditRemoteWidgetOwner=state.auditRemoteWidgetParent=nullptr;
    if(!IsA(widget,0x6621d40)){state.remoteWidgetAuditStage="remote_ability_widget_type_mismatch";return false;}
    void* owner=nullptr;
    if(!methods.owningPlayer||!Event(widget,methods.owningPlayer,&owner)){
        state.remoteWidgetAuditStage="remote_ability_widget_owner_query_unavailable";return false;
    }
    state.auditRemoteWidgetOwner=owner;audit.owner=owner;Identity(owner,audit.ownerId);
    if(owner!=state.controller){state.remoteWidgetAuditStage="remote_ability_widget_not_owned_by_actual_local_pc";return false;}
    void* parent=nullptr;const auto getParent=Function(L"/Script/UMG.Widget.GetParent",8);
    bool inViewport=false;
    if(!getParent||!Event(widget,getParent,&parent)||!original_hud::QueryWidgetInViewport(widget,inViewport)){
        state.remoteWidgetAuditStage="remote_ability_widget_attachment_query_unavailable";return false;
    }
    state.auditRemoteWidgetParent=parent;audit.parent=parent;if(parent)Identity(parent,audit.parentId);
    if(inViewport){state.remoteWidgetAuditStage="remote_ability_widget_unexpected_separate_viewport_root";return false;}
    // Original spectating can cache an unattached hero widget on a Role1
    // character. Cached UI is still owned by the single actual local PC.
    if(!parent){++state.cachedAbilityWidgetCount;state.remoteWidgetAuditStage="own_local_player_detached_hero_widget_cache";return true;}
    bool inOwnAbilityParent=false;auto ancestor=parent;
    for(unsigned n=0;ancestor&&n<16;++n){
        if(ancestor==Read<void*>(state.hud,0xa58)){inOwnAbilityParent=true;break;}
        void* next=nullptr;if(!Event(ancestor,getParent,&next)||next==ancestor)break;ancestor=next;
    }
    if(inOwnAbilityParent){
        // Native PlayerState.SpawnedCharacter and its attached hero widget can
        // arrive while all camera target pointers still reference the old body.
        // MaybeObserve's camera-pointer/100ms cache cannot establish this source.
        // Refresh original read-only getters before either current-life test.
        const auto& before=local_server_spectator_view::GetState();
        audit.spectatorSampleBefore=before.sampledAtMs;audit.spectatorTargetBefore=before.targetIdentity;
        local_server_spectator_view::Observe(state.world,state.controller,state.pawn);
        const auto& after=local_server_spectator_view::GetState();
        audit.spectatorRefreshed=true;audit.spectatorSampleAfter=after.sampledAtMs;audit.spectatorTargetAfter=after.targetIdentity;
    }
    const auto& view=local_server_spectator_view::GetState();
    if(local_server_spectator_view::Fresh()&&view.enabled&&view.localOwnerVerified&&view.targetCurrentLife
       &&view.cameraTargetConfirmed&&view.targetPawn==actor&&inOwnAbilityParent){
        RememberSpectatedAbility(actor,widget,parent);
        ++state.spectatedAbilityWidgetCount;state.remoteWidgetAuditStage="native_spectated_hero_widget_owned_by_actual_local_pc";return true;
    }
    if(NativeRespawnAbilityTransition(actor,widget,parent,inOwnAbilityParent)){
        ++state.transitionAbilityWidgetCount;state.remoteWidgetAuditStage=audit.transitionStage;return true;
    }
    state.remoteWidgetAuditStage="attached_hero_widget_is_not_current_native_spectator_target";
    return false;
}
inline bool AuditActorClass(uintptr_t getter,unsigned kind){
    struct Query {void* world;void* type;void** data=nullptr;int32_t count=0,capacity=0;} q{state.world,engine::Class(getter)};
    if(!Event(engine::DefaultObject(engine::Class(0x43a9670)),methods.actors,&q))return Fail(4534,"native_client_ownership_actor_enumeration_failed");
    bool valid=q.count>=0&&q.count<=128&&q.capacity>=q.count&&(!q.count||original_players::Readable(q.data,size_t(q.count)*8));
    if(valid)for(int i=0;i<q.count;++i){auto actor=q.data[i];
        if(kind==0){
            if(actor==state.controller)continue;
            ++state.foreignControllerCount;
            if(Read<void*>(actor,0x510))state.noForeignUI=false;
            if(Read<void*>(actor,0xc18)){++state.foreignUIManagerCount;state.noForeignUI=false;}
            if(Read<void*>(actor,0xc20)){++state.foreignUIDirectorCount;state.noForeignUI=false;}
            if(Read<void*>(actor,0x520)){++state.foreignHUDCount;state.noForeignUI=false;}
        }else if(kind==1){
            if(actor==state.pawn||Read<void*>(actor,0x488)==state.playerState)continue;
            ++state.remotePawnCount;if(Read<uint8_t>(actor,0x224)!=1)state.remoteRolesVerified=false;
            auto widget=Read<void*>(actor,0xb90);
            if(widget){++state.remoteAbilityWidgetCount;if(!OwnedSpectatorAbilityWidget(actor,widget)){
                state.noForeignUI=false;
                if(!state.failedAbilityWidgetAudit.actor)state.failedAbilityWidgetAudit=state.abilityWidgetAudit;
            }}
        }else{
            if(Read<void*>(actor,0x468)==state.controller)++state.ownHUDCount;
            else {++state.foreignHUDCount;state.noForeignUI=false;}
        }
    }
    if(q.data)engine::function<void(*)(void*)>(0x1915720)(q.data);
    return valid?true:Fail(4535,"native_client_ownership_actor_array_outside_bound");
}
inline bool AuditOwnership(){
    const auto now=GetTickCount64();if(state.lastAuditMs&&now-state.lastAuditMs<1000)return true;state.lastAuditMs=now;
    local_server_spectator_view::MaybeObserve(state.world,state.controller,state.pawn);
    state.ownershipAuditComplete=false;state.noForeignUI=true;state.remoteRolesVerified=true;
    state.ownHUDCount=state.foreignHUDCount=state.remotePawnCount=state.remoteAbilityWidgetCount=0;
    state.spectatedAbilityWidgetCount=state.cachedAbilityWidgetCount=state.transitionAbilityWidgetCount=0;
    state.abilityWidgetAudit={};state.failedAbilityWidgetAudit={};
    state.auditRemoteWidget=state.auditRemoteWidgetOwner=state.auditRemoteWidgetParent=nullptr;
    state.remoteWidgetAuditStage="no_remote_ability_widget";
    state.foreignControllerCount=state.foreignUIManagerCount=state.foreignUIDirectorCount=0;
    state.uiManager=Read<void*>(state.controller,0xc18);state.uiDirector=Read<void*>(state.controller,0xc20);
    state.ownUIManagerPresent=state.uiManager!=nullptr;state.ownUIDirectorPresent=state.uiDirector!=nullptr;
    // The native ShooterPC default has bCreateUIInstancesFromGlobals=false.
    // Absence stays explicit; do not manufacture a tickable manager/director or
    // rerun PostInitializeComponents merely to make a diagnostic flag true.
    state.uiManagerOwnershipVerified=(!state.uiManager||(IsA(state.uiManager,0x655e8b0)&&Read<void*>(state.uiManager,0x28)==state.controller
        &&Read<void*>(state.uiManager,0x68)==Read<void*>(state.hud,0x760)))
        &&(!state.uiDirector||(IsA(state.uiDirector,0x655c0d0)&&Read<void*>(state.uiDirector,0x28)==state.controller
        &&Read<void*>(state.uiDirector,0x50)==state.uiManager));
    if(!AuditActorClass(original_players::ShooterControllerClass,0)||!AuditActorClass(original_players::ShooterCharacterClass,1)
       ||!AuditActorClass(0x6cae7d0,2))return false;
    state.hudUnique=state.ownHUDCount==1&&state.foreignHUDCount==0;
    state.uiManagerUnique=state.ownUIManagerPresent&&state.foreignUIManagerCount==0&&state.uiManagerOwnershipVerified;
    state.ownershipAuditComplete=true;
    if(!state.uiManagerOwnershipVerified||!state.noForeignUI||!state.remoteRolesVerified)
        return Fail(4536,"native_client_remote_or_foreign_actor_has_local_presentation_ownership");
    return true;
}
inline bool CurrentSpectatedStreakSource(void*world,const original_streak_hud::Observation&row){
    const auto&view=local_server_spectator_view::GetState();
    if(!Thread()||!state.initialized||state.retired||world!=state.world||world!=original_players::world
       ||state.controller!=original_players::activeController||state.pawn!=original_players::activePawn
       ||!Live(state.world,state.worldId)||!Live(state.controller,state.controllerId)||!Live(state.pawn,state.pawnId)
       ||!Live(state.playerState,state.playerStateId)||!Live(state.localPlayer,state.localPlayerId)
       ||Read<uint8_t>(state.controller,0x224)!=2||Read<void*>(state.controller,0x510)!=state.localPlayer
       ||Read<void*>(state.localPlayer,0x38)!=state.controller||state.hud!=Read<void*>(state.controller,0x520)
       ||!state.hud||state.hud!=original_hud::State().hud||Read<void*>(state.hud,0x468)!=state.controller)return false;
    // Origin/source qualification is independent of the FOV/pose audit. The
    // native mode and exact current target/life remain mandatory every time.
    if(!local_server_spectator_view::Fresh()||!view.enabled||view.mode!=4||!view.nativeSpectatingKnown||!view.nativeSpectating
       ||!view.localOwnerVerified||!view.targetCurrentLife||!view.cameraTargetConfirmed||view.nativePlayerId<=0
       ||view.controller!=state.controller||view.ownPawn!=state.pawn||row.target!=view.targetPawn||row.source!=view.targetPawn
       ||row.owner!=state.controller||!row.ownerIsActualLocal||view.targetPawn==state.pawn||view.targetState==state.playerState
       ||!Live(view.targetPawn,view.targetIdentity)||!Live(view.targetState,view.targetStateIdentity)
       ||!IsA(view.targetPawn,original_players::ShooterCharacterClass)||!IsA(view.targetState,original_players::ShooterStateClass)
       ||Read<uint8_t>(view.targetPawn,0x224)!=1||Read<uint8_t>(view.targetState,0x224)!=1
       ||Read<void*>(view.targetPawn,0x488)!=view.targetState||row.playerState!=view.targetState
       ||Read<void*>(view.targetPawn,0x4a0)==state.controller)return false;
    const auto component=Read<void*>(state.controller,0xc60),camera=Read<void*>(state.controller,0x528);
    if(Read<uint8_t>(component,0x178)!=4||Read<void*>(component,0xc8)!=state.controller||Read<void*>(camera,0x468)!=state.controller
       ||Read<void*>(component,0x170)!=view.targetPawn||Read<void*>(component,0x180)!=view.targetPawn
       ||Read<void*>(camera,0x580)!=view.targetPawn||Read<void*>(camera,0xe80))return false;
    void*watched=nullptr;void*spawned=nullptr;int32_t playerId=-1;
    return local_server_spectator_view::Event(state.controller,L"/Script/ShooterGame.AresPlayerController.GetSpectatedSpawnedCharacter",8,&watched,0x64eaa00)
        &&watched==view.targetPawn&&Live(view.targetPawn,view.targetIdentity)&&Live(view.targetState,view.targetStateIdentity)
        &&local_server_spectator_view::Event(view.targetState,L"/Script/ShooterGame.ShooterPlayerState.GetSpawnedCharacter",8,&spawned)
        &&spawned==view.targetPawn&&Live(view.targetState,view.targetStateIdentity)
        &&local_server_spectator_view::Event(view.targetState,L"/Script/Engine.PlayerState.GetPlayerId",4,&playerId,0x49452a0)
        &&playerId==view.nativePlayerId&&Live(view.targetPawn,view.targetIdentity)&&Live(view.targetState,view.targetStateIdentity)
        &&Read<void*>(view.targetPawn,0x488)==view.targetState&&Read<void*>(state.controller,0xc60)==component
        &&Read<void*>(state.controller,0x528)==camera&&Read<uint8_t>(component,0x178)==4
        &&Read<void*>(component,0x170)==view.targetPawn&&Read<void*>(component,0x180)==view.targetPawn
        &&Read<void*>(camera,0x580)==view.targetPawn&&!Read<void*>(camera,0xe80);
}
inline std::array<void*,StreakMetadataCount> CurrentStreakMetadata(){
    const auto& s=original_streak_hud::State();const auto& m=original_streak_hud::methods;
    return {s.effectClass,s.componentClass,s.widgetClass,s.gameplay,s.timerLibrary,s.textLibrary,
        m.actors,m.owner,m.viewport,m.visible,m.visibility,m.tick,m.remaining,m.text,m.textToString,m.childCount,m.childAt,m.parent};
}
inline bool CaptureStreakMetadata(){
    const auto objects=CurrentStreakMetadata();state.streakMetadataBound=false;
    for(size_t i=0;i<objects.size();++i){auto& entry=streakMetadata[i];entry={};entry.object=objects[i];
        if(!Identity(entry.object,entry.identity))return Fail(4551,"native_client_streak_fresh_original_metadata_identity_unavailable");}
    state.streakMetadataBound=true;++state.streakMetadataGeneration;return true;
}
inline bool PrepareStreakObserver(){
    const auto& before=original_streak_hud::State();
    state.streakInnerExceptionRva=before.exceptionRva;
    // A real prior failure is terminal. Metadata retirement below must never
    // clear restartRequired/error or turn an exception into a ready observer.
    if(before.restartRequired||before.error){if(before.exceptionRva)state.exceptionRva=before.exceptionRva;
        return Fail(before.error?before.error:4552,before.stage);}
    if(state.streakStarted){
        if(!state.streakMetadataBound)return Fail(4553,"native_client_streak_original_metadata_was_not_bound");
        const auto objects=CurrentStreakMetadata();size_t expired=objects.size();
        for(size_t i=0;i<objects.size();++i){const auto& entry=streakMetadata[i];
            // Resolve the identity captured immediately after original Load/
            // Bind. Do not mint a new identity from a stale/reused raw pointer.
            if(entry.object!=objects[i]||!Live(entry.object,entry.identity)){expired=i;break;}}
        if(expired!=objects.size()){
            state.streakExpiredMetadataRole=StreakMetadataRoles[expired];++state.streakMetadataExpirations;
            state.streakRetiredSamples+=before.samples;const auto cadence=before.inspectionCadenceMs;
            // These are DLL observer caches only. No original FX/widget,
            // UObject flags, native root set or engine references are changed.
            original_streak_hud::state={};original_streak_hud::layout={};original_streak_hud::methods={};
            original_streak_hud::state.inspectionCadenceMs=cadence;
            streakMetadata={};state.streakMetadataBound=false;state.streakStarted=false;state.streakObserved=false;
            ++state.streakMetadataRefreshes;
        }
    }
    if(!state.streakStarted){
        if(!original_streak_hud::Initialize(state.world)){
            const auto& failed=original_streak_hud::State();state.streakInnerExceptionRva=failed.exceptionRva;
            if(failed.exceptionRva)state.exceptionRva=failed.exceptionRva;return Fail(failed.error,failed.stage);}
        if(!CaptureStreakMetadata())return false;state.streakStarted=true;
    }
    return true;
}
inline bool ObserveAudioAndStreak(){
    {ProbeScope probe(Probe::Audio);
        if(!state.audioStarted){if(!original_audio_listeners::Initialize(state.world))return Fail(original_audio_listeners::State().error,original_audio_listeners::State().stage);state.audioStarted=true;}
        else if(!original_audio_listeners::Sample())return Fail(original_audio_listeners::State().error,original_audio_listeners::State().stage);
    }
    const auto& a=original_audio_listeners::State();state.audioObserved=a.current.valid;state.audioWaiting=a.waiting;
    state.foreignListenerCount=uint32_t(a.current.nonLocalPC);
    state.audioOwnListenerVerified=a.deviceReady&&a.current.valid&&a.current.own==1&&!a.ambiguous&&a.current.nonLocalPC==0;
    if(state.foreignListenerCount)return Fail(4537,"native_client_nonlocal_pc_default_audio_listener_observed");
    // Sample only: never invoke the Standalone listener-removal reconciliation.
    {ProbeScope probe(Probe::Streak);
        if(!PrepareStreakObserver())return false;
        if(!original_streak_hud::BindSpectatedSource(CurrentSpectatedStreakSource,state.world,state.controller,state.hud))
            return Fail(original_streak_hud::State().error,original_streak_hud::State().stage);
        if(!original_streak_hud::Poll()){
            const auto& failed=original_streak_hud::State();state.streakInnerExceptionRva=failed.exceptionRva;
            if(failed.exceptionRva)state.exceptionRva=failed.exceptionRva;return Fail(failed.error,failed.stage);}
    }
    state.streakObserved=original_streak_hud::State().samples>0;
    return true;
}
inline bool ObserveInput(){
    auto playerInput=Read<void*>(state.controller,0x5e8),input=Read<void*>(state.pawn,0x230);
    state.bindingsReady=original_input_bindings::State().initialized&&original_input_bindings::State().links_valid;
    state.nativeInputReady=state.bindingsReady&&IsA(playerInput,0x64f9c00)&&Read<void*>(playerInput,0x28)==state.controller
        &&Read<void*>(state.controller,0x230)&&IsA(input,0x6492f30)&&Read<void*>(input,0xc8)==state.pawn;
    // SetPlayer/ClientRestart and the normal native tick own binding application,
    // mouse/action production and prediction on a network client. No manual
    // action submission, input-buffer drain, actor tick or movement call here.
    return true;
}
inline bool PollMenus(){
    if(!original_hud::State().originalHUDInstalled)return true;
    ProbeScope probe(Probe::UI);
    original_ui_input_mode::SetExternalMenu(false);
    if(!original_ui_input_mode::UpdateFromWindowEvents())return Fail(original_ui_input_mode::State().error,original_ui_input_mode::State().stage);
    if(!local_server_menu_input::Poll())return Fail(4555,"native_client_own_window_menu_input_failed");
    const auto& ui=original_ui_input_mode::State();state.menuRoutingReady=ui.initialized&&ui.localBindingsValid;
    state.gameplayBlocked=ui.blockGameplay;state.cursorVisible=ui.cursorVisible;return true;
}
inline bool PollImpl(float delta){
    if(!state.initialized||!Thread())return Fail(4538,"native_client_presentation_wrong_lifecycle_or_thread");
    if(state.error)return false;if(!std::isfinite(delta)||delta<0||delta>2)return Fail(4539,"native_client_presentation_invalid_delta");
    ++state.polls;state.warning=0;state.warningStage="none";
    if(!ValidateLocalPair(!state.retired))return false;
    local_server_feedback_view::Poll(state.controller);
    if(state.retired){if(!PollMenus())return false;return Waiting("native_client_presentation_waiting_native_respawn_and_initialize");}
    if(!local_server_equipment::State().clientAppearancePrepared){
        ProbeScope probe(Probe::AppearancePreparation);
        if(!local_server_equipment::PrepareClientAppearance())return Fail(local_server_equipment::State().error,local_server_equipment::State().stage);
    }
    {ProbeScope probe(Probe::Equipment);
        if(!local_server_equipment::PollClient(state.pawn))return Fail(local_server_equipment::State().error?local_server_equipment::State().error:4540,local_server_equipment::State().stage);
    }
    if(!ObserveInput())return false;
    const auto now=GetTickCount64();
    // Construction and native menu-state polling stay responsive each frame;
    // larger HUD/view/audio snapshots are bounded to ten samples per second.
    if(!state.hudReady||!state.lastSampleMs||now-state.lastSampleMs>=100){
        state.lastSampleMs=now;
        local_server_spectator_view::Observe(state.world,state.controller,state.pawn);
        {ProbeScope probe(Probe::HUD);if(!BindHUD())return false;}
        {ProbeScope probe(Probe::ShopConstruction);if(state.hudReady&&!PrepareShop())return false;}
        if(!SampleNativeView())return false;
        if(!ObserveAudioAndStreak())return false;
    }
    if(!PollMenus())return false;
    {ProbeScope probe(Probe::ShopInventory);if(shop.initialized&&!RefreshShopInventory())return false;}
    {ProbeScope probe(Probe::Topbar);if(!PollReplicaTopbar())return false;}
    {ProbeScope probe(Probe::Ownership);if(!AuditOwnership())return false;}
    CaptureReadiness();
    state.ready=state.hudReady&&state.hudUnique&&state.localPairVerified&&state.viewportOwnerVerified&&state.nativeInputReady
        &&state.cameraReady&&state.menuRoutingReady&&shop.initialized&&state.ownershipAuditComplete&&state.noForeignUI
        &&state.remoteRolesVerified&&state.uiManagerOwnershipVerified&&state.audioOwnListenerVerified
        &&(!state.topbarExpected||state.topbarReady);
    state.stage=state.ready?"native_client_original_presentation_bound_execution_unverified"
        :"native_client_presentation_waiting_native_replica_lifecycle";
    return true;
}
inline bool RetireImpl(void* oldPawn){
    if(!state.initialized||!Thread())return Fail(4541,"native_client_retire_wrong_lifecycle_or_thread");
    if(state.retired)return !oldPawn||oldPawn==state.pawn;
    if(!oldPawn||oldPawn!=state.pawn)return Fail(4542,"native_client_retire_requires_current_cached_pawn");
    // No old raw UObject read, gameplay cleanup, unpossess, RPC or GC operation.
    // The PC, its menus, LocalPlayer and viewport survive an ordinary respawn.
    state.retired=true;state.ready=false;state.hudReady=false;state.cameraBound=state.cameraReady=false;
    state.adsObserved=false;state.nativeInputReady=false;state.weapon={};state.pawnId={};++state.retirements;
    state.lastSampleMs=state.lastAuditMs=0;original_camera::state.bound=false;
    state.stage="native_client_presentation_retired_waiting_native_respawn";return true;
}
inline bool InitializeImpl(void* world,void* pc,void* ps,void* pawn){
    if(state.error||state.restartRequired)return false;
    if(!state.imageBase){
        state.imageBase=engine::image;state.faultsBeforeInitialize=uint64_t(original_faults::Read64(&original_faults::observedCount));
        MEMORY_BASIC_INFORMATION moduleInfo{};
        if(VirtualQuery(reinterpret_cast<const void*>(&InitializeImpl),&moduleInfo,sizeof(moduleInfo)))
            state.moduleBase=reinterpret_cast<uintptr_t>(moduleInfo.AllocationBase);
    }
    if(!engine::image||!original_players::Thread())return Fail(4543,"native_client_initialize_requires_original_game_thread");
    if(!world||!IsA(pc,original_players::ShooterControllerClass)||!IsA(ps,original_players::ShooterStateClass)
       ||!IsA(pawn,original_players::ShooterCharacterClass))return Fail(4544,"native_client_initialize_requires_received_native_player");
    if(state.initialized&&(state.world!=world||state.controller!=pc||state.playerState!=ps))
        return Fail(4545,"native_client_presentation_world_or_connection_change_requires_new_session");
    if(state.initialized&&!state.retired&&state.pawn==pawn)return true;
    if(state.initialized&&!state.retired&&!RetireImpl(state.pawn))return false;
    state.gameThread=GetCurrentThreadId();state.world=world;state.controller=pc;state.playerState=ps;state.pawn=pawn;
    state.localPlayer=Read<void*>(pc,0x510);
    if(!Identity(world,state.worldId)||!Identity(pc,state.controllerId)||!Identity(ps,state.playerStateId)
       ||!Identity(pawn,state.pawnId)||!Identity(state.localPlayer,state.localPlayerId))
        return Fail(4546,"native_client_original_weak_identity_capture_failed");
    if(!state.profilesVerified){state.profilesVerified=VerifyProfiles();if(!state.profilesVerified)return Fail(4547,"native_client_presentation_pinned_source_profile_changed");}
    if(!ValidateLocalPair()||!ResolveMethods())return state.error?false:Fail(4548,"native_client_original_reflected_api_signature_changed");
    if(original_players::world!=world||original_players::activeController!=pc||original_players::activePawn!=pawn
       ||original_players::localPlayer!=state.localPlayer||original_hud::world!=world||original_hud::controller!=pc)
        return Fail(4549,"native_client_root_must_adopt_received_native_player_before_presentation");
    state.initialized=true;state.retired=false;state.hudReady=false;state.cameraBound=false;state.cameraReady=false;
    state.lastSampleMs=state.lastAuditMs=0;shop.inventoryRefreshPending=true;++state.epochs;
    state.stage="native_client_original_player_adopted_waiting_presentation";return PollImpl(0);
}
inline LONG CaptureException(EXCEPTION_POINTERS* e){
    state.error=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:4550;
    const auto at=reinterpret_cast<uintptr_t>(e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionAddress:nullptr);
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.restartRequired=true;state.ready=false;
    state.stage="native_client_presentation_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* world,void* pc,void* ps,void* pawn){__try{return InitializeImpl(world,pc,ps,pawn);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool Poll(float delta){__try{return PollImpl(delta);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool Retire(void* oldPawn){__try{return RetireImpl(oldPawn);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool ToggleShop(){
    __try {if(!Thread()||!shop.initialized||!ValidateLocalPair(false))return false;int32_t index=0;
        if(!Event(state.controller,methods.toggleShop,&index))return false;++shop.toggles;return ObserveShopOpen();}
    __except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool ToggleOptions(){return Thread()&&original_hud::ToggleOriginalOptions();}
inline bool CloseUI(){return Thread()&&original_ui_input_mode::CloseActiveUI();}
inline void WriteShopOpenPath(FILE* f){
    // Serialize the bounded game-thread snapshot; never inspect native objects here.
    const auto& x=shop.path;const auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,",\"shop_open_path\":{\"scope\":\"read_only_current_state_not_execution_trace\",\"stage\":\"%s\",\"samples\":%llu,\"last_sample_ms\":%llu,\"minimum_interval_ms\":100,\"native_lookup_total\":%llu,\"get_world_rva\":%llu,\"get_world_profiled\":%s,\"world_queried\":%s,\"coordinator_live\":%s,\"controller_matches\":%s,\"controller\":\"%p\",\"world\":\"%p\",\"world_live\":%s,\"world_matches\":%s,\"player_state\":\"%p\",\"player_state_queried\":%s,\"player_state_matches\":%s,\"game_state\":\"%p\",\"game_state_live\":%s,\"game_state_shooter\":%s",
        x.stage,static_cast<unsigned long long>(x.samples),static_cast<unsigned long long>(x.lastSampleMs),static_cast<unsigned long long>(x.nativeLookupTotal),static_cast<unsigned long long>(x.getWorldRva),b(x.getWorldProfiled),b(x.worldQueried),b(x.coordinatorLive),b(x.controllerMatches),x.controller,x.world,b(x.worldLive),b(x.worldMatches),x.playerState,b(x.playerStateQueried),b(x.playerStateMatches),x.gameState,b(x.gameStateLive),b(x.gameStateShooter));
    std::fprintf(f,",\"entry_array_valid\":%s,\"entry_count\":%d,\"entry\":\"%p\",\"entry_live\":%s,\"screen\":\"%p\",\"screen_queried\":%s,\"screen_live\":%s,\"screen_matches\":%s,\"screen_object_flags\":%u,\"screen_weak_index\":%d,\"screen_weak_serial\":%d,\"menu_stack\":\"%p\",\"stack_live\":%s,\"stack_array_valid\":%s,\"open_entry_count\":%d,\"entry_in_open_stack\":%s,\"game_info_model\":\"%p\",\"player_model\":\"%p\",\"character_rule_allowed\":%s,\"character_shop_allowed\":%s,\"character_rule_required\":%s",
        b(x.entryArrayValid),x.entryCount,x.entry,b(x.entryLive),x.screen,b(x.screenQueried),b(x.screenLive),b(x.screenMatches),x.screenFlags,x.screenId.index,x.screenId.serial,x.menuStack,b(x.stackLive),b(x.stackArrayValid),x.openEntryCount,b(x.entryInOpenStack),x.gameInfoModel,x.playerModel,b(x.characterRuleAllowed),b(x.characterShopAllowed),b(x.characterRuleRequired));
    std::fprintf(f,",\"spawned_character\":\"%p\",\"spawned_live\":%s,\"spawned_matches\":%s,\"inventory\":\"%p\",\"inventory_live\":%s,\"inventory_owned\":%s,\"slots_live\":%s,\"starting_array_valid\":%s,\"starting_class_count\":%d,\"observed_starting_classes\":%d,\"starting_classes_complete\":%s,\"native_lookup_calls\":%d,\"deferred_defaults\":%d,\"missing_items\":%d,\"invalid_starting_classes\":%d,\"world_game_instance\":\"%p\",\"client_game_instance\":\"%p\",\"client_game_instance_live\":%s,\"flow_manager\":\"%p\",\"flow_manager_live\":%s,\"flow_state\":\"%p\",\"flow_state_live\":%s,\"flow_state_type\":%u,\"flow_in_game\":%s,\"original_tick_inputs_observed\":%s,\"flow_tick_invocations\":0,\"model_build_invocations\":0,\"policy_writes\":0,\"cdo_creation_invocations\":0,\"starting_classes\":[",
        x.spawnedCharacter,b(x.spawnedLive),b(x.spawnedMatches),x.inventory,b(x.inventoryLive),b(x.inventoryOwned),b(x.slotsLive),b(x.startingArrayValid),x.startingClassCount,x.observedStartingClasses,b(x.startingClassesComplete),x.nativeLookupCalls,x.deferredDefaults,x.missingItems,x.invalidStartingClasses,x.worldGameInstance,x.clientGameInstance,b(x.clientGameInstanceLive),x.flowManager,b(x.flowManagerLive),x.flowState,b(x.flowStateLive),unsigned(x.flowStateType),b(x.flowInGame),b(x.originalTickInputsObserved));
    for(int32_t i=0;i<x.observedStartingClasses;++i){if(i)std::fputc(',',f);const auto& row=x.startingClasses[size_t(i)];
        std::fprintf(f,"{\"index\":%d,\"class\":\"%p\",\"class_live\":%s,\"class_object\":%s,\"class_equippable\":%s,\"class_weak_index\":%d,\"class_weak_serial\":%d,\"defaults\":\"%p\",\"defaults_live\":%s,\"defaults_deferred\":%s,\"default_slot\":%u,\"queried\":%s,\"item\":\"%p\",\"item_live\":%s,\"item_equippable\":%s,\"item_owned\":%s,\"item_weak_index\":%d,\"item_weak_serial\":%d}",
            i,row.type,b(row.typeLive),b(row.classObject),b(row.classEquippable),row.typeId.index,row.typeId.serial,row.defaults,b(row.defaultsLive),b(row.defaultsDeferred),unsigned(row.defaultSlot),b(row.queried),row.item,b(row.itemLive),b(row.itemEquippable),b(row.itemOwned),row.itemId.index,row.itemId.serial);}
    std::fputs("]}",f);
}
inline void WriteAbilityWidgetAudit(FILE* f,const AbilityWidgetAudit& value){
    const auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"actor\":\"%p\",\"actor_weak\":[%d,%d],\"widget\":\"%p\",\"widget_weak\":[%d,%d],\"owner\":\"%p\",\"owner_weak\":[%d,%d],\"parent\":\"%p\",\"parent_weak\":[%d,%d],\"previous_recorded\":%s,\"previous_actor\":\"%p\",\"previous_actor_weak\":[%d,%d],\"logical_target\":\"%p\",\"local_target\":\"%p\",\"camera_target\":\"%p\",\"pending_target\":\"%p\",\"current_native_source\":%s,\"transition_allowed\":%s,\"transition_stage\":\"%s\"",
        value.actor,value.actorId.index,value.actorId.serial,value.widget,value.widgetId.index,value.widgetId.serial,
        value.owner,value.ownerId.index,value.ownerId.serial,value.parent,value.parentId.index,value.parentId.serial,
        b(value.previousRecorded),value.previousActor,value.previousActorId.index,value.previousActorId.serial,
        value.logicalTarget,value.localTarget,value.cameraTarget,value.pendingTarget,b(value.currentNativeSource),b(value.transitionAllowed),value.transitionStage);
    std::fprintf(f,",\"previous_widget\":\"%p\",\"previous_widget_weak\":[%d,%d],\"previous_parent\":\"%p\",\"previous_parent_weak\":[%d,%d]",
        value.previousWidget,value.previousWidgetId.index,value.previousWidgetId.serial,
        value.previousParent,value.previousParentId.index,value.previousParentId.serial);
    std::fprintf(f,",\"spectator_refreshed\":%s,\"spectator_sample_before_ms\":%llu,\"spectator_sample_after_ms\":%llu,\"spectator_target_before_weak\":[%d,%d],\"spectator_target_after_weak\":[%d,%d]}",
        b(value.spectatorRefreshed),static_cast<unsigned long long>(value.spectatorSampleBefore),
        static_cast<unsigned long long>(value.spectatorSampleAfter),value.spectatorTargetBefore.index,value.spectatorTargetBefore.serial,
        value.spectatorTargetAfter.index,value.spectatorTargetAfter.serial);
}
inline void WriteState(FILE* f){
    if(!f)return;const auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"warning\":%u,\"warning_stage\":\"%s\",\"initialized\":%s,\"retired\":%s,\"presentation_bound\":%s,\"restart_required\":%s,\"profiles_verified\":%s,\"polls\":%llu,\"epochs\":%llu,\"retirements\":%llu,\"view_samples\":%llu,\"net_mode\":%d,\"local_player_count\":%d,\"controller_role\":%u,\"pawn_role\":%u,\"pawn_remote_role\":%u,\"world\":\"%p\",\"controller\":\"%p\",\"player_state\":\"%p\",\"pawn\":\"%p\",\"local_player\":\"%p\",\"viewport_client\":\"%p\",\"scene_viewport\":\"%p\",\"hud\":\"%p\",\"ui_manager\":\"%p\",\"ui_director\":\"%p\"",
        state.stage,state.error,state.warning,state.warningStage,b(state.initialized),b(state.retired),b(state.ready),b(state.restartRequired),b(state.profilesVerified),
        static_cast<unsigned long long>(state.polls),static_cast<unsigned long long>(state.epochs),static_cast<unsigned long long>(state.retirements),static_cast<unsigned long long>(state.viewSamples),
        state.netMode,state.localPlayerCount,unsigned(state.controllerRole),unsigned(state.pawnRole),unsigned(state.pawnRemoteRole),state.world,state.controller,state.playerState,state.pawn,state.localPlayer,state.viewportClient,state.sceneViewport,state.hud,state.uiManager,state.uiDirector);
    std::fprintf(f,",\"local_pair_verified\":%s,\"native_client\":%s,\"viewport_owner_verified\":%s,\"settings_ready\":%s,\"bindings_ready\":%s,\"native_input_ready\":%s,\"hud_ready\":%s,\"hud_unique\":%s,\"first_person_widget_owned\":%s,\"camera_ready\":%s,\"ads_observed\":%s,\"menu_routing_ready\":%s,\"gameplay_blocked\":%s,\"cursor_visible\":%s,\"ownership_audit_complete\":%s,\"no_foreign_ui\":%s,\"remote_roles_verified\":%s,\"own_ui_manager_present\":%s,\"own_ui_director_present\":%s,\"ui_manager_ownership_verified\":%s,\"ui_manager_unique\":%s,\"own_huds\":%u,\"foreign_huds\":%u,\"remote_pawns\":%u,\"remote_ability_widgets\":%u,\"foreign_controllers\":%u,\"foreign_ui_managers\":%u,\"foreign_ui_directors\":%u",
        b(state.localPairVerified),b(state.nativeClient),b(state.viewportOwnerVerified),b(state.settingsReady),b(state.bindingsReady),b(state.nativeInputReady),b(state.hudReady),b(state.hudUnique),b(state.firstPersonWidgetOwned),b(state.cameraReady),b(state.adsObserved),b(state.menuRoutingReady),b(state.gameplayBlocked),b(state.cursorVisible),b(state.ownershipAuditComplete),b(state.noForeignUI),b(state.remoteRolesVerified),b(state.ownUIManagerPresent),b(state.ownUIDirectorPresent),b(state.uiManagerOwnershipVerified),b(state.uiManagerUnique),state.ownHUDCount,state.foreignHUDCount,state.remotePawnCount,state.remoteAbilityWidgetCount,state.foreignControllerCount,state.foreignUIManagerCount,state.foreignUIDirectorCount);
    std::fprintf(f,",\"spectated_ability_widgets\":%u,\"cached_ability_widgets\":%u,\"transition_ability_widgets\":%u",state.spectatedAbilityWidgetCount,state.cachedAbilityWidgetCount,state.transitionAbilityWidgetCount);
    std::fputs(",\"ability_widget_audit\":",f);WriteAbilityWidgetAudit(f,state.abilityWidgetAudit);
    std::fputs(",\"failed_ability_widget_audit\":",f);WriteAbilityWidgetAudit(f,state.failedAbilityWidgetAudit);
    std::fprintf(f,",\"remote_widget_audit_stage\":\"%s\",\"remote_widget\":\"%p\",\"remote_widget_owner\":\"%p\",\"remote_widget_parent\":\"%p\"",
        state.remoteWidgetAuditStage,state.auditRemoteWidget,state.auditRemoteWidgetOwner,state.auditRemoteWidgetParent);
    std::fprintf(f,",\"audio_observed\":%s,\"audio_waiting\":%s,\"audio_own_listener_verified\":%s,\"foreign_listeners\":%u,\"streak_observed\":%s,\"topbar_expected\":%s,\"topbar_ready\":%s,\"shop_stage\":\"%s\",\"shop_ready\":%s,\"shop_open_known\":%s,\"shop_open\":%s,\"shop_native_purchase_rpc_verified\":%s,\"shop_cache_supplied\":%s,\"shop_screen_supplied\":%s,\"shop_shopping_enabled\":%s,\"shop_money\":%d,\"shop_models\":%d,\"shop_model_refreshes\":%llu,\"weapon\":\"%p\",\"weapon_owner_verified\":%s,\"recoil_queried\":%s,\"pitch_recoil\":%.9g,\"yaw_recoil\":%.9g,\"firing_error\":%.9g,\"ammo_queried\":%s,\"ammo\":%d,\"predicted_ammo\":%d,\"authority_ammo\":%d,\"exception_rva\":%llu,\"native_prediction_and_camera_scheduler_retained\":true,\"manual_input_submissions\":0,\"client_equipment_grants\":0,\"client_economy_writes\":0,\"client_score_writes\":0,\"camera_or_fov_writes\":0,\"server_purchase_rpc_intercepted\":false,\"playable_verified\":false",
        b(state.audioObserved),b(state.audioWaiting),b(state.audioOwnListenerVerified),state.foreignListenerCount,b(state.streakObserved),b(state.topbarExpected),b(state.topbarReady),shop.stage,b(shop.initialized),b(shop.openKnown),b(shop.open),b(shop.nativePurchaseRPCVerified),b(shop.cacheSupplied),b(shop.screenSupplied),b(shop.shoppingEnabled),shop.money,shop.modelCount,static_cast<unsigned long long>(shop.refreshCalls),state.weapon.weapon,b(state.weapon.originalOwner),b(state.weapon.recoilQueried),state.weapon.pitchRecoil,state.weapon.yawRecoil,state.weapon.firingError,b(state.weapon.ammoQueried),state.weapon.currentAmmo,state.weapon.predictedAmmo,state.weapon.authorityAmmo,static_cast<unsigned long long>(state.exceptionRva));
    std::fprintf(f,",\"hud_attempted\":%s,\"hud_prerequisite_mask\":%u,\"hud_required_mask\":%u,\"hud_stage\":\"%s\",\"hud_error\":%u,\"hud_installed\":%s,\"hud_player_model_bound\":%s,\"hud_roots_present\":%s,\"hud_first_person_ps_bound\":%s,\"hud_first_person_character_bound\":%s,\"hud_model_gs_bound\":%s,\"hud_model_pc_bound\":%s,\"hud_element_count\":%d,\"ps_spawned_character\":\"%p\",\"ps_possessed_character\":\"%p\",\"hud_model\":\"%p\",\"hud_first_person_model\":\"%p\",\"hud_model_controller\":\"%p\",\"hud_model_game_state\":\"%p\",\"hud_first_person_state\":\"%p\",\"hud_first_person_character\":\"%p\",\"hud_first_person_widget\":\"%p\",\"hud_minimap_widget\":\"%p\",\"hud_ability_widget\":\"%p\",\"hud_ability_parent\":\"%p\"",
        b(state.hudAttempted),state.hudPrerequisiteMask,state.hudRequiredMask,state.hudStage,state.hudError,b(state.hudInstalled),b(state.hudPlayerModelBound),b(state.hudRootObjectsPresent),b(state.hudFirstPersonPSBound),b(state.hudFirstPersonCharacterBound),b(state.hudModelGSBound),b(state.hudModelPCBound),state.hudElementCount,state.psSpawnedCharacter,state.psPossessedCharacter,state.hudModel,state.hudFirstPersonModel,state.hudModelController,state.hudModelGameState,state.hudFirstPersonState,state.hudFirstPersonCharacter,state.hudFirstPersonWidget,state.hudMinimapWidget,state.hudAbilityWidget,state.hudAbilityParent);
    std::fprintf(f,",\"streak_current_spectated_widgets\":%u,\"streak_known_spectated_cache_widgets\":%u",original_streak_hud::State().spectatedWidgets,original_streak_hud::State().cachedSpectatedWidgets);
    std::fprintf(f,",\"streak_metadata_bound\":%s,\"streak_metadata_generation\":%u,\"streak_metadata_refreshes\":%u,\"streak_metadata_expirations\":%u,\"streak_expired_metadata_role\":\"%s\",\"streak_retired_samples\":%llu,\"streak_original_exception_rva\":%llu,\"streak_original_exception_address\":%llu,\"streak_metadata_gc_root_changes\":0,\"shop_ready_scope\":\"original_graph_only\",\"shop_can_open_abi_valid\":%s,\"shop_can_open_queried\":%s,\"shop_native_can_open\":%s,\"shop_coordinator_cached_can_open\":%s,\"shop_screen_native_open\":%s,\"shop_model_groups\":%d,\"shop_observed_models\":%d,\"shop_store_purchasables\":%d,\"shop_toggle_api_calls\":%llu,\"native_input_ready_scope\":\"original_object_links_only\"",
        b(state.streakMetadataBound),state.streakMetadataGeneration,state.streakMetadataRefreshes,state.streakMetadataExpirations,state.streakExpiredMetadataRole,static_cast<unsigned long long>(state.streakRetiredSamples),static_cast<unsigned long long>(state.streakInnerExceptionRva),static_cast<unsigned long long>(state.streakInnerExceptionRva?state.imageBase+state.streakInnerExceptionRva:0),b(shop.canOpenABIValid),b(shop.canOpenQueried),b(shop.nativeCanOpen),b(shop.coordinatorCanOpen),b(shop.screenNativeOpen),shop.modelGroupCount,shop.observedModelCount,shop.storePurchasableCount,static_cast<unsigned long long>(shop.toggles));
    const auto feedback=local_server_feedback_view::Json();std::fprintf(f,",\"kill_display\":%s",feedback.c_str());
    WriteShopOpenPath(f);
    const auto& ui=original_ui_input_mode::State();
    local_server_menu_input::Write(f);
    std::fprintf(f,",\"ui_mode_edges\":%u,\"ui_original_mode_dispatches\":%u,\"ui_viewport_mode_dispatches\":%u,\"ui_focus_dispatches\":%u,\"ui_lock_scope_active\":%s,\"ui_input_mode_lock\":%u,\"ui_native_ares_mode\":%u,\"ui_buy_edges\":%u,\"ui_escape_edges\":%u,\"ui_dismiss_fallbacks\":%u,\"ui_shop_open\":%s,\"ui_options_open\":%s,\"ui_map_open\":%s",
        ui.modeEdges,ui.originalModeDispatches,ui.viewportModeDispatches,ui.originalFocusDispatches,b(ui.inputLockScopeActive),unsigned(ui.inputLockNow),unsigned(ui.nativeAresMode),ui.buyEdges,ui.escapeEdges,ui.dismissFallbacks,b(ui.shopOpen),b(ui.optionsOpen),b(ui.mapOpen));
    std::fprintf(f,",\"camera_stage\":\"%s\",\"camera_owner_matches\":%s,\"camera_view_target_matches\":%s,\"camera_locally_controlled\":%s,\"camera_owner\":\"%p\",\"camera_view_target\":\"%p\",\"camera_pending_target\":\"%p\",\"view_target_component\":\"%p\",\"view_target_info_actor\":\"%p\",\"view_target_local_actor\":\"%p\",\"view_target_mode\":%u,\"view_target_default_mode\":%u,\"view_target_ready\":%u,\"view_target_allowed_modes\":%u,\"player_input\":\"%p\",\"pc_input_component\":\"%p\",\"pawn_input_component\":\"%p\",\"last_fault_operation\":\"%s\",\"last_fault_delta\":%llu,\"faults_before_initialize\":%llu,\"faults_observed\":%llu,\"image_base\":%llu,\"presentation_module_base\":%llu,\"fault_delta_is_temporal_correlation_only\":true,\"probe_observations\":[",
        state.cameraStage,b(state.cameraOwnerMatches),b(state.cameraViewTargetMatches),b(state.cameraLocallyControlled),state.cameraOwner,state.cameraViewTarget,state.cameraPendingTarget,state.viewTargetComponent,state.viewTargetInfoActor,state.viewTargetLocalActor,unsigned(state.viewTargetMode),unsigned(state.viewTargetDefaultMode),unsigned(state.viewTargetReady),unsigned(state.viewTargetAllowedModes),state.playerInput,state.pcInputComponent,state.pawnInputComponent,state.lastFaultOperation,static_cast<unsigned long long>(state.lastFaultDelta),static_cast<unsigned long long>(state.faultsBeforeInitialize),static_cast<unsigned long long>(state.faultsObserved),static_cast<unsigned long long>(state.imageBase),static_cast<unsigned long long>(state.moduleBase));
    for(size_t i=0;i<size_t(Probe::Count);++i){if(i)std::fputc(',',f);const auto& p=state.probes[i];
        std::fprintf(f,"{\"operation\":\"%s\",\"calls\":%llu,\"fault_delta\":%llu,\"last_fault_delta\":%llu}",ProbeNames[i],static_cast<unsigned long long>(p.calls),static_cast<unsigned long long>(p.faultDelta),static_cast<unsigned long long>(p.lastFaultDelta));}
    std::fputs("]}",f);
}
inline bool WriteDiagnostics(const wchar_t* path){FILE* f=nullptr;if(!path||_wfopen_s(&f,path,L"wb")||!f)return false;
    WriteState(f);const bool ok=std::ferror(f)==0;const bool closed=std::fclose(f)==0;return ok&&closed;}
} // namespace local_server_client
