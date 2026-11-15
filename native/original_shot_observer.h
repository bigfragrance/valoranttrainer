#pragma once
#include "original_weapon.h"
#include <array>
#include <cstdio>
#include <intrin.h>

// Passive observer of original FiringEffectComponent events. No shot generation,
// recoil/spread mutation or replacement of the original ProcessEvent call.
// Native cosmetic initial FireData initializes direction only. Pair it with the
// actual original FireShot source by source/Pawn/native index, never by camera.
// Runtime counters must establish coverage; hook installation alone is not proof.
namespace original_shots {
struct Shot {
    double observed_at=0;
    engine::Vec start{}, direction{};
    int32_t native_shot_index=0;
    uint32_t flags=0; // bit0 tracer, bit1 last, bit2 initial FireData, bit3 original FireShot origin
    int32_t ammo=-1;
    uint32_t projectile_index=0;
    uint64_t sequence=0;
};
static_assert(sizeof(Shot)==80 && offsetof(Shot,start)==8 && offsetof(Shot,direction)==32);
using Sink=void(*)(const Shot&);
using ProcessEvent=void(*)(void*,void*,void*);
struct AimSnapshot {
    bool inputRead=false,inputValid=false,controllerRead=false,cameraRead=false,facingRead=false;
    uint8_t pawnRole=255,aiControlled=0;
    void *inputState=nullptr,*controller=nullptr,*camera=nullptr;
    engine::Vec inputAim{},controllerRotation{},cameraRotation{},pawnFacingRotation{};
};
struct DirectionSample {
    uint64_t sequence=0,originSequence=0;
    int32_t nativeShot=-1;uint32_t projectile=0;
    double originalAt=0,observedAt=0;
    AimSnapshot aim;
    engine::Vec publishedDirection{},publishedAngles{},inputAngles{};
    bool publishedAnglesValid=false,inputAnglesValid=false;
    double inputToShotDegrees=-1,controllerPitchDelta=0,controllerYawDelta=0,cameraPitchDelta=0,cameraYawDelta=0;
};
struct Diagnostics {
    bool initialized=false;
    uint32_t error=0, hooks=0, pe_calls=0, shot_callbacks=0, start_callbacks=0;
    uint32_t initial_snapshots=0, invalid_payloads=0, duplicates=0, dropped_queue_events=0;
    uint32_t origin_hooks=0, origin_queries=0, native_origin_queries=0, invalid_native_origins=0;
    uint32_t invalid_result_origins=0, result_origin_matches=0, result_origin_mismatches=0;
    uint32_t origin_index_matches=0,origin_index_ambiguous=0,origin_index_missing=0;
    uint32_t native_origin_consumed=0,native_origin_used=0,native_origin_unmatched=0,native_origin_ambiguous=0;
    uint32_t native_origin_duplicate_payloads=0,native_origin_duplicate_rays=0,rejected_world_origins=0;
    uint64_t directions_observed=0;
    uint64_t rays=0;
    engine::Vec last_native_origin{},last_result_origin{};
    engine::Vec last_published_origin{};
    double native_origin_at=-1000,result_origin_at=-1000;
    int32_t last_result_shot=-1;
    int32_t last_native_shot=-1;
    uint64_t last_used_native_origin_sequence=0;
    uint64_t direction_pair_samples=0,input_aim_valid_flag_samples=0,controller_angle_samples=0,camera_angle_samples=0;
    double maximum_controller_yaw_delta=0,maximum_camera_yaw_delta=0,maximum_input_shot_delta=0;
    const char* stage="original_shot_observer_not_initialized";
};
inline Diagnostics state;
inline const Diagnostics& State(){return state;}
inline void* firing=nullptr;
inline void* ammoComponent=nullptr;
inline void* shotFunction=nullptr;
inline void* startFunction=nullptr;
inline ProcessEvent original=nullptr;
inline DWORD thread=0;
inline Sink sink=nullptr;
inline std::array<void*,162> tableStorage{}; // complete 161-function table + RTTI word
inline std::array<Shot,64> queue{};
inline uint64_t writeIndex=0,readIndex=0;
struct Seen {
    void* object=nullptr;int32_t object_index=0;bool present=false;uint64_t occurrence=0;
};
inline std::array<Seen,64> seen{};
inline uint64_t nextEffectOccurrence=0;
struct Key {
    int32_t shot=0;uint32_t projectile=0;engine::Vec start{},direction{};double at=-1000;
    uint64_t native_origin_sequence=0;
};
inline std::array<Key,64> recent{};
inline uint32_t nextKey=0;
using ViewLocation=engine::Vec*(*)(void*,engine::Vec*);
inline void* sourcePawn=nullptr;
inline ViewLocation originalViewLocation=nullptr;
// Copy the current table, including the existing original ActorTick observer.
// This per-object observer forwards GetPawnViewLocation without changing output.
inline std::array<void*,349> pawnTableStorage{};
struct OriginRecord {
    void* source=nullptr;void* pawn=nullptr;int32_t shot=-1;
    engine::Vec location{};double at=-1000;bool valid=false;
    bool consumed=false;uint64_t sequence=0;
    void* effect=nullptr;int32_t effect_object_index=0;uint64_t effect_occurrence=0;
    AimSnapshot aim;
};
inline std::array<OriginRecord,64> origins{};
inline uint64_t originWriteIndex=0;
inline std::array<DirectionSample,64> directionSamples{};
inline uint64_t directionWriteIndex=0;
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;return false;}
inline bool Readable(const void* pointer,size_t count){
    if(!pointer || !count)return false;
    MEMORY_BASIC_INFORMATION region{};
    if(!VirtualQuery(pointer,&region,sizeof(region))||region.State!=MEM_COMMIT||
       region.Protect&(PAGE_NOACCESS|PAGE_GUARD))return false;
    const uintptr_t begin=reinterpret_cast<uintptr_t>(pointer),base=reinterpret_cast<uintptr_t>(region.BaseAddress);
    return begin>=base && count<=region.RegionSize && begin-base<=region.RegionSize-count;
}
inline bool Finite(const engine::Vec& p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
inline bool WorldPoint(const engine::Vec& p){
    // Finite alone accepts misread pointer/count bytes such as 8.98e-319.
    const auto coordinate=[](double v){return std::isfinite(v)&&std::abs(v)<1.e9
        &&std::fpclassify(v)!=FP_SUBNORMAL;};
    return coordinate(p.x)&&coordinate(p.y)&&coordinate(p.z);
}
inline bool Same(const engine::Vec& a,const engine::Vec& b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
template<class T> inline bool SnapshotRead(void* object,size_t offset,T& value){
    if(!object||!Readable(static_cast<uint8_t*>(object)+offset,sizeof(T)))return false;
    std::memcpy(&value,static_cast<uint8_t*>(object)+offset,sizeof(T));return true;
}
inline AimSnapshot ReadAimSnapshot(void* pawn){
    AimSnapshot result;
    SnapshotRead(pawn,0x224,result.pawnRole);SnapshotRead(pawn,0x152c,result.aiControlled);
    SnapshotRead(pawn,0xba0,result.inputState);SnapshotRead(pawn,0x4a0,result.controller);
    result.facingRead=SnapshotRead(pawn,0x1158,result.pawnFacingRotation)&&Finite(result.pawnFacingRotation);
    uint8_t valid=0;
    result.inputRead=SnapshotRead(result.inputState,0x238,result.inputAim)&&Finite(result.inputAim);
    if(SnapshotRead(result.inputState,0x128,valid))result.inputValid=valid!=0;
    result.controllerRead=SnapshotRead(result.controller,0x4e8,result.controllerRotation)&&Finite(result.controllerRotation);
    SnapshotRead(result.controller,0x528,result.camera);
    result.cameraRead=SnapshotRead(result.camera,0x17d8,result.cameraRotation)&&Finite(result.cameraRotation);
    return result;
}
inline bool DiagnosticAngles(const engine::Vec& direction,engine::Vec& angles){
    const double lengthSquared=direction.x*direction.x+direction.y*direction.y+direction.z*direction.z;
    if(!Finite(direction)||lengthSquared<1.e-12)return false;
    constexpr double degrees=180.0/3.14159265358979323846;
    angles={std::atan2(direction.z,std::hypot(direction.x,direction.y))*degrees,
            std::atan2(direction.y,direction.x)*degrees,0};return true;
}
inline void RecordDirection(const Shot& event,const OriginRecord* origin){
    auto& sample=directionSamples[directionWriteIndex++%directionSamples.size()];sample={};
    sample.sequence=event.sequence;sample.nativeShot=event.native_shot_index;sample.projectile=event.projectile_index;
    sample.observedAt=event.observed_at;sample.publishedDirection=event.direction;
    sample.publishedAnglesValid=DiagnosticAngles(event.direction,sample.publishedAngles);
    if(!origin)return;
    sample.originSequence=origin->sequence;sample.originalAt=origin->at;sample.aim=origin->aim;
    ++state.direction_pair_samples;
    if(sample.aim.inputValid)++state.input_aim_valid_flag_samples;
    sample.inputAnglesValid=sample.aim.inputRead&&DiagnosticAngles(sample.aim.inputAim,sample.inputAngles);
    // These angles describe the already observed vectors. They never feed a
    // camera, projectile, trace, recoil/spread calculation or published Shot.
    if(sample.inputAnglesValid&&sample.publishedAnglesValid){
        const auto& a=sample.aim.inputAim;const auto& b=event.direction;
        const double norm=std::sqrt((a.x*a.x+a.y*a.y+a.z*a.z)*(b.x*b.x+b.y*b.y+b.z*b.z));
        double cosine=(a.x*b.x+a.y*b.y+a.z*b.z)/norm;
        cosine=(std::max)(-1.0,(std::min)(1.0,cosine));
        sample.inputToShotDegrees=std::acos(cosine)*(180.0/3.14159265358979323846);
        state.maximum_input_shot_delta=(std::max)(state.maximum_input_shot_delta,sample.inputToShotDegrees);
    }
    if(sample.publishedAnglesValid&&sample.aim.controllerRead){
        ++state.controller_angle_samples;
        sample.controllerPitchDelta=std::remainder(sample.publishedAngles.x-sample.aim.controllerRotation.x,360.0);
        sample.controllerYawDelta=std::remainder(sample.publishedAngles.y-sample.aim.controllerRotation.y,360.0);
        state.maximum_controller_yaw_delta=(std::max)(state.maximum_controller_yaw_delta,std::abs(sample.controllerYawDelta));
    }
    if(sample.publishedAnglesValid&&sample.aim.cameraRead){
        ++state.camera_angle_samples;
        sample.cameraPitchDelta=std::remainder(sample.publishedAngles.x-sample.aim.cameraRotation.x,360.0);
        sample.cameraYawDelta=std::remainder(sample.publishedAngles.y-sample.aim.cameraRotation.y,360.0);
        state.maximum_camera_yaw_delta=(std::max)(state.maximum_camera_yaw_delta,std::abs(sample.cameraYawDelta));
    }
}
__declspec(noinline) inline engine::Vec* ObserveViewLocation(void* pawn,engine::Vec* output){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    auto result=originalViewLocation(pawn,output);
    if(!state.initialized||pawn!=sourcePawn||GetCurrentThreadId()!=thread)return result;
    ++state.origin_queries;
    // Exact pinned original chain: GetFiringLocationAndDirection -> the original
    // firing-location helper -> FireShot. Other view queries are not shot data.
    if(caller!=engine::image+0x78840ca)return result;
    void* frames[24]{};
    const auto count=RtlCaptureStackBackTrace(0,24,frames,nullptr);
    bool originHelper=false,fireShot=false;
    for(USHORT i=0;i<count;++i){
        const auto pc=reinterpret_cast<uintptr_t>(frames[i]);
        originHelper|=pc==engine::image+0x71227dd;
        fireShot|=pc==engine::image+0x711f0ae;
    }
    if(!originHelper||!fireShot)return result;
    ++state.native_origin_queries;
    if(!Readable(result,sizeof(engine::Vec))){++state.invalid_native_origins;return result;}
    state.last_native_origin=*result;
    state.native_origin_at=static_cast<double>(GetTickCount64())/1000.;
    // Native FireShot reads +4a8 for the effect's ShotIndex at711f419 and
    // its OnShot delegate at711f62f, then increments it only at711f6cb.
    state.last_native_shot=original_weapon::Read<int32_t>(firing,0x4a8);
    const bool valid=WorldPoint(*result);
    auto& record=origins[originWriteIndex%origins.size()];record={};
    record.source=firing;record.pawn=pawn;record.shot=state.last_native_shot;record.location=*result;
    record.at=state.native_origin_at;record.valid=valid;record.sequence=++originWriteIndex;
    record.aim=ReadAimSnapshot(pawn);
    if(!valid)++state.invalid_native_origins;
    return result;
}
inline bool AttachOriginObserver(){
    sourcePawn=original_weapon::Read<void*>(firing,0x4c8);
    if(!original_weapon::IsA(sourcePawn,engine::Class(0x6c5c480)))
        return Fail(805,"original_firing_origin_pawn_invalid");
    auto current=original_weapon::Read<void**>(sourcePawn,0);
    if(!current||!Readable(current-1,sizeof(pawnTableStorage))||
       current[0x850/8]!=reinterpret_cast<void*>(engine::image+0x787f820))
        return Fail(806,"original_firing_origin_vtable_changed");
    std::memcpy(pawnTableStorage.data(),current-1,sizeof(pawnTableStorage));
    originalViewLocation=reinterpret_cast<ViewLocation>(current[0x850/8]);
    pawnTableStorage[1+0x850/8]=reinterpret_cast<void*>(ObserveViewLocation);
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(sourcePawn),pawnTableStorage.data()+1,current)!=current)
        return Fail(807,"original_firing_origin_observer_install_failed");
    ++state.origin_hooks;return true;
}
inline void Capture(void* source,int32_t shot,uint32_t flags,const uint8_t* array,
                    void* effect=nullptr,uint64_t effectOccurrence=0){
    if(!state.initialized||source!=firing||GetCurrentThreadId()!=thread||!Readable(array,16))return;
    auto data=original_weapon::Read<uint8_t*>(array,0);
    const auto count=original_weapon::Read<int32_t>(array,8),capacity=original_weapon::Read<int32_t>(array,12);
    if(count<1||count>64||capacity<count||!Readable(data,static_cast<size_t>(count)*48)){
        ++state.invalid_payloads;return;
    }
    const double now=static_cast<double>(GetTickCount64())/1000.;
    std::array<bool,64> validDirections{};
    for(int32_t i=0;i<count;++i){
        const auto direction=original_weapon::Read<engine::Vec>(data+i*48,0);
        const auto rawStart=original_weapon::Read<engine::Vec>(data+i*48,24);
        const double norm=direction.x*direction.x+direction.y*direction.y+direction.z*direction.z;
        validDirections[i]=Finite(direction)&&norm>=.9&&norm<=1.1;
        if(validDirections[i])++state.directions_observed;else ++state.invalid_payloads;
        state.last_result_origin=rawStart;state.result_origin_at=now;state.last_result_shot=shot;
        if(!WorldPoint(rawStart))++state.invalid_result_origins;
    }
    OriginRecord* correlated=nullptr;unsigned originMatches=0,consumedMatches=0;
    const bool initial=(flags&4U)!=0;
    const int32_t effectObjectIndex=effect&&Readable(effect,0x10)
        ?original_weapon::Read<int32_t>(effect,0xc):0;
    if(initial&&effectOccurrence)for(const auto& record:origins){
        if(record.consumed&&record.source==source&&record.pawn==sourcePawn&&record.shot==shot
           &&record.effect==effect&&record.effect_object_index==effectObjectIndex
           &&record.effect_occurrence==effectOccurrence){
            // A repeated snapshot of the same effect occurrence must not steal
            // a newer burst's as-yet unconsumed record with a reset ShotIndex.
            ++state.native_origin_duplicate_payloads;
            state.native_origin_duplicate_rays+=static_cast<uint32_t>(count);
            state.duplicates+=static_cast<uint32_t>(count);return;
        }
    }
    for(auto& record:origins)if(record.valid&&record.source==source&&record.pawn==sourcePawn
        &&record.shot==shot&&now-record.at>=0&&now-record.at<.25){
        if(record.consumed){++consumedMatches;continue;}
        correlated=&record;++originMatches;
    }
    if(originMatches==1)++state.origin_index_matches;
    else if(originMatches)++state.origin_index_ambiguous;
    else if(!consumedMatches)++state.origin_index_missing;
    if(initial){
        // Original effect decoding initializes only FiringDirection, even
        // though FireData has FiringResults48 storage. Its StartLocation bytes
        // must never be used, including when they happen to be zero or finite.
        if(originMatches>1){
            ++state.native_origin_ambiguous;++state.rejected_world_origins;return;
        }
        if(!originMatches){
            if(consumedMatches){++state.native_origin_duplicate_payloads;
                state.native_origin_duplicate_rays+=static_cast<uint32_t>(count);
                state.duplicates+=static_cast<uint32_t>(count);}
            else {++state.native_origin_unmatched;++state.rejected_world_origins;}
            return;
        }
        if(!effect||!effectOccurrence||!Readable(effect,0x10)){
            ++state.native_origin_unmatched;++state.rejected_world_origins;return;
        }
        // Consume exactly one original source/Pawn/native-index record for
        // the whole original pellet array. Consumed records cannot make a
        // later burst's reset ShotIndex ambiguous or swallow its new shots.
        correlated->consumed=true;correlated->effect=effect;
        correlated->effect_object_index=effectObjectIndex;correlated->effect_occurrence=effectOccurrence;
        ++state.native_origin_consumed;
        flags|=8U;
    }
    for(int32_t i=0;i<count;++i){
        if(!validDirections[i])continue;
        const auto direction=original_weapon::Read<engine::Vec>(data+i*48,0);
        const auto rawStart=original_weapon::Read<engine::Vec>(data+i*48,24);
        auto start=rawStart;
        if(originMatches==1){
            if(Same(rawStart,correlated->location))++state.result_origin_matches;
            else ++state.result_origin_mismatches;
        }
        // This is the actual original FireShot source location, captured on
        // its exact native call chain before the same native index advances.
        // Keep the original effect's direction unchanged; no camera ray or
        // replacement spread/recoil calculation is performed here.
        if(initial)start=correlated->location;
        if(!WorldPoint(start)){++state.invalid_payloads;++state.rejected_world_origins;continue;}
        const auto originSequence=originMatches==1?correlated->sequence:0;
        bool duplicate=false;
        for(const auto& k:recent){
            if(k.projectile!=static_cast<uint32_t>(i))continue;
            if(originSequence){if(k.native_origin_sequence==originSequence){duplicate=true;break;}}
            else if(now-k.at<.05&&k.shot==shot&&Same(k.start,start)&&Same(k.direction,direction)){
                duplicate=true;break;}
        }
        if(duplicate){++state.duplicates;if(initial)++state.native_origin_duplicate_rays;continue;}
        recent[nextKey++%recent.size()]={shot,static_cast<uint32_t>(i),start,direction,now,originSequence};
        // Original AmmoComponent.GetAmmo returns PredictedResourceAmount+10c;
        // +12c is its authored initial ammo property, not the live counter.
        Shot event{now,start,direction,shot,flags,original_weapon::Read<int32_t>(ammoComponent,0x10c),
                   static_cast<uint32_t>(i),++state.rays};
        state.last_published_origin=start;
        if(initial){++state.native_origin_used;state.last_used_native_origin_sequence=originSequence;}
        RecordDirection(event,originMatches==1?correlated:nullptr);
        // Keep the public observation ring even when a training consumer is
        // registered. The sink must enqueue work rather than reenter damage.
        if(writeIndex-readIndex>=queue.size()){++readIndex;++state.dropped_queue_events;}
        queue[writeIndex++%queue.size()]=event;
        if(sink)sink(event);
    }
}
inline void CaptureInitial(void* object){
    if(!Readable(object,0x180))return;
    auto source=original_weapon::Read<void*>(object,0x148);
    if(source!=firing)return;
    const int32_t index=original_weapon::Read<int32_t>(object,0x160);
    const bool last=original_weapon::Read<int32_t>(object,0x164)<=0;
    const bool tracer=original_weapon::Read<uint8_t>(object,0x168)!=0;
    uint64_t occurrence=0;const auto objectIndex=original_weapon::Read<int32_t>(object,0xc);
    for(const auto& entry:seen)if(entry.object==object&&entry.object_index==objectIndex){occurrence=entry.occurrence;break;}
    ++state.initial_snapshots;
    Capture(source,index,(tracer?1U:0U)|(last?2U:0U)|4U,static_cast<uint8_t*>(object)+0x150,object,occurrence);
}
inline void ObserveEvent(void* object,void* fn,void* parameters){
    ++state.pe_calls;
    if(fn==shotFunction && Readable(parameters,32)){
        ++state.shot_callbacks;
        const auto p=static_cast<uint8_t*>(parameters);
        const uint32_t flags=(p[12]?1U:0U)|(p[13]?2U:0U);
        Capture(original_weapon::Read<void*>(p,0),original_weapon::Read<int32_t>(p,8),flags,p+16,object);
    }
    // Always preserve original dispatch, including non-shooting events.
    original(object,fn,parameters);
    if(fn==startFunction){
        ++state.start_callbacks;
        if(Readable(object,0x180)){
            const auto index=original_weapon::Read<int32_t>(object,0xc);Seen* entry=nullptr;
            for(auto& item:seen)if(item.object==object&&item.object_index==index){entry=&item;break;}
            if(!entry)for(auto& item:seen)if(!item.object){entry=&item;break;}
            if(entry)*entry={object,index,true,++nextEffectOccurrence};
        }
        CaptureInitial(object);
    }
}
inline bool Initialize(void* originalFiring,void* originalAmmo,Sink callback=nullptr){
    if(state.initialized)return firing==originalFiring && ammoComponent==originalAmmo;
    if(!original_weapon::IsA(originalFiring,engine::Class(0x684d4c0)) ||
       !original_weapon::IsA(originalAmmo,engine::Class(0x63c7e00)))return Fail(801,"original_shot_components_invalid");
    shotFunction=engine::Find(L"/Script/ShooterGame.FiringEffectComponent.FiringStateOnShot");
    startFunction=engine::Find(L"/Script/ShooterGame.FiringEffectComponent.HandleStartEffect");
    if(!shotFunction||!startFunction)return Fail(802,"original_shot_reflection_missing");
    auto expected=reinterpret_cast<void**>(engine::image+0xb29a8a0);
    if(expected[0x298/8]!=reinterpret_cast<void*>(engine::image+0x1cdc850))
        return Fail(803,"original_firing_effect_vtable_changed");
    std::memcpy(tableStorage.data(),expected-1,sizeof(tableStorage));
    original=reinterpret_cast<ProcessEvent>(expected[0x298/8]);
    tableStorage[1+0x298/8]=reinterpret_cast<void*>(ObserveEvent);
    firing=originalFiring;ammoComponent=originalAmmo;sink=callback;thread=GetCurrentThreadId();
    if(!AttachOriginObserver())return false;
    state.initialized=true;state.stage="original_shot_observer_waiting_for_effects";return true;
}
inline void Poll(){
    if(!state.initialized||GetCurrentThreadId()!=thread)return;
    auto effects=original_weapon::Read<void**>(firing,0x500);
    const int count=original_weapon::Read<int>(firing,0x508),capacity=original_weapon::Read<int>(firing,0x50c);
    if(count<0||count>64||capacity<count||(count&&!Readable(effects,count*sizeof(void*)))){
        Fail(804,"original_active_firing_effect_array_invalid");return;
    }
    for(auto& item:seen)item.present=false;
    for(int i=0;i<count;++i){
        auto object=effects[i];if(!Readable(object,0x180))continue;
        auto expected=reinterpret_cast<void**>(engine::image+0xb29a8a0);
        auto current=*reinterpret_cast<void***>(object);
        if(current!=expected && current!=tableStorage.data()+1)continue;
        const int32_t objectIndex=original_weapon::Read<int32_t>(object,0xc);
        Seen* entry=nullptr;
        for(auto& item:seen)if(item.object==object && item.object_index==objectIndex){entry=&item;break;}
        const bool first=entry==nullptr;
        if(!entry)for(auto& item:seen)if(!item.object){entry=&item;break;}
        if(!entry)continue;
        if(first)*entry={object,objectIndex,true,++nextEffectOccurrence};else entry->present=true;
        if(current==expected){
            if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),tableStorage.data()+1,expected)!=expected)continue;
            ++state.hooks;
        }
        if(first)CaptureInitial(object);
    }
    for(auto& item:seen)if(!item.present)item={};
    if(state.hooks)state.stage=state.rays
        ?"original_shot_world_rays_observed":"original_shot_directions_without_valid_world_origins";
}
inline bool Pop(Shot& output){
    if(GetCurrentThreadId()!=thread||readIndex==writeIndex)return false;
    output=queue[readIndex++%queue.size()];return true;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    const auto vector=[&](const engine::Vec& v){
        std::fputc('[',file);const double values[]{v.x,v.y,v.z};
        for(int i=0;i<3;++i){if(i)std::fputc(',',file);
            if(std::isfinite(values[i]))std::fprintf(file,"%.17g",values[i]);else std::fputs("null",file);}
        std::fputc(']',file);
    };
    std::fprintf(file,"{\"schema_version\":1,\"stage\":\"%s\",\"initialized\":%s,\"error\":%u,"
        "\"directions_observed\":%llu,\"valid_world_rays\":%llu,\"effect_hooks\":%u,\"shot_callbacks\":%u,"
        "\"origin_hooks\":%u,\"view_queries\":%u,\"original_fire_origin_queries\":%u,\"invalid_native_origins\":%u,"
        "\"invalid_result_origins\":%u,\"result_origin_matches\":%u,\"result_origin_mismatches\":%u,"
        "\"origin_index_matches\":%u,\"origin_index_ambiguous\":%u,\"origin_index_missing\":%u,\"last_native_shot\":%d,"
        "\"native_origin_consumed\":%u,\"native_origin_used\":%u,\"native_origin_unmatched\":%u,\"native_origin_ambiguous\":%u,"
        "\"native_origin_duplicate_payloads\":%u,\"native_origin_duplicate_rays\":%u,\"rejected_world_origins\":%u,"
        "\"last_used_native_origin_sequence\":%llu,"
        "\"source_pawn\":\"%p\",\"last_native_origin_at\":%.9g,\"last_result_origin_at\":%.9g,"
        "\"last_result_shot\":%d,\"last_native_origin\":",
        state.stage,state.initialized?"true":"false",state.error,
        static_cast<unsigned long long>(state.directions_observed),static_cast<unsigned long long>(state.rays),
        state.hooks,state.shot_callbacks,state.origin_hooks,state.origin_queries,state.native_origin_queries,
        state.invalid_native_origins,state.invalid_result_origins,state.result_origin_matches,state.result_origin_mismatches,
        state.origin_index_matches,state.origin_index_ambiguous,state.origin_index_missing,state.last_native_shot,
        state.native_origin_consumed,state.native_origin_used,state.native_origin_unmatched,state.native_origin_ambiguous,
        state.native_origin_duplicate_payloads,state.native_origin_duplicate_rays,state.rejected_world_origins,
        static_cast<unsigned long long>(state.last_used_native_origin_sequence),
        sourcePawn,state.native_origin_at,state.result_origin_at,state.last_result_shot);
    vector(state.last_native_origin);std::fputs(",\"last_result_origin\":",file);vector(state.last_result_origin);
    std::fputs(",\"last_published_origin\":",file);vector(state.last_published_origin);
    std::fprintf(file,",\"direction_pair_samples\":%llu,\"input_aim_valid_flag_samples\":%llu,\"controller_angle_samples\":%llu,\"camera_angle_samples\":%llu,\"maximum_controller_yaw_delta_degrees\":%.9g,\"maximum_camera_yaw_delta_degrees\":%.9g,\"maximum_input_shot_delta_degrees\":%.9g,\"direction_samples\":[",
        static_cast<unsigned long long>(state.direction_pair_samples),static_cast<unsigned long long>(state.input_aim_valid_flag_samples),
        static_cast<unsigned long long>(state.controller_angle_samples),static_cast<unsigned long long>(state.camera_angle_samples),
        state.maximum_controller_yaw_delta,state.maximum_camera_yaw_delta,state.maximum_input_shot_delta);
    const auto first=directionWriteIndex>directionSamples.size()?directionWriteIndex-directionSamples.size():0;
    for(uint64_t i=first;i<directionWriteIndex;++i){const auto& sample=directionSamples[i%directionSamples.size()];
        if(i!=first)std::fputc(',',file);
        std::fprintf(file,"{\"sequence\":%llu,\"native_origin_sequence\":%llu,\"native_shot_index\":%d,\"projectile_index\":%u,\"original_at\":%.9g,\"observed_at\":%.9g,\"pawn_role\":%u,\"pawn_ai_controlled\":%s,\"input_read\":%s,\"input_aim_valid_flag\":%s,\"controller_read\":%s,\"camera_read\":%s,\"facing_read\":%s,\"input_to_shot_degrees\":%.9g,\"controller_pitch_delta_degrees\":%.9g,\"controller_yaw_delta_degrees\":%.9g,\"camera_pitch_delta_degrees\":%.9g,\"camera_yaw_delta_degrees\":%.9g,\"input_aim_direction\":",
            static_cast<unsigned long long>(sample.sequence),static_cast<unsigned long long>(sample.originSequence),sample.nativeShot,sample.projectile,
            sample.originalAt,sample.observedAt,static_cast<unsigned>(sample.aim.pawnRole),sample.aim.aiControlled?"true":"false",
            sample.aim.inputRead?"true":"false",sample.aim.inputValid?"true":"false",sample.aim.controllerRead?"true":"false",
            sample.aim.cameraRead?"true":"false",sample.aim.facingRead?"true":"false",sample.inputToShotDegrees,
            sample.controllerPitchDelta,sample.controllerYawDelta,sample.cameraPitchDelta,sample.cameraYawDelta);
        vector(sample.aim.inputAim);std::fputs(",\"input_direction_angles_diagnostic\":",file);vector(sample.inputAngles);
        std::fputs(",\"controller_rotation\":",file);vector(sample.aim.controllerRotation);
        std::fputs(",\"camera_rotation\":",file);vector(sample.aim.cameraRotation);
        std::fputs(",\"pawn_facing_control_rotation\":",file);vector(sample.aim.pawnFacingRotation);
        std::fputs(",\"original_firing_result_direction\":",file);vector(sample.publishedDirection);
        std::fputs(",\"firing_direction_angles_diagnostic\":",file);vector(sample.publishedAngles);std::fputc('}',file);
    }
    std::fputc(']',file);
    std::fputs(",\"result_stride\":48,\"direction_offset\":0,\"start_offset\":24,\"vector_size\":24,"
        "\"initial_origin_source\":\"original_FireShot_GetPawnViewLocation_exact_source_pawn_shot_index\","
        "\"origin_replacement_enabled\":true,\"initial_origin_native_flag\":8,\"direction_generation_changed\":false,\"angle_diagnostics_feed_gameplay\":false}\n",file);
    const bool okay=std::ferror(file)==0;std::fclose(file);return okay;
}
}
