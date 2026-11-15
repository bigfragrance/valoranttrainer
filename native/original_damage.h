#pragma once
#include "original_players.h"
#include "original_shot_observer.h"
#include <array>
#include <cstddef>
#include <limits>
#include <algorithm>

// Local authority records feed the shipped client replication handlers. The
// module does not generate shots, alter spread/recoil, set health directly,
// play replacement death clips, or invoke an account/platform lifecycle.
// Pinned-image metadata and native bodies: work/duel1-original-damage.
namespace original_damage {
inline constexpr size_t MaxTargets=32;
inline constexpr uintptr_t DamageableClass=0x6702670,SectionClass=0x65876c0;
inline constexpr uintptr_t PointResponseClass=0x64fdaa0,DamageTypeClass=0x6c89c60;
inline constexpr uintptr_t ReactionsClass=0x647a940,DeathComponentClass=0x6c74fc0;
struct Name { uint8_t bytes[12]{}; };
struct MovementTimestamp { int32_t weakIndex=0,weakSerial=0;float time=0;int32_t respawn=0; };
struct LifeTimestamp { MovementTimestamp movement;int32_t eventIndex=0; };
struct LifeChange { void* section=nullptr;float life=0,delta=0;uint8_t alive=0,padding[7]{}; };
template<class T> struct Array { T* data=nullptr;int32_t count=0,capacity=0; };
struct ResponseData {
    float taken=0,dealt=0;
    uint8_t killed=0,alive=1,pad10[6]{};
    void* instigator=nullptr;void* causer=nullptr;
    engine::Vec origin{};
    void* equippable=nullptr;void* damageType=nullptr;
    Array<LifeChange> changes;
    void* instigatorPawn=nullptr;void* damagerState=nullptr;void* killCreditState=nullptr;
    uint8_t region=0,pad113[3]{};
    LifeTimestamp timestamp;
    int32_t victimRespawn=0;
    uint8_t zoomed=0,focus=0,pad142[2]{};
};
struct PointData {
    ResponseData base;
    void* hitComponent=nullptr;Name bone;
    uint8_t wallPenetration=0,pad165[3]{};
    float falloff=1;uint32_t pad172=0;
    engine::Vec direction{},impact{},normal{},boneRelativeImpact{};
};
struct DeathData {
    Array<void> assists;
    int32_t killsForKiller=0,killsForVictim=0;
    void* montage=nullptr;void* effectOverride=nullptr;void* overrideContext=nullptr;
    uint8_t queued=0,padding[7]{};
};
struct ReactionResult { void* entry=nullptr;int32_t score=0,padding=0; };
static_assert(sizeof(Name)==12&&sizeof(MovementTimestamp)==16&&sizeof(LifeTimestamp)==20);
static_assert(sizeof(LifeChange)==24&&sizeof(ResponseData)==0x90&&sizeof(PointData)==0x110);
static_assert(offsetof(ResponseData,changes)==0x48&&offsetof(ResponseData,timestamp)==0x74);
static_assert(offsetof(ResponseData,victimRespawn)==0x88&&offsetof(PointData,bone)==0x98);
static_assert(offsetof(PointData,falloff)==0xa8);
static_assert(offsetof(PointData,direction)==0xb0&&offsetof(PointData,impact)==0xc8);
static_assert(sizeof(DeathData)==56&&offsetof(DeathData,montage)==0x18&&sizeof(ReactionResult)==16);
struct Hit {
    bool hit=false,blocking=false;
    uint8_t region=0;
    Name bone;
    void* component=nullptr;
    double distance=0;
    engine::Vec location{},normal{};
    uint8_t nativeHit[0xf0]{};
};
struct SectionSnapshot {
    void *object=nullptr,*typeClass=nullptr,*componentOwner=nullptr,*damageOwner=nullptr;
    bool readable=false,originalType=false,ownerMatches=false,nativeValues=false,validNumbers=false;
    bool alive=false,canBeDestroyed=false,allowPenetration=false,emptyShield=false;
    uint8_t type=255;uint32_t priority=0;
    float life=0,maximum=0,nativeLife=0,nativeMaximum=0,damageScale=0;
    char objectName[128]{},className[128]{};
};
struct SectionAudit {
    bool attempted=false,arrayValid=false,complete=false;
    void *pawn=nullptr,*damageable=nullptr,*selectedShield=nullptr;
    int32_t count=0,capacity=0;unsigned emptyShields=0,effectiveShields=0;
    uint64_t sampledAt=0;
    std::array<SectionSnapshot,32> entries{};
};
struct Target {
    bool bound=false,alive=false,nativeHealthVerified=false,originalPhysics=false;
    void *pawn=nullptr,*state=nullptr,*damageable=nullptr,*healthSection=nullptr,*shieldSection=nullptr,*core=nullptr,*death=nullptr;
    void *originalReactions=nullptr,*lastResponse=nullptr,*lastMontage=nullptr,*observedMontage=nullptr;
    float health=0,maxHealth=0,lastHealthBefore=0,lastHealthAfter=0;
    float rawHealth=0,shield=0,maxShield=0,rawShield=0,lastShieldBefore=0,lastShieldAfter=0;
    float lastHealthTaken=0,lastShieldTaken=0,lastHealthPenetration=0;
    uint32_t damageEvents=0,deathEvents=0,traceQueries=0,traceHits=0,montageSamples=0;
    int32_t bodyCount=0,physicsActors=0,pendingDamageEvents=0,sectionCount=0;
    unsigned emptyShieldSections=0,effectiveShieldSections=0;
    int32_t lastNativeEventIndex=0,lastReactionScore=0,lastReactionCandidates=0;
    uint64_t lastToken=0;
    std::array<uint64_t,128> appliedTokens{};
    size_t nextToken=0;
};
struct Result {
    bool applied=false,killed=false,duplicate=false,originalResponse=false;
    bool originalLifeReadback=false,originalDeathMontage=false;
    void* response=nullptr;void* montage=nullptr;
    float healthBefore=0,healthAfter=0,taken=0;
    float shieldBefore=0,shieldAfter=0,healthTaken=0,shieldTaken=0;
    unsigned sectionChanges=0;
    uint8_t region=0;
};
struct Diagnostics {
    bool initialized=false,restartRequired=false;
    unsigned error=0,boundTargets=0,damageEvents=0,deathEvents=0,duplicateEvents=0;
    unsigned nativeResponses=0,nativeHealthReadbacks=0,originalMontageSelections=0;
    unsigned nativeSectionCalculations=0,localArmoredPartitions=0;
    unsigned eventFailures=0,traceReflectionResolutions=0,traceLookupMissing=0;
    const wchar_t* failedEvent=nullptr;
    uint16_t failedEventExpected=0,failedEventActual=0;
    bool failedEventHasObject=false,failedEventHasFunction=false,failedEventHasProcessEvent=false;
    void *failedEventFunction=nullptr,*componentTraceFunction=nullptr,*componentTraceCandidate=nullptr,*componentTraceLookup=nullptr;
    uint16_t componentTraceCandidateSize=0;
    bool traceFunctionOwnerVerified=false,traceLookupMatches=false;
    uintptr_t exceptionRva=0;
    const char* stage="original_damage_not_initialized";
    const char* operation="none";
};
inline Diagnostics diagnostics;
inline std::array<Target,MaxTargets> targets{};
inline std::array<SectionAudit,MaxTargets> sectionAudits{};
inline DWORD gameThread=0;
inline void* world=nullptr;
inline const Diagnostics& State(){return diagnostics;}
template<class T> inline T Read(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
template<class T> inline void Put(uint8_t* p,size_t offset,const T& value){std::memcpy(p+offset,&value,sizeof(value));}
inline bool IsA(void* p,uintptr_t getter){return original_players::IsA(p,getter);}
inline bool Fail(unsigned error,const char* stage){diagnostics.error=error;diagnostics.stage=stage;return false;}
inline bool Thread(){
    return diagnostics.initialized&&!diagnostics.restartRequired&&gameThread==GetCurrentThreadId()
        ?true:Fail(1120,"original_damage_context_or_thread_unavailable");
}
inline bool EventWithFunction(void* object,const wchar_t* name,void* method,uint16_t size,void* params){
    const uint16_t actual=method?Read<uint16_t>(method,0xbe):0;
    auto table=Read<void**>(object,0);
    auto process=Read<void*>(table,0x298);
    if(!object||!method||actual!=size||!process){
        ++diagnostics.eventFailures;diagnostics.failedEvent=name;
        diagnostics.failedEventExpected=size;diagnostics.failedEventActual=actual;
        diagnostics.failedEventHasObject=object!=nullptr;diagnostics.failedEventHasFunction=method!=nullptr;
        diagnostics.failedEventHasProcessEvent=process!=nullptr;diagnostics.failedEventFunction=method;
        return false;
    }
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,method,params);return true;
}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* params){
    return EventWithFunction(object,name,engine::Find(name),size,params);
}
inline bool ReadMaxHealth(void* pawn,float& maximum){
    // UHT ReturnValue is IntProperty (kind3), not FloatProperty (kind10).
    // Original exec6c62480 converts the aggregate with cvttss2si at6c624b2.
    // Its four-byte ParmsSize alone cannot distinguish these return types.
    int32_t nativeMaximum=0;maximum=0;
    if(!Event(pawn,L"/Script/ShooterGame.ShooterCharacter.GetMaxHealth",4,&nativeMaximum)||nativeMaximum<=0)return false;
    maximum=static_cast<float>(nativeMaximum);return true;
}
inline void* ComponentTraceFunction(){
    if(diagnostics.componentTraceFunction)return diagnostics.componentTraceFunction;
    const auto found=engine::Find(L"/Script/Engine.PrimitiveComponent.K2_LineTraceComponent");
    diagnostics.componentTraceLookup=found;
    if(!found)++diagnostics.traceLookupMissing;
    // Exact pinned generated UFunction accessor. It delegates construction to
    // the original ConstructUFunction1d1b550 only if its native cache is empty.
    // 3e8f120 is its OUTER UClass constructor, not this function's accessor.
    auto method=engine::function<void*(*)()>(0x3e8dc50)();
    diagnostics.componentTraceCandidate=method;
    diagnostics.componentTraceCandidateSize=method?Read<uint16_t>(method,0xbe):0;
    ++diagnostics.traceReflectionResolutions;
    diagnostics.traceFunctionOwnerVerified=method&&Read<void*>(method,0x28)==engine::Class(0x2a653a0);
    diagnostics.traceLookupMatches=!found||found==method;
    if(!method||!diagnostics.traceFunctionOwnerVerified||!diagnostics.traceLookupMatches)return nullptr;
    diagnostics.componentTraceFunction=method;return method;
}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    diagnostics.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0001121U;
    const auto address=exception&&exception->ExceptionRecord
        ?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=address>=engine::image?address-engine::image:address;
    diagnostics.restartRequired=true;diagnostics.stage="original_damage_native_exception_requires_new_session";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* originalWorld){
    if(diagnostics.initialized)return world==originalWorld&&Thread();
    if(!engine::image||!originalWorld||!original_players::Thread())return Fail(1121,"original_damage_world_missing");
    auto state=Read<void*>(originalWorld,0x178);
    if(!IsA(state,original_players::ShooterGameStateClass))return Fail(1122,"original_damage_requires_original_game_state");
    world=originalWorld;gameThread=GetCurrentThreadId();diagnostics.initialized=true;
    diagnostics.error=0;diagnostics.stage="original_damage_local_authority_ready";return true;
}
inline bool InspectPhysics(Target& target){
    target.bodyCount=Read<int32_t>(target.core,0xd18);target.physicsActors=0;
    auto bodies=Read<void**>(target.core,0xd10);
    if(target.bodyCount<0||target.bodyCount>64||
       (target.bodyCount&&!original_players::Readable(bodies,static_cast<size_t>(target.bodyCount)*8)))
        return Fail(1123,"original_damage_original_physics_array_invalid");
    for(int32_t i=0;i<target.bodyCount;++i)
        if(original_players::Readable(bodies[i],0x160)&&Read<void*>(bodies[i],0x158))++target.physicsActors;
    target.originalPhysics=target.bodyCount==19&&target.physicsActors==19;
    return true;
}
inline bool DescribeSections(void* pawn,void* damageable,SectionAudit& audit){
    audit={};audit.attempted=true;audit.pawn=pawn;audit.damageable=damageable;audit.sampledAt=GetTickCount64();
    const auto sections=Read<Array<void*>>(damageable,0x1e8);audit.count=sections.count;audit.capacity=sections.capacity;
    if(sections.count<=0||sections.count>static_cast<int32_t>(audit.entries.size())||sections.capacity<sections.count
       ||!original_players::Readable(sections.data,static_cast<size_t>(sections.count)*sizeof(void*)))return false;
    audit.arrayValid=true;audit.complete=true;
    for(int32_t i=0;i<sections.count;++i){auto& row=audit.entries[i];row.object=sections.data[i];
        row.readable=original_players::Readable(row.object,0x150);row.originalType=row.readable&&IsA(row.object,SectionClass);
        if(!row.originalType){audit.complete=false;continue;}
        row.typeClass=Read<void*>(row.object,0x10);row.componentOwner=Read<void*>(row.object,0xc8);
        row.damageOwner=Read<void*>(row.object,0x110);row.ownerMatches=row.damageOwner==damageable;
        row.type=Read<uint8_t>(row.object,0x118);row.canBeDestroyed=Read<uint8_t>(row.object,0x119)!=0;
        row.priority=Read<uint32_t>(row.object,0x11c);row.alive=Read<uint8_t>(row.object,0x120)!=0;
        row.life=Read<float>(row.object,0x124);row.maximum=Read<float>(row.object,0x128);
        row.damageScale=Read<float>(row.object,0x144);row.allowPenetration=Read<uint8_t>(row.object,0x148)!=0;
        original_weapon::OriginalNameValue(static_cast<uint8_t*>(row.object)+0x18,row.objectName);
        original_weapon::OriginalNameValue(static_cast<uint8_t*>(row.typeClass)+0x18,row.className);
        // Native6fa4e40 resolves the original tuning tag and attribute modifier.
        // Raw MaximumLife+128 alone is not the effective maximum of a default
        // empty shield. GetLife also respects the real section alive state.
        row.nativeValues=Event(row.object,L"/Script/ShooterGame.DamageSectionComponent.GetLife",4,&row.nativeLife)
            &&Event(row.object,L"/Script/ShooterGame.DamageSectionComponent.GetMaximumLife",4,&row.nativeMaximum);
        row.validNumbers=std::isfinite(row.life)&&std::isfinite(row.maximum)&&std::isfinite(row.damageScale)
            &&std::isfinite(row.nativeLife)&&std::isfinite(row.nativeMaximum)
            &&row.life>=0&&row.maximum>=0&&row.nativeLife>=0&&row.nativeMaximum>=0;
        row.emptyShield=row.type==1&&row.nativeValues&&row.validNumbers&&row.nativeLife==0&&row.nativeMaximum==0;
        if(row.type==1){if(row.emptyShield)++audit.emptyShields;else ++audit.effectiveShields;}
        if(!row.ownerMatches||!row.nativeValues||!row.validNumbers)audit.complete=false;
    }
    return audit.complete;
}
inline bool InspectSections(Target& target,size_t index){
    if(index>=sectionAudits.size())return false;
    auto& audit=sectionAudits[index];DescribeSections(target.pawn,target.damageable,audit);
    if(!audit.arrayValid)return Fail(1129,"original_damage_section_array_invalid");
    target.healthSection=target.shieldSection=nullptr;target.sectionCount=audit.count;
    target.emptyShieldSections=audit.emptyShields;target.effectiveShieldSections=audit.effectiveShields;
    unsigned error=0;const char* reason=nullptr;
    for(int32_t i=0;i<audit.count;++i){const auto& row=audit.entries[i];
        if(!row.originalType||!row.ownerMatches){if(!error){error=1130;reason="original_damage_section_owner_mismatch";}continue;}
        if(!row.nativeValues||!row.validNumbers){if(!error){error=1155;reason="original_damage_section_native_life_query_invalid";}continue;}
        if(row.type==0){
            if(target.healthSection){if(!error){error=1131;reason="original_damage_multiple_health_sections";}}
            else target.healthSection=row.object;
        }else if(row.type==1){
            // Preserve original zero-capacity/default sections in place. A
            // depleted real armor still has a positive native maximum and is
            // counted, so two real armors cannot bypass the one-armor policy.
            if(row.emptyShield)continue;
            if(target.shieldSection){if(!error){error=1132;reason="original_damage_multiple_effective_shield_sections_not_supported";}}
            else target.shieldSection=row.object;
        }else if(row.nativeLife>.001f){if(!error){error=1156;reason="original_damage_nonstandard_active_section_requires_separate_partition_evidence";}}
    }
    audit.selectedShield=target.shieldSection;
    if(error)return Fail(error,reason);
    return target.healthSection?true:Fail(1133,"original_damage_health_section_unavailable");
}
inline void WriteSectionAudit(FILE* file,const SectionAudit& audit){
    auto b=[](bool value){return value?"true":"false";};
    auto number=[file](float value){if(std::isfinite(value))std::fprintf(file,"%.9g",value);else std::fputs("null",file);};
    std::fprintf(file,"{\"attempted\":%s,\"array_valid\":%s,\"complete\":%s,\"pawn\":\"%p\",\"damageable\":\"%p\",\"count\":%d,\"capacity\":%d,\"sampled_tick_ms\":%llu,\"empty_shields\":%u,\"effective_shields\":%u,\"selected_shield\":\"%p\",\"sections\":[",b(audit.attempted),b(audit.arrayValid),b(audit.complete),audit.pawn,audit.damageable,audit.count,audit.capacity,static_cast<unsigned long long>(audit.sampledAt),audit.emptyShields,audit.effectiveShields,audit.selectedShield);
    const int32_t count=audit.arrayValid?audit.count:0;
    for(int32_t i=0;i<count;++i){const auto& row=audit.entries[i];
        std::fprintf(file,"%s{\"order\":%d,\"object\":\"%p\",\"object_name\":",i?",":"",i,row.object);original_players::Text(file,row.objectName);
        std::fputs(",\"class_name\":",file);original_players::Text(file,row.className);
        std::fprintf(file,",\"class\":\"%p\",\"type\":%u,\"component_owner\":\"%p\",\"damage_owner\":\"%p\",\"owner_matches\":%s,\"original_type\":%s,\"alive\":%s,\"can_be_destroyed\":%s,\"allow_penetration\":%s,\"priority\":%u,\"native_values\":%s,\"valid_numbers\":%s,\"empty_shield\":%s,\"life\":",row.typeClass,static_cast<unsigned>(row.type),row.componentOwner,row.damageOwner,b(row.ownerMatches),b(row.originalType),b(row.alive),b(row.canBeDestroyed),b(row.allowPenetration),row.priority,b(row.nativeValues),b(row.validNumbers),b(row.emptyShield));
        number(row.life);std::fputs(",\"maximum\":",file);number(row.maximum);
        std::fputs(",\"native_life\":",file);number(row.nativeLife);std::fputs(",\"native_maximum\":",file);number(row.nativeMaximum);
        std::fputs(",\"damage_scale\":",file);number(row.damageScale);std::fputc('}',file);
    }
    std::fputs("]}",file);
}
inline bool SampleImpl(size_t index){
    if(!Thread()||index>=targets.size()||!targets[index].bound)return false;
    auto& target=targets[index];uint8_t alive=0;
    if(!Event(target.pawn,L"/Script/ShooterGame.ShooterCharacter.GetHealth",4,&target.health)
       ||!ReadMaxHealth(target.pawn,target.maxHealth)
       ||!Event(target.pawn,L"/Script/ShooterGame.ShooterCharacter.GetShield",4,&target.shield)
       ||!Event(target.pawn,L"/Script/ShooterGame.ShooterCharacter.GetMaxShield",4,&target.maxShield)
       ||!Event(target.damageable,L"/Script/ShooterGame.DamageableComponent.GetRawHealth",4,&target.rawHealth)
       ||!Event(target.damageable,L"/Script/ShooterGame.DamageableComponent.GetRawShield",4,&target.rawShield)
       ||!Event(target.pawn,L"/Script/ShooterGame.ShooterCharacter.IsAlive",1,&alive)
       ||!std::isfinite(target.health)||!std::isfinite(target.maxHealth)||target.health<0||target.maxHealth<=0
       ||!std::isfinite(target.shield)||!std::isfinite(target.maxShield)||target.shield<0||target.maxShield<0
       ||!std::isfinite(target.rawHealth)||!std::isfinite(target.rawShield)||target.rawHealth<0||target.rawShield<0)
        return Fail(1124,"original_damage_native_health_query_failed");
    target.alive=alive!=0;
    target.pendingDamageEvents=Read<int32_t>(target.damageable,0x248);
    target.nativeHealthVerified=target.alive==(Read<uint8_t>(target.damageable,0x1f9)!=0);
    if(!InspectPhysics(target))return false;
    target.observedMontage=Read<void*>(target.death,0x130);
    if(target.lastMontage&&target.observedMontage==target.lastMontage){
        void* anim=nullptr;void* active=nullptr;
        if(Event(target.core,L"/Script/Engine.SkeletalMeshComponent.GetAnimInstance",8,&anim)&&anim
           &&Event(anim,L"/Script/Engine.AnimInstance.GetCurrentActiveMontage",8,&active)&&active==target.lastMontage)
            ++target.montageSamples;
    }
    return target.nativeHealthVerified?true:Fail(1125,"original_damage_alive_readback_mismatch");
}
inline bool Sample(size_t index){
    __try{return SampleImpl(index);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool BindImpl(size_t index,void* originalPawn){
    if(!Thread()||index>=targets.size()||!IsA(originalPawn,original_players::ShooterCharacterClass))
        return Fail(1126,"original_damage_original_target_required");
    auto& previous=targets[index];
    if(previous.bound&&previous.pawn==originalPawn)return SampleImpl(index);
    if(previous.bound&&previous.alive)return Fail(1127,"original_damage_cannot_replace_living_target");
    Target target;target.pawn=originalPawn;target.state=Read<void*>(originalPawn,0x488);
    target.damageable=Read<void*>(originalPawn,0xc70);target.core=Read<void*>(originalPawn,0x4f0);
    target.death=Read<void*>(originalPawn,0xcb0);
    if(!IsA(target.damageable,DamageableClass)||Read<void*>(target.damageable,0xc8)!=originalPawn
       ||!target.core||!IsA(target.death,DeathComponentClass)||!IsA(target.state,original_players::ShooterStateClass))
        return Fail(1128,"original_damage_target_components_missing");
    if(!InspectSections(target,index))return false;
    if(!Read<uint8_t>(target.damageable,0x1f8))
        return Fail(1133,"original_damage_health_section_or_can_die_unavailable");
    if(!previous.bound)++diagnostics.boundTargets;
    target.bound=true;targets[index]=target;
    if(!SampleImpl(index))return false;
    diagnostics.error=0;diagnostics.stage="original_damage_target_bound";return true;
}
inline bool Bind(size_t index,void* originalPawn){
    __try{return BindImpl(index,originalPawn);}__except(Capture(GetExceptionInformation())){return false;}
}
// Component-only native trace. The caller must compare its distance with the
// original world obstruction trace before applying damage.
inline int TraceImpl(size_t index,const original_shots::Shot& shot,double maximum,Hit& hit){
    hit={};
    if(!Thread()||index>=targets.size()||!targets[index].bound||!targets[index].alive
       ||!std::isfinite(maximum)||maximum<=0||maximum>1000000
       ||!original_players::Finite(shot.start)||!original_players::Finite(shot.direction))return -1;
    auto& target=targets[index];if(!InspectPhysics(target)||!target.originalPhysics){
        Fail(1134,"original_damage_original_core_physics_not_ready");return -1;
    }
    const auto end=engine::Add(shot.start,engine::Mul(shot.direction,maximum));
    alignas(8) uint8_t params[0x170]{};Put(params,0,shot.start);Put(params,0x18,end);
    ++target.traceQueries;
    diagnostics.operation="original_core_component_body_trace";
    auto trace=ComponentTraceFunction();
    // Reflection ends after ReturnValue at168; the C++ storage has seven
    // trailing alignment bytes. Runtime ParmsSize is169, storage remains170.
    if(!EventWithFunction(target.core,L"/Script/Engine.PrimitiveComponent.K2_LineTraceComponent",trace,0x169,params)){
        Fail(1135,"original_damage_component_trace_signature_missing");return -1;
    }
    if(!params[0x168]){diagnostics.operation="none";return 0;}
    const auto distance=Read<float>(params,0x80);
    if(!std::isfinite(distance)||distance<0){Fail(1136,"original_damage_trace_distance_invalid");return -1;}
    hit.hit=true;hit.distance=distance;hit.component=target.core;
    hit.blocking=(params[0x125]&1)!=0;
    std::memcpy(hit.bone.bytes,params+0x68,sizeof(hit.bone));
    std::memcpy(&hit.location,params+0x38,sizeof(hit.location));
    std::memcpy(&hit.normal,params+0x50,sizeof(hit.normal));
    std::memcpy(hit.nativeHit,params+0x78,sizeof(hit.nativeHit));
    alignas(8) uint8_t regional[0xf8]{};
    std::memcpy(regional,hit.nativeHit,sizeof(hit.nativeHit));
    diagnostics.operation="original_core_native_region_query";
    if(!Event(target.pawn,L"/Script/ShooterGame.ShooterCharacter.GetRegionalDamage",0xf2,regional)){
        Fail(1137,"original_damage_regional_query_signature_missing");return -1;
    }
    hit.region=regional[0xf1];
    if(hit.region>2){Fail(1138,"original_damage_unexpected_original_region");return -1;}
    ++target.traceHits;diagnostics.operation="none";return 1;
}
inline int Trace(size_t index,const original_shots::Shot& shot,double maximum,Hit& hit){
    __try{return TraceImpl(index,shot,maximum,hit);}__except(Capture(GetExceptionInformation())){return -1;}
}
inline bool SelectDeathMontage(Target& target,const PointData& point,uint64_t token,DeathData& death){
    target.originalReactions=Read<void*>(target.pawn,0x11f0);
    if(!IsA(target.originalReactions,ReactionsClass)){
        // This is a const data query against the exact hero's original template
        // CDO, not a replacement animation asset or a hand-authored rule table.
        auto type=Read<void*>(target.pawn,0x11e8);
        if(!original_players::Derived(type,engine::Class(ReactionsClass)))
            return Fail(1139,"original_damage_original_death_rules_missing");
        target.originalReactions=engine::DefaultObject(type);
    }
    if(!IsA(target.originalReactions,ReactionsClass))return Fail(1139,"original_damage_original_death_rules_missing");
    engine::Vec forward{},right{},position{};
    if(!Event(target.pawn,L"/Script/Engine.Actor.GetActorForwardVector",24,&forward)
       ||!Event(target.pawn,L"/Script/Engine.Actor.GetActorRightVector",24,&right)
       ||!Event(target.pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&position))return Fail(1140,"original_damage_death_direction_query_failed");
    const engine::Vec toward{point.base.origin.x-position.x,point.base.origin.y-position.y,point.base.origin.z-position.z};
    const auto front=toward.x*forward.x+toward.y*forward.y;
    const auto side=toward.x*right.x+toward.y*right.y;
    const uint8_t direction=std::abs(front)>=std::abs(side)?(front>=0?0:3):(side>=0?2:1);
    struct Body {Name bone;uint8_t result=254,padding[3]{};} body{point.bone};
    if(!Event(target.death,L"/Script/ShooterGame.ShooterCharacterDeathReactionComponent.GetBodySectionForBone",13,&body))
        return Fail(1141,"original_damage_original_body_section_query_failed");
    auto damageCDO=engine::DefaultObject(point.base.damageType);
    if(!IsA(damageCDO,DamageTypeClass))return Fail(1142,"original_damage_original_damage_type_required");
    struct Lookup {
        uint8_t animation=0,directional=0,regional=0,category=0,body=0,padding[3]{};
        Name bone;uint32_t padding20=0;
        void* damageType=nullptr;float amount=0;uint32_t padding36=0;
        Array<ReactionResult> results;
    } lookup;
    static_assert(sizeof(Lookup)==56&&offsetof(Lookup,results)==40);
    lookup.animation=Read<uint8_t>(damageCDO,0xfc);lookup.directional=direction;lookup.regional=point.base.region;
    lookup.category=Read<uint8_t>(damageCDO,0xf8);lookup.body=body.result;lookup.bone=point.bone;
    lookup.damageType=damageCDO;lookup.amount=point.base.dealt;
    if(!Event(target.originalReactions,L"/Script/ShooterGame.AresHitReactions.LookupHitReactionResults",56,&lookup))
        return Fail(1143,"original_damage_original_reaction_lookup_missing");
    const auto array=Read<void*>(target.originalReactions,0x30);
    const int32_t count=Read<int32_t>(target.originalReactions,0x38);
    bool valid=count>0&&count<=4096&&lookup.results.count>=0&&lookup.results.count<=4096
        &&lookup.results.capacity>=lookup.results.count
        &&(!lookup.results.count||original_players::Readable(lookup.results.data,static_cast<size_t>(lookup.results.count)*16));
    int32_t best=0;uint32_t ties=0;
    if(valid){
        for(int32_t i=0;i<lookup.results.count;++i){
            const auto& result=lookup.results.data[i];
            const auto at=reinterpret_cast<uintptr_t>(result.entry),base=reinterpret_cast<uintptr_t>(array);
            if(at<base||at-base>=static_cast<size_t>(count)*56||(at-base)%56){valid=false;break;}
            auto montage=Read<void*>(result.entry,0x30);
            if(result.score>best&&IsA(montage,0x3f19390)){best=result.score;ties=1;}
            else if(result.score==best&&best>0&&IsA(montage,0x3f19390))++ties;
        }
        if(best>0&&ties){
            uint32_t selected=static_cast<uint32_t>(token%ties);
            for(int32_t i=0;i<lookup.results.count;++i){
                const auto& result=lookup.results.data[i];auto montage=Read<void*>(result.entry,0x30);
                if(result.score==best&&IsA(montage,0x3f19390)&&selected--==0){death.montage=montage;break;}
            }
        }
    }
    target.lastReactionCandidates=lookup.results.count;target.lastReactionScore=best;
    if(lookup.results.data)engine::function<void(*)(void*)>(0x1915720)(lookup.results.data);
    if(!valid||!death.montage)return Fail(1144,"original_damage_no_original_death_montage_match");
    ++diagnostics.originalMontageSelections;return true;
}
struct DamageCalculation {
    float damage=0;uint32_t padding4=0;void* type=nullptr;void* instigator=nullptr;
    void* causer=nullptr;int64_t source=0;uint8_t region=0,padding41[3]{};
    float applied=0,remaining=0;uint8_t simulation=1,padding53[3]{};
    float result=0;uint32_t padding60=0;
};
static_assert(sizeof(DamageCalculation)==64&&offsetof(DamageCalculation,applied)==44);
struct Penetration {uint8_t target=0,padding[3]{};float amount=0;};
static_assert(sizeof(Penetration)==8&&offsetof(Penetration,amount)==4);
inline bool NativeSectionCalculation(void* section,float damage,void* type,void* instigator,void* causer,
                                     uint8_t region,DamageCalculation& calculation){
    calculation={};calculation.damage=damage;calculation.type=type;calculation.instigator=instigator;
    calculation.causer=causer;calculation.region=region;
    const float life=Read<float>(section,0x124);
    if(!std::isfinite(damage)||damage<0||!std::isfinite(life)||life<0
       ||!Event(section,L"/Script/ShooterGame.DamageSectionComponent.AuthApplyDamage",60,&calculation)
       ||!std::isfinite(calculation.applied)||!std::isfinite(calculation.remaining)
       ||calculation.applied<0||calculation.applied>life+.01f||calculation.remaining<-.01f
       ||std::abs((calculation.applied+calculation.remaining)-damage)>.05f
       ||Read<float>(section,0x124)!=life)
        return Fail(1150,"original_damage_section_simulation_not_verified");
    ++diagnostics.nativeSectionCalculations;return true;
}
inline bool OriginalHealthPenetration(void* damageType,float& penetration){
    auto defaults=engine::DefaultObject(damageType);penetration=0;
    if(!IsA(defaults,DamageTypeClass))return Fail(1157,"original_damage_actual_type_cdo_missing");
    const auto healthScale=Read<float>(defaults,0x94),shieldScale=Read<float>(defaults,0x98);
    if(!std::isfinite(healthScale)||!std::isfinite(shieldScale)
       ||healthScale!=1.0f||shieldScale!=1.0f)
        return Fail(1158,"original_damage_nonunit_type_scales_require_separate_native_partition_evidence");
    if(Read<uint8_t>(defaults,0x90)){penetration=1;return true;}
    const auto values=Read<Array<Penetration>>(defaults,0x108);
    if(values.count<0||values.count>16||values.capacity<values.count
       ||(values.count&&!original_players::Readable(values.data,static_cast<size_t>(values.count)*sizeof(Penetration))))
        return Fail(1159,"original_damage_native_penetration_array_invalid");
    bool found=false;
    for(int32_t i=0;i<values.count;++i){const auto& value=values.data[i];
        if(value.target!=0||found||!std::isfinite(value.amount)||value.amount<0||value.amount>1)
            return Fail(1160,"original_damage_nonstandard_penetration_requires_separate_authority_evidence");
        found=true;penetration=value.amount;
    }
    return true;
}
inline bool ApplyPointImpl(size_t index,void* sourcePawn,void* equippable,void* actualDamageType,
                           const original_shots::Shot& shot,const Hit& hit,float requestedDamage,
                           uint64_t token,Result& result,const DeathData* localDeathRecord,float nativeFalloff){
    result={};
    if(!Thread()||index>=targets.size()||!targets[index].bound||!token||!hit.hit
       ||!std::isfinite(requestedDamage)||requestedDamage<=0||requestedDamage>1000000
       ||!std::isfinite(nativeFalloff)||nativeFalloff<0
       ||!original_players::Finite(shot.start)||!original_players::Finite(shot.direction)
       ||!original_players::Finite(hit.location)||!original_players::Finite(hit.normal)
       ||!IsA(sourcePawn,original_players::ShooterCharacterClass)||!IsA(equippable,0x645c660)
       ||!original_players::Derived(actualDamageType,engine::Class(DamageTypeClass)))
        return Fail(1145,"original_damage_point_context_invalid");
    auto& target=targets[index];
    for(auto previous:target.appliedTokens)
        if(previous==token){result.duplicate=true;++diagnostics.duplicateEvents;return true;}
    if(!SampleImpl(index)||!target.alive)return Fail(1146,"original_damage_target_not_alive");
    if(hit.component!=target.core||Read<void*>(equippable,0x4a8)!=sourcePawn)
        return Fail(1147,"original_damage_hit_or_equippable_owner_mismatch");
    if(Read<int32_t>(target.damageable,0x248)!=0)return Fail(1148,"original_damage_previous_notification_still_queued");
    if(!InspectSections(target,index))return false;
    const auto life=Read<float>(target.healthSection,0x124);
    if(!std::isfinite(life)||life<0||std::abs(life-target.health)>.01f)
        return Fail(1149,"original_damage_health_section_readback_mismatch");
    void* instigator=Read<void*>(sourcePawn,0x4a0);
    DamageCalculation healthCalculation,shieldCalculation;
    const float shieldLife=target.shieldSection?Read<float>(target.shieldSection,0x124):0;
    float healthBudget=requestedDamage,penetration=0;
    if(target.shieldSection){
        // The outer server partition was not recovered. This is explicitly
        // local authority using the real type's serialized Penetration data,
        // limited to the ordinary unit-scale Health+Shield schema. Each section
        // still uses original clamping/nonlethal logic and native replication.
        if(!std::isfinite(shieldLife)||shieldLife<0||std::abs(target.rawShield-shieldLife)>.01f
           ||Read<float>(target.healthSection,0x144)!=1.0f||Read<float>(target.shieldSection,0x144)!=1.0f
           ||!Read<uint8_t>(target.shieldSection,0x148))
            return Fail(1161,"original_damage_nonstandard_shield_section_not_supported");
        if(!OriginalHealthPenetration(actualDamageType,penetration))return false;
        const float bypass=requestedDamage*penetration;
        if(!NativeSectionCalculation(target.shieldSection,requestedDamage-bypass,actualDamageType,instigator,equippable,hit.region,shieldCalculation))return false;
        healthBudget=bypass+shieldCalculation.remaining;++diagnostics.localArmoredPartitions;
    }
    if(!NativeSectionCalculation(target.healthSection,healthBudget,actualDamageType,instigator,equippable,hit.region,healthCalculation))return false;
    const auto totalApplied=healthCalculation.applied+shieldCalculation.applied;
    if(totalApplied<=0){diagnostics.error=0;diagnostics.stage="original_damage_original_sections_rejected_damage";return true;}
    LifeChange healthChange;healthChange.section=target.healthSection;
    healthChange.life=(std::max)(0.0f,life-healthCalculation.applied);healthChange.delta=-healthCalculation.applied;
    healthChange.alive=healthChange.life>0?1:0;
    LifeChange shieldChange;shieldChange.section=target.shieldSection;
    shieldChange.life=(std::max)(0.0f,shieldLife-shieldCalculation.applied);shieldChange.delta=-shieldCalculation.applied;
    shieldChange.alive=shieldChange.life>0?1:0;
    std::array<LifeChange,2> changes{};int32_t changeCount=0;
    // AuthAttachDamageSection already runs original6fae770: Type descending,
    // then penetration flag and priority. Preserve that native array order.
    auto sections=Read<Array<void*>>(target.damageable,0x1e8);
    for(int32_t i=0;i<sections.count;++i){
        if(sections.data[i]==target.shieldSection&&shieldCalculation.applied>0)changes[changeCount++]=shieldChange;
        else if(sections.data[i]==target.healthSection&&healthCalculation.applied>0)changes[changeCount++]=healthChange;
    }
    PointData point;point.base.taken=totalApplied;point.base.dealt=requestedDamage;
    point.base.killed=healthChange.alive?0:1;point.base.alive=healthChange.alive;
    point.base.instigator=instigator;point.base.causer=equippable;
    point.base.origin=shot.start;point.base.equippable=equippable;point.base.damageType=actualDamageType;
    point.base.changes={changes.data(),changeCount,static_cast<int32_t>(changes.size())};point.base.instigatorPawn=sourcePawn;
    point.base.damagerState=Read<void*>(sourcePawn,0x488);point.base.killCreditState=point.base.damagerState;
    point.base.region=hit.region;
    if(!Event(target.pawn,L"/Script/ShooterGame.ShooterCharacter.GetCurrentCharacterTimestamp",16,&point.base.timestamp.movement)
       ||!Event(target.pawn,L"/Script/ShooterGame.ShooterCharacter.GetRespawnNumber",4,&point.base.victimRespawn))
        return Fail(1151,"original_damage_native_character_timestamp_missing");
    const auto eventIndex=Read<int32_t>(target.damageable,0x1e0);
    if(eventIndex==(std::numeric_limits<int32_t>::max)())return Fail(1152,"original_damage_native_event_index_exhausted");
    point.base.timestamp.eventIndex=(std::max)(eventIndex,0)+1;
    point.hitComponent=hit.component;point.bone=hit.bone;point.falloff=nativeFalloff;
    point.direction=shot.direction;point.impact=hit.location;point.normal=hit.normal;
    DeathData death=localDeathRecord?*localDeathRecord:DeathData{};
    if(point.base.killed&&!SelectDeathMontage(target,point,token,death))return false;
    diagnostics.operation="original_create_point_damage_response";
    // Exactly the three original operations performed by native multicast
    // implementation6fa05a0, retaining the original constructed response pointer
    // for the caller's immediate score/feedback event. No fake UObject/vtable.
    auto response=engine::function<void*(*)(void*)>(0x6f56010)(target.damageable);
    if(!IsA(response,PointResponseClass))return Fail(1153,"original_damage_response_constructor_failed");
    engine::function<void(*)(void*,void*,const PointData*)>(0x6f9ce10)(response,target.pawn,&point);
    result.response=response;result.originalResponse=true;++diagnostics.nativeResponses;
    diagnostics.operation="original_client_damage_and_death_notification";
    engine::function<void(*)(void*,void*,const DeathData*)>(0x6f9fe30)(target.damageable,response,&death);
    target.lastResponse=response;target.lastMontage=death.montage;
    target.lastToken=token;target.lastHealthBefore=life;target.lastShieldBefore=shieldLife;
    target.lastHealthTaken=healthCalculation.applied;target.lastShieldTaken=shieldCalculation.applied;target.lastHealthPenetration=penetration;
    target.appliedTokens[target.nextToken++%target.appliedTokens.size()]=token;
    target.lastNativeEventIndex=point.base.timestamp.eventIndex;
    if(!SampleImpl(index))return false;
    target.lastHealthAfter=target.health;target.lastShieldAfter=target.rawShield;
    const bool expectedAlive=healthChange.alive!=0;
    result.originalLifeReadback=std::abs(target.health-healthChange.life)<.01f&&target.alive==expectedAlive
        &&std::abs(Read<float>(target.healthSection,0x124)-healthChange.life)<.01f
        &&(!target.shieldSection||(std::abs(target.rawShield-shieldChange.life)<.01f
            &&std::abs(Read<float>(target.shieldSection,0x124)-shieldChange.life)<.01f));
    if(!result.originalLifeReadback)return Fail(1154,"original_damage_native_life_or_alive_notification_not_applied");
    result.applied=true;result.killed=point.base.killed!=0;result.healthBefore=life;result.healthAfter=target.health;
    result.taken=totalApplied;result.region=hit.region;result.montage=death.montage;
    result.shieldBefore=shieldLife;result.shieldAfter=target.rawShield;
    result.healthTaken=healthCalculation.applied;result.shieldTaken=shieldCalculation.applied;result.sectionChanges=static_cast<unsigned>(changeCount);
    result.originalDeathMontage=death.montage&&Read<void*>(target.death,0x130)==death.montage;
    ++target.damageEvents;++diagnostics.damageEvents;++diagnostics.nativeHealthReadbacks;
    if(result.killed){++target.deathEvents;++diagnostics.deathEvents;}
    diagnostics.error=0;diagnostics.operation="none";diagnostics.stage="original_damage_native_notification_applied";return true;
}
inline bool ApplyPoint(size_t index,void* sourcePawn,void* equippable,void* actualDamageType,
                       const original_shots::Shot& shot,const Hit& hit,float requestedDamage,
                       uint64_t token,Result& result,const DeathData* localDeathRecord=nullptr,float nativeFalloff=1){
    __try{return ApplyPointImpl(index,sourcePawn,equippable,actualDamageType,shot,hit,requestedDamage,token,result,localDeathRecord,nativeFalloff);}
    __except(Capture(GetExceptionInformation())){return false;}
}

// Additional HOST producers for actual accepted native Base/Radial events.
// These are separate wire types; neither is serialized as a fake point hit.
// Provenance: work/local-server-playable/hero-abilities/group0-damage.
enum class NativeEventKind:uint8_t {Base,Point,Radial,Unsupported};
struct NativeHitResult {alignas(8) uint8_t bytes[0xf0]{};};
struct RadialParams {float base=0,minimum=0,inner=0,outer=0,falloff=0;};
struct NativeRadialEvent {
    void* table=nullptr;void* damageType=nullptr;
    RadialParams params{};uint32_t padding36=0;
    engine::Vec origin{};
    Array<NativeHitResult> componentHits{};
};
struct RadialData {
    ResponseData base{};NativeRadialEvent event{};
    void* hitComponent=nullptr;Name bone{};uint32_t padding244=0;
    engine::Vec direction{},impact{},normal{};
};
struct NativeEventGeometry {
    NativeEventKind kind=NativeEventKind::Unsupported;
    engine::Vec origin{},direction{};Hit hit{};NativeRadialEvent radial{};
};
static_assert(sizeof(NativeHitResult)==0xf0&&sizeof(RadialParams)==20&&sizeof(NativeRadialEvent)==0x50);
static_assert(offsetof(NativeRadialEvent,params)==0x10&&offsetof(NativeRadialEvent,origin)==0x28
    &&offsetof(NativeRadialEvent,componentHits)==0x40);
static_assert(sizeof(RadialData)==0x140&&offsetof(RadialData,event)==0x90
    &&offsetof(RadialData,hitComponent)==0xe0&&offsetof(RadialData,bone)==0xe8
    &&offsetof(RadialData,direction)==0xf8&&offsetof(RadialData,impact)==0x110
    &&offsetof(RadialData,normal)==0x128);

inline bool nativeEventResponseProfilesVerified=false;
inline bool VerifyNativeEventResponseProfiles(){
    if(nativeEventResponseProfilesVerified)return true;
    struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
    constexpr Profile profiles[]{
        {0x6fa04c0,111,0x2a8cc23b98582b1dULL},{0x6fa0530,111,0x1d6ff3ecfa1c5bf3ULL},
        {0x6f56150,154,0xdd52f1c71c6a4ebaULL},{0x6f560b0,154,0x4938643ad731a168ULL},
        {0x6fa4160,127,0x9fedec9b26817423ULL},{0x6f9d340,365,0xcc7d7d7dad729575ULL},
        {0x6f9d4b0,115,0x168bf54835793186ULL},{0x6f9d530,324,0xdce8c8c0684cd855ULL},
        {0x6f9d1a0,270,0xeb4cc9942c4b2788ULL}
    };
    for(const auto& profile:profiles){auto bytes=reinterpret_cast<const uint8_t*>(engine::image+profile.rva);
        if(!original_players::Readable(bytes,profile.bytes))return Fail(1188,"original_event_damage_native_profile_unreadable");
        uint64_t digest=1469598103934665603ULL;
        for(size_t i=0;i<profile.bytes;++i)digest=(digest^bytes[i])*1099511628211ULL;
        if(digest!=profile.fnv)return Fail(1189,"original_event_damage_native_profile_mismatch");
    }
    struct Method {const wchar_t* name;uint16_t bytes;uintptr_t exec;};
    constexpr Method methods[]{
        {L"/Script/ShooterGame.DamageableComponent.MulticastNotifyDamage_Base",0xc8,0x6705c20},
        {L"/Script/ShooterGame.DamageableComponent.MulticastNotifyDamage_Radial",0x178,0x67056a0}
    };
    for(const auto& method:methods){auto fn=engine::Find(method.name);
        if(!fn||Read<uint16_t>(fn,0xbe)!=method.bytes||Read<uintptr_t>(fn,0xe0)!=engine::image+method.exec
           ||Read<void*>(fn,0x28)!=engine::Class(DamageableClass))
            return Fail(1190,"original_event_damage_typed_multicast_metadata_mismatch");
    }
    nativeEventResponseProfilesVerified=true;return true;
}

inline bool SelectNativeEventDeathMontage(Target& target,const ResponseData& base,const Name& nativeBone,
                                          uint64_t token,DeathData& death){
    target.originalReactions=Read<void*>(target.pawn,0x11f0);
    if(!IsA(target.originalReactions,ReactionsClass)){
        auto type=Read<void*>(target.pawn,0x11e8);
        if(!original_players::Derived(type,engine::Class(ReactionsClass)))
            return Fail(1170,"original_event_damage_original_death_rules_missing");
        target.originalReactions=engine::DefaultObject(type);
    }
    if(!IsA(target.originalReactions,ReactionsClass))return Fail(1170,"original_event_damage_original_death_rules_missing");
    engine::Vec forward{},right{},position{};
    if(!Event(target.pawn,L"/Script/Engine.Actor.GetActorForwardVector",24,&forward)
       ||!Event(target.pawn,L"/Script/Engine.Actor.GetActorRightVector",24,&right)
       ||!Event(target.pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&position))
        return Fail(1171,"original_event_damage_death_direction_query_failed");
    const engine::Vec toward{base.origin.x-position.x,base.origin.y-position.y,base.origin.z-position.z};
    const auto front=toward.x*forward.x+toward.y*forward.y;
    const auto side=toward.x*right.x+toward.y*right.y;
    const uint8_t direction=std::abs(front)>=std::abs(side)?(front>=0?0:3):(side>=0?2:1);
    struct Body {Name bone;uint8_t result=254,padding[3]{};} body{nativeBone};
    if(!Event(target.death,L"/Script/ShooterGame.ShooterCharacterDeathReactionComponent.GetBodySectionForBone",13,&body))
        return Fail(1172,"original_event_damage_original_body_section_query_failed");
    auto damageCDO=engine::DefaultObject(base.damageType);
    if(!IsA(damageCDO,DamageTypeClass))return Fail(1173,"original_event_damage_original_damage_type_missing");
    struct Lookup {
        uint8_t animation=0,directional=0,regional=0,category=0,body=0,padding[3]{};
        Name bone{};uint32_t padding20=0;
        void* damageType=nullptr;float amount=0;uint32_t padding36=0;
        Array<ReactionResult> results{};
    } lookup;
    static_assert(sizeof(Lookup)==56&&offsetof(Lookup,results)==40);
    lookup.animation=Read<uint8_t>(damageCDO,0xfc);lookup.directional=direction;lookup.regional=base.region;
    lookup.category=Read<uint8_t>(damageCDO,0xf8);lookup.body=body.result;lookup.bone=nativeBone;
    lookup.damageType=damageCDO;lookup.amount=base.dealt;
    if(!Event(target.originalReactions,L"/Script/ShooterGame.AresHitReactions.LookupHitReactionResults",56,&lookup))
        return Fail(1174,"original_event_damage_original_reaction_lookup_failed");
    const auto array=Read<void*>(target.originalReactions,0x30);
    const auto count=Read<int32_t>(target.originalReactions,0x38);
    bool valid=count>0&&count<=4096&&lookup.results.count>=0&&lookup.results.count<=4096
        &&lookup.results.capacity>=lookup.results.count
        &&(!lookup.results.count||original_players::Readable(lookup.results.data,static_cast<size_t>(lookup.results.count)*16));
    int32_t best=0;uint32_t ties=0;
    if(valid){
        for(int32_t i=0;i<lookup.results.count;++i){const auto& row=lookup.results.data[i];
            const auto at=reinterpret_cast<uintptr_t>(row.entry),first=reinterpret_cast<uintptr_t>(array);
            if(at<first||at-first>=static_cast<size_t>(count)*56||(at-first)%56){valid=false;break;}
            auto montage=Read<void*>(row.entry,0x30);
            if(row.score>best&&IsA(montage,0x3f19390)){best=row.score;ties=1;}
            else if(row.score==best&&best>0&&IsA(montage,0x3f19390))++ties;
        }
        if(best>0&&ties){uint32_t selected=static_cast<uint32_t>(token%ties);
            for(int32_t i=0;i<lookup.results.count;++i){const auto& row=lookup.results.data[i];
                auto montage=Read<void*>(row.entry,0x30);
                if(row.score==best&&IsA(montage,0x3f19390)&&selected--==0){death.montage=montage;break;}
            }
        }
    }
    target.lastReactionCandidates=lookup.results.count;target.lastReactionScore=best;
    if(lookup.results.data)engine::function<void(*)(void*)>(0x1915720)(lookup.results.data);
    if(!valid||!death.montage)return Fail(1175,"original_event_damage_no_original_death_montage_match");
    ++diagnostics.originalMontageSelections;return true;
}

inline bool ApplyNativeEventImpl(size_t index,void* sourcePawn,void* equippable,void* actualCauser,void* actualDamageType,
                                const NativeEventGeometry& geometry,float acceptedNativeDamage,uint64_t token,
                                Result& result,const DeathData* localDeathRecord){
    result={};
    if(!Thread()||!VerifyNativeEventResponseProfiles()||index>=targets.size()||!targets[index].bound||!token
       ||(geometry.kind!=NativeEventKind::Base&&geometry.kind!=NativeEventKind::Radial)
       ||!std::isfinite(acceptedNativeDamage)||acceptedNativeDamage<=0||acceptedNativeDamage>1000000
       ||!original_players::Finite(geometry.origin)||!original_players::Finite(geometry.direction)
       ||!original_players::Finite(geometry.hit.location)||!original_players::Finite(geometry.hit.normal)
       ||geometry.hit.region>2||!IsA(sourcePawn,original_players::ShooterCharacterClass)
       ||!IsA(equippable,0x645c660)||Read<void*>(equippable,0x4a8)!=sourcePawn
       ||!IsA(actualCauser,0x3d67f20)||Read<uint8_t>(actualCauser,0x224)!=3
       ||!original_players::Derived(actualDamageType,engine::Class(DamageTypeClass)))
        return Fail(1176,"original_event_damage_native_context_invalid_or_event_kind_unsupported");
    if(geometry.kind==NativeEventKind::Radial){const auto& event=geometry.radial;const auto& hits=event.componentHits;
        if(event.table!=reinterpret_cast<void*>(engine::image+0xab31980)||event.damageType!=actualDamageType
           ||!original_players::Finite(event.origin)||hits.count<0||hits.count>128||hits.capacity<hits.count
           ||(hits.count&&!original_players::Readable(hits.data,static_cast<size_t>(hits.count)*sizeof(NativeHitResult))))
            return Fail(1177,"original_event_damage_actual_radial_event_not_retained");
    }
    auto& target=targets[index];
    for(auto applied:target.appliedTokens)if(applied==token){result.duplicate=true;++diagnostics.duplicateEvents;return true;}
    if(!SampleImpl(index)||!target.alive)return Fail(1178,"original_event_damage_target_not_alive");
    if(Read<int32_t>(target.damageable,0x248)!=0)return Fail(1179,"original_event_damage_previous_notification_still_queued");
    if(!InspectSections(target,index))return false;
    const auto life=Read<float>(target.healthSection,0x124);
    const auto shieldLife=target.shieldSection?Read<float>(target.shieldSection,0x124):0;
    if(!std::isfinite(life)||life<0||std::abs(life-target.health)>.01f)
        return Fail(1180,"original_event_damage_native_health_readback_mismatch");
    void* instigator=Read<void*>(sourcePawn,0x4a0);
    DamageCalculation healthCalculation,shieldCalculation;
    float healthBudget=acceptedNativeDamage,penetration=0;
    if(target.shieldSection){
        // Preserve the existing explicit local authority boundary: no guessed
        // nonunit scales or new shield formula. Both partitions still execute
        // the original section simulation and the actual type penetration.
        if(!std::isfinite(shieldLife)||shieldLife<0||std::abs(target.rawShield-shieldLife)>.01f
           ||Read<float>(target.healthSection,0x144)!=1.0f||Read<float>(target.shieldSection,0x144)!=1.0f
           ||!Read<uint8_t>(target.shieldSection,0x148))
            return Fail(1181,"original_event_damage_nonstandard_shield_section_unsupported");
        if(!OriginalHealthPenetration(actualDamageType,penetration))return false;
        const auto bypass=acceptedNativeDamage*penetration;
        if(!NativeSectionCalculation(target.shieldSection,acceptedNativeDamage-bypass,actualDamageType,
            instigator,actualCauser,geometry.hit.region,shieldCalculation))return false;
        healthBudget=bypass+shieldCalculation.remaining;++diagnostics.localArmoredPartitions;
    }
    if(!NativeSectionCalculation(target.healthSection,healthBudget,actualDamageType,instigator,
        actualCauser,geometry.hit.region,healthCalculation))return false;
    const auto totalApplied=healthCalculation.applied+shieldCalculation.applied;
    if(totalApplied<=0){diagnostics.error=0;diagnostics.stage="original_event_damage_original_sections_rejected_damage";return true;}
    LifeChange healthChange;healthChange.section=target.healthSection;
    healthChange.life=(std::max)(0.0f,life-healthCalculation.applied);healthChange.delta=-healthCalculation.applied;
    healthChange.alive=healthChange.life>0?1:0;
    LifeChange shieldChange;shieldChange.section=target.shieldSection;
    shieldChange.life=(std::max)(0.0f,shieldLife-shieldCalculation.applied);shieldChange.delta=-shieldCalculation.applied;
    shieldChange.alive=shieldChange.life>0?1:0;
    std::array<LifeChange,2> changes{};int32_t changeCount=0;
    const auto sections=Read<Array<void*>>(target.damageable,0x1e8);
    for(int32_t i=0;i<sections.count;++i){
        if(sections.data[i]==target.shieldSection&&shieldCalculation.applied>0)changes[changeCount++]=shieldChange;
        else if(sections.data[i]==target.healthSection&&healthCalculation.applied>0)changes[changeCount++]=healthChange;
    }
    ResponseData base{};base.taken=totalApplied;base.dealt=acceptedNativeDamage;
    base.killed=healthChange.alive?0:1;base.alive=healthChange.alive;
    base.instigator=instigator;base.causer=actualCauser;base.origin=geometry.origin;base.equippable=equippable;
    base.damageType=actualDamageType;base.changes={changes.data(),changeCount,static_cast<int32_t>(changes.size())};
    base.instigatorPawn=sourcePawn;base.damagerState=Read<void*>(sourcePawn,0x488);base.killCreditState=base.damagerState;
    base.region=geometry.hit.region;
    if(!Event(target.pawn,L"/Script/ShooterGame.ShooterCharacter.GetCurrentCharacterTimestamp",16,&base.timestamp.movement)
       ||!Event(target.pawn,L"/Script/ShooterGame.ShooterCharacter.GetRespawnNumber",4,&base.victimRespawn))
        return Fail(1182,"original_event_damage_native_character_timestamp_missing");
    const auto eventIndex=Read<int32_t>(target.damageable,0x1e0);
    if(eventIndex==(std::numeric_limits<int32_t>::max)())return Fail(1183,"original_event_damage_event_index_exhausted");
    base.timestamp.eventIndex=(std::max)(eventIndex,0)+1;
    DeathData death=localDeathRecord?*localDeathRecord:DeathData{};
    if(base.killed&&!SelectNativeEventDeathMontage(target,base,geometry.hit.bone,token,death))return false;
    // Same original factory argument loaded by native6fa04c0/6fa0530.
    auto outer=Read<void*>(reinterpret_cast<void*>(engine::image+0xd98ee20),0);
    if(!outer)return Fail(1184,"original_event_damage_original_response_outer_missing");
    void* response=nullptr;
    if(geometry.kind==NativeEventKind::Base){
        response=engine::function<void*(*)(void*)>(0x6f56150)(outer);
        if(!IsA(response,0x64f43e0))return Fail(1185,"original_event_damage_base_response_factory_failed");
        engine::function<void(*)(void*,void*,const ResponseData*)>(0x6fa4160)(response,target.pawn,&base);
    }else{
        RadialData radial{};radial.base=base;radial.event=geometry.radial;
        radial.hitComponent=geometry.hit.component;radial.bone=geometry.hit.bone;
        radial.direction=geometry.direction;radial.impact=geometry.hit.location;radial.normal=geometry.hit.normal;
        response=engine::function<void*(*)(void*)>(0x6f560b0)(outer);
        if(!IsA(response,0x6501dc0))return Fail(1186,"original_event_damage_radial_response_factory_failed");
        engine::function<void(*)(void*,void*,const RadialData*)>(0x6f9d340)(response,target.pawn,&radial);
    }
    result.response=response;result.originalResponse=true;++diagnostics.nativeResponses;
    engine::function<void(*)(void*,void*,const DeathData*)>(0x6f9fe30)(target.damageable,response,&death);
    target.lastResponse=response;target.lastMontage=death.montage;target.lastToken=token;
    target.lastHealthBefore=life;target.lastShieldBefore=shieldLife;
    target.lastHealthTaken=healthCalculation.applied;target.lastShieldTaken=shieldCalculation.applied;
    target.lastHealthPenetration=penetration;target.lastNativeEventIndex=base.timestamp.eventIndex;
    target.appliedTokens[target.nextToken++%target.appliedTokens.size()]=token;
    if(!SampleImpl(index))return false;
    target.lastHealthAfter=target.health;target.lastShieldAfter=target.rawShield;
    result.originalLifeReadback=std::abs(target.health-healthChange.life)<.01f&&target.alive==(healthChange.alive!=0)
        &&std::abs(Read<float>(target.healthSection,0x124)-healthChange.life)<.01f
        &&(!target.shieldSection||(std::abs(target.rawShield-shieldChange.life)<.01f
            &&std::abs(Read<float>(target.shieldSection,0x124)-shieldChange.life)<.01f));
    if(!result.originalLifeReadback)return Fail(1187,"original_event_damage_native_life_notification_not_applied");
    result.applied=true;result.killed=base.killed!=0;result.healthBefore=life;result.healthAfter=target.health;
    result.taken=totalApplied;result.region=geometry.hit.region;result.montage=death.montage;
    result.shieldBefore=shieldLife;result.shieldAfter=target.rawShield;
    result.healthTaken=healthCalculation.applied;result.shieldTaken=shieldCalculation.applied;
    result.sectionChanges=static_cast<unsigned>(changeCount);
    result.originalDeathMontage=death.montage&&Read<void*>(target.death,0x130)==death.montage;
    ++target.damageEvents;++diagnostics.damageEvents;++diagnostics.nativeHealthReadbacks;
    if(result.killed){++target.deathEvents;++diagnostics.deathEvents;}
    diagnostics.error=0;diagnostics.operation="none";diagnostics.stage="original_event_damage_typed_native_notification_applied";return true;
}
inline bool ApplyNativeEvent(size_t index,void* sourcePawn,void* equippable,void* actualCauser,void* actualDamageType,
                            const NativeEventGeometry& geometry,float acceptedNativeDamage,uint64_t token,
                            Result& result,const DeathData* localDeathRecord=nullptr){
    __try{return ApplyNativeEventImpl(index,sourcePawn,equippable,actualCauser,actualDamageType,
        geometry,acceptedNativeDamage,token,result,localDeathRecord);}
    __except(Capture(GetExceptionInformation())){return false;}
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    const auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"stage\":");original_players::Text(f,diagnostics.stage);
    std::fprintf(f,",\"operation\":");original_players::Text(f,diagnostics.operation);
    std::fputs(",\"failed_event\":",f);const auto failed=original_players::Utf8(diagnostics.failedEvent);
    original_players::Text(f,failed.c_str());
    std::fprintf(f,",\"event_failures\":%u,\"failed_event_expected_parms_size\":%u,\"failed_event_actual_parms_size\":%u,\"failed_event_has_object\":%s,\"failed_event_has_function\":%s,\"failed_event_has_process_event\":%s,\"failed_event_function\":\"%p\",\"component_trace_function\":\"%p\",\"trace_reflection_resolutions\":%u,\"trace_lookup_missing\":%u,\"trace_function_owner_verified\":%s,\"trace_lookup_matches\":%s",
        diagnostics.eventFailures,static_cast<unsigned>(diagnostics.failedEventExpected),static_cast<unsigned>(diagnostics.failedEventActual),
        b(diagnostics.failedEventHasObject),b(diagnostics.failedEventHasFunction),b(diagnostics.failedEventHasProcessEvent),
        diagnostics.failedEventFunction,diagnostics.componentTraceFunction,diagnostics.traceReflectionResolutions,diagnostics.traceLookupMissing,
        b(diagnostics.traceFunctionOwnerVerified),b(diagnostics.traceLookupMatches));
    std::fprintf(f,",\"component_trace_lookup\":\"%p\",\"component_trace_candidate\":\"%p\",\"component_trace_candidate_parms_size\":%u",
        diagnostics.componentTraceLookup,diagnostics.componentTraceCandidate,static_cast<unsigned>(diagnostics.componentTraceCandidateSize));
    std::fprintf(f,",\"native_section_calculations\":%u,\"local_armored_partitions\":%u",diagnostics.nativeSectionCalculations,diagnostics.localArmoredPartitions);
    std::fprintf(f,",\"error\":%u,\"exception_rva\":%llu,\"restart_required\":%s,\"initialized\":%s,\"bound_targets\":%u,\"damage_events\":%u,\"death_events\":%u,\"duplicate_events\":%u,\"native_responses\":%u,\"native_health_readbacks\":%u,\"original_montage_selections\":%u,\"targets\":[",
        diagnostics.error,static_cast<unsigned long long>(diagnostics.exceptionRva),b(diagnostics.restartRequired),b(diagnostics.initialized),
        diagnostics.boundTargets,diagnostics.damageEvents,diagnostics.deathEvents,diagnostics.duplicateEvents,
        diagnostics.nativeResponses,diagnostics.nativeHealthReadbacks,diagnostics.originalMontageSelections);
    bool comma=false;
    for(size_t i=0;i<targets.size();++i){const auto& t=targets[i];if(!t.bound)continue;
        std::fprintf(f,"%s{\"index\":%zu,\"alive\":%s,\"health\":%.9g,\"max_health\":%.9g,\"native_health_verified\":%s,\"original_physics\":%s,\"body_count\":%d,\"physics_actors\":%d,\"pending_damage_events\":%d,\"damage_events\":%u,\"death_events\":%u,\"trace_queries\":%u,\"trace_hits\":%u,\"montage_playback_samples\":%u,\"last_native_event_index\":%d,\"last_reaction_score\":%d,\"last_reaction_candidates\":%d,\"last_health_before\":%.9g,\"last_health_after\":%.9g,\"last_token\":%llu,\"pawn\":\"%p\",\"damageable\":\"%p\",\"health_section\":\"%p\",\"last_response\":\"%p\",\"original_reactions\":\"%p\",\"last_montage\":\"%p\",\"observed_montage\":\"%p\"}",
            comma?",":"",i,b(t.alive),t.health,t.maxHealth,b(t.nativeHealthVerified),b(t.originalPhysics),t.bodyCount,t.physicsActors,
            t.pendingDamageEvents,t.damageEvents,t.deathEvents,t.traceQueries,t.traceHits,t.montageSamples,t.lastNativeEventIndex,
            t.lastReactionScore,t.lastReactionCandidates,t.lastHealthBefore,t.lastHealthAfter,static_cast<unsigned long long>(t.lastToken),
            t.pawn,t.damageable,t.healthSection,t.lastResponse,t.originalReactions,t.lastMontage,t.observedMontage);comma=true;
    }
    std::fputs("],\"shield_targets\":[",f);comma=false;
    for(size_t i=0;i<targets.size();++i){const auto& t=targets[i];if(!t.bound)continue;
        std::fprintf(f,"%s{\"index\":%zu,\"section_count\":%d,\"shield_section\":\"%p\",\"raw_health\":%.9g,\"shield\":%.9g,\"max_shield\":%.9g,\"raw_shield\":%.9g,\"last_shield_before\":%.9g,\"last_shield_after\":%.9g,\"last_health_taken\":%.9g,\"last_shield_taken\":%.9g,\"original_type_health_penetration\":%.9g}",comma?",":"",i,t.sectionCount,t.shieldSection,t.rawHealth,t.shield,t.maxShield,t.rawShield,t.lastShieldBefore,t.lastShieldAfter,t.lastHealthTaken,t.lastShieldTaken,t.lastHealthPenetration);comma=true;
    }
    std::fputs("],\"section_audits\":[",f);comma=false;
    for(size_t i=0;i<sectionAudits.size();++i){const auto& audit=sectionAudits[i];if(!audit.attempted)continue;
        std::fprintf(f,"%s{\"index\":%zu,\"audit\":",comma?",":"",i);WriteSectionAudit(f,audit);std::fputc('}',f);comma=true;
    }
    std::fputs("],\"local_authority_records\":true,\"unarmored_targets_only\":false,\"armor_partition_authority\":\"local_original_cdo_data\",\"native_section_clamp_reused\":true,\"outer_server_partition_recovered\":false,\"original_server_damage_claimed\":false,\"runtime_verified\":false}\n",f);
    const bool okay=std::ferror(f)==0;std::fclose(f);return okay;
}
}
