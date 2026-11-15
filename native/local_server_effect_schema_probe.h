#pragma once
#include "original_jett_priming.h"
#include <array>
#include <cstdio>
#include <iterator>

// Pure metadata observation on the original GT. No UFunction invocation,
// adapter binding, object factory, property write or executable allocation.
namespace local_server_effect_schema_probe {
struct OwnerSpec {const char* key;const wchar_t* path;};
inline constexpr OwnerSpec Owners[]{
    {"manager",L"/Script/ShooterGame.EffectManagerComponent"},
    {"active",L"/Script/ShooterGame.ActiveEffectInfo"},
    {"data",L"/Script/ShooterGame.EffectData"},{"id",L"/Script/ShooterGame.EffectID"},
    {"play",L"/Script/ShooterGame.EffectsBlueprintLibrary.PlayContinuousEffect"},
    {"update",L"/Script/ShooterGame.EffectsBlueprintLibrary.UpdateContinuousEffect"},
    {"stop",L"/Script/ShooterGame.EffectsBlueprintLibrary.StopContinuousEffect"},
    {"multicast_play",L"/Script/ShooterGame.EffectManagerComponent.MulticastPlayContinuousEffect"},
    {"multicast_update",L"/Script/ShooterGame.EffectManagerComponent.MulticastUpdateContinuousEffect"},
    {"multicast_stop",L"/Script/ShooterGame.EffectManagerComponent.MulticastStopContinuousEffect"}
};
// kind1=array (Inner+80), kind2=struct (Struct+78), kind0=scalar/object/name.
struct FieldSpec {uint8_t owner,kind;const wchar_t* name;int32_t offset,bytes;};
inline constexpr FieldSpec Fields[]{
    {0,1,L"PendingEffects",0xe8,16},{0,1,L"ServerActiveEffects",0x118,16},{0,1,L"ClientActiveEffects",0x128,16},
    {0,1,L"EffectsPendingReplication",0x150,16},
    {1,2,L"EffectID",0,24},{1,0,L"EffectType",0x18,8},{1,2,L"EffectData",0x20,88},{1,2,L"Transform",0x80,96},
    {1,0,L"Socket",0xe0,12},{1,0,L"StartTimeStamp",0xec,4},{1,0,L"EffectContainer",0xf0,8},
    {1,2,L"ProxyTimerHandle",0xf8,8},{1,0,L"AllianceFilter",0x100,1},
    {2,0,L"WaitOnReplicationActor",0,8},{2,1,L"FloatValues",8,16},{2,1,L"VectorValues",24,16},
    {2,1,L"ObjectValues",40,16},{2,1,L"ClassValues",56,16},{2,1,L"TextValues",72,16},
    {3,0,L"EffectID",0,8},{3,0,L"SourceID",8,12},{3,0,L"bLocalEffect",20,1},{3,0,L"bTransient",21,1},
    {4,0,L"EffectContainer",0,8},{4,0,L"AttachToActor",8,8},{4,0,L"EffectContext",16,8},{4,2,L"EffectData",24,88},
    {4,0,L"WorldContextObject",112,8},{4,2,L"Location",120,24},{4,2,L"Rotation",144,24},
    {4,0,L"AttachSocket",168,12},{4,0,L"bLocalOnly",180,1},{4,0,L"AllianceFilter",181,1},
    {4,0,L"bIsCosmetic",182,1},{4,0,L"bShouldRecordToReplay",183,1},{4,2,L"ReturnValue",184,24},
    {5,0,L"AttachToActor",0,8},{5,0,L"WorldContextObject",8,8},{5,2,L"EffectID",16,24},{5,2,L"EffectData",40,88},
    {6,0,L"AttachToActor",0,8},{6,0,L"WorldContextObject",8,8},{6,2,L"EffectID",16,24},{6,0,L"StopEffectType",40,1},
    {7,0,L"EffectContainer",0,8},{7,2,L"EffectData",8,88},{7,2,L"Transform",96,96},{7,0,L"AttachSocket",192,12},
    {7,2,L"EffectID",208,24},{7,0,L"ClientControllerThatTriggered",232,8},{7,0,L"StartMovementTime",240,4},{7,0,L"AllianceFilter",244,1},
    {8,2,L"EffectID",0,24},{8,2,L"EffectData",24,88},{8,0,L"ClientControllerThatTriggered",112,8},
    {9,2,L"EffectID",0,24},{9,0,L"ClientControllerThatTriggered",24,8},{9,0,L"StopEffectType",32,1},{9,0,L"StopMovementTime",36,4}
};
struct OwnerRow {void *object=nullptr,*type=nullptr,*outer=nullptr;uintptr_t exec=0;uint32_t flags=0;uint16_t parms=0;int32_t size=0,alignment=0;};
struct FieldRow {
    bool found=false,nameMatches=false,boundsValid=false;void *field=nullptr,*fieldClass=nullptr,*inner=nullptr,*structure=nullptr;
    int32_t offset=-1,bytes=0,dimensions=0,innerStride=0,structureSize=0;uint16_t repIndex=0;
    uint64_t flags=0,castFlags=0,nameBits=0;uint8_t boolBytes=0,boolOffset=0,boolMask=0;
};
struct Snapshot {uint64_t captures=0,at=0;std::array<OwnerRow,std::size(Owners)> owners{};std::array<FieldRow,std::size(Fields)> fields{};};
inline Snapshot cached{};inline SRWLOCK lock=SRWLOCK_INIT;
template<class T>inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Capture(){
    if(!engine::image||!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread)return false;
    Snapshot next{};next.at=GetTickCount64();
    for(size_t n=0;n<std::size(Owners);++n){auto& r=next.owners[n];r.object=engine::Find(Owners[n].path);
        r.type=Read<void*>(r.object,0x10);r.outer=Read<void*>(r.object,0x28);r.size=Read<int32_t>(r.object,0x60);r.alignment=Read<int32_t>(r.object,0x64);
        if(n>=4){r.parms=Read<uint16_t>(r.object,0xbe);r.flags=Read<uint32_t>(r.object,0xb8);
            auto exec=Read<uintptr_t>(r.object,0xe0);r.exec=exec>=engine::image?exec-engine::image:exec;}
    }
    for(size_t n=0;n<std::size(Fields);++n){const auto& spec=Fields[n];auto& r=next.fields[n];original_jett_priming::PropertyInfo d{};
        original_jett_priming::FieldOffset(next.owners[spec.owner].object,spec.name,size_t(spec.bytes),&d);
        r.found=d.field!=nullptr;r.nameMatches=d.nameMatches;r.boundsValid=d.boundsValid;r.field=d.field;r.fieldClass=d.fieldClass;
        r.offset=d.offset;r.bytes=d.elementSize;r.dimensions=d.arrayDim;r.flags=Read<uint64_t>(d.field,0x38);r.repIndex=Read<uint16_t>(d.field,0x40);
        r.castFlags=d.castFlags;r.nameBits=d.actualName;r.boolBytes=d.boolFieldSize;r.boolOffset=d.boolByteOffset;r.boolMask=d.boolFieldMask;
        if(spec.kind==1){r.inner=Read<void*>(d.field,0x80);r.innerStride=Read<int32_t>(r.inner,0x34);r.structure=Read<void*>(r.inner,0x78);}
        if(spec.kind==2)r.structure=Read<void*>(d.field,0x78);
        if(r.structure)r.structureSize=Read<int32_t>(r.structure,0x60);
    }
    AcquireSRWLockExclusive(&lock);next.captures=cached.captures+1;cached=next;ReleaseSRWLockExclusive(&lock);return true;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path||!*path)return false;AcquireSRWLockShared(&lock);const auto s=cached;ReleaseSRWLockShared(&lock);
    FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    std::fprintf(file,"{\"captures\":%llu,\"at_ms\":%llu,\"queried_ufunctions_invoked\":0,\"property_writes\":0,\"owners\":[",s.captures,s.at);
    for(size_t n=0;n<std::size(Owners);++n){const auto& r=s.owners[n];
        std::fprintf(file,"%s{\"key\":\"%s\",\"object\":\"%p\",\"type\":\"%p\",\"outer\":\"%p\",\"size_at_60\":%d,\"alignment_at_64\":%d,\"parms_size\":%u,\"flags\":\"0x%x\",\"exec_rva\":\"0x%llx\"}",
            n?",":"",Owners[n].key,r.object,r.type,r.outer,r.size,r.alignment,unsigned(r.parms),r.flags,static_cast<unsigned long long>(r.exec));
    }std::fputs("],\"fields\":[",file);
    for(size_t n=0;n<std::size(Fields);++n){const auto& spec=Fields[n];const auto& r=s.fields[n];char name[128]{};
        WideCharToMultiByte(CP_UTF8,0,spec.name,-1,name,int(sizeof(name)),nullptr,nullptr);
        std::fprintf(file,"%s{\"owner\":\"%s\",\"name\":\"%s\",\"found\":%s,\"field\":\"%p\",\"field_class\":\"%p\",\"expected_offset\":%d,\"offset\":%d,\"expected_bytes\":%d,\"bytes\":%d,\"dimensions\":%d,\"flags\":\"0x%llx\",\"rep_index_at_40\":%u,\"inner_at_80\":\"%p\",\"inner_stride_at_34\":%d,\"struct_at_78\":\"%p\",\"struct_size_at_60\":%d,\"cast_flags\":\"0x%llx\",\"name_bits\":\"0x%llx\",\"bool_bytes\":%u,\"bool_offset\":%u,\"bool_mask\":%u}",
            n?",":"",Owners[spec.owner].key,name,r.found?"true":"false",r.field,r.fieldClass,spec.offset,r.offset,spec.bytes,r.bytes,r.dimensions,
            static_cast<unsigned long long>(r.flags),unsigned(r.repIndex),r.inner,r.innerStride,r.structure,r.structureSize,
            static_cast<unsigned long long>(r.castFlags),static_cast<unsigned long long>(r.nameBits),unsigned(r.boolBytes),unsigned(r.boolOffset),unsigned(r.boolMask));
    }std::fputs("]}\n",file);const bool ok=std::ferror(file)==0;return std::fclose(file)==0&&ok;
}
} // namespace local_server_effect_schema_probe
