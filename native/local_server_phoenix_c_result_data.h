#pragma once
#include "local_server_phoenix_c_birth_data.h"
namespace local_server_phoenix_c_result_data {
using Frame=local_server_phoenix_c_birth_data::Frame;using Stack=local_server_phoenix_c_birth_data::Stack;
inline bool Frames(const Frame& getter,const Frame& wrapper,const Frame& outer,uintptr_t image,const Stack& stack,
    uintptr_t context,uintptr_t component,uintptr_t machine){
    namespace d=local_server_phoenix_c_birth_data;
    return context&&component&&machine&&getter.rip==image+0x6f77ccc&&wrapper.rip==image+0x7143df4&&outer.rip==image+0x713a23a
        &&getter.rsi==context&&getter.r15==machine&&wrapper.rsi==component&&outer.rsi==component
        &&d::AddEquals(getter.rsp,0xb9,getter.rbp)&&d::AddEquals(getter.rsp,0x120,wrapper.rsp)
        &&d::AddEquals(wrapper.rsp,0x70,outer.rsp)&&d::AddEquals(outer.rsp,0x130,outer.rbp)
        &&wrapper.rbp==outer.rbp&&d::InStack(stack,getter.rsp,0x120)&&d::InStack(stack,outer.rsp,0x340);
}
inline bool SameArray(uintptr_t beforeData,int32_t beforeCount,int32_t beforeCapacity,
    uintptr_t afterData,int32_t afterCount,int32_t afterCapacity){
    return beforeData==afterData&&beforeCount==afterCount&&beforeCapacity==afterCapacity;
}
} // namespace local_server_phoenix_c_result_data
