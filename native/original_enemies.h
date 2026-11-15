#pragma once
#include "original_players.h"
#include "original_content.h"
#include "original_enemy_colors.h"
#include "original_heroes.h"
#include <array>

// The local authority supplies roster/team/selection records and AI input only.
// The shipped hero Blueprint owns all meshes, materials, AnimationState, AnimBP,
// capsule, movement, physics and visibility. No generic presentation initializer,
// mesh replacement, material override, pose copying or per-frame teleport is used.
// Pinned static evidence: work/duel1-original-enemies; runtime verification is
// deliberately reported separately from construction and non-null pointers.
namespace original_enemies {
// Existing practice/probe loops deliberately retain three heroes. The complete
// path/name arrays now also serve a fixed unique-match selection; indices0..2
// remain Jett/Phoenix/Sage and ordinary HeroFor hashes retain their old result.
inline constexpr size_t HeroCount=3,HeroCatalogCount=original_heroes::PlayableHeroCount;
inline constexpr auto& HeroPaths=original_heroes::Paths;
inline constexpr auto& HeroNames=original_heroes::Names;
inline constexpr uintptr_t OutlineClass=0x64d0c50;
inline constexpr uintptr_t BombTeamClass=0x65e4040;
// Exact EAresTeam table at RVA c2ffb10: Red0, Blue1, Invalid254, MAX255.
inline constexpr uint8_t LocalTeam=0, EnemyTeam=1;
struct Options {
    bool outline=true;
    double strafeExtent=100.0;
    double aiStallTimeout=2.0;
};
inline Options options;
struct FootMotion {
    bool sampled=false,pending=true,leaderMapped=false;
    int32_t leftIndex=-1,rightIndex=-1;
    int32_t poseBufferIndex=-1,poseCount=0;
    uint32_t samples=0,changes=0,movingSamples=0,movingChanges=0,pendingSamples=0;
    uint64_t poseFrame=0;
    void* poseSource=nullptr;
    const char* cacheState="not_sampled";
    engine::Vec firstLeft{},firstRight{},left{},right{};
    double maximumStep=0,maximumExcursion=0,maximumMovingStillSeconds=0,movingStillSince=-1,lastSampleAt=-1,lastAttemptAt=-1;
};
struct Enemy {
    bool attempted=false,ready=false,enabled=false;
    bool selectionPublished=false,spawnPublished=false,possessionPublished=false;
    bool teamPublished=false,nativeEnemy=false,nativeLocalControl=false;
    bool outlineFound=false,outlineAttached=false,outlineApplied=false;
    bool coreAssetReady=false,cosmeticAssetReady=false,coreAnimReady=false;
    bool requestedWalk=false,requestedCrouch=false;
    bool originalCoreAnimClass=false,materialSlotsReady=false,actuallyMoved=false;
    uint32_t logicalId=0,heroIndex=0,inputCalls=0,worldInputCalls=0,samples=0;
    uint32_t materialMissingSamples=0,materialSlotChanges=0;
    uint8_t observedOutline=0;
    int32_t coreMaterialCount=0,cosmeticMaterialCount=0,missingMaterials=0;
    float capsuleHalfHeight=0,capsuleRadius=0,originalCapsuleHalfHeight=0,originalCapsuleRadius=0;
    double yaw=180,direction=1,lastMotionTime=-1,lastSampleTime=-1;
    engine::Vec anchor{},position{},velocity{},initialPosition{},rootScale{};
    engine::Vec lastWorldInput{},controlRotation{},actorRotation{};
    double minimumX=0,maximumX=0;
    engine::Vec coreRelativePosition{},coreRelativeScale{},cosmeticRelativeScale{};
    void *pawn=nullptr,*controller=nullptr,*playerState=nullptr,*team=nullptr,*core=nullptr,*cosmetic=nullptr;
    void *coreAsset=nullptr,*cosmeticAsset=nullptr,*coreAnim=nullptr,*cosmeticAnim=nullptr;
    void *outline=nullptr,*materialManager=nullptr,*movement=nullptr;
    original_content::CharacterIdentity identity;
    FootMotion coreFeet,cosmeticFeet;
};
inline std::array<Enemy,original_players::MaxPlayers> enemies{};
struct NameState {
    bool attempted=false,setterCalled=false,fullNameVerified=false,shortNameVerified=false;
    bool ownerInfoPresent=false,ownerInfoNameVerified=false,platformProviderAbsent=false;
    wchar_t requested[64]{},actualFull[64]{},actualShort[64]{},actualOwnerInfo[64]{};
};
inline std::array<NameState,original_players::MaxPlayers> names{};
inline unsigned error=0;
inline const char* stage="original_enemies_not_initialized";
inline bool restartRequired=false;
inline uintptr_t exceptionRva=0;
inline const char* exceptionOperation="none";
inline uint32_t created=0;
inline bool colorsApplied=false;
inline const wchar_t* failedEventName=L"";
inline uint16_t failedEventExpectedSize=0,failedEventActualSize=0;
inline bool failedEventHasObject=false,failedEventHasFunction=false;
template<class T> inline T Value(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline bool Fail(unsigned code,const char* text){error=code;stage=text;return false;}
inline bool Thread(){return !restartRequired&&original_players::Thread();}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* params){
    auto function=engine::Find(name);
    if(!object||!function||Value<uint16_t>(function,0xbe)!=size){
        failedEventName=name;failedEventExpectedSize=size;failedEventActualSize=Value<uint16_t>(function,0xbe);
        failedEventHasObject=object!=nullptr;failedEventHasFunction=function!=nullptr;return false;
    }
    auto table=Value<void**>(object,0);
    auto process=Value<void*>(table,0x298);
    if(!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,function,params);return true;
}
inline LONG RecordException(EXCEPTION_POINTERS* e){
    error=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0000a01U;
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    exceptionRva=at>=engine::image?at-engine::image:at;
    exceptionOperation=stage;restartRequired=true;stage="original_enemy_exception_requires_new_session";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool UniqueHeroMatchActive(){return original_heroes::State().matchAssigned;}
inline bool BeginUniqueHeroMatch(size_t totalPlayers,uint32_t shuffleSeed){
    return original_heroes::BeginUniqueMatch(totalPlayers,shuffleSeed)?true:Fail(original_heroes::State().error,original_heroes::State().stage);
}
inline uint32_t AssignedHero(size_t index){return original_heroes::AssignedHero(index);}
inline uint32_t AssignedLogicalId(size_t index){return original_heroes::AssignedLogicalId(index);}
inline const wchar_t* AssignedHeroPath(size_t index){return original_heroes::AssignedPath(index);}
inline bool ValidateMatchHero(size_t index){
    if(!UniqueHeroMatchActive())return true;
    if(index>=original_players::MaxPlayers)return false;
    const auto type=Value<void*>(original_players::players[index].character,0x10);
    return original_heroes::ValidateAssignedClass(index,type)?true:Fail(original_heroes::State().error,original_heroes::State().stage);
}
inline uint32_t HeroFor(uint32_t logicalId){
    uint32_t assigned=0;if(original_heroes::ResolveMatchLogicalId(logicalId,assigned))return assigned;
    uint32_t h=logicalId+0x9e3779b9u;h=(h^(h>>16))*0x85ebca6bu;
    h=(h^(h>>13))*0xc2b2ae35u;return (h^(h>>16))%static_cast<uint32_t>(HeroCount);
}
inline bool PublishSelection(size_t index){
    if(!Thread()||index>=original_players::MaxPlayers)return false;
    auto& entry=original_players::players[index];
    if(!entry.ready||!original_content::State().initialized)return Fail(1040,"original_enemy_content_or_roster_not_ready");
    const auto type=Value<void*>(entry.character,0x10);
    auto identity=original_content::FindCharacterByClass(type);
    if(!identity.found||!identity.dataAsset||identity.characterClass!=type||!original_content::Valid(identity.id))
        return Fail(1041,"original_enemy_character_identity_missing");
    const auto previous=Value<original_content::Guid>(entry.state,0xbf8);
    if(std::memcmp(&previous,&identity.id,sizeof(previous))!=0){
        // AuthSetDesiredClass is absent in this client. Publish only the real
        // service GUID resolved/round-tripped by the original ContentIndex.
        stage="original_enemy_publish_character_selection";
        if(!original_players::Write(entry.state,0xbf8,identity.id)
           ||!Event(entry.state,L"/Script/ShooterGame.ShooterPlayerState.OnRep_DesiredClass",0,nullptr))
            return Fail(1042,"original_enemy_selection_notification_failed");
    }
    void *actualClass=nullptr,*actualData=nullptr;
    if(!Event(entry.state,L"/Script/ShooterGame.ShooterPlayerState.GetDesiredClass",8,&actualClass)
       ||!Event(entry.state,L"/Script/ShooterGame.ShooterPlayerState.TryGetDesiredClassDataAsset",8,&actualData)
       ||actualClass!=type||actualData!=identity.dataAsset)
        return Fail(1043,"original_enemy_selection_readback_mismatch");
    enemies[index].identity=identity;enemies[index].selectionPublished=true;return true;
}
inline bool PublishCharacterAssociations(size_t index){
    if(!Thread()||index>=original_players::MaxPlayers)return false;
    auto& entry=original_players::players[index];
    if(!entry.ready||Value<void*>(entry.controller,0x4b0)!=entry.character
       ||Value<void*>(entry.character,0x488)!=entry.state)return Fail(1044,"original_enemy_controller_pair_missing");
    auto previousSpawn=Value<void*>(entry.state,0xbc8);
    if(previousSpawn!=entry.character){
        if(previousSpawn)return Fail(1045,"original_enemy_spawn_association_already_occupied");
        stage="original_enemy_publish_spawned_character";
        if(!original_players::Write(entry.state,0xbc8,entry.character)
           ||!Event(entry.state,L"/Script/ShooterGame.ShooterPlayerState.OnRep_SpawnedCharacter",8,&previousSpawn))
            return Fail(1046,"original_enemy_spawn_notification_failed");
    }
    auto previousPossessed=Value<void*>(entry.state,0xbd0);
    if(previousPossessed!=entry.character){
        if(previousPossessed)return Fail(1047,"original_enemy_possession_association_already_occupied");
        stage="original_enemy_publish_possessed_character";
        if(!original_players::Write(entry.state,0xbd0,entry.character)
           ||!Event(entry.state,L"/Script/ShooterGame.ShooterPlayerState.OnRep_PossessedCharacter",8,&previousPossessed))
            return Fail(1048,"original_enemy_possession_notification_failed");
    }
    void *spawned=nullptr,*possessed=nullptr;
    if(!Event(entry.state,L"/Script/ShooterGame.ShooterPlayerState.GetSpawnedCharacter",8,&spawned)
       ||!Event(entry.state,L"/Script/ShooterGame.ShooterPlayerState.GetPossessedCharacter",8,&possessed)
       ||spawned!=entry.character||possessed!=entry.character)
        return Fail(1049,"original_enemy_association_readback_mismatch");
    enemies[index].spawnPublished=true;enemies[index].possessionPublished=true;return true;
}
inline bool PublishTeam(void* playerState,uint8_t team){
    if(!Thread()||!original_players::IsA(playerState,original_players::ShooterStateClass)
       ||team>1)return false;
    auto component=Value<void*>(playerState,0x6a8);
    if (original_players::options.freeForAll) {
        return original_players::IsA(component,0x6849cb0) && Value<void*>(component,0xc8)==playerState
            ? true : Fail(1083,"original_ffa_team_component_missing");
    }
    if(!original_players::IsA(component,BombTeamClass)||Value<void*>(component,0xc8)!=playerState)
        return Fail(1050,"original_enemy_original_team_component_missing");
    if(Value<uint8_t>(component,0xe8)!=team){
        // The exact AuthSetTeam VM wrapper638b5a0 consumes its enum and returns
        // without a store/callee. This is the missing local authority record;
        // original OnRep dispatches the existing BaseTeam.OnTeamChanged delegate.
        stage="original_enemy_publish_team";
        if(!original_players::Write(component,0xe8,team)
           ||!Event(component,L"/Script/ShooterGame.BombTeamComponent.OnRep_TeamChanged",0,nullptr))
            return Fail(1051,"original_enemy_team_notification_failed");
    }
    return Value<uint8_t>(component,0xe8)==team;
}
struct NativeString {wchar_t* data=nullptr;int32_t count=0,capacity=0;};
static_assert(sizeof(NativeString)==16&&offsetof(NativeString,count)==8);
inline bool ReadNativeName(void* object,const wchar_t* method,wchar_t (&output)[64]){
    NativeString result;
    if(!Event(object,method,16,&result))return Fail(1075,"original_roster_name_query_failed");
    if(result.count<0||result.count>64||result.capacity<result.count
       ||(result.count&&(!result.data||!original_players::Readable(result.data,static_cast<size_t>(result.count)*sizeof(wchar_t)))))
        return Fail(1076,"original_roster_name_result_invalid");
    output[0]=0;
    const bool terminated=!result.count||result.data[result.count-1]==0;
    if(terminated&&result.count)std::memcpy(output,result.data,static_cast<size_t>(result.count)*sizeof(wchar_t));
    // The original generated FString-return wrappers use exactly this native
    // FMemory::Free dispatcher. No UE allocation is released by the DLL CRT.
    if(result.data)engine::function<void(*)(void*)>(0x1915720)(result.data);
    return terminated?true:Fail(1076,"original_roster_name_result_unterminated");
}
inline bool VerifyRosterNameImpl(size_t index){
    if(!Thread()||index>=names.size()||!names[index].attempted)return false;
    auto& entry=original_players::players[index];auto& result=names[index];
    result.fullNameVerified=result.shortNameVerified=result.ownerInfoNameVerified=false;
    result.ownerInfoPresent=false;
    if(!ReadNativeName(entry.state,L"/Script/Engine.PlayerState.GetPlayerName",result.actualFull)
       ||!ReadNativeName(entry.state,L"/Script/ShooterGame.AresPlayerStateBase.GetShortPlayerName",result.actualShort))return false;
    result.fullNameVerified=std::wcscmp(result.requested,result.actualFull)==0;
    // This is an assertion of the original short-name result, not a name shown
    // by a replacement UI. Native717d590 owns its20-UTF16-unit truncation.
    const size_t fullLength=std::wcslen(result.requested),shortLength=fullLength>20?20:fullLength;
    result.shortNameVerified=std::wcslen(result.actualShort)==shortLength
        &&std::wmemcmp(result.requested,result.actualShort,shortLength)==0;
    if(!result.fullNameVerified||!result.shortNameVerified)return Fail(1077,"original_roster_name_readback_mismatch");
    auto info=Value<void*>(entry.state,0xa18);
    if(info){
        result.ownerInfoPresent=true;void* infoState=nullptr;
        if(!original_players::IsA(info,0x64f4540)
           ||!Event(info,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.GetPlayerState",8,&infoState)
           ||infoState!=entry.state
           ||!ReadNativeName(info,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.GetShortPlayerName",result.actualOwnerInfo))
            return Fail(1078,"original_roster_name_owner_info_query_failed");
        result.ownerInfoNameVerified=std::wcscmp(result.actualShort,result.actualOwnerInfo)==0;
        if(!result.ownerInfoNameVerified)return Fail(1079,"original_roster_name_owner_info_mismatch");
    }
    return true;
}
inline bool VerifyRosterName(size_t index){
    __try{return VerifyRosterNameImpl(index);}__except(RecordException(GetExceptionInformation())){return false;}
}
inline bool ApplyRosterNameImpl(size_t index,const wchar_t* requested){
    if(!Thread()||index>=names.size()||!requested)return false;
    auto& entry=original_players::players[index];
    const size_t length=wcsnlen_s(requested,64);
    if(!entry.ready||!entry.registered||!length||length>=64)return Fail(1080,"original_roster_name_invalid_request");
    auto& result=names[index];result={};result.attempted=true;
    std::memcpy(result.requested,requested,(length+1)*sizeof(wchar_t));
    result.platformProviderAbsent=Value<void*>(entry.state,0x6a0)==nullptr;
    // PlatformPlayer names are account data. This local trainer's no-login path
    // deliberately has no such provider and never modifies or creates one.
    if(!result.platformProviderAbsent)return Fail(1081,"original_roster_name_platform_provider_present");
    auto table=Value<void**>(entry.state,0);
    auto setter=Value<void*>(table,0x7a0);
    if(setter!=reinterpret_cast<void*>(engine::image+0x4947310)
       ||Value<void*>(table,0x7a8)!=reinterpret_cast<void*>(engine::image+0x4947300))
        return Fail(1082,"original_roster_name_native_setter_changed");
    NativeString input{result.requested,static_cast<int32_t>(length+1),static_cast<int32_t>(length+1)};
    stage="original_roster_call_original_set_player_name";
    // Actual Engine.PlayerState::SetPlayerName(this,const FString&), not a VM
    // exec wrapper. It copies through the original FString allocator, invokes
    // the original OnRep and notifies existing original UI/name delegates.
    reinterpret_cast<void(*)(void*,const NativeString*)>(setter)(entry.state,&input);
    result.setterCalled=true;
    if(!VerifyRosterNameImpl(index))return false;
    if(!original_players::SetRequestedName(index,result.requested))return Fail(1083,"original_roster_name_label_sync_failed");
    entry.nativeNameApplied=true;error=0;stage="original_roster_name_applied_and_read_back";return true;
}
inline bool ApplyRosterName(size_t index,const wchar_t* requested){
    __try{return ApplyRosterNameImpl(index,requested);}__except(RecordException(GetExceptionInformation())){return false;}
}
inline bool BindRosterPlayerImpl(size_t index,uint8_t team){
    if(!Thread()||index>=original_players::MaxPlayers||team>1)return false;
    auto& entry=original_players::players[index];
    if(!entry.ready||!entry.registered)return Fail(1074,"original_roster_player_not_registered");
    if(!PublishSelection(index)||!PublishCharacterAssociations(index)||!PublishTeam(entry.state,team)
       ||!ApplyRosterNameImpl(index,entry.requestedName))return false;
    // Also usable for local index0. It never transfers LocalPlayer, changes the
    // active pawn, installs a second HUD or marks the player as a training enemy.
    error=0;stage="original_roster_player_native_records_bound";return true;
}
inline bool BindRosterPlayer(size_t index,uint8_t team=LocalTeam){
    __try{return BindRosterPlayerImpl(index,team);}__except(RecordException(GetExceptionInformation())){return false;}
}
inline bool VerifyEnemy(Enemy& target){
    if(original_players::localIndex<0)return Fail(1052,"original_enemy_local_player_missing");
    auto& local=original_players::players[static_cast<size_t>(original_players::localIndex)];
    if(!PublishTeam(local.state,LocalTeam)||!PublishTeam(target.playerState,EnemyTeam))return false;
    target.team=Value<void*>(target.playerState,0x6a8);
    struct TeamQuery {uint8_t other=LocalTeam;uint8_t result=0;} teamQuery;
    if(!original_players::options.freeForAll &&
       (!Event(target.team,L"/Script/ShooterGame.BombTeamComponent.IsEnemyTeam",2,&teamQuery)||!teamQuery.result))
        return Fail(1053,"original_enemy_team_comparison_failed");
    struct Alliance {void* source;void* target;uint8_t result=0;uint8_t padding[7]{};} alliance{local.character,target.pawn};
    static_assert(sizeof(Alliance)==24);
    // Generated C++ storage is24 bytes, but native UFunction.ParmsSize ends at
    // ReturnValue(bool)+16+1. Retain aligned storage and validate the17-byte
    // reflected payload rather than its trailing C++ structure padding.
    if(!Event(engine::DefaultObject(engine::Class(0x65a0b40)),L"/Script/ShooterGame.BaseTeamComponent.IsEnemy",17,&alliance))
        return Fail(1054,"original_enemy_alliance_query_missing");
    target.teamPublished=true;target.nativeEnemy=alliance.result!=0;
    return target.nativeEnemy?true:Fail(1055,"original_enemy_native_alliance_not_enemy");
}
inline void* NamedOutline(void* pawn,const wchar_t* name){
    return engine::function<void*(*)(void*,void*,const wchar_t*,bool)>(0x1d07dd0)
        (engine::Class(OutlineClass),pawn,name,false);
}
inline bool ConfigureOutline(Enemy& target){
    stage="original_enemy_existing_outline";
    target.outline=NamedOutline(target.pawn,L"OutlineCosmetic3P");
    if(!target.outline)target.outline=NamedOutline(target.pawn,L"OutlineCosmetic3P_GEN_VARIABLE");
    target.outlineFound=original_players::IsA(target.outline,OutlineClass);
    if(!target.outlineFound)return Fail(1056,"original_enemy_cosmetic_outline_subobject_missing");
    void* parent=nullptr;
    target.outlineAttached=Event(target.outline,L"/Script/Engine.SceneComponent.GetAttachParent",8,&parent)&&parent==target.cosmetic;
    if(!target.outlineAttached)return Fail(1057,"original_enemy_outline_attachment_mismatch");
    if(!original_enemy_colors::Apply(original_players::world))
        return Fail(1058,"original_enemy_original_colorblind_palette_failed");
    colorsApplied=original_enemy_colors::State().rendererDispatched;
    uint8_t mode=options.outline?3:0;
    if(!Event(target.outline,L"/Script/ShooterGame.AresOutlineComponent.SetOutlineMode",1,&mode)
       ||!Event(target.cosmetic,L"/Script/Engine.PrimitiveComponent.GetAresOutlineMode",1,&target.observedOutline))
        return Fail(1059,"original_enemy_outline_mode_query_failed");
    target.outlineApplied=target.observedOutline==mode;
    return target.outlineApplied?true:Fail(1060,"original_enemy_outline_mode_readback_mismatch");
}
inline bool ReadAsset(void* component,void*& asset){
    asset=nullptr;return Event(component,L"/Script/Engine.SkinnedMeshComponent.GetSkinnedAsset",8,&asset);
}
inline bool ReadMaterials(void* component,int& count,int& missing){
    count=0;missing=0;
    if(!Event(component,L"/Script/Engine.PrimitiveComponent.GetNumMaterials",4,&count)||count<0||count>64)return false;
    for(int i=0;i<count;++i){
        struct Material {int index=0,padding=0;void* result=nullptr;} material;
        material.index=i;
        if(!Event(component,L"/Script/Engine.PrimitiveComponent.GetMaterial",16,&material))return false;
        if(!material.result)++missing;
    }
    return true;
}
struct CachedFoot {
    bool available=false,leaderMapped=false;
    int32_t bufferIndex=-1,count=0;
    uint64_t frame=0;
    void* source=nullptr;
    const char* state="original_pose_not_evaluated";
    engine::Vec position{};
};
inline bool ReadFoot(void* component,const wchar_t* name,int32_t& index,CachedFoot& output){
    output={};
    if(!original_players::IsA(component,0x385f750))return Fail(1089,"original_enemy_pose_source_not_skeletal");
    if(index<0){
        alignas(8) uint8_t boneParameters[16]{};
        // This native asset-only lookup does not evaluate animation or update
        // component/socket transforms. No GetSocketTransform call is issued.
        engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(boneParameters,name,1);
        const int32_t notFound=-1;std::memcpy(boneParameters+12,&notFound,4);
        if(!Event(component,L"/Script/Engine.SkinnedMeshComponent.GetBoneIndex",16,boneParameters))
            return Fail(1086,"original_enemy_foot_bone_index_query_failed");
        std::memcpy(&index,boneParameters+12,4);
        if(index<0)return Fail(1087,"original_enemy_original_foot_bone_missing");
    }
    auto source=component;int32_t sourceIndex=index;
    // Exact original GetBoneTransform4246e10 leader branch: resolve the
    // component's real weak leader, then use THIS mesh's LeaderBoneMap to map
    // its own bone index into the leader. Never reuse another skeleton's index.
    auto leader=engine::function<void*(*)(void*)>(0x1d4cfa0)(static_cast<uint8_t*>(component)+0x718);
    if(leader){
        output.leaderMapped=true;output.source=leader;
        if(leader==component||!original_players::IsA(leader,0x385f750)||(Value<uint8_t>(leader,0xa0)&1)==0){
            output.state="original_leader_pose_unavailable";return true;
        }
        auto map=Value<int32_t*>(component,0x7c8);const int32_t mapCount=Value<int32_t>(component,0x7d0);
        if(mapCount<0||mapCount>4096||index>=mapCount||!original_players::Read(map,static_cast<size_t>(index)*4,sourceIndex)||sourceIndex<0){
            output.state="original_leader_bone_map_unavailable";return true;
        }
        // The pinned native branch reads one leader's completed buffer. A
        // nested leader is not represented as proof of a sampled cosmetic pose.
        if(engine::function<void*(*)(void*)>(0x1d4cfa0)(static_cast<uint8_t*>(leader)+0x718)){
            output.state="nested_original_leader_pose_pending";return true;
        }
        source=leader;
    }
    output.source=source;output.frame=Value<uint64_t>(source,0xd78);
    if(!output.frame)return true; // Ref-pose storage alone is not animation.
    output.bufferIndex=Value<int32_t>(source,0x7a8);
    if(output.bufferIndex<0||output.bufferIndex>1){output.state="original_pose_buffer_index_pending";return true;}
    const size_t arrayOffset=0x760+static_cast<size_t>(output.bufferIndex)*16;
    auto data=Value<void*>(source,arrayOffset);output.count=Value<int32_t>(source,arrayOffset+8);
    const int32_t capacity=Value<int32_t>(source,arrayOffset+12);
    if(!data||output.count<0||output.count>4096||capacity<output.count||sourceIndex>=output.count){
        output.state="original_pose_buffer_pending";return true;
    }
    // Original no-leader branch4249af5..4249b1f selects the current read buffer
    // at760+16*index, bounds-checks Num at768+16*index, and computes BoneIndex
    // *3<<5 ==96-byte FTransform. It only multiplies ComponentToWorld AFTER
    // reading this actual component-space pose. Read that completed cache only.
    engine::Transform transform;
    if(!original_players::Read(data,static_cast<size_t>(sourceIndex)*sizeof(transform),transform)){
        output.state="original_pose_cache_unreadable";return true;
    }
    if(Value<int32_t>(source,0x7a8)!=output.bufferIndex||Value<void*>(source,arrayOffset)!=data
       ||Value<int32_t>(source,arrayOffset+8)!=output.count||Value<uint64_t>(source,0xd78)!=output.frame){
        output.state="original_pose_cache_advanced_during_sample";return true;
    }
    output.position=transform.translation;
    if(!original_players::Finite(output.position)||std::abs(output.position.x)>1000000||std::abs(output.position.y)>1000000||std::abs(output.position.z)>1000000)
        return Fail(1089,"original_enemy_foot_pose_nonfinite_or_invalid");
    output.available=true;output.state=output.leaderMapped?"original_leader_mapped_component_pose":"original_component_pose_cache";
    return true;
}
inline double FootDistanceSquared(const engine::Vec& a,const engine::Vec& b){
    const double x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z;
}
inline bool SampleFeet(void* component,FootMotion& motion,double now,bool moving){
    if(!std::isfinite(now))return Fail(1089,"original_enemy_foot_pose_time_invalid");
    if(motion.lastAttemptAt>=0&&now-motion.lastAttemptAt<.1)return true;
    motion.lastAttemptAt=now;
    CachedFoot leftFoot,rightFoot;
    if(!ReadFoot(component,L"L_Foot",motion.leftIndex,leftFoot)
       ||!ReadFoot(component,L"R_Foot",motion.rightIndex,rightFoot))return false;
    motion.poseSource=leftFoot.source;motion.poseFrame=leftFoot.frame;motion.leaderMapped=leftFoot.leaderMapped;
    motion.poseBufferIndex=leftFoot.bufferIndex;motion.poseCount=leftFoot.count;motion.cacheState=leftFoot.state;
    motion.pending=!leftFoot.available||!rightFoot.available||leftFoot.source!=rightFoot.source||leftFoot.frame!=rightFoot.frame;
    if(motion.pending){
        if(leftFoot.available&&!rightFoot.available)motion.cacheState=rightFoot.state;
        else if(leftFoot.available&&rightFoot.available)motion.cacheState="original_pose_pair_advanced_during_sample";
        ++motion.pendingSamples;motion.movingStillSince=-1;return true;
    }
    const auto left=leftFoot.position,right=rightFoot.position;
    if(motion.sampled){
        const double leftStep=FootDistanceSquared(left,motion.left),rightStep=FootDistanceSquared(right,motion.right);
        const double step=std::sqrt(leftStep>rightStep?leftStep:rightStep);
        if(step>motion.maximumStep)motion.maximumStep=step;
        // The threshold is a diagnostic noise floor in centimeters. It never
        // controls a pose, animation, physics or visible character transform.
        const bool changed=step>.05;
        if(changed)++motion.changes;
        if(moving){
            ++motion.movingSamples;
            if(changed){++motion.movingChanges;motion.movingStillSince=-1;}
            else{
                if(motion.movingStillSince<0)motion.movingStillSince=now;
                const double still=now-motion.movingStillSince;
                if(still>motion.maximumMovingStillSeconds)motion.maximumMovingStillSeconds=still;
            }
        }else motion.movingStillSince=-1;
    }else{
        motion.firstLeft=left;motion.firstRight=right;motion.sampled=true;
    }
    motion.left=left;motion.right=right;motion.lastSampleAt=now;++motion.samples;
    const double leftSpan=FootDistanceSquared(left,motion.firstLeft),rightSpan=FootDistanceSquared(right,motion.firstRight);
    const double excursion=std::sqrt(leftSpan>rightSpan?leftSpan:rightSpan);
    if(excursion>motion.maximumExcursion)motion.maximumExcursion=excursion;
    return true;
}
inline bool SampleImpl(size_t index,double now){
    if(!Thread()||index>=enemies.size()||!enemies[index].attempted)return false;
    auto& target=enemies[index];
    stage="original_enemy_sample_original_presentation";
    target.core=Value<void*>(target.pawn,0x4f0);target.cosmetic=Value<void*>(target.pawn,0xf40);
    target.materialManager=Value<void*>(target.pawn,0xf48);target.movement=Value<void*>(target.pawn,0x4f8);
    if(!target.core||!target.cosmetic||!target.materialManager||!target.movement)
        return Fail(1061,"original_enemy_original_render_components_missing");
    if(!ReadAsset(target.core,target.coreAsset)||!ReadAsset(target.cosmetic,target.cosmeticAsset)
       ||!Event(target.core,L"/Script/Engine.SkeletalMeshComponent.GetAnimInstance",8,&target.coreAnim)
       ||!Event(target.cosmetic,L"/Script/Engine.SkeletalMeshComponent.GetAnimInstance",8,&target.cosmeticAnim))
        return Fail(1062,"original_enemy_original_presentation_query_failed");
    target.coreAssetReady=target.coreAsset!=nullptr;target.cosmeticAssetReady=target.cosmeticAsset!=nullptr;
    target.coreAnimReady=target.coreAnim!=nullptr;
    auto expectedAnim=engine::Find(L"/Game/Characters/_Core/3P/TP_Core_AnimGraph_v2.TP_Core_AnimGraph_v2_C");
    target.originalCoreAnimClass=expectedAnim&&original_players::Derived(Value<void*>(target.coreAnim,0x10),expectedAnim);
    const int oldCount=target.cosmeticMaterialCount;
    int missingCore=0,missingCosmetic=0;
    if(!ReadMaterials(target.core,target.coreMaterialCount,missingCore)
       ||!ReadMaterials(target.cosmetic,target.cosmeticMaterialCount,missingCosmetic))
        return Fail(1063,"original_enemy_material_query_failed");
    target.missingMaterials=missingCosmetic;
    target.materialSlotsReady=target.cosmeticMaterialCount>0&&!missingCosmetic;
    if(target.samples&&oldCount!=target.cosmeticMaterialCount)++target.materialSlotChanges;
    if(missingCosmetic)++target.materialMissingSamples;
    auto capsule=Value<void*>(target.pawn,0x500);
    target.capsuleHalfHeight=Value<float>(capsule,0x6d8);target.capsuleRadius=Value<float>(capsule,0x6dc);
    target.coreRelativePosition=Value<engine::Vec>(target.core,0x170);
    target.coreRelativeScale=Value<engine::Vec>(target.core,0x1a0);
    target.cosmeticRelativeScale=Value<engine::Vec>(target.cosmetic,0x1a0);
    uint8_t locallyControlled=0;
    if(!Event(target.pawn,L"/Script/Engine.Pawn.IsLocallyControlled",1,&locallyControlled)
       ||!Event(target.pawn,L"/Script/Engine.Actor.GetActorScale3D",24,&target.rootScale)
       ||!Event(target.pawn,L"/Script/Engine.Actor.K2_GetActorRotation",24,&target.actorRotation)
       ||!Event(target.controller,L"/Script/Engine.Controller.GetControlRotation",24,&target.controlRotation)
       ||!Event(target.pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&target.position)
       ||!Event(target.pawn,L"/Script/Engine.Actor.GetVelocity",24,&target.velocity))
        return Fail(1064,"original_enemy_movement_sample_failed");
    target.nativeLocalControl=locallyControlled!=0;
    if(!original_players::Finite(target.position)||!original_players::Finite(target.velocity))
        return Fail(1065,"original_enemy_nonfinite_native_movement");
    stage="original_enemy_sample_original_foot_animation";
    const bool moving=target.velocity.x*target.velocity.x+target.velocity.y*target.velocity.y>25.0;
    if(!SampleFeet(target.core,target.coreFeet,now,moving)||!SampleFeet(target.cosmetic,target.cosmeticFeet,now,moving))return false;
    if(!target.samples){
        target.initialPosition=target.position;target.minimumX=target.maximumX=target.position.x;target.lastMotionTime=now;
    }
    if(target.position.x<target.minimumX)target.minimumX=target.position.x;
    if(target.position.x>target.maximumX)target.maximumX=target.position.x;
    const double dx=target.position.x-target.initialPosition.x,dy=target.position.y-target.initialPosition.y;
    if(dx*dx+dy*dy>9.0)target.actuallyMoved=true;
    if(target.velocity.x*target.velocity.x+target.velocity.y*target.velocity.y>9.0)target.lastMotionTime=now;
    target.lastSampleTime=now;++target.samples;
    if(target.outlineFound)Event(target.cosmetic,L"/Script/Engine.PrimitiveComponent.GetAresOutlineMode",1,&target.observedOutline);
    error=0;stage="original_enemy_native_state_sampled";return true;
}
inline bool Sample(size_t index,double now){
    __try{return SampleImpl(index,now);}__except(RecordException(GetExceptionInformation())){return false;}
}
inline bool SpawnImpl(size_t index,uint32_t logicalId,const engine::Vec& feet,double yaw=180){
    if(!Thread()||index>=enemies.size()||!logicalId||!original_players::Finite(feet)||!std::isfinite(yaw))return false;
    if(static_cast<int>(index)==original_players::localIndex)return Fail(1066,"original_enemy_cannot_replace_local_player");
    if(UniqueHeroMatchActive()&&(!original_heroes::Player(index)||AssignedLogicalId(index)!=logicalId))
        return Fail(1090,"original_enemy_unique_match_requires_assigned_logical_id");
    auto& target=enemies[index];
    if(target.attempted)return Fail(1067,"original_enemy_roster_slot_already_attempted");
    target={};target.attempted=true;target.logicalId=logicalId;target.heroIndex=HeroFor(logicalId);
    target.anchor=feet;target.yaw=yaw;
    wchar_t name[64]{};swprintf_s(name,L"Target %u - %s",logicalId,HeroNames[target.heroIndex]);
    stage="original_enemy_create_original_roster_entry";
    if(!original_players::Configure(index,name,HeroPaths[target.heroIndex],feet,yaw,false)
       ||!original_players::CreatePlayer(index))return Fail(original_players::error,original_players::stage);
    auto& entry=original_players::players[index];
    if(!ValidateMatchHero(index))return false;
    target.pawn=entry.character;target.controller=entry.controller;target.playerState=entry.state;
    if(!BindRosterPlayerImpl(index,EnemyTeam)||!VerifyEnemy(target))return false;
    original_players::CharacterDefaults defaults;
    if(!original_players::ProbeDefaults(HeroPaths[target.heroIndex],defaults))return Fail(1068,"original_enemy_original_defaults_missing");
    target.originalCapsuleHalfHeight=defaults.capsuleHalfHeight;target.originalCapsuleRadius=defaults.capsuleRadius;
    if(!SampleImpl(index,static_cast<double>(GetTickCount64())/1000.0)||!ConfigureOutline(target))return false;
    // Readiness means true native actor/roster/configuration, not proven movement,
    // stable pixels, server damage or an exercised original weapon on this pawn.
    target.ready=true;target.enabled=true;++created;error=0;stage="original_enemy_created_pending_runtime_verification";return true;
}
inline bool Spawn(size_t index,uint32_t logicalId,const engine::Vec& feet,double yaw=180){
    __try{return SpawnImpl(index,logicalId,feet,yaw);}__except(RecordException(GetExceptionInformation())){return false;}
}
inline bool SpawnAssigned(size_t index,const engine::Vec& feet,double yaw=180){
    if(!UniqueHeroMatchActive()||!original_heroes::Player(index)||!index)return Fail(1090,"original_enemy_unique_match_slot_not_assigned");
    return Spawn(index,AssignedLogicalId(index),feet,yaw);
}
inline bool SubmitInput(size_t index,float forward,float right,double yaw,bool walking=false,bool crouching=false){
    if(!Thread()||index>=enemies.size()||!enemies[index].ready)return false;
    auto& target=enemies[index];
    if(!std::isfinite(forward)||!std::isfinite(right)||!std::isfinite(yaw)||std::abs(forward)>1||std::abs(right)>1)
        return Fail(1069,"original_enemy_invalid_ai_input");
    if(!target.enabled){forward=right=0;walking=crouching=false;}
    engine::Vec rotation{0,yaw,0};stage="original_enemy_submit_original_ai_inputs";
    // Original posture polling replaces transient held setters each frame.
    // Its own force-input query is the persistent AI input path. A missing
    // PlayerInput on this non-local controller remains an explicit diagnostic;
    // an override being stored does not prove the posture was consumed.
    struct ForcedMove {uint8_t input=0,state=0;};
    if(walking!=target.requestedWalk||!Value<uint8_t>(target.pawn,0xac1)){
        ForcedMove value{0,static_cast<uint8_t>(walking)};
        if(!Event(target.pawn,L"/Script/ShooterGame.ShooterCharacter.ForceMoveInputState",2,&value))
            return Fail(1070,"original_enemy_native_walk_override_failed");
        target.requestedWalk=walking;
    }
    if(crouching!=target.requestedCrouch||!Value<uint8_t>(target.pawn,0xac3)){
        ForcedMove value{1,static_cast<uint8_t>(crouching)};
        if(!Event(target.pawn,L"/Script/ShooterGame.ShooterCharacter.ForceMoveInputState",2,&value))
            return Fail(1070,"original_enemy_native_crouch_override_failed");
        target.requestedCrouch=crouching;
    }
    if(!Event(target.controller,L"/Script/Engine.Controller.SetControlRotation",24,&rotation)
       ||!Event(target.pawn,L"/Script/ShooterGame.ShooterCharacter.MoveForward",4,&forward)
       ||!Event(target.pawn,L"/Script/ShooterGame.ShooterCharacter.MoveRight",4,&right))
        return Fail(1070,"original_enemy_native_ai_input_failed");
    target.yaw=yaw;++target.inputCalls;error=0;stage="original_enemy_ai_inputs_submitted";return true;
}
inline bool SubmitWorldInput(size_t index,const engine::Vec& direction,double yaw,bool walking=false,bool crouching=false){
    if(!Thread()||index>=enemies.size()||!enemies[index].ready||!original_players::Finite(direction))return false;
    const double lengthSquared=direction.x*direction.x+direction.y*direction.y+direction.z*direction.z;
    if(lengthSquared>1.000001)return Fail(1084,"original_enemy_invalid_world_input");
    auto& target=enemies[index];
    if(!SubmitInput(index,0,0,yaw,walking,crouching))return false;
    const engine::Vec submitted=target.enabled?direction:engine::Vec{};
    auto table=Value<void**>(target.pawn,0);auto input=Value<void*>(table,0x8a0);
    if(input!=reinterpret_cast<void*>(engine::image+0x4856280))return Fail(1085,"original_enemy_native_world_input_entry_changed");
    stage="original_enemy_submit_original_world_input";
    // Same original Pawn.AddMovementInput virtual called by787cf80/787d1a0.
    // RCX=pawn,RDX=FVector*,XMM2=scale(float),R9=bForce(false). Native4856280
    // accumulates into the original movement component; its normal tick still
    // owns speed, friction, collision and animation. Avoid mixing the original
    // forward(ControlRotation) and right(ActorRotation) coordinate bases.
    reinterpret_cast<void(*)(void*,const engine::Vec*,float,bool)>(input)(target.pawn,&submitted,1.0f,false);
    target.lastWorldInput=submitted;++target.worldInputCalls;error=0;stage="original_enemy_world_ai_input_submitted";return true;
}
inline bool Update(size_t index,double now,const engine::Vec& playerPosition,bool moving=true){
    if(!Thread()||index>=enemies.size()||!std::isfinite(now)||!original_players::Finite(playerPosition))return false;
    auto& target=enemies[index];if(!target.ready)return false;
    if(target.lastSampleTime<0||now-target.lastSampleTime>=.1)if(!Sample(index,now))return false;
    // Bounds decisions use this actual frame's native position; the relatively
    // expensive material/animation diagnostic remains sampled at10Hz.
    if(!Event(target.pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&target.position))
        return Fail(1064,"original_enemy_movement_sample_failed");
    const double dx=playerPosition.x-target.position.x,dy=playerPosition.y-target.position.y;
    const double yaw=dx*dx+dy*dy>.01?std::atan2(dy,dx)*180.0/3.14159265358979323846:target.yaw;
    if(target.position.y>=target.anchor.y+options.strafeExtent)target.direction=-1;
    else if(target.position.y<=target.anchor.y-options.strafeExtent)target.direction=1;
    else if(target.lastMotionTime>=0&&now-target.lastMotionTime>options.aiStallTimeout){
        target.direction=-target.direction;target.lastMotionTime=now;
    }
    const engine::Vec direction{0,moving?target.direction:0,0};
    return SubmitWorldInput(index,direction,yaw);
}
inline bool SetEnabled(size_t index,bool enabled){
    if(!Thread()||index>=enemies.size()||!enemies[index].ready)return false;
    if(!original_players::SetEnabled(index,enabled))return Fail(1071,"original_enemy_visibility_change_failed");
    enemies[index].enabled=enabled;return SubmitInput(index,0,0,enemies[index].yaw);
}
inline bool RelocateForTrainingRound(size_t index,const engine::Vec& feet,double yaw){
    if(!Thread()||index>=enemies.size()||!enemies[index].ready||!original_players::Finite(feet)||!std::isfinite(yaw))return false;
    auto& target=enemies[index];
    const auto capsule=Value<void*>(target.pawn,0x500);
    const double half=Value<float>(capsule,0x6d8);
    if(!std::isfinite(half)||half<=0)return Fail(1072,"original_enemy_invalid_original_capsule");
    const engine::Vec root{feet.x,feet.y,feet.z+half+2};
    // A round reset is the sole explicit relocation. It does not claim native
    // server respawn/death and does not overwrite movement or animation state.
    if(!SubmitInput(index,0,0,yaw)||!engine::SetTransform(target.pawn,root,{0,yaw,0}))
        return Fail(1073,"original_enemy_round_relocation_failed");
    target.anchor=feet;target.initialPosition=root;target.actuallyMoved=false;target.lastMotionTime=-1;
    target.direction=1;target.yaw=yaw;return true;
}
inline void PrintFootMotion(FILE* f,const FootMotion& motion){
    std::fprintf(f,"{\"sampled\":%s,\"pending\":%s,\"leader_mapped\":%s,\"space\":\"component\",\"cache_state\":",motion.sampled?"true":"false",motion.pending?"true":"false",motion.leaderMapped?"true":"false");original_players::Text(f,motion.cacheState);
    std::fprintf(f,",\"left_bone_index\":%d,\"right_bone_index\":%d,\"pose_source\":\"%p\",\"pose_frame\":%llu,\"pose_buffer_index\":%d,\"pose_count\":%d,\"pending_samples\":%u,\"samples\":%u,\"pose_changes\":%u,\"moving_samples\":%u,\"moving_pose_changes\":%u,\"maximum_step_cm\":%.9g,\"maximum_excursion_cm\":%.9g,\"maximum_moving_still_seconds\":%.9g,\"left_foot\":",motion.leftIndex,motion.rightIndex,motion.poseSource,static_cast<unsigned long long>(motion.poseFrame),motion.poseBufferIndex,motion.poseCount,motion.pendingSamples,motion.samples,motion.changes,motion.movingSamples,motion.movingChanges,motion.maximumStep,motion.maximumExcursion,motion.maximumMovingStillSeconds);
    original_players::Vector(f,motion.left);std::fputs(",\"right_foot\":",f);original_players::Vector(f,motion.right);std::fputc('}',f);
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"stage\":");original_players::Text(f,stage);
    std::fprintf(f,",\"error\":%u,\"restart_required\":%s,\"exception_rva\":%llu,\"exception_operation\":",error,b(restartRequired),static_cast<unsigned long long>(exceptionRva));original_players::Text(f,exceptionOperation);
    const auto eventName=original_players::Utf8(failedEventName);
    std::fputs(",\"failed_event\":",f);original_players::Text(f,eventName.c_str());
    std::fprintf(f,",\"failed_event_expected_parms_size\":%u,\"failed_event_actual_parms_size\":%u,\"failed_event_has_object\":%s,\"failed_event_has_function\":%s",static_cast<unsigned>(failedEventExpectedSize),static_cast<unsigned>(failedEventActualSize),b(failedEventHasObject),b(failedEventHasFunction));
    std::fprintf(f,",\"created\":%u,\"renderer_colors_applied\":%s,\"enemies\":[",created,b(colorsApplied));
    bool first=true;
    for(size_t i=0;i<enemies.size();++i){
        const auto& e=enemies[i];if(!e.attempted)continue;if(!first)std::fputc(',',f);first=false;
        std::fprintf(f,"{\"roster_index\":%zu,\"logical_id\":%u,\"hero_index\":%u,\"hero\":",i,e.logicalId,e.heroIndex);
        const auto hero=original_players::Utf8(HeroNames[e.heroIndex]);original_players::Text(f,hero.c_str());
        std::fprintf(f,",\"ready\":%s,\"enabled\":%s,\"selection_published\":%s,\"spawn_published\":%s,\"possession_published\":%s,\"team_published\":%s,\"native_enemy\":%s,\"native_local_control\":%s,\"outline_found\":%s,\"outline_attached\":%s,\"outline_applied\":%s,\"outline_mode\":%u",b(e.ready),b(e.enabled),b(e.selectionPublished),b(e.spawnPublished),b(e.possessionPublished),b(e.teamPublished),b(e.nativeEnemy),b(e.nativeLocalControl),b(e.outlineFound),b(e.outlineAttached),b(e.outlineApplied),static_cast<unsigned>(e.observedOutline));
        std::fprintf(f,",\"core_asset_ready\":%s,\"cosmetic_asset_ready\":%s,\"core_anim_ready\":%s,\"original_core_anim_class\":%s,\"material_slots_ready\":%s,\"actually_moved\":%s,\"samples\":%u,\"input_calls\":%u,\"world_input_calls\":%u,\"material_missing_samples\":%u,\"material_slot_changes\":%u,\"core_materials\":%d,\"cosmetic_materials\":%d,\"missing_cosmetic_materials\":%d",b(e.coreAssetReady),b(e.cosmeticAssetReady),b(e.coreAnimReady),b(e.originalCoreAnimClass),b(e.materialSlotsReady),b(e.actuallyMoved),e.samples,e.inputCalls,e.worldInputCalls,e.materialMissingSamples,e.materialSlotChanges,e.coreMaterialCount,e.cosmeticMaterialCount,e.missingMaterials);
        std::fprintf(f,",\"capsule_half_height\":%.9g,\"capsule_radius\":%.9g,\"original_capsule_half_height\":%.9g,\"original_capsule_radius\":%.9g",e.capsuleHalfHeight,e.capsuleRadius,e.originalCapsuleHalfHeight,e.originalCapsuleRadius);
        const char* vectorNames[]{"anchor","position","velocity","root_scale","core_relative_position","core_relative_scale","cosmetic_relative_scale"};
        const engine::Vec* vectors[]{&e.anchor,&e.position,&e.velocity,&e.rootScale,&e.coreRelativePosition,&e.coreRelativeScale,&e.cosmeticRelativeScale};
        for(size_t n=0;n<7;++n){std::fputc(',',f);original_players::Text(f,vectorNames[n]);std::fputc(':',f);original_players::Vector(f,*vectors[n]);}
        std::fputs(",\"last_world_input\":",f);original_players::Vector(f,e.lastWorldInput);
        std::fputs(",\"control_rotation\":",f);original_players::Vector(f,e.controlRotation);
        std::fputs(",\"actor_rotation\":",f);original_players::Vector(f,e.actorRotation);
        std::fprintf(f,",\"sampled_minimum_x\":%.9g,\"sampled_maximum_x\":%.9g",e.minimumX,e.maximumX);
        std::fputs(",\"original_core_foot_motion\":",f);PrintFootMotion(f,e.coreFeet);
        std::fputs(",\"original_cosmetic_foot_motion\":",f);PrintFootMotion(f,e.cosmeticFeet);
        std::fprintf(f,",\"pawn\":\"%p\",\"controller\":\"%p\",\"player_state\":\"%p\",\"core\":\"%p\",\"cosmetic\":\"%p\",\"core_asset\":\"%p\",\"cosmetic_asset\":\"%p\",\"core_anim\":\"%p\",\"cosmetic_anim\":\"%p\",\"outline\":\"%p\",\"material_manager\":\"%p\"}",e.pawn,e.controller,e.playerState,e.core,e.cosmetic,e.coreAsset,e.cosmeticAsset,e.coreAnim,e.cosmeticAnim,e.outline,e.materialManager);
    }
    std::fputs("],\"native_names\":[",f);first=true;
    for(size_t i=0;i<names.size();++i){
        const auto& n=names[i];if(!n.attempted)continue;if(!first)std::fputc(',',f);first=false;
        std::fprintf(f,"{\"roster_index\":%zu,\"setter_called\":%s,\"platform_provider_absent\":%s,\"full_name_verified\":%s,\"short_name_verified\":%s,\"owner_info_present\":%s,\"owner_info_name_verified\":%s",i,b(n.setterCalled),b(n.platformProviderAbsent),b(n.fullNameVerified),b(n.shortNameVerified),b(n.ownerInfoPresent),b(n.ownerInfoNameVerified));
        const char* nameFields[]{"requested","original_full_name","original_short_name","original_owner_info_name"};
        const wchar_t* nameValues[]{n.requested,n.actualFull,n.actualShort,n.actualOwnerInfo};
        for(size_t j=0;j<4;++j){std::fputc(',',f);original_players::Text(f,nameFields[j]);std::fputc(':',f);const auto value=original_players::Utf8(nameValues[j]);original_players::Text(f,value.c_str());}
        std::fputc('}',f);
    }
    std::fputs("],\"original_accessibility_colors\":",f);original_enemy_colors::PrintDiagnostics(f);
    std::fputs(",\"runtime_pixels_verified\":false,\"original_server_damage_or_respawn_claimed\":false}\n",f);
    const bool ok=std::ferror(f)==0;std::fclose(f);return ok;
}
}
