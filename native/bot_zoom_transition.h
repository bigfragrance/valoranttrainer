#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace bot_zoom_transition {
// The complete original FZoomTransition, including its original padding.
struct alignas(4) Transition{std::array<uint8_t,36> bytes{};};
static_assert(sizeof(Transition)==36&&alignof(Transition)==4);
template<class T> inline T Value(const Transition& value,size_t offset){T out{};std::memcpy(&out,value.bytes.data()+offset,sizeof(out));return out;}
inline bool Equal(const Transition& a,const Transition& b){return a.bytes==b.bytes;}
inline bool Initialized(const Transition& value){
    for(const size_t offset:{size_t(0),size_t(4),size_t(12),size_t(16)}){
        const auto fov=Value<float>(value,offset);if(!std::isfinite(fov)||fov<=0||fov>=180)return false;
    }
    const auto duration=Value<float>(value,24);
    return Value<int32_t>(value,8)>=-1&&Value<int32_t>(value,20)>=-1
        &&std::isfinite(duration)&&duration>=0&&value.bytes[28]<=1&&value.bytes[32]<=2;
}
inline bool MayPublish(const Transition& auth,const Transition& prediction,const Transition& defaults,
                       bool owned,const Transition& lastPublished,bool nativeAuthorityObserved=false){
    if(nativeAuthorityObserved||!Initialized(prediction)||Equal(auth,prediction))return false;
    // Establish a missing authority record only from the real class default.
    // Once established, another native authority writer always takes priority.
    return owned?Equal(auth,lastPublished):!Initialized(auth)&&Equal(auth,defaults);
}
}
