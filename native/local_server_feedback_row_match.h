#pragma once
#include "local_server_feedback_wire.h"
#include <array>
#include <cstdint>
#include <initializer_list>

// Strict association of rows which the ORIGINAL pending callback really
// removed/rendered. No timer, sorting decision or UI mutation exists here.
namespace local_server_feedback_row_match {
inline constexpr size_t Capacity=64;
struct Object {uintptr_t address=0;int32_t index=-1,serial=0;};
struct Source {
    uintptr_t owner=0,left=0,right=0,response=0,weapon=0;
    int32_t killerOrdinal=0,victimOrdinal=0;uint32_t expiryBits=0;
    int32_t leftIndex=-1,leftSerial=0,rightIndex=-1,rightSerial=0;
    uint8_t ally=0,temporary=0,recovery=0;
    local_server_feedback_wire::Key key{};
};
struct Snapshot {
    size_t sources=0,children=0;std::array<Source,Capacity> pending{};
    std::array<Object,Capacity> rows{};
};
enum class Status:uint8_t {Mapped,NoChange,BadBounds,DuplicateSource,NewOrChangedPending,ChangedChildren,DuplicateChild,CountMismatch};
struct Pair {size_t source=0,child=0;};
struct Plan {Status status=Status::BadBounds;size_t count=0;std::array<Pair,Capacity> pairs{};};
inline bool Same(const Object& a,const Object& b){return a.address==b.address&&a.index==b.index&&a.serial==b.serial;}
inline bool Valid(const Object& a){return a.address&&a.index>=0&&a.serial>0;}
inline bool SameKey(const local_server_feedback_wire::Key& a,const local_server_feedback_wire::Key& b){
    return a.killerId==b.killerId&&a.victimId==b.victimId&&a.killerOrdinal==b.killerOrdinal&&a.victimOrdinal==b.victimOrdinal;
}
inline bool Same(const Source& a,const Source& b){
    return a.owner==b.owner&&a.left==b.left&&a.right==b.right&&a.response==b.response&&a.weapon==b.weapon
        &&a.killerOrdinal==b.killerOrdinal&&a.victimOrdinal==b.victimOrdinal&&a.expiryBits==b.expiryBits
        &&a.leftIndex==b.leftIndex&&a.leftSerial==b.leftSerial&&a.rightIndex==b.rightIndex&&a.rightSerial==b.rightSerial
        &&a.ally==b.ally&&a.temporary==b.temporary&&a.recovery==b.recovery&&SameKey(a.key,b.key);
}
inline Plan Associate(const Snapshot& before,const Snapshot& after){
    Plan plan{};
    if(before.sources>Capacity||after.sources>Capacity||before.children>Capacity||after.children>Capacity)return plan;
    for(const auto* snapshot:{&before,&after}){
        for(size_t n=0;n<snapshot->sources;++n)for(size_t k=0;k<n;++k)
            if(SameKey(snapshot->pending[n].key,snapshot->pending[k].key)){plan.status=Status::DuplicateSource;return plan;}
        for(size_t n=0;n<snapshot->children;++n){
            if(!Valid(snapshot->rows[n])){plan.status=Status::ChangedChildren;return plan;}
            for(size_t k=0;k<n;++k)if(Same(snapshot->rows[n],snapshot->rows[k])){plan.status=Status::DuplicateChild;return plan;}
        }
    }
    // The real Tick's removal loop preserves retained items. An added/changed
    // item or a nested callback makes this association ambiguous and is refused.
    size_t retained=0;
    for(size_t n=0;n<before.sources;++n){
        if(retained<after.sources&&Same(before.pending[n],after.pending[retained]))++retained;
        else plan.pairs[plan.count++].source=n;
    }
    if(retained!=after.sources){plan.status=Status::NewOrChangedPending;return plan;}
    if(after.children<before.children){plan.status=Status::ChangedChildren;return plan;}
    for(size_t n=0;n<before.children;++n)if(!Same(before.rows[n],after.rows[n])){plan.status=Status::ChangedChildren;return plan;}
    if(after.children-before.children!=plan.count){plan.status=Status::CountMismatch;return plan;}
    for(size_t n=0;n<plan.count;++n)plan.pairs[n].child=before.children+n;
    plan.status=plan.count?Status::Mapped:Status::NoChange;return plan;
}
} // namespace local_server_feedback_row_match
