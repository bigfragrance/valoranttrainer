#pragma once
#include "deathmatch_rules.h"
#include "training_camera_modifiers_probe.h"
#include <array>
#include <cmath>
#include <cstring>

// Explicit stage9 diagnostics only. Disabled by default. Reads native posture,
// view and completed Camera-bone caches; never evaluates a pose or changes input.
// Reference profiling follows original7878570/7877b10 once per real mesh asset.
namespace training_local_posture_probe {
using Vec=engine::Vec;
inline constexpr size_t Capacity=512,MaxProfiles=64,MaxBones=4096;
struct Quick {
    uint64_t observed=0,milliseconds=0,epoch=0;int npc=-1;bool requested=false,valid=false;
    char phase[48]{};void *pawn=nullptr,*pc=nullptr,*root=nullptr,*follow=nullptr,*pcm=nullptr;
    uint8_t held=0,state=0,syncHeld=0;float capsule=0;
    Vec actor{},followPosition{},pcmPosition{};
};
struct Component {
    void *object=nullptr,*owner=nullptr,*parent=nullptr;bool readable=false;
    uint8_t paddingBeforeWorld[7]{};
    engine::Transform world{};Vec relative{};uint8_t paddingAfterRelative[8]{};
};
struct Bone {
    bool available=false,stable=false,leaderMapped=false,referenceValid=false;
    int32_t index=-1,sourceIndex=-1,buffer=-1,count=0,capacity=0,referenceIndex=-1;
    uint8_t paddingBeforeFrame[4]{};
    uint64_t poseFrame=0;void *source=nullptr,*data=nullptr,*asset=nullptr,*skeleton=nullptr;
    uint8_t paddingBeforeTransforms[8]{};
    engine::Transform current{},reference{};Vec translationDelta{};
    const char* state="completed_camera_pose_not_sampled";
};
struct Sample {
    uint64_t sequence=0;bool pairedAfter=false,localChange=false,npcEdge=false,heldQuery=false,eyesQuery=false;
    Quick value{},previousObservation{};uint8_t originalHeld=0;float crouchCompression=0;
    void *localPlayer=nullptr,*pcmTarget=nullptr,*npcPawn=nullptr,*core=nullptr,*coreOwner=nullptr;
    void *anim=nullptr,*animOuter=nullptr,*animOwnerActor=nullptr,*animOwnerComponent=nullptr;
    bool actualLocalPair=false,animOwnerQuery=false;uint64_t corePoseFrame=0;
    uint8_t npcHeld=0,npcState=0;float npcCapsule=0,pcmTime=0;
    Vec eyes{},eyeRotation{},pcmRotation{};uint8_t paddingBeforeComponents[8]{};
    Component root{},follow{};Bone cameraBone{};
    unsigned error=0;const char* reason="captured";
};
struct Profile {
    void *asset=nullptr,*skeleton=nullptr;int32_t meshIndex=-1,referenceIndex=-1;
    bool attempted=false,referenceValid=false;uint8_t paddingBeforeReference[6]{};engine::Transform reference{};
    const char* reason="asset_not_profiled";uint8_t paddingAfterReason[8]{};
};
static_assert(sizeof(Quick)==200&&sizeof(Component)==160&&sizeof(Bone)==304&&sizeof(Sample)==1248&&sizeof(Profile)==144);
struct Ticket {uint64_t at=0;bool pending=false,seenRequest=false,request=false;};
struct Diagnostics {
    bool enabled=false,initialized=false;unsigned error=0;
    uint64_t observed=0,captured=0,overwritten=0,throttled=0,pairedAfter=0,localChanges=0,npcEdges=0;
    uint64_t profiles=0,referenceCalls=0,cacheAvailable=0,cachePending=0,exceptions=0;
    uintptr_t exceptionRva=0;const char* reason="probe_disabled";
};
inline Diagnostics state;
inline std::array<Sample,Capacity> ring{};
inline std::array<Profile,MaxProfiles> profiles{};
inline std::array<Ticket,original_players::MaxPlayers+1> tickets{};
inline std::array<uint64_t,16> otherPhaseTimes{};
inline Quick previous{};inline size_t count=0,head=0,profileCount=0;
inline uint64_t npcPeriodicAt=0,ownPeriodicAt=0;
inline void *heldFunction=nullptr,*eyesFunction=nullptr,*boneIndexFunction=nullptr,*animFunction=nullptr;
inline void *animActorFunction=nullptr,*animComponentFunction=nullptr;
inline void *ownerCacheAnim=nullptr,*ownerCacheMesh=nullptr,*ownerCacheActor=nullptr,*ownerCacheComponent=nullptr;
inline uint64_t ownerCacheEpoch=0;inline bool ownerCacheValid=false;
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* p,size_t at){return original_players::Value<T>(p,at);}
inline bool Thread(){return engine::image&&original_players::gameThread&&GetCurrentThreadId()==original_players::gameThread;}
inline void SetEnabled(bool enabled,bool reset=false){
    if(reset){state={};count=head=profileCount=0;tickets={};otherPhaseTimes={};previous={};npcPeriodicAt=ownPeriodicAt=0;
        ownerCacheAnim=ownerCacheMesh=ownerCacheActor=ownerCacheComponent=nullptr;ownerCacheEpoch=0;ownerCacheValid=false;}
    state.enabled=enabled;if(!enabled)state.reason="probe_disabled";
}
inline void Enable(bool enabled=true){SetEnabled(enabled);}
inline void* Method(const wchar_t* path,uint16_t bytes){auto p=engine::Find(path);return p&&Read<uint16_t>(p,0xbe)==bytes?p:nullptr;}
inline bool Initialize(){
    if(state.initialized)return true;
    heldFunction=Method(L"/Script/ShooterGame.ShooterCharacter.GetCrouchHeld",1);
    eyesFunction=Method(L"/Script/Engine.Actor.GetActorEyesViewPoint",48);
    boneIndexFunction=Method(L"/Script/Engine.SkinnedMeshComponent.GetBoneIndex",16);
    animFunction=Method(L"/Script/Engine.SkeletalMeshComponent.GetAnimInstance",8);
    animActorFunction=Method(L"/Script/Engine.AnimInstance.GetOwningActor",8);
    animComponentFunction=Method(L"/Script/Engine.AnimInstance.GetOwningComponent",8);
    state.initialized=heldFunction&&eyesFunction&&boneIndexFunction&&animFunction;
    if(!state.initialized){state.error=1;state.reason="posture_probe_original_query_abi_unavailable";}return state.initialized;
}
inline bool Event(void* object,void* function,void* parameters){
    auto table=Read<void**>(object,0);auto process=Read<void*>(table,0x298);
    if(!object||!function||!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,function,parameters);return true;
}
inline bool Finite(const engine::Transform& t){
    if(!original_players::Finite(t.translation)||!original_players::Finite(t.scale))return false;
    for(double q:t.rotation)if(!std::isfinite(q))return false;return true;
}
inline Component ReadComponent(void* object){
    Component out;out.object=object;if(!object)return out;out.owner=Read<void*>(object,0xc8);out.parent=Read<void*>(object,0xe8);
    out.readable=original_players::Read(object,0x300,out.world)&&original_players::Read(object,0x170,out.relative)
        &&Finite(out.world)&&original_players::Finite(out.relative);
    if(!out.readable){out.world={};out.relative={};}return out;
}
inline Quick ReadQuick(const char* phase,int npc,bool requested){
    Quick out;out.observed=++state.observed;out.milliseconds=GetTickCount64();out.npc=npc;out.requested=requested;
    strncpy_s(out.phase,phase?phase:"unknown",_TRUNCATE);out.pawn=original_players::activePawn;out.pc=original_players::activeController;
    const int index=original_players::localIndex;const auto player=index>=0?deathmatch_rules::Player(static_cast<size_t>(index)):nullptr;
    if(player&&player->pawn==out.pawn)out.epoch=player->spawnEpoch;
    out.root=Read<void*>(out.pawn,0x290);out.follow=Read<void*>(out.pawn,0x12a8);out.pcm=Read<void*>(out.pc,0x528);
    out.held=Read<uint8_t>(out.pawn,0x1538)&1;out.state=Read<uint8_t>(out.pawn,0x1139);out.syncHeld=Read<uint8_t>(out.pawn,0xf7a);
    out.capsule=Read<float>(Read<void*>(out.pawn,0x500),0x6d8);
    out.valid=out.pawn&&out.pc&&original_players::Read(out.root,0x320,out.actor)
        &&original_players::Read(out.follow,0x320,out.followPosition)&&original_players::Read(out.pcm,0x17c0,out.pcmPosition)
        &&original_players::Finite(out.actor)&&original_players::Finite(out.followPosition)&&original_players::Finite(out.pcmPosition)
        &&std::isfinite(out.capsule);
    if(!out.valid){out.actor={};out.followPosition={};out.pcmPosition={};if(!std::isfinite(out.capsule))out.capsule=0;}return out;
}
inline bool Changed(const Quick& a,const Quick& b){
    if(!a.valid||!b.valid||a.pawn!=b.pawn||a.pc!=b.pc||a.epoch!=b.epoch)return true;
    return a.held!=b.held||a.state!=b.state||std::abs(a.capsule-b.capsule)>.02f
        ||std::abs(a.followPosition.z-b.followPosition.z)>.25||std::abs(a.pcmPosition.z-b.pcmPosition.z)>.25;
}
inline bool Contains(const char* text,const char* word){
    if(!text||!word)return false;
    for(const char* p=text;*p;++p){size_t i=0;for(;word[i]&&p[i];++i){char c=p[i];if(c>='A'&&c<='Z')c=static_cast<char>(c-'A'+'a');if(c!=word[i])break;}if(!word[i])return true;}return false;
}
inline uint32_t Hash(const char* p){uint32_t h=2166136261u;for(;p&&*p;++p)h=(h^static_cast<uint8_t>(*p))*16777619u;return h;}
inline bool Select(const Quick& q,bool changed,bool& edge,bool& paired){
    edge=paired=false;const bool before=Contains(q.phase,"before"),after=Contains(q.phase,"after");
    auto& ticket=tickets[q.npc>=0&&q.npc<static_cast<int>(original_players::MaxPlayers)?static_cast<size_t>(q.npc):original_players::MaxPlayers];
    if(q.npc>=0){edge=!ticket.seenRequest||ticket.request!=q.requested;ticket.seenRequest=true;ticket.request=q.requested;}
    if(after&&!before&&ticket.pending){paired=true;ticket.pending=false;return true;}
    // Periodic sampling is100Hz for the entire NPC domain, not100Hz perNPC.
    // Each selected before retains its same-NPC after even inside that limit.
    auto& periodicAt=q.npc>=0?npcPeriodicAt:ownPeriodicAt;const bool periodic=q.milliseconds>=periodicAt;
    if(before&&!after){const bool selected=periodic||edge||changed;ticket.pending=selected;if(selected){ticket.at=q.milliseconds;periodicAt=q.milliseconds+10;}return selected;}
    if(after&&!before){const bool selected=periodic||edge||changed;if(selected){ticket.at=q.milliseconds;periodicAt=q.milliseconds+10;}return selected;}
    auto& at=otherPhaseTimes[Hash(q.phase)%otherPhaseTimes.size()];const bool selected=q.milliseconds>=at+10||edge||changed;if(selected)at=q.milliseconds;return selected;
}
inline Profile* ProfileAsset(void* pawn,void* mesh){
    auto asset=Read<void*>(mesh,0x708);if(!asset)asset=Read<void*>(mesh,0x710);
    if(!asset)return nullptr;
    for(size_t i=0;i<profileCount;++i)if(profiles[i].asset==asset)return &profiles[i];
    if(profileCount==profiles.size())return nullptr;
    auto& p=profiles[profileCount++];p={};p.attempted=true;p.asset=asset;++state.profiles;
    if(!original_players::IsA(mesh,0x385f750)||!original_players::IsA(asset,0x3815ac0)){p.reason="camera_mesh_asset_type_unverified";return &p;}
    alignas(8) uint8_t args[16]{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(args,L"Camera",1);
    const int32_t missing=-1;std::memcpy(args+12,&missing,4);
    if(!Event(mesh,boneIndexFunction,args)){p.reason="camera_bone_index_query_failed";return &p;}
    std::memcpy(&p.meshIndex,args+12,4);if(p.meshIndex<0){p.reason="camera_bone_missing_from_actual_mesh";return &p;}
    auto table=Read<void**>(asset,0);auto getter=Read<void*>(table,0x448);
    // Original skeletal asset v448=26cc5a0 is the leaf return[this+f8].
    // The Camera reference comes from this asset's USkeleton, not AnimInstance.
    if(getter!=reinterpret_cast<void*>(engine::image+0x26cc5a0)){p.reason="original_asset_skeleton_getter_changed";return &p;}
    p.skeleton=reinterpret_cast<void*(*)(void*)>(getter)(asset);
    if(!original_players::IsA(p.skeleton,0x3feaf30)){p.reason="original_asset_reference_skeleton_missing";return &p;}
    auto reference=static_cast<uint8_t*>(p.skeleton)+0x50;
    p.referenceIndex=engine::function<int32_t(*)(void*,const void*)>(0x38577a0)(reference,args);
    auto infos=Read<void*>(reference,0x20);auto poses=Read<void*>(reference,0x30);
    const int32_t infoCount=Read<int32_t>(reference,0x28),poseCount=Read<int32_t>(reference,0x38);
    if(!infos||!poses||infoCount<=0||infoCount>static_cast<int32_t>(MaxBones)||poseCount!=infoCount
       ||p.referenceIndex<0||p.referenceIndex>=infoCount){p.reason="original_reference_arrays_invalid";return &p;}
    // Bound the exact parent chain before original7877b10 walks it. The helper
    // reads asset parents/transforms and math only; no live pose/socket query.
    int32_t parent=p.referenceIndex;std::array<int32_t,128> seen{};size_t ancestors=0;
    while(parent!=-1){if(parent<0||parent>=infoCount||ancestors==seen.size()){p.reason="reference_parent_chain_invalid";return &p;}
        for(size_t n=0;n<ancestors;++n)if(seen[n]==parent){p.reason="reference_parent_cycle";return &p;}
        seen[ancestors++]=parent;if(!original_players::Read(infos,static_cast<size_t>(parent)*16+12,parent)){p.reason="reference_parent_unreadable";return &p;}}
    ++state.referenceCalls;
    engine::function<void(*)(void*,engine::Transform*,void*,int32_t)>(0x7877b10)(pawn,&p.reference,reference,p.referenceIndex);
    p.referenceValid=Finite(p.reference);if(!p.referenceValid)p.reference={};p.reason=p.referenceValid?"original_asset_reference_profiled":"original_reference_nonfinite";return &p;
}
inline Bone ReadCameraBone(void* pawn,void* mesh){
    Bone out;auto profile=ProfileAsset(pawn,mesh);if(!profile){out.state="camera_asset_profile_unavailable";return out;}
    out.asset=profile->asset;out.skeleton=profile->skeleton;out.index=profile->meshIndex;out.sourceIndex=out.index;
    out.referenceIndex=profile->referenceIndex;out.referenceValid=profile->referenceValid;out.reference=profile->reference;
    if(out.index<0){out.state=profile->reason;return out;}
    auto source=mesh;auto leader=engine::function<void*(*)(void*)>(0x1d4cfa0)(static_cast<uint8_t*>(mesh)+0x718);
    if(leader){out.leaderMapped=true;out.source=leader;
        if(leader==mesh||!original_players::IsA(leader,0x385f750)||(Read<uint8_t>(leader,0xa0)&1)==0){out.state="original_leader_pose_unavailable";return out;}
        auto map=Read<void*>(mesh,0x7c8);const int32_t n=Read<int32_t>(mesh,0x7d0);
        if(n<0||n>static_cast<int32_t>(MaxBones)||out.index>=n||!original_players::Read(map,static_cast<size_t>(out.index)*4,out.sourceIndex)||out.sourceIndex<0){out.state="original_leader_bone_map_unavailable";return out;}
        if(engine::function<void*(*)(void*)>(0x1d4cfa0)(static_cast<uint8_t*>(leader)+0x718)){out.state="nested_original_leader_pose_pending";return out;}source=leader;
    }
    out.source=source;out.poseFrame=Read<uint64_t>(source,0xd78);if(!out.poseFrame){out.state="original_pose_not_evaluated";return out;}
    out.buffer=Read<int32_t>(source,0x7a8);if(out.buffer<0||out.buffer>1){out.state="original_pose_buffer_index_pending";return out;}
    const size_t at=0x760+static_cast<size_t>(out.buffer)*16;out.data=Read<void*>(source,at);out.count=Read<int32_t>(source,at+8);out.capacity=Read<int32_t>(source,at+12);
    if(!out.data||out.count<=0||out.count>static_cast<int32_t>(MaxBones)||out.capacity<out.count||out.sourceIndex>=out.count){out.state="original_pose_buffer_pending";return out;}
    if(!original_players::Read(out.data,static_cast<size_t>(out.sourceIndex)*sizeof(engine::Transform),out.current)||!Finite(out.current)){out.current={};out.state="original_pose_cache_unreadable_or_nonfinite";return out;}
    out.stable=Read<int32_t>(source,0x7a8)==out.buffer&&Read<void*>(source,at)==out.data
        &&Read<int32_t>(source,at+8)==out.count&&Read<uint64_t>(source,0xd78)==out.poseFrame;
    if(!out.stable){out.state="original_pose_cache_advanced_during_sample";return out;}
    out.available=true;if(out.referenceValid){out.translationDelta={out.current.translation.x-out.reference.translation.x,out.current.translation.y-out.reference.translation.y,out.current.translation.z-out.reference.translation.z};
        if(!original_players::Finite(out.translationDelta)){out.referenceValid=false;out.translationDelta={};}}
    out.state=out.leaderMapped?"original_leader_mapped_completed_camera_pose":"original_completed_camera_pose";return out;
}
inline void CaptureImpl(const char* phase,int npc,bool requested){
    if(!state.enabled)return;if(!Thread()){state.error=2;state.reason="posture_probe_not_owned_game_thread";return;}
    const auto quick=ReadQuick(phase,npc,requested);const bool changed=Changed(previous,quick);bool edge=false,paired=false;
    const bool selected=Select(quick,changed,edge,paired);const auto prior=previous;previous=quick;
    if(!selected){++state.throttled;return;}Initialize();
    Sample s;s.sequence=++state.captured;s.value=quick;s.previousObservation=prior;s.localChange=changed;s.npcEdge=edge;s.pairedAfter=paired;
    if(changed)++state.localChanges;if(edge)++state.npcEdges;if(paired)++state.pairedAfter;
    s.localPlayer=original_players::localPlayer;s.actualLocalPair=s.localPlayer&&Read<void*>(quick.pc,0x510)==s.localPlayer
        &&Read<void*>(s.localPlayer,0x38)==quick.pc&&Read<void*>(quick.pc,0x4b0)==quick.pawn;
    s.heldQuery=Event(quick.pawn,heldFunction,&s.originalHeld);s.crouchCompression=Read<float>(quick.pawn,0x1134);
    struct Eyes {Vec position{},rotation{};} eyes;
    s.eyesQuery=Event(quick.pawn,eyesFunction,&eyes)&&original_players::Finite(eyes.position)&&original_players::Finite(eyes.rotation);
    if(s.eyesQuery){s.eyes=eyes.position;s.eyeRotation=eyes.rotation;}
    s.root=ReadComponent(quick.root);s.follow=ReadComponent(quick.follow);s.pcmTarget=Read<void*>(quick.pcm,0x580);
    s.pcmTime=Read<float>(quick.pcm,0x17b0);s.pcmRotation=Read<Vec>(quick.pcm,0x17d8);
    if(!original_players::Finite(s.pcmRotation))s.pcmRotation={};if(!std::isfinite(s.pcmTime))s.pcmTime=0;
    s.core=Read<void*>(quick.pawn,0xf30);s.coreOwner=Read<void*>(s.core,0xc8);s.corePoseFrame=Read<uint64_t>(s.core,0xd78);
    Event(s.core,animFunction,&s.anim);s.animOuter=Read<void*>(s.anim,0x28);
    if(ownerCacheAnim!=s.anim||ownerCacheMesh!=s.core||ownerCacheEpoch!=quick.epoch){
        ownerCacheAnim=s.anim;ownerCacheMesh=s.core;ownerCacheEpoch=quick.epoch;ownerCacheActor=ownerCacheComponent=nullptr;
        ownerCacheValid=Event(s.anim,animActorFunction,&ownerCacheActor)&&Event(s.anim,animComponentFunction,&ownerCacheComponent);
    }
    s.animOwnerActor=ownerCacheActor;s.animOwnerComponent=ownerCacheComponent;s.animOwnerQuery=ownerCacheValid;
    if(s.core&&original_players::IsA(s.core,0x385f750))s.cameraBone=ReadCameraBone(quick.pawn,s.core);
    if(s.cameraBone.available)++state.cacheAvailable;else ++state.cachePending;
    if(npc>=0&&npc<static_cast<int>(original_players::MaxPlayers)){s.npcPawn=original_players::players[static_cast<size_t>(npc)].character;
        s.npcHeld=Read<uint8_t>(s.npcPawn,0x1538)&1;s.npcState=Read<uint8_t>(s.npcPawn,0x1139);s.npcCapsule=Read<float>(Read<void*>(s.npcPawn,0x500),0x6d8);}
    if(!quick.valid||!s.heldQuery||!s.eyesQuery){s.error=3;s.reason="posture_probe_partial_original_readback";}
    if(!std::isfinite(s.crouchCompression))s.crouchCompression=0;if(!std::isfinite(s.npcCapsule))s.npcCapsule=0;
    ring[head]=s;head=(head+1)%Capacity;if(count<Capacity)++count;else ++state.overwritten;
    state.reason=s.reason;if(!s.error)state.error=0;
}
inline LONG CaptureException(EXCEPTION_POINTERS* e){
    ++state.exceptions;state.error=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:4;
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.reason="posture_probe_read_exception_gameplay_not_modified";return EXCEPTION_EXECUTE_HANDLER;
}
inline void Capture(const char* phase,int npc=-1,bool npcRequestedCrouch=false){
    if(!state.enabled)return;__try{CaptureImpl(phase,npc,npcRequestedCrouch);}__except(CaptureException(GetExceptionInformation())){}
}
inline void Vector(FILE* f,const Vec& v){original_players::Vector(f,v);}
inline void Transform(FILE* f,const engine::Transform& t){
    std::fputs("{\"translation\":",f);Vector(f,t.translation);std::fputs(",\"scale\":",f);Vector(f,t.scale);std::fputs(",\"rotation_xyzw\":[",f);
    for(size_t i=0;i<4;++i){if(i)std::fputc(',',f);if(std::isfinite(t.rotation[i]))std::fprintf(f,"%.17g",t.rotation[i]);else std::fputs("null",f);}std::fputs("]}",f);
}
inline void WriteQuick(FILE* f,const Quick& q){
    std::fprintf(f,"{\"observed\":%llu,\"milliseconds\":%llu,\"epoch\":%llu,\"npc\":%d,\"npc_requested_crouch\":%s,\"valid\":%s,\"phase\":",static_cast<unsigned long long>(q.observed),static_cast<unsigned long long>(q.milliseconds),static_cast<unsigned long long>(q.epoch),q.npc,q.requested?"true":"false",q.valid?"true":"false");original_players::Text(f,q.phase);
    std::fprintf(f,",\"pawn\":\"%p\",\"controller\":\"%p\",\"root\":\"%p\",\"follow\":\"%p\",\"pcm\":\"%p\",\"raw_held_1538\":%u,\"crouch_state_1139\":%u,\"sync_held_f7a\":%u,\"capsule_half_height\":%.9g,\"actor\":",q.pawn,q.pc,q.root,q.follow,q.pcm,static_cast<unsigned>(q.held),static_cast<unsigned>(q.state),static_cast<unsigned>(q.syncHeld),q.capsule);Vector(f,q.actor);std::fputs(",\"follow_position\":",f);Vector(f,q.followPosition);std::fputs(",\"pcm_position\":",f);Vector(f,q.pcmPosition);std::fputc('}',f);
}
inline void WriteComponent(FILE* f,const Component& c){
    std::fprintf(f,"{\"object\":\"%p\",\"owner\":\"%p\",\"parent\":\"%p\",\"readable\":%s,\"relative_position\":",c.object,c.owner,c.parent,c.readable?"true":"false");Vector(f,c.relative);std::fputs(",\"cached_world_transform\":",f);Transform(f,c.world);std::fputc('}',f);
}
inline void WriteBone(FILE* f,const Bone& b){
    std::fprintf(f,"{\"available\":%s,\"stable\":%s,\"leader_mapped\":%s,\"reference_valid\":%s,\"mesh_bone_index\":%d,\"source_bone_index\":%d,\"reference_bone_index\":%d,\"buffer\":%d,\"count\":%d,\"capacity\":%d,\"pose_frame\":%llu,\"source\":\"%p\",\"data\":\"%p\",\"asset\":\"%p\",\"skeleton\":\"%p\",\"state\":",b.available?"true":"false",b.stable?"true":"false",b.leaderMapped?"true":"false",b.referenceValid?"true":"false",b.index,b.sourceIndex,b.referenceIndex,b.buffer,b.count,b.capacity,static_cast<unsigned long long>(b.poseFrame),b.source,b.data,b.asset,b.skeleton);original_players::Text(f,b.state);std::fputs(",\"completed_component_space_camera\":",f);Transform(f,b.current);std::fputs(",\"original_reference_component_space_camera\":",f);Transform(f,b.reference);std::fputs(",\"translation_delta\":",f);Vector(f,b.translationDelta);std::fputc('}',f);
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;std::wstring temp(path);temp+=L".tmp";FILE* f=nullptr;if(_wfopen_s(&f,temp.c_str(),L"wb")||!f)return false;
    std::fprintf(f,"{\"schema_version\":1,\"enabled\":%s,\"initialized\":%s,\"error\":%u,\"observed\":%llu,\"captured\":%llu,\"overwritten\":%llu,\"throttled\":%llu,\"paired_after\":%llu,\"local_changes\":%llu,\"npc_edges\":%llu,\"profile_count\":%zu,\"native_reference_profile_calls\":%llu,\"cache_available\":%llu,\"cache_pending\":%llu,\"exceptions\":%llu,\"exception_rva\":%llu,\"capacity\":%zu,\"retained\":%zu,\"reason\":",state.enabled?"true":"false",state.initialized?"true":"false",state.error,static_cast<unsigned long long>(state.observed),static_cast<unsigned long long>(state.captured),static_cast<unsigned long long>(state.overwritten),static_cast<unsigned long long>(state.throttled),static_cast<unsigned long long>(state.pairedAfter),static_cast<unsigned long long>(state.localChanges),static_cast<unsigned long long>(state.npcEdges),profileCount,static_cast<unsigned long long>(state.referenceCalls),static_cast<unsigned long long>(state.cacheAvailable),static_cast<unsigned long long>(state.cachePending),static_cast<unsigned long long>(state.exceptions),static_cast<unsigned long long>(state.exceptionRva),Capacity,count);original_players::Text(f,state.reason);
    std::fputs(",\"active_pose_evaluation\":false,\"socket_or_bone_transform_query\":false,\"gameplay_writes\":false,\"periodic_hz_per_npc_domain\":100,\"periodic_hz_per_own_camera_domain\":100,\"paired_after_retained\":true,\"anim_owner_query_frequency\":\"once_per_anim_mesh_epoch\",\"samples\":[",f);
    const size_t first=(head+Capacity-count)%Capacity;
    for(size_t i=0;i<count;++i){const auto& s=ring[(first+i)%Capacity];if(i)std::fputc(',',f);
        std::fprintf(f,"{\"sequence\":%llu,\"paired_after\":%s,\"local_change\":%s,\"npc_edge\":%s,\"held_query\":%s,\"original_crouch_held\":%u,\"eyes_query\":%s,\"actual_local_pair\":%s,\"local_player\":\"%p\",\"pcm_view_target\":\"%p\",\"npc_pawn\":\"%p\",\"npc_raw_held\":%u,\"npc_crouch_state\":%u,\"npc_capsule\":%.9g,\"crouch_compression\":%.9g,\"pcm_cache_time\":%.9g,\"core1p\":\"%p\",\"core1p_owner\":\"%p\",\"core1p_pose_frame\":%llu,\"anim_instance\":\"%p\",\"anim_outer\":\"%p\",\"anim_owning_actor\":\"%p\",\"anim_owning_component\":\"%p\",\"anim_owner_query\":%s,\"error\":%u,\"reason\":",static_cast<unsigned long long>(s.sequence),s.pairedAfter?"true":"false",s.localChange?"true":"false",s.npcEdge?"true":"false",s.heldQuery?"true":"false",static_cast<unsigned>(s.originalHeld),s.eyesQuery?"true":"false",s.actualLocalPair?"true":"false",s.localPlayer,s.pcmTarget,s.npcPawn,static_cast<unsigned>(s.npcHeld),static_cast<unsigned>(s.npcState),s.npcCapsule,s.crouchCompression,s.pcmTime,s.core,s.coreOwner,static_cast<unsigned long long>(s.corePoseFrame),s.anim,s.animOuter,s.animOwnerActor,s.animOwnerComponent,s.animOwnerQuery?"true":"false",s.error);original_players::Text(f,s.reason);
        std::fputs(",\"observation\":",f);WriteQuick(f,s.value);std::fputs(",\"previous_observation\":",f);WriteQuick(f,s.previousObservation);
        std::fputs(",\"eyes\":",f);Vector(f,s.eyes);std::fputs(",\"eye_rotation\":",f);Vector(f,s.eyeRotation);std::fputs(",\"pcm_rotation\":",f);Vector(f,s.pcmRotation);
        std::fputs(",\"root\":",f);WriteComponent(f,s.root);std::fputs(",\"follow\":",f);WriteComponent(f,s.follow);std::fputs(",\"camera_bone\":",f);WriteBone(f,s.cameraBone);std::fputc('}',f);
    }
    std::fputs("],\"pcm_modifiers\":",f);training_camera_modifiers_probe::Write(f);
    std::fputs("}\n",f);const bool ok=std::ferror(f)==0;const bool closed=std::fclose(f)==0;
    return ok&&closed&&MoveFileExW(temp.c_str(),path,MOVEFILE_REPLACE_EXISTING);
}
}
