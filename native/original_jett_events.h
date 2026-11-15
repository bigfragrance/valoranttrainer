#pragma once
#include "original_jett_native.h"
#include <array>

// Fill only the stripped standalone event-registration records. Native
// RegisterEvent builds its real per-equippable TSet and central event pool.
// No event is emitted here, and no state-machine response or target is added.
namespace original_jett_events {
struct Registration {
    void *actor=nullptr,*component=nullptr,*registry=nullptr,*type=nullptr;
    bool before=false,after=false,componentEnabled=false;
    unsigned error=0;
};
struct Diagnostics {
    bool initialized=false,ready=false,restartRequired=false;
    unsigned error=0;
    const char* stage="original_jett_events_not_initialized";
    void* pawn=nullptr;
    uintptr_t exceptionRva=0;
};
inline Diagnostics state;
inline std::array<original_weapon::EquipEventRegistration,original_jett::SkillCount> equipped{};
inline Registration cloudburstThrown;
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;return false;}
inline bool Register(void* actor,uintptr_t getter,Registration& result){
    result.actor=actor;result.component=Read<void*>(actor,0xe18);result.registry=Read<void*>(result.component,0x198);
    result.type=engine::Class(getter);result.componentEnabled=Read<uint8_t>(result.component,0x189)!=0;
    if(Read<void*>(actor,0x4a8)!=state.pawn||!original_players::IsA(result.component,0x6768f30)
       ||Read<void*>(result.component,0xc8)!=actor||!result.componentEnabled||!result.registry||!result.type
       ||engine::function<int(*)(void*)>(0x3efc820)(actor)!=0
       ||engine::function<void*(*)(void*)>(0x6ff6000)(actor)!=result.registry){result.error=3101;return false;}
    result.before=engine::function<bool(*)(void*,void*)>(0x6ff68d0)(result.registry,result.type);
    // Preserve the native registration lifecycle even if another equippable
    // already registered this class in the shared central registry.
    engine::function<void(*)(void*,void*,uint8_t)>(0x6ff5f40)(result.component,result.type,0);
    result.after=engine::function<bool(*)(void*,void*)>(0x6ff68d0)(result.registry,result.type);
    if(!result.after)result.error=3102;
    return result.after;
}
inline bool InitializeImpl(void* pawn){
    if(!original_players::Thread()||!original_jett::State().loadoutGranted||pawn!=original_jett::State().pawn)
        return Fail(3103,"original_jett_events_requires_actual_granted_local_context");
    if(state.initialized)return state.pawn==pawn&&state.ready;
    state.pawn=pawn;state.initialized=true;
    for(size_t i=0;i<original_jett::SkillCount;++i){
        // False disables the gun response adapter. C/E have no authored
        // AutoEquip graph; their own original activation inputs remain intact.
        if(!original_weapon::RegisterOriginalOfflineEquipEventFor(pawn,original_jett::skills[i].actor,&equipped[i],false))
            return Fail(equipped[i].error,"original_jett_events_original_equipped_registration_failed");
    }
    if(!Register(original_jett::skills[0].actor,0x6767b90,cloudburstThrown))
        return Fail(cloudburstThrown.error,"original_jett_events_original_projectile_thrown_registration_failed");
    state.ready=true;state.error=0;state.stage="original_jett_events_native_registration_records_verified";return true;
}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0003100U;
    const auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;state.restartRequired=true;
    state.stage="original_jett_events_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* pawn){
    if(state.restartRequired)return false;
    __try{return InitializeImpl(pawn);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;std::wstring temporary(path);temporary+=L".tmp";FILE* f=nullptr;
    if(_wfopen_s(&f,temporary.c_str(),L"wb")||!f)return false;const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"pid\":%lu,\"stage\":",GetCurrentProcessId());original_players::Text(f,state.stage);
    std::fprintf(f,",\"error\":%u,\"initialized\":%s,\"ready\":%s,\"restart_required\":%s,\"exception_rva\":%llu,\"pawn\":\"%p\",\"equipped\":[",state.error,b(state.initialized),b(state.ready),b(state.restartRequired),static_cast<unsigned long long>(state.exceptionRva),state.pawn);
    for(size_t i=0;i<equipped.size();++i){
        if(i)std::fputc(',',f);const auto& e=equipped[i];
        std::fprintf(f,"{\"skill\":%zu,\"actor\":\"%p\",\"component\":\"%p\",\"registry\":\"%p\",\"type\":\"%p\",\"registered_before\":%s,\"registered_after\":%s,\"valid\":%s,\"error\":%u,\"equip_response_supplied\":%s}",i,original_jett::skills[i].actor,reinterpret_cast<void*>(e.component),reinterpret_cast<void*>(e.registry),reinterpret_cast<void*>(e.type),b(e.registered_before),b(e.registered_after),b(e.valid),e.error,b(e.local_equip_response_supplied));
    }
    const auto& r=cloudburstThrown;
    std::fprintf(f,"],\"cloudburst_projectile_thrown\":{\"actor\":\"%p\",\"component\":\"%p\",\"registry\":\"%p\",\"type\":\"%p\",\"component_enabled\":%s,\"registered_before\":%s,\"registered_after\":%s,\"error\":%u},\"emits_events\":false,\"changes_state_response_graphs\":false}\n",r.actor,r.component,r.registry,r.type,b(r.componentEnabled),b(r.before),b(r.after),r.error);
    const bool ok=std::ferror(f)==0,closed=std::fclose(f)==0;
    return ok&&closed&&MoveFileExW(temporary.c_str(),path,MOVEFILE_REPLACE_EXISTING);
}
}
