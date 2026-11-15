#pragma once
#include <cmath>
#include <cstdint>
#include <limits>
namespace local_server_phoenix_c_fire_gate_data {
struct TimerValue {bool known=false;float remaining=0;bool positive=false;};
inline TimerValue Remaining(bool originalSlotFound,uint8_t kind,double expiry,double internal){
    if(!originalSlotFound||kind==4)return {true,-1.0f,false};
    if(kind==3)return {true,0,false};
    if(!std::isfinite(expiry)||(kind==1&&!std::isfinite(internal)))return {};
    const double real=kind==1?expiry-internal:expiry;
    if(!std::isfinite(real)||real>std::numeric_limits<float>::max()||real<-std::numeric_limits<float>::max())return {};
    const float value=static_cast<float>(real);
    if(!std::isfinite(value))return {};return {true,value,value>0};
}
inline bool HandleIndex(uint64_t handle,int32_t count,uint32_t& index){
    index=static_cast<uint32_t>(handle&0xffffffU);return handle!=0&&count>0&&index<static_cast<uint32_t>(count);
}
} // namespace local_server_phoenix_c_fire_gate_data
