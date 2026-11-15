#pragma once
#include "local_server_remote_timeline_probe.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <type_traits>

// Passive host observation only. Original native source/contract is retained in
// npc-common-clock/producer-audit. No Mode/component factory or lifecycle call,
// native Tick/clock advance, serial allocation, or original field write occurs.
namespace local_server_authority_clock_probe {
namespace passive=local_server_remote_timeline_probe;
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
template<class T> inline bool ReadInto(const void* p,size_t offset,T& out){return original_players::Read(p,offset,out);}
struct Array {void* data=nullptr;int32_t count=0,capacity=0;};
struct Snapshot {
    const char* stage="native_authority_clock_not_sampled";
    uint64_t atMs=0,samples=0,readExceptions=0,sourceGeneration=0,progressSamples=0;
    uint32_t profileFailureRva=0;
    bool profiles=false,schema=false,hostContext=false,strongLiveChain=false;
    bool modePresent=false,modeTyped=false,modeWorld=false,componentPresent=false,componentTyped=false;
    bool componentOwner=false,componentOuter=false,componentWorld=false,registered=false,lifecycle=false;
    bool gameStateTyped=false,managerTyped=false,managerLinked=false,scheduleKnown=false,scheduleEmpty=false;
    bool clockPresent=false,clockShape=false,clockFields=false,currentTimestampKnown=false;
    bool samePreviousSource=false,clockReplaced=false,rawRewound=false,progressObserved=false;
    void *world=nullptr,*driver=nullptr,*mode=nullptr,*component=nullptr,*owner=nullptr,*outer=nullptr;
    void *gameState=nullptr,*manager=nullptr,*componentManager=nullptr,*clock=nullptr,*clockTable=nullptr,*vbtable=nullptr;
    passive::Label modeLabel{},componentLabel{};
    Array schedule{};
    uint32_t rawIndex=0,step=0,frameStartIndex=0;
    int32_t clockOffset=0;
    uint8_t fixedRate=0;
    float accumulated=0,frameStartAccumulated=0,componentWorldRealTime=0,lastTickDelta=0,currentTimestamp=0;
    double worldRealTime=0;
    bool worldRealTimeKnown=false,frameFieldsKnown=false;
};
static_assert(std::is_trivially_copyable_v<Snapshot>);
struct Profile {uint32_t rva,bytes;uint64_t fnv;};
// BEGIN GENERATED AUTHORITY CLOCK PROFILES
inline constexpr Profile Profiles[]{
    {0x18a3a00,3,0x69bc481c23de2d4dULL},
    {0x2a64ee0,132,0x83dfe973e78a6846ULL},
    {0x684d360,174,0x53fc5dd249a985ecULL},
    {0x6851940,5,0x219ab47055f24d33ULL},
    {0x6bdf750,174,0x604a45b6176767efULL},
    {0x6bdf830,62,0x5ef46186304dc6a2ULL},
    {0x6bdf870,45,0x61b4751033fd726dULL},
    {0x6bdf8a0,237,0xd3880b1303810333ULL},
    {0x6bdfa80,5,0x1fbfbd5e2977d41ULL},
    {0x6bf23f0,203,0x942cacf252046e3aULL},
    {0x6c84210,245,0x9f26b4cec09d6a7fULL},
    {0x6c8ec60,174,0xeb09a94601ed1650ULL},
    {0x6c96c70,32,0xb32fe36f7ed26075ULL},
    {0x6c9cc20,20,0x694e8303889ab078ULL},
    {0x6c9d280,174,0x618ffa5a020f15b6ULL},
    {0x717fb90,3981,0x612c1fb484c315a3ULL},
    {0x78680e0,82,0xb1b5ebcc88febb53ULL},
    {0x7868140,485,0xda54028a4df68c29ULL},
    {0x7868330,1274,0xd2303d603c6536dULL},
    {0x7868830,81,0x6e2c1b041ac85dc7ULL},
    {0x7868890,311,0xeb45b8757b312c1dULL},
    {0x78689d0,86,0x4248fca36264a067ULL},
    {0x7891890,954,0xdf5963fe34a9b68eULL},
    {0x7891cc0,86,0x41e321f91cc6b3d5ULL},
    {0x7892ad0,452,0xccaaf896bb3ec781ULL},
    {0x789e6c0,262,0xe9b02e3ff4dd30cdULL},
    {0x795ea10,1562,0x76c8bf41519c995dULL},
    {0xc6b3e90,8,0x46ae5ed4a37c8919ULL},
    {0xc8d0c4c,4,0x4d254a7f9dcdc931ULL},
};
// END GENERATED AUTHORITY CLOCK PROFILES
inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline Snapshot cached{};
inline uintptr_t checkedImage=0,schemaImage=0;
inline uint32_t failedProfile=0;
inline bool profileMatch=false,schemaMatch=false;
inline uint64_t lastSampleAt=0,schemaTriedAt=0;
inline Snapshot GetSnapshot(){AcquireSRWLockShared(&cacheLock);const auto copy=cached;ReleaseSRWLockShared(&cacheLock);return copy;}
inline void Publish(const Snapshot& s){AcquireSRWLockExclusive(&cacheLock);cached=s;ReleaseSRWLockExclusive(&cacheLock);}
inline bool VerifyProfiles(){
    if(checkedImage==engine::image)return profileMatch;
    checkedImage=engine::image;profileMatch=false;failedProfile=0;
    if(!passive::VerifyProfiles())return false;
    for(const auto& p:Profiles){
        const auto bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes)){failedProfile=p.rva;return false;}
        uint64_t value=14695981039346656037ULL;
        for(uint32_t n=0;n<p.bytes;++n)value=(value^bytes[n])*1099511628211ULL;
        if(value!=p.fnv){failedProfile=p.rva;return false;}
    }
    profileMatch=true;return true;
}
inline bool Schema(uint64_t now){
    if(schemaImage==engine::image&&(schemaMatch||now-schemaTriedAt<1000))return schemaMatch;
    schemaImage=engine::image;schemaTriedAt=now;schemaMatch=false;
    const auto world=engine::Class(0x3122cf0),mode=engine::Class(0x6c8ec60);
    const auto timing=engine::Class(0x6bdf750),gs=engine::Class(0x6c9d280),fixed=engine::Class(0x684d360);
    schemaMatch=passive::StrongField(world,L"AuthorityGameMode",0x170)
        &&passive::StrongField(world,L"GameState",0x178)
        &&passive::StrongField(mode,L"ServerMovementTimingComponent",0x868,timing)
        &&passive::StrongField(timing,L"FixedMoveTimeStepManager",0x108,fixed)
        &&passive::StrongField(gs,L"FixedMoveTimeStepManagerComponent",0xcc0,fixed)
        &&passive::Field(fixed,L"ScheduledTimeStepChanges",0xe0,16)
        &&Read<int32_t>(timing,0x60)==0x110&&Read<int32_t>(fixed,0x60)==0xf0;
    return schemaMatch;
}
inline bool PollImpl(void* world,void* driver,uint64_t now){
    const auto previous=GetSnapshot();Snapshot s{};
    s.world=world;s.driver=driver;s.atMs=now;s.samples=previous.samples+1;s.readExceptions=previous.readExceptions;
    s.sourceGeneration=previous.sourceGeneration;s.progressSamples=previous.progressSamples;
    s.profiles=VerifyProfiles();s.profileFailureRva=failedProfile;
    if(!s.profiles){s.stage="native_authority_clock_profile_unknown";Publish(s);return true;}
    s.hostContext=world&&driver&&world==original_players::world&&original_players::Thread()
        &&Read<void*>(world,0x40)==driver&&Read<void*>(driver,0x178)==world&&!Read<void*>(driver,0xc0);
    if(!s.hostContext){s.stage="native_authority_clock_waiting_owned_host_world";Publish(s);return true;}
    s.schema=Schema(now);
    if(!s.schema){s.stage="native_authority_clock_reflected_layout_unknown";Publish(s);return true;}
    s.gameState=Read<void*>(world,0x178);s.mode=Read<void*>(world,0x170);s.modePresent=s.mode!=nullptr;
    if(s.modePresent)s.modeLabel=passive::CaptureLabel(s.mode,0);
    if(!s.modePresent){s.stage="native_authority_clock_actual_mode_absent";Publish(s);return true;}
    s.modeTyped=passive::TypedInstance(s.mode,0x6c8ec60)&&Read<uint8_t>(s.mode,0x224)==3;
    if(!s.modeTyped){s.stage="native_authority_clock_actual_mode_not_shooter_authority";Publish(s);return true;}
    s.modeWorld=Read<void*>(Read<void**>(s.mode),0x1b8)==reinterpret_cast<void*>(engine::image+0x3ed83b0)
        &&engine::function<void*(*)(void*)>(0x3ed83b0)(s.mode)==world;
    if(!s.modeWorld){s.stage="native_authority_clock_mode_world_mismatch";Publish(s);return true;}
    s.component=Read<void*>(s.mode,0x868);s.componentPresent=s.component!=nullptr;
    if(s.componentPresent)s.componentLabel=passive::CaptureLabel(s.component,1);
    if(!s.componentPresent){s.stage="native_authority_clock_original_timing_component_absent";Publish(s);return true;}
    s.componentTyped=passive::TypedInstance(s.component,0x6bdf750);
    s.owner=Read<void*>(s.component,0xc8);s.outer=Read<void*>(s.component,0x28);
    s.componentOwner=s.owner==s.mode;s.componentOuter=s.outer==s.mode;
    const auto table=Read<void**>(s.component);
    s.componentWorld=Read<void*>(table,0x1b8)==reinterpret_cast<void*>(engine::image+0x2a64ee0)
        &&engine::function<void*(*)(void*)>(0x2a64ee0)(s.component)==world;
    s.lifecycle=Read<void*>(table,0x3e0)==reinterpret_cast<void*>(engine::image+0x7868330)
        &&Read<void*>(table,0x3e8)==reinterpret_cast<void*>(engine::image+0x7868830)
        &&Read<void*>(table,0x3f8)==reinterpret_cast<void*>(engine::image+0x78680e0);
    s.registered=(Read<uint8_t>(s.component,0xa0)&1)!=0;
    if(!s.componentTyped||!s.componentOwner||!s.componentOuter||!s.componentWorld||!s.lifecycle){
        s.stage="native_authority_clock_component_identity_or_original_lifecycle_unknown";Publish(s);return true;}
    s.gameStateTyped=passive::TypedInstance(s.gameState,0x6c9d280);
    s.manager=Read<void*>(s.gameState,0xcc0);s.componentManager=Read<void*>(s.component,0x108);
    s.managerTyped=passive::TypedInstance(s.manager,0x684d360);
    s.managerLinked=s.gameStateTyped&&s.managerTyped&&s.componentManager==s.manager
        &&Read<void*>(s.manager,0xc8)==s.gameState&&Read<void*>(s.manager,0x28)==s.gameState;
    if(!s.managerLinked){s.stage="native_authority_clock_original_beginplay_manager_link_absent";Publish(s);return true;}
    s.fixedRate=Read<uint8_t>(s.manager,0xd9);s.schedule=Read<Array>(s.manager,0xe0);
    s.scheduleKnown=s.schedule.count>=0&&s.schedule.count<=128&&s.schedule.capacity>=s.schedule.count
        &&s.schedule.capacity<=4096&&(!s.schedule.count||original_players::Readable(s.schedule.data,size_t(s.schedule.count)*8));
    s.scheduleEmpty=s.scheduleKnown&&!s.schedule.count;
    s.clock=Read<void*>(s.component,0xf8);s.clockPresent=s.clock!=nullptr;
    if(!s.clockPresent){s.stage="native_authority_clock_original_common_clock_absent";Publish(s);return true;}
    s.clockTable=Read<void*>(s.clock);s.vbtable=Read<void*>(s.clock,0x10);
    s.clockShape=s.clockTable==reinterpret_cast<void*>(engine::image+0xb600a60)
        &&s.vbtable==reinterpret_cast<void*>(engine::image+0xc6b3e90)
        &&Read<int32_t>(s.vbtable)==-0x10&&Read<int32_t>(s.vbtable,4)==0x990
        &&Read<void*>(s.clockTable,0)==reinterpret_cast<void*>(engine::image+0x789e910)
        &&Read<void*>(s.clockTable,0x30)==reinterpret_cast<void*>(engine::image+0x789f0b0);
    s.clockFields=s.clockShape&&ReadInto(s.clock,0x9a8,s.rawIndex)&&ReadInto(s.clock,0x9ac,s.step)
        &&ReadInto(s.clock,0x20,s.accumulated)&&ReadInto(s.clock,0x24,s.clockOffset)
        &&std::isfinite(s.accumulated)&&s.accumulated>=0;
    s.worldRealTimeKnown=ReadInto(world,0x758,s.worldRealTime)&&std::isfinite(s.worldRealTime);
    s.frameFieldsKnown=ReadInto(s.component,0xe8,s.frameStartIndex)&&ReadInto(s.component,0xec,s.frameStartAccumulated)
        &&ReadInto(s.component,0xf0,s.componentWorldRealTime)&&ReadInto(s.component,0x100,s.lastTickDelta)
        &&std::isfinite(s.frameStartAccumulated)&&std::isfinite(s.componentWorldRealTime)&&std::isfinite(s.lastTickDelta);
    if(!s.clockFields){s.stage="native_authority_clock_actual_clock_layout_unknown";Publish(s);return true;}
    s.samePreviousSource=previous.hostContext&&previous.clockFields&&previous.world==world&&previous.mode==s.mode&&previous.component==s.component;
    s.clockReplaced=s.samePreviousSource&&previous.clock!=s.clock;
    s.rawRewound=s.samePreviousSource&&!s.clockReplaced&&s.rawIndex<previous.rawIndex;
    if(!previous.clockFields||!s.samePreviousSource||s.clockReplaced||s.rawRewound)++s.sourceGeneration;
    s.progressObserved=s.samePreviousSource&&!s.clockReplaced&&!s.rawRewound&&s.rawIndex>previous.rawIndex;
    if(s.progressObserved)++s.progressSamples;
    //78689D0 returns uint32 raw converted through uint64 to float, times1/128.
    //Do not invent an accumulator term or execute a second native timing call.
    s.currentTimestamp=static_cast<float>(s.rawIndex)*(1.0f/128.0f);s.currentTimestampKnown=true;
    s.strongLiveChain=true;s.stage="native_authority_clock_actual_original_source_cached";Publish(s);return true;
}
inline bool Poll(void* world,void* driver){
    if(GetCurrentThreadId()!=original_players::gameThread)return true;
    const auto now=GetTickCount64();if(now-lastSampleAt<100)return true;lastSampleAt=now;
    __try{return PollImpl(world,driver,now);}__except(EXCEPTION_EXECUTE_HANDLER){
        Snapshot s{};s.world=world;s.driver=driver;s.atMs=now;
        const auto previous=GetSnapshot();s.samples=previous.samples+1;s.readExceptions=previous.readExceptions+1;
        s.sourceGeneration=previous.sourceGeneration;s.progressSamples=previous.progressSamples;
        s.stage="native_authority_clock_invalid_read_observed";Publish(s);return true;
    }
}
inline void Write(FILE* file){
    if(!file)return;const auto s=GetSnapshot();const auto b=passive::Bool;
    std::fprintf(file,"{\"stage\":\"%s\",\"error\":0,\"samples\":%llu,\"sampled_at_ms\":%llu,\"read_exceptions\":%llu,\"profiles_verified\":%s,\"profile_failure_rva\":%u,\"schema_verified\":%s,\"host_context\":%s,\"strong_property_live_chain\":%s,\"world\":\"%p\",\"driver\":\"%p\",\"mode\":\"%p\",\"mode_present\":%s,\"mode_typed\":%s,\"mode_world\":%s,\"mode_label\":",
        s.stage,static_cast<unsigned long long>(s.samples),static_cast<unsigned long long>(s.atMs),static_cast<unsigned long long>(s.readExceptions),b(s.profiles),s.profileFailureRva,b(s.schema),b(s.hostContext),b(s.strongLiveChain),s.world,s.driver,s.mode,b(s.modePresent),b(s.modeTyped),b(s.modeWorld));
    passive::WriteLabel(file,s.modeLabel);
    std::fprintf(file,",\"component\":\"%p\",\"component_present\":%s,\"component_typed\":%s,\"owner\":\"%p\",\"outer\":\"%p\",\"owner_matches\":%s,\"outer_matches\":%s,\"component_world\":%s,\"registered\":%s,\"original_lifecycle\":%s,\"component_label\":",
        s.component,b(s.componentPresent),b(s.componentTyped),s.owner,s.outer,b(s.componentOwner),b(s.componentOuter),b(s.componentWorld),b(s.registered),b(s.lifecycle));
    passive::WriteLabel(file,s.componentLabel);
    std::fprintf(file,",\"game_state\":\"%p\",\"game_state_typed\":%s,\"fixed_manager\":\"%p\",\"component_manager\":\"%p\",\"manager_typed\":%s,\"original_beginplay_manager_link\":%s,\"native_rate_d9\":%u,\"schedule_known\":%s,\"schedule_empty\":%s,\"schedule_count\":%d,\"schedule_capacity\":%d,\"clock\":\"%p\",\"clock_present\":%s,\"clock_table\":\"%p\",\"vbtable\":\"%p\",\"clock_shape\":%s,\"clock_fields\":%s,\"raw_index\":%u,\"step\":%u,\"offset24\":%d,\"accumulated20\":",
        s.gameState,b(s.gameStateTyped),s.manager,s.componentManager,b(s.managerTyped),b(s.managerLinked),unsigned(s.fixedRate),b(s.scheduleKnown),b(s.scheduleEmpty),s.schedule.count,s.schedule.capacity,s.clock,b(s.clockPresent),s.clockTable,s.vbtable,b(s.clockShape),b(s.clockFields),s.rawIndex,s.step,s.clockOffset);
    passive::Number(file,s.accumulated,s.clockFields);
    std::fprintf(file,",\"source_generation\":%llu,\"same_previous_source\":%s,\"clock_replaced\":%s,\"raw_rewound\":%s,\"progress_observed\":%s,\"progress_samples\":%llu,\"current_timestamp_known\":%s,\"current_timestamp\":",
        static_cast<unsigned long long>(s.sourceGeneration),b(s.samePreviousSource),b(s.clockReplaced),b(s.rawRewound),b(s.progressObserved),static_cast<unsigned long long>(s.progressSamples),b(s.currentTimestampKnown));
    passive::Number(file,s.currentTimestamp,s.currentTimestampKnown);
    std::fprintf(file,",\"frame_fields_known\":%s,\"frame_start_raw_e8\":%u,\"frame_start_accum_ec\":",b(s.frameFieldsKnown),s.frameStartIndex);passive::Number(file,s.frameStartAccumulated,s.frameFieldsKnown);
    std::fputs(",\"component_world_realtime_f0\":",file);passive::Number(file,s.componentWorldRealTime,s.frameFieldsKnown);
    std::fputs(",\"last_tick_delta100\":",file);passive::Number(file,s.lastTickDelta,s.frameFieldsKnown);
    std::fputs(",\"world_realtime758\":",file);passive::Number(file,s.worldRealTime,s.worldRealTimeKnown);
    std::fputs(",\"clock_writes\":0,\"mode_component_factory_calls\":0,\"native_tick_calls\":0,\"native_beginplay_calls\":0,\"native_serial_allocation_calls\":0,\"netguid_creation_calls\":0,\"source_is_round_lifetime\":true}",file);
}
} // namespace local_server_authority_clock_probe
