#pragma once
#include "original_players.h"

// One missing authority rule for the separate, offline GameModeBase host.
// Mirrors only the GameState publication in original AGameMode.SetMatchState
// (43cb864..43cb894). The original GameState RepNotify owns all world/actor
// BeginPlay behavior. Evidence: work/local-server-playable/character-lifecycle/.
namespace local_server_character_lifecycle {
struct NativeName {uint32_t comparison=0,number=0,display=0;};
static_assert(sizeof(NativeName)==12);
struct Diagnostics {
    bool started=false,profilesVerified=false,publicationAttempted=false;
    bool originalNotifyReturned=false,nativeBeginPlayPublished=false;
    bool restartRequired=false;
    unsigned error=0;
    uintptr_t exceptionRva=0;
    const char* stage="local_network_match_start_not_published";
    const char* operation="none";
    void *world=nullptr,*mode=nullptr,*gameState=nullptr;
    NativeName oldMatch{},oldPrevious{},publishedMatch{},resultPrevious{};
    uint8_t authorityRole=0,replicatedHasBegunPlay=0,worldFlags156=0;
};
inline Diagnostics state;
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* p,size_t o){return original_players::Value<T>(p,o);}
inline bool Fail(unsigned code,const char* reason){
    state.started=false;state.error=code;state.stage=reason;return false;
}
inline bool Same(const NativeName& a,const NativeName& b){
    // Native FName identity comparisons in 443c750 compare the first eight
    // bytes. The display index is still copied, exactly as in 43cb7e0.
    return a.comparison==b.comparison&&a.number==b.number;
}
inline bool Exact(const NativeName& a,const NativeName& b){
    return Same(a,b)&&a.display==b.display;
}
inline bool VerifyProfiles(){
    struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
    static constexpr Profile profiles[]{
        {0x43cb7e0,282,0xcfdcb0de0ec90a9bULL}, // original authority GS publication
        {0x443c750,177,0x6b082dee551913dcULL}, // full original OnRep_MatchState
        {0x443c640,85,0x6396acfda42acfc6ULL}, // original HandleMatchHasStarted
        {0x7179ba0,484,0x4bfee983d61e9c25ULL}, // actual Shooter/Ares waiting handler
        {0x7189730,174,0x81a51f691364c62fULL}, // actual replicated-begun-play handler
        {0x443bbe0,133,0x276a8dcf827ecb15ULL}, // both FNames default to EnteringMap
        {0x7179400,361,0xd5f8ef5da7217e6bULL}, // Ares GS retains those original defaults
        {0xaf4810,179,0xc5a8f7b2602a2ddbULL}, // original InProgress FName initializer
    };
    for(const auto& p:profiles){
        const auto* bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        uint64_t hash=14695981039346656037ULL;
        for(size_t i=0;i<p.bytes;++i)hash=(hash^bytes[i])*1099511628211ULL;
        if(hash!=p.fnv)return false;
    }
    return true;
}
inline bool CaptureResult(){
    const auto progress=Read<NativeName>(reinterpret_cast<void*>(engine::image),0xd7e6d70);
    state.publishedMatch=Read<NativeName>(state.gameState,0x4c0);
    state.resultPrevious=Read<NativeName>(state.gameState,0x4cc);
    state.replicatedHasBegunPlay=Read<uint8_t>(state.gameState,0x490);
    state.worldFlags156=Read<uint8_t>(state.world,0x156);
    state.nativeBeginPlayPublished=Exact(state.publishedMatch,progress)
        &&Exact(state.resultPrevious,progress)&&state.replicatedHasBegunPlay==1
        &&(state.worldFlags156&0x80)!=0;
    return state.nativeBeginPlayPublished;
}
inline bool PublishAuthorityMatchStartedImpl(void* world){
    if(state.error||state.restartRequired)return false;
    if(!original_players::Thread())return Fail(3480,"local_network_match_start_requires_original_game_thread");
    if(state.started){
        if(state.world!=world||Read<void*>(world,0x178)!=state.gameState||!CaptureResult())
            return Fail(3481,"local_network_started_match_identity_or_state_changed");
        return true;
    }
    if(state.publicationAttempted)return Fail(3482,"local_network_partial_match_start_requires_new_session");
    state.world=world;state.mode=Read<void*>(world,0x170);state.gameState=Read<void*>(world,0x178);
    state.stage="local_network_validate_authority_match_publication";
    if(!original_players::IsA(state.mode,0x439aa40)
       ||original_players::IsA(state.mode,0x43c93e0)
       ||!original_players::IsA(state.gameState,original_players::ShooterGameStateClass)
       ||!original_players::IsA(state.gameState,0x43c9540)
       ||Read<void*>(state.mode,0x4d8)!=state.gameState
       ||Read<void*>(state.gameState,0x470)!=state.mode
       ||Read<uint8_t>(state.gameState,0x224)!=3
       ||(Read<uint8_t>(state.gameState,0x11c)&0x30)!=0x20
       ||(Read<uint8_t>(world,0x156)&0x80)==0)
        return Fail(3483,"local_network_match_start_requires_registered_begun_authority_shooter_state_and_base_mode");
    state.authorityRole=3;
    if(!VerifyProfiles())return Fail(3484,"local_network_match_start_native_profile_mismatch");
    state.profilesVerified=true;
    auto table=Read<void**>(state.gameState,0);
    if(Read<uintptr_t>(table,0x810)!=engine::image+0x7179ba0
       ||Read<uintptr_t>(table,0x818)!=engine::image+0x443c640
       ||Read<uintptr_t>(table,0x830)!=engine::image+0x443c750
       ||Read<uintptr_t>(table,0x7e8)!=engine::image+0x7189730)
        return Fail(3485,"local_network_match_start_original_virtual_contract_mismatch");
    const auto entering=Read<NativeName>(reinterpret_cast<void*>(engine::image),0xd7e6f48);
    const auto progress=Read<NativeName>(reinterpret_cast<void*>(engine::image),0xd7e6d70);
    if(!entering.comparison||!progress.comparison||Same(entering,progress))
        return Fail(3486,"local_network_original_match_names_unavailable");
    state.oldMatch=Read<NativeName>(state.gameState,0x4c0);
    state.oldPrevious=Read<NativeName>(state.gameState,0x4cc);
    if(Same(state.oldMatch,progress)&&Same(state.oldPrevious,progress)&&CaptureResult()){
        state.started=true;state.stage="local_network_original_match_already_started";return true;
    }
    if(!Exact(state.oldMatch,entering)||!Exact(state.oldPrevious,entering)
       ||Read<uint8_t>(state.gameState,0x490)!=0)
        return Fail(3487,"local_network_match_start_requires_unchanged_original_entering_map_state");
    // This is the local authority's missing game rule, not a direct call to
    // AGameMode.SetMatchState on a GameModeBase layout. PreviousMatchState,
    // bReplicatedHasBegunPlay and world/actor initialization remain owned by
    // the full original GameState notification and normal replication.
    state.publicationAttempted=true;
    state.operation="publish_authority_match_state_in_progress";
    if(!original_players::Write(state.gameState,0x4c0,progress))
        return Fail(3488,"local_network_authority_match_name_publication_failed");
    state.operation="original_game_state_on_rep_match_state_443c750";
    reinterpret_cast<void(*)(void*)>(Read<void*>(table,0x830))(state.gameState);
    state.originalNotifyReturned=true;
    if(!CaptureResult())return Fail(3489,"local_network_original_match_notification_incomplete");
    state.operation="none";state.started=true;
    state.stage="local_network_authority_match_started_by_original_game_state";
    return true;
}
inline LONG CaptureException(EXCEPTION_POINTERS* exception){
    if(exception&&exception->ExceptionRecord){
        state.error=exception->ExceptionRecord->ExceptionCode;
        const auto at=reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress);
        state.exceptionRva=at>=engine::image?at-engine::image:at;
    }else state.error=3490;
    state.started=false;state.restartRequired=true;
    state.stage="local_network_match_start_native_exception_requires_new_session";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool PublishAuthorityMatchStarted(void* world){
    __try {return PublishAuthorityMatchStartedImpl(world);}
    __except(CaptureException(GetExceptionInformation())) {return false;}
}

// The shipped client retains the replicated PS fields and complete original
// notifications, but its Auth character setters are parameter-consuming stubs.
// Publish only a validated authority old-pawn -> new-pawn relationship. Clients
// receive these references through their normal actor channels and RepNotifies.
struct CharacterLinksDiagnostics {
    bool profilesVerified=false,metadataVerified=false,authorityPairVerified=false;
    bool spawnPublished=false,possessionPublished=false,nativeGettersMatched=false;
    bool previousSpawnConsumerMatched=false,restartRequired=false;
    unsigned error=0;
    uint32_t publications=0,idempotentCalls=0,spawnNotifications=0,possessionNotifications=0;
    uintptr_t exceptionRva=0;
    const char* stage="local_network_character_links_not_published";
    const char* operation="none";
    void *world=nullptr,*controller=nullptr,*playerState=nullptr,*pawn=nullptr,*expectedPrevious=nullptr;
    void *oldSpawn=nullptr,*oldPossessed=nullptr,*oldSpawnConsumer=nullptr;
    void *observedSpawn=nullptr,*observedPossessed=nullptr;
};
inline CharacterLinksDiagnostics characterLinks;
inline const CharacterLinksDiagnostics& CharacterLinksState(){return characterLinks;}
inline bool CharacterLinksFail(unsigned code,const char* reason){
    characterLinks.error=code;characterLinks.stage=reason;return false;
}
inline bool VerifyCharacterLinkProfiles(){
    struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
    static constexpr Profile profiles[]{
        {0x6cc4910,233,0x5ce8ec3722bca771ULL}, // original SpawnedCharacter notify exec
        {0x7197a00,2424,0xcaf5c0bcdc574a4eULL}, // full old/new damageable delegate lifecycle
        {0x6cc47d0,319,0xb78e03d17497b0f7ULL}, // full PossessedCharacter notify exec
        {0x6cc5940,32,0xa6e554f64b1730dcULL}, // original direct SpawnedCharacter getter exec
        {0x6cc5910,44,0x8cff394b07b7147dULL}, // original PossessedCharacter getter exec
        {0x7198a20,123,0xed0ecf5b1a67fe1aULL}, // original role/owner-aware possession getter
    };
    for(const auto& p:profiles){
        const auto* bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        uint64_t hash=14695981039346656037ULL;
        for(size_t i=0;i<p.bytes;++i)hash=(hash^bytes[i])*1099511628211ULL;
        if(hash!=p.fnv)return false;
    }
    return true;
}
inline bool CharacterLinkProperty(void* object,const wchar_t* name,int32_t expectedOffset){
    uint32_t nativeName[3]{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(nativeName,name,1);
    auto property=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)
        (Read<void*>(object,0x10),nativeName,3);
    constexpr uint64_t netAndRepNotify=0x100000020ULL;
    return property&&Read<int32_t>(property,0x30)==1&&Read<int32_t>(property,0x34)==8
        &&Read<int32_t>(property,0x44)==expectedOffset
        &&(Read<uint64_t>(property,0x38)&netAndRepNotify)==netAndRepNotify;
}
inline bool CharacterLinkEvent(void* object,const wchar_t* name,void* parameters){
    auto function=engine::Find(name);
    return function&&Read<uint16_t>(function,0xbe)==8&&engine::Event(object,name,parameters);
}
inline void* CharacterActorWorld(void* actor){
    auto function=Read<void*>(Read<void**>(actor,0),0x1b8);
    return function?reinterpret_cast<void*(*)(void*)>(function)(actor):nullptr;
}
inline bool PublishAuthorityCharacterLinksImpl(void* world,void* controller,void* playerState,
                                               void* pawn,void* expectedPrevious){
    auto& d=characterLinks;
    if(d.error||d.restartRequired)return false;
    if(!original_players::Thread())return CharacterLinksFail(3491,"local_network_character_links_require_original_game_thread");
    if(d.world&&d.world!=world)return CharacterLinksFail(3492,"local_network_character_links_world_changed");
    d.world=world;d.controller=controller;d.playerState=playerState;d.pawn=pawn;d.expectedPrevious=expectedPrevious;
    d.spawnPublished=false;d.possessionPublished=false;d.nativeGettersMatched=false;
    d.metadataVerified=false;d.authorityPairVerified=false;d.previousSpawnConsumerMatched=false;
    d.stage="local_network_validate_authority_character_links";
    if(!world||original_players::world!=world||(Read<uint8_t>(world,0x156)&0x80)==0
       ||!original_players::IsA(controller,original_players::ShooterControllerClass)
       ||!original_players::IsA(playerState,original_players::ShooterStateClass)
       ||!original_players::IsA(pawn,original_players::ShooterCharacterClass)
       ||Read<uint8_t>(controller,0x224)!=3||Read<uint8_t>(playerState,0x224)!=3||Read<uint8_t>(pawn,0x224)!=3
       ||Read<void*>(controller,0x470)!=playerState||Read<void*>(playerState,0x210)!=controller
       ||Read<void*>(controller,0x4b0)!=pawn||Read<void*>(pawn,0x4a0)!=controller
       ||Read<void*>(pawn,0x488)!=playerState||!original_players::Registered(playerState)
       ||!Read<uint8_t>(pawn,0xa90)||!original_players::IsA(Read<void*>(pawn,0xc70),0x6702670)
       ||CharacterActorWorld(controller)!=world||CharacterActorWorld(playerState)!=world||CharacterActorWorld(pawn)!=world)
        return CharacterLinksFail(3493,"local_network_character_links_require_actual_registered_authority_possession");
    if(expectedPrevious&&(expectedPrevious==pawn
       ||!original_players::IsA(expectedPrevious,original_players::ShooterCharacterClass)
       ||Read<uint8_t>(expectedPrevious,0x224)!=3||Read<void*>(expectedPrevious,0x488)!=playerState
       ||CharacterActorWorld(expectedPrevious)!=world))
        return CharacterLinksFail(3494,"local_network_character_links_expected_previous_identity_mismatch");
    d.authorityPairVerified=true;
    if(!d.profilesVerified){
        d.profilesVerified=VerifyCharacterLinkProfiles();
        if(!d.profilesVerified)return CharacterLinksFail(3495,"local_network_character_link_native_profile_mismatch");
    }
    if(!CharacterLinkProperty(playerState,L"SpawnedCharacter",0xbc8)
       ||!CharacterLinkProperty(playerState,L"PossessedCharacter",0xbd0))
        return CharacterLinksFail(3496,"local_network_character_link_replication_metadata_mismatch");
    d.metadataVerified=true;
    d.oldSpawn=Read<void*>(playerState,0xbc8);d.oldPossessed=Read<void*>(playerState,0xbd0);
    d.oldSpawnConsumer=Read<void*>(playerState,0x900);
    const bool alreadyPublished=d.oldSpawn==pawn&&d.oldPossessed==pawn;
    const bool expectedTuple=d.oldSpawn==expectedPrevious&&d.oldPossessed==expectedPrevious;
    if((!alreadyPublished&&!expectedTuple)||d.oldSpawnConsumer!=d.oldSpawn)
        return CharacterLinksFail(3497,"local_network_character_link_old_new_tuple_or_original_consumer_conflict");
    if(!alreadyPublished){
        // Preserve each original previous-pointer argument. Spawned notify's
        // full7197a00 additionally uses its own ClientPrevSpawnedCharacter+900
        // to unsubscribe the old damageable and subscribe to the new one.
        d.operation="authority_publish_spawned_character_original_notify";
        if(!original_players::Write(playerState,0xbc8,pawn)
           ||!CharacterLinkEvent(playerState,L"/Script/ShooterGame.ShooterPlayerState.OnRep_SpawnedCharacter",&d.oldSpawn))
            return CharacterLinksFail(3498,"local_network_original_spawned_character_publication_failed");
        ++d.spawnNotifications;
        if(Read<void*>(playerState,0xbc8)!=pawn||Read<void*>(playerState,0x900)!=pawn)
            return CharacterLinksFail(3499,"local_network_original_spawned_character_consumer_not_updated");
        d.spawnPublished=true;
        d.operation="authority_publish_possessed_character_original_notify";
        if(!original_players::Write(playerState,0xbd0,pawn)
           ||!CharacterLinkEvent(playerState,L"/Script/ShooterGame.ShooterPlayerState.OnRep_PossessedCharacter",&d.oldPossessed))
            return CharacterLinksFail(3500,"local_network_original_possessed_character_publication_failed");
        ++d.possessionNotifications;
    }else ++d.idempotentCalls;
    d.observedSpawn=nullptr;d.observedPossessed=nullptr;
    d.operation="original_player_state_character_getter_readbacks";
    if(!CharacterLinkEvent(playerState,L"/Script/ShooterGame.ShooterPlayerState.GetSpawnedCharacter",&d.observedSpawn)
       ||!CharacterLinkEvent(playerState,L"/Script/ShooterGame.ShooterPlayerState.GetPossessedCharacter",&d.observedPossessed)
       ||d.observedSpawn!=pawn||d.observedPossessed!=pawn
       ||Read<void*>(playerState,0xbc8)!=pawn||Read<void*>(playerState,0xbd0)!=pawn
       ||Read<void*>(playerState,0x210)!=controller||Read<void*>(controller,0x470)!=playerState
       ||Read<void*>(controller,0x4b0)!=pawn||Read<void*>(pawn,0x4a0)!=controller||Read<void*>(pawn,0x488)!=playerState)
        return CharacterLinksFail(3501,"local_network_original_character_link_getters_or_ownership_mismatch");
    d.previousSpawnConsumerMatched=Read<void*>(playerState,0x900)==pawn;
    if(!d.previousSpawnConsumerMatched)return CharacterLinksFail(3502,"local_network_original_spawned_consumer_identity_mismatch");
    d.spawnPublished=true;d.possessionPublished=true;d.nativeGettersMatched=true;
    if(!alreadyPublished)++d.publications;
    d.operation="none";d.stage="local_network_authority_original_character_links_published";return true;
}
inline LONG CaptureCharacterLinksException(EXCEPTION_POINTERS* exception){
    auto& d=characterLinks;
    if(exception&&exception->ExceptionRecord){
        d.error=exception->ExceptionRecord->ExceptionCode;
        const auto at=reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress);
        d.exceptionRva=at>=engine::image?at-engine::image:at;
    }else d.error=3503;
    d.restartRequired=true;d.stage="local_network_character_link_native_exception_requires_new_session";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool PublishAuthorityCharacterLinks(void* world,void* controller,void* playerState,
                                           void* pawn,void* expectedPrevious=nullptr){
    __try {return PublishAuthorityCharacterLinksImpl(world,controller,playerState,pawn,expectedPrevious);}
    __except(CaptureCharacterLinksException(GetExceptionInformation())) {return false;}
}
}
