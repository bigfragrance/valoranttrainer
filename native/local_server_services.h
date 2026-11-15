#pragma once
#include "original_hud.h"
#include "original_players.h"

// Pinned local network-session services only. Evidence and factory disassembly:
// work/local-server-playable/shooter-host/. Original authority/client builds
// remain separate; the normal online InitSystem graph is never invoked here.
namespace local_server_services {
inline constexpr uintptr_t PerfManagerClass=0x6a61370;
inline constexpr uintptr_t PerfMetricClass=0x6a61160;
inline constexpr uintptr_t PresenceManagerClass=0x6b19d80;
inline constexpr uintptr_t DisplayNameManagerClass=0x671c230;
inline constexpr uintptr_t EnginePlayerStateClass=0x42aae70;
inline constexpr uintptr_t PlatformPlayerClass=0x6a525f0;
inline constexpr uintptr_t PregameManagerClass=0x6a974e0;
inline constexpr uintptr_t CoreGameManagerClass=0x66d3420;
inline constexpr int32_t MetricCount=47;
inline constexpr size_t OfflineRosterCount=32;
struct Diagnostics {
    bool ready=false,host=false,profilesVerified=false,factoryInvoked=false;
    bool managerLinked=false,metricArrayVerified=false,nativeInitialized=false;
    bool localNameProvidersReady=false,offlineRosterReady=false,onlineNameServicesAbsent=false;
    bool localMatchContextsReady=false,pregameReused=false,coreGameCreated=false;
    bool idleMatchContexts=false,matchContextReferencesVerified=false,matchContextServicesAbsent=false;
    bool restartRequired=false;
    unsigned error=0;
    uintptr_t exceptionRva=0;
    const char* stage="local_network_services_not_started";
    const char* operation="none";
    void *world=nullptr,*gameInstance=nullptr,*clientInstance=nullptr;
    void *settingsManager=nullptr,*originalOwnerF8=nullptr,*performanceManager=nullptr;
    void *platformPlayerManager=nullptr,*presenceManager=nullptr,*displayNameManager=nullptr;
    int32_t metricCount=0,metricCapacity=0,verifiedMetricIds=0;
    uint32_t offlineNameCount=0,nativeNameGetterReadbacks=0;
    uint32_t uniqueIdKeyReads=0,emptyUniqueIdReads=0;
    int32_t lastUniqueIdKeyCharacters=0;
    uint32_t platformModelsCreated=0,platformCacheReadbacks=0;
    uint32_t idleMatchRefreshChecks=0,cacheRetainedAcrossMatchRefresh=0;
    bool platformStrongReferenceVerified=false;
    void* lastPlatformPlayer=nullptr;
    void *preloadManager=nullptr,*pregameManager=nullptr,*coreGameManager=nullptr;
};
inline Diagnostics state;
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* p,size_t o){return original_players::Value<T>(p,o);}
inline bool Fail(unsigned code,const char* reason){
    state.ready=false;state.offlineRosterReady=false;state.error=code;state.stage=reason;return false;
}

inline bool VerifyProfiles(){
    struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
    static constexpr Profile profiles[]{
        {0x702a7f0,504,0x4f62219516944fb6ULL}, // original complete local factory
        {0x6a692d0,129,0x4e6a02a1d3764b1bULL}, // manager constructor
        {0x6a69220,167,0xf72d9eb12b883068ULL}, // metric constructor
        {0x6a68a20,140,0x68e66e32d0f81fdaULL}, // first local sample ring
        {0x6a68980,148,0x7d0c81b2cd6a53e8ULL}, // second local sample ring
        {0x6b1e0e0,296,0x30ddfcba65f06611ULL}, // real empty presence cache constructor
        {0x6725710,215,0xc04851ed40b6f0ddULL}, // real empty display-name cache constructor
        {0x7355590,789,0xb26f128e55040025ULL}, // original cache insert/copy helper
        {0x1a1cb20,251,0xdfab83b9981e4119ULL}, // original local cache lookup
        {0x18bfd80,110,0xfb2c5760889503bdULL}, // original FString key hash
        {0x18bfea0,99,0xe0d0bf558435213bULL}, // original FString copy constructor
        {0x1b0e200,173,0x0422bd79eba1ba3eULL}, // original three-string value destructor
        {0x72ce060,348,0xbbf3e857851043ebULL}, // original display-name value constructor
        {0x72ce1c0,1348,0x6b7134ea801937eeULL}, // original getter, called only after cache-hit proof
        {0x3dbe420,271,0x3a0cf0e98c03a0eeULL}, // actual UniqueNetIdRepl string conversion
        {0x18a2f10,61,0xe7b90b4f44d3a19fULL}, // destructor for its owned FString result
        {0x74fce90,558,0xe9022c8f462ee00dULL}, // original local PlatformPlayer factory
        {0x6a55b10,1972,0xdb7a08f1d942d3a0ULL}, // its original local constructor/defaults
        {0x74fd0c0,214,0x19b0567f0e835315ULL}, // original display-name setter/delegates
        {0x1ae75b0,658,0xe45f6d4e78fa34b7ULL}, // original weak-player map insert/copy
        {0x43c92d0,120,0x4c8ec08900070dd7ULL}, // real GameInstance strong-reference AddUnique
        {0x1d4c320,1561,0x468c168a4c305e17ULL}, // original weak validity check
        {0x1d4cfa0,1562,0xb050d7efb4cf1a25ULL}, // original weak object resolution
        {0x75003a0,1551,0xf74cd4ec7311e512ULL}, // original player getter, verified cache-hit only
        {0x1cf35b0,1571,0x40614eae57888396ULL}, // actual GUObjectArray serial lookup
        {0x18d0910,253,0x8714dc8d0823d72dULL}, // original string/weak-player map lookup
        {0x7011b60,1572,0xffb0029beb648653ULL}, // factory's world/client GameFlow accessor
        {0x6aa4ac0,220,0x6b77cdda12a14f00ULL}, // complete original idle PregameManager constructor
        {0x66e14f0,286,0xf179fc26be7b0d3aULL}, // complete original idle CoreGameManager constructor
        {0x7505010,242,0xf65a44f42cafe2c1ULL}, // original refresh, empty pregame snapshot returns normally
        {0x7505110,1557,0x7b7fdc088a52b1bbULL}, // original refresh, empty core-game snapshot returns normally
        {0x6a97b40,568,0x6b4cca5a6be53a50ULL}, // full empty native pregame snapshot value constructor
        {0x6a97d80,1246,0x64f6d0ad2611db08ULL}, // matching local snapshot destructor
    };
    for(const auto& p:profiles){
        const auto* bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        uint64_t hash=14695981039346656037ULL;
        for(size_t i=0;i<p.bytes;++i)hash=(hash^bytes[i])*1099511628211ULL;
        if(hash!=p.fnv)return false;
    }
    return true;
}

struct NativeString {wchar_t* data=nullptr;int32_t count=0,capacity=0;};
struct NativeDisplayName {NativeString full{},game{},tag{};};
struct CacheInput {const NativeString* key=nullptr;const NativeDisplayName* value=nullptr;};
struct NativeWeakObject {int32_t index=-1,serial=0;};
struct PlayerCacheInput {const NativeString* key=nullptr;const NativeWeakObject* value=nullptr;};
static_assert(sizeof(NativeString)==16&&sizeof(NativeDisplayName)==48&&sizeof(CacheInput)==16);
static_assert(sizeof(NativeWeakObject)==8&&sizeof(PlayerCacheInput)==16);
struct OfflineName {
    wchar_t key[128]{},game[96]{},tag[24]{};
    void* platformPlayer=nullptr;
};
inline OfflineName offlineNames[OfflineRosterCount]{};
inline bool VerifyOfflineMatchContextRefresh();

inline NativeString Borrow(const wchar_t* value){
    if(!value||!*value)return {};
    const int32_t size=static_cast<int32_t>(std::wcslen(value))+1;
    return {const_cast<wchar_t*>(value),size,size};
}
inline bool EqualText(const NativeString& value,const wchar_t* text){
    if(!text)return value.count<=1;
    const size_t size=std::wcslen(text);
    if(!size)return value.count<=1;
    if(!value.data||value.count!=static_cast<int32_t>(size+1)||value.capacity<value.count)return false;
    for(size_t i=0;i<=size;++i)if(Read<wchar_t>(value.data,i*sizeof(wchar_t))!=text[i])return false;
    return true;
}
inline int32_t FindCachedName(const NativeString& key){
    if(!key.data||key.count<2||key.capacity<key.count)return -1;
    const auto hash=engine::function<uint32_t(*)(int32_t,const wchar_t*)>(0x18bfd80)(key.count-1,key.data);
    return engine::function<int32_t(*)(void*,uint32_t,const NativeString*)>(0x1a1cb20)
        (static_cast<uint8_t*>(state.displayNameManager)+0xa8,hash,&key);
}
inline bool CachedNameEquals(const NativeString& key,const NativeDisplayName& expected){
    const int32_t index=FindCachedName(key);
    const int32_t physicalCount=Read<int32_t>(state.displayNameManager,0xb0);
    auto entries=Read<uint8_t*>(state.displayNameManager,0xa8);
    if(index<0||index>=physicalCount||physicalCount>4096||!entries)return false;
    auto value=entries+static_cast<size_t>(index)*0x48+0x10;
    return EqualText(Read<NativeString>(value,0),expected.full.data)
        &&EqualText(Read<NativeString>(value,0x10),expected.game.data)
        &&EqualText(Read<NativeString>(value,0x20),expected.tag.data);
}
inline bool StronglyReferenced(void* object){
    const int32_t count=Read<int32_t>(state.gameInstance,0x60);
    const int32_t capacity=Read<int32_t>(state.gameInstance,0x64);
    auto values=Read<void**>(state.gameInstance,0x58);
    if(!object||count<0||count>65536||capacity<count||(count&&!values))return false;
    for(int32_t i=0;i<count;++i)
        if(Read<void*>(values,static_cast<size_t>(i)*sizeof(void*))==object)return true;
    return false;
}
inline void* FindCachedPlatformPlayer(const NativeString& key){
    if(!state.platformPlayerManager||!key.data||key.count<2)return nullptr;
    const auto hash=engine::function<uint32_t(*)(int32_t,const wchar_t*)>(0x18bfd80)(key.count-1,key.data);
    const int32_t index=engine::function<int32_t(*)(void*,uint32_t,const NativeString*)>(0x18d0910)
        (static_cast<uint8_t*>(state.platformPlayerManager)+0x140,hash,&key);
    const int32_t physicalCount=Read<int32_t>(state.platformPlayerManager,0x148);
    auto entries=Read<uint8_t*>(state.platformPlayerManager,0x140);
    if(index<0||index>=physicalCount||physicalCount>4096||!entries)return nullptr;
    auto weak=entries+static_cast<size_t>(index)*0x20+0x10;
    if(!engine::function<bool(*)(void*)>(0x1d4c320)(weak))return nullptr;
    return engine::function<void*(*)(void*)>(0x1d4cfa0)(weak);
}
inline bool PlatformPlayerMatches(void* player,const NativeString& key,const NativeDisplayName& expected){
    return original_players::IsA(player,PlatformPlayerClass)
        &&Read<void*>(player,0x28)==state.platformPlayerManager
        &&Read<uint8_t>(player,0x4a0)==0
        &&EqualText(Read<NativeString>(player,0x4a8),key.data)
        &&EqualText(Read<NativeString>(player,0x4b8),expected.full.data)
        &&EqualText(Read<NativeString>(player,0x4c8),expected.game.data)
        &&EqualText(Read<NativeString>(player,0x4d8),expected.tag.data);
}
inline bool EnsureCachedPlatformPlayer(const NativeString& key,const NativeDisplayName& name,void*& result){
    auto player=FindCachedPlatformPlayer(key);
    if(player&&!PlatformPlayerMatches(player,key,name))
        return Fail(3333,"local_network_existing_platform_player_cache_conflict");
    if(!player){
        state.operation="original_local_platform_player_factory_74fce90";
        // With the actual original RSO identity empty and a nonempty native
        // subject key, the original7500497 comparison yields accountSelf=false.
        // This is independent of UPlayer/PC/Pawn network ownership.
        auto rso=Read<void*>(state.clientInstance,0x750);
        if(!original_players::IsA(rso,0x6bd2cb0)||Read<int32_t>(rso,0x180)!=0)
            return Fail(3334,"local_network_platform_player_requires_empty_original_identity");
        NativeString ownedKey{};
        engine::function<NativeString*(*)(NativeString*,const NativeString*)>(0x18bfea0)(&ownedKey,&key);
        // Original factory owns/consumes this native string copy. It constructs
        // the real object, records its real manager Outer and subject, leaves
        // account flags at original defaults, and reads the existing GameFlow
        // reference without starting a state machine or an online service.
        player=engine::function<void*(*)(void*,NativeString*,bool)>(0x74fce90)
            (state.platformPlayerManager,&ownedKey,false);
        if(!original_players::IsA(player,PlatformPlayerClass)
           ||Read<void*>(player,0x28)!=state.platformPlayerManager
           ||Read<uint8_t>(player,0x4a0)!=0||!EqualText(Read<NativeString>(player,0x4a8),key.data))
            return Fail(3335,"local_network_original_platform_player_factory_incomplete");
        ++state.platformModelsCreated;
        engine::function<void(*)(void*,const NativeDisplayName*)>(0x74fd0c0)(player,&name);
        if(!PlatformPlayerMatches(player,key,name))
            return Fail(3336,"local_network_original_platform_player_name_readback_failed");
    }
    // PlayerCache is a weak-object map. Keep the real object alive through the
    // persistent GameInstance's reflected STRONG ObjectProperty array. The
    // original AddUnique preserves every preexisting reference and travels with
    // this GI; no synthetic GC flags or bare static pointer substitutes a root.
    if(!StronglyReferenced(player))
        engine::function<void(*)(void*,void*)>(0x43c92d0)(state.gameInstance,player);
    state.platformStrongReferenceVerified=StronglyReferenced(player);
    if(!state.platformStrongReferenceVerified)
        return Fail(3337,"local_network_platform_player_strong_reference_not_observed");
    if(!FindCachedPlatformPlayer(key)){
        NativeWeakObject identity;
        identity.index=Read<int32_t>(player,0xc);
        if(identity.index<0)return Fail(3338,"local_network_platform_player_object_index_invalid");
        identity.serial=engine::function<int32_t(*)(void*,int32_t)>(0x1cf35b0)
            (reinterpret_cast<void*>(engine::image+0xd7b87c0),identity.index);
        if(identity.serial<=0)return Fail(3339,"local_network_platform_player_native_serial_invalid");
        const PlayerCacheInput input{&key,&identity};int32_t inserted=-1;
        engine::function<void*(*)(void*,int32_t*,const PlayerCacheInput*)>(0x1ae75b0)
            (static_cast<uint8_t*>(state.platformPlayerManager)+0x140,&inserted,&input);
    }
    if(FindCachedPlatformPlayer(key)!=player||!StronglyReferenced(player))
        return Fail(3340,"local_network_platform_player_weak_cache_readback_failed");
    // The same original getter used by717e293 must return this rooted model.
    // Its const-reference key is borrowed, unlike the factory's owned argument.
    // Valid original weak-cache membership is proven immediately beforehand on
    // the same game thread, so the original early return avoids all miss-side
    // CoreGame/Pregame/party/SDK refresh work while OnRep remains unchanged.
    auto observed=engine::function<void*(*)(void*,const NativeString*)>(0x75003a0)
        (state.platformPlayerManager,&key);
    if(observed!=player||!PlatformPlayerMatches(observed,key,name))
        return Fail(3341,"local_network_original_platform_player_getter_readback_failed");
    // ShooterPS.BeginPlay calls these same original refreshes. Prove their
    // normal empty-match paths preserve the actual-ID weak cache before ACK.
    if(!VerifyOfflineMatchContextRefresh())return false;
    if(FindCachedPlatformPlayer(key)!=player||!StronglyReferenced(player))
        return Fail(3350,"local_network_platform_cache_not_retained_by_idle_match_refresh");
    ++state.cacheRetainedAcrossMatchRefresh;
    ++state.platformCacheReadbacks;state.lastPlatformPlayer=player;result=player;
    state.operation="none";return true;
}
inline bool BindReference(void* owner,size_t offset,void* value,uintptr_t type){
    if(!owner||!original_players::IsA(value,type))return Fail(3315,"local_network_name_provider_type_mismatch");
    auto previous=Read<void*>(owner,offset);
    if(previous&&previous!=value)return Fail(3316,"local_network_name_provider_reference_conflict");
    if((!previous&&!original_players::Write(owner,offset,value))||Read<void*>(owner,offset)!=value)
        return Fail(3317,"local_network_name_provider_reference_readback_failed");
    return true;
}
inline bool InspectOfflineMatchContexts(){
    state.idleMatchContexts=original_players::IsA(state.pregameManager,PregameManagerClass)
        &&original_players::IsA(state.coreGameManager,CoreGameManagerClass)
        &&Read<void*>(state.pregameManager,0x28)==state.clientInstance
        &&Read<void*>(state.coreGameManager,0x28)==state.clientInstance
        &&Read<uint8_t>(state.pregameManager,0x450)==0
        &&Read<uint8_t>(state.coreGameManager,0x210)==0;
    state.matchContextReferencesVerified=Read<void*>(state.clientInstance,0x4b0)==state.pregameManager
        &&Read<void*>(state.clientInstance,0x390)==state.coreGameManager
        &&Read<void*>(state.preloadManager,0x6b0)==state.pregameManager
        &&Read<void*>(state.preloadManager,0x6c0)==state.coreGameManager
        &&Read<void*>(state.platformPlayerManager,0xf8)==state.pregameManager
        &&Read<void*>(state.platformPlayerManager,0xe8)==state.coreGameManager;
    state.matchContextServicesAbsent=true;
    constexpr size_t pregameOnline[]{0xe0,0xf0,0xf8,0x100,0x108,0x110,0x118};
    constexpr size_t coreGameOnline[]{0x2a0,0x2a8,0x2b0,0x2b8,0x2c0,0x2c8,0x2d0};
    for(auto offset:pregameOnline)if(Read<void*>(state.pregameManager,offset))state.matchContextServicesAbsent=false;
    for(auto offset:coreGameOnline)if(Read<void*>(state.coreGameManager,offset))state.matchContextServicesAbsent=false;
    return state.idleMatchContexts&&state.matchContextReferencesVerified&&state.matchContextServicesAbsent;
}
inline bool VerifyOfflineMatchContextRefresh(){
    if(!state.localMatchContextsReady||!InspectOfflineMatchContexts())
        return Fail(3351,"local_network_original_idle_match_context_contract_changed");
    struct CacheStorage {uint8_t bytes[0x50];};
    const auto before=Read<CacheStorage>(state.platformPlayerManager,0x140);
    state.operation="original_platform_refresh_empty_pregame_7505010";
    engine::function<void(*)(void*)>(0x7505010)(state.platformPlayerManager);
    state.operation="original_platform_refresh_empty_core_game_7505110";
    engine::function<void(*)(void*)>(0x7505110)(state.platformPlayerManager);
    const auto after=Read<CacheStorage>(state.platformPlayerManager,0x140);
    if(std::memcmp(&before,&after,sizeof(before))!=0||!InspectOfflineMatchContexts())
        return Fail(3352,"local_network_idle_match_refresh_changed_player_cache_or_context");
    ++state.idleMatchRefreshChecks;state.operation="none";return true;
}
inline bool ResolveLocalMatchContext(size_t clientOffset,size_t preloadOffset,size_t platformOffset,
                                     uintptr_t type,void*& result,bool& created){
    void* candidates[]{Read<void*>(state.preloadManager,preloadOffset),
        Read<void*>(state.clientInstance,clientOffset),Read<void*>(state.platformPlayerManager,platformOffset)};
    result=nullptr;
    for(auto candidate:candidates)if(candidate){
        if(!original_players::IsA(candidate,type)||Read<void*>(candidate,0x28)!=state.clientInstance)
            return Fail(3353,"local_network_existing_match_context_type_or_owner_mismatch");
        if(result&&result!=candidate)return Fail(3354,"local_network_existing_match_context_reference_conflict");
        result=candidate;
    }
    if(!result){
        // Standard native SpawnObject invokes the whole pinned constructor.
        // Both constructors' only callee is the original UObject constructor;
        // no InitSystem, online service, account or has-data flag is invoked.
        result=original_hud::Construct(engine::Class(type),state.clientInstance);
        created=true;
    }
    return original_players::IsA(result,type)&&Read<void*>(result,0x28)==state.clientInstance
        ?true:Fail(3355,"local_network_original_match_context_construction_failed");
}
inline bool PrepareOfflineMatchContexts(){
    state.stage="local_network_bind_original_idle_match_contexts";
    state.preloadManager=Read<void*>(state.clientInstance,0x3f0);
    if(!original_players::IsA(state.preloadManager,0x6aa5740)
       ||Read<void*>(state.preloadManager,0x28)!=state.clientInstance)
        return Fail(3356,"local_network_original_preload_manager_required_for_match_context");
    bool pregameCreated=false,coreCreated=false;
    if(!ResolveLocalMatchContext(0x4b0,0x6b0,0xf8,PregameManagerClass,state.pregameManager,pregameCreated)
       ||!ResolveLocalMatchContext(0x390,0x6c0,0xe8,CoreGameManagerClass,state.coreGameManager,coreCreated))return false;
    state.pregameReused=!pregameCreated;state.coreGameCreated=state.coreGameCreated||coreCreated;
    if(Read<uint8_t>(state.pregameManager,0x450)!=0||Read<uint8_t>(state.coreGameManager,0x210)!=0)
        return Fail(3357,"local_network_match_context_requires_original_empty_snapshots");
    auto config=Read<void*>(state.clientInstance,0x748);
    if(!BindReference(state.pregameManager,0xe8,config,0x66596c0)
       ||!BindReference(state.coreGameManager,0x298,config,0x66596c0)
       ||!BindReference(state.clientInstance,0x4b0,state.pregameManager,PregameManagerClass)
       ||!BindReference(state.clientInstance,0x390,state.coreGameManager,CoreGameManagerClass)
       ||!BindReference(state.preloadManager,0x6b0,state.pregameManager,PregameManagerClass)
       ||!BindReference(state.preloadManager,0x6c0,state.coreGameManager,CoreGameManagerClass)
       ||!BindReference(state.platformPlayerManager,0xf8,state.pregameManager,PregameManagerClass)
       ||!BindReference(state.platformPlayerManager,0xe8,state.coreGameManager,CoreGameManagerClass))return false;
    // These are actual reflected ObjectProperties, so each manager is strongly
    // reachable through the original ClientGI/preload/platform service graph.
    // Appearance-context preparation reuses the identical Preload+6b0 object.
    if(!InspectOfflineMatchContexts())return Fail(3358,"local_network_idle_match_context_readback_failed");
    state.localMatchContextsReady=true;
    return VerifyOfflineMatchContextRefresh();
}
inline bool PrepareLocalNameProviders(){
    state.platformPlayerManager=Read<void*>(state.clientInstance,0x408);
    if(!original_players::IsA(state.platformPlayerManager,0x6a56d90))
        return Fail(3318,"local_network_platform_player_manager_required");
    state.stage="local_network_constructing_original_local_name_caches";
    // Both exact constructors initialize UObject, empty maps/delegates and
    // ordinary local fields only. Do not construct Presence/DisplayName SDKs,
    // invoke InitSystem, populate accounts, or alter their initialized flags.
    if(!original_hud::EnsureService(0x340,PresenceManagerClass)
       ||!original_hud::EnsureService(0x370,DisplayNameManagerClass))
        return Fail(3319,"local_network_local_name_cache_construction_failed");
    state.presenceManager=Read<void*>(state.clientInstance,0x340);
    state.displayNameManager=Read<void*>(state.clientInstance,0x370);
    if(!BindReference(state.platformPlayerManager,0x90,state.presenceManager,PresenceManagerClass)
       ||!BindReference(state.platformPlayerManager,0x98,state.displayNameManager,DisplayNameManagerClass))return false;
    state.onlineNameServicesAbsent=!Read<void*>(state.presenceManager,0x138)
        &&!Read<void*>(state.displayNameManager,0x90);
    if(!state.onlineNameServicesAbsent)
        return Fail(3320,"local_network_name_cache_requires_absent_online_services");
    auto rso=Read<void*>(state.clientInstance,0x750);
    if(!original_players::IsA(rso,0x6bd2cb0)||Read<int32_t>(rso,0x160)!=0
       ||Read<int32_t>(rso,0x170)!=0||Read<int32_t>(rso,0x180)!=0||Read<int32_t>(rso,0x190)!=0)
        return Fail(3321,"local_network_name_cache_requires_empty_original_identity");
    state.localNameProvidersReady=true;
    return true;
}

inline bool CacheOfflineSubjectImpl(const wchar_t* key,const wchar_t* game,const wchar_t* tag){
    if(state.restartRequired||state.error||!original_hud::Thread())return false;
    if(!state.localNameProvidersReady||!key||!*key||!game||!*game||!tag||!*tag
       ||std::wcslen(key)>=128||std::wcslen(game)>=96||std::wcslen(tag)>=24)
        return Fail(3322,"local_network_offline_name_arguments_invalid");
    size_t existing=state.offlineNameCount;
    for(size_t i=0;i<state.offlineNameCount;++i){
        if(_wcsicmp(offlineNames[i].key,key)==0){
            if(std::wcscmp(offlineNames[i].key,key)!=0||std::wcscmp(offlineNames[i].game,game)!=0
               ||std::wcscmp(offlineNames[i].tag,tag)!=0)
                return Fail(3323,"local_network_offline_name_key_conflict");
            existing=i;
        }else if(std::wcscmp(offlineNames[i].game,game)==0&&std::wcscmp(offlineNames[i].tag,tag)==0)
            return Fail(3324,"local_network_offline_display_names_must_be_distinct");
    }
    if(existing==state.offlineNameCount&&existing>=OfflineRosterCount)
        return Fail(3325,"local_network_offline_name_roster_full");
    state.stage="local_network_populating_original_display_name_cache";
    state.operation="original_local_display_name_cache_7355590";
    auto keyValue=Borrow(key),gameValue=Borrow(game),tagValue=Borrow(tag);
    NativeDisplayName value{};
    engine::function<NativeDisplayName*(*)(NativeDisplayName*,const NativeString*,const NativeString*)>(0x72ce060)
        (&value,&gameValue,&tagValue);
    const int32_t previous=FindCachedName(keyValue);
    if(previous>=0&&!CachedNameEquals(keyValue,value)){
        engine::function<void(*)(NativeDisplayName*)>(0x1b0e200)(&value);
        return Fail(3326,"local_network_existing_display_name_cache_conflict");
    }
    if(previous<0){
        const CacheInput input{&keyValue,&value};int32_t inserted=-1;
        // Exact native72ce62e..64a ABI. The helper copies all four strings into
        // the real typed map; it never receives an account or a callback.
        engine::function<void*(*)(void*,int32_t*,const CacheInput*)>(0x7355590)
            (static_cast<uint8_t*>(state.displayNameManager)+0xa8,&inserted,&input);
    }
    if(!CachedNameEquals(keyValue,value)){
        engine::function<void(*)(NativeDisplayName*)>(0x1b0e200)(&value);
        return Fail(3327,"local_network_offline_display_name_cache_readback_failed");
    }
    // Prove cache hit with the ORIGINAL getter. Its FString-by-value argument
    // is copied through the native constructor and consumed by the getter.
    // Cache membership was checked immediately above on this same game thread,
    // so this call returns before every friend/provider/cache-miss branch.
    NativeString ownedKey{};NativeDisplayName result{};
    engine::function<NativeString*(*)(NativeString*,const NativeString*)>(0x18bfea0)(&ownedKey,&keyValue);
    engine::function<NativeDisplayName*(*)(void*,NativeDisplayName*,NativeString*)>(0x72ce1c0)
        (state.displayNameManager,&result,&ownedKey);
    const bool matched=EqualText(result.full,value.full.data)&&EqualText(result.game,game)&&EqualText(result.tag,tag);
    engine::function<void(*)(NativeDisplayName*)>(0x1b0e200)(&result);
    void* platformPlayer=nullptr;
    const bool modelReady=matched&&EnsureCachedPlatformPlayer(keyValue,value,platformPlayer);
    engine::function<void(*)(NativeDisplayName*)>(0x1b0e200)(&value);
    if(!matched)return Fail(3328,"local_network_original_display_name_getter_readback_failed");
    if(!modelReady)return false;
    ++state.nativeNameGetterReadbacks;
    if(existing==state.offlineNameCount){
        wcscpy_s(offlineNames[existing].key,key);wcscpy_s(offlineNames[existing].game,game);
        wcscpy_s(offlineNames[existing].tag,tag);++state.offlineNameCount;
    }
    offlineNames[existing].platformPlayer=platformPlayer;
    state.offlineRosterReady=state.offlineNameCount>0&&state.onlineNameServicesAbsent;
    state.operation="none";return true;
}

inline bool OfflineSubjectRegistered(const wchar_t* key){
    if(state.error||state.restartRequired||!state.offlineRosterReady||!key||!original_hud::Thread())return false;
    for(size_t i=0;i<state.offlineNameCount;++i)if(std::wcscmp(offlineNames[i].key,key)==0){
        const auto borrowed=Borrow(key);
        auto player=offlineNames[i].platformPlayer;
        return FindCachedName(borrowed)>=0&&player&&FindCachedPlatformPlayer(borrowed)==player&&StronglyReferenced(player);
    }
    return false;
}

// Compatibility spelling only. The argument is UniqueId.ToString(), not
// PlayerNamePrivate or the ordinary ?Name travel option.
inline bool OfflineNameRegistered(const wchar_t* key){return OfflineSubjectRegistered(key);}

inline bool ReadPlayerSubjectKeyImpl(void* playerState,wchar_t* output,size_t capacity){
    if(output&&capacity)output[0]=0;
    if(state.error||state.restartRequired)return false;
    if(!original_hud::Thread())return Fail(3330,"local_network_unique_id_read_wrong_thread");
    if(!output||capacity<2||capacity>4096||!original_players::IsA(playerState,EnginePlayerStateClass))
        return Fail(3331,"local_network_unique_id_read_arguments_invalid");
    if(!state.profilesVerified){
        state.profilesVerified=VerifyProfiles();
        if(!state.profilesVerified)return Fail(3310,"local_network_services_source_profile_mismatch");
    }
    state.stage="local_network_reading_original_player_unique_id_key";
    state.operation="original_unique_net_id_to_string_3dbe420";
    NativeString value{};
    // Generated Engine.PlayerState metadata: UniqueId is a STRUCT at+498;
    // PlayerNamePrivate is a separate FString at+508. Match717e26d..717e279
    // exactly; never reinterpret UniqueId's internal storage as a wchar_t*.
    engine::function<NativeString*(*)(void*,NativeString*)>(0x3dbe420)
        (static_cast<uint8_t*>(playerState)+0x498,&value);
    ++state.uniqueIdKeyReads;
    state.lastUniqueIdKeyCharacters=value.count>0?value.count-1:0;
    const bool empty=value.count<=1;
    bool valid=value.data&&value.count>1&&value.capacity>=value.count
        &&static_cast<size_t>(value.count)<=capacity;
    if(valid){
        for(int32_t i=0;i<value.count;++i)output[i]=Read<wchar_t>(value.data,static_cast<size_t>(i)*sizeof(wchar_t));
        valid=output[value.count-1]==0&&std::wcslen(output)==static_cast<size_t>(value.count-1);
    }
    engine::function<void(*)(NativeString*)>(0x18a2f10)(&value);
    state.operation="none";
    if(empty){
        ++state.emptyUniqueIdReads;state.stage="local_network_waiting_original_player_unique_id";return false;
    }
    if(!valid){output[0]=0;return Fail(3332,"local_network_unique_id_string_layout_or_capacity_invalid");}
    state.stage="local_network_original_player_unique_id_key_read";return true;
}

inline bool InspectPerformanceManager(void* manager){
    state.performanceManager=manager;
    state.metricArrayVerified=false;state.verifiedMetricIds=0;
    if(!original_players::IsA(manager,PerfManagerClass))
        return Fail(3306,"local_network_perf_manager_type_mismatch");
    // Exact original factory stores its first two arguments in +98/+90.
    if(Read<void*>(manager,0x98)!=state.clientInstance
       ||Read<void*>(manager,0x90)!=state.originalOwnerF8)
        return Fail(3307,"local_network_perf_manager_original_inputs_mismatch");
    auto metrics=Read<void**>(manager,0xa8);
    state.metricCount=Read<int32_t>(manager,0xb0);
    state.metricCapacity=Read<int32_t>(manager,0xb4);
    if(!metrics||state.metricCount!=MetricCount||state.metricCapacity<MetricCount
       ||state.metricCapacity>256||Read<uint8_t>(manager,0x84)!=MetricCount)
        return Fail(3308,"local_network_perf_metric_array_incomplete");
    for(int32_t i=0;i<MetricCount;++i){
        auto metric=Read<void*>(metrics,static_cast<size_t>(i)*sizeof(void*));
        if(!original_players::IsA(metric,PerfMetricClass)||Read<uint8_t>(metric,0x30)!=i)
            return Fail(3309,"local_network_perf_metric_identity_mismatch");
        ++state.verifiedMetricIds;
    }
    // Read the native IsInitialized field; factory construction does not set it.
    state.nativeInitialized=Read<uint8_t>(manager,0x85)!=0;
    state.metricArrayVerified=true;
    return true;
}

inline bool PrepareImpl(void* world,void* clientGI,bool host){
    if(state.restartRequired||state.error)return false;
    if(!original_hud::Thread())return Fail(3301,"local_network_services_wrong_game_thread");
    if(!world||!original_players::IsA(clientGI,0x6429080))
        return Fail(3302,"local_network_services_world_or_client_missing");
    auto gi=Read<void*>(world,0x1d8);
    if(!original_players::IsA(gi,0x647e8a0)||Read<void*>(gi,0x2b8)!=clientGI
       ||Read<void*>(clientGI,0x2c8)!=gi)
        return Fail(3303,"local_network_services_game_instance_links_mismatch");
    if(state.clientInstance&&(state.clientInstance!=clientGI||state.gameInstance!=gi||state.host!=host))
        return Fail(3304,"local_network_services_session_identity_changed");
    state.world=world;state.gameInstance=gi;state.clientInstance=clientGI;state.host=host;
    state.settingsManager=Read<void*>(clientGI,0x378);
    if(!original_players::IsA(state.settingsManager,0x6510860)
       ||!Read<uint8_t>(state.settingsManager,0x890))
        return Fail(3305,"local_network_services_original_settings_required");
    // Original callsite7032f4a passes the actual AresGI+f8 pointer. Its concrete
    // native type is not needed here; preserve the value, including native null.
    state.originalOwnerF8=Read<void*>(gi,0xf8);
    if(!state.profilesVerified){
        state.profilesVerified=VerifyProfiles();
        if(!state.profilesVerified)return Fail(3310,"local_network_services_source_profile_mismatch");
    }
    auto manager=Read<void*>(clientGI,0x5e8);
    if(!manager){
        if(state.factoryInvoked)return Fail(3311,"local_network_perf_partial_factory_requires_new_session");
        state.stage="local_network_constructing_original_perf_manager_and_metrics";
        state.operation="original_perf_manager_factory_702a7f0";state.factoryInvoked=true;
        // Exact original ClientGI callsite7032f43..7032f52: this complete local
        // factory also creates every metric and its constructor-owned buffers.
        using Factory=void*(*)(void*,void*,void*);
        manager=engine::function<Factory>(0x702a7f0)(clientGI,state.originalOwnerF8,state.settingsManager);
        if(!InspectPerformanceManager(manager))return false;
        if(!original_players::Write(clientGI,0x5e8,manager))
            return Fail(3312,"local_network_perf_manager_owner_reference_failed");
    }else if(!InspectPerformanceManager(manager))return false;
    state.managerLinked=Read<void*>(clientGI,0x5e8)==manager;
    if(!state.managerLinked)return Fail(3313,"local_network_perf_manager_owner_readback_failed");
    // Actual subject keys become available from the server's accepted real
    // PlayerStates. The root transports their converted values to each client
    // and waits for native-cache ACKs before opening those actor channels.
    // Prepare creates providers only; it must not substitute ?Name strings.
    if(!PrepareLocalNameProviders())return false;
    if(!PrepareOfflineMatchContexts())return false;
    state.operation="none";state.ready=true;state.stage="local_network_original_local_services_ready";
    return true;
}

inline LONG CaptureException(EXCEPTION_POINTERS* exception){
    if(exception&&exception->ExceptionRecord){
        state.error=exception->ExceptionRecord->ExceptionCode;
        const auto at=reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress);
        state.exceptionRva=at>=engine::image?at-engine::image:at;
    }else state.error=3314;
    state.ready=false;state.offlineRosterReady=false;state.restartRequired=true;
    state.stage="local_network_services_native_exception_requires_new_session";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Prepare(void* world,void* clientGI,bool host){
    __try {return PrepareImpl(world,clientGI,host);}
    __except(CaptureException(GetExceptionInformation())) {return false;}
}
inline bool ReadPlayerSubjectKey(void* playerState,wchar_t* output,size_t capacity){
    __try {return ReadPlayerSubjectKeyImpl(playerState,output,capacity);}
    __except(CaptureException(GetExceptionInformation())) {return false;}
}
inline bool CacheOfflineSubject(const wchar_t* actualKey,const wchar_t* game,const wchar_t* tag=L"LOCAL"){
    __try {return CacheOfflineSubjectImpl(actualKey,game,tag);}
    __except(CaptureException(GetExceptionInformation())) {return false;}
}
}
