#pragma once
#include "local_server_combat.h"
#include "local_server_kill_ammo_rules.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <string>

// Missing separate-host rule wiring only. The original authority replenisher
// and its original owning-client RPC remain the sole resource operation.
// No native ammo field, equip/input state, reload animation, score or clock is
// written here. Original DM queue delay and captured-current-gun semantics are
// retained; the user's explicit full-ammo request replaces stock +30/+30.
namespace local_server_kill_ammo {
namespace rules=local_server_kill_ammo_rules;
using Weak=original_inventory_slots::WeakIdentity;
inline constexpr size_t MaxPlayers=original_players::MaxPlayers,MaxPending=128;
inline constexpr size_t SeenCapacity=MaxPlayers*(local_server_match::KillLimit+1);
inline constexpr double KillReloadDelay=0.10000000149011612;
struct Pending {
    bool used=false;uint64_t token=0;size_t source=0,victim=0;uint32_t epoch=0;
    void *pawn=nullptr,*gun=nullptr;Weak pawnId{},gunId{};double due=0;
};
struct Diagnostics {
    bool initialized=false,stopped=false,restartRequired=false,busy=false;
    unsigned error=0;uintptr_t exceptionRva=0;const char* stage="server_kill_ammo_not_initialized";
    void *world=nullptr,*gameState=nullptr;Weak worldId{};
    uint64_t committedKills=0,duplicateCommits=0,queued=0,pending=0;
    uint64_t policySkips=0,noFirearmSkips=0,cancelled=0,registrationDeferrals=0;
    uint64_t killRefills=0,pickupRequests=0,pickupRefills=0,stoppedPending=0;
    uint64_t lastToken=0;int32_t lastSource=-1;double worldTime=0;
    local_server_combat::AmmoRefillReadback lastRefill;
};
inline Diagnostics state;
inline std::array<Pending,MaxPending> pending{};
inline rules::Ledger<SeenCapacity> seen;
inline SRWLOCK jsonLock=SRWLOCK_INIT;
inline std::string jsonSnapshot="{\"schema_version\":1,\"stage\":\"server_kill_ammo_not_initialized\",\"error\":0,\"initialized\":false}";
inline const Diagnostics& GetState(){return state;}
inline bool Fail(unsigned error,const char* stage,bool fatal=false){state.error=error;state.stage=stage;state.restartRequired=state.restartRequired||fatal;return false;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Thread(){return !state.restartRequired&&local_server_combat::Thread();}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Context(){
    return Thread()&&state.initialized&&Live(state.world,state.worldId)
        &&local_server_match::GetState().world==state.world
        &&Read<void*>(state.world,0x178)==state.gameState;
}
inline bool Clock(){state.worldTime=Read<double>(state.world,0x740);return std::isfinite(state.worldTime)&&state.worldTime>=0;}
inline bool Infinite(bool& result){
    uint8_t actual=255;
    if(!original_damage::Event(state.gameState,L"/Script/ShooterGame.ShooterGameState.IsInfiniteAmmoEnabled",1,&actual)||actual>1)
        return Fail(5480,"server_kill_ammo_original_policy_getter_failed");
    // Either side of a legitimate configuration publication edge suppresses
    // this finite-ammo rule. Neither original policy nor the request is edited.
    result=actual!=0||(local_server_match::Configuration().config.flags&training_control::InfiniteAmmo)!=0;return true;
}
inline bool NativeAlive(void* pawn,bool& alive){
    uint8_t actual=255;
    if(!original_damage::Event(pawn,L"/Script/ShooterGame.ShooterCharacter.IsAlive",1,&actual)||actual>1)return false;
    alive=actual!=0;return true;
}
inline bool InitializeImpl(void* world){
    if(state.initialized)return Context()&&world==state.world;
    if(!Thread()||!local_server_match::GetState().initialized||local_server_match::GetState().world!=world
       ||!local_server_combat::GetState().initialized)return false;
    if(!original_inventory_slots::CaptureIdentity(world,state.worldId))return false;
    state.world=world;state.gameState=Read<void*>(world,0x178);
    if(!original_players::IsA(state.gameState,original_players::ShooterGameStateClass)||!Clock())return false;
    state.initialized=true;state.stage="server_kill_ammo_ready";return true;
}
inline bool KillCommitted(uint64_t token){
    bool native=false,match=false;
    for(const auto t:original_scoreboard::killTokens)native=native||t==token;
    for(const auto t:local_server_match::committedTokens)match=match||t==token;
    return token&&native&&match&&!local_server_match::pending.active;
}
inline bool OnCommittedKillImpl(uint64_t token,size_t source,size_t victim){
    if(!Context()||source>=MaxPlayers||victim>=MaxPlayers||source==victim||!token)return false;
    if(state.stopped||!local_server_match::FreeForAll())return true;
    const rules::Commit key{token,source,victim};const auto prior=seen.Lookup(key);
    if(prior==rules::Seen::Duplicate){++state.duplicateCommits;return true;}
    if(prior!=rules::Seen::New)return Fail(5481,"server_kill_ammo_duplicate_conflict_or_full");
    const auto s=local_server_match::Player(source),v=local_server_match::Player(victim);
    bool victimAlive=true;
    if(!s||!v||!KillCommitted(token)||!v->present||!Live(v->pawn,v->pawnIdentity)||v->alive||v->phase!=local_server_match::Phase::DeadWaiting
       ||!v->deathAccounted||!NativeAlive(v->pawn,victimAlive)||victimAlive)
        return Fail(5482,"server_kill_ammo_requires_committed_original_lethal_result");
    if(seen.Insert(key)!=rules::Seen::New)return Fail(5481,"server_kill_ammo_duplicate_conflict_or_full");
    ++state.committedKills;state.lastToken=token;state.lastSource=static_cast<int32_t>(source);
    bool infinite=false;if(!Infinite(infinite))return false;
    if(infinite){++state.policySkips;return true;}
    bool sourceAlive=false;
    if(!s->alive||s->phase!=local_server_match::Phase::Alive||!local_server_combat::CurrentRegistration(source)
       ||!NativeAlive(s->pawn,sourceAlive)||!sourceAlive){++state.cancelled;return true;}
    void* gun=nullptr;auto inventory=Read<void*>(s->pawn,0xc08);
    if(!original_damage::Event(inventory,L"/Script/ShooterGame.AresInventory.GetCurrentEquippable",8,&gun))
        return Fail(5483,"server_kill_ammo_current_equippable_query_failed");
    if(!original_players::IsA(gun,0x645c660)||!original_weapon::Component(gun,0x63c7e00)){
        ++state.noFirearmSkips;return true;
    }
    Pending next;next.used=true;next.token=token;next.source=source;next.victim=victim;
    next.epoch=s->epoch;next.pawn=s->pawn;next.gun=gun;
    if(Read<void*>(gun,0x4a8)!=next.pawn||!original_inventory_slots::CaptureIdentity(next.pawn,next.pawnId)
       ||!original_inventory_slots::CaptureIdentity(gun,next.gunId)||!Clock())
        return Fail(5484,"server_kill_ammo_current_gun_owner_identity_missing");
    next.due=state.worldTime+KillReloadDelay;
    for(auto& slot:pending)if(!slot.used){slot=next;++state.queued;++state.pending;state.stage="server_kill_ammo_native_delay_pending";return true;}
    return Fail(5485,"server_kill_ammo_queue_full");
}
inline bool RefillGun(size_t index,void* pawn,void* gun){
    // The shared helper performs exactly one Auth event and real native
    // magazine/reserve GetAmmo/GetMaxAmmo plus ownership/Weak readbacks.
    state.lastRefill={};
    if(!local_server_combat::RefillAmmoForWeapon(index,pawn,gun))
        return Fail(local_server_combat::GetState().error?local_server_combat::GetState().error:5486,
                    "server_kill_ammo_original_authority_refill_failed",true);
    state.lastRefill=local_server_combat::lastAmmoRefill;
    return state.lastRefill.valid;
}
inline bool PollImpl(){
    if(!Context()||!Clock())return false;
    if(state.stopped)return true;
    bool infinite=false;if(!Infinite(infinite))return false;
    state.busy=false;
    for(auto& entry:pending)if(entry.used){
        const auto player=local_server_match::Player(entry.source);
        const bool same=player&&player->epoch==entry.epoch&&player->pawn==entry.pawn&&Live(entry.pawn,entry.pawnId);
        const bool gun=Live(entry.gun,entry.gunId)&&Read<void*>(entry.gun,0x4a8)==entry.pawn;
        bool alive=false;
        if(same&&player->alive&&player->phase==local_server_match::Phase::Alive&&!NativeAlive(entry.pawn,alive))
            return Fail(5487,"server_kill_ammo_original_life_query_failed");
        rules::Gate gate{local_server_match::Active(),infinite,same,alive,gun,
                         same&&local_server_combat::CurrentRegistration(entry.source)};
        const auto action=rules::Decide(gate,state.worldTime,entry.due);
        if(action==rules::Action::Wait){if(!gate.registrationReady){++state.registrationDeferrals;state.busy=true;}continue;}
        // Retire this private intent before any mutating native call. A
        // partial native failure is never automatically replayed next frame.
        entry.used=false;--state.pending;
        if(action==rules::Action::Cancel){++state.cancelled;continue;}
        if(!RefillGun(entry.source,entry.pawn,entry.gun))return false;
        ++state.killRefills;
    }
    state.error=0;state.stage=state.pending?"server_kill_ammo_native_delay_pending":"server_kill_ammo_running";return true;
}
// Health-pack caller passes its actually overlapped living authority Pawn.
// The original pickup reloads both firearm slots, even when melee is current.
// This is an explicit pickup operation, so it is not a replayed kill and does
// not insert a kill token. Full limits are each resource's actual native max.
inline bool RefillForPawnImpl(size_t index,void* pawn){
    if(!Context()||state.stopped||!local_server_match::Active()||!local_server_combat::CurrentRegistration(index))return false;
    const auto player=local_server_match::Player(index);bool alive=false;
    if(!player||player->pawn!=pawn||!player->alive||player->phase!=local_server_match::Phase::Alive
       ||!NativeAlive(pawn,alive)||!alive)return false;
    ++state.pickupRequests;void* previous=nullptr;auto inventory=Read<void*>(pawn,0xc08);
    for(uint8_t slot=0;slot<2;++slot){
        struct Item {uint8_t slot=0,pad[7]{};void* result=nullptr;} item;item.slot=slot;
        static_assert(sizeof(Item)==16);
        if(!original_damage::Event(inventory,L"/Script/ShooterGame.AresInventory.GetFirstItemFromSlot",16,&item))
            return Fail(5488,"server_kill_ammo_original_pickup_slot_query_failed");
        if(!original_players::IsA(item.result,0x645c660)||!original_weapon::Component(item.result,0x63c7e00))continue;
        if(item.result==previous)return Fail(5489,"server_kill_ammo_duplicate_firearm_slot");
        previous=item.result;
        if(!RefillGun(index,pawn,item.result))return false;
        ++state.pickupRefills;
    }
    state.error=0;state.stage="server_kill_ammo_pickup_refilled";return true;
}
inline bool StopImpl(){
    // Retiring only this module's private intents needs no old UObject read.
    // This remains possible after a native failure or world/weak retirement.
    if(GetCurrentThreadId()!=local_server_combat::gameThread)return false;
    for(auto& entry:pending)if(entry.used){entry.used=false;++state.stoppedPending;}
    state.pending=0;state.stopped=true;state.stage="server_kill_ammo_stopped";return true;
}
inline LONG Capture(EXCEPTION_POINTERS* info){
    const auto record=info?info->ExceptionRecord:nullptr;
    const auto at=record?reinterpret_cast<uintptr_t>(record->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;
    Fail(record?record->ExceptionCode:0xe0005490U,"server_kill_ammo_native_exception_requires_new_session",true);
    return EXCEPTION_EXECUTE_HANDLER;
}
inline void CacheJson(){
    // Only the game thread reads live module state. JSON and file writers
    // below obtain this string copy; they never inspect an engine object.
    if(GetCurrentThreadId()!=local_server_combat::gameThread)return;
    char text[4096]{};
    const auto b=[](bool x){return x?"true":"false";};const auto u=[](uint64_t x){return static_cast<unsigned long long>(x);};
    const int count=std::snprintf(text,sizeof(text),"{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"stopped\":%s,\"restart_required\":%s,\"busy\":%s,\"committed_kills\":%llu,\"duplicate_commits\":%llu,\"queued\":%llu,\"pending\":%llu,\"policy_skips\":%llu,\"no_firearm_skips\":%llu,\"cancelled\":%llu,\"registration_deferrals\":%llu,\"kill_refills\":%llu,\"pickup_requests\":%llu,\"pickup_refills\":%llu,\"stopped_pending\":%llu,\"last_token\":%llu,\"last_source\":%d,\"world_time\":%.17g,\"original_delay_seconds\":%.17g,\"exception_rva\":%llu,\"native_ammo_field_writes\":0,\"kill_or_score_replays\":0,\"last_readback\":{\"valid\":%s,\"index\":%zu,\"epoch\":%u,\"magazine_before\":%d,\"magazine_after\":%d,\"magazine_maximum\":%d,\"reserve_present\":%s,\"reserve_before\":%d,\"reserve_after\":%d,\"reserve_maximum\":%d}}",
        state.stage,state.error,b(state.initialized),b(state.stopped),b(state.restartRequired),b(state.busy),u(state.committedKills),u(state.duplicateCommits),u(state.queued),u(state.pending),u(state.policySkips),u(state.noFirearmSkips),u(state.cancelled),u(state.registrationDeferrals),u(state.killRefills),u(state.pickupRequests),u(state.pickupRefills),u(state.stoppedPending),u(state.lastToken),state.lastSource,state.worldTime,KillReloadDelay,u(state.exceptionRva),
        b(state.lastRefill.valid),state.lastRefill.index,state.lastRefill.epoch,state.lastRefill.magazineBefore,state.lastRefill.magazineAfter,state.lastRefill.magazineMaximum,
        b(state.lastRefill.reservePresent),state.lastRefill.reserveBefore,state.lastRefill.reserveAfter,state.lastRefill.reserveMaximum);
    if(count<0||static_cast<size_t>(count)>=sizeof(text))return;
    struct Lock {Lock(){AcquireSRWLockExclusive(&jsonLock);}~Lock(){ReleaseSRWLockExclusive(&jsonLock);}} lock;
    jsonSnapshot.assign(text,static_cast<size_t>(count));
}
inline std::string Json(){
    struct Lock {Lock(){AcquireSRWLockShared(&jsonLock);}~Lock(){ReleaseSRWLockShared(&jsonLock);}} lock;
    return jsonSnapshot;
}
inline bool Initialize(void* world){__try{const bool okay=InitializeImpl(world);CacheJson();return okay;}__except(Capture(GetExceptionInformation())){CacheJson();return false;}}
inline bool OnCommittedKill(uint64_t token,size_t source,size_t victim){__try{const bool okay=OnCommittedKillImpl(token,source,victim);CacheJson();return okay;}__except(Capture(GetExceptionInformation())){CacheJson();return false;}}
inline bool Poll(){__try{const bool okay=PollImpl();CacheJson();return okay;}__except(Capture(GetExceptionInformation())){CacheJson();return false;}}
inline bool RefillForPawn(size_t index,void* pawn){__try{const bool okay=RefillForPawnImpl(index,pawn);CacheJson();return okay;}__except(Capture(GetExceptionInformation())){CacheJson();return false;}}
inline bool Stop(){__try{const bool okay=StopImpl();CacheJson();return okay;}__except(Capture(GetExceptionInformation())){CacheJson();return false;}}
inline bool WriteDiagnostics(const wchar_t* path){
    FILE* file=nullptr;if(!path||_wfopen_s(&file,path,L"wb")||!file)return false;
    const auto text=Json();const bool good=std::fwrite(text.data(),1,text.size(),file)==text.size()&&std::fputc('\n',file)!=EOF&&std::ferror(file)==0;
    std::fclose(file);return good;
}
} // namespace local_server_kill_ammo
