#pragma once
#include "original_inventory_slots.h"
#include <array>
#include <cstdio>
#include <iterator>

// Diagnostic metadata only. No queried UFunction, property setter or native frame
// is executed. This records the actual UFunction size rather than treating a
// generated C++ parameter container's trailing alignment as the runtime size.
namespace local_server_reflection_probe {
struct Spec {const char* key;const wchar_t* path;};
inline constexpr Spec Specs[]{
    {"is_server",L"/Script/Engine.KismetSystemLibrary.IsServer"},
    {"is_dedicated_server",L"/Script/Engine.KismetSystemLibrary.IsDedicatedServer"},
    {"equippable_register_event",L"/Script/ShooterGame.EquippableEventEquippableComponent.RegisterEvent"},
    {"resource_can_consume",L"/Script/ShooterGame.ResourceComponent.CanConsume"},
    {"resource_replenish",L"/Script/ShooterGame.ResourceComponent.Replenish"},
    {"ultimate_set_points",L"/Script/ShooterGame.ShooterPlayerState.AuthSetUltimatePoints"},
    {"ultimate_consume",L"/Script/ShooterGame.ShooterPlayerState.AuthConsumeUltimatePoints"},
    {"ultimate_active",L"/Script/ShooterGame.ShooterPlayerState.AuthSetUltimateActive"},
    {"spawn_ability_child",L"/Script/ShooterGame.ShooterBlueprintLibrary.SpawnAbilityChildActor"},
    {"spawn_game_object",L"/Script/ShooterGame.ShooterBlueprintLibrary.SpawnGameObject"}
};
struct Row {void *function=nullptr,*owner=nullptr;uintptr_t execRva=0;uint32_t flags=0;uint16_t bytes=0;bool found=false;};
struct Snapshot {uint64_t samples=0,at=0;std::array<Row,std::size(Specs)> rows{};};
inline Snapshot cache{};inline SRWLOCK lock=SRWLOCK_INIT;inline uint64_t nextSample=0;
inline void Poll(){
    if(!engine::image||!original_players::Thread())return;
    const auto now=GetTickCount64();if(now<nextSample)return;nextSample=now+1000;
    Snapshot value{};value.at=now;
    for(size_t i=0;i<std::size(Specs);++i){auto& row=value.rows[i];
        row.function=engine::Find(Specs[i].path);row.found=row.function!=nullptr;if(!row.found)continue;
        row.owner=original_players::Value<void*>(row.function,0x28);
        row.bytes=original_players::Value<uint16_t>(row.function,0xbe);
        row.flags=original_players::Value<uint32_t>(row.function,0xb8);
        const auto exec=original_players::Value<uintptr_t>(row.function,0xe0);
        row.execRva=exec>=engine::image&&exec<engine::image+0xe69c000?exec-engine::image:0;
    }
    AcquireSRWLockExclusive(&lock);value.samples=cache.samples+1;cache=value;ReleaseSRWLockExclusive(&lock);
}
inline void Write(FILE* file){
    if(!file)return;AcquireSRWLockShared(&lock);const auto value=cache;ReleaseSRWLockShared(&lock);
    std::fprintf(file,"{\"samples\":%llu,\"at_ms\":%llu,\"queried_functions_invoked\":0,\"rows\":[",value.samples,value.at);
    for(size_t i=0;i<std::size(Specs);++i){const auto& row=value.rows[i];
        std::fprintf(file,"%s{\"key\":\"%s\",\"found\":%s,\"function\":\"%p\",\"owner\":\"%p\",\"parms_size\":%u,\"flags\":%u,\"exec_rva\":\"0x%llx\"}",
            i?",":"",Specs[i].key,row.found?"true":"false",row.function,row.owner,unsigned(row.bytes),row.flags,static_cast<unsigned long long>(row.execRva));}
    std::fputs("]}",file);
}
}
