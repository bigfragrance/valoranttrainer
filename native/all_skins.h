#pragma once
#include "all_weapons.h"
#include "original_scope.h"
#include "original_appearance_context.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <unordered_set>
#include <vector>

// Real ContentIndex GUIDs and original pre-BeginPlay attachment factory inputs.
// A live skin change requests the same definition through the original complete
// weapon replacement/equip lifecycle. No old weapon trait map is hand-cleared.
namespace all_skins {
using Guid=original_content::Guid;
using Gate=all_weapons::Gate;
struct Selection {Guid weapon{},skin{},level{},chroma{};void *skinData=nullptr,*levelData=nullptr,*chromaData=nullptr;bool selected=false;};
struct Binding {
    Guid weaponId{},skinId{},levelId{},chromaId{};
    void *world=nullptr,*pawn=nullptr,*weapon=nullptr,*skinData=nullptr,*levelData=nullptr,*chromaData=nullptr;
    void *skinAttachment=nullptr,*chromaAttachment=nullptr,*scope=nullptr;
    void *coreInstance=nullptr,*animationState=nullptr;
    original_effects::WeakIdentity identity{},scopeIdentity{};
    unsigned generation=0,forcedCount=0,forcedNullCount=0;
    void* forced[64]{};
    std::vector<size_t> observations;
    uint64_t lastPrepare=0;
    bool prepared=false,assetsReady=false,scopeReady=false,ready=false,retired=false,scopeRequired=false;
    bool scopeTransformRead=false,animationTransformRead=false,animationConsumed=false,blueprintScopeMatches=false;
    unsigned reflexCatchups=0;
    uint64_t animationSamples=0;
    double scopeTransform[10]{},animationTransform[10]{};
};
using NativeNotify=void(*)(void*,uint8_t);
using NativePaths=void(*)(void*,uint8_t,void*,void*);
struct PreloadObservation {
    void *component=nullptr,*nativeInterface=nullptr,*originalTable=nullptr,*observedTable=nullptr;
    void* preloadManager=nullptr;
    NativeNotify notify=nullptr;
    NativePaths getPaths=nullptr;
    original_effects::WeakIdentity identity{};
    unsigned entered=0,completed=0;
    uint8_t assetTypes=0;
    bool requested=false,retired=false,reflexCaughtUp=false;
    bool requestRunning=false,requestReturned=false,pathSetObserved=false,pathSetValid=false;
    bool emptyCompletion=false,nativeLoadedQueried=false,nativeLoaded=false;
    int pathAllocated=-1,pathFree=-1,pathCount=-1;
    unsigned pathCalls=0;
};
struct Diagnostics {
    unsigned error=0,requests=0,completed=0,generation=0,forcedArrays=0,preloadRequests=0,preloadCallbacks=0,emptyCompletions=0;
    bool initialized=false,pending=false;
    const char* stage="all_skins_not_initialized";
    void* requestedWeapon=nullptr;
    Guid requestedDefinition{},requestedSkin{},requestedLevel{},requestedChroma{};
};
// A read-only identity/relationship audit, not a render/equip certification.
// Counts come from the original live index; no 1433-actor batch is created.
struct CatalogValidation {
    uint64_t elapsedMs=0;
    int expectedSkins=0,expectedLevels=0,expectedChromas=0;
    unsigned enumeratedSkins=0,verifiedSkins=0,verifiedWeaponLinks=0;
    unsigned levelLinks=0,chromaLinks=0,uniqueLevels=0,uniqueChromas=0;
    unsigned nullLevelLinks=0,nullChromaLinks=0,duplicateLinks=0;
    unsigned defaultChoices=0,verifiedDefaultChoices=0,combinations=0,verifiedCombinations=0;
    unsigned failures=0,failedSkins=0,error=0;
    bool nativeEnumeration=false,complete=false,coverageComplete=false,stateUnchanged=false,catalogStable=false,passed=false;
    Guid firstSkin{},firstLevel{},firstChroma{};
    const char* stage="all_skins_catalog_not_validated";
};
inline Diagnostics diagnostics;
inline std::vector<Selection> selections;
inline std::vector<Binding> bindings;
inline std::vector<PreloadObservation> observations;
inline const Diagnostics& State(){return diagnostics;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Same(const Guid& a,const Guid& b){return std::memcmp(&a,&b,sizeof(a))==0;}
inline bool Valid(const Guid& id){return original_content::Valid(id);}
inline bool Thread(){return original_content::Thread();}
inline bool Fail(unsigned error,const char* stage){diagnostics.error=error;diagnostics.stage=stage;return false;}
inline bool Event(void* object,const wchar_t* path,uint16_t bytes,void* args){
    auto fn=engine::Find(path);return object&&fn&&Read<uint16_t>(fn,0xbe)==bytes&&engine::Event(object,path,args);
}
inline void* SoftClass(void* object,size_t offset){return original_scope::ResolveLoadedSoft(static_cast<uint8_t*>(object)+offset);}
inline void* SoftCDO(void* object,size_t offset){auto type=object?SoftClass(object,offset):nullptr;return type?engine::DefaultObject(type):nullptr;}
inline void* Indexed(uintptr_t function,uintptr_t getter,const Guid& id){
    if(!all_skins::Valid(id))return nullptr;
    auto data=engine::function<void*(*)(void*,const Guid*)>(function)(original_content::State().index,&id);
    return original_players::IsA(data,getter)&&Same(Read<Guid>(data,0x38),id)?data:nullptr;
}
inline void* Skin(const Guid& id){return Indexed(0x71d1300,0x676d6b0,id);}
inline void* Level(const Guid& id){return Indexed(0x71d13b0,0x676f1b0,id);}
inline void* Chroma(const Guid& id){return Indexed(0x71d1460,0x676eaa0,id);}
inline bool InSoftArray(void* skin,size_t offset,void* wanted){
    auto data=Read<uint8_t*>(skin,offset);const int count=Read<int>(skin,offset+8),capacity=Read<int>(skin,offset+12);
    if(count<0||count>256||capacity<count||capacity>4096||(!data&&count))return false;
    for(int i=0;i<count;++i)if(SoftCDO(data,static_cast<size_t>(i)*48)==wanted)return true;
    return false;
}
inline bool Resolve(const all_weapons::Definition& definition,const Guid& skinId,const Guid& levelId,const Guid& chromaId,Selection& out){
    out={};out.weapon=definition.id;
    if(!definition.identityVerified||!original_players::IsA(definition.dataAsset,0x6638d00))
        return Fail(2401,"all_skins_original_weapon_definition_not_verified");
    if(all_skins::Valid(skinId))out.skinData=Skin(skinId);
    else{
        const int offset=original_scope::FieldOffset(Read<void*>(definition.dataAsset,0x10),L"DefaultSkin");
        if(offset>=0)out.skinData=SoftCDO(definition.dataAsset,static_cast<size_t>(offset));
    }
    if(!original_players::IsA(out.skinData,0x676d6b0)||Skin(Read<Guid>(out.skinData,0x38))!=out.skinData
        ||SoftCDO(out.skinData,0x58)!=definition.dataAsset)
        return Fail(2402,"all_skins_skin_guid_or_authored_weapon_relationship_invalid");
    if(all_skins::Valid(levelId))out.levelData=Level(levelId);
    else{
        const int count=Read<int>(out.skinData,0x90),capacity=Read<int>(out.skinData,0x94);
        auto data=Read<void*>(out.skinData,0x88);
        if(count>=0&&count<=256&&capacity>=count&&capacity<=4096&&(!count||data))
            for(int i=0;i<count&&!out.levelData;++i){
                auto candidate=SoftCDO(data,static_cast<size_t>(i)*48);
                if(original_players::IsA(candidate,0x676f1b0))out.levelData=candidate;
            }
    }
    if(!original_players::IsA(out.levelData,0x676f1b0)||Level(Read<Guid>(out.levelData,0x38))!=out.levelData
        ||!InSoftArray(out.skinData,0x88,out.levelData))
        return Fail(2403,"all_skins_level_guid_not_in_original_skin_levels");
    out.chromaData=all_skins::Valid(chromaId)?Chroma(chromaId):SoftCDO(out.skinData,0x98);
    if(!original_players::IsA(out.chromaData,0x676eaa0)||Chroma(Read<Guid>(out.chromaData,0x38))!=out.chromaData
        ||(!InSoftArray(out.skinData,0xc8,out.chromaData)&&SoftCDO(out.skinData,0x98)!=out.chromaData))
        return Fail(2404,"all_skins_chroma_guid_not_in_original_skin_chromas");
    out.skin=Read<Guid>(out.skinData,0x38);out.level=Read<Guid>(out.levelData,0x38);out.chroma=Read<Guid>(out.chromaData,0x38);
    out.selected=true;return true;
}
inline Selection* FindSelection(const Guid& weapon){for(auto& s:selections)if(Same(s.weapon,weapon))return &s;return nullptr;}
inline Binding* FindBinding(void* weapon){
    for(auto& b:bindings)if(!b.retired&&b.weapon==weapon&&original_effects::ResolveIdentity(b.identity)==weapon)return &b;
    return nullptr;
}
inline const Binding* CurrentBinding(){return FindBinding(all_weapons::CurrentWeapon());}
inline bool Initialize(){
    if(!Thread()||!original_content::State().initialized||!original_content::State().shouldStoreCDOs)
        return Fail(2405,"all_skins_original_index_cdos_or_game_thread_not_ready");
    if(!original_content_services::Refresh()||!original_content_services::State().requestPathReady)
        return Fail(2406,"all_skins_original_preload_services_not_ready");
    const auto& services=original_content_services::State();
    if(!original_appearance_context::Ensure(services.clientInstance,services.preloadManager,services.configManager))
        return Fail(original_appearance_context::State().error,original_appearance_context::State().stage);
    diagnostics.initialized=true;diagnostics.error=0;diagnostics.stage="all_skins_original_content_index_ready";return true;
}
inline bool SetSelection(const Guid& weaponId,const Guid& skinId,const Guid& levelId,const Guid& chromaId){
    if(!Initialize())return false;
    const auto definition=all_weapons::Find(weaponId);Selection next;
    if(!definition||!Resolve(*definition,skinId,levelId,chromaId,next))return false;
    auto existing=FindSelection(weaponId);if(existing)*existing=next;else selections.push_back(next);
    diagnostics.error=0;diagnostics.stage="all_skins_local_loadout_selection_validated";return true;
}
inline bool ApplySkin(void* currentWeapon,const Guid& weaponId,const Guid& skinId,const Guid& levelId,const Guid& chromaId){
    if(!Thread()||all_weapons::State().busy)return Fail(2407,"all_skins_switch_busy_or_wrong_thread");
    const auto definition=all_weapons::CurrentDefinition();
    if(!definition||!Same(definition->id,weaponId)||currentWeapon!=all_weapons::CurrentWeapon()
        ||Read<void*>(currentWeapon,0x210)!=all_weapons::State().pawn
        ||Read<void*>(currentWeapon,0x4a8)!=all_weapons::State().pawn)
        return Fail(2408,"all_skins_current_original_weapon_identity_mismatch");
    if(!SetSelection(weaponId,skinId,levelId,chromaId))return false;
    const auto selected=FindSelection(weaponId);
    if(!all_weapons::Equip(weaponId,true))return Fail(2409,"all_skins_original_same_weapon_rebuild_not_accepted");
    ++diagnostics.requests;diagnostics.pending=true;diagnostics.requestedWeapon=currentWeapon;
    diagnostics.requestedDefinition=weaponId;diagnostics.requestedSkin=selected->skin;
    diagnostics.requestedLevel=selected->level;diagnostics.requestedChroma=selected->chroma;
    diagnostics.stage="all_skins_waiting_for_original_replacement_equip";return true;
}
inline bool ApplySkin(void* currentWeapon,const Guid& skinId,const Guid& levelId,const Guid& chromaId){
    const auto d=all_weapons::CurrentDefinition();return d?ApplySkin(currentWeapon,d->id,skinId,levelId,chromaId):Fail(2408,"all_skins_current_definition_missing");
}
inline bool ForcedAttachments(Binding& binding,const all_weapons::Definition& definition){
    // Pinned EquippableDataAsset.ForcedAttachments+b8. Its real destructor
    // 6761580->3442630 uses allocation bits at+10/+20/+28 and element stride38.
    auto set=static_cast<uint8_t*>(definition.dataAsset)+0xb8;
    const int allocated=Read<int>(set,8),free=Read<int>(set,0x34),bitsCount=Read<int>(set,0x28);
    auto data=Read<uint8_t*>(set,0);auto bits=Read<uint32_t*>(set,0x20);
    if(!bits)bits=reinterpret_cast<uint32_t*>(set+0x10);
    if(allocated<0||allocated>64||free<0||free>allocated||bitsCount!=allocated||(!data&&allocated))
        return Fail(2410,"all_skins_original_forced_attachment_set_layout_invalid");
    unsigned occupied=0;
    for(int i=0;i<allocated;++i){
        if((Read<uint32_t>(bits,static_cast<size_t>(i/32)*4)&(1u<<(i%32)))==0)continue;
        ++occupied;const size_t offset=static_cast<size_t>(i)*0x38;
        // The real Melee/Burst sets contain an authored null soft class. Use
        // original KismetSystemLibrary.IsValidSoftClassReference (exec45d30d0)
        // to distinguish that null from a valid reference not yet resolved.
        // Original factory70f41e3 dereferences every fc0 element: never pass a
        // null CDO to that factory or silently skip an unresolved non-null one.
        struct ValidSoftClass{uint64_t reference[6]{};uint8_t result=0;uint8_t padding[7]{};} p;
        static_assert(sizeof(ValidSoftClass)==56);
        auto library=engine::DefaultObject(engine::Class(0x45aa110));
        if(!original_players::Read(data,offset,p.reference)
            ||!Event(library,L"/Script/Engine.KismetSystemLibrary.IsValidSoftClassReference",49,&p))
            return Fail(2437,"all_skins_original_forced_soft_reference_validation_unavailable");
        if(!p.result){++binding.forcedNullCount;continue;}
        auto asset=SoftCDO(data,offset);
        if(!original_players::IsA(asset,0x6763d30)||binding.forcedCount>=64)
            return Fail(2411,"all_skins_original_forced_attachment_identity_unavailable");
        binding.forced[binding.forcedCount++]=asset;
    }
    if(occupied!=static_cast<unsigned>(allocated-free))return Fail(2410,"all_skins_original_forced_attachment_bits_inconsistent");
    struct Array{void** data;int count,capacity;};static_assert(sizeof(Array)==16);
    const auto old=Read<Array>(binding.weapon,0xfc0);
    if(old.count){
        if(old.count!=static_cast<int>(binding.forcedCount)||old.capacity<old.count)return Fail(2412,"all_skins_original_attachment_array_already_differs");
        for(unsigned i=0;i<binding.forcedCount;++i)if(Read<void*>(old.data,i*8)!=binding.forced[i])return Fail(2412,"all_skins_original_attachment_array_already_differs");
        return true;
    }
    if(!binding.forcedCount)return true;
    if(old.data||old.capacity)return Fail(2412,"all_skins_unexpected_existing_attachment_array_allocation");
    // Exact existing FMalloc and ABI used at original1b9c292..1b9c2c5.
    // This allocation becomes the real Actor TArray's owned storage and will
    // be released by the original Actor/property destruction, never the CRT.
    auto allocator=Read<void*>(reinterpret_cast<void*>(engine::image+0xd978ce8));
    auto table=Read<void*>(allocator);auto allocate=Read<void*(*)(void*,size_t,uint32_t)>(table,0x28);
    if(!allocator||!allocate)return Fail(2413,"all_skins_original_allocator_unavailable");
    auto memory=static_cast<void**>(allocate(allocator,binding.forcedCount*sizeof(void*),8));
    if(!memory)return Fail(2413,"all_skins_original_attachment_array_allocation_failed");
    std::memcpy(memory,binding.forced,binding.forcedCount*sizeof(void*));
    Array value{memory,static_cast<int>(binding.forcedCount),static_cast<int>(binding.forcedCount)};
    if(!original_players::Write(binding.weapon,0xfc0,value)){
        engine::function<void(*)(void*)>(0x1915720)(memory);return Fail(2414,"all_skins_original_attachment_array_publication_failed");
    }
    ++diagnostics.forcedArrays;return true;
}
inline bool BeforeFinishSpawn(void* weapon,const all_weapons::Definition& definition){
    if(!Initialize())return false;
    if(!weapon||Read<void*>(weapon,0x10)!=definition.runtimeClass||Read<void*>(weapon,0xfa8)!=definition.dataAsset)
        return Fail(2415,"all_skins_deferred_weapon_definition_mismatch");
    uint8_t begun=0;
    if(!Event(weapon,L"/Script/Engine.Actor.HasActorBegunPlay",1,&begun)||begun)
        return Fail(2416,"all_skins_loadout_must_precede_original_beginplay");
    if(FindBinding(weapon))return Fail(2417,"all_skins_deferred_weapon_already_prepared");
    Selection selected;const auto saved=FindSelection(definition.id);
    if(saved)selected=*saved;else if(!Resolve(definition,Guid{},Guid{},Guid{},selected))return false;
    Binding binding;binding.world=all_weapons::State().world;binding.pawn=all_weapons::State().pawn;binding.weapon=weapon;
    binding.weaponId=definition.id;binding.skinId=selected.skin;binding.levelId=selected.level;binding.chromaId=selected.chroma;
    binding.skinData=selected.skinData;binding.levelData=selected.levelData;binding.chromaData=selected.chromaData;
    binding.generation=++diagnostics.generation;
    if(!original_effects::CaptureIdentity(weapon,binding.identity))return Fail(2418,"all_skins_deferred_weapon_weak_identity_unavailable");
    if(!ForcedAttachments(binding,definition))return false;
    if(!original_players::Write(weapon,0xfb0,binding.skinData)||!original_players::Write(weapon,0xfb8,binding.levelData)
        ||!original_players::Write(weapon,0xfd0,binding.chromaData))return Fail(2418,"all_skins_original_prefinish_loadout_publication_failed");
    binding.prepared=true;bindings.push_back(binding);diagnostics.stage="all_skins_original_prefinish_loadout_ready";return true;
}

inline size_t ObservationIndex(void* nativeInterface){
    for(size_t i=0;i<observations.size();++i)if(observations[i].nativeInterface==nativeInterface
        &&observations[i].observedTable==Read<void*>(nativeInterface))return i;
    return observations.size();
}
inline void ObserveNativePaths(void* nativeInterface,uint8_t assetTypes,void* outputSet,void* manager){
    const auto index=ObservationIndex(nativeInterface);
    if(index>=observations.size()||!observations[index].getPaths)return;
    auto original=observations[index].getPaths;original(nativeInterface,assetTypes,outputSet,manager);
    // Observe the actual TSet returned to the actual original manager. Its
    // 6eb51bb empty branch compares these same allocated/free fields and does
    // not dispatch interface.OnAssetsLoaded; it only runs an optional delegate.
    // No additional Blueprint query or adapter-generated path list is used.
    auto& record=observations[index];
    if(record.requestRunning&&assetTypes==7&&manager==record.preloadManager){
        ++record.pathCalls;record.pathSetObserved=true;
        record.pathAllocated=Read<int>(outputSet,8);record.pathFree=Read<int>(outputSet,0x34);
        record.pathSetValid=record.pathAllocated>=0&&record.pathAllocated<=65536
            &&record.pathFree>=0&&record.pathFree<=record.pathAllocated;
        record.pathCount=record.pathSetValid?record.pathAllocated-record.pathFree:-1;
    }
}
inline void ObserveNativeAssetsLoaded(void* nativeInterface,uint8_t assetTypes){
    const auto index=ObservationIndex(nativeInterface);
    // Every installed interface retains the complete original notifier. This
    // observer never replaces the Blueprint callback, material work, or mesh
    // creation. It records completion AFTER the original native callback.
    if(index>=observations.size()||!observations[index].notify)return;
    ++observations[index].entered;observations[index].assetTypes|=assetTypes;
    auto original=observations[index].notify;original(nativeInterface,assetTypes);
    ++observations[index].completed;++diagnostics.preloadCallbacks;
}
inline bool ObserveAndRequest(Binding& binding,void* component){
    for(size_t i=0;i<observations.size();++i){
        auto& known=observations[i];
        if(!known.retired&&known.component==component&&original_effects::ResolveIdentity(known.identity)==component){
            bool found=false;for(auto index:binding.observations)found=found||index==i;
            if(!found)binding.observations.push_back(i);return true;
        }
    }
    auto iface=engine::function<void*(*)(void*,void*)>(0x1cff3d0)(component,engine::Class(0x68ed750));
    auto table=Read<void**>(iface);
    struct InterfaceProfile{uintptr_t rva;size_t entries;uintptr_t next;};
    constexpr InterfaceProfile profiles[]={{0xb11b830,10,0xb85bedc},{0xb1870b0,7,0xc31b1b0},
        {0xb184ea0,6,0x662e3e0},{0xb18a740,6,0x662e820}};
    const InterfaceProfile* profile=nullptr;
    for(const auto& p:profiles)if(table==reinterpret_cast<void**>(engine::image+p.rva)){profile=&p;break;}
    if(iface!=static_cast<uint8_t*>(component)+0xd8||!table
        ||!profile||Read<void*>(table,profile->entries*8)!=reinterpret_cast<void*>(engine::image+profile->next)
        ||Read<void*>(table,0x10)!=reinterpret_cast<void*>(engine::image+0x710aaa0)
        ||Read<void*>(table,0x28)!=reinterpret_cast<void*>(engine::image+0x6e4ad90)
        ||Read<void*>(table,0x18)!=reinterpret_cast<void*>(engine::image+0x710ab80))
        return Fail(2419,"all_skins_original_preload_interface_profile_unsupported");
    PreloadObservation entry;entry.component=component;entry.nativeInterface=iface;entry.originalTable=table;
    entry.notify=reinterpret_cast<NativeNotify>(Read<void*>(table,0x18));
    entry.getPaths=reinterpret_cast<NativePaths>(Read<void*>(table,0x10));
    entry.preloadManager=original_content_services::State().preloadManager;
    if(!original_effects::CaptureIdentity(component,entry.identity))return Fail(2420,"all_skins_attachment_identity_unavailable");
    // Preserve each pinned native interface group and preceding compiler word.
    // Native constructors6638200/710ca70 establish the short-table boundaries
    // even when the following primary vtable is adjacent executable pointers.
    // Original GetPaths(slot10) and OnAssetsLoaded(slot18) both run in full;
    // primary vptr, asset paths and asset-readiness state are untouched.
    const size_t bytes=(profile->entries+1)*sizeof(void*);
    auto allocation=static_cast<void**>(VirtualAlloc(nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!allocation)return Fail(2421,"all_skins_preload_observer_allocation_failed");
    std::memcpy(allocation,table-1,bytes);auto copied=allocation+1;
    copied[0x10/8]=reinterpret_cast<void*>(&ObserveNativePaths);
    copied[0x18/8]=reinterpret_cast<void*>(&ObserveNativeAssetsLoaded);
    DWORD previous=0;
    if(!VirtualProtect(allocation,bytes,PAGE_READONLY,&previous)){
        VirtualFree(allocation,0,MEM_RELEASE);return Fail(2421,"all_skins_preload_observer_protection_failed");
    }
    entry.observedTable=copied;observations.push_back(entry);const size_t index=observations.size()-1;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(iface),copied,table)!=table){
        observations.pop_back();VirtualFree(allocation,0,MEM_RELEASE);return Fail(2422,"all_skins_attachment_interface_changed");
    }
    binding.observations.push_back(index);observations[index].requested=true;
    original_content_services::NativeDelegate completion;
    ++diagnostics.preloadRequests;
    observations[index].requestRunning=true;
    engine::function<void(*)(void*,void*,uint8_t,int32_t,original_content_services::NativeDelegate*)>(0x6eb4fc0)
        (original_content_services::State().preloadManager,iface,7,0,&completion);
    observations[index].requestRunning=false;observations[index].requestReturned=true;
    if(!observations[index].pathSetObserved||!observations[index].pathSetValid)
        return Fail(2438,"all_skins_original_preload_path_set_not_observed");
    if(observations[index].pathCount==0&&observations[index].entered==0){
        // Real standard skin/chroma attachments have zero soft dependencies.
        // Native6eb4fc0 returns through51bb..52a8 without their notification.
        // Also ask original ArePreloadableSoftRefsLoaded(7), then resume the
        // original notification once. This executes native710ab80 and the real
        // K2_OnAssetsLoaded Blueprint; counters advance only around that call.
        // It is the same original leaf used by weapon fan-out70f0fc0. Using
        // that whole collector here would require unrelated PregameManager.
        observations[index].nativeLoadedQueried=true;
        observations[index].nativeLoaded=Read<bool(*)(void*,uint8_t)>(table,0x28)(iface,7);
        if(observations[index].nativeLoaded){
            if(original_effects::ResolveIdentity(observations[index].identity)!=component
                ||Read<void*>(iface)!=copied)return Fail(2439,"all_skins_empty_preload_attachment_changed");
            observations[index].emptyCompletion=true;++diagnostics.emptyCompletions;
            Read<NativeNotify>(copied,0x18)(iface,7);
        }
    }
    return true;
}
inline bool GatherAttachments(Binding& binding){
    struct Components{void* type;void** data=nullptr;int count=0,capacity=0;} p{engine::Class(0x36837a0)};
    static_assert(sizeof(Components)==24);
    if(!Event(binding.weapon,L"/Script/Engine.Actor.K2_GetComponentsByClass",24,&p))
        return Fail(2423,"all_skins_original_component_enumeration_unavailable");
    bool valid=p.count>=0&&p.count<=512&&p.capacity>=p.count&&p.capacity<=4096&&(!p.count||p.data);
    if(valid)for(int i=0;i<p.count;++i){
        auto component=Read<void*>(p.data,static_cast<size_t>(i)*8);
        if(!original_players::IsA(component,0x6574f50)||Read<void*>(component,0xc8)!=binding.weapon)continue;
        const auto data=Read<void*>(component,0xf8);
        bool selected=data==binding.levelData||data==binding.chromaData;
        for(unsigned n=0;n<binding.forcedCount;++n)selected=selected||data==binding.forced[n];
        if(!selected)continue;
        if(data==binding.levelData)binding.skinAttachment=component;
        if(data==binding.chromaData)binding.chromaAttachment=component;
        if(!ObserveAndRequest(binding,component)){valid=false;break;}
    }
    if(p.data)engine::function<void(*)(void*)>(0x1915720)(p.data);
    return valid?true:diagnostics.error?false:Fail(2424,"all_skins_original_component_array_invalid");
}
inline void* Component(void* actor,void* type){
    struct P{void* type;void* result=nullptr;} p{type};
    return type&&Event(actor,L"/Script/Engine.Actor.GetComponentByClass",16,&p)?p.result:nullptr;
}
inline bool OriginalScopeCatchup(Binding& binding){
    auto reflexType=engine::Find(original_scope::kReflex);
    for(auto index:binding.observations){
        auto& entry=observations[index];
        if(entry.reflexCaughtUp||!reflexType||!original_weapon::IsA(entry.component,reflexType))continue;
        const int meshOffset=original_scope::FieldOffset(reflexType,L"ReflexMesh1PSpawned");
        const int parentOffset=original_scope::FieldOffset(reflexType,L"1P Parent Mesh");
        if(meshOffset<0||parentOffset<0)return Fail(2425,"all_skins_original_reflex_reflection_profile_changed");
        auto mesh=Read<void*>(entry.component,static_cast<size_t>(meshOffset));
        auto parent=Read<void*>(entry.component,static_cast<size_t>(parentOffset));
        if(!original_players::IsA(mesh,0x385f750)||!original_players::IsA(parent,0x385f750))return true;
        auto graph=engine::Find(original_scope::kReflexGraph);auto script=Read<void*>(graph,0x68);
        if(!graph||Read<void*>(graph,0x28)!=reflexType||Read<uint16_t>(graph,0xbe)!=4
            ||Read<int>(graph,0x70)!=3631||Read<uint8_t>(script,0)!=0x4c
            ||Read<uint32_t>(script,1)!=3628||Read<uint8_t>(script,761)!=0x5f)
            return Fail(2426,"all_skins_original_reflex_client_graph_profile_changed");
        int32_t originalEntry=761;
        if(!Event(entry.component,original_scope::kReflexGraph,4,&originalEntry))
            return Fail(2427,"all_skins_original_reflex_client_event_failed");
        entry.reflexCaughtUp=true;++binding.reflexCatchups;
    }
    auto scopeType=engine::Find(original_scope::kScope);binding.scope=Component(binding.weapon,scopeType);
    if(binding.scope){
        if(!original_effects::CaptureIdentity(binding.scope,binding.scopeIdentity))
            return Fail(2434,"all_skins_scope_weak_identity_unavailable");
        engine::Transform alignment{};
        const int offset=original_scope::FieldOffset(scopeType,L"ScopeTransformComponentSpace");
        if(offset!=0xe0||Read<int>(scopeType,0x60)!=0x140)return Fail(2428,"all_skins_original_scope_transform_layout_changed");
        binding.scopeReady=original_players::Read(binding.scope,0xe0,alignment)&&!original_scope::Identity(alignment);
    }
    auto zoom=engine::function<void*(*)(void*)>(0x6ec5800)(binding.weapon);
    binding.scopeRequired=zoom&&Read<int>(zoom,0x4b0)>0;
    if(!binding.scopeRequired)binding.scopeReady=true;
    return true;
}
inline Gate PrepareWeapon(void* weapon,const all_weapons::Definition& definition){
    if(!Thread())return Gate::Failed;
    auto binding=FindBinding(weapon);
    if(!binding||!binding->prepared||!Same(binding->weaponId,definition.id))return Gate::Failed;
    if(binding->ready)return Gate::Ready;
    const auto now=GetTickCount64();if(now-binding->lastPrepare<16)return Gate::Pending;binding->lastPrepare=now;
    if(Read<void*>(weapon,0x210)!=binding->pawn||Read<void*>(weapon,0x4a8)!=binding->pawn){
        Fail(2429,"all_skins_original_weapon_owner_not_bound");return Gate::Failed;
    }
    if(!GatherAttachments(*binding))return Gate::Failed;
    if(!binding->skinAttachment||!binding->chromaAttachment){diagnostics.stage="all_skins_waiting_for_original_factory_components";return Gate::Pending;}
    bool complete=!binding->observations.empty();
    for(auto index:binding->observations){
        const auto& observed=observations[index];
        complete=complete&&observed.completed>0&&observed.completed==observed.entered&&(observed.assetTypes&7)==7;
    }
    if(!complete){diagnostics.stage="all_skins_waiting_for_original_preload_callbacks";return Gate::Pending;}
    binding->assetsReady=true;
    if(!OriginalScopeCatchup(*binding))return Gate::Failed;
    if(!binding->scopeReady){diagnostics.stage="all_skins_waiting_for_original_scope_alignment";return Gate::Pending;}
    binding->ready=true;diagnostics.stage="all_skins_original_factory_assets_and_scope_ready";return Gate::Ready;
}
inline bool BeforeRetire(void* weapon){
    if(!Thread())return false;
    original_appearance_context::ObserveRetirement();
    auto binding=FindBinding(weapon);
    if(binding){
        for(auto index:binding->observations){
            auto& observed=observations[index];
            if(observed.entered!=observed.completed)return Fail(2430,"all_skins_preload_callback_still_active_during_retire");
            if(original_effects::ResolveIdentity(observed.identity)==observed.component){
                auto actual=Read<void*>(observed.nativeInterface);
                if(actual==observed.observedTable
                    &&InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(observed.nativeInterface),observed.originalTable,actual)!=actual)
                    return Fail(2431,"all_skins_preload_interface_retire_race");
                else if(actual!=observed.observedTable&&actual!=observed.originalTable)
                    return Fail(2431,"all_skins_preload_interface_owner_changed_before_retire");
            }
            observed.retired=true;
        }
        binding->retired=true;
    }
    // Detach only our previous-generation legacy observation layers before the
    // original Actor destruction. Native EndPlay must still reach its original
    // Blueprint, delegate unbind, and AddedMeshes destruction. Releasing an
    // observation record while leaving its wrapper installed would swallow it.
    for(auto& old:original_content_services::attachments){
        auto component=old.component;
        if(!component||Read<void*>(component,0xc8)!=weapon)continue;
        if(old.callbackEntered!=old.callbackCompleted)return Fail(2432,"all_skins_legacy_preload_callback_active");
        auto actual=Read<void*>(component);
        const bool champion=component==original_champions::skinObservation.component
            &&actual==original_champions::skinObservation.observedTable;
        if(actual!=old.originalTable&&actual!=old.observedTable&&!champion)
            return Fail(2433,"all_skins_legacy_attachment_observer_owner_changed");
        if(actual!=old.originalTable&&InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(component),old.originalTable,actual)!=actual)
            return Fail(2433,"all_skins_legacy_attachment_observer_detach_race");
        old={};
    }
    if(original_champions::skinObservation.weapon==weapon){
        original_champions::skinObservation={};original_champions::state={};
        original_champions::skinComponent=nullptr;original_champions::chromaComponent=nullptr;
    }
    return true;
}
inline bool OnWeaponChanged(void*,void* weapon,const all_weapons::Definition& definition){
    auto binding=FindBinding(weapon);if(!binding||!binding->ready)return false;
    // Bindings belong to each real retained inventory actor, not to the latest
    // spawn. Thus native 1/2/3/scroll switches may return to any existing slot.
    if(!Same(binding->weaponId,definition.id)||Read<void*>(weapon,0xfa8)!=definition.dataAsset
        ||Read<void*>(weapon,0x210)!=binding->pawn||Read<void*>(weapon,0x4a8)!=binding->pawn
        ||Read<void*>(weapon,0xfb0)!=binding->skinData||Read<void*>(weapon,0xfb8)!=binding->levelData
        ||Read<void*>(weapon,0xfd0)!=binding->chromaData)
        return Fail(2435,"all_skins_existing_original_weapon_binding_changed");
    for(auto index:binding->observations){
        const auto& entry=observations[index];
        if(entry.retired||original_effects::ResolveIdentity(entry.identity)!=entry.component
            ||Read<void*>(entry.component,0xc8)!=weapon)
            return Fail(2436,"all_skins_existing_original_attachment_expired");
    }
    binding->animationConsumed=false;binding->blueprintScopeMatches=false;
    if(diagnostics.pending&&Same(binding->weaponId,diagnostics.requestedDefinition)
        &&Same(binding->skinId,diagnostics.requestedSkin)&&Same(binding->levelId,diagnostics.requestedLevel)
        &&Same(binding->chromaId,diagnostics.requestedChroma)){
        diagnostics.pending=false;++diagnostics.completed;diagnostics.stage="all_skins_original_skin_switch_equipped";
    }
    return true;
}
inline bool CopyTransform(const engine::Transform& value,double (&out)[10]){
    const double values[]={value.rotation[0],value.rotation[1],value.rotation[2],value.rotation[3],
        value.translation.x,value.translation.y,value.translation.z,value.scale.x,value.scale.y,value.scale.z};
    for(unsigned i=0;i<10;++i)if(!std::isfinite(values[i]))return false;
    std::memcpy(out,values,sizeof(values));return true;
}
inline bool EqualTransform(const double (&a)[10],const double (&b)[10]){
    for(unsigned i=0;i<10;++i)if(std::abs(a[i]-b[i])>=1e-5)return false;return true;
}
inline bool Tick(){
    if(!Thread())return false;
    auto binding=FindBinding(all_weapons::CurrentWeapon());if(!binding||!binding->ready)return true;
    ++binding->animationSamples;
    binding->animationConsumed=false;binding->blueprintScopeMatches=false;
    binding->scopeTransformRead=false;binding->animationTransformRead=false;
    if(!binding->scopeRequired)return true;
    if(original_effects::ResolveIdentity(binding->scopeIdentity)!=binding->scope)return true;
    // Cached original fields only. In particular this never asks for a socket,
    // bone transform, pose update, or an animation/equippable refresh.
    engine::Transform scope{},nativeScope{},bpScope{};double blueprint[10]{};
    binding->scopeTransformRead=original_players::Read(binding->scope,0xe0,scope)&&CopyTransform(scope,binding->scopeTransform);
    auto core=Read<void*>(binding->pawn,0xf30);binding->coreInstance=Read<void*>(core,0xa70);
    binding->animationState=Read<void*>(binding->coreInstance,0x390);
    if(!original_players::IsA(binding->animationState,0x63cef20))return true;
    binding->animationTransformRead=original_players::Read(binding->animationState,0xa50,nativeScope)
        &&CopyTransform(nativeScope,binding->animationTransform);
    auto type=Read<void*>(binding->coreInstance,0x10);
    const int bpOffset=original_scope::FieldOffset(type,L"ScopeTransformComponentSpace");
    const int lastOffset=original_scope::FieldOffset(type,L"LastEquippable");
    binding->blueprintScopeMatches=bpOffset>=0&&original_players::Read(binding->coreInstance,static_cast<size_t>(bpOffset),bpScope)
        &&CopyTransform(bpScope,blueprint)&&EqualTransform(binding->scopeTransform,blueprint);
    binding->animationConsumed=binding->scopeTransformRead&&!original_scope::Identity(scope)
        &&binding->animationTransformRead&&EqualTransform(binding->scopeTransform,binding->animationTransform)
        &&binding->blueprintScopeMatches&&lastOffset>=0
        &&Read<void*>(binding->coreInstance,static_cast<size_t>(lastOffset))==binding->weapon
        &&Read<void*>(binding->animationState,0xcf0)==binding->weapon;
    return true;
}
inline void WriteGuid(FILE* file,const Guid& id){
    std::fprintf(file,"\"%08x-%04x-%04x-%04x-%04x%08x\"",id.words[0],id.words[1]>>16,id.words[1]&0xffff,
        id.words[2]>>16,id.words[2]&0xffff,id.words[3]);
}
inline void WriteTransform(FILE* file,const double (&value)[10]){
    std::fprintf(file,"[");for(unsigned i=0;i<10;++i)std::fprintf(file,"%s%.9g",i?",":"",value[i]);std::fprintf(file,"]");
}
inline bool CatalogFailure(CatalogValidation& out,unsigned code,const char* stage,
    const Guid& skin=Guid{},const Guid& level=Guid{},const Guid& chroma=Guid{}){
    ++out.failures;
    if(!out.error){out.error=code;out.stage=stage;out.firstSkin=skin;out.firstLevel=level;out.firstChroma=chroma;}
    return false;
}
inline bool CatalogSoftCDO(void* reference,void*& cdo,bool& present){
    cdo=nullptr;present=false;if(!reference)return false;
    auto type=original_scope::ResolveLoadedSoft(reference);
    if(type){
        // The initialized ContentIndex stores these actual existing CDOs.
        // Do not create a missing CDO as part of this read-only audit.
        present=true;cdo=Read<void*>(type,0x120);return cdo&&Read<void*>(cdo,0x10)==type;
    }
    struct P{uint64_t reference[6]{};uint8_t result=0;uint8_t padding[7]{};} p;
    static_assert(sizeof(P)==56);
    auto library=Read<void*>(engine::Class(0x45aa110),0x120);
    if(!original_players::Read(reference,0,p.reference)
        ||!Event(library,L"/Script/Engine.KismetSystemLibrary.IsValidSoftClassReference",49,&p))return false;
    present=p.result!=0;return !present;
}
inline bool CatalogResolve(CatalogValidation& out,const all_weapons::Definition& definition,
    const Guid& skinId,const Guid& levelId,const Guid& chromaId,void* skin,void* level,void* chroma){
    const Diagnostics before=diagnostics;Selection selected;
    const bool resolved=Resolve(definition,skinId,levelId,chromaId,selected);
    const unsigned resolveError=diagnostics.error;const char* resolveStage=diagnostics.stage;
    // Resolve only reads existing native/CDO identities. Its failure-report
    // fields belong to normal switching; preserve them around this audit.
    diagnostics=before;
    if(!resolved)return CatalogFailure(out,resolveError?resolveError:2468,resolveStage,skinId,levelId,chromaId);
    return selected.selected&&selected.skinData==skin&&selected.levelData==level&&selected.chromaData==chroma
        &&Same(selected.weapon,definition.id)
        ?true:CatalogFailure(out,2468,"all_skins_catalog_resolve_returned_other_identity",skinId,levelId,chromaId);
}
inline bool ValidateCatalogContents(CatalogValidation& out){
    auto index=original_content::State().index;
    out.expectedSkins=original_content::MapCount(index,0x150);
    out.expectedLevels=original_content::MapCount(index,0x1a0);
    out.expectedChromas=original_content::MapCount(index,0x240);
    if(out.expectedSkins<=0||out.expectedSkins>16384||out.expectedLevels<=0||out.expectedLevels>65536
        ||out.expectedChromas<=0||out.expectedChromas>65536)
        return CatalogFailure(out,2461,"all_skins_catalog_original_map_counts_invalid");
    struct PointerArray{
        void** data=nullptr;int count=0,capacity=0;
        ~PointerArray(){if(data)engine::function<void(*)(void*)>(0x1915720)(data);}
    } skins;
    static_assert(sizeof(PointerArray)==16);
    // Native GetAllSkins: map150, native element stridee8, CDO+d8.
    // Complete pinned body: work/all-skins/native/function-0x71c57d0.json.
    engine::function<void*(*)(void*,PointerArray*)>(0x71c57d0)(index,&skins);
    out.nativeEnumeration=true;
    if(skins.count!=out.expectedSkins||skins.capacity<skins.count||skins.capacity>65536||!skins.data)
        return CatalogFailure(out,2462,"all_skins_catalog_original_skin_enumeration_inconsistent");
    std::unordered_set<void*> seenSkins,seenLevels,seenChromas;
    seenSkins.reserve(static_cast<size_t>(out.expectedSkins));
    seenLevels.reserve(static_cast<size_t>(out.expectedLevels));seenChromas.reserve(static_cast<size_t>(out.expectedChromas));
    for(int n=0;n<skins.count;++n){
        ++out.enumeratedSkins;const auto failuresBefore=out.failures;
        auto skin=Read<void*>(skins.data,static_cast<size_t>(n)*8);const Guid skinId=Read<Guid>(skin,0x38);
        if(!original_players::IsA(skin,0x676d6b0)||!all_skins::Valid(skinId)||Skin(skinId)!=skin
            ||!seenSkins.insert(skin).second){
            CatalogFailure(out,2463,"all_skins_catalog_skin_identity_or_uniqueness_failed",skinId);++out.failedSkins;continue;
        }
        ++out.verifiedSkins;void* parent=nullptr;bool present=false;
        if(!CatalogSoftCDO(static_cast<uint8_t*>(skin)+0x58,parent,present)||!present
            ||!original_players::IsA(parent,0x6638d00)){
            CatalogFailure(out,2464,"all_skins_catalog_original_parent_weapon_unresolved",skinId);++out.failedSkins;continue;
        }
        const auto definition=all_weapons::Find(Read<Guid>(parent,0x38));
        auto nativeDefinition=definition?engine::function<void*(*)(void*,const Guid*)>(0x71ce540)(index,&definition->id):nullptr;
        if(!definition||!definition->identityVerified||definition->dataAsset!=parent||Read<void*>(nativeDefinition,0xa0)!=parent){
            CatalogFailure(out,2464,"all_skins_catalog_original_parent_weapon_mismatch",skinId);++out.failedSkins;continue;
        }
        ++out.verifiedWeaponLinks;
        void* levels[256]{};void* chromas[257]{};unsigned levelCount=0,chromaCount=0;
        for(unsigned group=0;group<2;++group){
            struct SoftArray{uint8_t* data=nullptr;int count=0,capacity=0;} refs;
            const size_t offset=group?0xc8:0x88;
            if(!original_players::Read(skin,offset,refs)||refs.count<0||refs.count>256
                ||refs.capacity<refs.count||refs.capacity>4096||(!refs.data&&refs.count)){
                CatalogFailure(out,2465,"all_skins_catalog_authored_soft_array_invalid",skinId);continue;
            }
            for(int j=0;j<refs.count;++j){
                void* data=nullptr;bool linked=false;
                if(!CatalogSoftCDO(refs.data+static_cast<size_t>(j)*48,data,linked)){
                    CatalogFailure(out,2466,"all_skins_catalog_non_null_soft_reference_unresolved",skinId);continue;
                }
                if(!linked){if(group)++out.nullChromaLinks;else ++out.nullLevelLinks;continue;}
                const Guid id=Read<Guid>(data,0x38);
                if(!all_skins::Valid(id)||!original_players::IsA(data,group?0x676eaa0:0x676f1b0)
                    ||(group?Chroma(id):Level(id))!=data){
                    CatalogFailure(out,2467,"all_skins_catalog_level_or_chroma_index_identity_failed",skinId,group?Guid{}:id,group?id:Guid{});continue;
                }
                if(group){
                    ++out.chromaLinks;seenChromas.insert(data);bool duplicate=false;
                    for(unsigned k=0;k<chromaCount;++k)duplicate=duplicate||chromas[k]==data;
                    if(duplicate)++out.duplicateLinks;else chromas[chromaCount++]=data;
                }else{
                    ++out.levelLinks;seenLevels.insert(data);bool duplicate=false;
                    for(unsigned k=0;k<levelCount;++k)duplicate=duplicate||levels[k]==data;
                    if(duplicate)++out.duplicateLinks;else levels[levelCount++]=data;
                }
            }
        }
        void* defaultChroma=nullptr;bool hasDefault=false;
        const bool defaultValid=CatalogSoftCDO(static_cast<uint8_t*>(skin)+0x98,defaultChroma,hasDefault)&&hasDefault
            &&original_players::IsA(defaultChroma,0x676eaa0)&&Chroma(Read<Guid>(defaultChroma,0x38))==defaultChroma;
        if(!defaultValid)
            CatalogFailure(out,2469,"all_skins_catalog_original_default_chroma_invalid",skinId);
        else{
            seenChromas.insert(defaultChroma);bool listed=false;
            for(unsigned k=0;k<chromaCount;++k)listed=listed||chromas[k]==defaultChroma;
            if(!listed)chromas[chromaCount++]=defaultChroma;
        }
        if(!levelCount||!chromaCount||!defaultValid)
            CatalogFailure(out,2469,"all_skins_catalog_no_valid_original_default_choice",skinId);
        else if(out.failures==failuresBefore){
            ++out.defaultChoices;
            if(CatalogResolve(out,*definition,skinId,Guid{},Guid{},skin,levels[0],defaultChroma))++out.verifiedDefaultChoices;
            for(unsigned l=0;l<levelCount;++l)for(unsigned c=0;c<chromaCount;++c){
                ++out.combinations;const Guid levelId=Read<Guid>(levels[l],0x38),chromaId=Read<Guid>(chromas[c],0x38);
                if(CatalogResolve(out,*definition,skinId,levelId,chromaId,skin,levels[l],chromas[c]))++out.verifiedCombinations;
            }
        }
        if(out.failures!=failuresBefore)++out.failedSkins;
    }
    out.uniqueLevels=static_cast<unsigned>(seenLevels.size());out.uniqueChromas=static_cast<unsigned>(seenChromas.size());
    out.complete=out.enumeratedSkins==static_cast<unsigned>(out.expectedSkins);
    out.coverageComplete=seenSkins.size()==static_cast<size_t>(out.expectedSkins)
        &&seenLevels.size()==static_cast<size_t>(out.expectedLevels)&&seenChromas.size()==static_cast<size_t>(out.expectedChromas);
    if(!out.coverageComplete)CatalogFailure(out,2470,"all_skins_catalog_original_index_coverage_incomplete");
    out.catalogStable=original_content::MapCount(index,0x150)==out.expectedSkins
        &&original_content::MapCount(index,0x1a0)==out.expectedLevels&&original_content::MapCount(index,0x240)==out.expectedChromas;
    if(!out.catalogStable)CatalogFailure(out,2471,"all_skins_catalog_index_changed_during_validation");
    return out.complete&&out.coverageComplete&&out.failures==0;
}
inline bool ValidateCatalog(CatalogValidation& out){
    out={};const auto start=GetTickCount64();
    if(!engine::image||original_players::gameThread!=GetCurrentThreadId()||!diagnostics.initialized
        ||!all_weapons::State().initialized||all_weapons::State().busy||!original_content::State().initialized
        ||!original_content::State().shouldStoreCDOs)
        return CatalogFailure(out,2460,"all_skins_catalog_requires_idle_initialized_game_thread");
    const Diagnostics saved=diagnostics;const auto selectionCount=selections.size(),bindingCount=bindings.size(),observationCount=observations.size();
    auto current=all_weapons::CurrentWeapon();const auto generation=all_weapons::State().generation,requests=all_weapons::State().requestSequence;
    const bool oldReady=all_weapons::State().ready,oldSkinReady=all_weapons::State().skinReady;
    const bool valid=ValidateCatalogContents(out);
    out.stateUnchanged=selections.size()==selectionCount&&bindings.size()==bindingCount&&observations.size()==observationCount
        &&all_weapons::CurrentWeapon()==current&&all_weapons::State().generation==generation&&all_weapons::State().requestSequence==requests
        &&all_weapons::State().ready==oldReady&&all_weapons::State().skinReady==oldSkinReady
        &&diagnostics.requests==saved.requests&&diagnostics.completed==saved.completed&&diagnostics.generation==saved.generation
        &&diagnostics.preloadRequests==saved.preloadRequests&&diagnostics.preloadCallbacks==saved.preloadCallbacks&&diagnostics.pending==saved.pending;
    diagnostics=saved;
    if(!out.stateUnchanged)CatalogFailure(out,2472,"all_skins_catalog_runtime_switch_state_changed");
    out.elapsedMs=GetTickCount64()-start;out.passed=valid&&out.stateUnchanged;
    if(out.passed)out.stage="all_skins_original_catalog_and_all_authored_combinations_verified";
    return out.passed;
}
inline bool WriteCatalogValidation(const wchar_t* path,const CatalogValidation& out){
    if(!path||!*path)return false;std::wstring temporary(path);temporary+=L".tmp";
    FILE* file=nullptr;if(_wfopen_s(&file,temporary.c_str(),L"wb")||!file)return false;
    std::fprintf(file,"{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"passed\":%d,\"complete\":%d,\"native_enumeration\":%d,\"coverage_complete\":%d,\"catalog_stable\":%d,\"state_unchanged\":%d,\"elapsed_ms\":%llu,\"expected_skins\":%d,\"expected_levels\":%d,\"expected_chromas\":%d,\"enumerated_skins\":%u,\"verified_skins\":%u,\"verified_weapon_links\":%u,\"level_links\":%u,\"chroma_links\":%u,\"unique_levels\":%u,\"unique_chromas\":%u,\"authored_null_level_links\":%u,\"authored_null_chroma_links\":%u,\"duplicate_links\":%u,\"default_choices\":%u,\"verified_default_choices\":%u,\"combinations\":%u,\"verified_combinations\":%u,\"failures\":%u,\"failed_skins\":%u,\"first_skin_guid\":",
        out.stage,out.error,out.passed,out.complete,out.nativeEnumeration,out.coverageComplete,out.catalogStable,out.stateUnchanged,
        static_cast<unsigned long long>(out.elapsedMs),out.expectedSkins,out.expectedLevels,out.expectedChromas,out.enumeratedSkins,out.verifiedSkins,
        out.verifiedWeaponLinks,out.levelLinks,out.chromaLinks,out.uniqueLevels,out.uniqueChromas,out.nullLevelLinks,out.nullChromaLinks,out.duplicateLinks,
        out.defaultChoices,out.verifiedDefaultChoices,out.combinations,out.verifiedCombinations,out.failures,out.failedSkins);
    WriteGuid(file,out.firstSkin);std::fprintf(file,",\"first_level_guid\":");WriteGuid(file,out.firstLevel);
    std::fprintf(file,",\"first_chroma_guid\":");WriteGuid(file,out.firstChroma);
    std::fprintf(file,",\"actors_created\":0,\"preload_or_equip_called\":false,\"ready_flags_written\":false,\"render_or_asset_preload_certified\":false}\n");
    const bool valid=std::ferror(file)==0;const bool closed=std::fclose(file)==0;
    return valid&&closed&&MoveFileExW(temporary.c_str(),path,MOVEFILE_REPLACE_EXISTING);
}
inline bool WriteDiagnostics(const wchar_t* path){
    FILE* file=nullptr;if(!path||_wfopen_s(&file,path,L"wb")||!file)return false;
    std::fprintf(file,"{\"schema_version\":3,\"stage\":\"%s\",\"error\":%u,\"initialized\":%d,\"pending\":%d,\"requests\":%u,\"completed\":%u,\"generation\":%u,\"forced_arrays\":%u,\"preload_requests\":%u,\"preload_callbacks\":%u,\"empty_preload_original_completions\":%u,\"binding_count\":%zu,\"manual_pose_or_material_writes\":false,\"diagnostic_socket_or_bone_getters_called\":false,\"bindings\":[",
        diagnostics.stage,diagnostics.error,diagnostics.initialized,diagnostics.pending,diagnostics.requests,diagnostics.completed,
        diagnostics.generation,diagnostics.forcedArrays,diagnostics.preloadRequests,diagnostics.preloadCallbacks,diagnostics.emptyCompletions,bindings.size());
    size_t n=0;
    for(const auto& b:bindings){
        std::fprintf(file,"%s{\"generation\":%u,\"weapon_guid\":",n++?",":"",b.generation);WriteGuid(file,b.weaponId);
        std::fprintf(file,",\"skin_guid\":");WriteGuid(file,b.skinId);std::fprintf(file,",\"level_guid\":");WriteGuid(file,b.levelId);
        std::fprintf(file,",\"chroma_guid\":");WriteGuid(file,b.chromaId);
        std::fprintf(file,",\"weapon\":\"%p\",\"pawn\":\"%p\",\"skin_attachment\":\"%p\",\"chroma_attachment\":\"%p\",\"scope\":\"%p\",\"prepared\":%d,\"assets_ready\":%d,\"scope_required\":%d,\"scope_ready\":%d,\"ready\":%d,\"retired\":%d,\"forced_count\":%u,\"authored_null_forced_count\":%u,\"reflex_client_catchups\":%u,\"animation_samples\":%llu,\"scope_transform_read\":%d,\"animation_transform_read\":%d,\"blueprint_scope_matches\":%d,\"animation_consumed\":%d,\"scope_transform\":",
            b.weapon,b.pawn,b.skinAttachment,b.chromaAttachment,b.scope,b.prepared,b.assetsReady,b.scopeRequired,b.scopeReady,b.ready,b.retired,
            b.forcedCount,b.forcedNullCount,b.reflexCatchups,static_cast<unsigned long long>(b.animationSamples),b.scopeTransformRead,b.animationTransformRead,b.blueprintScopeMatches,b.animationConsumed);
        WriteTransform(file,b.scopeTransform);std::fprintf(file,",\"native_gun_to_scope\":");WriteTransform(file,b.animationTransform);
        std::fprintf(file,",\"preloads\":[");size_t k=0;
        for(auto index:b.observations){const auto& p=observations[index];
            std::fprintf(file,"%s{\"component\":\"%p\",\"requested\":%d,\"entered\":%u,\"completed\":%u,\"asset_types\":%u,\"retired\":%d,\"reflex_caught_up\":%d,\"request_returned\":%d,\"path_calls\":%u,\"path_set_observed\":%d,\"path_set_valid\":%d,\"path_allocated\":%d,\"path_free\":%d,\"path_count\":%d,\"native_loaded_queried\":%d,\"native_loaded\":%d,\"empty_original_completion\":%d}",
                k++?",":"",p.component,p.requested,p.entered,p.completed,static_cast<unsigned>(p.assetTypes),p.retired,p.reflexCaughtUp,
                p.requestReturned,p.pathCalls,p.pathSetObserved,p.pathSetValid,p.pathAllocated,p.pathFree,p.pathCount,p.nativeLoadedQueried,p.nativeLoaded,p.emptyCompletion);
        }
        std::fprintf(file,"]}");
    }
    std::fprintf(file,"],\"appearance_context\":");original_appearance_context::WriteJson(file);
    std::fprintf(file,"}\n");const bool valid=std::ferror(file)==0;std::fclose(file);return valid;
}
}
