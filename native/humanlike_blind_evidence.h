#pragma once
#include <cstdint>

// Own-state evidence only. No opponent position/identity or rendered opacity.
namespace humanlike_blind_evidence {
struct Weak {int32_t index=-1,serial=0;};
struct Snapshot {
    bool valid=false,profiles=false,metadata=false,worldKnown=false;
    uint32_t index=0,epoch=0;uintptr_t pawn=0,manager=0,world=0,function=0;
    Weak pawnId{},managerId{},worldId{},functionId{};
    uint8_t role=0;int32_t activeCount=-1,capacity=-1;
};
enum class Reason:uint8_t {Known,UnknownSource,GenerationChanged,NativeQueryUnavailable,InvalidNativeValue,StateChanged};
struct Reading {bool known=false,active=false;Reason reason=Reason::UnknownSource;};
inline bool Same(const Weak&a,const Weak&b){return a.index==b.index&&a.serial==b.serial;}
inline bool Valid(const Snapshot&s){
    // Capacity is an allocation/overflow bound, not a modeled behavior slider.
    return s.valid&&s.profiles&&s.metadata&&s.worldKnown&&s.index>0&&s.index<32&&s.epoch&&s.pawn&&s.manager&&s.world&&s.function
        &&s.pawnId.index>=0&&s.pawnId.serial>0&&s.managerId.index>=0&&s.managerId.serial>0
        &&s.worldId.index>=0&&s.worldId.serial>0&&s.functionId.index>=0&&s.functionId.serial>0
        &&s.role<=3&&s.activeCount>=0&&s.capacity>=s.activeCount&&s.capacity<=4096;
}
inline Reading Evaluate(const Snapshot&before,const Snapshot&after,bool completed,uint8_t nativeValue){
    if(!Valid(before)||!Valid(after))return {};
    if(before.index!=after.index||before.epoch!=after.epoch||before.pawn!=after.pawn||before.manager!=after.manager
       ||before.world!=after.world||before.function!=after.function||before.role!=after.role
       ||!Same(before.pawnId,after.pawnId)||!Same(before.managerId,after.managerId)
       ||!Same(before.worldId,after.worldId)||!Same(before.functionId,after.functionId))
        return {false,false,Reason::GenerationChanged};
    if(!completed)return {false,false,Reason::NativeQueryUnavailable};
    if(nativeValue>1)return {false,false,Reason::InvalidNativeValue};
    if(before.activeCount!=after.activeCount||before.capacity!=after.capacity||bool(nativeValue)!=(after.activeCount>0))
        return {false,false,Reason::StateChanged};
    return {true,nativeValue!=0,Reason::Known};
}
} // namespace humanlike_blind_evidence
