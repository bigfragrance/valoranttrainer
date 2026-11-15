#pragma once
#include "original_inventory_slots.h"
#include <array>
#include <cmath>

// Restore the original PC.UpdateRotation -> Pawn.FaceRotation leaf omitted by
// the offline NPC's direct SetControlRotation producer. No Actor rotation,
// mesh transform, facing property, role, local predicate, or shot is written
// by this adapter. Original78800d0 retains death and controller-axis gates.
// Evidence: reports/duel1-original-bot-facing.json and its pinned disassembly.
namespace original_npc_facing {
using Vec=engine::Vec;
using Weak=original_inventory_slots::WeakIdentity;
using NativeFaceRotation=void(*)(void*,const Vec*,float);
inline constexpr size_t MaxPlayers=original_players::MaxPlayers,FaceSlot=0x868;
inline constexpr uintptr_t NativeFace=0x78800d0;
struct Snapshot {
    bool valid=false,alive=false,deathOverride=false;
    uint32_t controllerAxisFlags=0;uint8_t role=0,remoteRole=0;
    void *root=nullptr,*damageHandler=nullptr;
    Vec actor{},control{},facing{};
};
struct Entry {
    bool present=false,identityValid=false,nativeEntryVerified=false,localLinksPreserved=false;
    bool yawEnabled=false,originalDeathGate=false,yawMatched=false,facingMatched=false;
    unsigned error=0;uint64_t epoch=0,calls=0,completed=0,deadSkips=0,yawDisabled=0;
    uint64_t yawMatches=0,facingMatches=0,yawMismatches=0;
    void *pawn=nullptr,*controller=nullptr,*table=nullptr,*entry=nullptr;
    Weak pawnIdentity{},controllerIdentity{};
    float deltaSeconds=0;double yawErrorBefore=0,yawErrorAfter=0,maximumYawErrorAfter=0;
    Snapshot before{},after{};const char* stage="npc_facing_not_submitted";
};
struct Diagnostics {
    bool initialized=false,restartRequired=false;unsigned error=0;
    uint64_t calls=0,completed=0,bindings=0,deadSkips=0,yawDisabled=0,identityRejects=0;
    uint64_t yawMatches=0,facingMatches=0,yawMismatches=0;
    uintptr_t exceptionRva=0;double maximumYawErrorBefore=0,maximumYawErrorAfter=0;
    const char* stage="npc_facing_not_initialized";
};
struct LocalSnapshot {
    void *pc=nullptr,*pawn=nullptr,*player=nullptr,*pcPlayer=nullptr,*playerPC=nullptr;
    Vec control{};
};
inline Diagnostics diagnostics;
inline std::array<Entry,MaxPlayers> entries{};
inline void *actorRotationFunction=nullptr,*controlRotationFunction=nullptr;
inline const Diagnostics& State(){return diagnostics;}
inline const Entry* Player(size_t index){return index<entries.size()?&entries[index]:nullptr;}
template<class T> inline T Read(const void* o,size_t offset){return original_players::Value<T>(o,offset);}
inline bool Fail(Entry* e,unsigned code,const char* stage){
    diagnostics.error=code;diagnostics.stage=stage;
    if(e){e->error=code;e->stage=stage;}return false;
}
inline bool Event(void* o,void* function,Vec& result){
    auto table=Read<void**>(o,0);auto process=Read<void*>(table,0x298);
    if(!o||!function||!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(o,function,&result);
    return original_players::Finite(result);
}
inline double Angle(double value){double v=std::fmod(value+180.,360.);if(v<0)v+=360.;return v-180.;}
inline bool SameRotation(const Vec& a,const Vec& b,double tolerance=.000001){
    return std::abs(Angle(a.x-b.x))<=tolerance&&std::abs(Angle(a.y-b.y))<=tolerance&&std::abs(Angle(a.z-b.z))<=tolerance;
}
inline bool Initialize(){
    if(!original_players::Thread()||diagnostics.restartRequired)return false;
    if(diagnostics.initialized)return true;
    actorRotationFunction=engine::Find(L"/Script/Engine.Actor.K2_GetActorRotation");
    controlRotationFunction=engine::Find(L"/Script/Engine.Controller.GetControlRotation");
    if(!actorRotationFunction||!controlRotationFunction
       ||Read<uint16_t>(actorRotationFunction,0xbe)!=24||Read<uint16_t>(controlRotationFunction,0xbe)!=24)
        return Fail(nullptr,2418,"npc_facing_original_rotation_query_abi_mismatch");
    diagnostics.initialized=true;diagnostics.error=0;diagnostics.stage="npc_facing_original_queries_ready";return true;
}
inline LocalSnapshot Local(){
    LocalSnapshot s;s.pc=original_players::activeController;s.pawn=original_players::activePawn;s.player=original_players::localPlayer;
    s.pcPlayer=Read<void*>(s.pc,0x510);s.playerPC=Read<void*>(s.player,0x38);s.control=Read<Vec>(s.pc,0x4e8);return s;
}
inline bool LocalPreserved(const LocalSnapshot& s){
    const auto now=Local();return s.pc&&s.pawn&&s.player&&s.pcPlayer==s.player&&s.playerPC==s.pc
        &&s.pc==now.pc&&s.pawn==now.pawn&&s.player==now.player
        &&s.pcPlayer==now.pcPlayer&&s.playerPC==now.playerPC
        &&original_players::Finite(s.control)&&original_players::Finite(now.control)&&SameRotation(s.control,now.control);
}
inline bool NotLocal(void* pc,void* pawn){
    if(!pc||!pawn||pc==original_players::activeController||pawn==original_players::activePawn||Read<void*>(pc,0x510))return false;
    auto gi=Read<void*>(original_players::world,0x1d8);auto locals=Read<void**>(gi,0x40);const int32_t count=Read<int32_t>(gi,0x48);
    if(!locals||count<1||count>64)return false;
    for(int32_t n=0;n<count;++n){auto player=Read<void*>(locals,static_cast<size_t>(n)*8);
        if(!player||Read<void*>(player,0x38)==pc)return false;}
    return true;
}
inline bool CaptureSnapshot(void* pawn,void* pc,Snapshot& s){
    s={};s.root=Read<void*>(pawn,0x290);s.damageHandler=Read<void*>(pawn,0xc70);
    if(!original_players::Read(pawn,0x470,s.controllerAxisFlags)||!original_players::Read(pawn,0x224,s.role)
       ||!original_players::Read(pawn,0x128,s.remoteRole)||!original_players::Read(pawn,0x1158,s.facing))return false;
    uint8_t alive=0,overrideDeath=0;
    if((s.damageHandler&&!original_players::Read(s.damageHandler,0x1f9,alive))
       ||!original_players::Read(pawn,0x1440,overrideDeath))return false;
    s.alive=alive!=0;s.deathOverride=overrideDeath!=0;
    if(!original_players::Finite(s.facing)){s.facing={};return false;}
    if(!Event(pawn,actorRotationFunction,s.actor)||!Event(pc,controlRotationFunction,s.control)){
        if(!original_players::Finite(s.actor))s.actor={};if(!original_players::Finite(s.control))s.control={};return false;}
    s.valid=s.root&&original_players::Finite(s.actor)&&original_players::Finite(s.control);return s.valid;
}
inline bool ApplyImpl(size_t index,void* pawn,void* pc,uint64_t epoch,float deltaSeconds){
    if(!original_players::Thread()||diagnostics.restartRequired||!Initialize()||index>=entries.size())return false;
    auto& e=entries[index];const auto& roster=original_players::players[index];
    if(!std::isfinite(deltaSeconds)||deltaSeconds<0||deltaSeconds>.1f||!epoch)
        return Fail(&e,2419,"npc_facing_invalid_original_frame_delta_or_epoch");
    if(roster.local||static_cast<int>(index)==original_players::localIndex||!roster.ready||!roster.registered
       ||!roster.characterInitialized||roster.character!=pawn||roster.controller!=pc
       ||!original_players::IsA(pawn,original_players::ShooterCharacterClass)
       ||!original_players::IsA(pc,original_players::ShooterControllerClass)||!NotLocal(pc,pawn)){
        ++diagnostics.identityRejects;return Fail(&e,2420,"npc_facing_requires_real_nonlocal_roster_pair");}
    if(!e.present||e.pawn!=pawn||e.controller!=pc||e.epoch!=epoch){
        e={};e.present=true;e.pawn=pawn;e.controller=pc;e.epoch=epoch;++diagnostics.bindings;
        if(!original_inventory_slots::CaptureIdentity(pawn,e.pawnIdentity)
           ||!original_inventory_slots::CaptureIdentity(pc,e.controllerIdentity))
            return Fail(&e,2421,"npc_facing_weak_identity_capture_failed");
    }
    e.identityValid=original_inventory_slots::ResolveIdentity(e.pawnIdentity)==pawn
        &&original_inventory_slots::ResolveIdentity(e.controllerIdentity)==pc;
    if(!e.identityValid){++diagnostics.identityRejects;return Fail(&e,2422,"npc_facing_spawn_identity_expired");}
    e.table=Read<void*>(pawn,0);e.entry=Read<void*>(e.table,FaceSlot);
    e.nativeEntryVerified=e.entry==reinterpret_cast<void*>(engine::image+NativeFace);
    if(!e.nativeEntryVerified)return Fail(&e,2423,"npc_facing_original_virtual_entry_changed");
    e.deltaSeconds=deltaSeconds;e.after={};
    if(!CaptureSnapshot(pawn,pc,e.before))return Fail(&e,2424,"npc_facing_before_readback_failed");
    e.originalDeathGate=e.before.damageHandler&&!e.before.alive&&!e.before.deathOverride;
    e.yawEnabled=(e.before.controllerAxisFlags&2)!=0;
    // Retirement can arrive after the original death code unpossessed the PC.
    // The proven native death gate makes FaceRotation a no-op in that case.
    // Preserve the same no-op without requiring/rebuilding possession.
    if(e.originalDeathGate){e.after=e.before;++e.deadSkips;++diagnostics.deadSkips;e.error=0;e.stage="npc_facing_original_death_gate_noop";diagnostics.error=0;diagnostics.stage=e.stage;return true;}
    if(Read<void*>(pawn,0x4a0)!=pc||Read<void*>(pc,0x4b0)!=pawn)
        return Fail(&e,2425,"npc_facing_original_possession_pair_not_ready");
    if(!e.yawEnabled){++e.yawDisabled;++diagnostics.yawDisabled;return Fail(&e,2426,"npc_facing_original_controller_yaw_flag_disabled");}
    const auto local=Local();e.localLinksPreserved=false;
    e.yawErrorBefore=std::abs(Angle(e.before.actor.y-e.before.control.y));
    if(e.yawErrorBefore>diagnostics.maximumYawErrorBefore)diagnostics.maximumYawErrorBefore=e.yawErrorBefore;
    ++e.calls;++diagnostics.calls;
    // Same ABI as original492f9b8..9d9: this RCX, &GetControlRotation RDX,
    // native frame delta XMM2. Dispatch the intact preceding vtable's slot;
    // all existing actor observers remain installed and unmodified.
    reinterpret_cast<NativeFaceRotation>(e.entry)(pawn,&e.before.control,deltaSeconds);
    ++e.completed;++diagnostics.completed;
    if(!CaptureSnapshot(pawn,pc,e.after))return Fail(&e,2427,"npc_facing_after_readback_failed");
    e.localLinksPreserved=LocalPreserved(local);
    if(!e.localLinksPreserved||Read<void*>(pawn,0)!=e.table
       ||e.before.controllerAxisFlags!=e.after.controllerAxisFlags
       ||e.before.role!=e.after.role||e.before.remoteRole!=e.after.remoteRole
       ||!SameRotation(e.before.control,e.after.control))
        return Fail(&e,2428,"npc_facing_original_call_changed_identity_or_control");
    e.yawErrorAfter=std::abs(Angle(e.after.actor.y-e.after.control.y));
    if(e.yawErrorAfter>e.maximumYawErrorAfter)e.maximumYawErrorAfter=e.yawErrorAfter;
    if(e.yawErrorAfter>diagnostics.maximumYawErrorAfter)diagnostics.maximumYawErrorAfter=e.yawErrorAfter;
    e.yawMatched=e.yawErrorAfter<=.01;e.facingMatched=SameRotation(e.after.facing,e.after.control);
    if(e.yawMatched){++e.yawMatches;++diagnostics.yawMatches;}else{++e.yawMismatches;++diagnostics.yawMismatches;}
    if(e.facingMatched){++e.facingMatches;++diagnostics.facingMatches;}
    e.error=0;e.stage=e.yawMatched&&e.facingMatched?"npc_facing_native_yaw_and_facing_confirmed":"npc_facing_native_readback_not_converged";
    diagnostics.error=0;diagnostics.stage=e.stage;return true;
}
inline LONG Capture(EXCEPTION_POINTERS* e){
    const auto address=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=address>=engine::image?address-engine::image:address;diagnostics.restartRequired=true;
    Fail(nullptr,e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0002429U,"npc_facing_original_exception_requires_new_session");return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Apply(size_t index,void* pawn,void* pc,uint64_t epoch,float deltaSeconds=0){
    __try{return ApplyImpl(index,pawn,pc,epoch,deltaSeconds);}__except(Capture(GetExceptionInformation())){return false;}
}
inline void WriteSnapshot(FILE* f,const Snapshot& s){
    std::fprintf(f,"{\"valid\":%s,\"root\":\"%p\",\"damage_handler\":\"%p\",\"alive\":%s,\"death_override_1440\":%s,\"controller_axis_flags\":%u,\"role\":%u,\"remote_role\":%u,\"actor_rotation\":",
        s.valid?"true":"false",s.root,s.damageHandler,s.alive?"true":"false",s.deathOverride?"true":"false",s.controllerAxisFlags,static_cast<unsigned>(s.role),static_cast<unsigned>(s.remoteRole));
    original_players::Vector(f,s.actor);std::fputs(",\"control_rotation\":",f);original_players::Vector(f,s.control);
    std::fputs(",\"facing_control_rotation\":",f);original_players::Vector(f,s.facing);std::fputc('}',f);
}
inline void WriteEntry(FILE* f,size_t index){
    if(index>=entries.size()){std::fputs("null",f);return;}const auto& e=entries[index];const auto b=[](bool x){return x?"true":"false";};
    std::fprintf(f,"{\"present\":%s,\"epoch\":%llu,\"identity_valid\":%s,\"native_entry_verified\":%s,\"entry\":\"%p\",\"original_delta_seconds\":%.9g,\"calls\":%llu,\"completed\":%llu,\"dead_skips\":%llu,\"yaw_disabled\":%llu,\"yaw_enabled\":%s,\"original_death_gate\":%s,\"local_links_preserved\":%s,\"yaw_matched\":%s,\"facing_matched\":%s,\"yaw_matches\":%llu,\"facing_matches\":%llu,\"yaw_mismatches\":%llu,\"yaw_error_before\":%.9g,\"yaw_error_after\":%.9g,\"maximum_yaw_error_after\":%.9g,\"error\":%u,\"stage\":",
        b(e.present),static_cast<unsigned long long>(e.epoch),b(e.identityValid),b(e.nativeEntryVerified),e.entry,static_cast<double>(e.deltaSeconds),static_cast<unsigned long long>(e.calls),static_cast<unsigned long long>(e.completed),static_cast<unsigned long long>(e.deadSkips),static_cast<unsigned long long>(e.yawDisabled),b(e.yawEnabled),b(e.originalDeathGate),b(e.localLinksPreserved),b(e.yawMatched),b(e.facingMatched),static_cast<unsigned long long>(e.yawMatches),static_cast<unsigned long long>(e.facingMatches),static_cast<unsigned long long>(e.yawMismatches),e.yawErrorBefore,e.yawErrorAfter,e.maximumYawErrorAfter,e.error);
    original_players::Text(f,e.stage);std::fputs(",\"before\":",f);WriteSnapshot(f,e.before);std::fputs(",\"after\":",f);WriteSnapshot(f,e.after);std::fputc('}',f);
}
inline void WriteSummary(FILE* f){
    std::fprintf(f,"{\"initialized\":%s,\"error\":%u,\"restart_required\":%s,\"exception_rva\":%llu,\"bindings\":%llu,\"calls\":%llu,\"completed\":%llu,\"dead_skips\":%llu,\"yaw_disabled\":%llu,\"identity_rejects\":%llu,\"yaw_matches\":%llu,\"facing_matches\":%llu,\"yaw_mismatches\":%llu,\"maximum_yaw_error_before\":%.9g,\"maximum_yaw_error_after\":%.9g,\"stage\":",
        diagnostics.initialized?"true":"false",diagnostics.error,diagnostics.restartRequired?"true":"false",static_cast<unsigned long long>(diagnostics.exceptionRva),static_cast<unsigned long long>(diagnostics.bindings),static_cast<unsigned long long>(diagnostics.calls),static_cast<unsigned long long>(diagnostics.completed),static_cast<unsigned long long>(diagnostics.deadSkips),static_cast<unsigned long long>(diagnostics.yawDisabled),static_cast<unsigned long long>(diagnostics.identityRejects),static_cast<unsigned long long>(diagnostics.yawMatches),static_cast<unsigned long long>(diagnostics.facingMatches),static_cast<unsigned long long>(diagnostics.yawMismatches),diagnostics.maximumYawErrorBefore,diagnostics.maximumYawErrorAfter);
    original_players::Text(f,diagnostics.stage);std::fputc('}',f);
}
}
