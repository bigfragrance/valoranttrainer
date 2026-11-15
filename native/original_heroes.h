#pragma once
#include "original_heroes_catalog.h"
#include <array>
#include <limits>

// Missing offline match selection authority only. Original content identities,
// classes, Pawn creation, rendering, movement and ownership stay original.
// Root supplies a fresh per-match nonce-mixed seed; this allocator is pure and
// reproducible for that received seed and never touches bot ability profiles.
namespace original_heroes {
inline constexpr size_t MaxSlots=original_players::MaxPlayers;
inline constexpr uint32_t InvalidHero=(std::numeric_limits<uint32_t>::max)();
static_assert(PlayableHeroCount<=MaxSlots);
struct Assignment {bool assigned=false;uint32_t hero=InvalidHero,logicalId=0;};
struct NativeEntry {original_content::Guid id{};void* dataAsset=nullptr;int32_t hero=-1;bool playable=false,excluded=false;};
struct Diagnostics {
    bool initialized=false,runtimeCatalogVerified=false,matchAssigned=false,localJettReserved=false,restartRequired=false;
    unsigned error=0;uint32_t receivedShuffleSeed=0;size_t participants=0;
    int32_t nativeCharacterCount=0,nativePlayableCount=0,matchedPlayableCount=0,nonPlayableCount=0;
    void *world=nullptr,*contentIndex=nullptr;uintptr_t exceptionRva=0;
    original_content::Guid lastUnrecognizedGuid{};
    const char* stage="original_heroes_not_initialized";
};
inline Diagnostics diagnostics;
inline std::array<Assignment,MaxSlots> assignments{};
inline std::array<NativeEntry,original_content::MaxCharacters> nativeEntries{};
inline const Diagnostics& State(){return diagnostics;}
inline const Assignment* Player(size_t index){return index<assignments.size()&&assignments[index].assigned?&assignments[index]:nullptr;}
inline bool Thread(){return !diagnostics.restartRequired&&original_players::Thread();}
inline bool Fail(unsigned error,const char* stage){diagnostics.error=error;diagnostics.stage=stage;return false;}
inline bool SameGuid(const original_content::Guid& a,const original_content::Guid& b){return std::memcmp(&a,&b,sizeof(a))==0;}
inline int32_t CatalogIndex(const original_content::Guid& id){
    for(size_t i=0;i<PlayableHeroCount;++i)if(SameGuid(id,Catalog[i].id))return static_cast<int32_t>(i);return -1;
}
inline bool InitializeImpl(){
    if(!Thread())return false;const auto& content=original_content::State();
    if(!content.initialized||!content.characterIdentitiesVerified||content.characterCount<=0
       ||content.characterCount>static_cast<int32_t>(nativeEntries.size()))return Fail(2780,"original_heroes_verified_original_content_index_required");
    if(diagnostics.initialized)return diagnostics.world==original_players::world&&diagnostics.contentIndex==content.index
        ?true:Fail(2780,"original_heroes_world_or_native_content_index_changed_requires_new_session");
    diagnostics.world=original_players::world;diagnostics.contentIndex=content.index;
    diagnostics.nativeCharacterCount=content.characterCount;diagnostics.nativePlayableCount=diagnostics.matchedPlayableCount=diagnostics.nonPlayableCount=0;
    std::array<bool,PlayableHeroCount> seen{};
    for(int32_t i=0;i<content.characterCount;++i){
        const auto& actual=original_content::characters[i];auto& entry=nativeEntries[static_cast<size_t>(i)];entry={};
        entry.dataAsset=actual.dataAsset;entry.id=actual.id;
        if(!actual.found||!original_players::IsA(entry.dataAsset,original_content::CharacterDataClass)
           ||!original_content::Valid(entry.id))return Fail(2781,"original_heroes_original_character_identity_unverified");
        uint8_t playable=0,excluded=0;
        if(!original_players::Read(entry.dataAsset,0x1d0,playable)||!original_players::Read(entry.dataAsset,0x1d1,excluded))
            return Fail(2781,"original_heroes_original_playable_flags_unreadable");
        entry.playable=playable!=0;entry.excluded=excluded!=0;entry.hero=CatalogIndex(entry.id);
        if(!entry.playable||entry.excluded){++diagnostics.nonPlayableCount;continue;}
        ++diagnostics.nativePlayableCount;
        if(entry.hero<0){diagnostics.lastUnrecognizedGuid=entry.id;return Fail(2782,"original_heroes_unrecognized_original_playable_guid");}
        const size_t hero=static_cast<size_t>(entry.hero);
        if(seen[hero])return Fail(2782,"original_heroes_duplicate_original_playable_guid");
        seen[hero]=true;++diagnostics.matchedPlayableCount;
    }
    if(diagnostics.nativePlayableCount!=static_cast<int32_t>(PlayableHeroCount)||diagnostics.matchedPlayableCount!=static_cast<int32_t>(PlayableHeroCount))
        return Fail(2783,"original_heroes_runtime_playable_catalog_does_not_match_manifest");
    for(bool present:seen)if(!present)return Fail(2783,"original_heroes_pinned_playable_hero_missing_from_native_index");
    diagnostics.initialized=diagnostics.runtimeCatalogVerified=true;diagnostics.error=0;
    diagnostics.stage="original_heroes_original_playable_catalog_verified";return true;
}
inline LONG Capture(EXCEPTION_POINTERS* e){
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=at>=engine::image?at-engine::image:at;diagnostics.restartRequired=true;
    Fail(e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0002780U,"original_heroes_native_exception_requires_new_session");
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(){__try{return InitializeImpl();}__except(Capture(GetExceptionInformation())){return false;}}
inline uint32_t NextRandom(uint32_t& seed){
    seed+=0x9e3779b9u;uint32_t value=seed;value=(value^(value>>16))*0x85ebca6bu;
    value=(value^(value>>13))*0xc2b2ae35u;return value^(value>>16);
}
inline uint32_t BoundedRandom(uint32_t& seed,uint32_t bound){
    const uint32_t threshold=(0u-bound)%bound;uint32_t value=0;
    do{value=NextRandom(seed);}while(value<threshold);return value%bound;
}
inline bool BeginUniqueMatchImpl(size_t totalPlayers,uint32_t shuffleSeed){
    if(!Thread()||totalPlayers<2||totalPlayers>PlayableHeroCount||totalPlayers>assignments.size())
        return Fail(2784,"original_heroes_unique_match_requires_2_to_catalog_max_total_players");
    if(diagnostics.matchAssigned)return diagnostics.participants==totalPlayers&&diagnostics.receivedShuffleSeed==shuffleSeed
        ?true:Fail(2785,"original_heroes_match_allocation_is_immutable_start_new_session");
    if(!Initialize())return false;
    if(original_players::localIndex!=0||original_players::players[0].character!=original_players::activePawn)
        return Fail(2786,"original_heroes_original_local_jett_must_occupy_roster_slot_zero");
    auto localClass=original_players::Value<void*>(original_players::activePawn,0x10);
    const auto localIdentity=original_content::FindCharacterByClass(localClass);
    if(!localIdentity.found||!SameGuid(localIdentity.id,Catalog[0].id))return Fail(2786,"original_heroes_actual_local_jett_required");
    std::array<uint32_t,PlayableHeroCount-1> pool{};
    for(size_t n=0;n<pool.size();++n)pool[n]=static_cast<uint32_t>(n+1); // Never NPC Jett.
    uint32_t random=shuffleSeed;
    for(size_t n=pool.size();n>1;--n){const size_t other=BoundedRandom(random,static_cast<uint32_t>(n));
        const auto temporary=pool[n-1];pool[n-1]=pool[other];pool[other]=temporary;}
    assignments[0]={true,0,0};
    for(size_t index=1;index<totalPlayers;++index)assignments[index]={true,pool[index-1],static_cast<uint32_t>(index)};
    diagnostics.participants=totalPlayers;diagnostics.receivedShuffleSeed=shuffleSeed;
    diagnostics.localJettReserved=diagnostics.matchAssigned=true;diagnostics.error=0;
    diagnostics.stage="original_heroes_unique_match_allocation_fixed_until_session_end";return true;
}
inline bool BeginUniqueMatch(size_t totalPlayers,uint32_t shuffleSeed){
    __try{return BeginUniqueMatchImpl(totalPlayers,shuffleSeed);}__except(Capture(GetExceptionInformation())){return false;}
}
inline uint32_t AssignedHero(size_t index){const auto assignment=Player(index);return assignment?assignment->hero:InvalidHero;}
inline uint32_t AssignedLogicalId(size_t index){const auto assignment=Player(index);return assignment?assignment->logicalId:0;}
inline const wchar_t* AssignedPath(size_t index){const auto hero=AssignedHero(index);return hero<PlayableHeroCount?Paths[hero]:nullptr;}
inline bool ResolveMatchLogicalId(uint32_t logicalId,uint32_t& hero){
    if(!diagnostics.matchAssigned||logicalId>=diagnostics.participants)return false;
    const auto& assignment=assignments[logicalId];if(!assignment.assigned||assignment.logicalId!=logicalId)return false;
    hero=assignment.hero;return true;
}
inline bool ValidateAssignedClass(size_t index,void* loadedClass){
    if(!Thread()||!diagnostics.matchAssigned)return false;const auto assigned=Player(index);
    if(!assigned)return Fail(2787,"original_heroes_roster_slot_not_allocated_for_match");
    const auto identity=original_content::FindCharacterByClass(loadedClass);
    return identity.found&&SameGuid(identity.id,Catalog[assigned->hero].id)?true:Fail(2787,"original_heroes_actual_class_differs_from_fixed_match_assignment");
}
inline void Guid(FILE* file,const original_content::Guid& id){
    std::fprintf(file,"\"%08x%08x%08x%08x\"",id.words[0],id.words[1],id.words[2],id.words[3]);
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    const auto b=[](bool value){return value?"true":"false";};
    std::fputs("{\"schema_version\":1,\"stage\":",file);original_players::Text(file,diagnostics.stage);
    std::fprintf(file,",\"error\":%u,\"initialized\":%s,\"runtime_catalog_verified\":%s,\"playable_count\":%zu,\"native_character_count\":%d,\"native_playable_count\":%d,\"matched_playable_count\":%d,\"non_playable_count\":%d,\"match_assigned\":%s,\"participants\":%zu,\"received_shuffle_seed\":%u,\"local_jett_reserved\":%s,\"restart_required\":%s,\"exception_rva\":%llu,\"last_unrecognized_guid\":",
        diagnostics.error,b(diagnostics.initialized),b(diagnostics.runtimeCatalogVerified),PlayableHeroCount,diagnostics.nativeCharacterCount,
        diagnostics.nativePlayableCount,diagnostics.matchedPlayableCount,diagnostics.nonPlayableCount,b(diagnostics.matchAssigned),diagnostics.participants,
        diagnostics.receivedShuffleSeed,b(diagnostics.localJettReserved),b(diagnostics.restartRequired),static_cast<unsigned long long>(diagnostics.exceptionRva));
    Guid(file,diagnostics.lastUnrecognizedGuid);std::fputs(",\"assignments\":[",file);
    bool comma=false;for(size_t index=0;index<assignments.size();++index){const auto& assignment=assignments[index];if(!assignment.assigned)continue;
        std::fprintf(file,"%s{\"index\":%zu,\"hero_index\":%u,\"logical_id\":%u,\"name\":",comma?",":"",index,assignment.hero,assignment.logicalId);
        const auto label=original_players::Utf8(Names[assignment.hero]);original_players::Text(file,label.c_str());
        std::fputs(",\"class\":",file);const auto heroPath=original_players::Utf8(Paths[assignment.hero]);original_players::Text(file,heroPath.c_str());
        std::fputs(",\"id\":",file);Guid(file,Catalog[assignment.hero].id);std::fputc('}',file);comma=true;
    }
    std::fputs("],\"native_entries\":[",file);
    for(int32_t index=0;index<diagnostics.nativeCharacterCount;++index){const auto& entry=nativeEntries[static_cast<size_t>(index)];
        std::fprintf(file,"%s{\"index\":%d,\"data_asset\":\"%p\",\"playable\":%s,\"excluded\":%s,\"hero_index\":%d,\"id\":",
            index?",":"",index,entry.dataAsset,b(entry.playable),b(entry.excluded),entry.hero);Guid(file,entry.id);std::fputc('}',file);
    }
    std::fputs("],\"npc_jett_allowed\":false,\"unique_per_match\":true,\"reassign_on_death\":false,\"all_character_spawn_runtime_verified\":false}\n",file);
    const bool okay=std::ferror(file)==0;std::fclose(file);return okay;
}
}
