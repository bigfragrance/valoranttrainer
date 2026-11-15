#pragma once
#include <Windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

// Independent fixed-storage cosmetics diagnostics. This does not add/change
// the public 24 armor_poll_performance slots. Only the bound existing GT writes
// or formats these records; the existing feature cache publishes the JSON.
// QPC reports elapsed wall duration, including preemption, not CPU causality.
namespace local_server_cosmetics_timing {
inline constexpr size_t MaxPlayers=32;
enum class Section:size_t {Poll,Leader,ReadPeer,BuildWire,WriteAtomic,ReadMetadataFile,RosterBinding,Count};
inline constexpr const char* Names[]{"poll_inclusive","leader_original_work_inclusive","read_peer_inclusive",
    "build_wire_original_work_inclusive","write_atomic_file_io_inclusive","read_metadata_file_io_inclusive","roster_binding_inclusive"};
static_assert(std::size(Names)==static_cast<size_t>(Section::Count));
struct Aggregate {uint64_t count=0,totalTicks=0,lastTicks=0,maximumTicks=0;};
struct Player {Aggregate originalWork{},originalDispatch{};};
struct Snapshot {
    uint64_t frequency=0,clockFailures=0;
    std::array<Aggregate,static_cast<size_t>(Section::Count)> sections{};
    Aggregate originalDispatch{},leaderDispatch{},unattributedDispatch{};
    std::array<Player,MaxPlayers> players{};
    std::array<Aggregate,MaxPlayers> leaderRosterEntries{}; // query ordinal, never inferred player identity
};
enum class Context:uint8_t {Unattributed,Leader,Player};
struct WindowsClock {
    static uint32_t Thread(){return GetCurrentThreadId();}
    static bool Frequency(uint64_t&out){LARGE_INTEGER value{};const bool okay=QueryPerformanceFrequency(&value)&&value.QuadPart>0;out=okay?static_cast<uint64_t>(value.QuadPart):0;return okay;}
    static bool Counter(uint64_t&out){LARGE_INTEGER value{};const bool okay=QueryPerformanceCounter(&value)&&value.QuadPart>0;out=okay?static_cast<uint64_t>(value.QuadPart):0;return okay;}
};
inline uint64_t Add(uint64_t a,uint64_t b){const auto limit=(std::numeric_limits<uint64_t>::max)();return b>limit-a?limit:a+b;}
inline void Record(Aggregate&record,uint64_t ticks){record.count=Add(record.count,1);record.totalTicks=Add(record.totalTicks,ticks);record.lastTicks=ticks;if(ticks>record.maximumTicks)record.maximumTicks=ticks;}
inline double Milliseconds(uint64_t ticks,uint64_t frequency){return frequency?static_cast<double>(ticks)*1000./static_cast<double>(frequency):0;}
template<class Clock>class Recorder {
    Snapshot snapshot_{};uint32_t owner_=0;Context context_=Context::Unattributed;size_t player_=MaxPlayers;
public:
    bool OwnerThread()const{return owner_&&owner_==Clock::Thread();}
    bool Bind(uint32_t expected){
        if(!expected||expected!=Clock::Thread()||(owner_&&owner_!=expected))return false;
        if(owner_)return true;owner_=expected;if(!Clock::Frequency(snapshot_.frequency))++snapshot_.clockFailures;return true;
    }
    uint64_t Begin(){if(!OwnerThread()||!snapshot_.frequency)return 0;uint64_t value=0;if(!Clock::Counter(value)){++snapshot_.clockFailures;return 0;}return value;}
    bool Elapsed(uint64_t start,uint64_t&elapsed){
        if(!start||!OwnerThread())return false;uint64_t now=0;
        if(!Clock::Counter(now)||now<start){++snapshot_.clockFailures;return false;}elapsed=now-start;return true;
    }
    const Snapshot& ReadOwned()const{return snapshot_;} // called only after OwnerThread guard
    class ContextScope {
        Recorder&owner_;Context previous_;size_t previousPlayer_;bool active_;
    public:
        ContextScope(Recorder&owner,Context value,size_t player=MaxPlayers):owner_(owner),previous_(Context::Unattributed),previousPlayer_(MaxPlayers),active_(owner.OwnerThread()){
            if(active_){previous_=owner_.context_;previousPlayer_=owner_.player_;owner_.context_=value;owner_.player_=player;}
        }
        ContextScope(const ContextScope&)=delete;ContextScope&operator=(const ContextScope&)=delete;
        ~ContextScope(){if(active_){owner_.context_=previous_;owner_.player_=previousPlayer_;}}
    };
    class StageScope {
        Recorder&owner_;Section section_;uint64_t started_;
    public:
        StageScope(Recorder&owner,Section section):owner_(owner),section_(section),started_(owner.Begin()){}
        StageScope(const StageScope&)=delete;StageScope&operator=(const StageScope&)=delete;
        ~StageScope(){uint64_t elapsed=0;if(owner_.Elapsed(started_,elapsed))Record(owner_.snapshot_.sections[static_cast<size_t>(section_)],elapsed);}
    };
    class PlayerScope {
        Recorder&owner_;size_t player_;ContextScope context_;uint64_t started_;
    public:
        PlayerScope(Recorder&owner,size_t player):owner_(owner),player_(player),context_(owner,Context::Player,player),started_(owner.Begin()){}
        PlayerScope(const PlayerScope&)=delete;PlayerScope&operator=(const PlayerScope&)=delete;
        ~PlayerScope(){uint64_t elapsed=0;if(player_<MaxPlayers&&owner_.Elapsed(started_,elapsed))Record(owner_.snapshot_.players[player_].originalWork,elapsed);}
    };
    class DispatchScope {
        Recorder&owner_;Context context_;size_t player_;uint64_t started_;
    public:
        explicit DispatchScope(Recorder&owner):owner_(owner),context_(Context::Unattributed),player_(MaxPlayers),started_(0){
            if(owner_.OwnerThread()){context_=owner_.context_;player_=owner_.player_;started_=owner_.Begin();}
        }
        DispatchScope(const DispatchScope&)=delete;DispatchScope&operator=(const DispatchScope&)=delete;
        ~DispatchScope(){uint64_t elapsed=0;if(!owner_.Elapsed(started_,elapsed))return;
            Record(owner_.snapshot_.originalDispatch,elapsed);
            if(context_==Context::Leader)Record(owner_.snapshot_.leaderDispatch,elapsed);
            else if(context_==Context::Player&&player_<MaxPlayers)Record(owner_.snapshot_.players[player_].originalDispatch,elapsed);
            else Record(owner_.snapshot_.unattributedDispatch,elapsed);
        }
    };
    class LeaderEntryScope {
        Recorder&owner_;size_t ordinal_;uint64_t started_;
    public:
        LeaderEntryScope(Recorder&owner,size_t ordinal):owner_(owner),ordinal_(ordinal),started_(owner.Begin()){}
        LeaderEntryScope(const LeaderEntryScope&)=delete;LeaderEntryScope&operator=(const LeaderEntryScope&)=delete;
        ~LeaderEntryScope(){uint64_t elapsed=0;if(ordinal_<MaxPlayers&&owner_.Elapsed(started_,elapsed))Record(owner_.snapshot_.leaderRosterEntries[ordinal_],elapsed);}
    };
};
using Native=Recorder<WindowsClock>;using StageScope=Native::StageScope;using ContextScope=Native::ContextScope;
using PlayerScope=Native::PlayerScope;using DispatchScope=Native::DispatchScope;
using LeaderEntryScope=Native::LeaderEntryScope;
inline Native data{};
template<class Json>inline void WriteAggregate(Json&out,const Aggregate&a,uint64_t frequency){
    const auto u=[](uint64_t value){return static_cast<unsigned long long>(value);};
    out.Add("{\"count\":%llu,\"total_ticks\":%llu,\"last_ticks\":%llu,\"maximum_ticks\":%llu,\"total_ms\":%.9g,\"last_ms\":%.9g,\"maximum_ms\":%.9g}",
        u(a.count),u(a.totalTicks),u(a.lastTicks),u(a.maximumTicks),Milliseconds(a.totalTicks,frequency),Milliseconds(a.lastTicks,frequency),Milliseconds(a.maximumTicks,frequency));
}
template<class Json,class Clock>inline void WriteJson(Json&out,const Recorder<Clock>&source){
    if(!source.OwnerThread()){out.Add("{\"sampled\":false,\"owned_gt_cache_only\":true}");return;}
    const auto&s=source.ReadOwned();
    out.Add("{\"sampled\":true,\"clock\":\"QPC\",\"frequency\":%llu,\"clock_failures\":%llu,\"elapsed_wall_time\":true,\"aggregate_since_module_load\":true,\"nested_totals_not_additive\":true,\"dispatch_scope\":\"original_engine_Event_only\",\"sections\":{",
        static_cast<unsigned long long>(s.frequency),static_cast<unsigned long long>(s.clockFailures));
    for(size_t n=0;n<s.sections.size();++n){out.Add("%s\"%s\":",n?",":"",Names[n]);WriteAggregate(out,s.sections[n],s.frequency);}
    out.Add("},\"original_dispatch\":");WriteAggregate(out,s.originalDispatch,s.frequency);
    out.Add(",\"leader_dispatch\":");WriteAggregate(out,s.leaderDispatch,s.frequency);
    out.Add(",\"unattributed_dispatch\":");WriteAggregate(out,s.unattributedDispatch,s.frequency);
    out.Add(",\"players\":[");bool comma=false;
    for(size_t n=0;n<s.players.size();++n){const auto&p=s.players[n];if(!p.originalWork.count&&!p.originalDispatch.count)continue;
        out.Add("%s{\"index\":%zu,\"original_work_inclusive\":",comma?",":"",n);WriteAggregate(out,p.originalWork,s.frequency);
        out.Add(",\"original_dispatch\":");WriteAggregate(out,p.originalDispatch,s.frequency);out.Add("}");comma=true;
    }out.Add("],\"leader_roster_entries_by_query_ordinal\":[");comma=false;
    for(size_t n=0;n<s.leaderRosterEntries.size();++n){const auto&a=s.leaderRosterEntries[n];if(!a.count)continue;
        out.Add("%s{\"ordinal\":%zu,\"original_work_inclusive\":",comma?",":"",n);WriteAggregate(out,a,s.frequency);out.Add("}");comma=true;
    }out.Add("]}");
}
template<class Json>inline void WriteJson(Json&out){WriteJson(out,data);}
} // namespace local_server_cosmetics_timing
