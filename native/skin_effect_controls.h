#pragma once
#include "original_weapon.h"
#include "original_effects.h"
#include "training_champion_state.h"
#include <array>
#include <cmath>

// Local cosmetic controls for REAL original skin data. No shader/material/color
// is written here. Generic switches use native OverrideEffect and its real map;
// Champion switches enter only audited original Blueprint client presentation
// leaves which Standalone otherwise skips behind IsServer/CheckLocalOnly.
namespace skin_effect_controls {
inline constexpr size_t MaxControls=98,MaxSettings=256,MaxEffects=16;
inline constexpr uintptr_t EffectClass=0x648ada0;
enum class Mode:uint32_t {FollowOriginal=0,ForceOn=1,ForceOff=2};
enum class Kind:uint8_t {ChampionGlow,ChampionKillCount,OriginalEffectOverride};
struct Binding {
    void *world=nullptr,*pawn=nullptr,*weapon=nullptr,*skinData=nullptr,*levelData=nullptr,*chromaData=nullptr;
    void *skinAttachment=nullptr,*chromaAttachment=nullptr;
    uint64_t generation=0;
    bool assetsReady=false;
};
struct Control {
    char key[80]{},label[160]{},basePath[768]{},skinPath[768]{};
    Kind kind=Kind::OriginalEffectOverride;
    Mode requested=Mode::FollowOriginal,applied=Mode::FollowOriginal;
    float value=0,minimum=0,maximum=0;
    bool supported=false,available=false,appliedVerified=false,dirty=false,numeric=false,nextOriginalEvent=false;
    void *baseClass=nullptr,*skinClass=nullptr,*originalRuleClass=nullptr,*lastAppliedClass=nullptr;
    uint64_t applyCount=0;
    const char* reason="not_bound";
};
struct Setting {uint64_t skinScope=0;char key[80]{};Mode mode=Mode::FollowOriginal;float value=0;};
struct Profile {
    const wchar_t* package;
    const char* family;
    const wchar_t* eventName;
    const wchar_t* ownerField;
    const wchar_t* materialOne;
    const wchar_t* materialTwo;
    int32_t initializeOne,initializeTwo,update;
    uint16_t eventSize;
    int32_t memoryBytes;
    uint8_t entryOpcodes[3];
};
inline constexpr Profile profiles[]{
 {L"/Game/Equippables/Guns/Rifles/AK/Champions/FXC/FXC_AK_Champions_Champion_Effect","/AK/Champions/",L"Custom Champion Updated",L"Owned Gunskin",L"Dynamic Material",L"Outline Material",2801,0,2381,8,6409,{0x4c,0,0x0f}},
 {L"/Game/Equippables/Guns/Rifles/Carbine/Champions2/FXC/FXC_Carbine_Champions2_WhileEquipped_1P_Smoke_LV4","/Carbine/Champions2/",L"Custom Champion Updated",L"Owned Gunskin",L"Glass Dynamic Mat",L"Outline Material Fresnel",2148,0,7415,8,9434,{0x4c,0,0x0f}},
 {L"/Game/Equippables/Guns/Rifles/AK/Champions2023/FXC/FXC_AK_WhileEquipped_1P_Champions2023_Smoke_LV4","/AK/Champions2023/",L"Custom Champion Updated",L"My Gun Skin",L"Float Spark Mat",L"Outline Material",1085,1481,4986,8,9226,{0x0f,0x4c,0x5f}},
 {L"/Game/Equippables/Guns/Rifles/Carbine/Champions2024/FXC/FXC_Carbine_Champions2024_WhileEquipped_1P_Smoke_LV4","/Carbine/Champions2024/",L"Custom Champion Updated",L"My Gun Skin",L"Champ Aura Mat",L"Outline Material",717,2563,10010,8,14799,{0x0f,0x4c,0x5f}},
 {L"/Game/Equippables/Guns/Rifles/Carbine/Champions2026/FXC/FXC_Carbine_Champions2026_WhileEquipped_1P_Smoke_Lv4_Attacker","/Carbine/Champions2026/",L"Team Champion Updated",L"My Gun Skin",L"Champ Aura Mat",L"Champ Outline Mat",1783,0,4521,16,5922,{0x4c,0,0x14}},
 {L"/Game/Equippables/Guns/Rifles/Carbine/Champions2026/FXC/FXC_Carbine_Champions2026_WhileEquipped_1P_Smoke_Lv4_Defender","/Carbine/Champions2026/",L"Champion Updated",L"My Gun Skin",L"Champ Aura Mat",L"Champ Outline Mat",1965,0,7517,8,7723,{0x4c,0,0x14}}
};
struct Effect {
    void* actor=nullptr;
    original_effects::WeakIdentity identity{};
    std::array<uint8_t,22> nativeEffectId{};
    size_t profile=0;
    bool occupied=false,initializeAttempted=false,initialized=false,lastDispatchValid=false;
    bool parentInitializeAttempted=false,parentInitialized=false,sourceOwnershipVerified=false,presentationTargetVerified=false;
    bool originalChampionDependencyVerified=false;
    void *observedActorOwner=nullptr,*observedEquippable=nullptr,*observedOriginalTarget=nullptr,*observedSkinOwner=nullptr;
    void *observedPresentationTarget=nullptr,*observedMaterialOne=nullptr,*observedMaterialTwo=nullptr,*observedChampionComponent=nullptr;
    int32_t observedVfxMeshCount=-1;
    const char* initializeStage="original_effect_not_initialized";
    void *lastChampion=nullptr,*lastAttacking=nullptr,*lastDefending=nullptr;
    Mode lastMode=Mode::FollowOriginal;
    uint64_t dispatches=0,playbackEpochs=0;
    int32_t lastCosmeticKills=-1;
};
struct Diagnostics {
    bool bound=false,assetsReady=false,contextValid=false,snapshotReady=false,changingBinding=false,restartRequired=false;
    bool nativeOverrideAbi=false,reflectionAnchorVerified=false,originalChampionKnown=false;
    uint32_t error=0,commandError=0;
    int32_t expectedScriptBytes=0,actualScriptBytes=0,lastScriptEntry=0;
    bool originalScriptEntryVerified=false;
    uint64_t samples=0,bindingChanges=0,commands=0,originalMapUpdates=0,originalClientInitializations=0,originalChampionUpdates=0;
    uint64_t scope=0,exceptionRva=0;
    size_t controls=0,originalMapCount=0,effects=0;
    const char* stage="skin_effect_controls_unbound";
    const char* commandResult="none";
    wchar_t failedEvent[512]{};
    uint16_t expectedEventSize=0,actualEventSize=0;
};
inline Binding binding;
inline Diagnostics state;
inline std::array<Control,MaxControls> controls{};
inline std::array<Setting,MaxSettings> settings{};
inline std::array<Effect,MaxEffects> effects{};
inline original_effects::WeakIdentity weaponIdentity{},pawnIdentity{},skinIdentity{};
inline uint64_t lastScan=0,lastMapPoll=0;
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline bool Thread(){return !state.restartRequired&&original_players::Thread();}
inline uint64_t Hash(const char* text){uint64_t h=14695981039346656037ULL;for(;text&&*text;++text){h^=static_cast<uint8_t>(*text);h*=1099511628211ULL;}return h;}
inline bool CommandFailure(uint32_t code,const char* why){state.commandError=code;state.commandResult=why;return false;}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* parameters,void* expectedOuter=nullptr){
    auto fn=engine::Find(name);const auto actual=Read<uint16_t>(fn,0xbe);
    if(!object||!fn||actual!=size||(expectedOuter&&Read<void*>(fn,0x28)!=expectedOuter)){
        if(name)wcsncpy_s(state.failedEvent,name,_TRUNCATE);state.expectedEventSize=size;state.actualEventSize=actual;return false;
    }
    auto table=Read<void**>(object,0);auto call=Read<void*>(table,0x298);if(!call)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(call)(object,fn,parameters);return true;
}
template<size_t N> inline bool Path(void* object,char (&output)[N]){
    output[0]=0;struct Query{void* object;wchar_t* data=nullptr;int32_t count=0,capacity=0;} q{object};
    static_assert(sizeof(Query)==24);
    if(!Event(engine::DefaultObject(engine::Class(0x45aa110)),L"/Script/Engine.KismetSystemLibrary.GetPathName",24,&q))return false;
    bool ok=q.count>0&&q.count<=4096&&q.capacity>=q.count&&original_players::Readable(q.data,static_cast<size_t>(q.count)*2)&&q.data[q.count-1]==0;
    if(ok){const auto count=WideCharToMultiByte(CP_UTF8,0,q.data,q.count-1,output,static_cast<int>(N-1),nullptr,nullptr);ok=count>0;if(ok)output[count]=0;}
    if(q.data)engine::function<void(*)(void*)>(0x1915720)(q.data);return ok;
}
inline void* Field(void* object,const wchar_t* wanted){
    if(!object||!wanted)return nullptr;const auto target=original_players::Utf8(wanted);
    auto type=Read<void*>(object,0x10);
    // Exact native iterator3c69260 layout, without its object-only type filter:
    // UStruct.ChildProperties58, FField.Next18/Name20, native Super virtual2f8.
    // Offset_Internal44 is additionally checked against native EffectOverrides.
    for(unsigned depth=0;type&&depth<128;++depth){
        auto field=Read<void*>(type,0x58);
        for(unsigned count=0;field&&count<4096;++count){
            char name[128]{};original_weapon::OriginalNameValue(static_cast<uint8_t*>(field)+0x20,name);
            if(target==name)return field;
            auto next=Read<void*>(field,0x18);if(next==field)return nullptr;field=next;
        }
        auto table=Read<void**>(type,0);auto query=Read<void*>(table,0x2f8);if(!query)return nullptr;
        auto next=reinterpret_cast<void*(*)(void*)>(query)(type);if(next==type)return nullptr;type=next;
    }
    return nullptr;
}
inline bool ObjectField(void* object,const wchar_t* name,void*& value){
    value=nullptr;auto property=Field(object,name);if(!property)return false;
    const auto flags=Read<uint64_t>(Read<void*>(property,8),0x18);
    if((flags&0x4008001ULL)!=0x4008001ULL)return false;
    const auto offset=Read<int32_t>(property,0x44),size=Read<int32_t>(Read<void*>(object,0x10),0x60);
    if(offset<0x30||size<offset+8||size>2*1024*1024)return false;
    value=Read<void*>(object,static_cast<size_t>(offset));return true;
}
inline std::wstring TypePath(const Profile& p){std::wstring package=p.package;const auto slash=package.find_last_of(L'/');return package+L"."+package.substr(slash+1)+L"_C";}
inline bool Graph(void* actor,const Profile& p,int32_t entry){
    const auto path=TypePath(p);const auto leaf=path.substr(path.find_last_of(L'.')+1);const auto name=path+L".ExecuteUbergraph_"+leaf.substr(0,leaf.size()-2);
    auto fn=engine::Find(name.c_str());auto code=Read<const uint8_t*>(fn,0x68);
    state.expectedScriptBytes=p.memoryBytes;state.actualScriptBytes=Read<int32_t>(fn,0x70);state.lastScriptEntry=entry;
    uint8_t expected=entry==p.initializeOne?p.entryOpcodes[0]:entry==p.initializeTwo?p.entryOpcodes[1]:entry==p.update?p.entryOpcodes[2]:0;
    if(&p==&profiles[1]&&entry==7600)expected=0x0f;
    state.originalScriptEntryVerified=fn&&Read<void*>(fn,0x28)==Read<void*>(actor,0x10)
        &&state.actualScriptBytes==p.memoryBytes&&entry>=0&&entry<p.memoryBytes&&expected
        &&original_players::Readable(code,static_cast<size_t>(p.memoryBytes))&&code[0]==0x4c&&code[5]==0x4e&&code[entry]==expected;
    if(!state.originalScriptEntryVerified)return false;
    return Event(actor,name.c_str(),4,&entry,Read<void*>(actor,0x10));
}
inline bool Champion2024ParentGraph(void* actor,int32_t entry){
    // This inherited Lv2 original StartEffect owns My Gun Skin and My FXC
    // Target. Both assignments are skipped behind original client-only gates
    // in Standalone. Never initialize a replacement material or target here.
    constexpr auto parent=L"/Game/Equippables/Guns/Rifles/Carbine/Champions2024/FXC/FXC_Carbine_Champions2024_WhileEquipped_1P_Smoke_Lv2.FXC_Carbine_Champions2024_WhileEquipped_1P_Smoke_Lv2_C";
    constexpr auto function=L"/Game/Equippables/Guns/Rifles/Carbine/Champions2024/FXC/FXC_Carbine_Champions2024_WhileEquipped_1P_Smoke_Lv2.FXC_Carbine_Champions2024_WhileEquipped_1P_Smoke_Lv2_C.ExecuteUbergraph_FXC_Carbine_Champions2024_WhileEquipped_1P_Smoke_Lv2";
    auto type=engine::Find(parent),fn=engine::Find(function);auto code=Read<const uint8_t*>(fn,0x68);
    state.expectedScriptBytes=426;state.actualScriptBytes=Read<int32_t>(fn,0x70);state.lastScriptEntry=entry;
    state.originalScriptEntryVerified=(entry==96||entry==366)&&type&&fn&&Read<void*>(fn,0x28)==type
        &&original_players::Derived(Read<void*>(actor,0x10),type)&&state.actualScriptBytes==426
        &&original_players::Readable(code,426)&&code[0]==0x4e&&code[entry]==0x5f&&code[425]==0x53;
    return state.originalScriptEntryVerified&&Event(actor,function,4,&entry,type);
}
inline void* QueryOverride(void* weapon,void* base){
    struct Query{void* base;void* result=nullptr;} q{base};
    if(!Event(weapon,L"/Script/ShooterGame.AresEquippable.GetOverridableEffect",16,&q))return nullptr;return q.result;
}
inline int32_t MapCount(const void* map){
    const auto count=Read<int32_t>(map,8),capacity=Read<int32_t>(map,12),bits=Read<int32_t>(map,0x28),free=Read<int32_t>(map,0x34);
    return count<0||count>2048||capacity<count||bits<count||free<0||free>count?-1:count-free;
}
inline void* BaseClass(const void* entry){
    alignas(8) std::array<uint8_t,48> temporary{};std::memcpy(temporary.data(),entry,temporary.size());
    // Original soft-class resolver, on a borrowed temporary so no original map
    // cache bytes are changed. Invoke only after the real attachment preload.
    return engine::function<void*(*)(void*)>(0x1c96db0)(temporary.data());
}
inline void* MapEntry(void* weapon,void* base){
    auto map=static_cast<uint8_t*>(weapon)+0xcb0;const auto live=MapCount(map);if(live<0)return nullptr;
    const auto count=Read<int32_t>(map,8);auto data=Read<uint8_t*>(map,0);auto bits=Read<const uint32_t*>(map,0x20);
    if(!bits)bits=reinterpret_cast<const uint32_t*>(map+0x10);
    if(count&&(!original_players::Readable(data,static_cast<size_t>(count)*64)||!original_players::Readable(bits,static_cast<size_t>((count+31)/32)*4)))return nullptr;
    for(int32_t i=0;i<count;++i)if((bits[i/32]&(1U<<(i%32)))&&BaseClass(data+static_cast<size_t>(i)*64)==base)return data+static_cast<size_t>(i)*64;
    return nullptr;
}
inline bool SetMap(Control& control,void* target){
    auto entry=MapEntry(binding.weapon,control.baseClass);if(!entry||!target)return false;
    struct Query{alignas(8) uint8_t base[48]{};void* target=nullptr;} q;
    static_assert(sizeof(Query)==56&&offsetof(Query,target)==48);
    std::memcpy(q.base,entry,48);q.target=target;
    if(!Event(binding.weapon,L"/Script/ShooterGame.AresEquippable.OverrideEffect",56,&q))return false;
    if(QueryOverride(binding.weapon,control.baseClass)!=target)return false;
    control.lastAppliedClass=target;++control.applyCount;++state.originalMapUpdates;return true;
}
inline Setting* Stored(const char* key,bool create){
    Setting* empty=nullptr;for(auto& s:settings){if(s.skinScope==state.scope&&std::strcmp(s.key,key)==0)return &s;if(!s.skinScope&&!empty)empty=&s;}
    if(!create||!empty)return nullptr;empty->skinScope=state.scope;strcpy_s(empty->key,key);return empty;
}
inline void RestoreRequested(Control& c){if(auto saved=Stored(c.key,false)){c.requested=saved->mode;c.value=saved->value;c.dirty=true;}}
inline bool Context(){
    state.contextValid=state.bound&&binding.world&&binding.pawn&&binding.weapon
        &&original_effects::ResolveIdentity(weaponIdentity)==binding.weapon&&original_effects::ResolveIdentity(pawnIdentity)==binding.pawn
        &&Read<void*>(binding.weapon,0x4a8)==binding.pawn&&Read<void*>(binding.weapon,0x210)==binding.pawn
        &&engine::function<int(*)(void*)>(0x3efc820)(binding.weapon)==0;
    if(binding.skinAttachment)state.contextValid=state.contextValid
        &&original_effects::ResolveIdentity(skinIdentity)==binding.skinAttachment
        &&Read<void*>(binding.skinAttachment,0xc8)==binding.weapon&&Read<void*>(binding.skinAttachment,0x28)==binding.weapon
        &&Read<void*>(binding.skinAttachment,0xf8)==binding.levelData;
    return state.contextValid;
}
inline bool Snapshot(){
    state.controls=2;controls={};auto& glow=controls[0];auto& count=controls[1];
    strcpy_s(glow.key,"champion_glow");strcpy_s(glow.label,"冠军领先发光");glow.kind=Kind::ChampionGlow;
    strcpy_s(count.key,"skin_kill_count");strcpy_s(count.label,"皮肤击杀递进");count.kind=Kind::ChampionKillCount;count.numeric=true;count.maximum=25;
    char skinPath[768]{};if(binding.skinAttachment)Path(Read<void*>(binding.skinAttachment,0x10),skinPath);
    std::string lower=skinPath;for(auto& ch:lower)if(ch>='A'&&ch<='Z')ch=static_cast<char>(ch+('a'-'A'));
    const bool originalLevel4=lower.find("_lv4_skin_c")!=std::string::npos;
    for(const auto& p:profiles)if(std::strstr(skinPath,p.family)){
        glow.supported=originalLevel4;glow.reason=originalLevel4?"waiting_for_original_owned_champion_effect":"original_champion_level4_required";
        if(std::strcmp(p.family,"/Carbine/Champions2/")==0){count.supported=originalLevel4;count.reason=glow.reason;}
    }
    RestoreRequested(glow);RestoreRequested(count);
    auto map=static_cast<uint8_t*>(binding.weapon)+0xcb0;const auto live=MapCount(map);if(live<0)return false;state.originalMapCount=static_cast<size_t>(live);
    const auto countEntries=Read<int32_t>(map,8);auto data=Read<uint8_t*>(map,0);auto bits=Read<const uint32_t*>(map,0x20);
    if(!bits)bits=reinterpret_cast<const uint32_t*>(map+0x10);
    if(countEntries&&(!original_players::Readable(data,static_cast<size_t>(countEntries)*64)||!original_players::Readable(bits,static_cast<size_t>((countEntries+31)/32)*4)))return false;
    for(int32_t i=0;i<countEntries;++i){
        if(!(bits[i/32]&(1U<<(i%32))))continue;auto entry=data+static_cast<size_t>(i)*64;
        auto base=BaseClass(entry),skin=Read<void*>(entry,0x30);
        if(!base||!skin||base==skin||!original_players::Derived(base,engine::Class(EffectClass))||!original_players::Derived(skin,engine::Class(EffectClass)))continue;
        if(state.controls==MaxControls)break;auto& c=controls[state.controls++];c.kind=Kind::OriginalEffectOverride;
        c.baseClass=base;c.skinClass=skin;c.originalRuleClass=skin;c.supported=true;c.nextOriginalEvent=true;
        if(!Path(base,c.basePath)||!Path(skin,c.skinPath)){c.reason="original_class_path_query_failed";continue;}
        sprintf_s(c.key,"fxc_%016llx",static_cast<unsigned long long>(Hash(c.basePath)));
        char leaf[128]{};original_weapon::OriginalObjectName(skin,leaf);sprintf_s(c.label,"原皮效果：%s",leaf);
        c.available=QueryOverride(binding.weapon,base)==skin;c.reason=c.available?"native_override_map_and_original_getter_verified":"original_getter_differs_from_map";
        RestoreRequested(c);
    }
    state.snapshotReady=true;return true;
}
inline bool ApplyMap(Control& c){
    if(!c.available)return false;auto actual=QueryOverride(binding.weapon,c.baseClass);
    if(!actual){c.appliedVerified=false;c.reason="original_override_getter_unavailable";return false;}
    const bool owned=c.applied!=Mode::FollowOriginal&&c.lastAppliedClass&&actual==c.lastAppliedClass;
    // Native condition changes observed outside our own last write remain the
    // authored rule, including a native return to BaseFXC. Follow does not pin
    // the initial snapshot's skin class over the live original rule.
    if(!owned){
        c.originalRuleClass=actual;c.lastAppliedClass=nullptr;
        if(actual!=c.baseClass&&actual!=c.skinClass){c.skinClass=actual;Path(actual,c.skinPath);}
    }
    if(c.requested==Mode::FollowOriginal){
        if(owned&&actual!=c.originalRuleClass&&!SetMap(c,c.originalRuleClass)){
            c.appliedVerified=false;c.reason="original_rule_restore_not_read_back";return false;
        }
        c.appliedVerified=QueryOverride(binding.weapon,c.baseClass)==c.originalRuleClass;
        if(c.appliedVerified){c.applied=Mode::FollowOriginal;c.lastAppliedClass=nullptr;c.dirty=false;c.reason="live_original_override_rule_preserved";}
        return c.appliedVerified;
    }
    auto desired=c.requested==Mode::ForceOff?c.baseClass:c.skinClass;
    if(actual!=desired&&!SetMap(c,desired)){c.appliedVerified=false;c.reason="original_override_request_not_read_back";return false;}
    c.applied=c.requested;c.appliedVerified=QueryOverride(binding.weapon,c.baseClass)==desired;c.dirty=false;
    if(c.appliedVerified)c.reason="native_override_request_and_getter_verified";return c.appliedVerified;
}
inline void* OtherPlayerState(void* local){for(const auto& p:original_players::players)if(p.state&&p.state!=local&&original_players::IsA(p.state,original_players::ShooterStateClass))return p.state;return nullptr;}
inline bool OriginalChampion(void*& global,void*& attacking,void*& defending){
    return training_champion_state::ReadChampionState(binding.world,global,attacking,defending);
}
inline Effect* Track(void* actor,size_t profile){
    original_effects::WeakIdentity identity{};if(!original_effects::CaptureIdentity(actor,identity))return nullptr;Effect* empty=nullptr;
    std::array<uint8_t,22> nativeId{};std::memcpy(nativeId.data(),static_cast<uint8_t*>(actor)+0x480,nativeId.size());
    for(auto& e:effects){
        if(e.occupied&&original_effects::ResolveIdentity(e.identity)!=e.actor)e={};
        if(e.occupied&&e.identity.index==identity.index&&e.identity.serial==identity.serial){
            if(e.nativeEffectId!=nativeId){const auto epochs=e.playbackEpochs+1;e={};e.occupied=true;e.actor=actor;e.identity=identity;e.profile=profile;e.nativeEffectId=nativeId;e.playbackEpochs=epochs;}
            return &e;
        }
        if(!e.occupied&&!empty)empty=&e;
    }
    if(!empty)return nullptr;empty->occupied=true;empty->actor=actor;empty->identity=identity;empty->profile=profile;empty->nativeEffectId=nativeId;empty->playbackEpochs=1;return empty;
}
inline bool InitializeFailure(Effect& e,const char* why){
    e.initializeStage=why;if(!controls[0].available)controls[0].reason=why;return false;
}
inline bool Champion2024Parent(Effect& e){
    e.observedActorOwner=Read<void*>(e.actor,0x210);
    ObjectField(e.actor,L"Equippable",e.observedEquippable);
    // EffectContainer.OriginalTarget620 is a real eight-byte WEAK reference,
    // not an actor pointer. Resolve it through the original weak-object API.
    e.observedOriginalTarget=original_effects::ResolveIdentity(Read<original_effects::WeakIdentity>(e.actor,0x620));
    ObjectField(e.actor,L"My Gun Skin",e.observedSkinOwner);
    ObjectField(e.actor,L"My FXC Target",e.observedPresentationTarget);
    // This transient native cache is observed null in the real local client
    // lifecycle. It is not the source used by the original Lv2 assignments.
    // A present cache must agree; the original StartEffect frame target is
    // independently required to read back as the actual Pawn after leaf366.
    e.sourceOwnershipVerified=e.observedActorOwner==binding.pawn&&e.observedEquippable==binding.weapon
        &&(!e.observedOriginalTarget||e.observedOriginalTarget==binding.pawn);
    if(!e.sourceOwnershipVerified)return InitializeFailure(e,"champion2024_original_owner_equippable_or_weak_target_mismatch");
    void* inventory=nullptr;void* current=nullptr;
    if(!ObjectField(binding.pawn,L"Inventory",inventory)||!inventory
       ||!Event(inventory,L"/Script/ShooterGame.AresInventory.GetCurrentEquippable",8,&current)||current!=binding.weapon)
        return InitializeFailure(e,"champion2024_original_parent_current_inventory_mismatch");
    struct Query{void* type;void* component=nullptr;} q{engine::Find(L"/Game/Personalization/PersonalizationComponents/Weapon_Skin_Base.Weapon_Skin_Base_C")};
    if(!q.type||!Event(binding.weapon,L"/Script/Engine.Actor.GetComponentByClass",16,&q)||q.component!=binding.skinAttachment)
        return InitializeFailure(e,"champion2024_original_parent_skin_query_mismatch");
    if(e.observedSkinOwner!=binding.skinAttachment||e.observedPresentationTarget!=binding.pawn){
        if(e.parentInitializeAttempted)return InitializeFailure(e,"champion2024_original_parent_initialization_incomplete");
        e.parentInitializeAttempted=true;
        if(!Champion2024ParentGraph(e.actor,96))return InitializeFailure(e,"champion2024_parent_owner_original_script_rejected");
        if(!ObjectField(e.actor,L"My Gun Skin",e.observedSkinOwner)||e.observedSkinOwner!=binding.skinAttachment)
            return InitializeFailure(e,"champion2024_original_parent_owner_not_read_back");
        if(!Champion2024ParentGraph(e.actor,366))return InitializeFailure(e,"champion2024_parent_target_original_script_rejected");
        if(!ObjectField(e.actor,L"My FXC Target",e.observedPresentationTarget)||e.observedPresentationTarget!=binding.pawn)
            return InitializeFailure(e,"champion2024_original_parent_target_not_read_back");
    }
    e.parentInitialized=true;e.presentationTargetVerified=e.observedPresentationTarget==binding.pawn;
    return true;
}
inline bool Champion2024Component(Effect& e){
    if(!training_champion_state::Poll(binding.world))return InitializeFailure(e,"champion2024_original_ranking_component_not_ready");
    auto component=training_champion_state::State().component;
    if(!component||Read<void*>(component,0xc8)!=Read<void*>(binding.world,0x178))
        return InitializeFailure(e,"champion2024_original_ranking_component_owner_mismatch");
    if(!ObjectField(e.actor,L"myCompGameStateKillChampion",e.observedChampionComponent))
        return InitializeFailure(e,"champion2024_original_ranking_component_field_unavailable");
    if(e.observedChampionComponent&&e.observedChampionComponent!=component)
        return InitializeFailure(e,"champion2024_original_ranking_component_source_mismatch");
    if(!e.observedChampionComponent){
        // Bare practice ShooterGameState has no Base_Game_State Blueprint
        // property. Supply only this missing real object reference through the
        // original property setter before original Lv4 binds its delegates.
        auto property=Field(e.actor,L"myCompGameStateKillChampion");if(!property)return false;
        struct Query{void* object;uint8_t name[12]{};uint8_t pad[4]{};void* value;} q{e.actor,{}, {},component};
        static_assert(sizeof(Query)==32&&offsetof(Query,value)==24);
        std::memcpy(q.name,static_cast<uint8_t*>(property)+0x20,12);
        if(!Event(engine::DefaultObject(engine::Class(0x45aa110)),L"/Script/Engine.KismetSystemLibrary.SetObjectPropertyByName",32,&q)
           ||!ObjectField(e.actor,L"myCompGameStateKillChampion",e.observedChampionComponent)||e.observedChampionComponent!=component)
            return InitializeFailure(e,"champion2024_original_ranking_component_reference_not_read_back");
    }
    e.originalChampionDependencyVerified=true;return true;
}
inline bool Champion2024Meshes(Effect& e){
    auto property=Field(e.actor,L"VFXMesh List");
    const auto offset=Read<int32_t>(property,0x44),size=Read<int32_t>(Read<void*>(e.actor,0x10),0x60);
    if(!property||Read<int32_t>(property,0x34)!=16||offset<0x30||size<offset+16||size>2*1024*1024)
        return InitializeFailure(e,"champion2024_original_vfx_mesh_array_field_unavailable");
    auto array=static_cast<uint8_t*>(e.actor)+offset;auto data=Read<void**>(array,0);
    e.observedVfxMeshCount=Read<int32_t>(array,8);const auto capacity=Read<int32_t>(array,12);
    if(e.observedVfxMeshCount<=0||e.observedVfxMeshCount>256||capacity<e.observedVfxMeshCount
       ||!original_players::Readable(data,static_cast<size_t>(e.observedVfxMeshCount)*8))
        return InitializeFailure(e,"champion2024_original_vfx_mesh_objects_not_ready");
    for(int32_t i=0;i<e.observedVfxMeshCount;++i)if(!data[i])return InitializeFailure(e,"champion2024_original_vfx_mesh_object_null");
    return true;
}
inline bool InitializeEffect(Effect& e){
    const auto& p=profiles[e.profile];void* owner=nullptr;
    if(e.profile==3&&(!Champion2024Parent(e)||!Champion2024Component(e)))return false;
    if(!ObjectField(e.actor,p.ownerField,owner)||owner!=binding.skinAttachment){
        if(e.profile!=0)return InitializeFailure(e,"original_champion_skin_owner_field_not_ready");
        struct Query{void* type;void* component=nullptr;} q{engine::Find(L"/Game/Personalization/PersonalizationComponents/Weapon_Skin_Base.Weapon_Skin_Base_C")};
        if(!q.type||!Event(binding.weapon,L"/Script/Engine.Actor.GetComponentByClass",16,&q)||q.component!=binding.skinAttachment)
            return InitializeFailure(e,"original_champion_source_skin_query_mismatch");
    }
    void *one=nullptr,*two=nullptr;
    e.observedSkinOwner=owner;
    const bool repairedParentNeedsChild=e.profile==3&&e.parentInitializeAttempted&&!e.initializeAttempted;
    if(!repairedParentNeedsChild&&owner==binding.skinAttachment&&ObjectField(e.actor,p.materialOne,one)&&one&&ObjectField(e.actor,p.materialTwo,two)&&two){
        e.observedMaterialOne=one;e.observedMaterialTwo=two;
        if(e.profile==3&&!Champion2024Meshes(e))return false;
        e.initialized=true;e.initializeStage="original_owned_material_objects_verified";return true;
    }
    if(!e.initializeAttempted){
        e.initializeAttempted=true;
        if(!Graph(e.actor,p,p.initializeOne)||(p.initializeTwo&&!Graph(e.actor,p,p.initializeTwo)))
            return InitializeFailure(e,"original_champion_client_initialization_script_rejected");
        ++state.originalClientInitializations;
    }
    e.initialized=ObjectField(e.actor,p.ownerField,owner)&&owner==binding.skinAttachment
        &&ObjectField(e.actor,p.materialOne,one)&&one&&ObjectField(e.actor,p.materialTwo,two)&&two;
    e.observedSkinOwner=owner;e.observedMaterialOne=one;e.observedMaterialTwo=two;
    if(!e.initialized)return InitializeFailure(e,"original_champion_owned_material_objects_not_read_back");
    if(e.profile==3&&!Champion2024Meshes(e)){e.initialized=false;return false;}
    e.initializeStage="original_client_leaves_and_owned_material_objects_verified";
    return e.initialized;
}
inline bool ApplyKillCount(Effect& e){
    auto& c=controls[1];if(e.profile!=1||!e.initialized)return false;
    void* component=nullptr;if(!ObjectField(e.actor,L"Comp_FXC_Game_Events",component)||!component||Read<void*>(component,0xc8)!=e.actor)return false;
    auto property=Field(component,L"MyKills");if(!property||Read<int32_t>(property,0x34)!=4)return false;
    const auto offset=Read<int32_t>(property,0x44),size=Read<int32_t>(Read<void*>(component,0x10),0x60);
    if(offset<0x30||size<offset+4||size>2*1024*1024)return false;
    int32_t desired=0;
    if(c.requested==Mode::ForceOn)desired=static_cast<int32_t>(c.value);
    else if(c.requested==Mode::FollowOriginal){
        void* info=nullptr;
        if(!Event(binding.pawn,L"/Script/ShooterGame.ShooterCharacter.GetOwnerExclusivePlayerInfo",8,&info)||!info
           ||!Event(info,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.GetKills",4,&desired))return false;
    }
    const auto before=Read<int32_t>(component,static_cast<size_t>(offset));
    if(c.dirty||c.applied!=c.requested||before!=desired||e.lastCosmeticKills!=desired){
        struct Query{void* object;uint8_t name[12]{};int32_t value=0;} q{component};
        static_assert(sizeof(Query)==24&&offsetof(Query,value)==20);
        std::memcpy(q.name,static_cast<uint8_t*>(property)+0x20,12);q.value=desired;
        if(!Event(engine::DefaultObject(engine::Class(0x45aa110)),L"/Script/Engine.KismetSystemLibrary.SetIntPropertyByName",24,&q)
           ||Read<int32_t>(component,static_cast<size_t>(offset))!=desired||!Graph(e.actor,profiles[1],7600))return false;
        e.lastCosmeticKills=desired;++c.applyCount;c.applied=c.requested;c.appliedVerified=true;c.dirty=false;controls[0].dirty=true;
    }
    c.available=true;c.applied=c.requested;c.appliedVerified=e.lastCosmeticKills==desired
        &&Read<int32_t>(component,static_cast<size_t>(offset))==desired;
    c.reason="original_cosmetic_MyKills_and_original_clamp0_25_graph_completed";return c.appliedVerified;
}
inline bool ApplyChampion(Effect& e,bool force){
    auto& c=controls[0];const auto& p=profiles[e.profile];if(!e.initialized&&!InitializeEffect(e))return false;
    void *global=nullptr,*attacking=nullptr,*defending=nullptr;state.originalChampionKnown=OriginalChampion(global,attacking,defending);
    auto local=Read<void*>(binding.pawn,0x488),other=OtherPlayerState(local);
    if(!original_players::IsA(local,original_players::ShooterStateClass))return false;
    if(c.requested==Mode::FollowOriginal&&!state.originalChampionKnown)return false;
    if(c.requested==Mode::ForceOn)global=attacking=defending=local;
    if(c.requested==Mode::ForceOff){if(!other)return false;global=attacking=defending=other;}
    // Avoid null Blueprint contexts for "no current champion"; this is only
    // the presentation-event argument, never the real game-state leader.
    if(!global)global=other;if(!attacking)attacking=other;if(!defending)defending=other;
    if(!global||(p.eventSize==16&&(!attacking||!defending)))return false;
    const bool changed=force||c.dirty||!e.lastDispatchValid||e.lastMode!=c.requested
        ||e.lastChampion!=global||e.lastAttacking!=attacking||e.lastDefending!=defending;
    if(changed){
        auto event=TypePath(p)+L"."+p.eventName;struct Pair{void* attacker;void* defender;} parameters{attacking,defending};
        void* input=p.eventSize==16?static_cast<void*>(&parameters):static_cast<void*>(&global);
        if(!Event(e.actor,event.c_str(),p.eventSize,input,Read<void*>(e.actor,0x10))||!Graph(e.actor,p,p.update))return false;
        e.lastMode=c.requested;e.lastChampion=global;e.lastAttacking=attacking;e.lastDefending=defending;e.lastDispatchValid=true;
        ++e.dispatches;++c.applyCount;++state.originalChampionUpdates;
    }
    c.available=true;c.applied=c.requested;c.appliedVerified=e.lastDispatchValid;c.dirty=false;c.reason="original_champion_event_and_client_leaf_completed";return c.appliedVerified;
}
inline bool ScanEffects(bool immediate){
    const auto now=GetTickCount64();if(!immediate&&now-lastScan<250)return true;lastScan=now;state.effects=0;
    controls[0].available=controls[0].appliedVerified=false;controls[1].available=controls[1].appliedVerified=false;
    for(size_t index=0;index<std::size(profiles);++index){
        const auto& p=profiles[index];auto type=engine::Find(TypePath(p).c_str());if(!type)continue;
        struct Query{void* world;void* type;void** data=nullptr;int32_t count=0,capacity=0;} q{binding.world,type};
        if(!Event(engine::DefaultObject(engine::Class(0x43a9670)),L"/Script/Engine.GameplayStatics.GetAllActorsOfClass",32,&q))return false;
        struct Owner{void* allocation;~Owner(){if(allocation)engine::function<void(*)(void*)>(0x1915720)(allocation);}} owner{q.data};
        if(q.count<0||q.count>512||q.capacity<q.count||(q.count&&!original_players::Readable(q.data,static_cast<size_t>(q.count)*8)))return false;
        for(int32_t i=0;i<q.count;++i){
            auto actor=q.data[i];if(Read<void*>(actor,0x10)!=type||Read<uint8_t>(actor,0x5fc))continue;
            void* equippable=nullptr;ObjectField(actor,L"Equippable",equippable);
            const auto target=original_effects::ResolveIdentity(Read<original_effects::WeakIdentity>(actor,0x620));
            if(equippable!=binding.weapon&&Read<void*>(actor,0x210)!=binding.weapon&&target!=binding.weapon)continue;
            auto effect=Track(actor,index);if(!effect)continue;++state.effects;
            if(!effect->initialized&&!InitializeEffect(*effect))continue;
            ApplyKillCount(*effect);ApplyChampion(*effect,false);
        }
    }
    return true;
}
inline bool PrepareForBindingChangeImpl(){
    if(!Thread())return false;
    if(Context()&&state.snapshotReady)for(size_t i=2;i<state.controls;++i){auto& c=controls[i];
        if(c.applied!=Mode::FollowOriginal&&c.lastAppliedClass&&QueryOverride(binding.weapon,c.baseClass)==c.lastAppliedClass){
            if(c.lastAppliedClass!=c.originalRuleClass&&!SetMap(c,c.originalRuleClass)){
                c.appliedVerified=false;c.reason="original_rule_restore_before_rebind_failed";return false;
            }
            c.lastAppliedClass=nullptr;c.applied=Mode::FollowOriginal;
        }
    }
    state.changingBinding=true;return true;
}
inline bool OnWeaponChangedImpl(const Binding& next){
    if(!Thread())return false;
    if(state.bound&&binding.weapon==next.weapon&&binding.skinAttachment==next.skinAttachment&&binding.generation==next.generation){binding.assetsReady=next.assetsReady;state.assetsReady=next.assetsReady;return true;}
    if(state.bound&&binding.weapon!=next.weapon)PrepareForBindingChangeImpl();
    binding=next;state.bound=next.world&&next.pawn&&next.weapon;state.assetsReady=next.assetsReady;state.snapshotReady=false;state.changingBinding=false;
    state.controls=state.effects=0;controls={};effects={};lastScan=lastMapPoll=0;++state.bindingChanges;
    if(!state.bound){state.stage="skin_effect_controls_waiting_for_actual_weapon";return true;}
    if(!original_effects::CaptureIdentity(next.weapon,weaponIdentity)||!original_effects::CaptureIdentity(next.pawn,pawnIdentity)
       ||(next.skinAttachment&&!original_effects::CaptureIdentity(next.skinAttachment,skinIdentity)))return false;
    char scope[768]{};if(next.skinData)Path(Read<void*>(next.skinData,0x10),scope);if(!scope[0])Path(Read<void*>(next.weapon,0x10),scope);state.scope=scope[0]?Hash(scope):0;
    state.stage="skin_effect_controls_waiting_for_original_attachment_assets";return true;
}
inline bool TickImpl(){
    if(!Thread())return false;++state.samples;if(!state.bound||state.changingBinding||!binding.assetsReady||!state.scope)return true;
    if(!Context()){state.stage="skin_effect_controls_actual_binding_expired";return true;}
    if(!state.snapshotReady){
        auto set=engine::Find(L"/Script/ShooterGame.AresEquippable.OverrideEffect"),get=engine::Find(L"/Script/ShooterGame.AresEquippable.GetOverridableEffect");
        state.nativeOverrideAbi=set&&get&&Read<uint16_t>(set,0xbe)==56&&Read<uint16_t>(get,0xbe)==16;
        auto anchor=Field(binding.weapon,L"EffectOverrides");state.reflectionAnchorVerified=anchor&&Read<int32_t>(anchor,0x44)==0xcb0;
        if(!state.nativeOverrideAbi||!state.reflectionAnchorVerified){state.stage="skin_effect_controls_original_reflection_or_override_abi_unavailable";return true;}
        if(!Snapshot()){state.stage="skin_effect_controls_original_effect_map_unavailable";return true;}
    }
    const auto now=GetTickCount64();const bool mapPoll=now-lastMapPoll>=250;
    for(size_t i=2;i<state.controls;++i)if(controls[i].available&&(mapPoll||controls[i].dirty))ApplyMap(controls[i]);
    if(mapPoll)lastMapPoll=now;
    if(controls[0].supported&&!ScanEffects(false)){state.stage="skin_effect_controls_original_effect_query_failed";return true;}
    state.stage="skin_effect_controls_original_data_bound";return true;
}
inline bool SetOverrideImpl(const char* key,uint32_t mode,float value){
    if(!Thread()||!key||strnlen_s(key,80)>=80||mode>2||!std::isfinite(value))return CommandFailure(3201,"invalid_effect_request");
    if(!TickImpl()||!state.snapshotReady)return CommandFailure(3202,"actual_skin_not_ready");
    for(size_t i=0;i<state.controls;++i){auto& c=controls[i];if(std::strcmp(c.key,key)!=0)continue;
        if(!c.supported)return CommandFailure(3203,"effect_not_supported_by_current_original_skin");
        if(c.numeric&&(value<c.minimum||value>c.maximum||std::floor(value)!=value))return CommandFailure(3204,"original_kill_progression_value_must_be_integer0_25");
        auto setting=Stored(key,true);if(!setting)return CommandFailure(3205,"effect_setting_capacity");
        setting->mode=c.requested=static_cast<Mode>(mode);setting->value=c.value=value;c.dirty=true;c.appliedVerified=false;++state.commands;
        if(c.kind==Kind::OriginalEffectOverride){if(!ApplyMap(c))return CommandFailure(3206,c.reason);}
        else{if(!ScanEffects(true)||!c.available||!c.appliedVerified)return CommandFailure(3207,"original_owned_champion_effect_not_ready");}
        state.commandError=0;state.commandResult="original_request_applied_and_read_back";return true;
    }
    return CommandFailure(3208,"effect_key_not_in_current_original_skin");
}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0003200U;
    const auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;state.restartRequired=true;state.stage="skin_effect_controls_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool OnWeaponChanged(const Binding& next){__try{return OnWeaponChangedImpl(next);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Initialize(void* world,void* pawn){
    if(!Thread()||!world||pawn!=original_players::activePawn||!original_players::IsA(pawn,original_players::ShooterCharacterClass))return false;
    if(!state.bound){binding.world=world;binding.pawn=pawn;state.stage="skin_effect_controls_waiting_for_actual_weapon";}return true;
}
template<class CurrentBinding> inline bool OnWeaponChanged(const CurrentBinding& next){
    Binding supplied{next.world,next.pawn,next.weapon,next.skinData,next.levelData,next.chromaData,
        next.skinAttachment,next.chromaAttachment,static_cast<uint64_t>(next.generation),next.assetsReady};
    return OnWeaponChanged(supplied);
}
inline bool PrepareForBindingChange(){__try{return PrepareForBindingChangeImpl();}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Tick(){__try{return TickImpl();}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Poll(){return Tick();}
inline bool SetOverride(const char* key,uint32_t mode,float value=0){__try{return SetOverrideImpl(key,mode,value);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;std::wstring temporary(path);temporary+=L".tmp";FILE* f=nullptr;if(_wfopen_s(&f,temporary.c_str(),L"wb")||!f)return false;
    const auto b=[](bool x){return x?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"pid\":%lu,\"stage\":",GetCurrentProcessId());original_players::Text(f,state.stage);
    std::fprintf(f,",\"error\":%u,\"command_error\":%u,\"command_result\":",state.error,state.commandError);original_players::Text(f,state.commandResult);
    std::fprintf(f,",\"bound_pawn\":\"%p\"",binding.pawn);
    std::fputs(",\"last_failed_original_event\":",f);const auto failedEvent=original_players::Utf8(state.failedEvent);original_players::Text(f,failedEvent.c_str());
    std::fprintf(f,",\"expected_event_size\":%u,\"actual_event_size\":%u",static_cast<unsigned>(state.expectedEventSize),static_cast<unsigned>(state.actualEventSize));
    std::fprintf(f,",\"expected_script_bytes\":%d,\"actual_script_bytes\":%d,\"last_script_entry\":%d,\"original_script_entry_verified\":%s",state.expectedScriptBytes,state.actualScriptBytes,state.lastScriptEntry,b(state.originalScriptEntryVerified));
    std::fprintf(f,",\"bound\":%s,\"assets_ready\":%s,\"context_valid\":%s,\"snapshot_ready\":%s,\"native_override_abi\":%s,\"reflection_anchor_verified\":%s,\"restart_required\":%s,\"exception_rva\":%llu,\"samples\":%llu,\"binding_changes\":%llu,\"commands\":%llu,\"original_map_updates\":%llu,\"original_client_initializations\":%llu,\"original_champion_updates\":%llu,\"original_map_count\":%zu,\"original_champion_known\":%s,\"active_original_champion_effects\":%zu,\"generation\":%llu,\"weapon\":\"%p\",\"skin_attachment\":\"%p\",\"chroma_attachment\":\"%p\",\"controls\":[",b(state.bound),b(binding.assetsReady),b(state.contextValid),b(state.snapshotReady),b(state.nativeOverrideAbi),b(state.reflectionAnchorVerified),b(state.restartRequired),static_cast<unsigned long long>(state.exceptionRva),static_cast<unsigned long long>(state.samples),static_cast<unsigned long long>(state.bindingChanges),static_cast<unsigned long long>(state.commands),static_cast<unsigned long long>(state.originalMapUpdates),static_cast<unsigned long long>(state.originalClientInitializations),static_cast<unsigned long long>(state.originalChampionUpdates),state.originalMapCount,b(state.originalChampionKnown),state.effects,static_cast<unsigned long long>(binding.generation),binding.weapon,binding.skinAttachment,binding.chromaAttachment);
    for(size_t i=0;i<state.controls;++i){const auto& c=controls[i];if(i)std::fputc(',',f);std::fputs("{\"key\":",f);original_players::Text(f,c.key);std::fputs(",\"label\":",f);original_players::Text(f,c.label);
        std::fprintf(f,",\"supported\":%s,\"available\":%s,\"requested_mode\":%u,\"applied_mode\":%u,\"applied_verified\":%s,\"modes\":[0,1,2],\"numeric\":%s,\"value\":%.9g,\"minimum\":%.9g,\"maximum\":%.9g,\"next_original_event\":%s,\"apply_count\":%llu,\"reason\":",b(c.supported),b(c.available),static_cast<unsigned>(c.requested),static_cast<unsigned>(c.applied),b(c.appliedVerified),b(c.numeric),c.value,c.minimum,c.maximum,b(c.nextOriginalEvent),static_cast<unsigned long long>(c.applyCount));original_players::Text(f,c.reason);
        std::fputs(",\"base_class\":",f);original_players::Text(f,c.basePath);std::fputs(",\"skin_class\":",f);original_players::Text(f,c.skinPath);
        std::fprintf(f,",\"last_observed_original_rule_class\":\"%p\",\"last_manual_map_class\":\"%p\"}",c.originalRuleClass,c.lastAppliedClass);
    }
    std::fputs("],\"effects\":[",f);bool comma=false;
    for(const auto& e:effects)if(e.occupied){if(comma)std::fputc(',',f);comma=true;
        std::fprintf(f,"{\"actor\":\"%p\",\"profile\":%zu,\"object_index\":%d,\"weak_serial\":%d,\"identity_live\":%s,\"initialize_attempted\":%s,\"initialized_with_original_material_objects\":%s,\"original_event_and_client_leaf_completed\":%s,\"original_champion_dispatches\":%llu,\"last_mode\":%u,\"last_champion\":\"%p\",\"last_attacking_champion\":\"%p\",\"last_defending_champion\":\"%p\",\"last_cosmetic_kills\":%d,\"original_class\":",e.actor,e.profile,e.identity.index,e.identity.serial,b(original_effects::ResolveIdentity(e.identity)==e.actor),b(e.initializeAttempted),b(e.initialized),b(e.lastDispatchValid),static_cast<unsigned long long>(e.dispatches),static_cast<unsigned>(e.lastMode),e.lastChampion,e.lastAttacking,e.lastDefending,e.lastCosmeticKills);
        const auto name=original_players::Utf8(profiles[e.profile].package);original_players::Text(f,name.c_str());
        std::fputs(",\"initialization_stage\":",f);original_players::Text(f,e.initializeStage);
        std::fprintf(f,",\"parent_initialize_attempted\":%s,\"original_parent_initialized\":%s,\"source_ownership_verified\":%s,\"original_presentation_target_verified\":%s,\"original_champion_component_verified\":%s,\"observed_actor_owner\":\"%p\",\"observed_equippable\":\"%p\",\"resolved_original_weak_target\":\"%p\",\"observed_skin_owner\":\"%p\",\"observed_presentation_target\":\"%p\",\"observed_material_one\":\"%p\",\"observed_material_two\":\"%p\",\"observed_champion_component\":\"%p\",\"original_vfx_mesh_count\":%d",b(e.parentInitializeAttempted),b(e.parentInitialized),b(e.sourceOwnershipVerified),b(e.presentationTargetVerified),b(e.originalChampionDependencyVerified),e.observedActorOwner,e.observedEquippable,e.observedOriginalTarget,e.observedSkinOwner,e.observedPresentationTarget,e.observedMaterialOne,e.observedMaterialTwo,e.observedChampionComponent,e.observedVfxMeshCount);
        std::fprintf(f,",\"playback_epochs\":%llu,\"original_effect_id_hex\":\"",static_cast<unsigned long long>(e.playbackEpochs));
        for(const auto byte:e.nativeEffectId)std::fprintf(f,"%02x",static_cast<unsigned>(byte));std::fputs("\"}",f);
    }
    std::fprintf(f,"],\"training_champion_state_error\":%u,\"original_champion_rule_updates\":%llu",training_champion_state::State().error,static_cast<unsigned long long>(training_champion_state::State().originalAssignments));
    std::fputs(",\"writes_material_or_color_parameters\":false,\"changes_weapon_attachment_composition\":false,\"manual_override_changes_real_scores_or_champion\":false,\"original_champion_rules_may_update_real_state\":true,\"rendered_pixels_verified\":false}\n",f);
    const bool ok=std::ferror(f)==0,closed=std::fclose(f)==0;return ok&&closed&&MoveFileExW(temporary.c_str(),path,MOVEFILE_REPLACE_EXISTING);
}
}
