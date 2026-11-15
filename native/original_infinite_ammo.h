#pragma once
#include "original_players.h"
#include <array>

// Local training control for the shipped infinite-magazine game rule. Native
// AmmoComponent::Consume6ef0e80 reads ShooterGameState.GameInfo+33 and returns
// success without consuming when enabled; all original firing/recoil/state
// processing remains in place. The shipped SetInfiniteMagazine cheat wrapper
// only parses its integer and returns, so the missing authority publication is
// one original GameInfo bool followed by original OnRep_GameInfo(old64).
// Evidence: work/duel1-original-infinite-ammo. No per-frame resource mutation.
namespace original_infinite_ammo {
inline constexpr size_t GameInfoOffset=0x7d8,GameInfoSize=64;
inline constexpr size_t InfiniteAmmoField=0x33,InfiniteReserveField=0x34;
using GameInfo=std::array<uint8_t,GameInfoSize>;
struct Diagnostics {
    bool contextBound=false,initialized=false,enabled=false,requestedEnabled=false;
    bool nativeAmmoRule=false,nativeReserveRule=false,requestedRuleMatches=false;
    bool authority=false,worldGameStateMatches=false,localAmmoOwnerValid=false;
    bool originalConsumeVerified=false,originalReplenishVerified=false;
    bool localWritePreservedOtherFields=true,reserveRulePreserved=true;
    bool notificationCalled=false,refillReadbackValid=false,restartRequired=false;
    uint32_t toggles=0,setRequests=0,ruleWrites=0,notifications=0,polls=0;
    uint32_t replenishCalls=0,ammoDropsWhileEnabled=0;
    int32_t ammo=0,maxAmmo=0,reserveAmmo=0,minimumEnabledAmmo=-1;
    int32_t replenishRequested=0,replenishReturned=0,ammoBeforeEnable=0,ammoAfterEnable=0;
    uint8_t originalRule=0,originalReserveRule=0;
    unsigned error=0;
    uintptr_t exceptionRva=0;
    void *world=nullptr,*gameState=nullptr,*ammoComponent=nullptr,*ammoOwner=nullptr,*owningPawn=nullptr;
    const char* stage="original_infinite_ammo_not_initialized";
    const char* operation="none";
    const wchar_t* failedEvent=L"";
    uint16_t expectedParmsSize=0,actualParmsSize=0;
};
inline Diagnostics state;
inline const Diagnostics& State(){return state;}
inline bool Enabled(){return state.initialized&&state.enabled;}
inline const wchar_t* StatusText(){return Enabled()?L"INFINITE AMMO: ON  [F6]":L"INFINITE AMMO: OFF  [F6]";}
template<class T> inline T Value(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned error,const char* text){state.error=error;state.stage=text;return false;}
inline bool Thread(){return !state.restartRequired&&original_players::Thread();}
inline bool CanEvent(void* object,const wchar_t* name,uint16_t size){
    auto method=engine::Find(name);const uint16_t actual=Value<uint16_t>(method,0xbe);
    auto table=Value<void**>(object,0);auto process=Value<void*>(table,0x298);
    if(!object||!method||actual!=size||!process){
        state.failedEvent=name;state.expectedParmsSize=size;state.actualParmsSize=actual;return false;
    }
    return true;
}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* parameters){
    if(!CanEvent(object,name,size))return false;
    auto method=engine::Find(name);auto table=Value<void**>(object,0);auto process=Value<void*>(table,0x298);
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,method,parameters);return true;
}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0001820U;
    const auto at=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;
    state.restartRequired=true;state.stage="original_infinite_ammo_exception_requires_new_session";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Context(){
    if(!Thread()||!state.contextBound||state.world!=original_players::world)
        return Fail(1820,"original_infinite_ammo_original_context_required");
    state.worldGameStateMatches=Value<void*>(state.world,0x178)==state.gameState;
    state.authority=original_players::IsA(state.gameState,original_players::ShooterGameStateClass)
        &&Value<uint8_t>(state.gameState,0x224)==3;
    if(!state.worldGameStateMatches||!state.authority)
        return Fail(1821,"original_infinite_ammo_original_authority_game_state_required");
    return true;
}
inline bool ReadRules(){
    uint8_t ammoRule=0,reserveRule=0;
    if(!Event(state.gameState,L"/Script/ShooterGame.ShooterGameState.IsInfiniteAmmoEnabled",1,&ammoRule)
       ||!Event(state.gameState,L"/Script/ShooterGame.ShooterGameState.IsInfiniteReserveEnabled",1,&reserveRule))
        return Fail(1822,"original_infinite_ammo_original_rule_getter_missing");
    state.nativeAmmoRule=ammoRule!=0;state.nativeReserveRule=reserveRule!=0;
    state.enabled=state.nativeAmmoRule;state.requestedRuleMatches=state.enabled==state.requestedEnabled;
    return true;
}
inline bool ReadMagazine(){
    if(!state.ammoComponent)return Fail(1823,"original_infinite_ammo_magazine_not_bound");
    if(!Event(state.ammoComponent,L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,&state.ammo)
       ||!Event(state.ammoComponent,L"/Script/ShooterGame.AmmoComponent.GetMaxAmmo",4,&state.maxAmmo)
       ||state.maxAmmo<=0||state.maxAmmo>65535||state.ammo<0||state.ammo>state.maxAmmo)
        return Fail(1824,"original_infinite_ammo_original_magazine_query_failed");
    auto reserve=Value<void*>(state.ammoComponent,0x138);state.reserveAmmo=0;
    if(reserve&&(!original_players::IsA(reserve,0x63c7e00)
       ||!Event(reserve,L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,&state.reserveAmmo)))
        return Fail(1825,"original_infinite_ammo_original_reserve_query_failed");
    return true;
}
inline bool BindAmmoImpl(void* magazine){
    if(!Context()||!original_players::IsA(magazine,0x63c7e00))
        return Fail(1826,"original_infinite_ammo_real_ammo_component_required");
    auto owner=Value<void*>(magazine,0xc8);auto pawn=Value<void*>(owner,0x4a8);
    state.localAmmoOwnerValid=original_players::IsA(owner,0x645c660)
        &&pawn==original_players::activePawn
        &&original_players::IsA(pawn,original_players::ShooterCharacterClass)
        &&Value<uint8_t>(pawn,0x224)==3;
    if(!state.localAmmoOwnerValid)return Fail(1827,"original_infinite_ammo_requires_actual_local_weapon");
    auto table=Value<void**>(magazine,0);
    state.originalConsumeVerified=Value<void*>(table,0x4f0)==reinterpret_cast<void*>(engine::image+0x6ef0e80);
    state.originalReplenishVerified=Value<void*>(table,0x4f8)==reinterpret_cast<void*>(engine::image+0x6ef0f60);
    if(!state.originalConsumeVerified||!state.originalReplenishVerified)
        return Fail(1828,"original_infinite_ammo_native_resource_path_changed");
    state.ammoComponent=magazine;state.ammoOwner=owner;state.owningPawn=pawn;
    state.minimumEnabledAmmo=-1;
    return ReadMagazine();
}
inline bool BindAmmo(void* magazine){
    __try{return BindAmmoImpl(magazine);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool PollImpl(){
    if(!Context())return false;
    if(state.owningPawn!=original_players::activePawn
       ||Value<void*>(state.ammoComponent,0xc8)!=state.ammoOwner
       ||Value<void*>(state.ammoOwner,0x4a8)!=state.owningPawn)
        return Fail(1829,"original_infinite_ammo_local_weapon_changed_rebind_required");
    const bool previousEnabled=state.enabled;const int32_t previousAmmo=state.ammo;
    if(!ReadRules()||!ReadMagazine())return false;
    if(previousEnabled&&state.enabled&&state.ammo<previousAmmo)++state.ammoDropsWhileEnabled;
    if(state.enabled&&(state.minimumEnabledAmmo<0||state.ammo<state.minimumEnabledAmmo))state.minimumEnabledAmmo=state.ammo;
    ++state.polls;return true;
}
inline bool Poll(){
    __try{return PollImpl();}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool ReplenishForEnable(){
    if(!ReadMagazine())return false;
    state.ammoBeforeEnable=state.ammo;state.ammoAfterEnable=state.ammo;
    state.replenishRequested=state.maxAmmo-state.ammo;state.replenishReturned=0;
    if(state.replenishRequested>0){
        struct Replenish {int32_t units=0,result=0;} parameters;
        static_assert(sizeof(Replenish)==8);
        parameters.units=state.replenishRequested;
        state.operation="original_infinite_ammo_enable_edge_native_replenish";
        // Original virtual Ammo.Replenish6ef0f60 caps the request and dispatches
        // Resource.Replenish6f617e0 -> original commit6f619c0/events. This happens
        // once on an enable edge, including when the magazine was already0.
        if(!Event(state.ammoComponent,L"/Script/ShooterGame.ResourceComponent.Replenish",8,&parameters))
            return Fail(1830,"original_infinite_ammo_native_replenish_unavailable");
        state.replenishReturned=parameters.result;++state.replenishCalls;
        if(!ReadMagazine())return false;
        state.ammoAfterEnable=state.ammo;
    }
    state.refillReadbackValid=state.ammoAfterEnable==state.maxAmmo;
    return state.refillReadbackValid?true:Fail(1831,"original_infinite_ammo_enable_refill_readback_mismatch");
}
inline bool SetEnabledImpl(bool enabled){
    if(!Context()||!state.ammoComponent)return false;
    if(!CanEvent(state.gameState,L"/Script/ShooterGame.ShooterGameState.OnRep_GameInfo",64)
       ||!ReadRules())return Fail(1832,"original_infinite_ammo_game_info_notification_unavailable");
    const bool enableEdge=enabled&&(!state.enabled||!state.initialized);
    const bool reserveBefore=state.nativeReserveRule;
    const auto previous=Value<GameInfo>(state.gameState,GameInfoOffset);
    if(previous[InfiniteAmmoField]>1||previous[InfiniteReserveField]>1)
        return Fail(1833,"original_infinite_ammo_game_info_bool_layout_invalid");
    state.requestedEnabled=enabled;++state.setRequests;
    if(state.enabled!=enabled){
        state.operation="original_infinite_ammo_publish_missing_local_game_rule";
        const uint8_t value=enabled?1:0;
        // The only local authority record supplied here. All other GameInfo
        // fields (time/phase/abilities/reserve/money/etc.) are retained.
        if(!original_players::Write(state.gameState,GameInfoOffset+InfiniteAmmoField,value))
            return Fail(1834,"original_infinite_ammo_local_rule_publication_failed");
        auto expected=previous;expected[InfiniteAmmoField]=value;
        const auto actual=Value<GameInfo>(state.gameState,GameInfoOffset);
        state.localWritePreservedOtherFields=actual==expected;
        if(!state.localWritePreservedOtherFields)return Fail(1835,"original_infinite_ammo_other_game_info_fields_changed");
        ++state.ruleWrites;
        auto oldParameters=previous;
        state.operation="original_infinite_ammo_original_onrep_game_info";
        if(!Event(state.gameState,L"/Script/ShooterGame.ShooterGameState.OnRep_GameInfo",64,oldParameters.data()))
            return Fail(1836,"original_infinite_ammo_original_game_info_notification_failed");
        state.notificationCalled=true;++state.notifications;
    }
    if(!ReadRules()||state.enabled!=enabled)
        return Fail(1837,"original_infinite_ammo_rule_readback_mismatch");
    state.reserveRulePreserved=state.nativeReserveRule==reserveBefore;
    if(!state.reserveRulePreserved)return Fail(1838,"original_infinite_ammo_reserve_rule_changed");
    if(enableEdge&&!ReplenishForEnable())return false;
    if(!PollImpl())return false;
    state.initialized=true;state.error=0;state.operation="none";
    state.stage=enabled?"original_infinite_ammo_enabled":"original_infinite_ammo_disabled";return true;
}
inline bool SetEnabled(bool enabled){
    __try{return SetEnabledImpl(enabled);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool Toggle(){
    __try{
        if(!state.initialized||!PollImpl())return false;
        ++state.toggles;return SetEnabledImpl(!state.enabled);
    }__except(Capture(GetExceptionInformation())){return false;}
}
inline bool InitializeImpl(void* world,void* magazine,bool initiallyEnabled){
    if(!Thread()||!world||world!=original_players::world)
        return Fail(1839,"original_infinite_ammo_initialize_original_world_required");
    if(state.initialized){
        if(world!=state.world)return Fail(1840,"original_infinite_ammo_world_changed_requires_new_session");
        // Repeated initialization is read-only; it cannot reset a user's F6 selection.
        return state.ammoComponent==magazine?PollImpl():BindAmmoImpl(magazine);
    }
    state.world=world;state.gameState=Value<void*>(world,0x178);state.contextBound=true;
    if(!Context()||!BindAmmoImpl(magazine)||!ReadRules())return false;
    state.originalRule=Value<uint8_t>(state.gameState,GameInfoOffset+InfiniteAmmoField);
    state.originalReserveRule=Value<uint8_t>(state.gameState,GameInfoOffset+InfiniteReserveField);
    return SetEnabledImpl(initiallyEnabled);
}
inline bool Initialize(void* world,void* magazine,bool initiallyEnabled=true){
    __try{return InitializeImpl(world,magazine,initiallyEnabled);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    const auto b=[](bool value){return value?"true":"false";};
    std::fputs("{\"schema_version\":1,\"stage\":",file);original_players::Text(file,state.stage);
    std::fprintf(file,",\"error\":%u,\"initialized\":%s,\"enabled\":%s,\"requested_enabled\":%s,\"native_ammo_rule\":%s,\"native_reserve_rule\":%s,\"requested_rule_matches\":%s,\"authority\":%s,\"world_game_state_matches\":%s,\"local_ammo_owner_valid\":%s,\"original_consume_verified\":%s,\"original_replenish_verified\":%s",state.error,b(state.initialized),b(state.enabled),b(state.requestedEnabled),b(state.nativeAmmoRule),b(state.nativeReserveRule),b(state.requestedRuleMatches),b(state.authority),b(state.worldGameStateMatches),b(state.localAmmoOwnerValid),b(state.originalConsumeVerified),b(state.originalReplenishVerified));
    std::fprintf(file,",\"local_write_preserved_other_fields\":%s,\"reserve_rule_preserved\":%s,\"original_rule\":%u,\"original_reserve_rule\":%u,\"notification_called\":%s,\"refill_readback_valid\":%s,\"toggles\":%u,\"set_requests\":%u,\"rule_writes\":%u,\"notifications\":%u,\"polls\":%u,\"replenish_calls\":%u,\"ammo_drops_while_enabled\":%u,\"ammo\":%d,\"maximum_ammo\":%d,\"reserve_ammo\":%d,\"minimum_enabled_ammo\":%d,\"enable_refill_requested\":%d,\"enable_refill_returned\":%d,\"ammo_before_enable\":%d,\"ammo_after_enable\":%d",b(state.localWritePreservedOtherFields),b(state.reserveRulePreserved),static_cast<unsigned>(state.originalRule),static_cast<unsigned>(state.originalReserveRule),b(state.notificationCalled),b(state.refillReadbackValid),state.toggles,state.setRequests,state.ruleWrites,state.notifications,state.polls,state.replenishCalls,state.ammoDropsWhileEnabled,state.ammo,state.maxAmmo,state.reserveAmmo,state.minimumEnabledAmmo,state.replenishRequested,state.replenishReturned,state.ammoBeforeEnable,state.ammoAfterEnable);
    std::fprintf(file,",\"restart_required\":%s,\"exception_rva\":%llu,\"operation\":",b(state.restartRequired),static_cast<unsigned long long>(state.exceptionRva));original_players::Text(file,state.operation);
    std::fputs(",\"failed_event\":",file);const auto method=original_players::Utf8(state.failedEvent);original_players::Text(file,method.c_str());
    std::fprintf(file,",\"expected_parms_size\":%u,\"actual_parms_size\":%u,\"world\":\"%p\",\"game_state\":\"%p\",\"ammo_component\":\"%p\",\"ammo_owner\":\"%p\",\"owning_pawn\":\"%p\",\"per_frame_resource_writes\":false,\"runtime_shot_acceptance_verified\":false}\n",static_cast<unsigned>(state.expectedParmsSize),static_cast<unsigned>(state.actualParmsSize),state.world,state.gameState,state.ammoComponent,state.ammoOwner,state.owningPawn);
    const bool result=std::ferror(file)==0;std::fclose(file);return result;
}
}
