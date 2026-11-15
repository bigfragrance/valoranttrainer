#pragma once
#include "local_server_equipment.h"
#include "local_server_names.h"
#include "training_control.h"
#include "skin_effect_controls.h"
#include "local_server_cosmetics_timing.h"
#include "local_server_owned_paths.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

// Native equipment cosmetics, independent of the standalone weapon/effect
// singletons. Authority stores canonical preferences and can select native
// server FXC mappings. Non-replicated material/map choices travel as bounded
// presentation metadata to the actual paired client, never as gameplay state.
// Evidence/limits: work/local-server-playable/cosmetics/README.md.
namespace local_server_cosmetics {
namespace timing=local_server_cosmetics_timing;
using Guid=original_content::Guid;
using Weak=original_inventory_slots::WeakIdentity;
using Result=training_control::Result;
using Mode=skin_effect_controls::Mode;
using Kind=skin_effect_controls::Kind;
using Profile=skin_effect_controls::Profile;
inline constexpr size_t MaxPlayers=32,MaxControls=98,MaxEffects=32,MaxPreferences=8192;
inline constexpr uint64_t WireMagic=0x31534f4352545656ULL;
inline constexpr uint32_t WireVersion=2,PublishMilliseconds=250,MaximumAgeMilliseconds=2500;
inline constexpr uint32_t MetadataRetryMilliseconds=50,MetadataAccessDeniedGraceMilliseconds=3000,MetadataStallMilliseconds=5000;
inline constexpr auto& ChampionProfiles=skin_effect_controls::profiles;
enum class Reason:uint32_t {
    None,NativeRule,NativeMapVerified,NextOriginalEvent,WaitingWeapon,WaitingAssets,
    WaitingNativeMap,UnsupportedSkin,WaitingOriginalEffect,WaitingOriginalMaterials,
    WaitingChampion,WaitingOtherPlayer,WaitingOriginalKills,OriginalGraphRejected,
    NativeReadbackFailed,WaitingPeer,WaitingPeerScope,PeerVerified,PeerPending,
    RestoredOriginal,InvalidSource,Capacity,StaleMetadata,OriginalMaterialEvent,UnsupportedKey
};
inline const char* Explain(Reason value){
    switch(value){
    case Reason::NativeRule:return "native_original_cosmetic_rule_observed";
    case Reason::NativeMapVerified:return "native_override_map_and_original_getter_verified";
    case Reason::NextOriginalEvent:return "native_mapping_verified_next_original_event_controls_presentation";
    case Reason::WaitingWeapon:return "waiting_actual_native_weapon_and_canonical_skin";
    case Reason::WaitingAssets:return "waiting_original_attachment_preload";
    case Reason::WaitingNativeMap:return "waiting_actual_original_effect_override_map";
    case Reason::UnsupportedSkin:return "effect_not_supported_by_actual_original_skin_level";
    case Reason::WaitingOriginalEffect:return "waiting_live_original_owned_champion_effect";
    case Reason::WaitingOriginalMaterials:return "waiting_original_owned_material_objects";
    case Reason::WaitingChampion:return "waiting_original_native_champion_condition";
    case Reason::WaitingOtherPlayer:return "waiting_actual_other_native_player_state_for_nonleading_presentation";
    case Reason::WaitingOriginalKills:return "waiting_original_owner_kill_query_for_cosmetic_progression";
    case Reason::OriginalGraphRejected:return "original_blueprint_profile_or_entry_rejected";
    case Reason::NativeReadbackFailed:return "original_cosmetic_request_not_read_back";
    case Reason::WaitingPeer:return "waiting_ack_bound_actual_client_presentation";
    case Reason::WaitingPeerScope:return "waiting_matching_native_client_skin_and_epoch";
    case Reason::PeerVerified:return "actual_client_original_cosmetic_readback_verified_pixels_unverified";
    case Reason::PeerPending:return "actual_client_presentation_pending_original_runtime_objects";
    case Reason::RestoredOriginal:return "adapter_owned_override_restored_to_observed_native_rule";
    case Reason::InvalidSource:return "native_source_identity_or_ownership_mismatch";
    case Reason::Capacity:return "bounded_cosmetic_capacity_reached";
    case Reason::StaleMetadata:return "paired_cosmetic_metadata_expired_or_changed";
    case Reason::OriginalMaterialEvent:return "original_client_event_and_audited_material_leaf_completed_pixels_unverified";
    case Reason::UnsupportedKey:return "effect_key_not_in_actual_original_skin_capabilities";
    default:return "cosmetic_state_not_yet_observed";
    }
}
struct Scope {Guid weapon{},skin{},level{},chroma{};};
inline bool Same(const Scope& left,const Scope& right){return std::memcmp(&left,&right,sizeof(Scope))==0;}
inline bool Valid(const Scope& value){return original_content::Valid(value.weapon)&&original_content::Valid(value.skin)
    &&original_content::Valid(value.level)&&original_content::Valid(value.chroma);}
struct Preference {size_t index=0;Scope scope{};char key[80]{};Mode mode=Mode::FollowOriginal;float value=0;uint64_t revision=0;};
struct Control {
    char key[80]{},label[160]{},basePath[768]{},skinPath[768]{};
    Kind kind=Kind::OriginalEffectOverride;
    Mode requested=Mode::FollowOriginal,applied=Mode::FollowOriginal;
    float value=0,minimum=0,maximum=0,observedValue=0;
    bool supported=false,available=false,verified=false,numeric=false,nextOriginalEvent=false,dirty=false;
    bool serverMapVerified=false,peerVerified=false;
    void *baseClass=nullptr,*skinClass=nullptr,*nativeRule=nullptr,*lastApplied=nullptr;
    uint64_t baseHash=0,skinHash=0,applications=0;
    Reason reason=Reason::None;
};
struct Effect {
    void* actor=nullptr;Weak identity{};std::array<uint8_t,22> nativeId{};
    size_t profile=0;bool occupied=false,initializeAttempted=false,initialized=false,parentAttempted=false;
    bool dispatched=false;Mode lastMode=Mode::FollowOriginal;
    void *lastGlobal=nullptr,*lastAttacking=nullptr,*lastDefending=nullptr;
    void *originalGlobal=nullptr,*originalAttacking=nullptr,*originalDefending=nullptr;
    int32_t lastKills=-1;uint64_t playbackEpochs=0,dispatches=0,lastConditionRevision=0;
};
struct ChampionOwnershipProbe {
    const char* stage="not_sampled";uint32_t checks=0,actorWeakRejected=0,weaponWeakRejected=0,fieldRejected=0;
    uint32_t offsetRejected=0,getWorldRejected=0,worldRejected=0,elementRejected=0,castRejected=0,boundsRejected=0;
    void *actor=nullptr,*property=nullptr,*fieldClass=nullptr,*getWorld=nullptr,*observedWorld=nullptr,*owner=nullptr;
    void *expectedWorld=nullptr,*expectedWeapon=nullptr;
    Weak actorIdentity{};uintptr_t getWorldRva=0;uint64_t castFlags=0,propertyFlags=0;
    int32_t arrayDim=0,elementSize=0,offset=-1,classBytes=0;
    bool actorWeak=false,weaponWeak=false,fieldKnown=false,offsetMatches=false,getWorldKnown=false;
    bool worldQueried=false,worldMatches=false,elementMatches=false,castMatches=false,bounds=false,ownerMatches=false;
};
struct ChampionScan {
    void* type=nullptr;bool queried=false,queryValid=false;
    int32_t actors=-1;uint32_t exact=0,differentClass=0,ended=0,missingEquippable=0,otherWeapon=0,owned=0;
    ChampionOwnershipProbe ownership{};
};
struct Player {
    bool registered=false,human=false,sourceReady=false,snapshotReady=false,assetsReady=false;
    bool peerScope=false,settingsReceived=false,hasPeerReceipt=false,restartRequired=false;
    uint32_t index=0,epoch=0;unsigned error=0;
    void *controller=nullptr,*playerState=nullptr,*pawn=nullptr,*inventory=nullptr,*weapon=nullptr;
    void *skinData=nullptr,*levelData=nullptr,*chromaData=nullptr,*skinAttachment=nullptr,*chromaAttachment=nullptr;
    Weak controllerId{},stateId{},pawnId{},weaponId{},skinAttachmentId{},chromaAttachmentId{},skinRequestId{},chromaRequestId{};
    int32_t nativeMapEntries=-1;uint64_t preloadRequests=0;
    Scope scope{};uint64_t generation=0,lastScan=0,lastMapPoll=0,peerAt=0,lastAppliedConditionRevision=0;
    size_t controlCount=0,effectCount=0;std::array<Control,MaxControls> controls{};std::array<Effect,MaxEffects> effects{};
    uint64_t championScans=0;std::array<ChampionScan,std::size(ChampionProfiles)> championScan{};
    const char* stage="local_cosmetics_player_unregistered";
};
struct LeaderReplication {
    void *owner=nullptr,*outer=nullptr;
    uint8_t flagsA0=0,flagsC1=0,flagsC2=0;
    bool ownerMatches=false,outerMatches=false,weakIdentitiesLive=false,replicates=false,usesRegisteredList=false;
    bool arraysValid=false,registeredIdentityValid=false,conditionSupported=false,ready=false,pending=true;
    int32_t replicatedCount=0,registeredCount=0,replicatedMatches=0,registeredMatches=0,condition=-1;
    const char* stage="local_cosmetics_native_component_route_not_observed";
};
struct Leader {
    bool ready=false,created=false,creationAttempted=false,scriptsVerified=false,originalGameMode=false;
    void *gameState=nullptr,*component=nullptr,*type=nullptr,*global=nullptr,*attacking=nullptr,*defending=nullptr;
    Weak gameStateId{},componentId{};
    uint64_t lastPoll=0,lastFingerprint=0,assignments=0,broadcasts=0,conditionRevision=0;uint32_t lastBroadcastGeneration=0;
    int32_t eligibleCount=0;std::array<void*,MaxPlayers> beforeStates{};std::array<Weak,MaxPlayers> beforeIdentities{};std::array<int32_t,MaxPlayers> beforeKills{};
    LeaderReplication replication{};
    const char* stage="local_cosmetics_native_champion_not_bound";
};
struct State {
    bool initialized=false,host=false,profilesVerified=false,worldBound=false,peerBound=false,restartRequired=false;
    bool metadataReady=false;unsigned error=0,lastReject=0,lastWin32Error=0;
    uint32_t port=0,processId=0,hostPid=0,clientPid=0,rosterGeneration=0;
    uint64_t pairDigest=0,rosterDigest=0,sequence=0,lastPeerSequence=0,lastPeerChecksum=0;
    uint64_t polls=0,commands=0,settingsRevision=0,lastPublish=0,publications=0,receipts=0,rejections=0;
    uint64_t nativeMapWrites=0,materialGraphCalls=0,cosmeticCounterWrites=0,restores=0;
    uintptr_t exceptionRva=0;void* world=nullptr;DWORD gameThread=0;
    size_t activeIndex=0;const char* stage="local_cosmetics_not_initialized";
    const char* commandStage="none";Result commandResult=Result::Unavailable;
};
struct MetadataIO {
    const char* operation="none",*lastWriteOperation="none",*lastReadOperation="none",*contractError="none";
    std::wstring path,lastWritePath,lastReadPath,temporaryPath;
    DWORD win32Error=0,lastWriteError=0,lastReadError=0,cleanupError=0;
    uint64_t writeAttempts=0,writeFailures=0,transientFailures=0,readFailures=0,missingReads=0;
    uint64_t temporarySequence=0,consecutiveWriteFailures=0,firstWriteFailureAt=0,lastWriteFailureAt=0;
    uint64_t nextWriteAttemptAt=0,lastWriteSuccessAt=0;
    int64_t observedBytes=-1;uint32_t expectedBytes=0;
    bool retryPending=false,stalled=false,persistentFailure=false,lastReadMissing=false;
};
struct CanonicalLeaderIdentity {int32_t playerId=-1;void* playerState=nullptr;Weak identity{};};
struct LeaderLookup {
    bool profileVerified=false,sourceMetadataReady=false,resolvedMetadataReady=false;
    void* gameState=nullptr;Weak worldId{},gameStateId{};
    std::array<CanonicalLeaderIdentity,MaxPlayers> identities{};
    std::array<int32_t,3> sourcePlayerIds{{-1,-1,-1}},resolvedPlayerIds{{-1,-1,-1}};
    uint64_t resolutions=0,pending=0,invalidIds=0,duplicateIds=0,weakChanges=0,worldMismatches=0;
    int32_t lastPlayerId=-1;const char* stage="local_cosmetics_canonical_leaders_not_observed";
};
inline State state;
inline MetadataIO metadataIO;
inline Leader leader;
inline LeaderLookup leaderLookup;
inline std::array<Player,MaxPlayers> players{};
inline std::vector<Preference> preferences;
inline std::wstring roleDirectory,pairDirectory,authorityPath,clientPath;
inline const State& GetState(){return state;}
inline const MetadataIO& GetMetadataIO(){return metadataIO;}
inline const Leader& GetLeaderState(){return leader;}
inline const LeaderLookup& GetLeaderLookupState(){return leaderLookup;}
inline const Player* GetState(size_t index){return index<MaxPlayers?&players[index]:nullptr;}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Thread(){return !state.restartRequired&&engine::image&&original_players::Thread()
    &&(!state.gameThread||state.gameThread==GetCurrentThreadId());}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;return false;}
inline bool Fail(Player& player,unsigned code,const char* stage){player.error=code;player.stage=stage;return false;}
inline bool Wait(const char* stage){state.stage=stage;return true;}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Capture(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline uint64_t Hash(const void* bytes,size_t count){uint64_t result=14695981039346656037ULL;auto data=static_cast<const uint8_t*>(bytes);
    for(size_t i=0;i<count;++i)result=(result^data[i])*1099511628211ULL;return result;}
inline uint64_t Hash(const char* text){return text?Hash(text,std::strlen(text)):0;}
inline bool Event(void* object,const wchar_t* name,uint16_t bytes,void* arguments,void* outer=nullptr){
    auto fn=engine::Find(name);if(!(object&&fn&&Read<uint16_t>(fn,0xbe)==bytes&&(!outer||Read<void*>(fn,0x28)==outer)))return false;
    timing::DispatchScope measuredDispatch(timing::data);
    return engine::Event(object,name,arguments);
}
inline void* FieldType(void* type,const wchar_t* name){
    uint32_t nativeName[3]{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(nativeName,name,1);
    return type?engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,nativeName,3):nullptr;
}
inline void* Field(void* object,const wchar_t* name){return FieldType(Read<void*>(object,0x10),name);}
inline bool ObjectField(void* object,const wchar_t* name,void*& result){
    result=nullptr;auto property=Field(object,name);if(!property||Read<int32_t>(property,0x34)!=8)return false;
    const auto cast=Read<uint64_t>(Read<void*>(property,8),0x18);const auto offset=Read<int32_t>(property,0x44);
    if((cast&0x4008001ULL)!=0x4008001ULL||offset<0x30||offset+8>Read<int32_t>(Read<void*>(object,0x10),0x60))return false;
    result=Read<void*>(object,static_cast<size_t>(offset));return true;
}
template<size_t N> inline bool Path(void* object,char (&output)[N]){
    output[0]=0;struct Query{void* object;wchar_t* data=nullptr;int32_t count=0,capacity=0;} query{object};
    if(!Event(engine::DefaultObject(engine::Class(0x45aa110)),L"/Script/Engine.KismetSystemLibrary.GetPathName",24,&query))return false;
    bool okay=query.count>1&&query.count<=4096&&query.capacity>=query.count
        &&original_players::Readable(query.data,static_cast<size_t>(query.count)*2)&&query.data[query.count-1]==0;
    if(okay){const auto count=WideCharToMultiByte(CP_UTF8,0,query.data,query.count-1,output,static_cast<int>(N-1),nullptr,nullptr);
        okay=count>0;if(okay)output[count]=0;}
    if(query.data)engine::function<void(*)(void*)>(0x1915720)(query.data);return okay;
}
struct CodeProfile{uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr CodeProfile Profiles[]{
    {0x6465df0,246,0xcd04593e9f3a9c45ULL},{0x6465ef0,678,0xfd74da531bdafecULL},
    {0x70ee9a0,736,0x7b192edd39fae5ccULL},{0x71279a0,1178,0x4c280b9a26a4077dULL},
    {0x1c96db0,313,0x6b6fbfa845dbf9ddULL},{0x6e4ad90,1501,0xd40efc843a01a84aULL},
    {0x6eb4fc0,4126,0x2678620c3b073bb2ULL},
    {0x49452a0,31,0x9ad43e3e250e5fc5ULL},{0x3ed83b0,1609,0x15d2d4ecc2f49cf7ULL},
    {0x6fc27d0,25,0x8012c95c8c54123fULL},{0x2a64ee0,132,0x83dfe973e78a6846ULL}
};
inline bool VerifyProfiles(){
    if(state.profilesVerified)return true;
    for(const auto& profile:Profiles){auto code=reinterpret_cast<void*>(engine::image+profile.rva);
        if(!original_players::Readable(code,profile.bytes)||Hash(code,profile.bytes)!=profile.fnv)
            return Fail(5101,"local_cosmetics_original_native_profile_mismatch");}
    auto set=engine::Find(L"/Script/ShooterGame.AresEquippable.OverrideEffect"),get=engine::Find(L"/Script/ShooterGame.AresEquippable.GetOverridableEffect");
    auto field=FieldType(engine::Class(0x645c660),L"EffectOverrides");
    if(!set||!get||Read<uint16_t>(set,0xbe)!=56||Read<uint16_t>(get,0xbe)!=16
       ||!field||Read<int32_t>(field,0x44)!=0xcb0||(Read<uint64_t>(field,0x38)&0x20))
        return Fail(5102,"local_cosmetics_original_nonreplicated_map_schema_mismatch");
    state.profilesVerified=true;return true;
}
inline bool ValidKey(const char* key){
    if(!key||!std::memchr(key,0,80))return false;
    if(!std::strcmp(key,"champion_glow")||!std::strcmp(key,"skin_kill_count"))return true;
    if(std::strlen(key)!=20||std::strncmp(key,"fxc_",4))return false;
    for(size_t i=4;i<20;++i)if(!((key[i]>='0'&&key[i]<='9')||(key[i]>='a'&&key[i]<='f')))return false;
    return true;
}
inline Preference* Stored(size_t index,const Scope& scope,const char* key,bool create){
    for(auto& value:preferences)if(value.index==index&&Same(value.scope,scope)&&!std::strcmp(value.key,key))return &value;
    if(!create||preferences.size()==MaxPreferences)return nullptr;
    Preference value;value.index=index;value.scope=scope;strcpy_s(value.key,key);preferences.push_back(value);return &preferences.back();
}
inline void RestoreRequested(Player& player,Control& control){
    if(state.host)if(auto preferred=Stored(player.index,player.scope,control.key,false)){
        control.requested=preferred->mode;control.value=preferred->value;control.dirty=true;}
}
inline void* QueryOverride(void* weapon,void* base){struct Query{void* base;void* result=nullptr;} query{base};
    return Event(weapon,L"/Script/ShooterGame.AresEquippable.GetOverridableEffect",16,&query)?query.result:nullptr;}
inline int32_t MapCount(void* map){
    const auto allocated=Read<int32_t>(map,8),capacity=Read<int32_t>(map,12),bits=Read<int32_t>(map,0x28),free=Read<int32_t>(map,0x34);
    return allocated<0||allocated>2048||capacity<allocated||bits<allocated||free<0||free>allocated?-1:allocated-free;
}
inline void* BaseClass(void* entry){alignas(8) uint8_t borrowed[48]{};std::memcpy(borrowed,entry,sizeof(borrowed));
    return engine::function<void*(*)(void*)>(0x1c96db0)(borrowed);}
inline void* MapEntry(void* weapon,void* base){
    auto map=static_cast<uint8_t*>(weapon)+0xcb0;if(MapCount(map)<0)return nullptr;
    const auto count=Read<int32_t>(map,8);auto data=Read<uint8_t*>(map);auto bits=Read<uint32_t*>(map,0x20);
    if(!bits)bits=reinterpret_cast<uint32_t*>(map+0x10);
    if(count&&(!original_players::Readable(data,static_cast<size_t>(count)*64)
       ||!original_players::Readable(bits,static_cast<size_t>((count+31)/32)*4)))return nullptr;
    for(int32_t i=0;i<count;++i)if((bits[i/32]&(1U<<(i%32)))&&BaseClass(data+static_cast<size_t>(i)*64)==base)
        return data+static_cast<size_t>(i)*64;
    return nullptr;
}
inline bool SetMap(Player& player,Control& control,void* desired){
    auto entry=MapEntry(player.weapon,control.baseClass);if(!entry||!desired)return false;
    struct Params{alignas(8) uint8_t base[48]{};void* target=nullptr;} params;
    static_assert(sizeof(Params)==56&&offsetof(Params,target)==48);
    std::memcpy(params.base,entry,48);params.target=desired;
    if(!Event(player.weapon,L"/Script/ShooterGame.AresEquippable.OverrideEffect",56,&params)
       ||QueryOverride(player.weapon,control.baseClass)!=desired)return false;
    control.lastApplied=desired;++control.applications;++state.nativeMapWrites;return true;
}
inline bool ApplyMap(Player& player,Control& control){
    if(!control.baseClass||(!state.host&&!control.available))return false;auto actual=QueryOverride(player.weapon,control.baseClass);
    if(!actual){control.verified=false;control.reason=Reason::NativeReadbackFailed;return false;}
    const bool owned=control.applied!=Mode::FollowOriginal&&control.lastApplied&&actual==control.lastApplied;
    if(!owned){control.nativeRule=actual;control.lastApplied=nullptr;
        if(actual!=control.baseClass&&actual!=control.skinClass){control.skinClass=actual;Path(actual,control.skinPath);control.skinHash=Hash(control.skinPath);}}
    if(control.requested==Mode::FollowOriginal){
        if(owned&&actual!=control.nativeRule&&!SetMap(player,control,control.nativeRule))return false;
        control.verified=QueryOverride(player.weapon,control.baseClass)==control.nativeRule;
        if(control.verified){control.lastApplied=nullptr;control.applied=Mode::FollowOriginal;control.dirty=false;control.reason=Reason::NativeRule;}
        return control.verified;
    }
    auto desired=control.requested==Mode::ForceOff?control.baseClass:control.skinClass;
    if(actual!=desired&&!SetMap(player,control,desired)){control.reason=Reason::NativeReadbackFailed;return false;}
    control.verified=QueryOverride(player.weapon,control.baseClass)==desired;control.applied=control.requested;control.dirty=false;
    control.serverMapVerified=state.host&&control.verified;control.reason=control.verified?Reason::NextOriginalEvent:Reason::NativeReadbackFailed;
    return control.verified;
}
inline bool RestoreOwnedMaps(Player& player){
    if(!Live(player.weapon,player.weaponId))return true;
    for(size_t i=2;i<player.controlCount;++i){auto& control=player.controls[i];
        if(control.lastApplied&&control.nativeRule&&QueryOverride(player.weapon,control.baseClass)==control.lastApplied){
            if(control.lastApplied!=control.nativeRule&&!SetMap(player,control,control.nativeRule))return false;
            control.lastApplied=nullptr;control.applied=Mode::FollowOriginal;control.verified=false;++state.restores;}}
    return true;
}
inline bool AttachmentLoaded(void* component){
    if(!component)return false;auto iface=engine::function<void*(*)(void*,void*)>(0x1cff3d0)(component,engine::Class(0x68ed750));
    if(iface!=static_cast<uint8_t*>(component)+0xd8)return false;
    auto query=Read<void*>(Read<void*>(iface),0x28);
    return query==reinterpret_cast<void*>(engine::image+0x6e4ad90)&&reinterpret_cast<bool(*)(void*,uint8_t)>(query)(iface,7);
}
inline bool FindAttachments(Player& player){
    auto previousSkin=player.skinAttachment,previousChroma=player.chromaAttachment;
    const auto previousSkinId=player.skinAttachmentId,previousChromaId=player.chromaAttachmentId;
    struct Query{void* type;void** items=nullptr;int32_t count=0,capacity=0;} query{engine::Class(0x36837a0)};
    if(!Event(player.weapon,L"/Script/Engine.Actor.K2_GetComponentsByClass",24,&query))return false;
    bool okay=query.count>=0&&query.count<=512&&query.capacity>=query.count
        &&(!query.count||original_players::Readable(query.items,static_cast<size_t>(query.count)*8));
    player.skinAttachment=player.chromaAttachment=nullptr;
    if(okay)for(int32_t i=0;i<query.count;++i){auto component=query.items[i];
        if(!original_players::IsA(component,0x6574f50)||Read<void*>(component,0xc8)!=player.weapon)continue;
        if(Read<void*>(component,0xf8)==player.levelData)player.skinAttachment=component;
        if(Read<void*>(component,0xf8)==player.chromaData)player.chromaAttachment=component;}
    if(query.items)engine::function<void(*)(void*)>(0x1915720)(query.items);
    player.assetsReady=okay&&player.skinAttachment&&player.chromaAttachment
        &&AttachmentLoaded(player.skinAttachment)&&AttachmentLoaded(player.chromaAttachment);
    if(player.skinAttachment&&!Capture(player.skinAttachment,player.skinAttachmentId))return false;
    if(player.chromaAttachment&&!Capture(player.chromaAttachment,player.chromaAttachmentId))return false;
    if((previousSkin&&(previousSkin!=player.skinAttachment||!Live(previousSkin,previousSkinId)))
       ||(previousChroma&&(previousChroma!=player.chromaAttachment||!Live(previousChroma,previousChromaId)))){
        if(!RestoreOwnedMaps(player))return false;player.snapshotReady=false;player.effects={};player.effectCount=0;
        player.championScan={};player.championScans=0;
        player.lastAppliedConditionRevision=0;player.settingsReceived=player.hasPeerReceipt=player.peerScope=false;++player.generation;}
    return okay;
}
inline bool RequestAttachmentPreload(Player& player,void* component,Weak& requested){
    if(!component||AttachmentLoaded(component))return true;Weak identity{};
    if(!Capture(component,identity))return false;
    if(requested.index==identity.index&&requested.serial==identity.serial)return true;
    auto iface=engine::function<void*(*)(void*,void*)>(0x1cff3d0)(component,engine::Class(0x68ed750));
    if(iface!=static_cast<uint8_t*>(component)+0xd8||Read<void*>(Read<void*>(iface),0x28)!=reinterpret_cast<void*>(engine::image+0x6e4ad90))return false;
    auto manager=original_content_services::State().preloadManager;if(!manager)return false;
    requested=identity;original_content_services::NativeDelegate completion;
    // Original resource request/notification path. No completion bit, asset
    // path, material or private observer table is fabricated. A zero-path
    // request that the native manager does not notify remains explicitly
    // pending; this adapter does not guess an empty-path completion.
    engine::function<void(*)(void*,void*,uint8_t,int32_t,original_content_services::NativeDelegate*)>(0x6eb4fc0)
        (manager,iface,7,0,&completion);++player.preloadRequests;return true;
}
inline bool Context(const Player& player){
    return player.registered&&Live(player.controller,player.controllerId)&&Live(player.playerState,player.stateId)
        &&Live(player.pawn,player.pawnId)&&Live(player.weapon,player.weaponId)
        &&Read<void*>(player.controller,0x470)==player.playerState&&Read<void*>(player.controller,0x4b0)==player.pawn
        &&Read<void*>(player.pawn,0x4a0)==player.controller&&Read<void*>(player.pawn,0x488)==player.playerState&&Read<void*>(player.pawn,0xc08)==player.inventory
        &&Read<void*>(player.inventory,0x280)==player.weapon&&Read<void*>(player.weapon,0x210)==player.pawn
        &&Read<void*>(player.weapon,0x4a8)==player.pawn&&Read<uint8_t>(player.pawn,0x224)==(state.host?3:2)
        &&(state.host?Read<uint8_t>(player.weapon,0x224)==3:
            (Read<uint8_t>(player.weapon,0x224)>0&&Read<uint8_t>(player.weapon,0x224)<3));
}
inline bool SnapshotControls(Player& player){
    player.controls={};player.controlCount=2;auto& glow=player.controls[0];auto& kills=player.controls[1];
    strcpy_s(glow.key,"champion_glow");strcpy_s(glow.label,"冠军领先发光");glow.kind=Kind::ChampionGlow;
    strcpy_s(kills.key,"skin_kill_count");strcpy_s(kills.label,"皮肤击杀递进");kills.kind=Kind::ChampionKillCount;kills.numeric=true;kills.maximum=25;
    auto skinType=all_skins::SoftClass(player.levelData,0x58);char path[768]{};Path(skinType,path);
    std::string lower(path);for(auto& character:lower)if(character>='A'&&character<='Z')character=static_cast<char>(character-'A'+'a');
    const bool levelFour=lower.find("_lv4_skin_c")!=std::string::npos;
    for(const auto& profile:ChampionProfiles)if(std::strstr(path,profile.family)){
        glow.supported=levelFour;if(!std::strcmp(profile.family,"/Carbine/Champions2/"))kills.supported=levelFour;}
    glow.reason=glow.supported?Reason::WaitingOriginalEffect:Reason::UnsupportedSkin;
    kills.reason=kills.supported?Reason::WaitingOriginalEffect:Reason::UnsupportedSkin;
    RestoreRequested(player,glow);RestoreRequested(player,kills);
    auto map=static_cast<uint8_t*>(player.weapon)+0xcb0;const int32_t live=MapCount(map);if(live<0)return false;
    const auto count=Read<int32_t>(map,8);auto data=Read<uint8_t*>(map);auto bits=Read<uint32_t*>(map,0x20);
    if(!bits)bits=reinterpret_cast<uint32_t*>(map+0x10);
    if(count&&(!original_players::Readable(data,static_cast<size_t>(count)*64)
       ||!original_players::Readable(bits,static_cast<size_t>((count+31)/32)*4)))return false;
    for(int32_t i=0;i<count;++i){if(!(bits[i/32]&(1U<<(i%32))))continue;auto entry=data+static_cast<size_t>(i)*64;
        auto base=BaseClass(entry),skin=Read<void*>(entry,0x30);
        if(!base||!skin||base==skin||!original_players::Derived(base,engine::Class(0x648ada0))
           ||!original_players::Derived(skin,engine::Class(0x648ada0)))continue;
        if(player.controlCount==MaxControls)return false;auto& control=player.controls[player.controlCount++];
        control.baseClass=base;control.skinClass=control.nativeRule=skin;control.supported=true;control.nextOriginalEvent=true;
        if(!Path(base,control.basePath)||!Path(skin,control.skinPath))return false;
        control.baseHash=Hash(control.basePath);control.skinHash=Hash(control.skinPath);
        sprintf_s(control.key,"fxc_%016llx",static_cast<unsigned long long>(control.baseHash));
        char name[128]{};original_weapon::OriginalObjectName(skin,name);sprintf_s(control.label,"原皮效果：%s",name);
        control.available=QueryOverride(player.weapon,base)==skin;control.reason=control.available?Reason::NativeMapVerified:Reason::WaitingNativeMap;
        RestoreRequested(player,control);
    }
    player.snapshotReady=true;return true;
}
inline bool RefreshSource(Player& player){
    timing::PlayerScope measuredPlayerSource(timing::data,player.index);
    if(!player.registered||!Live(player.pawn,player.pawnId)||!Live(player.playerState,player.stateId))return false;
    auto inventory=Read<void*>(player.pawn,0xc08),weapon=Read<void*>(inventory,0x280);
    auto definition=all_weapons::FindByClass(Read<void*>(weapon,0x10));Scope scope;
    if(definition){scope={definition->id,Read<Guid>(weapon,0xfe8),Read<Guid>(weapon,0xff8),Read<Guid>(weapon,0x1038)};}
    if(!definition||!Valid(scope)||Read<void*>(weapon,0x210)!=player.pawn||Read<void*>(weapon,0x4a8)!=player.pawn){
        if(player.weapon&&!RestoreOwnedMaps(player))return false;
        for(size_t i=0;i<player.controlCount;++i){auto& control=player.controls[i];control.available=control.verified=control.peerVerified=false;control.reason=Reason::WaitingWeapon;}
        player.sourceReady=player.snapshotReady=player.assetsReady=false;player.stage=Explain(Reason::WaitingWeapon);return true;}
    all_skins::Selection canonical;
    if(!all_skins::Resolve(*definition,scope.skin,scope.level,scope.chroma,canonical)
       ||Read<void*>(weapon,0xfb0)!=canonical.skinData||Read<void*>(weapon,0xfb8)!=canonical.levelData
       ||Read<void*>(weapon,0xfd0)!=canonical.chromaData)return false;
    const bool changed=weapon!=player.weapon||!Live(player.weapon,player.weaponId)||!Same(scope,player.scope);
    if(changed){if(player.weapon&&!RestoreOwnedMaps(player))return false;
        player.weapon=weapon;player.inventory=inventory;player.scope=scope;
        player.skinData=canonical.skinData;player.levelData=canonical.levelData;player.chromaData=canonical.chromaData;
        player.snapshotReady=false;player.settingsReceived=player.hasPeerReceipt=player.peerScope=false;
        player.skinRequestId=player.chromaRequestId={};player.nativeMapEntries=-1;
        player.lastAppliedConditionRevision=0;
        player.effects={};player.effectCount=0;player.controlCount=0;++player.generation;
        player.championScan={};player.championScans=0;
        if(!Capture(weapon,player.weaponId))return false;
    }
    player.sourceReady=Context(player);if(!player.sourceReady)return false;
    if(!FindAttachments(player))return false;
    if(!RequestAttachmentPreload(player,player.skinAttachment,player.skinRequestId)
       ||!RequestAttachmentPreload(player,player.chromaAttachment,player.chromaRequestId))return false;
    if(!player.assetsReady&&!state.host){if(player.snapshotReady&&!RestoreOwnedMaps(player))return false;
        for(size_t i=0;i<player.controlCount;++i){auto& control=player.controls[i];control.available=control.verified=control.peerVerified=false;control.reason=Reason::WaitingAssets;}
        player.snapshotReady=false;player.stage=Explain(Reason::WaitingAssets);return true;}
    const auto entries=MapCount(static_cast<uint8_t*>(player.weapon)+0xcb0);
    if(player.snapshotReady&&entries!=player.nativeMapEntries){if(!RestoreOwnedMaps(player))return false;player.snapshotReady=false;}
    if(!player.snapshotReady&&!SnapshotControls(player))return false;
    player.nativeMapEntries=entries;
    player.stage=player.assetsReady?"local_cosmetics_actual_canonical_native_weapon_bound":"local_cosmetics_authority_scope_waiting_original_cosmetic_assets";return true;
}
inline std::wstring TypePath(const Profile& profile){std::wstring result=profile.package;const auto slash=result.find_last_of(L'/');
    return result+L"."+result.substr(slash+1)+L"_C";}
inline bool Graph(void* actor,const Profile& profile,int32_t entry){
    if(state.host)return false;
    const auto type=local_server_cosmetics::TypePath(profile),leaf=type.substr(type.find_last_of(L'.')+1);
    const auto function=type+L".ExecuteUbergraph_"+leaf.substr(0,leaf.size()-2);
    auto fn=engine::Find(function.c_str());auto code=Read<const uint8_t*>(fn,0x68);
    uint8_t expected=entry==profile.initializeOne?profile.entryOpcodes[0]:entry==profile.initializeTwo?profile.entryOpcodes[1]
        :entry==profile.update?profile.entryOpcodes[2]:0;
    if(&profile==&ChampionProfiles[1]&&entry==7600)expected=0x0f;
    if(!fn||Read<void*>(fn,0x28)!=Read<void*>(actor,0x10)||Read<int32_t>(fn,0x70)!=profile.memoryBytes
       ||entry<0||entry>=profile.memoryBytes||!expected||!original_players::Readable(code,static_cast<size_t>(profile.memoryBytes))
       ||code[0]!=0x4c||code[5]!=0x4e||code[entry]!=expected)return false;
    if(!Event(actor,function.c_str(),4,&entry,Read<void*>(actor,0x10)))return false;
    ++state.materialGraphCalls;return true;
}
inline bool ParentGraph(void* actor,int32_t entry){
    constexpr auto typePath=L"/Game/Equippables/Guns/Rifles/Carbine/Champions2024/FXC/FXC_Carbine_Champions2024_WhileEquipped_1P_Smoke_Lv2.FXC_Carbine_Champions2024_WhileEquipped_1P_Smoke_Lv2_C";
    constexpr auto function=L"/Game/Equippables/Guns/Rifles/Carbine/Champions2024/FXC/FXC_Carbine_Champions2024_WhileEquipped_1P_Smoke_Lv2.FXC_Carbine_Champions2024_WhileEquipped_1P_Smoke_Lv2_C.ExecuteUbergraph_FXC_Carbine_Champions2024_WhileEquipped_1P_Smoke_Lv2";
    auto type=engine::Find(typePath),fn=engine::Find(function);auto code=Read<const uint8_t*>(fn,0x68);
    if(state.host||(entry!=96&&entry!=366)||!type||!fn||Read<void*>(fn,0x28)!=type
       ||!original_players::Derived(Read<void*>(actor,0x10),type)||Read<int32_t>(fn,0x70)!=426
       ||!original_players::Readable(code,426)||code[0]!=0x4e||code[entry]!=0x5f||code[425]!=0x53)return false;
    if(!Event(actor,function,4,&entry,type))return false;++state.materialGraphCalls;return true;
}
inline bool SetPresentationObject(void* object,const wchar_t* name,void* value){
    if(state.host)return false;auto property=Field(object,name);
    if(!property||(Read<uint64_t>(property,0x38)&0x20)||Read<int32_t>(property,0x34)!=8)return false;
    struct Query{void* object;uint8_t name[12]{};uint8_t pad[4]{};void* value;} query{object,{}, {},value};
    static_assert(sizeof(Query)==32&&offsetof(Query,value)==24);
    std::memcpy(query.name,static_cast<uint8_t*>(property)+0x20,12);void* observed=nullptr;
    return Event(engine::DefaultObject(engine::Class(0x45aa110)),L"/Script/Engine.KismetSystemLibrary.SetObjectPropertyByName",32,&query)
        &&ObjectField(object,name,observed)&&observed==value;
}
inline bool LeaderScripts(){
    struct Expected{const wchar_t* name;int32_t bytes;uint16_t parameters;uint8_t first;};
    constexpr Expected expected[]{
        {L"Assign Kill Champion",1761,8,0x4c},{L"GetHighestPlayer",735,97,0x31},
        {L"ExecuteUbergraph_Comp_GameState_KillChampion",452,4,0x4e},
        {L"MulticastUpdateChampion",36,8,0x64},{L"MulticastUpdateTeamChampion",54,16,0x64},{L"ReceiveBeginPlay",18,0,0x46}};
    for(const auto& item:expected){const auto function=std::wstring(training_champion_state::MethodPrefix)+item.name;
        auto fn=engine::Find(function.c_str());auto code=Read<const uint8_t*>(fn,0x68);
        if(!fn||Read<void*>(fn,0x28)!=leader.type||Read<uint16_t>(fn,0xbe)!=item.parameters||Read<int32_t>(fn,0x70)!=item.bytes
           ||!original_players::Readable(code,static_cast<size_t>(item.bytes))||code[0]!=item.first||code[item.bytes-1]!=0x53)return false;
        if(item.first==0x64&&(Read<uint32_t>(fn,0xb8)!=0x0c004040U))return false;
    }
    return true;
}
inline bool ReadLeaders(){
    if(!Live(leader.gameState,leader.gameStateId)||!Live(leader.component,leader.componentId)
       ||Read<void*>(leader.component,0xc8)!=leader.gameState)return false;
    if(!ObjectField(leader.component,L"Champion",leader.global)||!ObjectField(leader.component,L"AttackingChampion",leader.attacking)
       ||!ObjectField(leader.component,L"DefendingChampion",leader.defending))return false;
    for(const auto value:{leader.global,leader.attacking,leader.defending})if(value&&!original_players::IsA(value,original_players::ShooterStateClass))return false;
    return true;
}
inline bool BindLeader(){
    auto gameState=Read<void*>(state.world,0x178);if(!original_players::IsA(gameState,original_players::ShooterGameStateClass))return true;
    if(!leader.type){leader.type=engine::Find(training_champion_state::ClassPath);
        if(!leader.type)leader.type=engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t)>(0x1d0c6e0)
            (engine::Class(0x36837a0),nullptr,training_champion_state::ClassPath,nullptr,0);}
    if(!leader.type){leader.stage="local_cosmetics_original_champion_class_pending";return true;}
    if(!original_players::Derived(leader.type,engine::Class(0x36837a0)))return Fail(5103,"local_cosmetics_original_champion_component_class_mismatch");
    if(!LeaderScripts())return Fail(5103,"local_cosmetics_original_champion_blueprint_profile_mismatch");leader.scriptsVerified=true;
    struct Query{void* type;void* component=nullptr;} query{leader.type};
    if(!Event(gameState,L"/Script/Engine.Actor.GetComponentByClass",16,&query))return false;
    if(!query.component&&state.host){
        if(leader.creationAttempted)return Fail(5104,"local_cosmetics_native_champion_partial_creation_requires_new_pair");
        if(Read<uint8_t>(gameState,0x224)!=3)return Fail(5105,"local_cosmetics_champion_authority_game_state_missing");
        leader.creationAttempted=true;
        struct alignas(16) Add{void* type;uint8_t manual=0,pad[7]{};engine::Transform transform;uint8_t deferred=1,pad2[7]{};void* result=nullptr;} add{leader.type};
        static_assert(sizeof(Add)==128);
        if(!Event(gameState,L"/Script/Engine.Actor.AddComponentByClass",128,&add)||!add.result)return false;
        struct alignas(16) Finish{void* component;uint8_t manual=0,pad[7]{};engine::Transform transform;} finish{add.result};
        static_assert(sizeof(Finish)==112);
        if(!Event(gameState,L"/Script/Engine.Actor.FinishAddComponent",112,&finish))return false;
        query.component=add.result;leader.created=true;
    }
    if(!query.component){leader.ready=false;leader.stage="local_cosmetics_waiting_native_champion_component_replication";return true;}
    if(Read<void*>(query.component,0xc8)!=gameState||Read<void*>(query.component,0x28)!=gameState)return false;
    leader.gameState=gameState;leader.component=query.component;
    if(!Capture(gameState,leader.gameStateId)||!Capture(query.component,leader.componentId))return false;
    for(const auto name:{L"Champion",L"AttackingChampion",L"DefendingChampion"}){auto property=Field(query.component,name);
        if(!property||Read<int32_t>(property,0x34)!=8||(Read<uint64_t>(property,0x38)&0x20))
            return Fail(5106,"local_cosmetics_champion_multicast_only_field_schema_mismatch");}
    if(state.host){uint8_t replicated=1;if(!Event(query.component,L"/Script/Engine.ActorComponent.SetIsReplicated",1,&replicated))return false;}
    leader.originalGameMode=original_players::IsA(Read<void*>(state.world,0x170),0x6c8ec60);
    leader.ready=ReadLeaders();leader.stage=leader.ready?"local_cosmetics_original_champion_component_bound":"local_cosmetics_champion_fields_pending";return true;
}
inline bool PollLeader(){
    timing::ContextScope measuredLeaderContext(timing::data,timing::Context::Leader);
    timing::StageScope measuredLeader(timing::data,timing::Section::Leader);
    if(!leader.ready||!Live(leader.component,leader.componentId)){if(!BindLeader())return false;}
    if(!leader.ready)return true;if(!state.host)return ReadLeaders();
    const auto now=GetTickCount64();if(now-leader.lastPoll<250)return ReadLeaders();leader.lastPoll=now;
    // Native sorted-roster795B180/6E15E80 uses actual World.GameState and
    // PlayerArray, not World.AuthorityGameMode. Original BP BeginPlay binds
    // mode events; this host's real roster-change/Assign poll supplies that
    // missing event source without replacing its ranking or tie rules.
    struct Query{void* world;void* viewer;uint8_t alliance=3,omitBots=1,omitCoaches=1,pad[5]{};void** items=nullptr;int32_t count=0,capacity=0;} query{leader.component,leader.global};
    static_assert(sizeof(Query)==40&&offsetof(Query,items)==24);
    if(!Event(engine::DefaultObject(engine::Class(0x6c14c60)),L"/Script/ShooterGame.ShooterBlueprintLibrary.GetShooterPlayerStatesSortedByScore",40,&query))return false;
    bool okay=query.count>=0&&query.count<=static_cast<int32_t>(MaxPlayers)&&query.capacity>=query.count
        &&(!query.count||original_players::Readable(query.items,static_cast<size_t>(query.count)*8));
    std::array<void*,MaxPlayers> nextStates{};std::array<Weak,MaxPlayers> nextIdentities{};std::array<int32_t,MaxPlayers> nextKills{};
    void* killer=nullptr;uint64_t fingerprint=14695981039346656037ULL;
    if(okay)for(int32_t i=0;i<query.count;++i){
        timing::LeaderEntryScope measuredLeaderEntry(timing::data,static_cast<size_t>(i));
        auto ps=query.items[i];void* info=nullptr;int32_t kills=-1;Weak identity{};
        if(!original_players::IsA(ps,original_players::ShooterStateClass)||!Capture(ps,identity)
           ||!Event(ps,L"/Script/ShooterGame.ShooterPlayerState.GetOwnerExclusivePlayerInfo",8,&info)||!info
           ||!Event(info,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.GetKills",4,&kills)||kills<0){okay=false;break;}
        struct Team{void* actor;void* result=nullptr;} team{info};uint8_t teamRole=0;
        if(!Event(engine::DefaultObject(engine::Class(0x65a0b40)),L"/Script/ShooterGame.BaseTeamComponent.GetTeamComponent",16,&team)
           ||!team.result||!Event(team.result,L"/Script/ShooterGame.BaseTeamComponent.GetTeamRole",1,&teamRole)){okay=false;break;}
        nextStates[static_cast<size_t>(i)]=ps;nextIdentities[static_cast<size_t>(i)]=identity;nextKills[static_cast<size_t>(i)]=kills;
        const uint64_t values[]{static_cast<uint32_t>(identity.index),static_cast<uint32_t>(identity.serial),static_cast<uint32_t>(kills),teamRole};
        fingerprint^=Hash(values,sizeof(values));fingerprint*=1099511628211ULL;
        for(size_t before=0;before<MaxPlayers;++before)if(leader.beforeStates[before]==ps
           &&leader.beforeIdentities[before].index==identity.index&&leader.beforeIdentities[before].serial==identity.serial
           &&kills>leader.beforeKills[before])killer=ps;
    }
    if(query.items)engine::function<void(*)(void*)>(0x1915720)(query.items);
    if(!okay){leader.stage="local_cosmetics_original_champion_roster_pending";return true;}
    leader.eligibleCount=query.count;leader.beforeStates=nextStates;leader.beforeIdentities=nextIdentities;leader.beforeKills=nextKills;
    if(!leader.assignments||fingerprint!=leader.lastFingerprint){
        const auto function=std::wstring(training_champion_state::MethodPrefix)+L"Assign Kill Champion";
        if(!Event(leader.component,function.c_str(),8,&killer,leader.type))return false;
        ++leader.assignments;++leader.conditionRevision;leader.lastFingerprint=fingerprint;
    }
    if(!ReadLeaders())return false;
    leader.stage="local_cosmetics_original_ranking_and_native_multicast_observed";return true;
}
inline bool ObserveLeaderReplication(){
    auto& observed=leader.replication;observed={};
    if(!leader.gameState||!leader.component){observed.stage="local_cosmetics_waiting_native_component_source";return true;}
    observed.weakIdentitiesLive=Live(leader.gameState,leader.gameStateId)&&Live(leader.component,leader.componentId);
    if(!observed.weakIdentitiesLive)return Fail(5122,"local_cosmetics_native_component_replication_weak_identity_changed");
    observed.owner=Read<void*>(leader.component,0xc8);observed.outer=Read<void*>(leader.component,0x28);
    observed.ownerMatches=observed.owner==leader.gameState;observed.outerMatches=observed.outer==leader.gameState;
    observed.flagsA0=Read<uint8_t>(leader.component,0xa0);observed.flagsC1=Read<uint8_t>(leader.component,0xc1);
    observed.flagsC2=Read<uint8_t>(leader.component,0xc2);
    if(!observed.ownerMatches||!observed.outerMatches||Read<void*>(state.world,0x178)!=leader.gameState)
        return Fail(5123,"local_cosmetics_native_component_replication_owner_or_world_changed");
    // Original SetIsReplicated4131070 uses A0&20 and original3EE5170
    // maintains these actual owner replication lists. C1&10 is bAutoActivate;
    // it is not a replication requirement. A0&1 is not used by this route.
    observed.replicates=(observed.flagsA0&0x20)!=0;
    observed.usesRegisteredList=(Read<uint8_t>(leader.gameState,0x11b)&1)!=0;
    auto replicated=Read<void**>(leader.gameState,0x330);
    const auto replicatedCount=Read<int32_t>(leader.gameState,0x338),replicatedCapacity=Read<int32_t>(leader.gameState,0x33c);
    auto registered=Read<uint8_t*>(leader.gameState,0x320);
    const auto registeredCount=Read<int32_t>(leader.gameState,0x328),registeredCapacity=Read<int32_t>(leader.gameState,0x32c);
    observed.replicatedCount=replicatedCount;observed.registeredCount=registeredCount;
    observed.arraysValid=replicatedCount>=0&&replicatedCount<=512&&replicatedCapacity>=replicatedCount&&replicatedCapacity<=8192
        &&registeredCount>=0&&registeredCount<=512&&registeredCapacity>=registeredCount&&registeredCapacity<=8192
        &&(!replicatedCount||(replicated&&original_players::Readable(replicated,static_cast<size_t>(replicatedCount)*8)))
        &&(!registeredCount||(registered&&original_players::Readable(registered,static_cast<size_t>(registeredCount)*0x28)));
    if(!observed.arraysValid)return Fail(5124,"local_cosmetics_native_component_replication_array_bounds_invalid");
    for(int32_t i=0;i<replicatedCount;++i)if(replicated[i]==leader.component)++observed.replicatedMatches;
    for(int32_t i=0;i<registeredCount;++i){auto row=registered+static_cast<size_t>(i)*0x28;
        if(Read<void*>(row)!=leader.component)continue;++observed.registeredMatches;
        observed.condition=Read<int32_t>(row,0x10);observed.registeredIdentityValid=Live(leader.component,Read<Weak>(row,8));}
    if(observed.registeredMatches&&!observed.registeredIdentityValid)
        return Fail(5122,"local_cosmetics_registered_component_replication_weak_identity_changed");
    // The original GameState component default condition is COND_None(0).
    // Other native conditions remain visible/pending rather than invented.
    observed.conditionSupported=observed.condition==0;
    const bool membership=observed.usesRegisteredList
        ?observed.registeredMatches==1&&observed.registeredIdentityValid&&observed.conditionSupported:observed.replicatedMatches==1;
    observed.ready=leader.ready&&observed.replicates&&membership;observed.pending=!observed.ready;
    observed.stage=observed.ready?"local_cosmetics_actual_native_component_replication_route_observed":
        !observed.replicates?"local_cosmetics_waiting_original_component_replication_flag":
        "local_cosmetics_waiting_actual_native_component_replication_membership";
    return true;
}
template<class Visitor> inline bool VisitReplicationGraph(Visitor visitor){
    if(!state.host)return true;
    if(!ObserveLeaderReplication())return false;
    // This optional source cannot suppress already valid native actors while
    // its ordinary component route is pending. No component is cast as Actor.
    if(leader.replication.ready)visitor(leader.gameState,leader.component);return true;
}
inline void* OtherState(void* local){
    auto gameState=Read<void*>(state.world,0x178);auto data=Read<void**>(gameState,0x480);const auto count=Read<int32_t>(gameState,0x488);
    if(count<0||count>static_cast<int32_t>(MaxPlayers)||(!data&&count))return nullptr;
    for(int32_t i=0;i<count;++i){auto ps=Read<void*>(data,static_cast<size_t>(i)*8);
        if(ps&&ps!=local&&original_players::IsA(ps,original_players::ShooterStateClass))return ps;}
    return nullptr;
}
inline Effect* Track(Player& player,void* actor,size_t profile){
    Weak identity{};if(!Capture(actor,identity))return nullptr;std::array<uint8_t,22> nativeId{};
    std::memcpy(nativeId.data(),static_cast<uint8_t*>(actor)+0x480,nativeId.size());Effect* empty=nullptr;
    for(auto& effect:player.effects){if(effect.occupied&&!Live(effect.actor,effect.identity))effect={};
        if(effect.occupied&&effect.identity.index==identity.index&&effect.identity.serial==identity.serial){
            if(effect.nativeId!=nativeId){const auto epoch=effect.playbackEpochs+1;effect={};effect.occupied=true;effect.actor=actor;
                effect.identity=identity;effect.profile=profile;effect.nativeId=nativeId;effect.playbackEpochs=epoch;}return &effect;}
        if(!effect.occupied&&!empty)empty=&effect;}
    if(!empty)return nullptr;empty->occupied=true;empty->actor=actor;empty->identity=identity;empty->profile=profile;empty->nativeId=nativeId;empty->playbackEpochs=1;return empty;
}
inline bool EffectWeapon(Player& player,void* actor,size_t profile,void*& equippable){
    equippable=nullptr;
    if(profile!=0)return ObjectField(actor,L"Equippable",equippable);
    // The 2021 authored effect directly extends EffectContainer and has no
    // Equippable property. Its original PlayClientSimulatedEffect receives
    // the skin's weapon as AttachToActor; the real manager's owner becomes the
    // spawned FXC Actor.Owner. Preserve that original ownership route.
    auto& d=player.championScan[profile].ownership;const auto previous=d;d={};d.checks=previous.checks+1;
    d.actorWeakRejected=previous.actorWeakRejected;d.weaponWeakRejected=previous.weaponWeakRejected;d.fieldRejected=previous.fieldRejected;
    d.offsetRejected=previous.offsetRejected;d.getWorldRejected=previous.getWorldRejected;d.worldRejected=previous.worldRejected;
    d.elementRejected=previous.elementRejected;d.castRejected=previous.castRejected;d.boundsRejected=previous.boundsRejected;
    d.actor=actor;d.expectedWorld=state.world;d.expectedWeapon=player.weapon;
    d.actorWeak=Capture(actor,d.actorIdentity);d.weaponWeak=Live(player.weapon,player.weaponId);
    if(!d.actorWeak){++d.actorWeakRejected;d.stage="fxc_actor_weak_not_live";return false;}
    if(!d.weaponWeak){++d.weaponWeakRejected;d.stage="current_weapon_weak_not_live";return false;}
    d.property=Field(actor,L"Owner");d.fieldKnown=d.property!=nullptr;
    if(!d.fieldKnown){++d.fieldRejected;d.stage="original_actor_owner_property_not_found";return false;}
    d.fieldClass=Read<void*>(d.property,8);d.castFlags=Read<uint64_t>(d.fieldClass,0x18);
    d.propertyFlags=Read<uint64_t>(d.property,0x38);d.arrayDim=Read<int32_t>(d.property,0x30);
    d.elementSize=Read<int32_t>(d.property,0x34);d.offset=Read<int32_t>(d.property,0x44);
    d.classBytes=Read<int32_t>(Read<void*>(actor,0x10),0x60);d.offsetMatches=d.offset==0x210;
    d.elementMatches=d.elementSize==8;d.castMatches=(d.castFlags&0x4008001ULL)==0x4008001ULL;
    d.bounds=d.offset>=0x30&&d.offset+8<=d.classBytes;
    d.getWorld=Read<void*>(Read<void**>(actor),0x1b8);const auto address=reinterpret_cast<uintptr_t>(d.getWorld);
    d.getWorldRva=address>=engine::image?address-engine::image:address;
    // EffectContainer's original override asks its actual OwningManager for
    // World; only its null-manager branch uses Actor.GetWorld. Both complete
    // native implementations are pinned, not arbitrary virtual overrides.
    d.getWorldKnown=d.getWorld==reinterpret_cast<void*>(engine::image+0x3ed83b0)
        ||d.getWorld==reinterpret_cast<void*>(engine::image+0x6fc27d0);
    if(!d.offsetMatches){++d.offsetRejected;d.stage="original_actor_owner_offset_mismatch";return false;}
    if(!d.getWorldKnown){++d.getWorldRejected;d.stage="actual_fxc_get_world_not_profiled";return false;}
    d.observedWorld=reinterpret_cast<void*(*)(void*)>(d.getWorld)(actor);d.worldQueried=true;
    d.worldMatches=d.observedWorld==state.world;
    if(!d.worldMatches){++d.worldRejected;d.stage="actual_fxc_original_world_mismatch";return false;}
    if(!d.elementMatches){++d.elementRejected;d.stage="original_actor_owner_element_size_mismatch";return false;}
    if(!d.castMatches){++d.castRejected;d.stage="original_actor_owner_cast_flags_mismatch";return false;}
    if(!d.bounds){++d.boundsRejected;d.stage="original_actor_owner_outside_actual_class";return false;}
    equippable=Read<void*>(actor,static_cast<size_t>(d.offset));d.owner=equippable;d.ownerMatches=equippable==player.weapon;
    d.stage=d.ownerMatches?"original_actor_owner_matches_current_weapon":"original_actor_owner_is_other_weapon";return true;
}
inline bool InitializeEffect(Player& player,Effect& effect){
    if(state.host||!Context(player)||!Live(effect.actor,effect.identity))return false;
    auto& control=player.controls[0];const auto& profile=ChampionProfiles[effect.profile];void* equippable=nullptr;
    EffectWeapon(player,effect.actor,effect.profile,equippable);
    if(equippable!=player.weapon){control.reason=Reason::InvalidSource;return false;}
    void* owner=nullptr;ObjectField(effect.actor,profile.ownerField,owner);
    if(effect.profile==3){
        auto target=original_effects::ResolveIdentity(Read<original_effects::WeakIdentity>(effect.actor,0x620));
        if(Read<void*>(effect.actor,0x210)!=player.pawn||(target&&target!=player.pawn)){control.reason=Reason::InvalidSource;return false;}
        void* presentation=nullptr;ObjectField(effect.actor,L"My FXC Target",presentation);
        if((owner!=player.skinAttachment||presentation!=player.pawn)&&!effect.parentAttempted){
            effect.parentAttempted=true;
            if(!ParentGraph(effect.actor,96)||!ParentGraph(effect.actor,366)){control.reason=Reason::OriginalGraphRejected;return false;}
            ObjectField(effect.actor,profile.ownerField,owner);ObjectField(effect.actor,L"My FXC Target",presentation);}
        if(owner!=player.skinAttachment||presentation!=player.pawn){control.reason=Reason::WaitingOriginalMaterials;return false;}
        void* actualComponent=nullptr;
        if(!leader.ready||!ObjectField(effect.actor,L"myCompGameStateKillChampion",actualComponent)){control.reason=Reason::WaitingChampion;return false;}
        if(actualComponent&&actualComponent!=leader.component){control.reason=Reason::InvalidSource;return false;}
        if(!actualComponent&&!SetPresentationObject(effect.actor,L"myCompGameStateKillChampion",leader.component))return false;
    }
    void *one=nullptr,*two=nullptr;
    if(owner!=player.skinAttachment||!ObjectField(effect.actor,profile.materialOne,one)||!one
       ||!ObjectField(effect.actor,profile.materialTwo,two)||!two){
        if(effect.initializeAttempted){control.reason=Reason::WaitingOriginalMaterials;return false;}
        effect.initializeAttempted=true;
        if(!local_server_cosmetics::Graph(effect.actor,profile,profile.initializeOne)||(profile.initializeTwo&&!local_server_cosmetics::Graph(effect.actor,profile,profile.initializeTwo))){
            control.reason=Reason::OriginalGraphRejected;return false;}
        ObjectField(effect.actor,profile.ownerField,owner);ObjectField(effect.actor,profile.materialOne,one);ObjectField(effect.actor,profile.materialTwo,two);
    }
    effect.initialized=owner==player.skinAttachment&&one&&two;
    if(effect.initialized&&effect.profile==3){auto property=Field(effect.actor,L"VFXMesh List");
        if(!property||Read<int32_t>(property,0x34)!=16)return false;auto array=static_cast<uint8_t*>(effect.actor)+Read<int32_t>(property,0x44);
        const auto count=Read<int32_t>(array,8),capacity=Read<int32_t>(array,12);auto data=Read<void**>(array);
        if(count<=0||count>256||capacity<count||!original_players::Readable(data,static_cast<size_t>(count)*8))return false;
        for(int32_t i=0;i<count;++i)if(!data[i])return false;}
    if(!effect.initialized)control.reason=Reason::WaitingOriginalMaterials;return effect.initialized;
}
inline bool ApplyKillCount(Player& player,Effect& effect){
    auto& control=player.controls[1];if(effect.profile!=1||!effect.initialized)return false;
    control.reason=Reason::WaitingOriginalMaterials;
    void* component=nullptr;
    if(!ObjectField(effect.actor,L"Comp_FXC_Game_Events",component)||!component||Read<void*>(component,0xc8)!=effect.actor)return false;
    auto property=Field(component,L"MyKills");if(!property||Read<int32_t>(property,0x34)!=4||(Read<uint64_t>(property,0x38)&0x20))return false;
    const auto offset=Read<int32_t>(property,0x44);if(offset<0x30||offset+4>Read<int32_t>(Read<void*>(component,0x10),0x60))return false;
    int32_t desired=0;
    if(control.requested==Mode::ForceOn)desired=static_cast<int32_t>(control.value);
    else if(control.requested==Mode::FollowOriginal){void* info=nullptr;
        if(!Event(player.pawn,L"/Script/ShooterGame.ShooterCharacter.GetOwnerExclusivePlayerInfo",8,&info)||!info
           ||!Event(info,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.GetKills",4,&desired)||desired<0){control.reason=Reason::WaitingOriginalKills;return false;}}
    if(control.dirty||control.applied!=control.requested||Read<int32_t>(component,static_cast<size_t>(offset))!=desired||effect.lastKills!=desired){
        struct Query{void* object;uint8_t name[12]{};int32_t value=0;} query{component};static_assert(sizeof(Query)==24&&offsetof(Query,value)==20);
        std::memcpy(query.name,static_cast<uint8_t*>(property)+0x20,12);query.value=desired;
        if(!Event(engine::DefaultObject(engine::Class(0x45aa110)),L"/Script/Engine.KismetSystemLibrary.SetIntPropertyByName",24,&query)
           ||Read<int32_t>(component,static_cast<size_t>(offset))!=desired||!local_server_cosmetics::Graph(effect.actor,ChampionProfiles[1],7600))return false;
        effect.lastKills=desired;++control.applications;++state.cosmeticCounterWrites;player.controls[0].dirty=true;
    }
    control.available=control.verified=true;control.applied=control.requested;control.observedValue=static_cast<float>(desired);
    control.dirty=false;control.reason=Reason::OriginalMaterialEvent;return true;
}
inline bool PresentationLeaders(const Player& player,void*& global,void*& attacking,void*& defending,uint64_t& revision);
inline bool ApplyChampion(Player& player,Effect& effect){
    auto& control=player.controls[0];if(!effect.initialized&&!InitializeEffect(player,effect))return false;
    const auto& profile=ChampionProfiles[effect.profile];void *global=nullptr,*attacking=nullptr,*defending=nullptr;uint64_t revision=0;
    const bool originalCondition=PresentationLeaders(player,global,attacking,defending,revision);
    const auto originalGlobal=global,originalAttacking=attacking,originalDefending=defending;
    auto local=player.playerState,other=OtherState(local);
    if(control.requested==Mode::FollowOriginal&&!originalCondition){control.reason=Reason::WaitingChampion;return false;}
    if(control.requested==Mode::ForceOn)global=attacking=defending=local;
    if(control.requested==Mode::ForceOff)global=attacking=defending=other;
    if(!global)global=other;if(!attacking)attacking=other;if(!defending)defending=other;
    if(!global||(profile.eventSize==16&&(!attacking||!defending))){control.reason=Reason::WaitingOtherPlayer;return false;}
    const bool changed=control.dirty||!effect.dispatched||effect.lastMode!=control.requested||effect.lastGlobal!=global
        ||effect.lastAttacking!=attacking||effect.lastDefending!=defending||effect.originalGlobal!=originalGlobal
        ||effect.originalAttacking!=originalAttacking||effect.originalDefending!=originalDefending||effect.lastConditionRevision!=revision;
    if(changed){const auto function=local_server_cosmetics::TypePath(profile)+L"."+profile.eventName;
        struct Pair{void* attacker;void* defender;} pair{attacking,defending};void* argument=profile.eventSize==16?static_cast<void*>(&pair):static_cast<void*>(&global);
        if(!Event(effect.actor,function.c_str(),profile.eventSize,argument,Read<void*>(effect.actor,0x10))||!local_server_cosmetics::Graph(effect.actor,profile,profile.update)){
            control.reason=Reason::OriginalGraphRejected;return false;}
        effect.dispatched=true;effect.lastMode=control.requested;effect.lastGlobal=global;effect.lastAttacking=attacking;effect.lastDefending=defending;
        effect.originalGlobal=originalGlobal;effect.originalAttacking=originalAttacking;effect.originalDefending=originalDefending;
        effect.lastConditionRevision=revision;
        ++effect.dispatches;++control.applications;}
    control.available=control.verified=true;control.applied=control.requested;control.dirty=false;control.reason=Reason::OriginalMaterialEvent;return true;
}
inline bool ScanEffects(Player& player){
    if(state.host||!Context(player))return true;const auto now=GetTickCount64();if(now-player.lastScan<250)return true;player.lastScan=now;
    player.effectCount=0;player.controls[0].available=player.controls[0].verified=false;player.controls[1].available=player.controls[1].verified=false;
    size_t glowAttempted=0,glowVerified=0,killsAttempted=0,killsVerified=0;uint64_t conditionRevision=0;
    Reason glowFailure=Reason::None,killsFailure=Reason::None;
    ++player.championScans;player.championScan={};
    for(auto& effect:player.effects)if(effect.occupied&&(!Live(effect.actor,effect.identity)||Read<uint8_t>(effect.actor,0x5fc)))effect={};
    for(size_t i=0;i<std::size(ChampionProfiles);++i){auto& scan=player.championScan[i];
        auto type=engine::Find(local_server_cosmetics::TypePath(ChampionProfiles[i]).c_str());scan.type=type;if(!type)continue;
        struct Query{void* world;void* type;void** items=nullptr;int32_t count=0,capacity=0;} query{state.world,type};
        scan.queried=true;
        if(!Event(engine::DefaultObject(engine::Class(0x43a9670)),L"/Script/Engine.GameplayStatics.GetAllActorsOfClass",32,&query))return false;
        const bool okay=query.count>=0&&query.count<=512&&query.capacity>=query.count
            &&(!query.count||original_players::Readable(query.items,static_cast<size_t>(query.count)*8));
        scan.actors=query.count;scan.queryValid=okay;
        if(okay)for(int32_t n=0;n<query.count;++n){auto actor=query.items[n];void* equippable=nullptr;
            if(Read<void*>(actor,0x10)!=type){++scan.differentClass;continue;}++scan.exact;
            if(Read<uint8_t>(actor,0x5fc)){++scan.ended;continue;}
            if(!EffectWeapon(player,actor,i,equippable)){++scan.missingEquippable;continue;}
            if(equippable!=player.weapon){++scan.otherWeapon;continue;}++scan.owned;
            ++glowAttempted;if(i==1&&player.controls[1].supported)++killsAttempted;++player.effectCount;
            auto effect=Track(player,actor,i);if(!effect){glowFailure=Reason::Capacity;if(i==1)killsFailure=Reason::Capacity;continue;}
            if(!effect->initialized&&!InitializeEffect(player,*effect)){glowFailure=player.controls[0].reason;
                if(i==1)killsFailure=Reason::WaitingOriginalMaterials;continue;}
            if(i==1&&player.controls[1].supported){if(ApplyKillCount(player,*effect))++killsVerified;
                else killsFailure=player.controls[1].reason;}
            if(ApplyChampion(player,*effect)){++glowVerified;conditionRevision=effect->lastConditionRevision;}
            else glowFailure=player.controls[0].reason;
        }
        if(query.items)engine::function<void(*)(void*)>(0x1915720)(query.items);if(!okay)return false;
    }
    player.controls[0].available=player.controls[0].verified=glowAttempted&&glowAttempted==glowVerified;
    player.controls[1].available=player.controls[1].verified=killsAttempted&&killsAttempted==killsVerified;
    if(player.controls[0].verified)player.lastAppliedConditionRevision=conditionRevision;
    else if(!glowAttempted)player.controls[0].reason=Reason::WaitingOriginalEffect;
    else player.controls[0].reason=glowFailure==Reason::None?Reason::WaitingOriginalMaterials:glowFailure;
    if(!killsAttempted&&player.controls[1].supported)player.controls[1].reason=Reason::WaitingOriginalEffect;
    else if(killsAttempted&&!player.controls[1].verified)player.controls[1].reason=killsFailure==Reason::None?Reason::WaitingOriginalMaterials:killsFailure;
    return true;
}
inline void WriteChampionOwnershipProbe(FILE* file,const ChampionOwnershipProbe& d){
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"stage\":\"%s\",\"checks\":%u,\"actor\":\"%p\",\"actor_weak\":[%d,%d],\"expected_weapon\":\"%p\",\"actor_weak_live\":%s,\"weapon_weak_live\":%s,\"owner_property\":\"%p\",\"owner_field_class\":\"%p\",\"owner_field_known\":%s,\"owner_array_dim\":%d,\"owner_element_size\":%d,\"owner_offset\":%d,\"actual_class_bytes\":%d,\"owner_cast_flags\":\"0x%llx\",\"owner_property_flags\":\"0x%llx\",\"owner_offset_matches\":%s,\"owner_element_matches\":%s,\"owner_cast_matches\":%s,\"owner_in_bounds\":%s,\"actual_get_world\":\"%p\",\"actual_get_world_rva\":\"0x%llx\",\"get_world_profile_known\":%s,\"world_queried\":%s,\"observed_world\":\"%p\",\"expected_world\":\"%p\",\"world_matches\":%s,\"observed_owner\":\"%p\",\"owner_matches_current_weapon\":%s,\"rejected\":{\"actor_weak\":%u,\"weapon_weak\":%u,\"field_missing\":%u,\"offset\":%u,\"get_world\":%u,\"world\":%u,\"element_size\":%u,\"cast_flags\":%u,\"bounds\":%u}}",
        d.stage,d.checks,d.actor,d.actorIdentity.index,d.actorIdentity.serial,d.expectedWeapon,b(d.actorWeak),b(d.weaponWeak),d.property,d.fieldClass,b(d.fieldKnown),
        d.arrayDim,d.elementSize,d.offset,d.classBytes,static_cast<unsigned long long>(d.castFlags),static_cast<unsigned long long>(d.propertyFlags),
        b(d.offsetMatches),b(d.elementMatches),b(d.castMatches),b(d.bounds),d.getWorld,static_cast<unsigned long long>(d.getWorldRva),b(d.getWorldKnown),b(d.worldQueried),
        d.observedWorld,d.expectedWorld,b(d.worldMatches),d.owner,b(d.ownerMatches),d.actorWeakRejected,d.weaponWeakRejected,d.fieldRejected,d.offsetRejected,
        d.getWorldRejected,d.worldRejected,d.elementRejected,d.castRejected,d.boundsRejected);
}
inline void WriteChampionScanDiagnostics(FILE* file,const Player& player){
    std::fprintf(file,"{\"scans\":%llu,\"minimum_interval_ms\":250,\"host_skips_presentation_scan\":%s,\"event_replays\":0,\"effect_spawns\":0,\"gameplay_writes\":0,\"scope\":\"cached_original_scan_filters_only\",\"profiles\":[",
        static_cast<unsigned long long>(player.championScans),state.host?"true":"false");
    for(size_t i=0;i<player.championScan.size();++i){const auto& scan=player.championScan[i];
        const char* stage=!player.championScans?"not_sampled":!scan.type?"type_not_loaded":!scan.queried?"query_not_called":
            !scan.queryValid?"query_invalid":!scan.actors?"query_empty":scan.owned?"owned_original_effect_found":"original_instances_rejected_by_existing_filters";
        std::fprintf(file,"%s{\"profile\":%zu,\"stage\":\"%s\",\"type\":\"%p\",\"type_found\":%s,\"queried\":%s,\"query_valid\":%s,\"actors\":%d,\"exact_class\":%u,\"different_class\":%u,\"raw_5fc_nonzero\":%u,\"ownership_source\":\"%s\",\"equippable_source_unavailable\":%u,\"different_equippable\":%u,\"owned\":%u,\"ownership_gate\":",
            i?",":"",i,stage,scan.type,scan.type?"true":"false",scan.queried?"true":"false",scan.queryValid?"true":"false",
            scan.actors,scan.exact,scan.differentClass,scan.ended,i==0?"original_actor_owner":"authored_equippable_property",scan.missingEquippable,scan.otherWeapon,scan.owned);
        WriteChampionOwnershipProbe(file,scan.ownership);std::fputc('}',file);
    }std::fputs("]}",file);
}
enum WireFlags:uint32_t{Supported=1,Available=2,Verified=4,Numeric=8,NextEvent=16};
inline constexpr uint32_t KnownWireFlags=31;
#pragma pack(push,4)
struct WireScope{Scope ids{};uint32_t index=0,epoch=0;int32_t nativeRespawn=0;uint32_t reserved=0;};
struct WireControl{
    char key[80]{},label[160]{};
    uint32_t kind=0,flags=0,requested=0,applied=0;
    float value=0,minimum=0,maximum=0,observedValue=0;
    uint32_t reason=0;uint64_t baseHash=0,skinHash=0;
};
struct Wire{
    uint64_t magic=WireMagic;uint32_t version=WireVersion,bytes=0;
    uint32_t hostPid=0,clientPid=0,port=0,generation=0;
    uint64_t pairDigest=0,rosterDigest=0,sampledAt=0,sequence=0,revision=0,checksum=0;
    WireScope scope{};uint32_t count=0,kind=0,flags=0,reserved=0;
    int32_t leaderSlots[3]{-1,-1,-1};uint32_t leaderEpochs[3]{};int32_t leaderPlayerIds[3]{-1,-1,-1};
    uint64_t conditionRevision=0,reservedTail=0;
    WireControl controls[MaxControls]{};
};
#pragma pack(pop)
static_assert(sizeof(Scope)==64&&sizeof(WireScope)==80&&sizeof(WireControl)==292);
static_assert(sizeof(Wire)==28844&&offsetof(Wire,scope)==80&&offsetof(Wire,leaderPlayerIds)==200&&offsetof(Wire,controls)==228);
static_assert(sizeof(training_control::Effect)==88&&offsetof(training_control::Effect,mode)==80);
inline Wire peerPacket{},authorityPacket{};
inline bool peerPacketKnown=false,authorityPacketKnown=false;
inline uint64_t Checksum(const Wire& packet){auto copy=packet;copy.checksum=0;return Hash(&copy,sizeof(copy));}
inline bool AbsoluteLocal(const std::wstring& input,std::wstring& output){
    return local_server_owned_paths::CanonicalAbsolute(input,output);
}
inline uint64_t DirectoryDigest(std::wstring value){for(auto& character:value)if(character>=L'A'&&character<=L'Z')character=static_cast<wchar_t>(character-L'A'+L'a');
    return Hash(value.data(),value.size()*sizeof(wchar_t));}
inline bool SameFile(const std::wstring& left,const std::wstring& right){std::wstring a,b;return AbsoluteLocal(left,a)&&AbsoluteLocal(right,b)&&!_wcsicmp(a.c_str(),b.c_str());}
inline bool MetadataReadFailure(const char* operation,const std::wstring& path,DWORD error,const char* contract="none"){
    metadataIO.operation=metadataIO.lastReadOperation=operation;metadataIO.path=metadataIO.lastReadPath=path;
    metadataIO.win32Error=metadataIO.lastReadError=error;metadataIO.contractError=contract;
    metadataIO.lastReadMissing=error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND;
    if(metadataIO.lastReadMissing)++metadataIO.missingReads;else ++metadataIO.readFailures;
    if(error&&!metadataIO.lastReadMissing)state.lastWin32Error=error;return false;
}
inline bool ReadMetadataFileExact(const std::wstring& path,void* output,DWORD expected){
    timing::StageScope measuredReadMetadataFile(timing::data,timing::Section::ReadMetadataFile);
    metadataIO.expectedBytes=expected;metadataIO.observedBytes=-1;metadataIO.lastReadMissing=false;
    // The target is immutable after atomic publication. An old open handle
    // remains a complete previous packet while the peer replaces its name.
    const auto handle=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(handle==INVALID_HANDLE_VALUE){const auto error=GetLastError();return MetadataReadFailure("open_read",path,error);}
    LARGE_INTEGER size{};
    if(!GetFileSizeEx(handle,&size)){const auto error=GetLastError();CloseHandle(handle);return MetadataReadFailure("get_read_size",path,error);}
    metadataIO.observedBytes=size.QuadPart;
    if(size.QuadPart!=static_cast<LONGLONG>(expected)){CloseHandle(handle);return MetadataReadFailure("check_read_size",path,ERROR_SUCCESS,"unexpected_metadata_file_size");}
    DWORD received=0;
    if(!ReadFile(handle,output,expected,&received,nullptr)){const auto error=GetLastError();CloseHandle(handle);return MetadataReadFailure("read_packet",path,error);}
    if(received!=expected){CloseHandle(handle);metadataIO.observedBytes=received;return MetadataReadFailure("read_packet_size",path,ERROR_SUCCESS,"short_metadata_read");}
    if(!CloseHandle(handle)){const auto error=GetLastError();return MetadataReadFailure("close_read",path,error);}
    metadataIO.lastReadError=ERROR_SUCCESS;metadataIO.contractError="none";return true;
}
inline bool RosterBinding(local_server_names::Ack& ack){
    timing::StageScope measuredRosterBinding(timing::data,timing::Section::RosterBinding);
    const auto& names=local_server_names::GetState();const auto& roster=local_server_names::roster;
    if(!names.initialized||names.host!=state.host||names.error||!names.ready||!roster.generation||!roster.count
       ||roster.count>MaxPlayers||names.appliedGeneration!=roster.generation||names.digest!=local_server_names::Digest(roster)
       ||!SameFile(local_server_names::path,pairDirectory+L"\\native-roster.bin")
       ||!SameFile(local_server_names::ackPath,pairDirectory+L"\\native-roster-ack.bin")
       ||!ReadMetadataFileExact(local_server_names::ackPath,&ack,static_cast<DWORD>(sizeof(ack))))return false;
    if(ack.magic!=local_server_names::Magic||ack.digest!=names.digest||ack.generation!=roster.generation||ack.count!=roster.count
       ||ack.publisher!=roster.publisher||!ack.publisher||!ack.recipient||ack.publisher==ack.recipient
       ||(state.host?ack.publisher:ack.recipient)!=state.processId)return false;
    state.hostPid=ack.publisher;state.clientPid=ack.recipient;state.rosterDigest=ack.digest;state.rosterGeneration=ack.generation;
    return true;
}
inline bool BindRosterPlayer(Player& player){
    wchar_t subject[256]{};if(!local_server_services::ReadPlayerSubjectKey(player.playerState,subject,256)||!subject[0])return false;
    const auto& roster=local_server_names::roster;
    for(size_t i=0;i<roster.count;++i){const auto& entry=roster.entries[i];
        if(entry.slot==player.index&&!std::wcscmp(entry.subject,subject)){
            if(player.epoch&&player.epoch!=entry.epoch)return false;player.epoch=entry.epoch;return player.epoch!=0;}}
    return false;
}
inline bool WorldBinding(void* connection){
    auto driver=Read<void*>(state.world,0x40);if(!driver||Read<void*>(driver,0x178)!=state.world)return false;
    auto server=Read<void*>(driver,0xc0);
    if(!state.host){if(!server||Read<void*>(server,0x60)!=driver||(connection&&connection!=server))return false;
        for(const auto& player:players)if(player.registered&&player.human&&Read<void*>(server,0xa0)!=player.controller)return false;return true;}
    if(server||!connection||Read<void*>(connection,0x60)!=driver)return false;
    auto peers=Read<void**>(driver,0xc8);if(Read<int32_t>(driver,0xd0)!=1||!peers||Read<void*>(peers)!=connection)return false;
    auto controller=Read<void*>(connection,0xa0);
    if(!controller||Read<void*>(connection,0x38)!=controller||Read<void*>(controller,0x510)!=connection||Read<uint8_t>(controller,0x224)!=3)return false;
    for(const auto& player:players)if(player.registered&&player.human&&player.controller!=controller)return false;return true;
}
enum class MetadataWriteResult:uint8_t {Published,Deferred,Failed};
inline bool SharingConflict(DWORD error){return error==ERROR_SHARING_VIOLATION||error==ERROR_LOCK_VIOLATION
    ||error==ERROR_USER_MAPPED_FILE||error==ERROR_DELETE_PENDING||error==ERROR_BUSY||error==ERROR_RETRY;}
inline MetadataWriteResult MetadataWriteFailure(const char* operation,const std::wstring& path,DWORD error,const char* contract="none"){
    const auto now=GetTickCount64();++metadataIO.writeFailures;++metadataIO.consecutiveWriteFailures;
    if(!metadataIO.firstWriteFailureAt)metadataIO.firstWriteFailureAt=now;metadataIO.lastWriteFailureAt=now;
    metadataIO.operation=metadataIO.lastWriteOperation=operation;metadataIO.path=metadataIO.lastWritePath=path;
    metadataIO.win32Error=metadataIO.lastWriteError=error;metadataIO.contractError=contract;state.lastWin32Error=error;
    const auto elapsed=now-metadataIO.firstWriteFailureAt;
    const bool deniedGrace=error==ERROR_ACCESS_DENIED&&elapsed<MetadataAccessDeniedGraceMilliseconds;
    const bool uniqueNameCollision=error==ERROR_FILE_EXISTS||error==ERROR_ALREADY_EXISTS;
    const bool transient=SharingConflict(error)||deniedGrace||uniqueNameCollision;
    metadataIO.retryPending=transient;metadataIO.stalled=elapsed>=MetadataStallMilliseconds;metadataIO.persistentFailure=!transient;
    if(transient){++metadataIO.transientFailures;metadataIO.nextWriteAttemptAt=now+MetadataRetryMilliseconds;return MetadataWriteResult::Deferred;}
    return MetadataWriteResult::Failed;
}
inline void RemoveOwnedMetadataTemporary(const std::wstring& path){
    if(DeleteFileW(path.c_str()))metadataIO.cleanupError=ERROR_SUCCESS;
    else {const auto error=GetLastError();metadataIO.cleanupError=error==ERROR_FILE_NOT_FOUND?ERROR_SUCCESS:error;}
}
inline MetadataWriteResult WriteAtomic(const std::wstring& path,const Wire& packet){
    timing::StageScope measuredWriteAtomic(timing::data,timing::Section::WriteAtomic);
    ++metadataIO.writeAttempts;++metadataIO.temporarySequence;
    if(!metadataIO.temporarySequence)return MetadataWriteFailure("allocate_temporary_sequence",path,ERROR_SUCCESS,"metadata_temporary_sequence_exhausted");
    const auto temporary=path+L".tmp."+std::to_wstring(state.processId)+L"."+std::to_wstring(GetCurrentThreadId())+L"."+std::to_wstring(metadataIO.temporarySequence);
    metadataIO.temporaryPath=temporary;
    // A failed old rename or another diagnostic opener cannot compete with a
    // fixed CREATE_ALWAYS temporary. Only this successful CREATE_NEW is ours.
    const auto handle=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(handle==INVALID_HANDLE_VALUE){const auto error=GetLastError();return MetadataWriteFailure("create_temporary",temporary,error);}
    DWORD written=0;
    if(!WriteFile(handle,&packet,static_cast<DWORD>(sizeof(packet)),&written,nullptr)){
        const auto error=GetLastError();CloseHandle(handle);const auto result=MetadataWriteFailure("write_temporary",temporary,error);
        RemoveOwnedMetadataTemporary(temporary);return result;}
    if(written!=static_cast<DWORD>(sizeof(packet))){CloseHandle(handle);
        const auto result=MetadataWriteFailure("check_temporary_write_size",temporary,ERROR_SUCCESS,"short_metadata_write");
        RemoveOwnedMetadataTemporary(temporary);return result;}
    // This is a replaceable, process-bound session mailbox, not saved settings.
    // Close the complete cached write before the atomic rename. Forcing disk
    // durability here stalls the game thread without improving peer visibility.
    if(!CloseHandle(handle)){const auto error=GetLastError();const auto result=MetadataWriteFailure("close_temporary",temporary,error);
        RemoveOwnedMetadataTemporary(temporary);return result;}
    if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING)){
        const auto error=GetLastError();const auto result=MetadataWriteFailure("replace_target",path,error);
        RemoveOwnedMetadataTemporary(temporary);return result;}
    metadataIO.retryPending=metadataIO.stalled=metadataIO.persistentFailure=false;metadataIO.consecutiveWriteFailures=0;
    metadataIO.firstWriteFailureAt=metadataIO.lastWriteFailureAt=metadataIO.nextWriteAttemptAt=0;metadataIO.lastWriteSuccessAt=GetTickCount64();
    return MetadataWriteResult::Published;
}
inline void ToWire(const Control& source,WireControl& target){
    target={};strcpy_s(target.key,source.key);strcpy_s(target.label,source.label);target.kind=static_cast<uint32_t>(source.kind);
    target.flags=(source.supported?Supported:0U)|(source.available?Available:0U)|(source.verified?Verified:0U)
        |(source.numeric?Numeric:0U)|(source.nextOriginalEvent?NextEvent:0U);
    target.requested=static_cast<uint32_t>(source.requested);target.applied=static_cast<uint32_t>(source.applied);
    target.value=source.value;target.minimum=source.minimum;target.maximum=source.maximum;target.observedValue=source.observedValue;
    target.reason=static_cast<uint32_t>(source.reason);target.baseHash=source.baseHash;target.skinHash=source.skinHash;
}
enum class LeaderLookupFailure:uint8_t {Pending,InvalidId,DuplicateId,WeakIdentity,World};
inline bool LeaderLookupPending(const char* stage,int32_t id=-1,LeaderLookupFailure reason=LeaderLookupFailure::Pending){
    ++leaderLookup.pending;leaderLookup.stage=stage;leaderLookup.lastPlayerId=id;
    if(reason==LeaderLookupFailure::InvalidId)++leaderLookup.invalidIds;
    if(reason==LeaderLookupFailure::DuplicateId)++leaderLookup.duplicateIds;
    if(reason==LeaderLookupFailure::WeakIdentity)++leaderLookup.weakChanges;
    if(reason==LeaderLookupFailure::World)++leaderLookup.worldMismatches;
    return false;
}
inline bool VerifyLeaderIdentityProfile(){
    if(leaderLookup.profileVerified)return true;
    const auto type=engine::Class(0x42aae70);auto field=FieldType(type,L"PlayerId");
    auto getter=engine::Find(L"/Script/Engine.PlayerState.GetPlayerId");
    if(!field||Read<int32_t>(field,0x44)!=0x468||Read<int32_t>(field,0x34)!=4||(Read<uint64_t>(field,0x38)&0x20)==0
       ||!getter||Read<void*>(getter,0x28)!=type||Read<uint16_t>(getter,0xbe)!=4
       ||Read<void*>(getter,0xe0)!=reinterpret_cast<void*>(engine::image+0x49452a0))
        return LeaderLookupPending("local_cosmetics_canonical_player_id_profile_mismatch");
    leaderLookup.profileVerified=true;return true;
}
inline void* LeaderActorWorld(void* actor){
    // Original Actor.GetWorld resolves this actor's actual Level -> World.
    const auto target=Read<void*>(Read<void**>(actor),0x1b8);
    return target==reinterpret_cast<void*>(engine::image+0x3ed83b0)?reinterpret_cast<void*(*)(void*)>(target)(actor):nullptr;
}
inline bool NativeLeaderPlayerId(void* ps,int32_t& id){
    id=-1;
    return VerifyLeaderIdentityProfile()&&original_players::IsA(ps,0x42aae70)
        &&Event(ps,L"/Script/Engine.PlayerState.GetPlayerId",4,&id)&&id==Read<int32_t>(ps,0x468);
}
inline bool ResolveCanonicalLeader(int32_t id,void* expected,void*& result){
    result=nullptr;leaderLookup.lastPlayerId=id;
    if(id<0)return LeaderLookupPending("local_cosmetics_canonical_leader_player_id_invalid",id,LeaderLookupFailure::InvalidId);
    if(!Live(state.world,leaderLookup.worldId))return LeaderLookupPending("local_cosmetics_canonical_leader_world_weak_identity_changed",id,LeaderLookupFailure::World);
    auto gameState=Read<void*>(state.world,0x178);
    if(!original_players::IsA(gameState,original_players::ShooterGameStateClass)||LeaderActorWorld(gameState)!=state.world)
        return LeaderLookupPending("local_cosmetics_canonical_leader_game_state_world_pending",id,LeaderLookupFailure::World);
    if(!leaderLookup.gameState){
        if(!Capture(gameState,leaderLookup.gameStateId))return LeaderLookupPending("local_cosmetics_canonical_leader_game_state_weak_pending",id,LeaderLookupFailure::WeakIdentity);
        leaderLookup.gameState=gameState;
    }
    if(gameState!=leaderLookup.gameState||!Live(gameState,leaderLookup.gameStateId)
       ||(leader.ready&&gameState!=leader.gameState))
        return LeaderLookupPending("local_cosmetics_canonical_leader_game_state_identity_changed",id,LeaderLookupFailure::WeakIdentity);
    auto items=Read<void**>(gameState,0x480);const auto count=Read<int32_t>(gameState,0x488),capacity=Read<int32_t>(gameState,0x48c);
    if(count<0||count>static_cast<int32_t>(MaxPlayers)||capacity<count||capacity>4096
       ||(count&&(!items||!original_players::Readable(items,static_cast<size_t>(count)*8))))
        return LeaderLookupPending("local_cosmetics_canonical_leader_player_array_pending",id);
    int32_t matches=0;void* selected=nullptr;Weak identity{};
    for(int32_t i=0;i<count;++i){auto ps=Read<void*>(items,static_cast<size_t>(i)*8);
        if(!ps||(Read<uint32_t>(ps,8)&0x60000000U)||!original_players::IsA(ps,0x42aae70))continue;
        Weak current{};int32_t actual=-1;
        if(!Capture(ps,current)||!NativeLeaderPlayerId(ps,actual)||!Live(ps,current))
            return LeaderLookupPending("local_cosmetics_canonical_leader_player_id_readback_pending",id,LeaderLookupFailure::WeakIdentity);
        if(actual!=id)continue;
        if(LeaderActorWorld(ps)!=state.world)return LeaderLookupPending("local_cosmetics_canonical_leader_player_state_foreign_world",id,LeaderLookupFailure::World);
        ++matches;selected=ps;identity=current;
    }
    if(matches>1)return LeaderLookupPending("local_cosmetics_canonical_leader_duplicate_native_player_id",id,LeaderLookupFailure::DuplicateId);
    if(matches!=1||!original_players::IsA(selected,original_players::ShooterStateClass))
        return LeaderLookupPending("local_cosmetics_canonical_leader_player_state_not_replicated",id);
    if(expected&&selected!=expected)return LeaderLookupPending("local_cosmetics_canonical_leader_source_not_in_actual_player_array",id,LeaderLookupFailure::WeakIdentity);
    if(!Live(state.world,leaderLookup.worldId)||!Live(gameState,leaderLookup.gameStateId)||Read<void*>(state.world,0x178)!=gameState
       ||Read<void**>(gameState,0x480)!=items||Read<int32_t>(gameState,0x488)!=count||!Live(selected,identity))
        return LeaderLookupPending("local_cosmetics_canonical_leader_roster_changed_during_read",id,LeaderLookupFailure::WeakIdentity);
    CanonicalLeaderIdentity* empty=nullptr;
    for(auto& cached:leaderLookup.identities){
        if(!cached.playerState){if(!empty)empty=&cached;continue;}
        if(cached.playerId==id){
            if(cached.playerState!=selected||!Live(selected,cached.identity)||cached.identity.index!=identity.index||cached.identity.serial!=identity.serial)
                return LeaderLookupPending("local_cosmetics_canonical_leader_player_state_identity_changed",id,LeaderLookupFailure::WeakIdentity);
            result=selected;++leaderLookup.resolutions;return true;
        }
        if(cached.playerState==selected)return LeaderLookupPending("local_cosmetics_canonical_leader_native_player_id_changed",id,LeaderLookupFailure::InvalidId);
    }
    if(!empty)return LeaderLookupPending("local_cosmetics_canonical_leader_identity_cache_full",id);
    empty->playerId=id;empty->playerState=selected;empty->identity=identity;
    result=selected;++leaderLookup.resolutions;return true;
}
inline bool EncodeLeaderReference(void* ps,int32_t& id,int32_t& slot,uint32_t& epoch){
    id=slot=-1;epoch=0;if(!ps)return true;
    Weak identity{};if(!Capture(ps,identity)||!NativeLeaderPlayerId(ps,id))
        return LeaderLookupPending("local_cosmetics_canonical_leader_source_id_readback_pending",id,LeaderLookupFailure::WeakIdentity);
    void* actual=nullptr;if(!ResolveCanonicalLeader(id,ps,actual))return false;
    wchar_t subject[256]{};if(!local_server_services::ReadPlayerSubjectKey(ps,subject,256))
        return LeaderLookupPending("local_cosmetics_canonical_leader_subject_readback_pending",id);
    // Empty native subjects have no NameRoster entry. Their real PlayerId is
    // sufficient; it is never converted into a synthetic account identity.
    if(subject[0]){
        const auto& roster=local_server_names::roster;if(roster.count>MaxPlayers)return LeaderLookupPending("local_cosmetics_canonical_leader_name_roster_bounds",id);
        uint32_t matches=0;
        for(size_t i=0;i<roster.count;++i){const auto& entry=roster.entries[i];
            if(std::wcscmp(subject,entry.subject))continue;
            if(entry.slot>=MaxPlayers||!entry.epoch)return LeaderLookupPending("local_cosmetics_canonical_leader_name_roster_identity_invalid",id);
            ++matches;slot=static_cast<int32_t>(entry.slot);epoch=entry.epoch;
        }
        if(matches>1)return LeaderLookupPending("local_cosmetics_canonical_leader_name_roster_identity_ambiguous",id);
    }
    return Live(ps,identity)||LeaderLookupPending("local_cosmetics_canonical_leader_source_weak_identity_changed",id,LeaderLookupFailure::WeakIdentity);
}
inline void* ResolveRosterState(int32_t slot,uint32_t epoch,int32_t id){
    void* ps=nullptr;if(!ResolveCanonicalLeader(id,nullptr,ps))return nullptr;
    if(slot<0){if(epoch)return nullptr;return ps;}
    if(slot>=static_cast<int32_t>(MaxPlayers)||!epoch)return nullptr;
    const auto& roster=local_server_names::roster;if(roster.count>MaxPlayers)return nullptr;
    const local_server_names::Entry* entry=nullptr;uint32_t matches=0;
    for(size_t i=0;i<roster.count;++i)if(roster.entries[i].slot==static_cast<uint32_t>(slot)&&roster.entries[i].epoch==epoch){entry=&roster.entries[i];++matches;}
    wchar_t subject[256]{};
    if(matches!=1||!entry||!entry->subject[0]||!local_server_services::ReadPlayerSubjectKey(ps,subject,256)||!subject[0]||std::wcscmp(subject,entry->subject)){
        LeaderLookupPending("local_cosmetics_canonical_leader_name_roster_crosscheck_pending",id);return nullptr;}
    return ps;
}
inline bool PresentationLeaders(const Player& player,void*& global,void*& attacking,void*& defending,uint64_t& revision){
    global=leader.global;attacking=leader.attacking;defending=leader.defending;revision=leader.conditionRevision;
    if(state.host)return leader.ready;
    const auto now=GetTickCount64();
    if(authorityPacketKnown&&player.settingsReceived&&player.peerScope
       &&player.peerAt<=now&&now-player.peerAt<=MaximumAgeMilliseconds&&Same(authorityPacket.scope.ids,player.scope)
       &&authorityPacket.scope.index==player.index&&authorityPacket.scope.epoch==player.epoch){
        leaderLookup.resolvedMetadataReady=false;leaderLookup.resolvedPlayerIds={{-1,-1,-1}};
        if(authorityPacket.flags!=1)return LeaderLookupPending("local_cosmetics_canonical_leader_authority_reference_pending");
        void* values[3]{};
        for(size_t i=0;i<3;++i){
            if(authorityPacket.leaderPlayerIds[i]>=0){values[i]=ResolveRosterState(authorityPacket.leaderSlots[i],authorityPacket.leaderEpochs[i],authorityPacket.leaderPlayerIds[i]);
                if(!values[i])return false;leaderLookup.resolvedPlayerIds[i]=authorityPacket.leaderPlayerIds[i];}
        }
        // These are real, locally mapped original PlayerStates selected by the
        // original host rule. Only the FXC presentation event receives them;
        // no local GameState leader, score or PlayerState field is assigned.
        global=values[0];attacking=values[1];defending=values[2];revision=authorityPacket.conditionRevision;
        leaderLookup.resolvedMetadataReady=true;leaderLookup.stage="local_cosmetics_canonical_leader_presentation_arguments_resolved";return true;
    }
    return leader.ready;
}
inline bool PacketScope(const Wire& packet,const Player& player){
    return player.sourceReady&&Same(packet.scope.ids,player.scope)&&packet.scope.index==player.index&&packet.scope.epoch==player.epoch
        &&packet.scope.nativeRespawn==Read<int32_t>(player.pawn,0x143c)&&!packet.scope.reserved;
}
inline bool ValidatePacket(const Wire& packet,const local_server_names::Ack& ack,uint32_t kind){
    if(packet.magic!=WireMagic||packet.version!=WireVersion||packet.bytes!=sizeof(Wire)||packet.kind!=kind||packet.count>MaxControls
       ||packet.hostPid!=ack.publisher||packet.clientPid!=ack.recipient||packet.port!=state.port||packet.generation!=ack.generation
       ||packet.pairDigest!=state.pairDigest||packet.rosterDigest!=ack.digest||packet.checksum!=Checksum(packet)
       ||!packet.sequence||packet.reserved||packet.reservedTail||packet.flags>1||(packet.flags&&!packet.conditionRevision)
       ||!Valid(packet.scope.ids)||packet.scope.index>=MaxPlayers||!packet.scope.epoch||packet.scope.reserved)return false;
    const auto now=GetTickCount64();if(packet.sampledAt>now||now-packet.sampledAt>MaximumAgeMilliseconds)return false;
    for(size_t i=0;i<packet.count;++i){const auto& control=packet.controls[i];
        if(!ValidKey(control.key)||!std::memchr(control.label,0,sizeof(control.label))||control.kind>2||(control.flags&~KnownWireFlags)
           ||control.requested>2||control.applied>2||control.reason>static_cast<uint32_t>(Reason::UnsupportedKey)
           ||!std::isfinite(control.value)||!std::isfinite(control.minimum)||!std::isfinite(control.maximum)||!std::isfinite(control.observedValue)
           ||((control.flags&Verified)&&((control.flags&(Supported|Available))!=(Supported|Available))))return false;
        if(!std::strcmp(control.key,"champion_glow")){
            if(control.kind!=static_cast<uint32_t>(Kind::ChampionGlow)||(control.flags&Numeric))return false;
        }else if(!std::strcmp(control.key,"skin_kill_count")){
            if(control.kind!=static_cast<uint32_t>(Kind::ChampionKillCount)||!(control.flags&Numeric)||control.minimum!=0||control.maximum!=25
               ||control.value<0||control.value>25||std::floor(control.value)!=control.value)return false;
        }else{
            char expected[80]{};sprintf_s(expected,"fxc_%016llx",static_cast<unsigned long long>(control.baseHash));
            if(control.kind!=static_cast<uint32_t>(Kind::OriginalEffectOverride)||(control.flags&Numeric)||!control.baseHash
               ||std::strcmp(expected,control.key))return false;
        }
        for(size_t j=0;j<i;++j)if(!std::strcmp(packet.controls[j].key,control.key))return false;
    }
    for(size_t i=0;i<3;++i){
        if(packet.leaderPlayerIds[i]<-1||packet.leaderSlots[i]<-1||packet.leaderSlots[i]>=static_cast<int32_t>(MaxPlayers)
           ||((packet.leaderSlots[i]<0)!=(packet.leaderEpochs[i]==0))
           ||(packet.leaderPlayerIds[i]<0&&(packet.leaderSlots[i]>=0||packet.leaderEpochs[i])))return false;
        // Client receipts and pending authority references contain no leader
        // identities. A null champion is distinct from an unresolved real PS.
        if((packet.kind!=1||packet.flags!=1)&&(packet.leaderPlayerIds[i]!=-1||packet.leaderSlots[i]!=-1||packet.leaderEpochs[i]))return false;
    }
    return true;
}
inline Control* FindControl(Player& player,const char* key){for(size_t i=0;i<player.controlCount;++i)if(!std::strcmp(player.controls[i].key,key))return &player.controls[i];return nullptr;}
inline const WireControl* FindControl(const Wire& packet,const char* key){for(size_t i=0;i<packet.count;++i)if(!std::strcmp(packet.controls[i].key,key))return &packet.controls[i];return nullptr;}
inline void MergeCapabilities(Player& player,const Wire& packet){
    player.peerScope=true;player.peerAt=packet.sampledAt;player.hasPeerReceipt=true;
    for(size_t i=0;i<packet.count;++i){const auto& remote=packet.controls[i];auto control=FindControl(player,remote.key);
        if(!control){if(player.controlCount==MaxControls||remote.kind!=static_cast<uint32_t>(Kind::OriginalEffectOverride))continue;
            control=&player.controls[player.controlCount++];strcpy_s(control->key,remote.key);strcpy_s(control->label,remote.label);
            control->kind=Kind::OriginalEffectOverride;control->baseHash=remote.baseHash;control->skinHash=remote.skinHash;
            control->nextOriginalEvent=true;control->reason=Reason::WaitingPeer;RestoreRequested(player,*control);}
        if(static_cast<uint32_t>(control->kind)!=remote.kind)continue;
        if(control->kind==Kind::OriginalEffectOverride){if(control->baseHash&&control->baseHash!=remote.baseHash)continue;
            control->supported=control->supported||(remote.flags&Supported)!=0;}
        // Only acknowledged original client cosmetic readback is reflected.
        // Class paths/pointers from the client are never loaded or executed.
        const auto preferred=Stored(player.index,player.scope,control->key,false);
        const auto mode=preferred?preferred->mode:Mode::FollowOriginal;const float value=preferred?preferred->value:0;
        bool conditionCurrent=true;
        control->peerVerified=(remote.flags&Verified)&&remote.applied==static_cast<uint32_t>(mode)
            &&remote.requested==static_cast<uint32_t>(mode)&&remote.value==value;
        if(control->kind==Kind::ChampionGlow&&leader.conditionRevision)conditionCurrent=packet.conditionRevision==leader.conditionRevision;
        if(control->kind==Kind::ChampionKillCount){int32_t desired=0;
            if(mode==Mode::ForceOn)desired=static_cast<int32_t>(value);
            else if(mode==Mode::FollowOriginal){void* info=nullptr;
                conditionCurrent=Event(player.pawn,L"/Script/ShooterGame.ShooterCharacter.GetOwnerExclusivePlayerInfo",8,&info)&&info
                    &&Event(info,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.GetKills",4,&desired)&&desired>=0;}
            conditionCurrent=conditionCurrent&&remote.observedValue==static_cast<float>(desired);
        }
        if(control->kind==Kind::OriginalEffectOverride&&control->skinHash)conditionCurrent=control->skinHash==remote.skinHash;
        control->peerVerified=control->peerVerified&&conditionCurrent;
        control->available=(remote.flags&Available)!=0;control->verified=control->peerVerified;control->applied=static_cast<Mode>(remote.applied);
        control->observedValue=remote.observedValue;
        control->reason=control->peerVerified?Reason::PeerVerified:
            ((!conditionCurrent||remote.requested!=static_cast<uint32_t>(mode)||remote.applied!=static_cast<uint32_t>(mode))?Reason::PeerPending:static_cast<Reason>(remote.reason));
    }
}
inline bool ConsumePeer(const local_server_names::Ack& ack){
    timing::StageScope measuredReadPeer(timing::data,timing::Section::ReadPeer);
    Wire packet;if(!ReadMetadataFileExact(state.host?clientPath:authorityPath,&packet,static_cast<DWORD>(sizeof(packet))))return true;
    if(!ValidatePacket(packet,ack,state.host?2U:1U)){
        ++state.rejections;state.lastReject=5110;state.metadataReady=false;return true;}
    if(packet.sequence<state.lastPeerSequence||(packet.sequence==state.lastPeerSequence&&packet.checksum!=state.lastPeerChecksum)){
        ++state.rejections;state.lastReject=5111;return true;}
    state.lastPeerSequence=packet.sequence;state.lastPeerChecksum=packet.checksum;state.lastReject=0;
    if(state.host){peerPacket=packet;peerPacketKnown=true;}else {authorityPacket=packet;authorityPacketKnown=true;}
    auto& player=players[packet.scope.index];
    timing::PlayerScope measuredPeerPlayer(timing::data,player.index);
    if(!player.registered||!player.human||!BindRosterPlayer(player)||!PacketScope(packet,player))return true;
    player.peerAt=packet.sampledAt;player.peerScope=true;state.metadataReady=true;++state.receipts;
    if(state.host)MergeCapabilities(player,packet);
    else{
        player.settingsReceived=true;
        for(size_t i=0;i<player.controlCount;++i){auto& control=player.controls[i];auto entry=FindControl(packet,control.key);if(!entry)continue;
            if(entry->kind!=static_cast<uint32_t>(control.kind)||!control.supported)continue;
            if(control.kind==Kind::OriginalEffectOverride&&entry->baseHash&&entry->baseHash!=control.baseHash)continue;
            const auto mode=static_cast<Mode>(entry->requested);
            if(control.requested!=mode||control.value!=entry->value){control.requested=mode;control.value=entry->value;control.dirty=true;control.verified=false;}
        }
        // Original leader values themselves still arrive via native multicast.
        // The packet's roster-bound condition may be used only as presentation
        // arguments if the original component has not received that edge yet.
    }
    return true;
}
inline bool FillPacket(Player& player,const local_server_names::Ack& ack,Wire& packet){
    timing::PlayerScope measuredWirePlayer(timing::data,player.index);
    timing::StageScope measuredWireBuild(timing::data,timing::Section::BuildWire);
    packet={};packet.bytes=static_cast<uint32_t>(sizeof(Wire));packet.hostPid=ack.publisher;packet.clientPid=ack.recipient;packet.port=state.port;
    packet.generation=ack.generation;packet.pairDigest=state.pairDigest;packet.rosterDigest=ack.digest;packet.sampledAt=GetTickCount64();
    packet.sequence=++state.sequence;packet.revision=state.settingsRevision;packet.scope.ids=player.scope;packet.scope.index=player.index;
    packet.scope.epoch=player.epoch;packet.scope.nativeRespawn=Read<int32_t>(player.pawn,0x143c);packet.kind=state.host?1U:2U;
    packet.count=static_cast<uint32_t>(player.controlCount);
    for(size_t i=0;i<player.controlCount;++i)ToWire(player.controls[i],packet.controls[i]);
    packet.conditionRevision=state.host?leader.conditionRevision:player.lastAppliedConditionRevision;
    if(state.host){
        leaderLookup.sourceMetadataReady=false;leaderLookup.sourcePlayerIds={{-1,-1,-1}};
        if(leader.ready&&leader.conditionRevision){
            void* leaders[]{leader.global,leader.attacking,leader.defending};bool ready=true;const char* pendingStage=nullptr;
            for(size_t i=0;i<3;++i){
                const bool encoded=EncodeLeaderReference(leaders[i],packet.leaderPlayerIds[i],packet.leaderSlots[i],packet.leaderEpochs[i]);
                leaderLookup.sourcePlayerIds[i]=packet.leaderPlayerIds[i];
                if(!encoded){ready=false;if(!pendingStage)pendingStage=leaderLookup.stage;}
            }
            if(ready){packet.flags=1;leaderLookup.sourceMetadataReady=true;leaderLookup.stage="local_cosmetics_canonical_leader_authority_references_ready";}
            else{
                // Preserve the actual native leader in authority. Suppress
                // incomplete metadata rather than turning that leader null.
                for(size_t i=0;i<3;++i){packet.leaderPlayerIds[i]=-1;packet.leaderSlots[i]=-1;packet.leaderEpochs[i]=0;}
                leaderLookup.stage=pendingStage;
            }
        }else leaderLookup.stage="local_cosmetics_canonical_leader_original_rule_pending";
    }
    packet.checksum=Checksum(packet);return packet.sequence!=0;
}
inline bool Publish(const local_server_names::Ack& ack){
    const auto now=GetTickCount64();
    if(metadataIO.retryPending){if(now<metadataIO.nextWriteAttemptAt)return true;}
    else if(now-state.lastPublish<PublishMilliseconds)return true;
    state.lastPublish=now;
    for(auto& player:players)if(player.registered&&player.human&&player.sourceReady&&player.snapshotReady&&BindRosterPlayer(player)){
        Wire packet;if(!FillPacket(player,ack,packet))return false;
        const auto result=WriteAtomic(state.host?authorityPath:clientPath,packet);
        if(result==MetadataWriteResult::Failed)return false;
        if(result==MetadataWriteResult::Published)++state.publications;
        return true;
    }
    return true;
}
inline void ExpirePeer(Player& player){
    const auto now=GetTickCount64();if(!player.peerScope||!player.peerAt||now-player.peerAt<=MaximumAgeMilliseconds)return;
    player.peerScope=player.hasPeerReceipt=player.settingsReceived=false;state.metadataReady=false;
    for(size_t i=0;i<player.controlCount;++i){auto& control=player.controls[i];control.peerVerified=control.verified=false;
        if(!state.host&&control.requested!=Mode::FollowOriginal){control.requested=Mode::FollowOriginal;control.dirty=true;}
        control.reason=Reason::StaleMetadata;}
}
inline bool BroadcastNativeLeaders(const local_server_names::Ack& ack){
    timing::ContextScope measuredBroadcastContext(timing::data,timing::Context::Leader);
    timing::StageScope measuredBroadcast(timing::data,timing::Section::Leader);
    if(!state.host||!leader.ready||leader.lastBroadcastGeneration==ack.generation)return true;
    const auto one=std::wstring(training_champion_state::MethodPrefix)+L"MulticastUpdateChampion";
    const auto two=std::wstring(training_champion_state::MethodPrefix)+L"MulticastUpdateTeamChampion";
    struct Pair{void* attacking;void* defending;} team{leader.attacking,leader.defending};
    if(!Event(leader.component,one.c_str(),8,&leader.global,leader.type)||!Event(leader.component,two.c_str(),16,&team,leader.type))return false;
    leader.lastBroadcastGeneration=ack.generation;++leader.broadcasts;++leader.conditionRevision;return true;
}
inline bool ConfigureBridgeImpl(const std::wstring& output,uint32_t port){
    std::wstring canonical;if(!AbsoluteLocal(output,canonical)||!local_server_owned_paths::RoleDirectory(canonical,state.host?1U:2U)
       ||port<1024||port>65535)return Fail(5112,"local_cosmetics_pair_directory_or_port_invalid");
    const auto at=canonical.find_last_of(L'\\');if(at==std::wstring::npos||_wcsicmp(canonical.c_str()+at+1,state.host?L"host":L"client"))
        return Fail(5113,"local_cosmetics_role_directory_mismatch");
    const auto parent=canonical.substr(0,at);if(parent.size()<=3)return false;
    if(!roleDirectory.empty()&&(_wcsicmp(roleDirectory.c_str(),canonical.c_str())||state.port!=port))
        return Fail(5114,"local_cosmetics_pair_changed_requires_new_module");
    roleDirectory=canonical;pairDirectory=parent;authorityPath=parent+L"\\native-cosmetics-authority.bin";clientPath=parent+L"\\native-cosmetics-client.bin";
    state.port=port;state.pairDigest=DirectoryDigest(parent);return true;
}
inline bool InitializeImpl(bool host,void* world,const std::wstring& output,uint32_t port){
    if(!Thread()||!world)return false;
    if(state.initialized)return state.host==host&&state.world==world&&ConfigureBridgeImpl(output,port);
    state.host=host;state.world=world;state.processId=GetCurrentProcessId();state.gameThread=GetCurrentThreadId();
    (void)timing::data.Bind(state.gameThread);
    if(!VerifyProfiles()||!ConfigureBridgeImpl(output,port))return false;
    if(!VerifyLeaderIdentityProfile())return Fail(5125,"local_cosmetics_canonical_player_id_schema_mismatch");
    if(!Capture(world,leaderLookup.worldId))return Fail(5126,"local_cosmetics_canonical_world_weak_identity_unavailable");
    state.initialized=true;state.stage="local_cosmetics_waiting_original_player_and_ack_bound_client";return true;
}
inline bool RetireImpl(size_t index,void* pawn,uint32_t epoch){
    if(!Thread()||index>=MaxPlayers)return false;auto& player=players[index];if(!player.registered)return true;
    if((pawn&&pawn!=player.pawn)||(epoch&&player.epoch&&epoch!=player.epoch))return Fail(5115,"local_cosmetics_retirement_identity_or_epoch_mismatch");
    if(!RestoreOwnedMaps(player))return Fail(player,5116,"local_cosmetics_owned_native_map_restore_failed");
    player={};return true; // canonical preferences intentionally survive
}
inline bool RegisterImpl(size_t index,void* controller,void* playerState,void* pawn,uint32_t epoch,bool human){
    if(!Thread()||!state.initialized||index>=MaxPlayers)return false;if(!human)return true;
    if(!original_players::IsA(pawn,original_players::ShooterCharacterClass)||!original_players::IsA(playerState,original_players::ShooterStateClass)
       ||Read<void*>(controller,0x470)!=playerState||Read<void*>(controller,0x4b0)!=pawn||Read<void*>(pawn,0x488)!=playerState
       ||Read<void*>(pawn,0x4a0)!=controller||Read<uint8_t>(pawn,0x224)!=(state.host?3:2)||(state.host&&!epoch))
        return Fail(5117,"local_cosmetics_actual_native_human_ownership_or_epoch_invalid");
    auto& player=players[index];if(player.registered&&player.pawn==pawn&&(!epoch||player.epoch==epoch)&&Live(pawn,player.pawnId))return true;
    if(player.registered&&!RetireImpl(index,player.pawn,player.epoch))return false;
    player.registered=true;player.human=true;player.index=static_cast<uint32_t>(index);player.epoch=epoch;
    player.controller=controller;player.playerState=playerState;player.pawn=pawn;
    if(!Capture(controller,player.controllerId)||!Capture(playerState,player.stateId)||!Capture(pawn,player.pawnId))
        return Fail(player,5118,"local_cosmetics_native_player_weak_identity_unavailable");
    player.stage="local_cosmetics_actual_player_registered_waiting_native_weapon";return true;
}
inline Result Reply(Result result,const char* text,char* message,size_t capacity){
    state.commandResult=result;state.commandStage=text;if(message&&capacity)strncpy_s(message,capacity,text,_TRUNCATE);return result;
}
inline Result DispatchEffectImpl(size_t index,const training_control::Effect& command,char* message,size_t capacity){
    ++state.commands;
    if(!Thread()||!state.initialized||!state.host)return Reply(Result::Unavailable,"local_cosmetics_effect_requires_authority_game_thread",message,capacity);
    if(index>=MaxPlayers||!ValidKey(command.key)||command.mode>2||!std::isfinite(command.value))
        return Reply(Result::Invalid,"local_cosmetics_effect_payload_invalid",message,capacity);
    auto& player=players[index];if(!player.registered||!player.human||player.error||!RefreshSource(player)||!player.sourceReady||!player.snapshotReady)
        return Reply(Result::Unavailable,"local_cosmetics_actual_original_skin_not_ready",message,capacity);
    auto control=FindControl(player,command.key);
    if(!control)return Reply(Result::Unavailable,"local_cosmetics_effect_key_not_in_current_native_capabilities",message,capacity);
    if(!control->supported)return Reply(Result::Unavailable,"local_cosmetics_effect_not_supported_by_canonical_original_level",message,capacity);
    if(control->numeric&&(command.value<control->minimum||command.value>control->maximum||std::floor(command.value)!=command.value))
        return Reply(Result::Invalid,"local_cosmetics_original_kill_progression_requires_integer_0_25",message,capacity);
    auto preference=Stored(index,player.scope,command.key,true);
    if(!preference)return Reply(Result::Unavailable,"local_cosmetics_canonical_preference_capacity",message,capacity);
    preference->mode=static_cast<Mode>(command.mode);preference->value=command.value;preference->revision=++state.settingsRevision;
    control->requested=preference->mode;control->value=command.value;control->dirty=true;control->verified=control->peerVerified=false;
    if(control->kind==Kind::OriginalEffectOverride&&control->baseClass&&control->available){
        control->serverMapVerified=ApplyMap(player,*control);control->verified=false;
    }
    state.lastPublish=0;return Reply(Result::Pending,"local_cosmetics_override_saved_waiting_actual_client_readback",message,capacity);
}
inline bool PollImpl(void* connection){
    if(!Thread()||!state.initialized||state.error)return false;++state.polls;
    timing::StageScope measuredPoll(timing::data,timing::Section::Poll);
    state.worldBound=WorldBinding(connection);if(!state.worldBound)return Wait("local_cosmetics_waiting_actual_native_network_world");
    if(!PollLeader())return false;
    for(auto& player:players)if(player.registered&&player.human){state.activeIndex=player.index;
        if(!RefreshSource(player))return Fail(player,5119,"local_cosmetics_native_source_binding_or_schema_failed");}
    local_server_names::Ack ack;state.peerBound=RosterBinding(ack);
    if(state.peerBound){if(!BroadcastNativeLeaders(ack)||!ConsumePeer(ack))return false;}
    for(auto& player:players)if(player.registered&&player.human&&player.sourceReady&&player.snapshotReady){
        timing::PlayerScope measuredPlayerMap(timing::data,player.index);
        state.activeIndex=player.index;ExpirePeer(player);const auto now=GetTickCount64();const bool mapPoll=now-player.lastMapPoll>=250;
        for(size_t i=2;i<player.controlCount;++i){auto& control=player.controls[i];
            if(control.baseClass&&(state.host||control.available)&&(control.dirty||mapPoll)){
                ApplyMap(player,control);if(state.host){control.serverMapVerified=control.verified;control.verified=control.peerVerified;
                    control.reason=control.peerVerified?Reason::PeerVerified:Reason::WaitingPeer;}
            }}
        if(mapPoll)player.lastMapPoll=now;
        if(!state.host&&player.controls[0].supported&&!ScanEffects(player))return Fail(player,5120,"local_cosmetics_original_effect_query_failed");
    }
    if(state.peerBound&&!Publish(ack))return Fail(5121,"local_cosmetics_atomic_presentation_metadata_publish_failed");
    state.stage=metadataIO.retryPending?"local_cosmetics_presentation_metadata_retry_pending":
        state.peerBound?"local_cosmetics_native_rules_and_paired_presentation_polled":"local_cosmetics_waiting_original_roster_ack";return true;
}
inline LONG CaptureException(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0005100U;
    auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;state.restartRequired=true;
    state.stage="local_cosmetics_exception_requires_new_owned_pair";if(state.activeIndex<MaxPlayers)players[state.activeIndex].restartRequired=true;
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(bool host,void* world,const std::wstring& output,uint32_t port){
    __try{return InitializeImpl(host,world,output,port);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool Register(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch,bool human=true){
    __try{return RegisterImpl(index,pc,ps,pawn,epoch,human);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool Retire(size_t index,void* pawn=nullptr,uint32_t epoch=0){
    __try{return RetireImpl(index,pawn,epoch);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool Poll(void* connection=nullptr){__try{return PollImpl(connection);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline Result DispatchEffect(size_t index,const training_control::Effect& effect,char* message,size_t capacity){
    __try{return DispatchEffectImpl(index,effect,message,capacity);}__except(CaptureException(GetExceptionInformation())){
        return Reply(Result::Failed,"local_cosmetics_effect_exception_requires_new_pair",message,capacity);}}
inline Result DispatchEffect(const uint8_t* bytes,uint32_t count,char* message,size_t capacity){
    if(!bytes||count!=sizeof(training_control::Effect))return Reply(Result::Invalid,"local_cosmetics_effect_payload_size_invalid",message,capacity);
    training_control::Effect effect{};std::memcpy(&effect,bytes,sizeof(effect));return DispatchEffect(0,effect,message,capacity);
}
inline Result Apply(size_t index,const training_control::Effect& effect){return DispatchEffect(index,effect,nullptr,0);}
inline void WriteMetadataIODiagnostics(FILE* file){
    const auto& io=metadataIO;const auto boolean=[](bool value){return value?"true":"false";};
    std::fputs("{\"operation\":",file);original_players::Text(file,io.operation);
    std::fputs(",\"path\":",file);original_players::Text(file,original_players::Utf8(io.path.c_str()).c_str());
    std::fputs(",\"last_write_operation\":",file);original_players::Text(file,io.lastWriteOperation);
    std::fputs(",\"last_write_path\":",file);original_players::Text(file,original_players::Utf8(io.lastWritePath.c_str()).c_str());
    std::fputs(",\"last_read_operation\":",file);original_players::Text(file,io.lastReadOperation);
    std::fputs(",\"last_read_path\":",file);original_players::Text(file,original_players::Utf8(io.lastReadPath.c_str()).c_str());
    std::fputs(",\"temporary_path\":",file);original_players::Text(file,original_players::Utf8(io.temporaryPath.c_str()).c_str());
    std::fputs(",\"contract_error\":",file);original_players::Text(file,io.contractError);
    std::fprintf(file,",\"win32_error\":%u,\"last_write_error\":%u,\"last_read_error\":%u,\"cleanup_error\":%u,\"expected_bytes\":%u,\"observed_bytes\":%lld,\"write_attempts\":%llu,\"write_failures\":%llu,\"transient_failures\":%llu,\"read_failures\":%llu,\"missing_reads\":%llu,\"temporary_sequence\":%llu,\"consecutive_write_failures\":%llu,\"first_write_failure_at\":%llu,\"last_write_failure_at\":%llu,\"next_write_attempt_at\":%llu,\"last_write_success_at\":%llu,\"retry_pending\":%s,\"stalled\":%s,\"persistent_failure\":%s,\"last_read_missing\":%s}",
        static_cast<unsigned>(io.win32Error),static_cast<unsigned>(io.lastWriteError),static_cast<unsigned>(io.lastReadError),static_cast<unsigned>(io.cleanupError),
        io.expectedBytes,static_cast<long long>(io.observedBytes),static_cast<unsigned long long>(io.writeAttempts),static_cast<unsigned long long>(io.writeFailures),
        static_cast<unsigned long long>(io.transientFailures),static_cast<unsigned long long>(io.readFailures),static_cast<unsigned long long>(io.missingReads),
        static_cast<unsigned long long>(io.temporarySequence),static_cast<unsigned long long>(io.consecutiveWriteFailures),static_cast<unsigned long long>(io.firstWriteFailureAt),
        static_cast<unsigned long long>(io.lastWriteFailureAt),static_cast<unsigned long long>(io.nextWriteAttemptAt),static_cast<unsigned long long>(io.lastWriteSuccessAt),
        boolean(io.retryPending),boolean(io.stalled),boolean(io.persistentFailure),boolean(io.lastReadMissing));
}
inline void WriteLeaderLookupDiagnostics(FILE* file){
    const auto& lookup=leaderLookup;size_t cached=0;for(const auto& identity:lookup.identities)if(identity.playerState)++cached;
    std::fprintf(file,"{\"wire_version\":%u,\"wire_bytes\":%zu,\"stage\":\"%s\",\"profile_verified\":%s,\"source_metadata_ready\":%s,\"resolved_metadata_ready\":%s,\"source_player_ids\":[%d,%d,%d],\"resolved_player_ids\":[%d,%d,%d],\"last_player_id\":%d,\"cached_local_identities\":%zu,\"resolutions\":%llu,\"pending\":%llu,\"invalid_ids\":%llu,\"duplicate_ids\":%llu,\"weak_changes\":%llu,\"world_mismatches\":%llu,\"writes_native_identity\":false,\"native_multicast_delivery_verified\":false,\"rendered_pixels_verified\":false}",
        WireVersion,sizeof(Wire),lookup.stage,lookup.profileVerified?"true":"false",lookup.sourceMetadataReady?"true":"false",lookup.resolvedMetadataReady?"true":"false",
        lookup.sourcePlayerIds[0],lookup.sourcePlayerIds[1],lookup.sourcePlayerIds[2],lookup.resolvedPlayerIds[0],lookup.resolvedPlayerIds[1],lookup.resolvedPlayerIds[2],
        lookup.lastPlayerId,cached,static_cast<unsigned long long>(lookup.resolutions),static_cast<unsigned long long>(lookup.pending),
        static_cast<unsigned long long>(lookup.invalidIds),static_cast<unsigned long long>(lookup.duplicateIds),static_cast<unsigned long long>(lookup.weakChanges),
        static_cast<unsigned long long>(lookup.worldMismatches));
}
template<class Json> inline void WriteEffects(Json& out,size_t index=0){
    out.Add("[");if(index<MaxPlayers){const auto& player=players[index];
        for(size_t i=0;i<player.controlCount;++i){const auto& control=player.controls[i];if(i)out.Add(",");
            out.Add("{\"key\":");out.String(control.key);out.Add(",\"label\":");out.String(control.label);
            out.Add(",\"available\":%s,\"supported\":%s,\"requested\":%u,\"applied\":%u,\"verified\":%s,\"description\":",
                control.available?"true":"false",control.supported?"true":"false",static_cast<unsigned>(control.requested),
                static_cast<unsigned>(control.applied),control.verified?"true":"false");out.String(Explain(control.reason));
            out.Add(",\"numeric\":%s,\"value\":%.9g,\"minimum\":%.9g,\"maximum\":%.9g,\"observed_value\":%.9g,\"server_map_verified\":%s,\"peer_verified\":%s,\"next_original_event\":%s,\"rendered_pixels_verified\":false}",
                control.numeric?"true":"false",static_cast<double>(control.value),static_cast<double>(control.minimum),static_cast<double>(control.maximum),
                static_cast<double>(control.observedValue),control.serverMapVerified?"true":"false",control.peerVerified?"true":"false",control.nextOriginalEvent?"true":"false");
        }}out.Add("]");
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    std::fprintf(file,"{\"schema_version\":1,\"host\":%s,\"stage\":\"%s\",\"error\":%u,\"command_result\":%u,\"command_stage\":\"%s\",\"profiles_verified\":%s,\"world_bound\":%s,\"peer_bound\":%s,\"metadata_ready\":%s,\"publications\":%llu,\"receipts\":%llu,\"rejections\":%llu,\"preferences\":%zu,\"native_map_writes\":%llu,\"original_material_graph_calls\":%llu,\"cosmetic_counter_writes\":%llu,\"restores\":%llu,\"native_leader_assignments\":%llu,\"native_leader_broadcasts\":%llu,\"leader_eligible_count\":%d,\"leader_stage\":\"%s\",\"exception_rva\":%llu,\"rendered_pixels_verified\":false,\"leader_replication\":",
        state.host?"true":"false",state.stage,state.error,static_cast<unsigned>(state.commandResult),state.commandStage,state.profilesVerified?"true":"false",
        state.worldBound?"true":"false",state.peerBound?"true":"false",state.metadataReady?"true":"false",
        static_cast<unsigned long long>(state.publications),static_cast<unsigned long long>(state.receipts),static_cast<unsigned long long>(state.rejections),preferences.size(),
        static_cast<unsigned long long>(state.nativeMapWrites),static_cast<unsigned long long>(state.materialGraphCalls),
        static_cast<unsigned long long>(state.cosmeticCounterWrites),static_cast<unsigned long long>(state.restores),
        static_cast<unsigned long long>(leader.assignments),static_cast<unsigned long long>(leader.broadcasts),leader.eligibleCount,leader.stage,
        static_cast<unsigned long long>(state.exceptionRva));
    const auto& route=leader.replication;const auto boolean=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"stage\":\"%s\",\"owner\":\"%p\",\"outer\":\"%p\",\"flags_a0\":%u,\"flags_c1\":%u,\"flags_c2\":%u,\"owner_matches\":%s,\"outer_matches\":%s,\"weak_live\":%s,\"replicates\":%s,\"uses_registered_list\":%s,\"arrays_valid\":%s,\"replicated_count\":%d,\"registered_count\":%d,\"replicated_matches\":%d,\"registered_matches\":%d,\"registered_condition\":%d,\"registered_identity_valid\":%s,\"condition_supported\":%s,\"ready\":%s,\"pending\":%s},\"metadata_io\":",
        route.stage,route.owner,route.outer,static_cast<unsigned>(route.flagsA0),static_cast<unsigned>(route.flagsC1),static_cast<unsigned>(route.flagsC2),
        boolean(route.ownerMatches),boolean(route.outerMatches),boolean(route.weakIdentitiesLive),boolean(route.replicates),boolean(route.usesRegisteredList),
        boolean(route.arraysValid),route.replicatedCount,route.registeredCount,route.replicatedMatches,route.registeredMatches,route.condition,
        boolean(route.registeredIdentityValid),boolean(route.conditionSupported),boolean(route.ready),boolean(route.pending));
    WriteMetadataIODiagnostics(file);std::fputs(",\"canonical_leader_lookup\":",file);WriteLeaderLookupDiagnostics(file);std::fputs(",\"players\":[",file);
    bool comma=false;for(const auto& player:players)if(player.registered){
        std::fprintf(file,"%s{\"index\":%u,\"epoch\":%u,\"generation\":%llu,\"source_ready\":%s,\"snapshot_ready\":%s,\"assets_ready\":%s,\"control_count\":%zu,\"effect_count\":%zu,\"peer_scope\":%s,\"error\":%u,\"stage\":\"%s\",\"champion_scan\":",
            comma?",":"",player.index,player.epoch,static_cast<unsigned long long>(player.generation),player.sourceReady?"true":"false",
            player.snapshotReady?"true":"false",player.assetsReady?"true":"false",player.controlCount,player.effectCount,player.peerScope?"true":"false",player.error,player.stage);
        WriteChampionScanDiagnostics(file,player);std::fputc('}',file);comma=true;}
    std::fputs("]}\n",file);const bool okay=std::ferror(file)==0;std::fclose(file);return okay;
}
} // namespace local_server_cosmetics
