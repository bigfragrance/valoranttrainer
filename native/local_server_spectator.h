#pragma once
#include "bot_spectator_contract.h"
#include "bot_spectator_order.h"
#include "local_server_match.h"
#include "local_server_combat.h"
#include "local_server_viewtarget.h"
#include "local_server_zoom_authority.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

// Explicit offline-server observation policy. Mode changes and target selection
// use the complete original APIs; the native mouse ActionComponents keep their
// own order, conditions, input consumption and rate limiter. Only the missing
// authority logical-actor publication is supplemented, like watch-possessed.
namespace local_server_spectator {
using Request=bot_spectator_contract::Request;
using Result=training_control::Result;
using Player=local_server_match::AuthorityPlayer;
struct State {
    bool enabled=false,returnPending=false,published=false,selectionPending=false,selectionFailed=false;
    bool nativeAckKnown=false,byRequestAttempted=false,pendingNativeRequest=false,pendingSerialKnown=false;
    int32_t pendingSerial=0;void* pendingAction=nullptr;
    original_inventory_slots::WeakIdentity pendingActionIdentity{};
    int32_t nativeRequestIndex=0,nativeAckIndex=0;
    uint64_t selectionStartedMs=0,lastNativeRequestMs=0,nativeFindRequests=0,nativeStepRequests=0;
    void* targetBeforeRequest=nullptr;
    uint32_t requestedIndex=0,selectedIndex=0;int32_t nativePlayerId=-1;
    int32_t logicalRosterIndex=-1,localRosterIndex=-1;
    uint8_t observedMode=255,defaultMode=255,allowedModes=0;
    bool serverPolicyInstalled=false,serverPolicyProfiles=false,targetQueryKnown=false;
    bool serverPolicyRetired=false;
    bool userCancelledAdmin=false;
    uint32_t targetQueryCount=0;int32_t lastDirection=-1;
    uint64_t originalServerCalls=0,serverSelections=0,playerInputRequests=0;
    bool targetPoseKnown=false;uint64_t targetPoseMs=0;
    int32_t targetPosePlayerId=-1;uint32_t targetPoseEpoch=0;
    bool observerProbeKnown=false,observerHumanExcluded=false,damageToHumanAllowed=false,damageFromHumanAllowed=false;
    int32_t observerProbePeer=-1;
    uint64_t observerProbeMs=0;
    engine::Vec targetEyes{},targetViewRotation{},targetFacing{},targetControlRotation{},observerTargetRotation{};
    float targetPawnFov=0;
    uint64_t requests=0,modeCalls=0,targetCalls=0,publications=0,nativeTargetChanges=0,respawnFollows=0;
    void *component=nullptr,*target=nullptr,*targetState=nullptr;
    original_inventory_slots::WeakIdentity targetIdentity{};
    const char* stage="native_spectator_authority_off";
};
inline State state{},published{};
inline bool programmedOrderRequest=false;
inline SRWLOCK publishedLock=SRWLOCK_INIT;
inline void PublishSnapshot(){AcquireSRWLockExclusive(&publishedLock);published=state;ReleaseSRWLockExclusive(&publishedLock);}
inline State Snapshot(){AcquireSRWLockShared(&publishedLock);const auto value=published;ReleaseSRWLockShared(&publishedLock);return value;}
inline const State& GetState(){return state;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Live(void* object,const original_inventory_slots::WeakIdentity& id){return object&&original_inventory_slots::ResolveIdentity(id)==object;}
inline bool Event(void* object,const wchar_t* path,uint16_t bytes,void* params,uintptr_t exec=0){
    auto method=engine::Find(path);return object&&method&&Read<uint16_t>(method,0xbe)==bytes
        &&original_players::Derived(Read<void*>(object,0x10),Read<void*>(method,0x28))
        &&(!exec||Read<uintptr_t>(method,0xe0)==engine::image+exec)&&engine::Event(object,path,params);
}
inline const Player* Human(){auto value=local_server_match::Player(0);return value&&value->human?value:nullptr;}
inline bool Context(){
    const auto human=Human();const auto& d=local_server_viewtarget::GetState();const auto& ids=local_server_viewtarget::binding;
    if(!original_players::Thread()||!human||!d.bound||d.error||!d.ownerVerified||!d.profilesVerified||!d.metadataVerified||!d.replicationRouteVerified
       ||d.world!=local_server_match::GetState().world||d.controller!=human->pc||d.playerState!=human->ps
       ||!Live(d.world,ids.world)||!Live(d.controller,ids.controller)||!Live(d.playerState,ids.playerState)
       ||!Live(d.driver,ids.driver)||!Live(d.connection,ids.connection)||!Live(d.component,ids.component)
       ||Read<void*>(d.world,0x40)!=d.driver||Read<void*>(d.controller,0x510)!=d.connection
       ||Read<void*>(d.controller,0xc60)!=d.component||Read<void*>(d.component,0xc8)!=d.controller
       ||Read<void*>(d.connection,0x38)!=d.controller||Read<void*>(d.connection,0xa0)!=d.controller
       ||Read<uint8_t>(d.connection,0x13c)!=3||Read<uint8_t>(d.connection,0x1d8)!=3)return false;
    state.component=d.component;return true;
}
inline bool Current(const Player& player,bool allowHuman);
inline void ObserveNativeTargets(){
    state.logicalRosterIndex=state.localRosterIndex=-1;
    state.observerProbeKnown=false;state.observerProbePeer=-1;
    if(!Live(state.component,local_server_viewtarget::binding.component))return;
    state.observedMode=Read<uint8_t>(state.component,0x178);
    state.defaultMode=Read<uint8_t>(state.component,0x16b);
    state.allowedModes=Read<uint8_t>(state.component,0x16a);
    const auto logical=Read<void*>(state.component,0x170),local=Read<void*>(state.component,0x180);
    for(const auto& player:local_server_match::AuthorityPlayers()){
        if(!player.present||!Live(player.pawn,player.pawnIdentity))continue;
        if(player.pawn==logical)state.logicalRosterIndex=static_cast<int32_t>(player.index);
        if(player.pawn==local)state.localRosterIndex=static_cast<int32_t>(player.index);
    }
    const auto human=Human();
    if(human&&Current(*human,true)&&local_server_match::Active()){
        for(const auto& player:local_server_match::AuthorityPlayers())if(Current(player,false)){
            state.observerProbeKnown=true;state.observerHumanExcluded=local_server_match::IsObserver(0);
            state.observerProbeMs=GetTickCount64();
            state.observerProbePeer=static_cast<int32_t>(player.index);
            state.damageToHumanAllowed=local_server_match::CanDamage(player.index,0);
            state.damageFromHumanAllowed=local_server_match::CanDamage(0,player.index);break;
        }
    }
    const auto now=GetTickCount64();
    if(!state.enabled||!state.serverPolicyProfiles||now-state.targetPoseMs<100)return;
    state.targetPoseKnown=false;state.targetPoseMs=now;
    for(const auto& player:local_server_match::AuthorityPlayers())if(player.pawn==local&&Current(player,false)){
        struct Eyes{engine::Vec position{},rotation{};} query{};
        if(Event(player.pawn,L"/Script/Engine.Actor.GetActorEyesViewPoint",48,&query)
           &&original_players::Finite(query.position)&&original_players::Finite(query.rotation)){
            state.targetPoseKnown=true;state.targetEyes=query.position;state.targetViewRotation=query.rotation;
            state.targetPosePlayerId=player.playerId;state.targetPoseEpoch=player.epoch;
            state.targetFacing=Read<engine::Vec>(player.pawn,0x1158);state.targetControlRotation=Read<engine::Vec>(player.pc,0x4e8);
            state.observerTargetRotation=Read<engine::Vec>(Human()->pc,0x540);
            state.targetPawnFov=engine::function<float(*)(void*)>(0x7880050)(player.pawn);
        }
        break;
    }
}
inline bool Current(const Player& player,bool allowHuman=false){
    return player.present&&player.ready&&player.alive&&(allowHuman||!player.human)
        &&Live(player.pawn,player.pawnIdentity)&&Live(player.ps,player.psIdentity)&&Live(player.pc,player.pcIdentity)
        &&Read<void*>(player.pawn,0x488)==player.ps&&Read<void*>(player.pawn,0x4a0)==player.pc
        &&Read<void*>(player.ps,0xbc8)==player.pawn&&Read<void*>(player.ps,0xbd0)==player.pawn;
}
inline bool Mode(uint8_t value){
    if(Read<uint8_t>(state.component,0x178)==value)return true;
    ++state.modeCalls;
    return Event(state.component,L"/Script/ShooterGame.ViewTargetComponent.AuthSetMode",1,&value,0x6dd78b0)
        &&Read<uint8_t>(state.component,0x178)==value;
}
inline bool Eligible(const Player& player,uint8_t mode,bool allowHuman=false){
    return Current(player,allowHuman)&&(Read<uint8_t>(state.component,0x16a)&mode)==mode
        &&engine::function<bool(*)(void*,void*,uint8_t)>(0x785a040)(state.component,player.pawn,mode);
}
inline void* InOrder(){
    const auto human=Human();auto component=human?Read<void*>(human->pc,0xc70):nullptr;
    original_inventory_slots::WeakIdentity id{};
    return component&&original_inventory_slots::CaptureIdentity(component,id)&&Live(component,id)
        &&original_players::IsA(component,0x6d0fa80)&&Read<void*>(component,0xc8)==human->pc?component:nullptr;
}
inline void ReadNativeAck(void* action=nullptr){
    state.nativeAckKnown=false;const auto component=action?action:InOrder();const auto transition=Read<void*>(component,0xe8);
    original_inventory_slots::WeakIdentity id{};
    if(!transition||!original_inventory_slots::CaptureIdentity(transition,id)||!Live(transition,id)
       ||!local_server_match::Property(Read<void*>(transition,0x10),L"AuthSpectateRequestAck",4,0x114))return;
    state.nativeRequestIndex=Read<int32_t>(transition,0x110);state.nativeAckIndex=Read<int32_t>(transition,0x114);state.nativeAckKnown=true;
}
inline bool NativeSettled(){
    if(state.pendingNativeRequest){
        if(!Live(state.pendingAction,state.pendingActionIdentity)||!state.pendingSerialKnown)return false;
        ReadNativeAck(state.pendingAction);
        if(!state.nativeAckKnown||state.nativeAckIndex<state.pendingSerial||state.nativeAckIndex<state.nativeRequestIndex)return false;
        state.pendingNativeRequest=false;state.pendingAction=nullptr;
    }
    ReadNativeAck();return state.nativeAckKnown&&state.nativeAckIndex>=state.nativeRequestIndex;
}
inline void RecordNativeRequest(void* action,int32_t before,bool called){
    if(!called)return;ReadNativeAck(action);
    if(!state.nativeAckKnown||state.nativeRequestIndex!=before){
        state.pendingNativeRequest=true;state.pendingAction=action;
        state.pendingSerialKnown=state.nativeAckKnown&&original_inventory_slots::CaptureIdentity(action,state.pendingActionIdentity);
        state.pendingSerial=state.nativeRequestIndex;
    }
}
inline bool FindOriginal(){
    ++state.nativeFindRequests;state.lastNativeRequestMs=GetTickCount64();state.targetBeforeRequest=Read<void*>(state.component,0x180);
    const auto human=Human();const bool previous=programmedOrderRequest;programmedOrderRequest=true;
    const bool called=human&&Event(human->pc,L"/Script/ShooterGame.AresPlayerController.AuthFindValidSpectateTarget",0,nullptr,0x64e8490);
    programmedOrderRequest=previous;
    ReadNativeAck();return called;
}
// The shipped InOrder server implementation ACKs, then calls a complete
// replay-only selector which returns null in a normal IP world. Supply the
// missing local-server policy only on this admitted owner's existing component.
// Client bindings, native rate/request/RPC, original ACK/delegates, target
// eligibility and the original target setter all retain their complete paths.
struct OrderBinding {
    void* component=nullptr;original_inventory_slots::WeakIdentity identity{};
    local_server_combat::firearms::Vtable table{};
};
inline OrderBinding orderBinding{};
using OrderImplementation=void(*)(void*,uint8_t,bool,int32_t);
inline bool PolicyProfiles(){
    struct Profile{uintptr_t rva;size_t bytes;uint64_t fnv;};
    constexpr Profile profiles[]{
        {0x7858680,174,0x4cb660af1de281d9ULL},
        {0x785ac40,888,0x7a6f549d5a3beb1eULL},
        {0x785a790,235,0xbf07bea51ca9310aULL},
        {0x1915720,59,0x9330f93033e42badULL},
        {0x1cdc710,311,0xbe3c726fa4fbfdcdULL},
        {0x64e8b60,360,0xfd2ea2bf411f1430ULL},
        {0x7880050,120,0xb2fa49567e1d26ddULL},
    };
    for(const auto& p:profiles){
        const auto data=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(data,p.bytes))return false;
        uint64_t fnv=14695981039346656037ULL;
        for(size_t i=0;i<p.bytes;++i)fnv=(fnv^data[i])*1099511628211ULL;
        if(fnv!=p.fnv)return false;
    }
    return true;
}
inline const Player* PlayerForTarget(void* target){
    for(const auto& player:local_server_match::AuthorityPlayers())
        if(player.pawn==target&&Current(player))return &player;
    return nullptr;
}
inline bool TargetQuerySchema(void* method){
    original_inventory_slots::WeakIdentity identity{};
    if(!method||!original_inventory_slots::CaptureIdentity(method,identity)||!Live(method,identity)
       ||Read<uint8_t>(method,0xbc)!=2||Read<uint16_t>(method,0xbe)!=24||Read<uint16_t>(method,0xc0)!=8)return false;
    const auto mode=local_server_match::Property(method,L"Mode",1,0);
    const auto result=local_server_match::Property(method,L"ReturnValue",16,8);
    const auto inner=Read<void*>(result,0x80);
    return mode&&result&&(Read<uint64_t>(mode,0x38)&0x80)==0x80
        &&(Read<uint64_t>(result,0x38)&0x580)==0x580
        &&Read<int32_t>(inner,0x34)==8&&Read<int32_t>(inner,0x30)==1
        &&Read<void*>(inner,0x78)==engine::Class(original_players::ShooterCharacterClass);
}
inline bool NativeTargets(void* pc,local_server_viewtarget::Array& out){
    struct Query{uint8_t mode=4,pad[7]{};local_server_viewtarget::Array result{};} query{};
    static_assert(sizeof(Query)==24);
    auto base=engine::Find(L"/Script/ShooterGame.AresPlayerController.GetShooterCharacterSpectateTargets");
    if(!base||!TargetQuerySchema(base)||Read<uint32_t>(base,0xb8)!=0x8020c00
       ||Read<uintptr_t>(base,0xe0)!=engine::image+0x64e8b60)return false;
    auto method=engine::function<void*(*)(void*,const void*)>(0x1cdc710)(pc,static_cast<uint8_t*>(base)+0x18);
    if(!TargetQuerySchema(method)
       ||!original_players::Readable(static_cast<uint8_t*>(method)+0x18,12)
       ||std::memcmp(static_cast<uint8_t*>(method)+0x18,static_cast<uint8_t*>(base)+0x18,12)!=0
       ||!original_players::Derived(Read<void*>(pc,0x10),Read<void*>(method,0x28)))return false;
    const auto process=Read<void*>(Read<void*>(pc),0x298);
    if(!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(pc,method,&query);
    out=query.result;
    return local_server_viewtarget::ValidArray(out,8,4096);
}
inline bool Publish(const Player& player,uint8_t mode,bool allowHuman);
inline void LocalOrderImplementation(void* component,uint8_t direction,bool fromPlayerInput,int32_t serial){
    const auto& binding=orderBinding;
    const auto parent=binding.table.previous?reinterpret_cast<OrderImplementation>(binding.table.previous[0x4d8/8]):nullptr;
    if(!parent)return;
    const bool ours=component==binding.component&&Live(component,binding.identity)
        &&Read<void**>(component)==binding.table.Address();
    const auto before=ours?Read<void*>(state.component,0x180):nullptr;
    parent(component,direction,fromPlayerInput,serial);
    if(!ours)return;
    ++state.originalServerCalls;
    if(!state.enabled||state.returnPending||direction>1||!Context()
       ||component!=InOrder()||!Live(component,binding.identity)||Read<void**>(component)!=binding.table.Address()
       ||Read<void*>(local_server_match::GetState().world,0xf8)
       ||Read<uint8_t>(state.component,0x178)!=bot_spectator_contract::SpectateEveryone)return;
    state.lastDirection=direction;
    if(fromPlayerInput&&!programmedOrderRequest){
        ++state.playerInputRequests;state.selectionPending=state.selectionFailed=false;
        state.byRequestAttempted=true;state.userCancelledAdmin=true;
    }
    const auto after=Read<void*>(state.component,0x180);
    const auto nativeSelected=PlayerForTarget(after);
    if(after!=before&&nativeSelected&&Eligible(*nativeSelected,4)){Publish(*nativeSelected,4,false);return;}
    // Several normal RPCs can arrive in one host frame. Advance only from a
    // logical target already synchronized with the preceding native selection.
    if(nativeSelected&&after!=Read<void*>(state.component,0x170)&&!Publish(*nativeSelected,4,false))return;
    const Player* selected=nullptr;
    const auto forced=Read<void*>(state.component,0x188);
    if(forced){const auto value=PlayerForTarget(forced);if(value&&Eligible(*value,4))selected=value;}
    local_server_viewtarget::Array targets{};
    if(!selected){
        state.targetQueryKnown=NativeTargets(Human()->pc,targets);
        struct Release{void* data;~Release(){if(data)engine::function<void(*)(void*)>(0x1915720)(data);}} release{targets.data};
        state.targetQueryCount=state.targetQueryKnown?static_cast<uint32_t>(targets.count):0;
        if(!state.targetQueryKnown)return;
        auto values=reinterpret_cast<void**>(targets.data);
        const auto index=bot_spectator_order::Select(values,targets.count,Read<void*>(state.component,0x170),direction,
            [](void* target){const auto player=PlayerForTarget(target);return player&&Eligible(*player,4);});
        if(index>=0)selected=PlayerForTarget(values[index]);
    }
    if(!selected||!Eligible(*selected,4))return;
    engine::function<void(*)(void*,void*)>(0x785a790)(state.component,selected->pawn);
    if(Read<void*>(state.component,0x180)==selected->pawn&&Publish(*selected,4,false)){
        ++state.serverSelections;state.stage="native_spectator_local_server_original_target_selected";
        if(fromPlayerInput&&!programmedOrderRequest&&state.requestedIndex!=selected->index){
            state.requestedIndex=static_cast<uint32_t>(selected->index);++state.nativeTargetChanges;
        }
    }
}
inline bool BindOrderPolicy(){
    if(state.serverPolicyRetired)return false;
    const auto component=InOrder();if(!component)return false;
    if(orderBinding.table.installed){
        if(!Live(orderBinding.component,orderBinding.identity)){
            orderBinding.table.installed=false;state.serverPolicyInstalled=false;state.serverPolicyRetired=true;
            return false; // one admitted owner per owned process; retain the old table storage
        }
        state.serverPolicyInstalled=component==orderBinding.component&&Live(component,orderBinding.identity)
            &&Read<void**>(component)==orderBinding.table.Address();return state.serverPolicyInstalled;
    }
    if(!PolicyProfiles()){state.serverPolicyProfiles=false;return false;}
    state.serverPolicyProfiles=true;
    if(Read<void*>(Read<void*>(component),0x4d8)!=reinterpret_cast<void*>(engine::image+0x7858680)
       ||!original_inventory_slots::CaptureIdentity(component,orderBinding.identity)
       ||!local_server_combat::firearms::CopyVtable(component,orderBinding.table,0x4d8/8+1))return false;
    orderBinding.component=component;
    state.serverPolicyInstalled=local_server_combat::firearms::Install(component,orderBinding.table,0x4d8/8,
        reinterpret_cast<void*>(&LocalOrderImplementation));return state.serverPolicyInstalled;
}
inline bool RestoreOrderPolicy(){
    if(!orderBinding.table.installed){state.serverPolicyInstalled=false;return true;}
    if(!Live(orderBinding.component,orderBinding.identity)){orderBinding.table.installed=false;state.serverPolicyInstalled=false;return true;}
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(orderBinding.component),orderBinding.table.previous,
       orderBinding.table.Address())!=orderBinding.table.Address())return false;
    orderBinding.table.installed=false;state.serverPolicyInstalled=false;return true;
}
inline bool Close(){
    if(!orderBinding.table.installed&&!state.enabled&&!state.returnPending)return true;
    if(!original_players::Thread())return false;
    state.enabled=state.returnPending=state.selectionPending=false;
    const bool restored=RestoreOrderPolicy();
    state.stage=restored?"native_spectator_owned_order_policy_closed":"native_spectator_owned_order_policy_close_conflict";
    PublishSnapshot();return restored;
}
inline bool Publish(const Player& player,uint8_t mode,bool allowHuman=false){
    if(mode==bot_spectator_contract::SpectateEveryone&&!NativeSettled())return false;
    if(!Eligible(player,mode,allowHuman)||Read<void*>(state.component,0x180)!=player.pawn)return false;
    if(Read<void*>(state.component,0x170)!=player.pawn){
        if(!original_players::Write(state.component,0x170,player.pawn))return false;++state.publications;
    }
    state.target=player.pawn;state.targetState=player.ps;state.targetIdentity=player.pawnIdentity;
    state.selectedIndex=static_cast<uint32_t>(player.index);state.nativePlayerId=player.playerId;
    state.published=Read<void*>(state.component,0x170)==player.pawn&&Read<uint8_t>(state.component,0x178)==mode;
    return state.published;
}
inline bool ReturnOwn(){
    const auto human=Human();
    if(state.pendingNativeRequest&&!NativeSettled()){state.stage="native_spectator_waiting_original_ack_before_return";return false;}
    ReadNativeAck();if(state.nativeAckKnown&&state.nativeAckIndex<state.nativeRequestIndex){state.stage="native_spectator_waiting_original_ack_before_return";return false;}
    if(!human||!Current(*human,true)){state.stage="native_spectator_waiting_own_respawn_before_return";return false;}
    if(Read<uint8_t>(state.component,0x178)==bot_spectator_contract::WatchPossessed
       &&Read<void*>(state.component,0x180)!=human->pawn)Mode(bot_spectator_contract::SpectateEveryone);
    if(!Mode(bot_spectator_contract::WatchPossessed)||!Eligible(*human,bot_spectator_contract::WatchPossessed,true)
       ||Read<void*>(state.component,0x180)!=human->pawn||!Publish(*human,bot_spectator_contract::WatchPossessed,true)){
        // Keep the default mode1 publisher away from a foreign logical target.
        Mode(bot_spectator_contract::SpectateEveryone);
        state.stage="native_spectator_waiting_original_own_target_policy";return false;
    }
    if(!RestoreOrderPolicy()){state.stage="native_spectator_waiting_owned_order_policy_restore";return false;}
    state.enabled=state.returnPending=state.selectionPending=false;state.selectionFailed=false;state.requestedIndex=0;state.stage="native_spectator_own_target_published_waiting_client";return true;
}
inline bool ProgressSelection(){
    const auto wanted=local_server_match::Player(state.requestedIndex);if(!wanted||!Current(*wanted)){
        state.stage="native_spectator_waiting_requested_respawn";return false;
    }
    if(GetTickCount64()-state.selectionStartedMs>15000){state.selectionPending=false;state.selectionFailed=true;state.stage="native_spectator_original_selection_timeout";return false;}
    if(!NativeSettled()){state.stage="native_spectator_waiting_original_ack_or_transition_metadata";return false;}
    if(Read<void*>(state.component,0x180)==wanted->pawn&&Publish(*wanted,bot_spectator_contract::SpectateEveryone)){
        state.selectionPending=state.selectionFailed=false;state.stage="native_spectator_requested_target_published_waiting_client";return true;
    }
    const auto now=GetTickCount64();
    if(now-state.selectionStartedMs>15000){state.selectionPending=false;state.selectionFailed=true;state.stage="native_spectator_original_selection_timeout";return false;}
    if(!Eligible(*wanted,bot_spectator_contract::SpectateEveryone)){
        state.stage="native_spectator_waiting_requested_target_eligibility";return false;
    }
    if(now-state.lastNativeRequestMs<250)return false;
    if(!NativeSettled()){state.stage="native_spectator_waiting_original_transition_ack";return false;}
    // Admin selection reuses the authority ACK; it must not allocate a second
    // request sequence in the host's copy of the client's ScreenTransition.
    state.stage=FindOriginal()?"native_spectator_original_find_advance_requested":"native_spectator_original_find_unavailable";return false;
}
inline Result HandleImpl(const Request& request){
    ++state.requests;if(!bot_spectator_contract::Valid(request))return Result::Invalid;
    if(!Context()){state.stage="native_spectator_waiting_verified_authority_owner";return Result::Unavailable;}
    if(!request.enabled){
        if(!state.enabled&&!state.returnPending&&Read<uint8_t>(state.component,0x178)==1){state.stage="native_spectator_already_watch_possessed";return Result::Applied;}
        state.selectionPending=state.selectionFailed=false;state.returnPending=true;state.stage="native_spectator_return_requested";
        return ReturnOwn()?Result::Applied:Result::Pending;
    }
    const auto player=local_server_match::Player(request.rosterIndex);
    if(!player||player->human||player->playerId!=request.nativePlayerId){state.stage="native_spectator_selected_identity_no_longer_matches";return Result::Invalid;}
    if(!BindOrderPolicy()){state.stage="native_spectator_waiting_owned_order_server_policy";return Result::Unavailable;}
    const bool alreadyObserving=state.enabled;
    if(!Mode(bot_spectator_contract::SpectateEveryone)){state.stage="native_spectator_original_mode_not_available";return Result::Unavailable;}
    state.enabled=true;state.returnPending=false;state.requestedIndex=request.rosterIndex;
    state.userCancelledAdmin=false;
    state.selectionPending=true;state.selectionFailed=false;state.byRequestAttempted=false;
    state.stage="native_spectator_requested_waiting_original_selection";
    state.selectionStartedMs=GetTickCount64();state.lastNativeRequestMs=0;state.targetBeforeRequest=nullptr;
    if(!Current(*player)){state.stage="native_spectator_waiting_requested_respawn";return Result::Pending;}
    if(Read<void*>(state.component,0x180)!=player->pawn&&!state.pendingNativeRequest
       &&(!alreadyObserving||NativeSettled()))FindOriginal();
    return ProgressSelection()?Result::Applied:Result::Pending;
}
inline Result Handle(const Request& request){const auto result=HandleImpl(request);ObserveNativeTargets();PublishSnapshot();return result;}
inline bool OwnsAuthorityPolicy(){return state.enabled||state.returnPending;}
inline void PollImpl(){
    if(!OwnsAuthorityPolicy()||!Context())return;
    if(state.returnPending){ReturnOwn();return;}
    if(!Mode(bot_spectator_contract::SpectateEveryone)){state.stage="native_spectator_waiting_original_mode";return;}
    const auto nativeTarget=Read<void*>(state.component,0x180);bool nativeValid=false;
    const auto viewed=PlayerForTarget(nativeTarget);
    if(viewed&&local_server_zoom_authority::Publish(*viewed)==local_server_zoom_authority::Result::Failed){
        state.stage="native_spectator_zoom_authority_publication_failed";
        local_server_match::Fail(9038,state.stage);return;
    }
    for(const auto& player:local_server_match::AuthorityPlayers()){
        if(player.present&&!player.human&&player.pawn==nativeTarget&&Current(player)){
            if(!Publish(player,bot_spectator_contract::SpectateEveryone)){
                state.published=false;state.stage="native_spectator_waiting_original_selected_target_policy";
                if(state.selectionPending)ProgressSelection();return;
            }
            nativeValid=true;
            if(!state.selectionPending){
                if(state.requestedIndex!=player.index){state.requestedIndex=static_cast<uint32_t>(player.index);++state.nativeTargetChanges;}
                state.stage="native_spectator_following_original_selected_target";
            }
            break;
        }
    }
    if(state.selectionPending){ProgressSelection();return;}
    if(nativeValid||state.selectionFailed)return;
    if(state.userCancelledAdmin){
        // Follow only a target genuinely selected before the user action. A
        // failed manual cycle must never revive the cancelled admin goal.
        const auto previous=local_server_match::Player(state.selectedIndex);
        if(!state.selectedIndex||!previous||!Current(*previous)
           ||!Eligible(*previous,bot_spectator_contract::SpectateEveryone)){
            state.stage="native_spectator_waiting_original_selected_target_after_manual_action";return;
        }
        state.requestedIndex=state.selectedIndex;
    }
    const auto selected=local_server_match::Player(state.requestedIndex);
    if(!selected||!Current(*selected)){state.stage="native_spectator_waiting_selected_respawn";return;}
    state.selectionPending=true;state.byRequestAttempted=false;state.selectionStartedMs=GetTickCount64();
    ++state.respawnFollows;if(NativeSettled())FindOriginal();ProgressSelection();
}
inline void Poll(){PollImpl();ObserveNativeTargets();PublishSnapshot();}
inline std::string Json(){
    const auto value=Snapshot();char text[2600]{};const auto b=[](bool v){return v?"true":"false";};
    std::snprintf(text,sizeof(text),"{\"enabled\":%s,\"return_pending\":%s,\"selection_pending\":%s,\"selection_failed\":%s,\"requested_index\":%u,\"selected_index\":%u,\"native_player_id\":%d,\"published\":%s,\"stage\":\"%s\",\"requests\":%llu,\"original_mode_calls\":%llu,\"original_by_request_calls\":%llu,\"original_find_requests\":%llu,\"original_next_requests\":%llu,\"native_ack_known\":%s,\"pending_native_request\":%s,\"pending_serial_known\":%s,\"pending_serial\":%d,\"native_request_index\":%d,\"native_ack_index\":%d,\"logical_target_publications\":%llu,\"native_target_changes\":%llu,\"respawn_follows\":%llu,\"client_execution_verified\":false,\"camera_updates\":0,\"possession_changes\":0}",
        b(value.enabled),b(value.returnPending),b(value.selectionPending),b(value.selectionFailed),value.requestedIndex,value.selectedIndex,value.nativePlayerId,b(value.published),value.stage,
        static_cast<unsigned long long>(value.requests),static_cast<unsigned long long>(value.modeCalls),static_cast<unsigned long long>(value.targetCalls),static_cast<unsigned long long>(value.nativeFindRequests),static_cast<unsigned long long>(value.nativeStepRequests),b(value.nativeAckKnown),b(value.pendingNativeRequest),b(value.pendingSerialKnown),value.pendingSerial,value.nativeRequestIndex,value.nativeAckIndex,
        static_cast<unsigned long long>(value.publications),static_cast<unsigned long long>(value.nativeTargetChanges),static_cast<unsigned long long>(value.respawnFollows));
    std::string result=text;if(!result.empty()&&result.back()=='}')result.pop_back();
    char targets[700]{};std::snprintf(targets,sizeof(targets),",\"observed_mode\":%u,\"default_mode\":%u,\"allowed_modes\":%u,\"logical_roster_index\":%d,\"local_roster_index\":%d,\"server_policy_installed\":%s,\"server_policy_profiles\":%s,\"target_query_known\":%s,\"target_query_count\":%u,\"last_direction\":%d,\"original_server_calls\":%llu,\"server_selections\":%llu,\"player_input_requests\":%llu}",
        unsigned(value.observedMode),unsigned(value.defaultMode),unsigned(value.allowedModes),value.logicalRosterIndex,value.localRosterIndex,
        b(value.serverPolicyInstalled),b(value.serverPolicyProfiles),b(value.targetQueryKnown),value.targetQueryCount,value.lastDirection,
        static_cast<unsigned long long>(value.originalServerCalls),static_cast<unsigned long long>(value.serverSelections),static_cast<unsigned long long>(value.playerInputRequests));
    result+=targets;if(!result.empty()&&result.back()=='}')result.pop_back();
    const auto finite=[](double v){return std::isfinite(v)?v:0.;};
    char pose[900]{};std::snprintf(pose,sizeof(pose),",\"target_pose_known\":%s,\"target_pose_ms\":%llu,\"target_pose_player_id\":%d,\"target_pose_epoch\":%u,\"target_eyes\":[%.9g,%.9g,%.9g],\"target_view_rotation\":[%.9g,%.9g,%.9g],\"target_facing\":[%.9g,%.9g,%.9g],\"target_control_rotation\":[%.9g,%.9g,%.9g],\"observer_target_rotation\":[%.9g,%.9g,%.9g],\"target_pawn_fov\":%.9g}",
        b(value.targetPoseKnown),static_cast<unsigned long long>(value.targetPoseMs),value.targetPosePlayerId,value.targetPoseEpoch,finite(value.targetEyes.x),finite(value.targetEyes.y),finite(value.targetEyes.z),
        finite(value.targetViewRotation.x),finite(value.targetViewRotation.y),finite(value.targetViewRotation.z),finite(value.targetFacing.x),finite(value.targetFacing.y),finite(value.targetFacing.z),
        finite(value.targetControlRotation.x),finite(value.targetControlRotation.y),finite(value.targetControlRotation.z),finite(value.observerTargetRotation.x),finite(value.observerTargetRotation.y),finite(value.observerTargetRotation.z),finite(value.targetPawnFov));
    result+=pose;if(!result.empty()&&result.back()=='}')result.pop_back();
    result+=",\"zoom_authority\":"+local_server_zoom_authority::Json();
    char observer[400]{};std::snprintf(observer,sizeof(observer),",\"observer_probe_known\":%s,\"observer_probe_ms\":%llu,\"observer_probe_fresh\":%s,\"observer_human_excluded\":%s,\"observer_probe_peer\":%d,\"damage_to_human_allowed\":%s,\"damage_from_human_allowed\":%s}",
        b(value.observerProbeKnown),static_cast<unsigned long long>(value.observerProbeMs),b(value.observerProbeMs&&GetTickCount64()-value.observerProbeMs<=500),
        b(value.observerHumanExcluded),value.observerProbePeer,b(value.damageToHumanAllowed),b(value.damageFromHumanAllowed));
    result+=observer;return result;
}
}
