#pragma once
#include <Windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

// GT-owned measurement only. No timer, thread, allocation, file output, engine
// calls, input/physics, or gameplay-clock writes. QPC is elapsed WALL duration,
// including scheduling/preemption; nested totals must not be added together.
namespace armor_poll_performance {
enum class Slot:size_t {HostTick,ArmorPoll,SourceGuard,Ready,FullSample,DescribeSections,JsonCall,JsonBuild,TimingExport,
    NetTickAdapter,NpcAuthorityPoll,SensorPass,PureBrainTick,ReplicationRegistry,ControlPublish,
    HostEquipmentPoll,HostCombatPollAndCache,FeatureSnapshotBuild,HostWeaponBindLoop,CombatDiagnosticRefresh,HostLoadoutMaintenance,HostMatchPoll,HostPostCombatTail,HostReadyRefresh,Count};
inline constexpr const char* Names[]{"host_tick_inclusive","armor_poll_inclusive","source_guard","equipment_ready",
    "full_native_sample_inclusive","describe_sections_inclusive","json_call_inclusive","json_build_inclusive","timing_export_inclusive",
    "net_tick_adapter_inclusive","npc_authority_poll_inclusive","single_sensor_pass_inclusive",
    "pure_brain_tick_aggregate","replication_registry_inclusive","gt_control_publish_inclusive",
    "host_equipment_poll_inclusive","host_combat_poll_and_cache_inclusive","gt_feature_snapshot_build_inclusive",
    "host_current_weapon_bind_loop_inclusive","combat_diagnostic_refresh_inclusive",
    "host_loadout_maintenance_inclusive","host_match_poll_inclusive","host_post_combat_tail_inclusive","host_ready_refresh_inclusive"};
static_assert(sizeof(Names)/sizeof(Names[0])==static_cast<size_t>(Slot::Count),"QPC slot labels mismatch");
inline constexpr std::array<uint64_t,17> UpperUs{{1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536}};
struct Aggregate {
    uint64_t started=0,completed=0,totalTicks=0,maximumTicks=0;
    std::array<uint64_t,UpperUs.size()+1> histogram{};
};
struct Snapshot {
    uint64_t frequency=0,clockFailures=0;
    std::array<Aggregate,static_cast<size_t>(Slot::Count)> slots{};
};
inline Snapshot data{};
inline DWORD ownerThread=0;
inline bool enabled=true,frequencyQueried=false;
inline bool BindGameThread(DWORD expected){
    if(!expected||expected!=GetCurrentThreadId()||(ownerThread&&ownerThread!=expected))return false;
    ownerThread=expected;return true;
}
inline uint64_t Frequency(){
    if(!frequencyQueried){frequencyQueried=true;LARGE_INTEGER frequency{};
        if(QueryPerformanceFrequency(&frequency)&&frequency.QuadPart>0)data.frequency=static_cast<uint64_t>(frequency.QuadPart);
        else ++data.clockFailures;}
    return data.frequency;
}
inline uint64_t Clock(){LARGE_INTEGER value{};if(!QueryPerformanceCounter(&value)||value.QuadPart<=0){++data.clockFailures;return 0;}
    return static_cast<uint64_t>(value.QuadPart);}
inline uint64_t Begin(Slot slot){
    if(!enabled||!ownerThread||ownerThread!=GetCurrentThreadId())return 0;
    ++data.slots[static_cast<size_t>(slot)].started;
    return Frequency()?Clock():0;
}
inline void End(Slot slot,uint64_t started){
    if(!started)return;
    const uint64_t now=Clock();if(!now)return;
    if(now<started){++data.clockFailures;return;}
    auto&record=data.slots[static_cast<size_t>(slot)];const auto elapsed=now-started;
    ++record.completed;
    record.totalTicks=elapsed>(std::numeric_limits<uint64_t>::max)()-record.totalTicks?(std::numeric_limits<uint64_t>::max)():record.totalTicks+elapsed;
    if(elapsed>record.maximumTicks)record.maximumTicks=elapsed;
    size_t bucket=0;while(bucket<UpperUs.size()
       &&static_cast<double>(elapsed)*1000000.>static_cast<double>(data.frequency)*static_cast<double>(UpperUs[bucket]))++bucket;
    ++record.histogram[bucket];
}
class Scope {
    Slot slot_;uint64_t started_;
public:
    explicit Scope(Slot slot):slot_(slot),started_(Begin(slot)){}
    Scope(const Scope&)=delete;Scope&operator=(const Scope&)=delete;
    ~Scope(){End(slot_,started_);}
};
inline Snapshot ReadSnapshot(){return data;} // same owned GT only; publish via existing cached JSON
inline double Milliseconds(uint64_t ticks,uint64_t frequency){return frequency?static_cast<double>(ticks)*1000./static_cast<double>(frequency):0.;}
} // namespace armor_poll_performance
