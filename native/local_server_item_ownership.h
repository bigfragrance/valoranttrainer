#pragma once
#include "original_inventory_slots.h"
#include <array>
#include <cstdint>
#include <cstdio>

// Explicit missing HOST ownership policy, using the retained AActor native
// SetAutonomousProxy API. The retail AresItem server grant/owner bodies are
// stripped; this is not claimed to recover those bodies. Only the admitted
// human's actual owned grant can publish autonomous remote simulation. Native
// ActorChannel owner filtering, Role replication and client RoleChanged own
// all receiving behavior. No client Role, enabled flag or state pointer writes.
namespace local_server_item_ownership {
using Weak=original_inventory_slots::WeakIdentity;
inline constexpr size_t SlotCount=17;
enum class Source:uint8_t {PublishedSlot,PendingAuthorityGrant};
enum class Status:uint8_t {NotObserved,NonHumanUnchanged,WaitingOwner,Published,AlreadyPublished,Failed};
struct State {
    bool profilesVerified=false,restartRequired=false;unsigned error=0;
    uint64_t requests=0,publications=0,alreadyPublished=0,nonHumanUnchanged=0,waiting=0;
    uintptr_t exceptionRva=0;DWORD gameThread=0;
    Status status=Status::NotObserved;Source source=Source::PublishedSlot;
    const char* stage="native_item_ownership_not_observed";
    void *world=nullptr,*controller=nullptr,*playerState=nullptr,*pawn=nullptr,*inventory=nullptr,*item=nullptr,*slot=nullptr,*connection=nullptr;
    uint8_t itemRole=0,remoteBefore=0,remoteAfter=0;uint32_t checkedSlots=0;
};
struct ClientObservation {
    unsigned items=0,autonomousItems=0,simulatedItems=0,authorityItems=0,invalidRoles=0;
    unsigned machines=0,enabledMachines=0,predictedStates=0,authorityStates=0,roleSelectedStates=0;
    unsigned ownedItems=0,validSlots=0;bool complete=false,allNativePredictionReady=false;
};
struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr Profile Profiles[]{
    {0x3efa490,438,0xff38cf4b45f539ceULL},
    {0x3ed83b0,60,0xb0370ccdf6164999ULL},{0x3ed83ec,1515,0xeebf5d5f356d29e0ULL},{0x3ed89d7,34,0x3250c3a3c23e774eULL},
    {0x3edaad0,32,0x5841da428e817d8aULL},
    {0x70ed640,78,0xc452c3a80e224204ULL},{0x70ed690,46,0x499020e562cdc62dULL},
    {0x70ed6be,307,0xc31b1a1edf9a05a5ULL},{0x70ed7f1,17,0x6e39167453336a6eULL}
};
inline State state;
inline const State& GetState(){return state;}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Capture(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline bool Thread(){return engine::image&&original_players::Thread()&&!state.restartRequired
    &&(!state.gameThread||state.gameThread==GetCurrentThreadId());}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.status=Status::Failed;state.stage=stage;return false;}
inline bool Wait(const char* stage){++state.waiting;state.status=Status::WaitingOwner;state.stage=stage;return true;}
inline uint64_t Hash(const void* bytes,size_t count){uint64_t h=14695981039346656037ULL;auto data=static_cast<const uint8_t*>(bytes);
    for(size_t i=0;i<count;++i)h=(h^data[i])*1099511628211ULL;return h;}
inline bool VerifyProfiles(){
    if(state.profilesVerified)return true;
    for(const auto& profile:Profiles){auto code=reinterpret_cast<const void*>(engine::image+profile.rva);
        if(!original_players::Readable(code,profile.bytes)||Hash(code,profile.bytes)!=profile.fnv)
            return Fail(3570,"native_item_ownership_retained_api_profile_mismatch");}
    state.profilesVerified=true;return true;
}
inline void* ActorWorld(void* actor){
    auto method=Read<void*>(Read<void*>(actor),0x1b8);
    return method==reinterpret_cast<void*>(engine::image+0x3ed83b0)
        ?reinterpret_cast<void*(*)(void*)>(method)(actor):nullptr;
}
inline bool ValidSlot(void* inventory,void* slot,size_t index){
    return original_players::IsA(slot,0x68f63c0)&&Read<void*>(slot,0x28)==inventory
        &&Read<void*>(slot,0x40)==inventory&&Read<uint8_t>(slot,0x30)==index;
}
inline bool Contains(void* slot,void* item){
    if(!original_players::IsA(slot,0x69cf3d0))return Read<void*>(slot,0x38)==item;
    auto data=Read<void**>(slot,0x50);const auto count=Read<int32_t>(slot,0x58),capacity=Read<int32_t>(slot,0x5c);
    if(count<0||count>256||capacity<count||(!data&&count)||!original_players::Readable(data,static_cast<size_t>(count)*8))return false;
    for(int32_t i=0;i<count;++i)if(data[i]==item)return true;return false;
}
inline bool ActualHuman(void* world,void* pc,void* ps,void* pawn,void* inventory,void*& connection){
    connection=Read<void*>(pc,0x510);
    if(!original_players::IsA(pc,original_players::ShooterControllerClass)||!original_players::IsA(ps,original_players::ShooterStateClass)
       ||!original_players::IsA(pawn,original_players::ShooterCharacterClass)||!original_players::IsA(inventory,0x6499330)
       ||Read<uint8_t>(pc,0x224)!=3||Read<uint8_t>(ps,0x224)!=3||Read<uint8_t>(pawn,0x224)!=3||Read<uint8_t>(pawn,0x128)!=2
       ||Read<void*>(pc,0x470)!=ps||Read<void*>(pc,0x4b0)!=pawn||Read<void*>(ps,0x210)!=pc
       ||Read<void*>(pawn,0x4a0)!=pc||Read<void*>(pawn,0x488)!=ps||Read<void*>(pawn,0xc08)!=inventory
       ||Read<void*>(inventory,0xc8)!=pawn||ActorWorld(pawn)!=world||ActorWorld(pc)!=world||ActorWorld(ps)!=world)return false;
    auto driver=Read<void*>(world,0x40);auto peers=Read<void**>(driver,0xc8);
    return connection&&driver&&Read<void*>(driver,0x178)==world&&!Read<void*>(driver,0xc0)
        &&Read<int32_t>(driver,0xd0)==1&&peers&&Read<void*>(peers)==connection
        &&Read<void*>(connection,0x60)==driver&&Read<void*>(connection,0x38)==pc&&Read<void*>(connection,0xa0)==pc
        &&Read<int32_t>(connection,0x13c)==3&&Read<uint8_t>(connection,0x1d8)==3;
}
// A pending grant may call this after native Actor.Owner/MyPawn callbacks and
// authored SlotType are established, before Contents publication/ActorChannel.
// The caller is the validated host grant producer, never a client pointer or RPC.
// PublishedSlot additionally requires actual slot containment. NPC PCs have no
// Player/connection and are deliberately unchanged.
inline bool PrepareImpl(void* world,void* pc,void* ps,void* pawn,void* inventory,void* item,
                        void* targetSlot=nullptr,Source source=Source::PublishedSlot){
    if(!Thread())return false;++state.requests;state.source=source;state.checkedSlots=0;
    state.world=world;state.controller=pc;state.playerState=ps;state.pawn=pawn;state.inventory=inventory;state.item=item;state.slot=targetSlot;
    state.remoteBefore=state.remoteAfter=Read<uint8_t>(item,0x128);state.itemRole=Read<uint8_t>(item,0x224);
    if(!pc||!Read<void*>(pc,0x510)){++state.nonHumanUnchanged;state.status=Status::NonHumanUnchanged;
        state.stage="native_item_ownership_nonhuman_policy_unchanged";return true;}
    if(!VerifyProfiles())return false;state.gameThread=GetCurrentThreadId();
    void* connection=nullptr;
    if(!ActualHuman(world,pc,ps,pawn,inventory,connection))return Fail(3571,"native_item_ownership_requires_actual_single_admitted_human");
    state.connection=connection;
    if(!item||!original_players::IsA(item,0x645c660)||state.itemRole!=3)return Fail(3572,"native_item_ownership_requires_actual_authority_equippable");
    if(Read<void*>(item,0x210)!=pawn||Read<void*>(item,0x4a8)!=pawn)return Wait("native_item_ownership_waiting_original_owner_callbacks");
    if(ActorWorld(item)!=world||(Read<uint8_t>(item,0x11a)&0x10)==0)
        return Fail(3573,"native_item_ownership_world_or_native_replication_gate_invalid");
    if(source!=Source::PublishedSlot&&source!=Source::PendingAuthorityGrant)return Fail(3574,"native_item_ownership_grant_source_invalid");
    size_t selected=SlotCount;void* actualSlot=nullptr;
    for(size_t index=0;index<SlotCount;++index){auto slot=Read<void*>(inventory,0x1b8+index*8);
        if(!ValidSlot(inventory,slot,index))return Wait("native_item_ownership_waiting_actual_seventeen_native_slots");
        ++state.checkedSlots;
        if((targetSlot&&targetSlot==slot)||(!targetSlot&&Contains(slot,item))){if(actualSlot)return Fail(3575,"native_item_ownership_item_slot_ambiguous");actualSlot=slot;selected=index;}}
    state.slot=actualSlot;
    if(!actualSlot||selected!=Read<uint8_t>(item,0x4a0)||(source==Source::PublishedSlot&&!Contains(actualSlot,item)))
        return Fail(3576,"native_item_ownership_authored_slot_or_published_membership_invalid");
    auto getConnection=Read<void*>(Read<void*>(item),0x590);
    if(getConnection!=reinterpret_cast<void*>(engine::image+0x3edaad0)
       ||reinterpret_cast<void*(*)(void*)>(getConnection)(item)!=connection)
        return Fail(3577,"native_item_ownership_original_owner_connection_does_not_match_admission");
    std::array<void*,9> objects{world,pc,ps,pawn,inventory,item,actualSlot,connection,Read<void*>(world,0x40)};
    std::array<Weak,9> identities{};
    for(size_t i=0;i<objects.size();++i)if(!Capture(objects[i],identities[i]))return Fail(3578,"native_item_ownership_original_weak_identity_missing");
    if(state.remoteBefore!=1&&state.remoteBefore!=2)return Fail(3579,"native_item_ownership_unexpected_authority_remote_role");
    if(state.remoteBefore==2){++state.alreadyPublished;state.status=Status::AlreadyPublished;state.stage="native_item_ownership_autonomous_remote_policy_already_published";return true;}
    // Original API performs its own bReplicates check, RemoteRole publication,
    // forced property comparison and original network policy notifications.
    engine::function<void(*)(void*,uint8_t,uint8_t)>(0x3efa490)(item,1,1);
    state.remoteAfter=Read<uint8_t>(item,0x128);
    for(size_t i=0;i<objects.size();++i)if(!Live(objects[i],identities[i]))return Fail(3580,"native_item_ownership_weak_identity_changed_during_original_api");
    void* afterConnection=nullptr;
    if(state.remoteAfter!=2||Read<uint8_t>(item,0x224)!=3||Read<void*>(item,0x210)!=pawn||Read<void*>(item,0x4a8)!=pawn
       ||!ActualHuman(world,pc,ps,pawn,inventory,afterConnection)||afterConnection!=connection)
        return Fail(3581,"native_item_ownership_original_policy_or_owner_readback_failed");
    ++state.publications;state.status=Status::Published;state.stage="native_item_ownership_host_policy_published_client_prediction_unverified";return true;
}
inline LONG CaptureException(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0003570U;
    const auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;state.restartRequired=true;state.status=Status::Failed;
    state.stage="native_item_ownership_exception_requires_new_owned_pair";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Prepare(void* world,void* pc,void* ps,void* pawn,void* inventory,void* item,
                    void* targetSlot=nullptr,Source source=Source::PublishedSlot){
    __try{return PrepareImpl(world,pc,ps,pawn,inventory,item,targetSlot,source);}
    __except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool ObserveClientItem(void* pawn,void* item,ClientObservation& result){
    if(!item)return true;if(!original_players::IsA(item,0x645c660))return true;
    ++result.items;const auto role=Read<uint8_t>(item,0x224);
    if(role==1)++result.simulatedItems;else if(role==2)++result.autonomousItems;else if(role==3)++result.authorityItems;else ++result.invalidRoles;
    if(Read<void*>(item,0x210)==pawn&&Read<void*>(item,0x4a8)==pawn)++result.ownedItems;
    auto machines=Read<void**>(item,0xe30);const auto count=Read<int32_t>(item,0xe38);
    if(count<0||count>64||(!machines&&count)||(count&&!original_players::Readable(machines,static_cast<size_t>(count)*8)))return false;
    for(int32_t i=0;i<count;++i){auto machine=machines[i];if(!original_players::IsA(machine,0x67722c0)||Read<void*>(machine,0xc8)!=item)return false;
        ++result.machines;if(Read<uint8_t>(machine,0x348))++result.enabledMachines;
        if(Read<void*>(machine,0x1b0))++result.predictedStates;if(Read<void*>(machine,0x1c8))++result.authorityStates;
        if(Read<void*>(machine,role==3?0x1c8:0x1b0))++result.roleSelectedStates;}
    return true;
}
inline bool ObserveClientImpl(void* pawn,ClientObservation& result){
    result={};if(!Thread()||Read<uint8_t>(pawn,0x224)!=2)return false;
    auto inventory=Read<void*>(pawn,0xc08);if(!original_players::IsA(inventory,0x6499330)||Read<void*>(inventory,0xc8)!=pawn)return false;
    std::array<void*,512> observed{};size_t observedCount=0;
    for(size_t index=0;index<SlotCount;++index){auto slot=Read<void*>(inventory,0x1b8+index*8);if(!ValidSlot(inventory,slot,index))return true;++result.validSlots;
        auto visit=[&](void* item){if(!item)return true;for(size_t i=0;i<observedCount;++i)if(observed[i]==item)return true;
            if(observedCount==observed.size())return false;observed[observedCount++]=item;return ObserveClientItem(pawn,item,result);};
        if(original_players::IsA(slot,0x69cf3d0)){auto data=Read<void**>(slot,0x50);const auto count=Read<int32_t>(slot,0x58),capacity=Read<int32_t>(slot,0x5c);
            if(count<0||count>256||capacity<count||(!data&&count)||(count&&!original_players::Readable(data,static_cast<size_t>(count)*8)))return false;
            for(int32_t i=0;i<count;++i)if(!visit(data[i]))return false;
        }else if(!visit(Read<void*>(slot,0x38)))return false;}
    result.complete=result.validSlots==SlotCount&&result.items>0&&result.items==result.ownedItems&&!result.authorityItems&&!result.invalidRoles;
    result.allNativePredictionReady=result.complete&&result.items==result.autonomousItems&&result.machines>0
        &&result.enabledMachines==result.machines&&result.predictedStates==result.machines&&result.roleSelectedStates==result.machines;return true;
}
inline bool ObserveClient(void* pawn,ClientObservation& result){
    __try{return ObserveClientImpl(pawn,result);}__except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    std::fprintf(file,"{\"policy\":\"missing_host_owned_item_simulation_policy_via_retained_native_api\",\"stage\":\"%s\",\"error\":%u,\"status\":%u,\"source\":%u,\"profiles_verified\":%s,\"restart_required\":%s,\"requests\":%llu,\"publications\":%llu,\"already_published\":%llu,\"nonhuman_unchanged\":%llu,\"waiting\":%llu,\"checked_slots\":%u,\"item_role\":%u,\"remote_before\":%u,\"remote_after\":%u,\"exception_rva\":%llu,\"original_item_server_body_recovered\":false,\"client_role_writes\":false,\"machine_field_writes\":false,\"client_prediction_proven\":false}\n",
        state.stage,state.error,static_cast<unsigned>(state.status),static_cast<unsigned>(state.source),state.profilesVerified?"true":"false",state.restartRequired?"true":"false",
        static_cast<unsigned long long>(state.requests),static_cast<unsigned long long>(state.publications),static_cast<unsigned long long>(state.alreadyPublished),
        static_cast<unsigned long long>(state.nonHumanUnchanged),static_cast<unsigned long long>(state.waiting),state.checkedSlots,static_cast<unsigned>(state.itemRole),
        static_cast<unsigned>(state.remoteBefore),static_cast<unsigned>(state.remoteAfter),static_cast<unsigned long long>(state.exceptionRva));
    const bool okay=std::ferror(file)==0;std::fclose(file);return okay;
}
} // namespace local_server_item_ownership
