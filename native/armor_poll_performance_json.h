#pragma once
#include "armor_poll_performance.h"
#include <cstdio>
#include <string>
#include <utility>

// Only the owned GT's existing CaptureFeatureSnapshot calls this function.
// No logging, file output, extra thread or engine/object query. Serialization
// has its own <=10 Hz cache even if an error bypasses the feature-cache gate.
namespace armor_poll_performance {
inline std::string exportedJson="{\"sampled\":false}";
inline uint64_t exportedAt=0;
inline bool haveExport=false;
inline const std::string& Json(){
    const uint64_t now=GetTickCount64();
    if(haveExport&&now>=exportedAt&&now-exportedAt<100)return exportedJson;
    Scope measurement(Slot::TimingExport);
    const auto perf=ReadSnapshot(); // this export/outer host call are not yet completed
    const auto u=[](uint64_t value){return static_cast<unsigned long long>(value);};
    char text[1024]{};std::string value;
    std::snprintf(text,sizeof(text),"{\"sampled\":true,\"sampled_at_monotonic_ms\":%llu,\"enabled\":%s,\"aggregate_since_module_load\":true,\"frequency\":%llu,\"clock_failures\":%llu,\"elapsed_wall_time\":true,\"nested_totals_not_additive\":true,\"histogram_upper_us\":[",
        u(now),enabled?"true":"false",u(perf.frequency),u(perf.clockFailures));value=text;
    for(size_t n=0;n<UpperUs.size();++n){std::snprintf(text,sizeof(text),"%s%llu",n?",":"",u(UpperUs[n]));value+=text;}
    value+=",null],\"slots\":{";
    for(size_t n=0;n<perf.slots.size();++n){const auto& a=perf.slots[n];
        std::snprintf(text,sizeof(text),"%s\"%s\":{\"started\":%llu,\"completed\":%llu,\"total_ticks\":%llu,\"maximum_ticks\":%llu,\"mean_ms\":%.9g,\"maximum_ms\":%.9g,\"histogram\":[",
            n?",":"",Names[n],u(a.started),u(a.completed),u(a.totalTicks),u(a.maximumTicks),
            a.completed?Milliseconds(a.totalTicks,perf.frequency)/static_cast<double>(a.completed):0.,Milliseconds(a.maximumTicks,perf.frequency));value+=text;
        for(size_t b=0;b<a.histogram.size();++b){std::snprintf(text,sizeof(text),"%s%llu",b?",":"",u(a.histogram[b]));value+=text;}
        value+="]}";
    }
    value+="}}";exportedJson=std::move(value);exportedAt=now;haveExport=true;return exportedJson;
}
} // namespace armor_poll_performance
