#pragma once
#include <cstdint>

// Preserve the original returned target order. The current actor may be absent
// after death; the native selector starts at index zero in that case.
namespace bot_spectator_order {
template<class Target,class Eligible>
inline int32_t Select(Target* targets,int32_t count,Target current,uint8_t direction,Eligible eligible){
    if(!targets||count<=0||count>4096||direction>1)return -1;
    int32_t start=0;
    for(int32_t i=0;i<count;++i)if(targets[i]==current){
        start=direction?(i?i-1:count-1):(i+1)%count;break;
    }
    auto index=start;
    for(int32_t visited=0;visited<count;++visited){
        if(eligible(targets[index]))return index;
        index=direction?(index?index-1:count-1):(index+1)%count;
    }
    return -1;
}
}
