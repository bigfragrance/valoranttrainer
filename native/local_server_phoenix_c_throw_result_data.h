#pragma once
#include "local_server_phoenix_c_birth_data.h"
namespace local_server_phoenix_c_throw_result_data {
using Frame=local_server_phoenix_c_birth_data::Frame;using Stack=local_server_phoenix_c_birth_data::Stack;
inline bool SnapshotEntry(uintptr_t rbp,uintptr_t heap,int32_t count,int32_t capacity,size_t index,uintptr_t entry,const Stack& stack){
    namespace d=local_server_phoenix_c_birth_data;
    if(rbp<0x39||!d::InStack(stack,rbp-0x39,96)||count<=0||count>64||capacity<count||capacity>65536
        ||index>=static_cast<size_t>(count)||(!heap&&(count>4||capacity!=4)))return false;
    return d::AddEquals(heap?heap:rbp-0x39,index*20,entry);
}
inline bool Frames(const Frame& delegate,const Frame& outer,uintptr_t image,const Stack& stack,uintptr_t component,size_t index){
    namespace d=local_server_phoenix_c_birth_data;
    return component&&index<64&&outer.rip==image+0x713a622&&outer.rsi==component
        &&d::AddEquals(outer.rsp,0x130,outer.rbp)&&d::InStack(stack,outer.rsp,0x340)
        &&delegate.rip==image+0x1cf6ac9&&delegate.rdi==component&&delegate.rsi==index
        &&d::AddEquals(delegate.rsp,0xb9,delegate.rbp)&&delegate.r14==component+0x300&&delegate.r15==outer.rbp-0x30;
}
inline bool Headers(uintptr_t nativeData,int32_t nativeCount,int32_t nativeCapacity,
                    uintptr_t copyData,int32_t copyCount,int32_t copyCapacity,
                    uintptr_t paramData,int32_t paramCount,int32_t paramCapacity){
    namespace d=local_server_phoenix_c_birth_data;
    return d::ArrayShape(nativeData,nativeCount,nativeCapacity,64)&&d::ArrayShape(copyData,copyCount,copyCapacity,64)
        &&nativeCount==copyCount&&copyData==paramData&&copyCount==paramCount&&copyCapacity==paramCapacity;
}
} // namespace local_server_phoenix_c_throw_result_data
