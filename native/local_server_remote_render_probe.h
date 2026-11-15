#pragma once
#include "original_scope.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

// Passive, game-thread sampling of actual replicated characters. This neither
// changes visibility/relevance nor repairs registration or mesh state. The
// writer reads only the cached POD. Source: character-render-review/.
namespace local_server_remote_render_probe {
inline constexpr size_t MaxRemote=29,MeshCount=6;
struct Weak {int32_t index=-1,serial=0;};
struct Array {void** data=nullptr;int32_t count=0,capacity=0;};
static_assert(sizeof(Weak)==8&&sizeof(Array)==16);
struct Mesh {
    void *object=nullptr,*owner=nullptr,*parent=nullptr,*skinnedAsset=nullptr,*skeletalMesh=nullptr;
    void *animClass=nullptr,*animInstance=nullptr,*leader=nullptr;Weak identity{};
    bool live=false,typed=false,skinned=false,skeletal=false,registered=false,renderStateCreated=false;
    bool visible=false,hiddenInGame=false,mainPass=false,ownerNoSee=false,onlyOwnerSee=false,firstPerson=false;
    bool poseAvailable=false;uint8_t flagsA0=0,flags1D0=0,flags3B1=0,flags3B2=0;
    uint64_t poseFrame=0;int32_t poseBuffer=-1,poseCount=0,poseCapacity=0;
    engine::Vec position{},scale{};
};
struct Row {
    void *playerState=nullptr,*pawn=nullptr,*root=nullptr,*weapon=nullptr,*movement=nullptr,*stream=nullptr,*fog=nullptr,*visibility=nullptr;
    Weak playerStateId{},pawnId{};
    uint8_t role=255,actorFlags65=0,initialized=0,movementStream1080=0,streamProcessingE0=0;
    bool links=false,channel=false,hidden=false,locallyHidden=false,movementLive=false,streamLive=false;
    bool fogLive=false,fogRelevant=false,fogHidden=false,fogReceived=false,fogSchema=false;
    bool visibilityLive=false,visibilitySchema=false,seenByResult=false;
    bool registryKnown=false,registryContains=false;int32_t registryHops=0;
    bool historyShapeKnown=false;void* historyData=nullptr;
    uint32_t historyRaw1498=0,historyRaw149C=0,historyRaw14A0=0;
    void *fogManager=nullptr,*fogOwner=nullptr,*visibilityOwner=nullptr;
    int32_t visibilityCharacters=0,visibilityCharactersCapacity=0;float visibilityLostTime=0;
    engine::Vec pawnPosition{},weaponPosition{};
    std::array<Mesh,MeshCount> meshes{};
};
struct Snapshot {
    const char* stage="remote_render_probe_not_sampled";
    unsigned error=0;bool profiles=false,schema=false,ownContext=false,rosterValid=false;
    bool pcManagerLive=false,gsManagerLive=false,managerSame=false,visibilityRpcKnown=false;
    bool managerSchema=false;uint64_t atMs=0,samples=0;uint32_t sampled=0,skippedOwn=0,invalid=0,truncated=0;
    uint32_t profileFailureRva=0;void *world=nullptr,*controller=nullptr,*ownPawn=nullptr,*gs=nullptr,*connection=nullptr;
    void *pcManager=nullptr,*gsManager=nullptr,*visibilityRpc=nullptr;
    int32_t rosterCount=0;std::array<Row,MaxRemote> rows{};
};
inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline Snapshot cached{};
inline bool profilesChecked=false,profilesMatch=false;
inline uint32_t failedProfile=0;
inline uint64_t lastSampleAt=0;
struct Profile {uint32_t rva,bytes;uint64_t fnv;};
inline constexpr Profile Profiles[]{
    {0x1cf35b0,1571,0x40614eae57888396ULL},{0x1d4cfa0,1562,0xb050d7efb4cf1a25ULL},
    {0x1a7b720,196,0x4982523f72f33036ULL},{0x1c63870,269,0xde792136130ef719ULL},
    {0x3ed83b0,1609,0x15d2d4ecc2f49cf7ULL},{0x40e6df0,385,0xccee9eb5e245e130ULL},
    {0x78924b0,239,0x5f2be1d81dbcfefeULL},{0x7894010,1714,0x57c150b56d3eecfbULL},
    {0x7866400,388,0x406b4c6c25a0d421ULL},{0x781af80,53,0x5bd94d325b1d897cULL},
    {0x6f30950,142,0xc765862d5b81223aULL},{0x6f2fcd0,55,0x4bc198bdc6d74844ULL},
    {0x36c7580,180,0xecc03e484b1fca16ULL},{0x2a653a0,180,0xb1a914af6e04cfc7ULL},
    {0x3ea3e80,180,0xce6ea80a609ee232ULL},{0x385f750,180,0x99b0a29b41cc8650ULL},
    {0x6b662e0,174,0xcf0948dc72eaf2cbULL},{0x6c7d4c0,174,0x2672484d4b1a1ca2ULL},
    {0x63ac660,174,0x5ba33c3e076fa723ULL},
    {0x64edf90,949,0x9e3c5de874ecc344ULL},
};
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Identity(void* object,Weak& id){
    id.index=Read<int32_t>(object,0xc);if(!object||id.index<0)return false;
    id.serial=engine::function<int32_t(*)(void*,int32_t)>(0x1cf35b0)
        (reinterpret_cast<void*>(engine::image+0xd7b87c0),id.index);
    return id.serial>0&&engine::function<void*(*)(const Weak*)>(0x1d4cfa0)(&id)==object;
}
inline bool Live(void* object){Weak id{};return Identity(object,id);}
inline bool VerifyProfiles(){
    if(profilesChecked)return profilesMatch;profilesChecked=true;
    for(const auto& p:Profiles){const auto* bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes)){failedProfile=p.rva;return false;}
        uint64_t value=14695981039346656037ULL;
        for(uint32_t i=0;i<p.bytes;++i)value=(value^bytes[i])*1099511628211ULL;
        if(value!=p.fnv){failedProfile=p.rva;return false;}
    }
    profilesMatch=true;return true;
}
inline bool Field(void* type,const wchar_t* name,int offset){return original_scope::FieldOffset(type,name)==offset;}
inline bool Schema(void* pawn){
    const auto type=Read<void*>(pawn,0x10);
    return Field(type,L"bHidden",0x65)&&Field(type,L"bLocallyHidden",0x65)
        &&Field(type,L"Mesh",0x4f0)&&Field(type,L"Mesh1P",0xf30)&&Field(type,L"MeshOverlay1P",0xf38)
        &&Field(type,L"MeshCosmetic3P",0xf40)&&Field(type,L"FogOfWarComponent",0xa68)
        &&Field(type,L"NetworkStreamComponent",0xba8)
        &&Field(engine::Class(0x36c7580),L"bIsActive",0xc1)
        &&Field(engine::Class(0x2a653a0),L"bVisible",0x1d0)
        &&Field(engine::Class(0x2a653a0),L"bHiddenInGame",0x251)
        &&Field(engine::Class(0x2a653a0),L"bRenderInMainPass",0x3b1)
        &&Field(engine::Class(0x2a653a0),L"bOwnerNoSee",0x3b2)
        &&Field(engine::Class(0x2a653a0),L"bOnlyOwnerSee",0x3b2)
        &&Field(engine::Class(0x3ea3e80),L"SkeletalMesh",0x708)
        &&Field(engine::Class(0x3ea3e80),L"SkinnedAsset",0x710)
        &&Field(engine::Class(0x385f750),L"AnimClass",0xa68)
        &&Field(engine::Class(0x385f750),L"AnimScriptInstance",0xa70);
}
inline void SampleMesh(void* object,Mesh& m){
    m={};m.object=object;m.live=Identity(object,m.identity);if(!m.live)return;
    m.typed=original_players::IsA(object,0x2a653a0);if(!m.typed)return;
    m.owner=Read<void*>(object,0xc8);m.parent=Read<void*>(object,0xe8);
    m.flagsA0=Read<uint8_t>(object,0xa0);m.flags1D0=Read<uint8_t>(object,0x1d0);
    m.flags3B1=Read<uint8_t>(object,0x3b1);m.flags3B2=Read<uint8_t>(object,0x3b2);
    m.registered=(m.flagsA0&1)!=0;m.renderStateCreated=(m.flagsA0&2)!=0;
    m.visible=(m.flags1D0&0x20)!=0;m.hiddenInGame=(Read<uint8_t>(object,0x251)&2)!=0;
    m.mainPass=(m.flags3B1&0x10)!=0;m.ownerNoSee=(m.flags3B2&8)!=0;m.onlyOwnerSee=(m.flags3B2&0x10)!=0;
    m.firstPerson=(Read<uint32_t>(object,0x394)&1)!=0;
    m.position=Read<engine::Vec>(object,0x320);m.scale=Read<engine::Vec>(object,0x340);
    m.skinned=original_players::IsA(object,0x3ea3e80);m.skeletal=original_players::IsA(object,0x385f750);
    if(m.skinned){
        m.skeletalMesh=Read<void*>(object,0x708);m.skinnedAsset=Read<void*>(object,0x710);
        Weak leaderId=Read<Weak>(object,0x718);m.leader=engine::function<void*(*)(const Weak*)>(0x1d4cfa0)(&leaderId);
        m.poseBuffer=Read<int32_t>(object,0x7a8);
        if(m.poseBuffer>=0&&m.poseBuffer<=1){const size_t at=0x760+static_cast<size_t>(m.poseBuffer)*16;
            auto data=Read<void*>(object,at);m.poseCount=Read<int32_t>(object,at+8);m.poseCapacity=Read<int32_t>(object,at+12);
            m.poseAvailable=data&&m.poseCount>0&&m.poseCount<=4096&&m.poseCapacity>=m.poseCount;
        }
    }
    if(m.skeletal){m.animClass=Read<void*>(object,0xa68);m.animInstance=Read<void*>(object,0xa70);m.poseFrame=Read<uint64_t>(object,0xd78);}
}
inline bool RegistryContains(void* manager,void* movement,bool& result,int32_t& hops){
    result=false;hops=0;if(!manager||!movement)return false;
    const int32_t count=Read<int32_t>(manager,0x128),capacity=Read<int32_t>(manager,0x12c),free=Read<int32_t>(manager,0x154);
    if(count<0||count>4096||capacity<count||free<0||free>count)return false;
    if(count==free)return true;
    const int32_t hashSize=Read<int32_t>(manager,0x168);
    if(hashSize<1||hashSize>8192||(hashSize&(hashSize-1)))return false;
    auto buckets=Read<int32_t*>(manager,0x160);
    if(!buckets){if(hashSize!=1)return false;buckets=reinterpret_cast<int32_t*>(static_cast<uint8_t*>(manager)+0x158);}
    auto elements=Read<uint8_t*>(manager,0x120);
    if(!elements||!original_players::Readable(buckets,static_cast<size_t>(hashSize)*4)
       ||!original_players::Readable(elements,static_cast<size_t>(count)*16))return false;
    uint32_t hash=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(movement)>>4);
    hash=(hash^(hash>>16))*0x85ebca6bU;hash=(hash^(hash>>13))*0xc2b2ae35U;hash^=hash>>16;
    int32_t index=buckets[hash&(static_cast<uint32_t>(hashSize)-1)];
    while(index!=-1){if(index<0||index>=count||hops>=count)return false;++hops;
        const auto entry=elements+static_cast<size_t>(index)*16;
        if(Read<void*>(entry)==movement){result=true;return true;}index=Read<int32_t>(entry,8);
    }
    return true;
}
inline bool ActorChannel(void* connection,void* actor){
    const auto channels=Read<Array>(connection,0x78);
    if(channels.count<0||channels.count>4096||channels.capacity<channels.count||!channels.data)return false;
    for(int32_t n=0;n<channels.count;++n){auto channel=Read<void*>(channels.data,static_cast<size_t>(n)*8);
        if(channel&&Read<void*>(channel,0x30)==connection&&!(Read<uint8_t>(channel,0x38)&2)&&Read<void*>(channel,0x78)==actor)return true;}
    return false;
}
inline void Publish(const Snapshot& sample){
    AcquireSRWLockExclusive(&cacheLock);cached=sample;ReleaseSRWLockExclusive(&cacheLock);
}
inline bool PollImpl(void* world,void* pc,void* ownPawn){
    const auto now=GetTickCount64();if(now-lastSampleAt<100)return true;lastSampleAt=now;
    Snapshot next{};next.atMs=now;
    AcquireSRWLockShared(&cacheLock);next.samples=cached.samples+1;ReleaseSRWLockShared(&cacheLock);
    next.world=world;next.controller=pc;next.ownPawn=ownPawn;
    if(!engine::image||!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread){
        next.stage="remote_render_probe_waiting_original_game_thread";Publish(next);return true;}
    next.profiles=VerifyProfiles();next.profileFailureRva=failedProfile;
    if(!next.profiles){next.error=6201;next.stage="remote_render_probe_profile_unknown";Publish(next);return true;}
    next.connection=Read<void*>(Read<void*>(world,0x40),0xc0);
    next.ownContext=Live(world)&&original_players::IsA(pc,original_players::ShooterControllerClass)&&Live(pc)
        &&original_players::IsA(ownPawn,original_players::ShooterCharacterClass)&&Live(ownPawn)
        &&Read<uint8_t>(pc,0x224)==2&&Read<uint8_t>(ownPawn,0x224)==2&&Read<void*>(pc,0x4b0)==ownPawn
        &&Read<void*>(ownPawn,0x4a0)==pc&&Read<void*>(next.connection,0x38)==pc
        &&Read<void*>(next.connection,0x60)==Read<void*>(world,0x40)&&Read<uint8_t>(next.connection,0x13c)==3;
    if(!next.ownContext){next.stage="remote_render_probe_waiting_received_owned_client";Publish(next);return true;}
    next.schema=Schema(ownPawn);
    if(!next.schema){next.error=6202;next.stage="remote_render_probe_property_schema_unknown";Publish(next);return true;}
    next.gs=Read<void*>(world,0x178);const auto roster=Read<Array>(next.gs,0x480);next.rosterCount=roster.count;
    next.rosterValid=original_players::IsA(next.gs,original_players::ShooterGameStateClass)&&Live(next.gs)
        &&roster.count>=0&&roster.count<=128&&roster.capacity>=roster.count&&roster.capacity<=256
        &&(!roster.count||original_players::Readable(roster.data,static_cast<size_t>(roster.count)*8));
    if(!next.rosterValid){next.stage="remote_render_probe_waiting_native_roster";Publish(next);return true;}
    next.pcManager=Read<void*>(pc,0xd10);next.gsManager=Read<void*>(next.gs,0xcc8);
    next.pcManagerLive=original_players::IsA(next.pcManager,0x6b662e0)&&Live(next.pcManager);
    next.gsManagerLive=original_players::IsA(next.gsManager,0x6b662e0)&&Live(next.gsManager);
    next.managerSame=next.pcManagerLive&&next.gsManagerLive&&next.pcManager==next.gsManager;
    next.managerSchema=next.gsManagerLive&&Field(Read<void*>(next.gsManager,0x10),L"RemoteCharacterMovementComponents",0x120);
    next.visibilityRpc=engine::Find(L"/Script/ShooterGame.AresPlayerController.ClientUpdateVisibility");
    next.visibilityRpcKnown=next.visibilityRpc&&Read<uint16_t>(next.visibilityRpc,0xbe)==73
        &&Read<uint32_t>(next.visibilityRpc,0xb8)==0x01820c40U
        &&Read<uintptr_t>(next.visibilityRpc,0xe0)==engine::image+0x64edf90;
    auto ownPS=Read<void*>(pc,0x470);
    for(int32_t n=0;n<roster.count;++n){auto ps=Read<void*>(roster.data,static_cast<size_t>(n)*8),pawn=Read<void*>(ps,0xbc8);
        if(ps==ownPS||pawn==ownPawn){++next.skippedOwn;continue;}
        if(next.sampled>=MaxRemote){++next.truncated;continue;}
        Row row{};row.playerState=ps;row.pawn=pawn;
        if(!original_players::IsA(ps,original_players::ShooterStateClass)||!Identity(ps,row.playerStateId)
           ||!original_players::IsA(pawn,original_players::ShooterCharacterClass)||!Identity(pawn,row.pawnId)
           ||Read<uint8_t>(pawn,0x224)!=1||Read<void*>(pawn,0x488)!=ps
           ||Read<void*>(Read<void**>(pawn),0x1b8)!=reinterpret_cast<void*>(engine::image+0x3ed83b0)
           ||engine::function<void*(*)(void*)>(0x3ed83b0)(pawn)!=world){++next.invalid;continue;}
        row.links=true;row.role=Read<uint8_t>(pawn,0x224);row.actorFlags65=Read<uint8_t>(pawn,0x65);
        row.hidden=(row.actorFlags65&2)!=0;row.locallyHidden=(row.actorFlags65&4)!=0;row.initialized=Read<uint8_t>(pawn,0xa90);
        row.channel=ActorChannel(next.connection,pawn);row.root=Read<void*>(pawn,0x290);row.pawnPosition=Read<engine::Vec>(row.root,0x320);
        row.movement=Read<void*>(pawn,0x4f8);row.stream=Read<void*>(pawn,0xba8);row.movementLive=Live(row.movement);row.streamLive=Live(row.stream);
        if(row.movementLive){row.movementStream1080=Read<uint8_t>(row.movement,0x1080);row.historyData=Read<void*>(row.movement,0x1488);
            row.historyRaw1498=Read<uint32_t>(row.movement,0x1498);row.historyRaw149C=Read<uint32_t>(row.movement,0x149c);row.historyRaw14A0=Read<uint32_t>(row.movement,0x14a0);row.historyShapeKnown=true;
            if(next.managerSchema)row.registryKnown=RegistryContains(next.gsManager,row.movement,row.registryContains,row.registryHops);}
        if(row.streamLive&&Field(Read<void*>(row.stream,0x10),L"bIsProcessingStreamData",0xe0))
            row.streamProcessingE0=Read<uint8_t>(row.stream,0xe0);
        row.fog=Read<void*>(pawn,0xa68);row.fogLive=original_players::IsA(row.fog,0x6c7d4c0)&&Live(row.fog);
        if(row.fogLive){auto type=Read<void*>(row.fog,0x10);
            row.fogSchema=Field(type,L"bOwnerIsRelevantToClient",0x1e1)&&Field(type,L"bOwnerIsHiddenForClient",0x1e2)
                &&Field(type,L"bReceivedNetUpdate",0x238)&&Field(type,L"ManagerComponent",0x338)
                &&Field(type,L"OwnerCharacter",0x3a8);
            if(row.fogSchema){row.fogRelevant=Read<uint8_t>(row.fog,0x1e1)!=0;row.fogHidden=Read<uint8_t>(row.fog,0x1e2)!=0;
                row.fogReceived=Read<uint8_t>(row.fog,0x238)!=0;row.fogManager=Read<void*>(row.fog,0x338);row.fogOwner=Read<void*>(row.fog,0x3a8);}}
        auto findComponent=Read<void*>(Read<void**>(pawn),0x740);
        if(findComponent==reinterpret_cast<void*>(engine::image+0x40e6df0))row.visibility=reinterpret_cast<void*(*)(void*,void*)>(findComponent)(pawn,engine::Class(0x63ac660));
        row.visibilityLive=original_players::IsA(row.visibility,0x63ac660)&&Live(row.visibility);
        if(row.visibilityLive){auto type=Read<void*>(row.visibility,0x10);row.visibilityOwner=Read<void*>(row.visibility,0xc8);
            row.visibilitySchema=Field(type,L"bSeenByResult",0x100)&&Field(type,L"CharactersWithVisibility",0x108)&&Field(type,L"VisibilityLostTime",0x16c);
            if(row.visibilitySchema){row.seenByResult=Read<uint8_t>(row.visibility,0x100)!=0;row.visibilityCharacters=Read<int32_t>(row.visibility,0x110);
                row.visibilityCharactersCapacity=Read<int32_t>(row.visibility,0x114);row.visibilityLostTime=Read<float>(row.visibility,0x16c);}}
        row.weapon=Read<void*>(Read<void*>(pawn,0xc08),0x280);
        if(!Live(row.weapon)||Read<void*>(row.weapon,0x4a8)!=pawn)row.weapon=nullptr;
        if(row.weapon)row.weaponPosition=Read<engine::Vec>(Read<void*>(row.weapon,0x290),0x320);
        const std::array<void*,MeshCount> meshes{Read<void*>(pawn,0x4f0),Read<void*>(pawn,0xf40),Read<void*>(pawn,0xf30),Read<void*>(pawn,0xf38),Read<void*>(row.weapon,0xde0),Read<void*>(row.weapon,0xdf0)};
        for(size_t i=0;i<MeshCount;++i)SampleMesh(meshes[i],row.meshes[i]);
        next.rows[next.sampled++]=row;
    }
    next.stage="remote_render_probe_cached_actual_simulated_characters";Publish(next);return true;
}
inline bool Poll(void* world,void* pc,void* ownPawn){
    __try{return PollImpl(world,pc,ownPawn);}
    __except(EXCEPTION_EXECUTE_HANDLER){Snapshot failed{};failed.atMs=GetTickCount64();failed.error=6203;
        failed.stage="remote_render_probe_read_exception_unknown";Publish(failed);return true;}
}
inline Snapshot GetSnapshot(){AcquireSRWLockShared(&cacheLock);const auto copy=cached;ReleaseSRWLockShared(&cacheLock);return copy;}
inline void Vector(FILE* file,const engine::Vec& value){
    if(original_players::Finite(value))std::fprintf(file,"[%.9g,%.9g,%.9g]",value.x,value.y,value.z);else std::fputs("null",file);
}
inline void Write(FILE* file){
    if(!file)return;const auto s=GetSnapshot();const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"scope\":\"read_only_actual_remote_character_render\",\"stage\":\"%s\",\"error\":%u,\"profiles_verified\":%s,\"schema_verified\":%s,\"own_context\":%s,\"roster_valid\":%s,\"sampled_at_ms\":%llu,\"samples\":%llu,\"minimum_interval_ms\":100,\"maximum_remote\":29,\"sampled\":%u,\"skipped_own\":%u,\"invalid\":%u,\"truncated\":%u,\"profile_failure_rva\":%u,\"world\":\"%p\",\"controller\":\"%p\",\"own_pawn\":\"%p\",\"game_state\":\"%p\",\"connection\":\"%p\",\"pc_remote_manager\":\"%p\",\"gs_remote_manager\":\"%p\",\"pc_manager_live\":%s,\"gs_manager_live\":%s,\"same_manager\":%s,\"registry_schema\":%s,\"visibility_rpc_metadata_known\":%s,\"visibility_rpc_calls\":0,\"visibility_or_relevance_writes\":0,\"registration_writes\":0,\"material_opacity_known\":false,\"scene_proxy_layout_known\":false,\"pose_or_transform_updates_forced\":0,\"playable_verified\":false,\"characters\":[",
        s.stage,s.error,b(s.profiles),b(s.schema),b(s.ownContext),b(s.rosterValid),static_cast<unsigned long long>(s.atMs),static_cast<unsigned long long>(s.samples),s.sampled,s.skippedOwn,s.invalid,s.truncated,s.profileFailureRva,
        s.world,s.controller,s.ownPawn,s.gs,s.connection,s.pcManager,s.gsManager,b(s.pcManagerLive),b(s.gsManagerLive),b(s.managerSame),b(s.managerSchema),b(s.visibilityRpcKnown));
    constexpr const char* labels[MeshCount]{"core3p","cosmetic3p","core1p","cosmetic1p","weapon1p","weapon3p"};
    for(uint32_t n=0;n<s.sampled;++n){const auto& r=s.rows[n];if(n)std::fputc(',',file);
        std::fprintf(file,"{\"pawn\":\"%p\",\"player_state\":\"%p\",\"pawn_weak\":[%d,%d],\"state_weak\":[%d,%d],\"role\":%u,\"links\":%s,\"actor_channel\":%s,\"initialized_a90\":%u,\"actor_flags65\":%u,\"hidden\":%s,\"locally_hidden\":%s,\"pawn_position\":",
            r.pawn,r.playerState,r.pawnId.index,r.pawnId.serial,r.playerStateId.index,r.playerStateId.serial,unsigned(r.role),b(r.links),b(r.channel),unsigned(r.initialized),unsigned(r.actorFlags65),b(r.hidden),b(r.locallyHidden));Vector(file,r.pawnPosition);
        std::fprintf(file,",\"weapon\":\"%p\",\"weapon_position\":",r.weapon);Vector(file,r.weaponPosition);
        std::fprintf(file,",\"movement\":\"%p\",\"movement_live\":%s,\"movement_stream1080\":%u,\"stream\":\"%p\",\"stream_live\":%s,\"stream_processing_e0\":%u,\"registry_membership_known\":%s,\"registry_contains\":%s,\"registry_hops\":%d,\"remote_history_raw\":{\"shape_from_original_source\":%s,\"semantic_counts_known\":false,\"data1488\":\"%p\",\"value1498\":%u,\"value149c\":%u,\"value14a0\":%u},\"fog\":{\"object\":\"%p\",\"live\":%s,\"schema\":%s,\"relevant_to_client\":%s,\"hidden_for_client\":%s,\"received_net_update\":%s,\"manager\":\"%p\",\"owner_character\":\"%p\"},\"visibility\":{\"object\":\"%p\",\"live\":%s,\"schema\":%s,\"owner\":\"%p\",\"seen_by_result\":%s,\"characters\":%d,\"characters_capacity\":%d,\"lost_time_finite\":%s,\"lost_time\":%.9g},\"meshes\":[",
            r.movement,b(r.movementLive),unsigned(r.movementStream1080),r.stream,b(r.streamLive),unsigned(r.streamProcessingE0),b(r.registryKnown),b(r.registryContains),r.registryHops,b(r.historyShapeKnown),r.historyData,r.historyRaw1498,r.historyRaw149C,r.historyRaw14A0,
            r.fog,b(r.fogLive),b(r.fogSchema),b(r.fogRelevant),b(r.fogHidden),b(r.fogReceived),r.fogManager,r.fogOwner,r.visibility,b(r.visibilityLive),b(r.visibilitySchema),r.visibilityOwner,b(r.seenByResult),r.visibilityCharacters,r.visibilityCharactersCapacity,b(std::isfinite(r.visibilityLostTime)),std::isfinite(r.visibilityLostTime)?static_cast<double>(r.visibilityLostTime):0.);
        for(size_t i=0;i<MeshCount;++i){const auto& m=r.meshes[i];if(i)std::fputc(',',file);
            std::fprintf(file,"{\"kind\":\"%s\",\"object\":\"%p\",\"weak\":[%d,%d],\"live\":%s,\"typed\":%s,\"owner\":\"%p\",\"parent\":\"%p\",\"registered\":%s,\"render_state_created\":%s,\"visible\":%s,\"hidden_in_game\":%s,\"main_pass\":%s,\"owner_no_see\":%s,\"only_owner_see\":%s,\"first_person_rendering\":%s,\"flags_a0\":%u,\"flags_1d0\":%u,\"flags_3b1\":%u,\"flags_3b2\":%u,\"skeletal_mesh\":\"%p\",\"skinned_asset\":\"%p\",\"anim_class\":\"%p\",\"anim_instance\":\"%p\",\"leader\":\"%p\",\"pose_frame\":%llu,\"pose_buffer\":%d,\"pose_count\":%d,\"pose_capacity\":%d,\"pose_available\":%s,\"position\":",
                labels[i],m.object,m.identity.index,m.identity.serial,b(m.live),b(m.typed),m.owner,m.parent,b(m.registered),b(m.renderStateCreated),b(m.visible),b(m.hiddenInGame),b(m.mainPass),b(m.ownerNoSee),b(m.onlyOwnerSee),b(m.firstPerson),unsigned(m.flagsA0),unsigned(m.flags1D0),unsigned(m.flags3B1),unsigned(m.flags3B2),m.skeletalMesh,m.skinnedAsset,m.animClass,m.animInstance,m.leader,static_cast<unsigned long long>(m.poseFrame),m.poseBuffer,m.poseCount,m.poseCapacity,b(m.poseAvailable));
            Vector(file,m.position);std::fputs(",\"scale\":",file);Vector(file,m.scale);std::fputc('}',file);}
        std::fputs("]}",file);}
    std::fputs("]}",file);
}
} // namespace local_server_remote_render_probe
