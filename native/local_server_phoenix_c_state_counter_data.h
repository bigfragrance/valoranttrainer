#pragma once
#include <Windows.h>
#include <cstdint>
#include <cstddef>

namespace local_server_phoenix_c_state_counter_data {
using Enter=void(*)(void*,void*,float,float);
using Exit=void(*)(void*,void*,float,uint8_t);
using Fire=void(*)(void*,float,float);
using Sample=void(*)(void*);
inline constexpr size_t TableEntries=178,TableBytes=0x590;
inline constexpr size_t EnterIndex=0x4d8/8,ExitIndex=0x4e8/8,FireIndex=0x558/8;
static_assert(TableEntries*sizeof(void*)==TableBytes);
// The prefix is copied as opaque data. It is not identified as RTTI.
template<class Pointer>inline void CopyTable(Pointer* owned,const Pointer* positive){
    owned[0]=positive[-1];for(size_t n=0;n<TableEntries;++n)owned[n+1]=positive[n];
}
inline bool RestoreAllowed(bool sameGeneration,bool active,uintptr_t current,uintptr_t owned){
    return sameGeneration&&!active&&current==owned;
}
inline void SafeSample(Sample sample,void* opaque,uint64_t& faults,uint32_t& exception){
    __try{sample(opaque);}
    __except((exception=GetExceptionCode(),++faults,EXCEPTION_EXECUTE_HANDLER)){}
}
// The original invocation is outside the diagnostic SEH handler. No retry.
inline void EnterOnce(Enter original,void* object,void* context,float first,float second,
    Sample before,Sample after,void* opaque,uint64_t& faults,uint32_t& exception){
    SafeSample(before,opaque,faults,exception);original(object,context,first,second);SafeSample(after,opaque,faults,exception);
}
inline void ExitOnce(Exit original,void* object,void* context,float first,uint8_t reason,
    Sample before,Sample after,void* opaque,uint64_t& faults,uint32_t& exception){
    SafeSample(before,opaque,faults,exception);original(object,context,first,reason);SafeSample(after,opaque,faults,exception);
}
inline void FireOnce(Fire original,void* object,float first,float second,Sample before,Sample after,void* opaque,uint64_t& faults,uint32_t& exception){
    SafeSample(before,opaque,faults,exception);original(object,first,second);SafeSample(after,opaque,faults,exception);
}
} // namespace local_server_phoenix_c_state_counter_data
