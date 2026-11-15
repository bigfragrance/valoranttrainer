#pragma once
#include <cstdint>
#include <cstring>

// A temporary observation command, not a combat/configuration profile.
// Roster slot and native PlayerId are deliberately separate identities.
namespace bot_spectator_contract {
#pragma pack(push,4)
struct Request {
    uint32_t enabled=0;
    uint32_t rosterIndex=0;
    int32_t nativePlayerId=0;
    uint32_t reserved=0;
};
#pragma pack(pop)
static_assert(sizeof(Request)==16,"Spectator command layout changed");
inline constexpr uint32_t CommandNumber=18;
inline constexpr uint8_t WatchPossessed=1,SpectateEveryone=4;
inline bool Valid(const Request& request) {
    if(request.reserved||request.enabled>1)return false;
    if(!request.enabled)return request.rosterIndex==0&&request.nativePlayerId==0;
    return request.rosterIndex>0&&request.rosterIndex<32
        &&request.nativePlayerId>0&&request.nativePlayerId<=4097;
}
inline bool Decode(const uint8_t* bytes,uint32_t count,Request& out) {
    out={};if(!bytes||count!=sizeof(Request))return false;
    Request value{};std::memcpy(&value,bytes,sizeof(value));
    if(!Valid(value))return false;out=value;return true;
}
}
