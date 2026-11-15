#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Presentation receipts only. The authority's original TimerManager supplies
// both window counts; no receiver clock advances or resets a combat counter.
namespace local_server_feedback_wire {
inline constexpr uint64_t Magic=0x314b424446545656ULL; // VVTFDBK1
inline constexpr uint32_t Version=1,Capacity=128;
inline constexpr uint32_t HeaderBytes=128,RecordBytes=88,WireBytes=HeaderBytes+Capacity*RecordBytes;
// Transport receipt retention only; never the native kill-window duration.
inline constexpr uint64_t MaximumRecordAgeMs=5000;
struct Record {
    uint64_t sequence=0,damageToken=0,committedAtMs=0;
    int32_t killerId=-1,victimId=-1;
    uint32_t killerEpoch=0,victimEpoch=0;
    int32_t killerOrdinal=0,victimOrdinal=0,killerWindow=0,victimWindow=0;
    int32_t killerRespawn=-1,victimRespawn=-1;
    float nativeWindowRate=-1,nativeWindowRemaining=-1;
    int32_t nativeDamageEvent=0;uint32_t reserved=0;
    uint64_t checksum=0;
};
struct Header {
    uint64_t magic=0;uint32_t version=Version,bytes=WireBytes;
    uint64_t pairDigest=0,rosterDigest=0,hostStarted=0;
    uint32_t hostPid=0,clientPid=0,rosterGeneration=0,rosterCount=0;
    volatile int32_t sequence=0;uint32_t writeIndex=0;
    uint64_t commitSequence=0,lastCommittedAt=0,reserved[6]{};
};
struct Wire {Header header;std::array<Record,Capacity> records{};};
struct Binding {
    uint64_t pairDigest=0,rosterDigest=0,hostStarted=0;
    uint32_t hostPid=0,clientPid=0,rosterGeneration=0,rosterCount=0;
};
struct Key {
    int32_t killerId=-1,victimId=-1,killerOrdinal=0,victimOrdinal=0;
    int32_t victimRespawn=-1,nativeDamageEvent=0;
    bool timestampKnown=false;
};
enum class Lookup:uint8_t {Found,Missing,Incomplete,WrongSession,Malformed,Expired};
static_assert(sizeof(Record)==RecordBytes&&sizeof(Header)==HeaderBytes&&sizeof(Wire)==WireBytes);
static_assert(offsetof(Header,sequence)==56&&offsetof(Header,commitSequence)==64);
static_assert(offsetof(Record,killerOrdinal)==40&&offsetof(Record,killerWindow)==48&&offsetof(Record,checksum)==80);
inline uint64_t Hash(const void* input,size_t bytes){
    auto p=static_cast<const uint8_t*>(input);uint64_t h=14695981039346656037ULL;
    for(size_t n=0;n<bytes;++n)h=(h^p[n])*1099511628211ULL;return h;
}
inline uint64_t Checksum(const Record& record){auto copy=record;copy.checksum=0;return Hash(&copy,sizeof(copy));}
inline bool ValidRecord(const Record& r){
    return r.sequence&&r.damageToken&&r.committedAtMs&&r.killerId>=0&&r.victimId>=0&&r.killerId!=r.victimId
        &&r.killerEpoch&&r.victimEpoch&&r.killerOrdinal>0&&r.victimOrdinal>=0
        &&r.killerWindow>0&&r.killerWindow<=r.killerOrdinal&&r.victimWindow>=0&&r.victimWindow<=r.victimOrdinal
        &&r.killerRespawn>=0&&r.victimRespawn>=0&&r.nativeDamageEvent>0&&!r.reserved
        &&std::isfinite(r.nativeWindowRate)&&std::isfinite(r.nativeWindowRemaining)
        &&r.nativeWindowRate>0&&r.nativeWindowRemaining>=0&&r.nativeWindowRemaining<=r.nativeWindowRate+.01f
        &&r.checksum==Checksum(r);
}
inline bool ValidBinding(const Binding& b){
    return b.pairDigest&&b.rosterDigest&&b.hostStarted&&b.hostPid&&b.clientPid&&b.hostPid!=b.clientPid
        &&b.rosterGeneration&&b.rosterCount>=1&&b.rosterCount<=32;
}
inline Lookup ValidateHeader(const Header& h,const Binding& b){
    if(h.sequence&1)return Lookup::Incomplete;
    if(h.magic!=Magic||h.version!=Version||h.bytes!=WireBytes||h.writeIndex>=Capacity)return Lookup::Malformed;
    if(!ValidBinding(b)||h.pairDigest!=b.pairDigest||h.rosterDigest!=b.rosterDigest||h.hostStarted!=b.hostStarted
       ||h.hostPid!=b.hostPid||h.clientPid!=b.clientPid||h.rosterGeneration!=b.rosterGeneration||h.rosterCount!=b.rosterCount)
        return Lookup::WrongSession;
    for(auto value:h.reserved)if(value)return Lookup::Malformed;
    return Lookup::Found;
}
inline bool Matches(const Record& r,const Key& k){
    return r.killerId==k.killerId&&r.victimId==k.victimId&&r.killerOrdinal==k.killerOrdinal&&r.victimOrdinal==k.victimOrdinal
        &&(!k.timestampKnown||(r.victimRespawn==k.victimRespawn&&r.nativeDamageEvent==k.nativeDamageEvent));
}
inline Lookup Find(const Wire& value,const Binding& binding,const Key& key,uint64_t now,Record& result){
    const auto header=ValidateHeader(value.header,binding);if(header!=Lookup::Found)return header;
    bool expired=false;
    for(const auto& r:value.records){
        if(!r.sequence||!Matches(r,key))continue;
        if(!ValidRecord(r)||r.sequence>value.header.commitSequence)return Lookup::Malformed;
        if(now<r.committedAtMs||now-r.committedAtMs>MaximumRecordAgeMs){expired=true;continue;}
        result=r;return Lookup::Found;
    }
    return expired?Lookup::Expired:Lookup::Missing;
}
} // namespace local_server_feedback_wire
