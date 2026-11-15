#pragma once
#include "original_players.h"
#include "original_weapon.h"
#include <array>

// Supply only missing local authority tuning DATA from the actual installed
// DataTable. Native DataTable row lookup, native GameplayTag/AbilityTuningValue
// map insertion/copy/destruction and original change notifications are reused.
// No skill parameter, duration, trajectory, state response or CDO is invented.
// Provenance: work/duel1-original-jett-native/tuning-*.json and its report.
namespace original_ability_tuning {
inline constexpr uintptr_t ComponentClass=0x637a920,DataTableClass=0x3cea760;
inline constexpr size_t MapBytes=0x50,MaxComponents=16,MaxRows=512;
struct NativeName{uint32_t words[3]{};};
static_assert(sizeof(NativeName)==12);
struct Value{uint8_t bytes[24]{};};
static_assert(sizeof(Value)==24);
struct WeakIdentity{int32_t index=-1,serial=0;};
struct Record {
    void *actor=nullptr,*component=nullptr,*table=nullptr,*rowStruct=nullptr;
    WeakIdentity identity{};
    bool attempted=false,ready=false,tableGetterValid=false,tableTypeValid=false,rowStructValid=false;
    bool nativeSnapshotCopied=false,nativeSnapshotDestroyed=false,changeNotification=false,sourceRowsPreserved=false;
    int32_t rows=0,mapBefore=0,mapAfter=0,imported=0,reused=0,overridesPreserved=0,readback=0;
    uint32_t valueTypes[4]{};
};
struct Diagnostics {
    uint32_t error=0,componentCount=0,readyCount=0,actorsWithoutComponent=0,retiredRecords=0;
    bool restartRequired=false;
    uintptr_t exceptionRva=0;
    const char* stage="original_ability_tuning_not_initialized";
    const char* operation="none";
    const wchar_t* failedEvent=nullptr;
    uint16_t expectedEventSize=0,actualEventSize=0;
};
inline Diagnostics state;
inline std::array<Record,MaxComponents> records{};
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(uint32_t code,const char* stage){state.error=code;state.stage=stage;return false;}
inline bool Thread(){return !state.restartRequired&&original_players::Thread();}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* parameters){
    auto fn=engine::Find(name);const auto actual=Read<uint16_t>(fn,0xbe);
    if(!object||!fn||actual!=size){state.failedEvent=name;state.expectedEventSize=size;state.actualEventSize=actual;return false;}
    auto table=Read<void**>(object,0);auto process=Read<void*>(table,0x298);
    if(!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,fn,parameters);return true;
}
inline int32_t MapCount(const void* map){
    const auto count=Read<int32_t>(map,8),capacity=Read<int32_t>(map,12);
    const auto bits=Read<int32_t>(map,0x28),free=Read<int32_t>(map,0x34);
    if(count<0||count>8192||capacity<count||bits<count||free<0||free>count)return -1;
    return count-free;
}
inline void* FindValue(void* map,const void* tag){
    if(MapCount(map)<0)return nullptr;
    const auto count=Read<int32_t>(map,8);auto data=Read<uint8_t*>(map,0);
    auto bits=Read<const uint32_t*>(map,0x20);
    if(!bits)bits=reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(map)+0x10);
    if(count&&(!original_players::Readable(data,static_cast<size_t>(count)*48)
       ||!original_players::Readable(bits,static_cast<size_t>((count+31)/32)*4)))return nullptr;
    for(int32_t n=0;n<count;++n){
        if(!(bits[n/32]&(1U<<(n%32))))continue;
        auto item=data+static_cast<size_t>(n)*48;
        // Original6f10fe0 compares the semantic first eight bytes of FName;
        // native insertion retains all12 bytes, including its display index.
        if(std::memcmp(item,tag,8)==0)return item+0x10;
    }
    return nullptr;
}
inline bool NativeEmptyMap(const void* map){
    return MapCount(map)==0&&Read<int32_t>(map,8)==0&&!Read<void*>(map,0)
        &&!Read<void*>(map,0x20)&&!Read<void*>(map,0x40);
}
struct MapSnapshot {
    alignas(8) uint8_t bytes[MapBytes]{};
    bool copied=false;
    uint8_t alignmentPadding[7]{};
    void Destroy(){
        if(!copied)return;
        // Exact lifecycle from AbilityTuningComponent destructor6383660:
        // native hash allocation+40, then native sparse-map destructor20338f0.
        if(auto hash=Read<void*>(bytes,0x40))engine::function<void(*)(void*)>(0x1915720)(hash);
        engine::function<void(*)(void*)>(0x20338f0)(bytes);copied=false;
    }
    ~MapSnapshot(){Destroy();}
};
inline bool SnapshotMap(void* actualMap,MapSnapshot& snapshot){
    auto defaults=engine::DefaultObject(engine::Class(ComponentClass));
    if(!defaults)return Fail(2701,"original_ability_tuning_native_empty_map_default_unavailable");
    auto empty=static_cast<uint8_t*>(defaults)+0xe8;
    if(!NativeEmptyMap(empty))return Fail(2701,"original_ability_tuning_native_empty_map_default_unavailable");
    std::memcpy(snapshot.bytes,empty,MapBytes);
    // Same original copy operation used by original OnRep6f11474. TuningValue
    // consists of native scalar fields and a reflected UClass reference; native
    // insertion/copy owns all allocator work, with no CRT-owned UE allocations.
    engine::function<void(*)(void*,const void*)>(0x58b16c0)(snapshot.bytes,actualMap);
    snapshot.copied=true;
    if(MapCount(snapshot.bytes)!=MapCount(actualMap))return Fail(2702,"original_ability_tuning_native_map_snapshot_count_mismatch");
    for(const auto offset:{size_t(0),size_t(0x20),size_t(0x40)}){
        const auto allocation=Read<void*>(snapshot.bytes,offset);
        if(allocation&&allocation==Read<void*>(actualMap,offset)){
            snapshot.copied=false;state.restartRequired=true;
            return Fail(2703,"original_ability_tuning_native_map_copy_aliased_source");
        }
    }
    return true;
}
inline uint64_t DigestRows(const std::array<const uint8_t*,MaxRows>& rows,int32_t count){
    uint64_t value=1469598103934665603ULL;
    for(int32_t n=0;n<count;++n)for(size_t i=0;i<48;++i)value=(value^rows[n][i])*1099511628211ULL;
    return value;
}
inline Record* FindRecord(void* component){
    for(auto& record:records)if(record.attempted){
        if(engine::function<void*(*)(const WeakIdentity*)>(0x1d4cfa0)(&record.identity)!=record.component){
            if(record.ready&&state.readyCount)--state.readyCount;
            if(state.componentCount)--state.componentCount;
            ++state.retiredRecords;record={};continue;
        }
        if(record.component==component)return &record;
    }
    for(auto& record:records)if(!record.attempted)return &record;
    return nullptr;
}
inline bool EnsureComponentImpl(void* component){
    if(!Thread()||!original_players::IsA(component,ComponentClass))return Fail(2704,"original_ability_tuning_requires_actual_component_on_game_thread");
    auto record=FindRecord(component);if(!record)return Fail(2705,"original_ability_tuning_live_diagnostic_capacity_exhausted");
    if(record->ready)return true;
    if(record->attempted)return Fail(2706,"original_ability_tuning_partial_import_requires_new_session");
    record->attempted=true;record->component=component;record->actor=Read<void*>(component,0xc8);
    record->identity.index=Read<int32_t>(component,0xc);
    if(record->identity.index<0)return Fail(2707,"original_ability_tuning_component_weak_identity_invalid");
    record->identity.serial=engine::function<int32_t(*)(void*,int32_t)>(0x1cf35b0)
        (reinterpret_cast<void*>(engine::image+0xd7b87c0),record->identity.index);
    if(record->identity.index<0||record->identity.serial<=0)return Fail(2707,"original_ability_tuning_component_weak_identity_invalid");
    ++state.componentCount;state.operation="original_ability_tuning_query_actual_table";
    record->tableGetterValid=Event(component,L"/Script/ShooterGame.AbilityTuningComponent.GetAbilityTuningTable",8,&record->table);
    record->tableTypeValid=original_players::IsA(record->table,DataTableClass);
    if(!record->tableGetterValid||!record->tableTypeValid)return Fail(2708,"original_ability_tuning_original_data_table_missing");
    record->rowStruct=Read<void*>(record->table,0x30);
    record->rowStructValid=record->rowStruct==engine::Find(L"/Script/ShooterGame.AbilityTuningDataTableRow");
    if(!record->rowStruct||!record->rowStructValid)return Fail(2709,"original_ability_tuning_original_row_struct_mismatch");
    struct RowNames{void* table;NativeName* data=nullptr;int32_t count=0,capacity=0;} names{record->table};
    static_assert(sizeof(RowNames)==24);
    if(!Event(engine::DefaultObject(engine::Class(0x42f1030)),L"/Script/Engine.DataTableFunctionLibrary.GetDataTableRowNames",24,&names))
        return Fail(2710,"original_ability_tuning_native_row_names_query_failed");
    struct NamesOwner{void* data;~NamesOwner(){if(data)engine::function<void(*)(void*)>(0x1915720)(data);}} namesOwner{names.data};
    if(names.count<=0||names.count>static_cast<int32_t>(MaxRows)||names.capacity<names.count
       ||!original_players::Readable(names.data,static_cast<size_t>(names.count)*sizeof(NativeName)))
        return Fail(2711,"original_ability_tuning_native_row_names_array_invalid");
    record->rows=names.count;auto map=static_cast<uint8_t*>(component)+0xe8;
    record->mapBefore=MapCount(map);if(record->mapBefore<0)return Fail(2712,"original_ability_tuning_original_map_invalid");
    std::array<const uint8_t*,MaxRows> rows{};
    for(int32_t n=0;n<names.count;++n){
        // Actual UDataTable row-map vtable query and native FindRowUnchecked;
        // this borrowed row was deserialized by the installed client itself.
        auto row=engine::function<const uint8_t*(*)(void*,const NativeName*)>(0x42ee820)(record->table,names.data+n);
        if(!original_players::Readable(row,48)||!Read<uint64_t>(row,8)||Read<uint8_t>(row,0x18)>3)
            return Fail(2713,"original_ability_tuning_native_row_tag_or_value_invalid");
        if(Read<uint8_t>(row,0x18)==2&&!std::isfinite(Read<float>(row,0x20)))
            return Fail(2714,"original_ability_tuning_original_float_nonfinite");
        for(int32_t prior=0;prior<n;++prior)if(std::memcmp(rows[prior]+8,row+8,8)==0)
            return Fail(2715,"original_ability_tuning_original_table_has_duplicate_semantic_tag");
        rows[n]=row;++record->valueTypes[Read<uint8_t>(row,0x18)];
    }
    const auto sourceDigest=DigestRows(rows,names.count);
    MapSnapshot before;
    if(!SnapshotMap(map,before))return false;
    record->nativeSnapshotCopied=true;state.operation="original_ability_tuning_supply_missing_original_rows";
    for(int32_t n=0;n<names.count;++n){
        auto row=rows[n];auto existing=FindValue(map,row+8);
        if(existing){
            ++record->reused;
            if(std::memcmp(existing,row+0x18,sizeof(Value))!=0)++record->overridesPreserved;
            continue;
        }
        auto value=engine::function<void*(*)(void*,const void*,const void*)>(0x6f242e0)(map,row+8,row+0x18);
        if(!value||std::memcmp(value,row+0x18,sizeof(Value))!=0)
            return Fail(2716,"original_ability_tuning_original_map_insert_readback_failed");
        ++record->imported;
    }
    if(record->imported){
        state.operation="original_ability_tuning_original_change_notification";
        engine::function<void(*)(void*,const void*,const void*)>(0x6f11900)(component,before.bytes,map);
        record->changeNotification=true;
    }
    record->mapAfter=MapCount(map);
    for(int32_t n=0;n<names.count;++n)if(FindValue(map,rows[n]+8))++record->readback;
    if(record->mapAfter<record->mapBefore+record->imported||record->readback!=names.count)
        return Fail(2717,"original_ability_tuning_original_map_lookup_readback_failed");
    // Borrowed row bytes never change, and original runtime overrides remain.
    record->sourceRowsPreserved=sourceDigest==DigestRows(rows,names.count);
    if(!record->sourceRowsPreserved)return Fail(2720,"original_ability_tuning_original_source_rows_changed");
    before.Destroy();record->nativeSnapshotDestroyed=true;
    record->ready=true;++state.readyCount;state.error=0;state.operation="none";
    state.stage="original_ability_tuning_original_table_rows_ready";return true;
}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0002700U;
    const auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;state.restartRequired=true;
    state.stage="original_ability_tuning_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool EnsureComponent(void* component){
    __try{return EnsureComponentImpl(component);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool EnsureActorImpl(void* actor){
    if(!Thread()||!actor)return Fail(2718,"original_ability_tuning_actual_actor_missing");
    auto component=original_weapon::Component(actor,ComponentClass);
    if(!component){++state.actorsWithoutComponent;return true;}
    if(Read<void*>(component,0xc8)!=actor)return Fail(2719,"original_ability_tuning_component_owner_mismatch");
    return EnsureComponentImpl(component);
}
inline bool EnsureActor(void* actor){
    __try{return EnsureActorImpl(actor);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;std::wstring temporary(path);temporary+=L".tmp";
    FILE* f=nullptr;if(_wfopen_s(&f,temporary.c_str(),L"wb")||!f)return false;
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"pid\":%lu,\"stage\":",GetCurrentProcessId());original_players::Text(f,state.stage);
    std::fprintf(f,",\"operation\":");original_players::Text(f,state.operation);
    std::fprintf(f,",\"error\":%u,\"restart_required\":%s,\"exception_rva\":%llu,\"component_count\":%u,\"ready_count\":%u,\"actors_without_component\":%u,\"retired_records\":%u,\"failed_event\":",state.error,b(state.restartRequired),static_cast<unsigned long long>(state.exceptionRva),state.componentCount,state.readyCount,state.actorsWithoutComponent,state.retiredRecords);
    const auto event=original_players::Utf8(state.failedEvent);original_players::Text(f,event.c_str());
    std::fprintf(f,",\"expected_event_size\":%u,\"actual_event_size\":%u,\"components\":[",state.expectedEventSize,state.actualEventSize);
    bool first=true;
    for(const auto& r:records)if(r.attempted){
        if(!first)std::fputc(',',f);first=false;
        std::fprintf(f,"{\"actor\":\"%p\",\"component\":\"%p\",\"table\":\"%p\",\"row_struct\":\"%p\",\"ready\":%s,\"table_getter_valid\":%s,\"table_type_valid\":%s,\"row_struct_valid\":%s,\"rows\":%d,\"map_before\":%d,\"map_after\":%d,\"imported\":%d,\"reused\":%d,\"overrides_preserved\":%d,\"readback\":%d,\"native_snapshot_copied\":%s,\"native_snapshot_destroyed\":%s,\"change_notification\":%s,\"source_rows_preserved\":%s,\"value_types\":[%u,%u,%u,%u]}",r.actor,r.component,r.table,r.rowStruct,b(r.ready),b(r.tableGetterValid),b(r.tableTypeValid),b(r.rowStructValid),r.rows,r.mapBefore,r.mapAfter,r.imported,r.reused,r.overridesPreserved,r.readback,b(r.nativeSnapshotCopied),b(r.nativeSnapshotDestroyed),b(r.changeNotification),b(r.sourceRowsPreserved),r.valueTypes[0],r.valueTypes[1],r.valueTypes[2],r.valueTypes[3]);
    }
    std::fputs("],\"uses_authored_table_values_only\":true,\"changes_original_tables_or_cdos\":false,\"runtime_skill_execution_verified\":false}\n",f);
    const bool ok=std::ferror(f)==0,closed=std::fclose(f)==0;
    return ok&&closed&&MoveFileExW(temporary.c_str(),path,MOVEFILE_REPLACE_EXISTING);
}
}
