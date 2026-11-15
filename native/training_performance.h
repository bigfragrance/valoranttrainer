#pragma once
#include <Windows.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>

// Game-thread timing only. Fixed storage, no per-frame allocation or disk I/O.
// These observations never change native scheduling or gameplay time.
namespace training_performance {
enum class Slot : size_t { Frame, Gap, Session, Rules, Combat, Topbar, StreakHUD,
    Bots, OtherSession, Effects, UI, LocalGameplay, Input, Snapshot, Diagnostics,
    TopbarModels, TopbarPoints, TopbarAudit, SnapshotSamples, SnapshotFiles, Count };
inline constexpr const char* names[]{"trainer_tick","tick_interval","session","rules","combat","topbar","streak_hud",
    "bots","other_session","effects","ui","local_gameplay","input","snapshot","diagnostics",
    "topbar_models","topbar_points","topbar_audit","snapshot_samples","snapshot_files"};
struct Aggregate { uint64_t count=0,total=0,maximum=0; std::array<uint64_t,12> histogram{}; };
inline std::array<Aggregate, size_t(Slot::Count)> aggregates{};
inline std::array<Aggregate, size_t(Slot::Count)> windowAggregates{};
struct Slow { uint64_t at=0; Slot slot=Slot::Frame; double milliseconds=0; };
inline std::array<Slow,32> slow{};
inline uint64_t slowCount=0,previousFrame=0,lastWrite=0,windowStarted=0;
inline uint64_t Frequency(){static const uint64_t value=[](){LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return uint64_t(f.QuadPart);}();return value;}
inline uint64_t Clock(){LARGE_INTEGER n{};QueryPerformanceCounter(&n);return uint64_t(n.QuadPart);}
inline double Milliseconds(uint64_t n){return double(n)*1000.0/double(Frequency());}
inline void Record(Slot slot,uint64_t elapsed){
    const double ms=Milliseconds(elapsed);size_t bucket=0;double bound=.125;
    while(bucket+1<12&&ms>bound){++bucket;bound*=2;}
    for(auto storage:{&aggregates,&windowAggregates}){auto& a=(*storage)[size_t(slot)];
        ++a.count;a.total+=elapsed;if(elapsed>a.maximum)a.maximum=elapsed;++a.histogram[bucket];}
    if(ms>=16.0){slow[slowCount%slow.size()]={GetTickCount64(),slot,ms};++slowCount;}
}
class Scope {
    Slot slot; uint64_t since;
public:
    explicit Scope(Slot value):slot(value),since(Clock()){}
    Scope(const Scope&)=delete;Scope& operator=(const Scope&)=delete;
    ~Scope(){Record(slot,Clock()-since);}
    void Step(Slot value){const auto now=Clock();Record(slot,now-since);slot=value;since=now;}
};
inline void Write(const std::wstring& directory){
    const auto now=GetTickCount64();if(now-lastWrite<5000)return;lastWrite=now;
    FILE* file=nullptr;if(_wfopen_s(&file,(directory+L"training-performance.json").c_str(),L"wb")||!file)return;
    std::fprintf(file,"{\"schema_version\":1,\"pid\":%lu,\"at_ms\":%llu,\"clock\":\"QPC\",\"histogram_upper_bounds_ms\":[0.125,0.25,0.5,1,2,4,8,16,32,64,128,null],\"sections\":{",GetCurrentProcessId(),static_cast<unsigned long long>(now));
    for(size_t i=0;i<aggregates.size();++i){const auto& a=aggregates[i];
        std::fprintf(file,"%s\"%s\":{\"count\":%llu,\"mean_ms\":%.6f,\"maximum_ms\":%.6f,\"histogram\":[",i?",":"",names[i],static_cast<unsigned long long>(a.count),a.count?Milliseconds(a.total)/double(a.count):0,Milliseconds(a.maximum));
        for(size_t j=0;j<a.histogram.size();++j)std::fprintf(file,"%s%llu",j?",":"",static_cast<unsigned long long>(a.histogram[j]));
        std::fputs("]}",file);
    }
    std::fprintf(file,"},\"window_started_at_ms\":%llu,\"last_window_sections\":{",static_cast<unsigned long long>(windowStarted));
    for(size_t i=0;i<windowAggregates.size();++i){const auto& a=windowAggregates[i];
        std::fprintf(file,"%s\"%s\":{\"count\":%llu,\"mean_ms\":%.6f,\"maximum_ms\":%.6f,\"histogram\":[",i?",":"",names[i],static_cast<unsigned long long>(a.count),a.count?Milliseconds(a.total)/double(a.count):0,Milliseconds(a.maximum));
        for(size_t j=0;j<a.histogram.size();++j)std::fprintf(file,"%s%llu",j?",":"",static_cast<unsigned long long>(a.histogram[j]));std::fputs("]}",file);
    }
    std::fputs("},\"recent_slow_sections\":[",file);const auto count=slowCount<slow.size()?slowCount:uint64_t(slow.size());
    for(uint64_t i=0;i<count;++i){const auto& sample=slow[(slowCount-count+i)%slow.size()];std::fprintf(file,"%s{\"at_ms\":%llu,\"section\":\"%s\",\"milliseconds\":%.6f}",i?",":"",static_cast<unsigned long long>(sample.at),names[size_t(sample.slot)],sample.milliseconds);}
    std::fputs("]}\n",file);std::fclose(file);
    windowAggregates={};windowStarted=now;
}
class Frame {
    uint64_t since;const std::wstring& directory;
public:
    explicit Frame(const std::wstring& value):since(Clock()),directory(value){if(!windowStarted)windowStarted=GetTickCount64();if(previousFrame)Record(Slot::Gap,since-previousFrame);previousFrame=since;}
    ~Frame(){Record(Slot::Frame,Clock()-since);Write(directory);}
};
}
