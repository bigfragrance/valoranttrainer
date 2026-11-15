#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace local_server_hero_skill_readback {
inline constexpr size_t MaxMachines=16,HistoryCapacity=16;
struct Object {
    uintptr_t pointer=0,type=0;int32_t index=-1,serial=0;
    bool weakKnown=false,nameKnown=false,classNameKnown=false;
    char name[96]{},className[96]{};
};
struct Machine {
    Object object{},current{};bool valid=false,enabled=false,currentKnown=false,links=false;
    bool subframeKnown=false,pendingKnown=false,pendingFirstKnown=false;
    float subframe=0;int32_t pendingCount=-1,pendingCapacity=-1;uint64_t pendingFirstRaw=0;
};
struct Resource {
    Object object{};uint8_t kind=0;
    bool currentKnown=false,authorityKnown=false,configuredMaximumKnown=false;
    bool signatureKnown=false,ultimatePointsKnown=false;
    int32_t current=0,authority=0,configuredMaximum=0,signatureProgress=0,signaturePerUse=0,ultimatePoints=0;
};
struct Sample {
    bool valid=false,host=false,selectedAuthored=false,inputKnown=false,heldKnown=false;
    bool machineOverflow=false,resourceAmbiguous=false;uint8_t role=255,primaryHeld=255,slot=255;
    uint32_t epoch=0,hero=0;uint64_t frameworkRegistration=0,atMs=0,serial=0;
    Object pawn{},playerState{},ability{},inventory{},input{};
    int32_t acceptedInputIndex=-1;size_t machineCount=0,observedMachineCount=0;
    std::array<Machine,MaxMachines> machines{};Resource resource{};
    const char* stage="hero_skill_not_sampled";
};
struct History {
    std::array<Sample,HistoryCapacity> rows{};size_t count=0,next=0;uint64_t changes=0;
};
inline void Invalidate(Sample& value,const char* stage,uint64_t atMs,bool host,uint32_t epoch){
    value={};value.stage=stage;value.atMs=atMs;value.host=host;value.epoch=epoch;
}
inline bool QueueShape(uintptr_t data,int32_t count,int32_t capacity){
    return count>=0&&capacity>=count&&capacity<=4096&&(!count||data!=0);
}
inline bool SameLife(const Sample& a,const Sample& b){
    return a.epoch&&a.epoch==b.epoch&&a.frameworkRegistration==b.frameworkRegistration
        &&a.pawn.weakKnown&&b.pawn.weakKnown&&a.pawn.pointer==b.pawn.pointer
        &&a.pawn.index==b.pawn.index&&a.pawn.serial==b.pawn.serial;
}
inline bool Changed(const Sample& a,const Sample& b){
    if(a.valid!=b.valid||!SameLife(a,b)||a.ability.pointer!=b.ability.pointer||a.selectedAuthored!=b.selectedAuthored
       ||a.inputKnown!=b.inputKnown||a.heldKnown!=b.heldKnown||a.primaryHeld!=b.primaryHeld||a.machineCount!=b.machineCount)return true;
    for(size_t n=0;n<a.machineCount&&n<MaxMachines;++n){const auto& x=a.machines[n];const auto& y=b.machines[n];
        if(x.valid!=y.valid||x.object.pointer!=y.object.pointer||x.currentKnown!=y.currentKnown||x.current.pointer!=y.current.pointer
           ||x.enabled!=y.enabled||x.pendingKnown!=y.pendingKnown||x.pendingCount!=y.pendingCount)return true;}
    const auto& x=a.resource;const auto& y=b.resource;
    return x.object.pointer!=y.object.pointer||x.currentKnown!=y.currentKnown||x.current!=y.current
        ||x.authorityKnown!=y.authorityKnown||x.authority!=y.authority||x.ultimatePointsKnown!=y.ultimatePointsKnown
        ||x.ultimatePoints!=y.ultimatePoints||x.signatureKnown!=y.signatureKnown||x.signatureProgress!=y.signatureProgress;
}
inline void Remember(History& history,const Sample& current){
    if(history.count&&!Changed(history.rows[(history.next+HistoryCapacity-1)%HistoryCapacity],current))return;
    history.rows[history.next]=current;history.next=(history.next+1)%HistoryCapacity;
    if(history.count<HistoryCapacity)++history.count;++history.changes;
}
inline bool RetireSample(Sample& value,uintptr_t pawn,uint32_t epoch){
    if(value.pawn.pointer!=pawn||value.epoch!=epoch)return false;
    value.valid=false;value.stage="hero_skill_exact_life_retired";return true;
}
inline void Add(std::string& out,const char* text){out+=text;}
template<class... A> inline void Print(std::string& out,const char* format,A... args){
    char buffer[768]{};const int size=std::snprintf(buffer,sizeof(buffer),format,args...);
    if(size>0&&static_cast<size_t>(size)<sizeof(buffer))out.append(buffer,static_cast<size_t>(size));
}
inline const char* Bool(bool b){return b?"true":"false";}
inline void String(std::string& out,const char* text){
    out+='"';for(const auto* at=reinterpret_cast<const unsigned char*>(text);*at;++at){
        const auto c=*at;if(c=='"'||c=='\\'){out+='\\';out+=static_cast<char>(c);}
        else if(c<32)Print(out,"\\u%04x",static_cast<unsigned>(c));else out+=static_cast<char>(c);
    }out+='"';
}
inline void Integer(std::string& out,bool known,int32_t value){if(known)Print(out,"%d",value);else Add(out,"null");}
inline void ObjectJson(std::string& out,const Object& object){
    Print(out,"{\"pointer\":\"%llx\",\"class\":\"%llx\",\"weak_known\":%s,\"weak\":[%d,%d],\"name\":",
        static_cast<unsigned long long>(object.pointer),static_cast<unsigned long long>(object.type),Bool(object.weakKnown),object.index,object.serial);
    if(object.nameKnown)String(out,object.name);else Add(out,"null");Add(out,",\"class_name\":");
    if(object.classNameKnown)String(out,object.className);else Add(out,"null");Add(out,"}");
}
inline void SampleJson(std::string& out,const Sample& s){
    Print(out,"{\"valid\":%s,\"host\":%s,\"at_ms\":%llu,\"serial\":%llu,\"hero\":%u,\"epoch\":%u,\"registration\":%llu,\"stage\":",
        Bool(s.valid),Bool(s.host),static_cast<unsigned long long>(s.atMs),static_cast<unsigned long long>(s.serial),s.hero,s.epoch,static_cast<unsigned long long>(s.frameworkRegistration));String(out,s.stage);
    Add(out,",\"pawn\":");ObjectJson(out,s.pawn);Add(out,",\"player_state\":");ObjectJson(out,s.playerState);
    Add(out,",\"selected_ability\":");ObjectJson(out,s.ability);Add(out,",\"inventory\":");ObjectJson(out,s.inventory);
    Print(out,",\"selected_authored\":%s,\"actual_role\":%u,\"actual_slot\":%u,\"input\":",Bool(s.selectedAuthored),unsigned(s.role),unsigned(s.slot));ObjectJson(out,s.input);
    Print(out,",\"accepted_input_index_known\":%s,\"accepted_input_index\":",Bool(s.inputKnown));Integer(out,s.inputKnown,s.acceptedInputIndex);
    Add(out,",\"primary_held\":");if(s.heldKnown)Print(out,"%u",unsigned(s.primaryHeld));else Add(out,"null");
    Print(out,",\"machine_count\":%zu,\"observed_machine_count\":%zu,\"machine_overflow\":%s,\"machines\":[",s.machineCount,s.observedMachineCount,Bool(s.machineOverflow));
    for(size_t n=0;n<s.machineCount&&n<MaxMachines;++n){const auto& m=s.machines[n];if(n)Add(out,",");
        Print(out,"{\"valid\":%s,\"enabled\":%s,\"links\":%s,\"object\":",Bool(m.valid),Bool(m.enabled),Bool(m.links));ObjectJson(out,m.object);
        Print(out,",\"current_known\":%s,\"current_state\":",Bool(m.currentKnown));ObjectJson(out,m.current);
        Add(out,",\"subframe_seconds\":");if(m.subframeKnown&&std::isfinite(m.subframe))Print(out,"%.9g",double(m.subframe));else Add(out,"null");
        Add(out,",\"pending_count\":");Integer(out,m.pendingKnown,m.pendingCount);
        Add(out,",\"pending_capacity\":");Integer(out,m.pendingKnown,m.pendingCapacity);
        Add(out,",\"pending_first_raw\":");if(m.pendingFirstKnown)Print(out,"\"%016llx\"",static_cast<unsigned long long>(m.pendingFirstRaw));else Add(out,"null");Add(out,"}");
    }
    Print(out,"],\"resource_ambiguous\":%s,\"resource\":{\"kind\":%u,\"object\":",Bool(s.resourceAmbiguous),unsigned(s.resource.kind));ObjectJson(out,s.resource.object);
    const auto& r=s.resource;Add(out,",\"current_native_storage\":");Integer(out,r.currentKnown,r.current);
    Add(out,",\"authority_storage\":");Integer(out,r.authorityKnown,r.authority);Add(out,",\"configured_maximum\":");Integer(out,r.configuredMaximumKnown,r.configuredMaximum);
    Add(out,",\"signature_progress\":");Integer(out,r.signatureKnown,r.signatureProgress);Add(out,",\"signature_per_use\":");Integer(out,r.signatureKnown,r.signaturePerUse);
    Add(out,",\"ultimate_ps_points\":");Integer(out,r.ultimatePointsKnown,r.ultimatePoints);
    Add(out,",\"effective_maximum_known\":false,\"can_consume_known\":false,\"consume_state_resource_resolution_known\":false}}");
}
inline std::string Format(const Sample& latest,const History& history,const char* stage,bool initialized,bool host,
                          bool profiles,bool schema,uint64_t polls,uint64_t faults,uint32_t exceptionCode){
    std::string out;out.reserve(32768);Add(out,"{\"stage\":");String(out,stage);
    Print(out,",\"initialized\":%s,\"host\":%s,\"profiles_verified\":%s,\"schema_verified\":%s,\"polls\":%llu,\"sample_faults\":%llu,\"last_sample_exception\":%u,\"latest\":",
        Bool(initialized),Bool(host),Bool(profiles),Bool(schema),static_cast<unsigned long long>(polls),static_cast<unsigned long long>(faults),exceptionCode);SampleJson(out,latest);
    Print(out,",\"sampled_changes\":%llu,\"history\":[",static_cast<unsigned long long>(history.changes));
    for(size_t n=0;n<history.count;++n){if(n)Add(out,",");SampleJson(out,history.rows[(history.next+HistoryCapacity-history.count+n)%HistoryCapacity]);}
    Add(out,"],\"sample_interval_ms\":25,\"history_records_are_past_samples\":true,\"snapshot_can_miss_transient_inputs\":true,\"zero_pending_is_not_no_input_proof\":true,\"accepted_index_is_not_skill_activation_proof\":true,\"state_without_weak_is_current_parent_reference_only\":true,\"weak_capture_calls\":0,\"weak_serial_allocation_possible\":false,\"native_tick_calls\":0,\"synthetic_input_calls\":0,\"role_resource_clock_state_writes\":0,\"native_name_formatter_uses_temporary_string_allocations\":true,\"metadata_name_lookup_may_refresh_caches\":true,\"network_delivery_verified\":false,\"skill_execution_verified\":false}");return out;
}
} // namespace local_server_hero_skill_readback
