#pragma once
#include <cstdint>
#include <cstddef>
namespace local_server_phoenix_c_fast_class_data {
enum class Stage:uint8_t {NotSampled,SingletonUninitialized,NullCandidate,MetadataUnreadable,DepthUnsupported,DepthMismatch,ChainUnreadable,SlotMismatch,SlotMatch};
struct Chain {
    bool singletonRead=false,candidateRead=false,known=false,matches=false,stable=false;Stage stage=Stage::NotSampled;
    void *singleton=nullptr,*candidate=nullptr,*array=nullptr,*entry=nullptr;int32_t depth=-1,candidateDepth=-1;
};
template<class Reader>inline Chain ReadChain(const void* singletonAddress,void* candidate,bool candidateRead,Reader reader){
    Chain out{};out.candidate=candidate;out.candidateRead=candidateRead;
    if(!reader(singletonAddress,0,out.singleton)){out.stage=Stage::MetadataUnreadable;return out;}out.singletonRead=true;
    if(!out.singleton){out.stage=Stage::SingletonUninitialized;return out;}
    if(!candidateRead){out.stage=Stage::MetadataUnreadable;return out;}
    if(!candidate){out.known=true;out.stage=Stage::NullCandidate;}
    else{
        if(!reader(out.singleton,0x40,out.depth)||!reader(candidate,0x40,out.candidateDepth)){out.stage=Stage::MetadataUnreadable;return out;}
        if(out.depth<0||out.depth>1024||out.candidateDepth<0||out.candidateDepth>4096){out.stage=Stage::DepthUnsupported;return out;}
        if(out.depth>out.candidateDepth){out.known=true;out.stage=Stage::DepthMismatch;}
        else{
            if(!reader(candidate,0x38,out.array)||!out.array||!reader(out.array,static_cast<size_t>(out.depth)*8,out.entry)){out.stage=Stage::ChainUnreadable;return out;}
            out.known=true;out.matches=reinterpret_cast<uintptr_t>(out.entry)==reinterpret_cast<uintptr_t>(out.singleton)+0x38;
            out.stage=out.matches?Stage::SlotMatch:Stage::SlotMismatch;
        }
    }
    void* sameSingleton=nullptr;int32_t sameDepth=-1,sameCandidateDepth=-1;void *sameArray=nullptr,*sameEntry=nullptr;
    out.stable=reader(singletonAddress,0,sameSingleton)&&sameSingleton==out.singleton;
    if(candidate)out.stable=out.stable&&reader(out.singleton,0x40,sameDepth)&&sameDepth==out.depth
        &&reader(candidate,0x40,sameCandidateDepth)&&sameCandidateDepth==out.candidateDepth;
    if(out.stage==Stage::SlotMatch||out.stage==Stage::SlotMismatch)out.stable=out.stable&&reader(candidate,0x38,sameArray)&&sameArray==out.array
        &&reader(out.array,static_cast<size_t>(out.depth)*8,sameEntry)&&sameEntry==out.entry;
    if(!out.stable)out.known=false;return out;
}
struct Sample {
    bool ownerClassKnown=false,rawClassKnown=false,selectedClassKnown=false,worldDriverKnown=false,worldModeKnown=false;
    void *ownerClass=nullptr,*rawClass=nullptr,*selectedClass=nullptr,*world=nullptr,*worldDriver=nullptr;int32_t worldMode=-1;
    Chain ownerAres{},rawProjectile{},selectedProjectile{};
};
template<class Reader>inline Sample Capture(uintptr_t image,const void* component,void* world,Reader reader){
    Sample out{};if(!image)return out;void* owner=nullptr;uint64_t tag=0;
    if(reader(component,0xc8,owner)&&owner)out.ownerClassKnown=reader(owner,0x10,out.ownerClass);
    out.rawClassKnown=reader(component,0x310,out.rawClass);
    out.selectedClassKnown=reader(component,0x318,tag)&&tag==0&&out.rawClassKnown;if(out.selectedClassKnown)out.selectedClass=out.rawClass;
    out.ownerAres=ReadChain(reinterpret_cast<void*>(image+0xda69810),out.ownerClass,out.ownerClassKnown,reader);
    out.rawProjectile=ReadChain(reinterpret_cast<void*>(image+0xda9cc58),out.rawClass,out.rawClassKnown,reader);
    out.selectedProjectile=ReadChain(reinterpret_cast<void*>(image+0xda9cc58),out.selectedClass,out.selectedClassKnown,reader);
    out.world=world;if(world)out.worldDriverKnown=reader(world,0x40,out.worldDriver);
    if(out.worldDriverKnown&&out.worldDriver){void* check=nullptr;out.worldModeKnown=reader(world,0x40,check)&&check==out.worldDriver;if(out.worldModeKnown)out.worldMode=3;}
    return out;
}
} // namespace local_server_phoenix_c_fast_class_data
