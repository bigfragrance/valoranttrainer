#pragma once
#include "original_players.h"
#include "original_scope.h"
#include <array>

namespace training_world_geometry {
inline void* world=nullptr;
inline unsigned capsuleQueries=0,groundQueries=0,blocked=0;
inline bool capsuleAbi=false;
inline const char* stage="training_geometry_not_initialized";
struct Array {void** data=nullptr;int32_t count=0,capacity=0;};
inline Array Ignore(std::array<void*,128>& storage){
    int count=0;
    const auto add=[&](void* actor){if(actor&&count<static_cast<int>(storage.size()))storage[static_cast<size_t>(count++)]=actor;};
    add(original_players::fallbackPawn);
    for(const auto& entry:original_players::players)if(entry.ready){add(entry.character);add(original_players::Value<void*>(entry.inventory,0x280));}
    return {storage.data(),count,count};
}
inline bool Initialize(void* value){world=value;return world&&original_players::Thread();}
inline bool Ground(const engine::Vec& nearby,engine::Vec& feet,double above=120,double below=400,bool* queryKnown=nullptr){
    if(queryKnown)*queryKnown=false;
    if(!world||!original_players::Thread()||!original_players::Finite(nearby))return false;
    const engine::Vec start{nearby.x,nearby.y,nearby.z+above},end{nearby.x,nearby.y,nearby.z-below};
    alignas(16) uint8_t parameters[0x178]{};
    original_players::Put(parameters,0,world);original_players::Put(parameters,8,start);original_players::Put(parameters,0x20,end);
    parameters[0x39]=1;parameters[0x148]=1;
    std::array<void*,128> ignored{};const auto array=Ignore(ignored);original_players::Put(parameters,0x40,array);
    auto fn=engine::Find(L"/Script/Engine.KismetSystemLibrary.LineTraceSingle");
    if(original_players::Value<uint16_t>(fn,0xbe)!=0x171)return false;
    ++groundQueries;
    parameters[0x170]=255;
    if(!engine::Event(engine::DefaultObject(engine::Class(0x45aa110)),L"/Script/Engine.KismetSystemLibrary.LineTraceSingle",parameters)
       ||parameters[0x170]>1)return false;
    if(!parameters[0x170]){if(queryKnown)*queryKnown=true;return false;}
    if(!(parameters[0x105]&1))return false;
    const auto distance=original_players::Value<float>(parameters,0x60);
    if(!std::isfinite(distance)||distance<0||distance>above+below)return false;
    feet={nearby.x,nearby.y,start.z-distance};const bool valid=original_players::Finite(feet);if(queryKnown)*queryKnown=valid;return valid;
}
inline bool CapsuleProfile(){
    if(capsuleAbi)return true;
    auto fn=engine::Find(L"/Script/Engine.KismetSystemLibrary.CapsuleTraceSingle");
    if(!fn||original_players::Value<uint16_t>(fn,0xbe)!=0x179)return false;
    struct Expected {const wchar_t* name;int offset;};
    constexpr Expected fields[]{{L"WorldContextObject",0},{L"Start",8},{L"End",32},{L"Radius",56},{L"HalfHeight",60},
        {L"TraceChannel",64},{L"bTraceComplex",65},{L"ActorsToIgnore",72},{L"DrawDebugType",88},{L"OutHit",96},{L"bIgnoreSelf",336},{L"ReturnValue",376}};
    for(const auto& expected:fields)if(original_scope::FieldOffset(fn,expected.name)!=expected.offset)return false;
    capsuleAbi=true;return true;
}
inline bool CapsuleClear(const engine::Vec& startFeet,const engine::Vec& endFeet,double radius,double half){
    if(!CapsuleProfile()||!original_players::Finite(startFeet)||!original_players::Finite(endFeet)
        ||!std::isfinite(radius)||!std::isfinite(half)||radius<=0||half<radius)return false;
    auto start=startFeet,end=endFeet;start.z+=half+2;end.z+=half+2;
    if(start.x==end.x&&start.y==end.y&&start.z==end.z)end.z+=.1;
    alignas(16) uint8_t parameters[0x180]{};
    original_players::Put(parameters,0,world);original_players::Put(parameters,8,start);original_players::Put(parameters,32,end);
    original_players::Put(parameters,56,static_cast<float>(radius));original_players::Put(parameters,60,static_cast<float>(half));
    parameters[65]=1;parameters[336]=1;
    std::array<void*,128> ignored{};const auto array=Ignore(ignored);original_players::Put(parameters,72,array);
    ++capsuleQueries;
    if(!engine::Event(engine::DefaultObject(engine::Class(0x45aa110)),L"/Script/Engine.KismetSystemLibrary.CapsuleTraceSingle",parameters))return false;
    if(parameters[376]){++blocked;return false;}
    return true;
}
inline bool NativeStepHeight(size_t index,double& step){
    if(!original_players::Thread()||index>=original_players::MaxPlayers)return false;
    const auto movement=original_players::players[index].movement;
    const auto type=original_players::Value<void*>(movement,0x10);
    const int offset=original_scope::FieldOffset(type,L"MaxStepHeight");
    if(offset<0)return false;
    step=original_players::Value<float>(movement,static_cast<size_t>(offset));
    return std::isfinite(step)&&step>0&&step<=100;
}
inline bool ValidateGroundSegment(size_t index,const engine::Vec& from,const engine::Vec& to,double radius,bool* queryKnown=nullptr){
    if(queryKnown)*queryKnown=false;
    double step=0;
    if(!original_players::Finite(from)||!original_players::Finite(to)||!std::isfinite(radius)||radius<=0
       ||!NativeStepHeight(index,step))return false;
    const double dx=to.x-from.x,dy=to.y-from.y,length=std::sqrt(dx*dx+dy*dy);
    const int samples=(std::min)(128,(std::max)(1,static_cast<int>(std::ceil(length/(std::max)(10.0,radius)))));
    engine::Vec previous=from;
    for(int n=0;n<=samples;++n){const double t=static_cast<double>(n)/samples;
        engine::Vec expected{from.x+(to.x-from.x)*t,from.y+(to.y-from.y)*t,from.z+(to.z-from.z)*t},floor{};
        bool known=false;
        const bool supported=Ground(expected,floor,step+10,step+10,&known);
        if(!known)return false;
        if(!supported||std::abs(floor.z-expected.z)>step||std::abs(floor.z-previous.z)>step){
            if(queryKnown)*queryKnown=true;return false;
        }
        previous=floor;
    }
    if(queryKnown)*queryKnown=true;return true;
}
inline bool ValidateSegment(size_t index,const engine::Vec& from,const engine::Vec& to,double radius,double half){
    if(index>=original_players::MaxPlayers||!CapsuleClear(from,to,radius,half)
       ||!ValidateGroundSegment(index,from,to,radius))return false;
    stage="training_geometry_original_capsule_and_supporting_ground_verified";return true;
}

}
