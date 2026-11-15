#pragma once
#include "humanlike_bot_config.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

// Independent sidecar / command-17 payload. V2 Header, Initial and Configuration
// never contain this data and their existing layouts remain unchanged.
namespace human_bot_wire {
inline constexpr uint64_t Magic=0x3130544f42485456ULL; // VTHBOT01
inline constexpr uint32_t Version=1,HeaderBytes=128,MaxProfiles=32,MaxParams=4000;
inline constexpr size_t CommandCapacity=16384;
inline constexpr uint64_t FnvOffset=0xcbf29ce484222325ULL,FnvPrime=0x100000001b3ULL;
enum class Operation:uint32_t {Initial=1,Stage=2,Commit=3,Abort=4};
enum Flags:uint32_t {Enabled=1};
#pragma pack(push,4)
struct Header {
    uint64_t magic=Magic;
    uint32_t version=Version,headerBytes=HeaderBytes,bytes=HeaderBytes,operation=0;
    uint32_t revision=0,seed=0,flags=0,profileCount=0,paramCount=0,recordBytes=0;
    uint32_t partIndex=0,partCount=0,firstProfile=0,recordsInPart=0,payloadBytes=0,reserved=0;
    std::array<uint8_t,32> schemaDigest{};
    uint64_t contentChecksum=0,transactionId=0,reservedTail=0;
};
struct Record {
    uint32_t rosterIndex=0,stableBotId=0;
    uint64_t profileFingerprint=0;
    humanlike_bot_config::Profile profile{};
};
#pragma pack(pop)
static_assert(sizeof(Header)==HeaderBytes&&offsetof(Header,revision)==24&&offsetof(Header,schemaDigest)==72);
static_assert(offsetof(Header,contentChecksum)==104&&offsetof(Header,transactionId)==112&&offsetof(Header,reservedTail)==120);
static_assert(humanlike_bot_config::ParamCount>0&&humanlike_bot_config::ParamCount<=MaxParams);
static_assert(sizeof(humanlike_bot_config::Profile)==humanlike_bot_config::ParamCount*sizeof(float));
static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559);
static_assert(std::is_trivially_copyable_v<Header>&&std::is_trivially_copyable_v<Record>);
static_assert(sizeof(Record)==16+sizeof(humanlike_bot_config::Profile)&&offsetof(Record,profile)==16);
inline constexpr uint32_t RecordBytes=static_cast<uint32_t>(sizeof(Record));
inline constexpr size_t MaximumInitialBytes=HeaderBytes+MaxProfiles*sizeof(Record);
static_assert(HeaderBytes+RecordBytes<=CommandCapacity,"one full profile must fit one existing command");

inline uint64_t Hash(uint64_t value,const void* data,size_t bytes){
    const auto p=static_cast<const uint8_t*>(data);
    for(size_t n=0;n<bytes;++n)value=(value^p[n])*FnvPrime;
    return value;
}
inline uint64_t U32(uint64_t value,uint32_t word){
    const uint8_t bytes[]{uint8_t(word),uint8_t(word>>8),uint8_t(word>>16),uint8_t(word>>24)};
    return Hash(value,bytes,sizeof(bytes));
}
inline uint64_t U64(uint64_t value,uint64_t word){
    for(unsigned n=0;n<8;++n){const uint8_t byte=uint8_t(word>>(n*8));value=Hash(value,&byte,1);}
    return value;
}
inline uint64_t ContentPrefix(const Header& h){
    auto value=FnvOffset;
    for(const auto word:{h.revision,h.seed,h.flags,h.profileCount,h.paramCount,h.recordBytes})value=U32(value,word);
    value=Hash(value,h.schemaDigest.data(),h.schemaDigest.size());
    return U64(value,h.transactionId);
}
inline uint64_t Fingerprint(const Record& record){
    auto value=U32(FnvOffset,record.rosterIndex);value=U32(value,record.stableBotId);
    // This wire is little-endian IEEE754 binary32. The owned native target and
    // Python struct pack are both Windows x64; no pointer/padding bytes travel.
    return Hash(value,record.profile.values.data(),sizeof(record.profile));
}
inline bool SameBundle(const Header& a,const Header& b){
    return a.revision==b.revision&&a.seed==b.seed&&a.flags==b.flags&&a.profileCount==b.profileCount
        &&a.paramCount==b.paramCount&&a.recordBytes==b.recordBytes&&a.partCount==b.partCount
        &&a.schemaDigest==b.schemaDigest&&a.contentChecksum==b.contentChecksum&&a.transactionId==b.transactionId;
}
inline bool HeaderValid(const Header& h){
    return h.magic==Magic&&h.version==Version&&h.headerBytes==HeaderBytes&&h.reserved==0&&h.reservedTail==0
        &&h.profileCount>0&&h.profileCount<=MaxProfiles&&h.paramCount==humanlike_bot_config::ParamCount
        &&h.recordBytes==RecordBytes&&h.flags<=Enabled&&h.transactionId!=0&&h.seed!=0&&h.revision<=0x7fffffffU
        &&h.schemaDigest==humanlike_bot_config::SchemaDigest&&h.partCount>0&&h.partCount<=MaxProfiles
        &&h.payloadBytes<=MaxProfiles*RecordBytes&&h.bytes==HeaderBytes+h.payloadBytes;
}
inline bool Shape(const uint8_t* data,uint32_t count,bool initial,Header& h){
    if(!data||count<HeaderBytes||count>(initial?MaximumInitialBytes:CommandCapacity))return false;
    std::memcpy(&h,data,sizeof(h));if(!HeaderValid(h)||h.bytes!=count)return false;
    const auto operation=static_cast<Operation>(h.operation);
    if(initial)return operation==Operation::Initial&&h.partIndex==0&&h.partCount==1&&h.firstProfile==0
        &&h.recordsInPart==h.profileCount&&h.payloadBytes==h.profileCount*RecordBytes;
    if(operation==Operation::Stage)return h.partIndex<h.partCount&&h.firstProfile<h.profileCount
        &&h.recordsInPart>0&&h.recordsInPart<=h.profileCount-h.firstProfile
        &&h.payloadBytes==h.recordsInPart*RecordBytes;
    if(operation==Operation::Commit)return h.partIndex==h.partCount&&h.firstProfile==h.profileCount
        &&h.recordsInPart==0&&h.payloadBytes==0;
    if(operation==Operation::Abort)return h.partIndex==0&&h.firstProfile==0&&h.recordsInPart==0&&h.payloadBytes==0;
    return false;
}
inline bool ValidCommand(const uint8_t* data,uint32_t bytes){Header h;return Shape(data,bytes,false,h);}
} // namespace human_bot_wire
