#pragma once
#include "original_players.h"
#include "local_server_mode_startup.h"

// Exclude the retained startup LocalPlayer's exact Engine.PlayerState from the
// authority match roster. Preserve all startup objects and every other entry.
// Evidence: work/local-server-playable/character-lifecycle/bootstrap-roster-*.
namespace local_server_bootstrap_roster {
inline constexpr size_t MaxRoster=original_players::MaxPlayers+1;
struct Diagnostics {
    bool complete=false,profilesVerified=false,bootstrapIdentityVerified=false;
    bool rosterMetadataVerified=false,presentBefore=false,removed=false;
    bool otherEntriesPreserved=false,bootstrapOwnershipPreserved=false,restartRequired=false;
    unsigned error=0;
    uint32_t checks=0,nativeRemoveCalls=0;
    int32_t beforeCount=0,afterCount=0;
    uintptr_t exceptionRva=0;
    const char* stage="local_network_bootstrap_roster_not_checked";
    const char* operation="none";
    void *world=nullptr,*gameState=nullptr,*controller=nullptr,*localPlayer=nullptr;
    void *bootstrapState=nullptr,*bootstrapPawn=nullptr;
};
inline Diagnostics state;
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* p,size_t o){return original_players::Value<T>(p,o);}
inline bool Fail(unsigned code,const char* reason){state.complete=false;state.error=code;state.stage=reason;return false;}
inline bool VerifyProfiles(){
    struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
    static constexpr Profile profiles[]{
        {0x7187b50,314,0x86aa847058f4d31bULL}, // actual ShooterGS PostInitializeComponents
        {0x443c1c0,900,0xe602362af9476ac4ULL}, // original Engine.GameState parent
        {0x443eb00,1207,0xcbaf11880cbf6787ULL}, // registers GS, imports all existing Engine PS
        {0x71904f0,546,0x1ac1dcbfb5c59831ULL}, // actual full ShooterGS.RemovePlayerState
        {0x7190720,93,0x06b4c11f21f25395ULL}, // original roster change notifications
        {0x443f200,155,0xcc16da8427daa0c0ULL}, // original base array-removal contract
    };
    for(const auto& p:profiles){
        const auto* bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        uint64_t hash=14695981039346656037ULL;
        for(size_t i=0;i<p.bytes;++i)hash=(hash^bytes[i])*1099511628211ULL;
        if(hash!=p.fnv)return false;
    }
    return true;
}
inline void* ActorWorld(void* actor){
    auto function=Read<void*>(Read<void**>(actor,0),0x1b8);
    return function?reinterpret_cast<void*(*)(void*)>(function)(actor):nullptr;
}
inline bool RosterMetadata(void* gs){
    uint32_t name[3]{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(name,L"PlayerArray",1);
    auto property=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(Read<void*>(gs,0x10),name,3);
    constexpr uint64_t netAndRepNotify=0x100000020ULL;
    return property&&Read<int32_t>(property,0x30)==1&&Read<int32_t>(property,0x34)==16
        &&Read<int32_t>(property,0x44)==0x480
        &&(Read<uint64_t>(property,0x38)&netAndRepNotify)==netAndRepNotify;
}
struct BootstrapSnapshot {
    void* refs[13]{};
    uint32_t flags[4]{};
    uint8_t roles[3]{};
    int32_t localCount=0,localCapacity=0;
};
inline BootstrapSnapshot CaptureBootstrap(){
    BootstrapSnapshot b{};
    auto gi=Read<void*>(state.world,0x1d8);auto locals=Read<void**>(gi,0x40);
    b.refs[0]=Read<void*>(state.controller,0x470);b.refs[1]=Read<void*>(state.controller,0x4b0);
    b.refs[2]=Read<void*>(state.controller,0x510);b.refs[3]=Read<void*>(state.bootstrapState,0x210);
    b.refs[4]=Read<void*>(state.bootstrapPawn,0x4a0);b.refs[5]=Read<void*>(state.bootstrapPawn,0x488);
    b.refs[6]=Read<void*>(state.localPlayer,0x38);b.refs[7]=gi;b.refs[8]=locals;
    b.refs[9]=Read<void*>(locals,0);b.refs[10]=Read<void*>(state.bootstrapState,0x10);
    b.refs[11]=Read<void*>(state.controller,0x10);b.refs[12]=Read<void*>(state.bootstrapPawn,0x10);
    void* objects[]{state.controller,state.bootstrapState,state.bootstrapPawn,state.localPlayer};
    for(size_t i=0;i<4;++i)b.flags[i]=Read<uint32_t>(objects[i],8);
    for(size_t i=0;i<3;++i)b.roles[i]=Read<uint8_t>(objects[i],0x224);
    b.localCount=Read<int32_t>(gi,0x48);b.localCapacity=Read<int32_t>(gi,0x4c);return b;
}
inline bool SameBootstrap(const BootstrapSnapshot& a,const BootstrapSnapshot& b){
    for(size_t i=0;i<13;++i)if(a.refs[i]!=b.refs[i])return false;
    for(size_t i=0;i<4;++i)if(a.flags[i]!=b.flags[i])return false;
    for(size_t i=0;i<3;++i)if(a.roles[i]!=b.roles[i])return false;
    return a.localCount==b.localCount&&a.localCapacity==b.localCapacity;
}
struct RosterEntry {void *object=nullptr,*owner=nullptr,*type=nullptr;uint32_t flags=0;uint8_t role=0;};
inline RosterEntry CaptureEntry(void* ps){
    return {ps,Read<void*>(ps,0x210),Read<void*>(ps,0x10),Read<uint32_t>(ps,8),Read<uint8_t>(ps,0x224)};
}
inline bool SameEntry(const RosterEntry& a,const RosterEntry& b){
    return a.object==b.object&&a.owner==b.owner&&a.type==b.type&&a.flags==b.flags&&a.role==b.role;
}
inline bool ExcludeBootstrapStateImpl(void* world,void* newGS,void* knownBootstrapPC,void* knownLP){
    auto& d=state;
    if(d.error||d.restartRequired)return false;
    if(!original_players::Thread())return Fail(3520,"local_network_bootstrap_roster_requires_original_game_thread");
    if(d.world&&(d.world!=world||d.gameState!=newGS||d.controller!=knownBootstrapPC||d.localPlayer!=knownLP))
        return Fail(3521,"local_network_bootstrap_roster_identity_changed");
    d.world=world;d.gameState=newGS;d.controller=knownBootstrapPC;d.localPlayer=knownLP;++d.checks;
    const bool nativeStartup=local_server_mode_startup::GetState().configured
        &&local_server_mode_startup::GetState().world==world;
    if(nativeStartup&&local_server_mode_startup::options.noLocalPlayerBootstrap){
        const auto gi=Read<void*>(world,0x1d8);
        if(knownBootstrapPC||knownLP||Read<int32_t>(gi,0x48)!=0
           ||!local_server_mode_startup::ReadyForAdoption(world,nullptr)||newGS!=Read<void*>(world,0x178))
            return Fail(3522,"local_network_dedicated_startup_has_no_bootstrap_player_to_exclude");
        d.bootstrapIdentityVerified=d.otherEntriesPreserved=d.bootstrapOwnershipPreserved=d.complete=true;
        d.stage="local_network_native_dedicated_no_local_bootstrap_roster_mutation";return true;
    }
    d.bootstrapState=Read<void*>(knownBootstrapPC,0x470);d.bootstrapPawn=Read<void*>(knownBootstrapPC,0x4b0);
    const bool pawnless=nativeStartup&&!d.bootstrapPawn
        &&local_server_mode_startup::ReadyForAdoption(world,knownBootstrapPC)
        &&knownLP==local_server_mode_startup::GetState().bootstrapPlayer;

    d.stage="local_network_identify_bootstrap_only_roster_entry";
    auto gi=Read<void*>(world,0x1d8);auto locals=Read<void**>(gi,0x40);
    if(!world||Read<void*>(world,0x178)!=newGS
       ||!original_players::IsA(newGS,original_players::ShooterGameStateClass)||Read<uint8_t>(newGS,0x224)!=3
       ||Read<void*>(Read<void*>(world,0x170),0x4d8)!=newGS
       ||Read<void*>(newGS,0x470)!=Read<void*>(world,0x170)
       ||!original_players::IsA(knownBootstrapPC,0x3c0a200)||!original_players::IsA(knownLP,0x3b6e680)
       ||Read<int32_t>(gi,0x48)!=1||Read<void*>(locals,0)!=knownLP
       ||Read<void*>(knownLP,0x38)!=knownBootstrapPC||Read<void*>(knownBootstrapPC,0x510)!=knownLP
       ||!d.bootstrapState||Read<void*>(d.bootstrapState,0x10)!=engine::Class(0x42aae70)
       ||Read<void*>(d.bootstrapState,0x210)!=knownBootstrapPC
       ||(!pawnless&&(!original_players::IsA(d.bootstrapPawn,0x4307cd0)||Read<void*>(d.bootstrapPawn,0x4a0)!=knownBootstrapPC))
       ||Read<uint8_t>(knownBootstrapPC,0x224)!=3||Read<uint8_t>(d.bootstrapState,0x224)!=3
       ||ActorWorld(newGS)!=world||ActorWorld(knownBootstrapPC)!=world
       ||ActorWorld(d.bootstrapState)!=world||(!pawnless&&ActorWorld(d.bootstrapPawn)!=world))
        return Fail(3522,"local_network_bootstrap_roster_requires_exact_original_local_engine_player_tuple");
    d.bootstrapIdentityVerified=true;
    if(!d.profilesVerified){d.profilesVerified=VerifyProfiles();if(!d.profilesVerified)return Fail(3523,"local_network_bootstrap_roster_native_profile_mismatch");}
    d.rosterMetadataVerified=RosterMetadata(newGS);
    auto table=Read<void**>(newGS,0);auto remove=Read<void*>(table,0x7c0);
    if(!d.rosterMetadataVerified||remove!=reinterpret_cast<void*>(engine::image+0x71904f0))
        return Fail(3524,"local_network_bootstrap_roster_original_array_or_remove_contract_mismatch");
    d.beforeCount=Read<int32_t>(newGS,0x488);const auto capacity=Read<int32_t>(newGS,0x48c);
    auto array=Read<void**>(newGS,0x480);
    if(d.beforeCount<0||d.beforeCount>static_cast<int32_t>(MaxRoster)||capacity<d.beforeCount||(d.beforeCount&&!array))
        return Fail(3525,"local_network_bootstrap_roster_invalid_or_unbounded_original_array");
    RosterEntry before[MaxRoster]{};int32_t bootstrapIndex=-1;
    for(int32_t i=0;i<d.beforeCount;++i){
        auto ps=Read<void*>(array,static_cast<size_t>(i)*8);
        if(!original_players::IsA(ps,0x42aae70))return Fail(3526,"local_network_bootstrap_roster_contains_invalid_player_state");
        for(int32_t j=0;j<i;++j)if(before[j].object==ps)return Fail(3527,"local_network_bootstrap_roster_contains_duplicate_player_state");
        before[i]=CaptureEntry(ps);if(ps==d.bootstrapState)bootstrapIndex=i;
    }
    const auto bootstrapBefore=CaptureBootstrap();d.presentBefore=bootstrapIndex>=0;
    if(d.presentBefore){
        d.operation="original_shooter_game_state_remove_player_state_71904f0";
        // Original native ABI: RCX=the actual GS, RDX=one PS pointer. This is
        // not a reflected UFUNCTION and does not invoke Logout or actor destroy.
        reinterpret_cast<void(*)(void*,void*)>(remove)(newGS,d.bootstrapState);++d.nativeRemoveCalls;d.removed=true;
    }
    d.afterCount=Read<int32_t>(newGS,0x488);array=Read<void**>(newGS,0x480);
    if(d.afterCount!=d.beforeCount-(d.presentBefore?1:0)||(d.afterCount&&!array))
        return Fail(3528,"local_network_bootstrap_roster_native_remove_count_mismatch");
    int32_t next=0;
    for(int32_t i=0;i<d.beforeCount;++i){
        if(i==bootstrapIndex)continue;
        auto after=CaptureEntry(Read<void*>(array,static_cast<size_t>(next++)*8));
        if(!SameEntry(before[i],after))return Fail(3529,"local_network_bootstrap_roster_other_participant_changed");
    }
    d.otherEntriesPreserved=true;d.bootstrapOwnershipPreserved=SameBootstrap(bootstrapBefore,CaptureBootstrap());
    if(!d.bootstrapOwnershipPreserved)return Fail(3530,"local_network_bootstrap_roster_startup_objects_or_ownership_changed");
    d.operation="none";d.complete=true;d.stage=d.presentBefore?"local_network_bootstrap_state_excluded_from_native_roster":"local_network_bootstrap_state_already_absent_from_native_roster";
    return true;
}
inline LONG CaptureException(EXCEPTION_POINTERS* exception){
    if(exception&&exception->ExceptionRecord){
        state.error=exception->ExceptionRecord->ExceptionCode;
        const auto at=reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress);
        state.exceptionRva=at>=engine::image?at-engine::image:at;
    }else state.error=3531;
    state.complete=false;state.restartRequired=true;state.stage="local_network_bootstrap_roster_native_exception_requires_new_session";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool ExcludeBootstrapState(void* world,void* newGS,void* knownBootstrapPC,void* knownLP){
    __try {return ExcludeBootstrapStateImpl(world,newGS,knownBootstrapPC,knownLP);}
    __except(CaptureException(GetExceptionInformation())) {return false;}
}
}
