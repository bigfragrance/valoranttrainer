#pragma once
#include <cstdint>
namespace local_server_phoenix_c_actor_gate_data {
struct Values {bool world=false,level=false;uint8_t world156=0,allow=0,suppress=0,lifecycle=0;uint32_t flags=0;int32_t levelCount=0;};
inline bool WorldGate(const Values& v){
    const bool worldReady=v.world&&v.level&&(v.world156&0x40U)&&v.levelCount!=0;
    const bool originalAllow=v.world&&v.allow!=0;
    return (worldReady||originalAllow||(v.flags&0x10U))&&v.suppress==0;
}
inline bool BegunPlay(uint8_t native11c){return (native11c&0x30U)==0x20U;}
inline bool FunctionGate(uint32_t flags,int32_t scriptCount){return (flags&0x400U)||scriptCount!=0;}
} // namespace local_server_phoenix_c_actor_gate_data
