#pragma once
#include "local_server_feedback_wire.h"
#include <algorithm>
#include <cstdint>

namespace local_server_feedback_pending {
inline constexpr uint64_t MaximumWaitMs=250;
enum class Action:uint8_t {Wait,ForwardMapped,ForwardOriginal,DropStale,Consumed};
struct State {uint64_t queuedAt=0;bool consumed=false;};
struct Projection {int32_t bannerCount=0,bannerMaximum=5,feedKiller=0,feedVictim=0;};
inline Action Decide(const State& p,local_server_feedback_wire::Lookup lookup,uint64_t now,bool widgetAndPayloadLive,bool originalOrderReady=true){
    if(p.consumed)return Action::Consumed;
    if(!widgetAndPayloadLive)return Action::DropStale;
    if(lookup==local_server_feedback_wire::Lookup::Found&&originalOrderReady)return Action::ForwardMapped;
    if(now<p.queuedAt||now-p.queuedAt>=MaximumWaitMs)return Action::ForwardOriginal;
    return Action::Wait;
}
inline bool Consume(State& p){if(p.consumed)return false;p.consumed=true;return true;}
inline Projection Project(const local_server_feedback_wire::Record& r){
    Projection p;p.feedKiller=r.killerWindow;p.feedVictim=r.victimWindow;
    p.bannerCount=(std::min)(r.killerWindow,6);p.bannerMaximum=(std::max)(5,p.bannerCount);return p;
}
} // namespace local_server_feedback_pending
