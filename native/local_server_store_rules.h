#pragma once
#include "local_server_mode_startup.h"
#include "original_inventory_slots.h"
#include <array>
#include <cstdio>
#include <cstring>

// Explicit training policy for the separate local native authority in both
// configured Practice and FFA: keep the original store available and ignore
// shopping restrictions. Publish original GameInfo only; native clients retain
// their CanTransact/CanOpenShop, menu, input and purchase RPC implementations.
// Evidence: work/local-server-playable/client-presentation/store-rules.md.
namespace local_server_store_rules {
using Weak=original_inventory_slots::WeakIdentity;
struct alignas(8) Info {uint8_t bytes[64]{};};
static_assert(sizeof(Info)==64&&alignof(Info)==8);
struct Name {uint32_t words[3]{};};
struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr Profile RequiredProfiles[]{
    {0x2244350,5,0x2b47636f13bb08d7ULL},
    {0x3e8dce0,5,0x569d186f2c47f266ULL},
    {0x6ca43d0,522,0x4b7c0fd5f0277fe9ULL},
    {0x6ca0ad0,49,0xa3e289958fc0a6f1ULL},
    {0x6ca0c00,35,0x41a6568d22e8e0c6ULL},
    {0x1a7b720,196,0x4982523f72f33036ULL},
    {0x1c63870,269,0xde792136130ef719ULL},
    {0x1cf35b0,1571,0x40614eae57888396ULL},
    {0x1d4cfa0,1562,0xb050d7efb4cf1a25ULL},
    {0x3efc820,176,0x7f1113b3f79164b2ULL},
    {0x3ed83b0,1609,0x15d2d4ecc2f49cf7ULL},
};
struct State {
    bool initialized=false,published=false,policyReady=false,profilesVerified=false,metadataVerified=false;
    bool authorityVerified=false,preListen=false,freeForAll=false,publicationAttempted=false;
    bool preservedExistingPolicy=false,copyPreservedOtherBytes=false,originalNotifyReturned=false;
    bool gettersQueried=false,shoppingAllowed=false,useShoppingRestrictions=true,restartRequired=false;
    unsigned error=0;uint32_t gameThread=0,calls=0,polls=0,publications=0,notifications=0,getterCalls=0;
    int32_t netMode=-1;uint8_t storeEnabled=0,ignoreRestrictions=0,shoppingEnabled=0;
    uint8_t storeByteOffset=0,storeByteMask=0,storeFieldMask=0,ignoreByteOffset=0,ignoreByteMask=0,ignoreFieldMask=0;
    uint64_t gameInfoPropertyFlags=0;uintptr_t exceptionRva=0;
    const char* stage="authority_store_rules_not_published";
    const char* operation="none";
    void *world=nullptr,*gameState=nullptr,*mode=nullptr,*driver=nullptr,*infoType=nullptr;
    void *gameInfoProperty=nullptr,*storeProperty=nullptr,*ignoreProperty=nullptr;
    void *onRep=nullptr,*shoppingGetter=nullptr,*restrictionsGetter=nullptr;
    Weak worldId{},stateId{},modeId{};
    Info before{},submitted{},after{};
};
inline State state{};
inline const State& GetState(){return state;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Fail(unsigned code,const char* reason){state.error=code;state.policyReady=false;state.stage=reason;
    if(state.publicationAttempted)state.restartRequired=true;return false;}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool Identity(void* p,Weak& id){return original_inventory_slots::CaptureIdentity(p,id)&&Live(p,id);}
inline bool Thread(){return original_players::Thread()&&(!state.initialized||state.gameThread==GetCurrentThreadId());}
inline void* ActorWorld(void* actor){auto entry=Read<void*>(Read<void*>(actor),0x1b8);
    return entry==reinterpret_cast<void*>(engine::image+0x3ed83b0)?reinterpret_cast<void*(*)(void*)>(entry)(actor):nullptr;}
inline bool VerifyProfiles(){
    for(const auto& p:RequiredProfiles){const auto* bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes))return false;uint64_t hash=14695981039346656037ULL;
        for(size_t i=0;i<p.bytes;++i)hash=(hash^bytes[i])*1099511628211ULL;
        if(hash!=p.fnv)return false;}
    return true;
}
inline void* Property(void* type,const wchar_t* name,int32_t bytes,int32_t offset,uint64_t required=0){
    Name key{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&key,name,1);
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&key,3);
    const auto flags=Read<uint64_t>(Read<void*>(field,8),0x18);
    return field&&(flags&0x8001)==0x8001&&Read<uint64_t>(field,0x20)==Read<uint64_t>(&key)
        &&Read<int32_t>(field,0x30)==1&&Read<int32_t>(field,0x34)==bytes&&Read<int32_t>(field,0x44)==offset
        &&(Read<uint64_t>(field,0x38)&required)==required?field:nullptr;
}
inline bool NativeBool(void* property,uint8_t& byteOffset,uint8_t& byteMask,uint8_t& fieldMask){
    const auto flags=Read<uint64_t>(Read<void*>(property,8),0x18);
    byteOffset=Read<uint8_t>(property,0x79);byteMask=Read<uint8_t>(property,0x7a);fieldMask=Read<uint8_t>(property,0x7b);
    return property&&(flags&0x28001)==0x28001&&Read<uint8_t>(property,0x78)==1
        &&byteOffset==0&&byteMask&&fieldMask;
}
inline bool Function(void*& result,const wchar_t* path,uint16_t bytes,uintptr_t exec){
    result=engine::Find(path);return result&&Read<uint16_t>(result,0xbe)==bytes
        &&Read<void*>(result,0x28)==engine::Class(original_players::ShooterGameStateClass)
        &&Read<void*>(result,0xe0)==reinterpret_cast<void*>(engine::image+exec)
        &&(Read<uint32_t>(result,0xb8)&0x440)==0x400;
}
inline bool Metadata(){
    auto& d=state;d.infoType=engine::Find(L"/Script/ShooterGame.AresGameInfo");
    constexpr uint64_t netAndRepNotify=0x100000020ULL;
    d.gameInfoProperty=Property(Read<void*>(d.gameState,0x10),L"GameInfo",64,0x7d8,netAndRepNotify);
    d.storeProperty=Property(d.infoType,L"bIsStoreEnabled",1,0x31);
    d.ignoreProperty=Property(d.infoType,L"bIsIgnoreShoppingRestrictionsEnabled",1,0x36);
    d.gameInfoPropertyFlags=Read<uint64_t>(d.gameInfoProperty,0x38);
    d.metadataVerified=d.infoType&&Read<int32_t>(d.infoType,0x60)==64&&d.gameInfoProperty
        &&NativeBool(d.storeProperty,d.storeByteOffset,d.storeByteMask,d.storeFieldMask)
        &&NativeBool(d.ignoreProperty,d.ignoreByteOffset,d.ignoreByteMask,d.ignoreFieldMask)
        &&Function(d.onRep,L"/Script/ShooterGame.ShooterGameState.OnRep_GameInfo",64,0x6ca43d0)
        &&Function(d.shoppingGetter,L"/Script/ShooterGame.ShooterGameState.IsShoppingAllowed",1,0x6ca0ad0)
        &&Function(d.restrictionsGetter,L"/Script/ShooterGame.ShooterGameState.UseShoppingRestrictionRules",1,0x6ca0c00);
    return d.metadataVerified;
}
inline bool Context(){
    auto& d=state;d.authorityVerified=false;
    const auto& startup=local_server_mode_startup::GetState();
    const bool originalModeSource=startup.configured&&startup.world==d.world;
    const bool stateProvenance=originalModeSource
        ? local_server_mode_startup::OwnsStartedGameState(d.world,d.gameState,d.mode)
        : d.gameState&&d.gameState==original_players::createdGameState;
    //43CFBB0 supplies original SpawnParameters.Owner=null; AuthorityGameMode
    // is published separately at GS470. The manual trainer factory instead
    // supplies Owner=Mode. Preserve both real factory contracts unchanged.
    const bool ownerProvenance=originalModeSource?Read<void*>(d.gameState,0x210)==nullptr
        :Read<void*>(d.gameState,0x210)==d.mode;
    if(!Thread()||!Live(d.world,d.worldId)||!Live(d.gameState,d.stateId)||!Live(d.mode,d.modeId)
       ||d.world!=original_players::world||!stateProvenance
       ||Read<void*>(d.world,0x178)!=d.gameState||Read<void*>(d.world,0x170)!=d.mode
       ||!original_players::IsA(d.gameState,original_players::ShooterGameStateClass)
       ||!original_players::IsA(d.mode,0x439aa40)||Read<uint8_t>(d.gameState,0x224)!=3||Read<uint8_t>(d.mode,0x224)!=3
       ||Read<void*>(d.gameState,0x470)!=d.mode||!ownerProvenance
       ||Read<void*>(d.mode,0x4d8)!=d.gameState||ActorWorld(d.gameState)!=d.world||ActorWorld(d.mode)!=d.world
       ||Read<void*>(d.world,0xf8)||d.freeForAll!=original_players::options.freeForAll)
        return Fail(4010,"authority_store_rules_canonical_world_role_owner_or_epoch_mismatch");
    d.netMode=engine::function<int32_t(*)(void*)>(0x3efc820)(d.gameState);d.driver=Read<void*>(d.world,0x40);
    d.preListen=d.netMode==0&&!d.driver;
    if(!d.preListen&&((d.netMode!=1&&d.netMode!=2)||!d.driver||Read<void*>(d.driver,0x178)!=d.world||Read<void*>(d.driver,0xc0)))
        return Fail(4011,"authority_store_rules_require_prelisten_host_or_native_server");
    d.authorityVerified=true;return true;
}
inline bool Dispatch(void* function,void* parameters){
    auto entry=Read<void*>(Read<void*>(state.gameState),0x298);
    if(!function||!entry)return false;reinterpret_cast<void(*)(void*,void*,void*)>(entry)(state.gameState,function,parameters);return true;
}
inline bool CapturePolicy(){
    auto& d=state;d.gettersQueried=false;
    if(!original_players::Read(d.gameState,0x7d8,d.after))return Fail(4012,"authority_store_rules_game_info_readback_failed");
    d.storeEnabled=d.after.bytes[0x31];d.ignoreRestrictions=d.after.bytes[0x36];d.shoppingEnabled=Read<uint8_t>(d.gameState,0xa08);
    uint8_t allowed=255,restricted=255;
    d.operation="original_server_store_policy_getter_readbacks";
    if(!Dispatch(d.shoppingGetter,&allowed)||!Dispatch(d.restrictionsGetter,&restricted)||allowed>1||restricted>1)
        return Fail(4013,"authority_store_rules_original_policy_getter_dispatch_failed");
    d.getterCalls+=2;d.gettersQueried=true;d.shoppingAllowed=allowed!=0;d.useShoppingRestrictions=restricted!=0;
    d.policyReady=d.storeEnabled==1&&d.ignoreRestrictions==1&&d.shoppingAllowed&&!d.useShoppingRestrictions;
    if(!d.policyReady)return Fail(4014,"authority_store_rules_original_policy_or_published_values_changed");
    d.operation="none";return true;
}
inline bool PollImpl(){
    if(state.error||state.restartRequired)return false;
    if(!state.initialized)return Fail(4015,"authority_store_rules_poll_before_initialization");
    if(!Context()||!CapturePolicy())return false;++state.polls;
    state.stage="authority_store_rules_native_policy_validated_client_delivery_unverified";return true;
}
inline bool InitializeOrPublishImpl(void* world){
    auto& d=state;if(d.error||d.restartRequired)return false;++d.calls;
    if(d.initialized){if(world!=d.world)return Fail(4016,"authority_store_rules_world_replacement_requires_new_session");return PollImpl();}
    if(!world||!original_players::Thread())return Fail(4017,"authority_store_rules_original_game_thread_context_missing");
    d.world=world;d.gameState=Read<void*>(world,0x178);d.mode=Read<void*>(world,0x170);
    d.gameThread=GetCurrentThreadId();d.freeForAll=original_players::options.freeForAll;
    d.profilesVerified=VerifyProfiles();if(!d.profilesVerified)return Fail(4018,"authority_store_rules_pinned_source_profile_mismatch");
    if(!Identity(world,d.worldId)||!Identity(d.gameState,d.stateId)||!Identity(d.mode,d.modeId))
        return Fail(4019,"authority_store_rules_actual_world_state_mode_weak_identity_missing");
    if(!Context())return false;
    if(!Metadata())return Fail(4020,"authority_store_rules_net_struct_native_bool_or_notify_abi_mismatch");
    if(!original_players::Read(d.gameState,0x7d8,d.before))return Fail(4021,"authority_store_rules_original_game_info_unreadable");
    if(d.before.bytes[0x31]>1||d.before.bytes[0x36]>1)return Fail(4022,"authority_store_rules_original_native_bool_domain_mismatch");
    d.submitted=d.before;d.initialized=true;
    if(d.before.bytes[0x31]==1&&d.before.bytes[0x36]==1){d.preservedExistingPolicy=true;d.copyPreservedOtherBytes=true;}
    else{
        // Execute the two ORIGINAL generated bool setters on a complete local
        // copy. No unrelated original byte is reconstructed or defaulted.
        d.operation="original_store_bool_setters_on_full_game_info_copy";
        engine::function<void(*)(void*)>(0x2244350)(&d.submitted);
        engine::function<void(*)(void*)>(0x3e8dce0)(&d.submitted);
        d.copyPreservedOtherBytes=true;
        for(size_t i=0;i<sizeof(Info);++i)if(i!=0x31&&i!=0x36&&d.submitted.bytes[i]!=d.before.bytes[i])d.copyPreservedOtherBytes=false;
        if(d.submitted.bytes[0x31]!=1||d.submitted.bytes[0x36]!=1||!d.copyPreservedOtherBytes)
            return Fail(4023,"authority_store_rules_original_setters_changed_unrelated_game_info_bytes");
        d.publicationAttempted=true;d.operation="publish_authority_original_game_info_with_training_store_policy";
        if(!original_players::Write(d.gameState,0x7d8,d.submitted))return Fail(4024,"authority_store_rules_full_original_struct_publication_failed");
        const auto written=Read<Info>(d.gameState,0x7d8);
        if(std::memcmp(&written,&d.submitted,sizeof(Info)))return Fail(4025,"authority_store_rules_struct_publication_readback_mismatch");
        ++d.publications;d.operation="original_on_rep_game_info_previous_full_struct";
        if(!Dispatch(d.onRep,&d.before))return Fail(4026,"authority_store_rules_original_notify_dispatch_failed");
        ++d.notifications;d.originalNotifyReturned=true;
        // Original OnRep may update its own local clock/derived fields. Those
        // native side effects remain owned by the complete original handler.
    }
    if(!CapturePolicy())return false;d.published=true;
    d.stage="authority_store_rules_published_waiting_original_client_replication";return true;
}
inline LONG CaptureException(EXCEPTION_POINTERS* exception){
    if(exception&&exception->ExceptionRecord){state.error=exception->ExceptionRecord->ExceptionCode;
        const auto address=reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress);
        state.exceptionRva=address>=engine::image?address-engine::image:address;
    }else state.error=4027;
    state.restartRequired=true;state.policyReady=false;state.stage="authority_store_rules_native_exception_requires_new_session";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool InitializeOrPublish(void* world){
    __try{return InitializeOrPublishImpl(world);}__except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool Poll(){__try{return PollImpl();}__except(CaptureException(GetExceptionInformation())){return false;}}
inline void WriteState(FILE* f){
    if(!f)return;const auto& d=state;auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"published\":%s,\"policy_ready\":%s,\"profiles_verified\":%s,\"metadata_verified\":%s,\"authority_verified\":%s,\"pre_listen\":%s,\"free_for_all\":%s,\"preserved_existing_policy\":%s,\"copy_preserved_other_bytes\":%s,\"original_notify_returned\":%s,\"calls\":%u,\"polls\":%u,\"publications\":%u,\"notifications\":%u,\"getter_calls\":%u,\"native_mode\":%d,\"store_enabled\":%u,\"ignore_shopping_restrictions\":%u,\"shopping_enabled_a08\":%u,\"original_getters_queried\":%s,\"original_shopping_allowed\":%s,\"original_use_shopping_restrictions\":%s,\"game_info_property_flags\":%llu,\"world\":\"%p\",\"game_state\":\"%p\",\"mode\":\"%p\",\"operation\":\"%s\",\"exception_rva\":%llu,\"restart_required\":%s,\"client_writes\":0,\"client_policy_override\":false,\"client_delivery_verified\":false,\"physical_b_open_verified\":false}",
        d.stage,d.error,b(d.initialized),b(d.published),b(d.policyReady),b(d.profilesVerified),b(d.metadataVerified),b(d.authorityVerified),b(d.preListen),b(d.freeForAll),b(d.preservedExistingPolicy),b(d.copyPreservedOtherBytes),b(d.originalNotifyReturned),d.calls,d.polls,d.publications,d.notifications,d.getterCalls,d.netMode,unsigned(d.storeEnabled),unsigned(d.ignoreRestrictions),unsigned(d.shoppingEnabled),b(d.gettersQueried),b(d.shoppingAllowed),b(d.useShoppingRestrictions),static_cast<unsigned long long>(d.gameInfoPropertyFlags),d.world,d.gameState,d.mode,d.operation,static_cast<unsigned long long>(d.exceptionRva),b(d.restartRequired));
}
inline bool WriteDiagnostics(const wchar_t* path){FILE* f=nullptr;if(!path||_wfopen_s(&f,path,L"wb")||!f)return false;
    WriteState(f);const bool ok=std::ferror(f)==0;const bool closed=std::fclose(f)==0;return ok&&closed;}
}
