#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <string>

// Independent little-endian sidecar. No Initial, human-bot bank, MMF, engine
// pointer or behavior parameter is added to this catalog/entry identity wire.
namespace local_server_bot_roster_wire {
inline constexpr uint64_t Magic=0x3130545352425456ULL; // ASCII VTBRST01
inline constexpr uint32_t Version=1,HeaderBytes=128,RowBytes=32,HeroCount=30,MaximumBots=29;
inline constexpr size_t MaximumBytes=HeaderBytes+MaximumBots*RowBytes;
inline constexpr uint64_t FnvOffset=0xcbf29ce484222325ULL,FnvPrime=0x100000001b3ULL;
struct Guid {std::array<uint32_t,4> words{};};
#pragma pack(push,4)
struct Header {
    uint64_t magic=Magic;
    uint32_t version=Version,headerBytes=HeaderBytes,rowBytes=RowBytes,bytes=0;
    uint32_t revision=0,seed=0,totalPlayers=0,botCount=0,humanHeroIndex=0,flags=0;
    Guid humanId{};std::array<uint8_t,32> catalogProfile{};
    uint64_t initialFnv=0,rowsFnv=0,reserved0=0,reserved1=0;
};
struct Row {uint32_t slot=0,heroIndex=0;uint64_t entryToken=0;Guid id{};};
#pragma pack(pop)
static_assert(sizeof(Guid)==16&&sizeof(Header)==HeaderBytes&&sizeof(Row)==RowBytes);
static_assert(offsetof(Header,version)==8&&offsetof(Header,bytes)==20&&offsetof(Header,revision)==24
    &&offsetof(Header,humanId)==48&&offsetof(Header,catalogProfile)==64&&offsetof(Header,initialFnv)==96
    &&offsetof(Header,rowsFnv)==104&&offsetof(Header,reserved0)==112&&offsetof(Header,reserved1)==120);
static_assert(offsetof(Row,entryToken)==8&&offsetof(Row,id)==16);
struct Context {
    uint32_t revision=0,seed=0,totalPlayers=0;uint64_t initialFnv=0;
    std::array<uint8_t,32> catalogProfile{};
};
struct Plan {bool valid=false;Header header{};std::array<Row,MaximumBots> rows{};};
enum class Error:uint32_t {
    None=0,Shape=9020,Header=9021,ContextMismatch=9022,Count=9023,Catalog=9024,
    RowsChecksum=9025,Slot=9026,Token=9027,DuplicateHero=9028,CommandLine=9029
};
inline uint64_t Hash(const void*data,size_t bytes,uint64_t value=FnvOffset){
    const auto p=static_cast<const uint8_t*>(data);
    for(size_t n=0;n<bytes;++n)value=(value^p[n])*FnvPrime;return value;
}
inline bool Same(const Guid&a,const Guid&b){return a.words==b.words;}
inline bool Same(const Context&a,const Context&b){return a.revision==b.revision&&a.seed==b.seed
    &&a.totalPlayers==b.totalPlayers&&a.initialFnv==b.initialFnv&&a.catalogProfile==b.catalogProfile;}
// CatalogMatch(index,Guid) is the adapter to the exact pinned catalog, never
// an external path, caller-provided UObject or approximation by display name.
template<class CatalogMatch>inline Error Parse(const void*data,size_t bytes,const Context&context,CatalogMatch match,Plan&out){
    out={};if(!data||bytes<HeaderBytes||bytes>MaximumBytes)return Error::Shape;
    Header h{};std::memcpy(&h,data,sizeof(h));
    if(h.magic!=Magic||h.version!=Version||h.headerBytes!=HeaderBytes||h.rowBytes!=RowBytes||h.bytes!=bytes
       ||h.flags||h.reserved0||h.reserved1)return Error::Header;
    if(context.totalPlayers<2||context.totalPlayers>HeroCount||h.totalPlayers!=context.totalPlayers
       ||h.botCount!=context.totalPlayers-1||h.botCount>MaximumBots||bytes!=HeaderBytes+static_cast<size_t>(h.botCount)*RowBytes)return Error::Count;
    if(h.revision!=context.revision||h.seed!=context.seed||h.initialFnv!=context.initialFnv||h.catalogProfile!=context.catalogProfile)return Error::ContextMismatch;
    if(h.humanHeroIndex>=HeroCount||!match(h.humanHeroIndex,h.humanId))return Error::Catalog;
    const auto payload=static_cast<const uint8_t*>(data)+HeaderBytes;
    if(Hash(payload,bytes-HeaderBytes)!=h.rowsFnv)return Error::RowsChecksum;
    Plan candidate{};candidate.header=h;std::array<bool,HeroCount>seen{};seen[h.humanHeroIndex]=true;
    for(uint32_t n=0;n<h.botCount;++n){auto&r=candidate.rows[n];std::memcpy(&r,payload+static_cast<size_t>(n)*RowBytes,RowBytes);
        if(r.slot!=n+1)return Error::Slot;
        if(!r.entryToken)return Error::Token;
        for(uint32_t before=0;before<n;++before)if(candidate.rows[before].entryToken==r.entryToken)return Error::Token;
        if(r.heroIndex>=HeroCount||!match(r.heroIndex,r.id))return Error::Catalog;
        if(seen[r.heroIndex])return Error::DuplicateHero;seen[r.heroIndex]=true;
    }
    candidate.valid=true;out=candidate;return Error::None;
}
inline const Row* Entry(const Plan&plan,size_t slot){
    return plan.valid&&slot>0&&slot<=plan.header.botCount&&plan.rows[slot-1].slot==slot?&plan.rows[slot-1]:nullptr;
}
inline bool Matches(const Plan&plan,const Context&context,uint32_t humanHero,const Guid&humanId){
    const auto&h=plan.header;return plan.valid&&h.revision==context.revision&&h.seed==context.seed
        &&h.totalPlayers==context.totalPlayers&&h.botCount+1==context.totalPlayers&&h.initialFnv==context.initialFnv
        &&h.catalogProfile==context.catalogProfile&&h.humanHeroIndex==humanHero&&Same(h.humanId,humanId);
}
template<size_t Capacity>inline bool Assign(const Plan&plan,const Context&context,uint32_t humanHero,const Guid&humanId,std::array<uint32_t,Capacity>&out){
    if(!Matches(plan,context,humanHero,humanId)||context.totalPlayers>Capacity)return false;
    std::array<uint32_t,Capacity>fixed{};fixed[0]=humanHero;
    for(uint32_t slot=1;slot<context.totalPlayers;++slot){const auto*r=Entry(plan,slot);if(!r)return false;fixed[slot]=r->heroIndex;}
    out=fixed;return true;
}
// Explicit launch contract: exactly one =1 flag means missing sidecar must
// fail. Its absence preserves historical fixtures; malformed/duplicate flags
// never silently become legacy. Quotes may wrap the entire option token.
inline bool RequiredArgument(const wchar_t*command,bool&required){
    required=false;if(!command)return false;unsigned count=0;
    constexpr const wchar_t* option=L"-LocalServerBotRosterRequired";
    constexpr const wchar_t* exact=L"-LocalServerBotRosterRequired=1";
    const auto length=std::wcslen(option);
    for(size_t at=0;command[at];){
        while(command[at]==L' '||command[at]==L'\t')++at;if(!command[at])break;
        std::wstring token;bool quoted=false;
        while(command[at]&&(quoted||(command[at]!=L' '&&command[at]!=L'\t'))){
            if(command[at]==L'"')quoted=!quoted;else token+=command[at];++at;
        }
        if(token.compare(0,length,option)==0&&(token.size()==length||token[length]==L'=')){
            if(quoted||token!=exact||++count>1)return false;required=true;
        }
    }return true;
}
} // namespace local_server_bot_roster_wire
