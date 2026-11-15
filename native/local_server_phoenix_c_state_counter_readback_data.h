#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include "local_server_phoenix_c_fast_class_data.h"

namespace local_server_phoenix_c_state_counter_readback_data {
enum Known:uint32_t {Thrown=1U,Delay=2U,Handle=4U,RawClass=8U,TagFirst8=16U,Owner=32U,OwnerClass=64U,OwnerFlags=128U};
struct Raw {
    bool sampled=false,ownerMatchesAbility=false,ownerWeakMatches=false;
    uint32_t known=0,delayBits=0,ownerFlags=0;uint8_t thrown=0;
    uint64_t handle=0,tagFirst8=0;void *rawClass=nullptr,*owner=nullptr,*ownerClass=nullptr;
    uint32_t componentFlags=0;bool componentFlagsKnown=false;local_server_phoenix_c_fast_class_data::Sample fast{};
};
inline bool Has(const Raw& r,Known bit){return (r.known&static_cast<uint32_t>(bit))!=0;}
// Reader receives (base, offset, typed output). No engine function or write.
template<class Reader>inline Raw Capture(const void* component,Reader reader,uintptr_t image=0,void* world=nullptr){
    Raw out{};out.sampled=true;out.componentFlagsKnown=reader(component,8,out.componentFlags);
    if(reader(component,0x3c0,out.thrown))out.known|=Thrown;
    if(reader(component,0x368,out.delayBits))out.known|=Delay;
    if(reader(component,0x3b8,out.handle))out.known|=Handle;
    if(reader(component,0x310,out.rawClass))out.known|=RawClass;
    if(reader(component,0x318,out.tagFirst8))out.known|=TagFirst8;
    if(reader(component,0xc8,out.owner)){out.known|=Owner;
        if(out.owner){
            if(reader(out.owner,0x10,out.ownerClass))out.known|=OwnerClass;
            if(reader(out.owner,8,out.ownerFlags))out.known|=OwnerFlags;
        }}
    out.fast=local_server_phoenix_c_fast_class_data::Capture(image,component,world,reader);return out;
}
// A Fire record remains available when the following Exit updates latest.
template<class Last>inline void Publish(Last& latest,std::array<Last,3>& byKind,size_t kind,const Last& value){
    if(kind>=byKind.size())return;latest=value;byKind[kind]=value;
}
} // namespace local_server_phoenix_c_state_counter_readback_data
