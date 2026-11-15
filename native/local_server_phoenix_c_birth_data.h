#pragma once
#include <Windows.h>
#include <cstdint>
#include <cstddef>

namespace local_server_phoenix_c_birth_data {
struct Frame {
    uintptr_t rip=0,rsp=0,rbp=0,rbx=0,rsi=0,rdi=0,r12=0,r13=0,r14=0,r15=0;
};
struct Stack {uintptr_t low=0,high=0;};
inline bool InStack(const Stack& s,uintptr_t at,size_t bytes){
    return s.low<s.high&&at>=s.low&&at<=s.high&&bytes<=s.high-at;
}
inline bool AddEquals(uintptr_t a,uintptr_t b,uintptr_t expected){
    return a<=UINTPTR_MAX-b&&a+b==expected;
}
inline bool DirectThrowParents(const Frame& one,const Frame& all,const Frame& throwing,
    uintptr_t image,const Stack& stack){
    return one.rip==image+0x78ce68d&&all.rip==image+0x78ce23d
        &&throwing.rip==image+0x713a1a4&&AddEquals(one.rsp,0xc9,one.rbp)
        &&AddEquals(all.rsp,0xf9,all.rbp)&&AddEquals(throwing.rsp,0x130,throwing.rbp)
        &&one.rsp<all.rsp&&all.rsp<throwing.rsp&&InStack(stack,throwing.rsp,0x340);
}
inline bool FreshFrames(const Frame& native,const Frame& one,const Frame& all,const Frame& throwing,
    uintptr_t image,const Stack& stack,uintptr_t actor,uintptr_t selectedClass,uintptr_t owner){
    // This is before the instruction at the recovered PC executes. RBX is the
    // actor; RAX and native.RSP+78 are deliberately not proof inputs.
    return actor&&selectedClass&&owner&&native.rip==image+0x78cf16a&&native.rbx==actor
        &&native.r14==owner&&native.r15==owner&&native.rsp<one.rsp
        &&InStack(stack,native.rsp,0x208)&&DirectThrowParents(one,all,throwing,image,stack)
        &&one.rbx==selectedClass&&all.rbx==selectedClass;
}
inline bool PoolFrames(const Frame& reuse,const Frame& native,const Frame& one,const Frame& all,const Frame& throwing,
    uintptr_t image,const Stack& stack,uintptr_t actor,uintptr_t selectedClass,uintptr_t owner){
    return actor&&selectedClass&&owner&&reuse.rip==image+0x78cd771&&reuse.rdi==actor
        &&native.rip==image+0x78cf00d&&native.rbx==actor&&native.r14==owner&&native.r15==owner
        &&reuse.rsp<native.rsp&&native.rsp<one.rsp&&InStack(stack,native.rsp,0x208)
        &&DirectThrowParents(one,all,throwing,image,stack)&&one.rbx==selectedClass&&all.rbx==selectedClass;
}
inline bool ArrayShape(uintptr_t data,int32_t count,int32_t capacity,size_t maximum){
    return count>=0&&static_cast<size_t>(count)<=maximum&&capacity>=count&&capacity<=65536
        &&(!count||data!=0);
}
enum class Origin:uint8_t {Unknown,FreshPending,PoolPending,ThrowResult};
template<class T>inline bool SameElements(const T* left,const T* right,size_t count){
    for(size_t n=0;n<count;++n)if(left[n]!=right[n])return false;return true;
}
inline bool IncomingMatches(uintptr_t node,uintptr_t object,uintptr_t actualFunction,uintptr_t receiver,
    bool redirected,bool entryKnown,int32_t actualEntry,int32_t expectedEntry){
    return actualFunction&&receiver&&node==actualFunction&&object==receiver
        &&(!redirected||(entryKnown&&actualEntry==expectedEntry));
}
using Exec=void(*)(void*,void*,void*);
using Sample=void(*)(void*);
// Only SafeSample catches diagnostic SEH. The real original remains outside
// that exception handler and can neither be replayed nor swallowed.
inline void SafeSample(Sample sample,void* opaque,uint64_t& faults,uint32_t& exception){
    __try{sample(opaque);}
    __except((exception=GetExceptionCode(),++faults,EXCEPTION_EXECUTE_HANDLER)){}
}
inline void ForwardOnce(Exec original,void* object,void* frame,void* result,
    Sample before,Sample after,void* opaque,uint64_t& faults,uint32_t& exception){
    SafeSample(before,opaque,faults,exception);
    original(object,frame,result);
    SafeSample(after,opaque,faults,exception);
}
} // namespace local_server_phoenix_c_birth_data
