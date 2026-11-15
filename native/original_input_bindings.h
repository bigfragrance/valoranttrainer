#pragma once
#include "original_players.h"
#include "original_inventory_slots.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <cstddef>
#include <iterator>
#include <cmath>

// Original default mappings and action traits only. No authored key table,
// account initialization, roaming settings request or telemetry Init. The real
// Options close graph may commit settings changes to its local bounded queue.
// Static evidence: reports/duel1-original-input-bindings.json.
namespace original_input_bindings {
inline constexpr uintptr_t ClientClass=0x6429080,ManagerClass=0x638c320,TraitsClass=0x63a6110;
inline constexpr const wchar_t* TraitsPath=L"/Game/InputActions/ActionTraitsManager.ActionTraitsManager_C";
struct Binding {
    bool found=false,bind_set_found=false,key_down_queried=false,base_key_down=false,analog_queried=false;
    uint8_t axis_direction=0,shift=0,ctrl=0,alt=0,cmd=0,gamepad_shift=0,tap_hold_type=0;
    int32_t bind_index=0;
    float analog=0;
    uintptr_t bind_set=0;
    uint32_t key_name_words[3]{};
    char action[128]{},key_name[128]{};
};
struct Diagnostics {
    bool initialized=false,factory_verified=false,traits_created=false,traits_map_ready=false;
    bool telemetry_created=false,telemetry_local_only=false,empty_identity_verified=false;
    bool content_manager_created=false,content_index_shared=false,manager_created=false,links_valid=false;
    bool native_getter_matches=false,all_sample_bind_sets_found=false;
    unsigned error=0,sample_count=0;
    uintptr_t client=0,game_instance=0,world=0,traits_type=0,traits=0,telemetry=0,rso=0;
    uintptr_t content_manager=0,content_index=0,manager=0,input_settings_cdo=0,exception_rva=0;
    int action_mapping_count=0,axis_mapping_count=0,traits_class_count=0,traits_map_count=0,action_bind_set_count=0;
    uint16_t key_down_parms=0,analog_parms=0;
    uint64_t factory_calls=0,queries=0;
    const char* stage="original_input_bindings_not_initialized";
    std::array<Binding,12> samples{};
};
inline Diagnostics state;
inline const Diagnostics& State(){return state;}
struct SettingsCommitContext {
    bool profileVerified=false,ready=false;
    void* manager=nullptr;
    original_inventory_slots::WeakIdentity identity{};
    uint32_t constructions=0,checks=0;
};
inline SettingsCommitContext settingsCommit{};
struct ViewTransport {
    bool identity_valid=false,native_entries_verified=false,have_submission=false,aim_valid=false;
    bool last_call_unchanged=false,observed_last_direction=false,queued_view_observed=false;
    unsigned error=0;
    uintptr_t pawn=0,controller=0,input=0,input_state=0,exception_rva=0;
    uint64_t calls=0,submissions=0,unchanged=0,observed_matches=0;
    int pending_before=0,pending_after=0,last_pending_count=0,history_capacity=0,history_count=0;
    engine::Vec native_sample{},last_submitted{},observed_aim{};
    const char* stage="original_view_input_not_submitted";
};
inline ViewTransport view;
inline const ViewTransport& ViewState(){return view;}
struct LocalPCInput {
    bool initialized=false,player_input_is_ares=false,owner_valid=false,native_entries_verified=false,bindings_applied=false;
    unsigned error=0;
    uintptr_t controller=0,pawn=0,player_input=0,controller_component=0,pawn_component=0,exception_rva=0;
    uint64_t initialize_calls=0,apply_calls=0,update_calls=0,mouse_producer_calls=0,mouse_samples=0,mouse_events_submitted=0;
    uint64_t blocked_updates=0,discard_calls=0,raw_axis_clear_calls=0;
    int yaw_samples=0,pitch_samples=0,raw_actions=0,raw_events=0,trailing_events=0;
    engine::Vec rotation_before{},rotation_after{};
    bool rotation_changed=false;
    const char* stage="original_local_pc_input_not_initialized";
};
inline LocalPCInput localInput;
inline const LocalPCInput& LocalPCState(){return localInput;}
template<class T> inline T Read(const void* p,size_t off){return original_players::Value<T>(p,off);}
template<class T> inline bool Write(void* p,size_t off,const T& value){return original_players::Write(p,off,value);}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;return false;}
inline bool Thread(){return engine::image&&original_players::gameThread==GetCurrentThreadId();}
inline bool ViewFail(unsigned error,const char* stage){view.error=error;view.stage=stage;return false;}
inline bool CloseDirection(const engine::Vec& a,const engine::Vec& b){return std::abs(a.x-b.x)<1e-10&&std::abs(a.y-b.y)<1e-10&&std::abs(a.z-b.z)<1e-10;}
inline void ObserveViewInput(){
    auto inputState=reinterpret_cast<void*>(view.input_state);if(!Thread()||!inputState)return;
    view.aim_valid=Read<uint8_t>(inputState,0x128)!=0;view.observed_aim=Read<engine::Vec>(inputState,0x238);
    view.history_capacity=Read<int>(inputState,0x11c);view.history_count=Read<int>(inputState,0x120);
    view.last_pending_count=Read<int>(inputState,0xaa0);view.queued_view_observed=false;
    auto events=Read<uint8_t*>(inputState,0xa98);const int cap=Read<int>(inputState,0xaa4);
    if(events&&view.last_pending_count>=0&&view.last_pending_count<=1024&&cap>=view.last_pending_count)
      for(int n=0;n<view.last_pending_count;++n)if(Read<uint8_t>(events,n*40)==5&&CloseDirection(Read<engine::Vec>(events,n*40+8),view.last_submitted))view.queued_view_observed=true;
    view.observed_last_direction=view.have_submission&&view.aim_valid&&CloseDirection(view.observed_aim,view.last_submitted);
    if(view.observed_last_direction)++view.observed_matches;
}
inline bool SubmitStandaloneViewInputImpl(){
    ++view.calls;view.identity_valid=false;view.last_call_unchanged=false;
    if(!Thread()||!state.initialized)return ViewFail(1521,"original_view_input_context_not_ready");
    auto pawn=original_players::activePawn;auto pc=original_players::activeController;auto player=original_players::localPlayer;
    if(!original_players::IsA(pawn,original_players::ShooterCharacterClass)||!original_players::IsA(pc,original_players::ShooterControllerClass)
      ||Read<void*>(pawn,0x4a0)!=pc||Read<void*>(pc,0x4b0)!=pawn||!original_players::IsA(player,0x3b6e680)
      ||Read<void*>(pc,0x510)!=player||Read<void*>(player,0x38)!=pc)return ViewFail(1522,"original_view_input_real_local_player_identity_invalid");
    if(engine::function<int(*)(void*)>(0x3efc820)(pawn)!=0)return true;
    auto table=Read<void**>(pawn,0);auto get=Read<void*>(table,0x740);
    auto input=get?reinterpret_cast<void*(*)(void*,void*)>(get)(pawn,engine::Class(0x6492f30)):nullptr;
    auto inputState=Read<void*>(pawn,0xba0);
    if(!original_players::IsA(input,0x6492f30)||!original_players::IsA(inputState,0x6494370)
      ||Read<void*>(input,0xc8)!=pawn||Read<void*>(input,0x170)!=pawn||Read<void*>(inputState,0xc8)!=pawn||Read<void*>(inputState,0x148)!=pawn
      ||Read<void*>(input,0)!=reinterpret_cast<void*>(engine::image+0xb099220)
      ||Read<void*>(inputState,0)!=reinterpret_cast<void*>(engine::image+0xb093c10))return ViewFail(1523,"original_view_input_component_identity_invalid");
    if(view.input_state&&view.input_state!=reinterpret_cast<uintptr_t>(inputState))view.have_submission=false;
    view.pawn=reinterpret_cast<uintptr_t>(pawn);view.controller=reinterpret_cast<uintptr_t>(pc);
    view.input=reinterpret_cast<uintptr_t>(input);view.input_state=reinterpret_cast<uintptr_t>(inputState);view.identity_valid=true;
    ObserveViewInput();
    if(!Read<void*>(inputState,0x108)||view.history_capacity<=0||view.history_capacity>4096)
      return ViewFail(1524,"original_view_input_native_history_not_constructed");
    constexpr uint8_t sampler[]{0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x70,0x20,0x57,0x48,0x81,0xec,0x60};
    constexpr uint8_t constructor[]{0x48,0x89,0x4c,0x24,0x08,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0xc7,0x41,0x04};
    constexpr uint8_t submit[]{0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xf9,0x48,0x8b,0xda};
    view.native_entries_verified=std::memcmp(reinterpret_cast<const void*>(engine::image+0x70a5420),sampler,sizeof(sampler))==0
      &&std::memcmp(reinterpret_cast<const void*>(engine::image+0x70aa150),constructor,sizeof(constructor))==0
      &&std::memcmp(reinterpret_cast<const void*>(engine::image+0x70a7110),submit,sizeof(submit))==0;
    if(!view.native_entries_verified)return ViewFail(1525,"original_view_input_native_entry_mismatch");
    // Original FVector-return ABI: RCX=input component, RDX=return storage,
    // R8=the actual local PC. The original body applies its real view modifiers.
    engine::Vec direction{};
    engine::function<engine::Vec*(*)(void*,engine::Vec*,void*)>(0x70a5420)(input,&direction,pc);
    view.native_sample=direction;
    const double length2=direction.x*direction.x+direction.y*direction.y+direction.z*direction.z;
    if(!std::isfinite(length2)||length2<.99||length2>1.01)return ViewFail(1526,"original_view_input_native_sample_invalid");
    if(view.have_submission&&CloseDirection(direction,view.last_submitted)){
      view.last_call_unchanged=true;++view.unchanged;view.error=0;view.stage="original_view_input_unchanged";return true;}
    const int pending=Read<int>(inputState,0xaa0),cap=Read<int>(inputState,0xaa4);
    if(pending<0||pending>1024||cap<pending||(pending&&!Read<void*>(inputState,0xa98)))return ViewFail(1527,"original_view_input_destination_buffer_invalid");
    view.pending_before=pending;
    // This is the exact POD event in70a5af1..70a5b3e: tag5, native FVector at8,
    // total40 bytes. Only the missing local transport envelope is supplied here.
    // No state vector/angle is written, no direction is computed by this DLL.
    struct Event{uint8_t type=5,pad[7]{};engine::Vec direction;uint8_t tail[8]{};} event{5,{},direction,{}};
    static_assert(sizeof(Event)==40&&offsetof(Event,direction)==8);
    alignas(8) uint8_t buffer[0x748]{};
    engine::function<void*(*)(void*)>(0x70aa150)(buffer);
    bool sent=false;
    __try{
      const int one=1;std::memcpy(buffer+0x740,&one,4);
      // Original typed TArray growth used by this exact40-byte event producer.
      engine::function<void(*)(void*,int)>(0x18f5ce0)(buffer+0x738,0);
      auto data=Read<void*>(buffer,0x738);
      if(data&&Read<int>(buffer,0x744)>=1){
        std::memcpy(data,&event,sizeof(event));
        engine::function<void(*)(void*,const void*)>(0x70a9a90)(buffer,&event);
        // Native70a7110 ->70a9b60 APPENDS to destination360.pending. It never
        // replaces an existing pending action/view. The later action submission
        // uses that same native append and therefore preserves this view first.
        engine::function<void(*)(void*,void*)>(0x70a7110)(inputState,buffer);
        sent=Read<int>(buffer,0x740)==0;
      }
    }__finally{engine::function<void(*)(void*)>(0x70aa200)(buffer);}
    if(!sent)return ViewFail(1528,"original_view_input_native_transport_failed");
    view.last_submitted=direction;view.have_submission=true;++view.submissions;
    view.pending_after=Read<int>(inputState,0xaa0);ObserveViewInput();
    if(view.pending_after<view.pending_before)return ViewFail(1529,"original_view_input_append_lost_pending_events");
    view.error=0;view.stage="original_view_input_submitted_to_original_tick";return true;
}
inline int ViewException(EXCEPTION_POINTERS* p){const auto at=reinterpret_cast<uintptr_t>(p&&p->ExceptionRecord?p->ExceptionRecord->ExceptionAddress:nullptr);
    view.exception_rva=at>=engine::image&&at-engine::image<0x10000000?at-engine::image:0;ViewFail(1530,"original_view_input_native_exception");return EXCEPTION_EXECUTE_HANDLER;}
inline bool SubmitStandaloneViewInput(){__try{return SubmitStandaloneViewInputImpl();}__except(ViewException(GetExceptionInformation())){return false;}}
inline bool Array(void* object,size_t offset,int maximum){auto data=Read<void*>(object,offset);const int n=Read<int>(object,offset+8),cap=Read<int>(object,offset+12);return n>=0&&n<=maximum&&cap>=n&&(!n||data);}
inline int MapCount(void* object,size_t offset){const int slots=Read<int>(object,offset+8),free=Read<int>(object,offset+0x34);return slots>=0&&slots<=4096&&free>=0&&free<=slots?slots-free:-1;}
inline void* Construct(void* type,void* outer){
    if(!type||!outer||(Read<uint32_t>(type,0xdc)&1))return nullptr;
    struct Parameters{void* type;void* outer;void* result=nullptr;} parameters{type,outer};
    static_assert(sizeof(Parameters)==24&&offsetof(Parameters,result)==16);
    return engine::Event(engine::DefaultObject(engine::Class(0x43a9670)),L"/Script/Engine.GameplayStatics.SpawnObject",&parameters)?parameters.result:nullptr;
}
inline bool EmptyIdentity(void* rso){
    if(!original_players::IsA(rso,0x6bd2cb0))return false;
    // The native binding-change cache copies only these response strings. The
    // already-created offline identity provider must still contain no identity.
    for(size_t off:{size_t(0x158),size_t(0x168),size_t(0x178),size_t(0x188)}){
      const int count=Read<int>(rso,off+8),cap=Read<int>(rso,off+12);auto data=Read<wchar_t*>(rso,off);
      if(count<0||count>1||cap<count||(count&&(!data||Read<wchar_t>(data,0)!=0)))return false;
    }
    return Read<uint8_t>(rso,0x198)==0;
}
inline void NameText(const void* name,char (&out)[128]){
    out[0]=0;if(!name)return;struct String{wchar_t* data=nullptr;int32_t count=0,capacity=0;} text;
    engine::function<void*(*)(const void*,void*)>(0x1a7bf60)(name,&text);
    if(text.data&&text.count>0&&text.count<=1024&&text.capacity>=text.count){
      const int n=WideCharToMultiByte(CP_UTF8,0,text.data,text.count-1,out,127,nullptr,nullptr);if(n>0)out[n]=0;}
    engine::function<void(*)(void*)>(0x18a2f10)(&text);
}
// Borrowed chord is valid only inside this callback, before native shared-reference release.
using NativeChordVisitor=bool(*)(const Binding&,const uint8_t*,void*);
inline bool QueryImpl(const wchar_t* action,uint8_t direction,int index,Binding& out,bool sampleNativeKey,
                      NativeChordVisitor visitor=nullptr,void* context=nullptr){
    out=Binding{};out.axis_direction=direction;out.bind_index=index;
    if(!Thread()||!state.initialized||!action||!*action||direction>2||index<0||index>7)return false;
    alignas(8) uint8_t name[16]{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(name,action,1);NameText(name,out.action);
    auto manager=reinterpret_cast<void*>(state.manager);
    auto set=engine::function<void*(*)(void*,const void*,uint8_t)>(0x708cb20)(manager,name,direction);
    out.bind_set=reinterpret_cast<uintptr_t>(set);out.bind_set_found=original_players::IsA(set,0x638c060);
    if(!out.bind_set_found)return false;
    // FActionChord is 40 bytes: the real FKey32, five modifier bytes and the
    // original TapHoldType byte37. Native query owns its shared FKey reference.
    alignas(8) uint8_t chord[40]{};
    bool visited=true;
    __try{
    out.found=engine::function<bool(*)(void*,const void*,uint8_t,int32_t,void*,bool)>(0x708d250)
      (manager,name,direction,index,chord,true);
    if(out.found){
      std::memcpy(out.key_name_words,chord,12);NameText(chord,out.key_name);
      out.shift=chord[32];out.ctrl=chord[33];out.alt=chord[34];out.cmd=chord[35];out.gamepad_shift=chord[36];out.tap_hold_type=chord[37];
      if(sampleNativeKey&&original_players::activeController){
        alignas(8) uint8_t parameters[40]{};std::memcpy(parameters,chord,32);
        if(state.key_down_parms==33){out.key_down_queried=engine::Event(original_players::activeController,L"/Script/Engine.PlayerController.IsInputKeyDown",parameters);out.base_key_down=parameters[32]!=0;}
        std::memset(parameters+32,0,8);
        if(state.analog_parms==36){out.analog_queried=engine::Event(original_players::activeController,L"/Script/Engine.PlayerController.GetInputAnalogKeyState",parameters);std::memcpy(&out.analog,parameters+32,4);}
      }
      if(visitor)visited=visitor(out,chord,context);
    }
    }__finally{engine::function<void(*)(void*)>(0x18a31e0)(chord+16);}
    ++state.queries;return out.found&&visited;
}
inline int RecordException(EXCEPTION_POINTERS* p){state.error=1519;state.stage="original_input_bindings_native_exception";
    const auto at=reinterpret_cast<uintptr_t>(p&&p->ExceptionRecord?p->ExceptionRecord->ExceptionAddress:nullptr);
    state.exception_rva=at>=engine::image&&at-engine::image<0x10000000?at-engine::image:0;return EXCEPTION_EXECUTE_HANDLER;}
inline bool Query(const wchar_t* action,uint8_t direction,int index,Binding& out,bool sampleNativeKey=false){
    __try{return QueryImpl(action,direction,index,out,sampleNativeKey);}
    __except(RecordException(GetExceptionInformation())){return false;}
}
inline bool QueryWithNativeChord(const wchar_t* action,uint8_t direction,int index,Binding& out,
                                 NativeChordVisitor visitor,void* context){
    if(!visitor)return false;
    __try{return QueryImpl(action,direction,index,out,true,visitor,context);}
    __except(RecordException(GetExceptionInformation())){return false;}
}
inline bool VerifySettingsCommitProfile(){
    if(settingsCommit.profileVerified)return true;
    // pin14776335: actual settings commit, manager/queue constructors and
    // queue consumer. The consumer waits while its transport (+18) is null.
    struct Profile{uintptr_t rva;size_t bytes;uint64_t hash;};
    static constexpr Profile profiles[]{
        {0x79362c0,0x804,0xd27f078f93945d7fULL},
        {0x6d77270,0xa9,0x1fd97e22c0c17dd8ULL},
        {0x771f150,0x13b,0xfb08e22921d082ceULL},
        {0x771f3f0,0x87,0xbf9e04cbfb901b24ULL},
        {0x771f690,0x8d,0x5d1107db1cf6749cULL}
    };
    for(const auto& p:profiles){
        auto code=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(code,p.bytes))return false;
        uint64_t hash=14695981039346656037ULL;
        for(size_t n=0;n<p.bytes;++n)hash=(hash^code[n])*1099511628211ULL;
        if(hash!=p.hash)return false;
    }
    settingsCommit.profileVerified=true;return true;
}
inline bool EnsureSettingsCommitContext(void* client,bool create=false){
    settingsCommit.ready=false;++settingsCommit.checks;
    if(!Thread()||!original_players::IsA(client,ClientClass)
        ||reinterpret_cast<uintptr_t>(client)!=state.client)
        return Fail(1560,"original_settings_commit_owner_invalid");
    auto cache=Read<void*>(client,0x5c0);
    if(!original_players::IsA(cache,0x6a81250)
        ||reinterpret_cast<uintptr_t>(cache)!=state.telemetry
        ||Read<void*>(cache,0xe0)!=Read<void*>(client,0x378)
        ||Read<void*>(cache,0xe8)!=Read<void*>(client,0x750)
        ||Read<void*>(cache,0xf0)||Read<uint16_t>(cache,0x78))
        return Fail(1561,"original_settings_commit_cache_dependencies_invalid");
    if(!VerifySettingsCommitProfile())return Fail(1562,"original_settings_commit_profile_mismatch");
    auto manager=Read<void*>(cache,0xf8);
    if(!manager&&create&&!settingsCommit.manager){
        // Real original TelemetryManager and its bounded (100 item) queue.
        // Outer is the real local client, as at702da3d. Only cache+f8 retains
        // this local context; do not publish it as the global telemetry service.
        // No InitSystem, transport binding, account or network setup is called.
        manager=Construct(engine::Class(0x6d73e20),client);
        if(!original_players::IsA(manager,0x6d73e20)
            ||!Write(cache,0xf8,manager))
            return Fail(1563,"original_settings_commit_context_construction_failed");
        settingsCommit.manager=manager;++settingsCommit.constructions;
        if(!original_inventory_slots::CaptureIdentity(manager,settingsCommit.identity))
            return Fail(1563,"original_settings_commit_context_identity_failed");
    }
    // Do not replace foreign, retired or partially initialized service objects.
    if(!manager||manager!=settingsCommit.manager
        ||original_inventory_slots::ResolveIdentity(settingsCommit.identity)!=manager
        ||!original_players::IsA(manager,0x6d73e20)||Read<void*>(manager,0x28)!=client
        ||Read<uint16_t>(manager,0x78)||Read<void*>(manager,0xd8))
        return Fail(1564,"original_settings_commit_context_not_local_or_live");
    // Original ctor771f150 owns the queue thread, event and sentinel. Reject
    // an incomplete/stopped queue before Options.Exit reaches the commit leaf.
    if(!Read<void*>(manager,0xd0)||!Read<void*>(manager,0xe8)
        ||!Read<void*>(manager,0xf0)||!Read<void*>(manager,0xf8)
        ||Read<int32_t>(manager,0xe0)!=0)
        return Fail(1565,"original_settings_commit_queue_not_ready");
    settingsCommit.ready=true;return true;
}
inline bool InitializeImpl(void* client){
    if(!Thread())return Fail(1501,"original_input_bindings_wrong_thread");
    if(state.client&&state.client!=reinterpret_cast<uintptr_t>(client))return Fail(1502,"original_input_bindings_client_changed");
    state.client=reinterpret_cast<uintptr_t>(client);auto gi=Read<void*>(client,0x2c8);auto world=original_players::world;
    state.game_instance=reinterpret_cast<uintptr_t>(gi);state.world=reinterpret_cast<uintptr_t>(world);
    if(!original_players::IsA(client,ClientClass)||!original_players::IsA(gi,0x647e8a0)||Read<void*>(gi,0x2b8)!=client
      ||Read<void*>(client,0x28)!=gi||Read<void*>(world,0x1d8)!=gi)return Fail(1503,"original_input_bindings_client_identity_invalid");
    auto settings=Read<void*>(client,0x378);auto rso=Read<void*>(client,0x750);state.rso=reinterpret_cast<uintptr_t>(rso);
    state.empty_identity_verified=EmptyIdentity(rso);
    if(!original_players::IsA(settings,0x6510860)||!Read<uint8_t>(settings,0x890)||!state.empty_identity_verified)
      return Fail(1504,"original_input_bindings_local_settings_dependencies_missing");
    auto index=Read<void*>(gi,0x2c8);state.content_index=reinterpret_cast<uintptr_t>(index);
    if(!original_players::IsA(index,0x66711f0)||!Read<uint8_t>(index,0xf8))return Fail(1505,"original_input_bindings_original_content_index_not_ready");
    auto content=Read<void*>(client,0x308);
    if(!content){content=Construct(engine::Class(0x667b420),client);
      if(!original_players::IsA(content,0x667b420)||!Write(content,0xa0,index)||!Write(client,0x308,content))return Fail(1506,"original_input_bindings_content_provider_construction_failed");state.content_manager_created=true;}
    state.content_manager=reinterpret_cast<uintptr_t>(content);state.content_index_shared=original_players::IsA(content,0x667b420)&&Read<void*>(content,0xa0)==index;
    if(!state.content_index_shared)return Fail(1507,"original_input_bindings_content_provider_mismatch");
    auto traits=Read<void*>(client,0x5d8);
    if(!traits){
      auto type=engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t)>(0x1d0c6e0)(engine::Class(TraitsClass),nullptr,TraitsPath,nullptr,0);
      state.traits_type=reinterpret_cast<uintptr_t>(type);
      if(!type||!original_players::Derived(type,engine::Class(TraitsClass)))return Fail(1508,"original_input_bindings_authored_traits_class_missing");
      traits=Construct(type,client);
      if(!original_players::IsA(traits,TraitsClass)||!Write(client,0x5d8,traits))return Fail(1509,"original_input_bindings_authored_traits_construction_failed");state.traits_created=true;
    }
    state.traits=reinterpret_cast<uintptr_t>(traits);state.traits_class_count=Read<int>(traits,0x38);state.traits_map_count=MapCount(traits,0x40);
    state.traits_map_ready=original_players::IsA(traits,TraitsClass)&&Array(traits,0x30,4096)&&state.traits_class_count>0&&state.traits_map_count>0;
    if(!state.traits_map_ready)return Fail(1510,"original_input_bindings_authored_traits_postconstruct_not_observed");
    auto telemetry=Read<void*>(client,0x5c0);
    if(!telemetry){telemetry=Construct(engine::Class(0x6a81250),client);
      if(!original_players::IsA(telemetry,0x6a81250)||!Write(client,0x5c0,telemetry))return Fail(1511,"original_input_bindings_local_change_cache_construction_failed");state.telemetry_created=true;}
    state.telemetry=reinterpret_cast<uintptr_t>(telemetry);
    if(!original_players::IsA(telemetry,0x6a81250)||Read<void*>(telemetry,0xf0)||Read<uint16_t>(telemetry,0x78))
      return Fail(1512,"original_input_bindings_change_cache_is_not_local_only");
    if((Read<void*>(telemetry,0xe0)&&Read<void*>(telemetry,0xe0)!=settings)||(Read<void*>(telemetry,0xe8)&&Read<void*>(telemetry,0xe8)!=rso)
      ||!Write(telemetry,0xe0,settings)||!Write(telemetry,0xe8,rso))return Fail(1513,"original_input_bindings_change_cache_dependencies_mismatch");
    if(!EnsureSettingsCommitContext(client,true))return false;
    state.telemetry_local_only=true;
    auto input=engine::DefaultObject(engine::Class(0x410a230));state.input_settings_cdo=reinterpret_cast<uintptr_t>(input);
    state.action_mapping_count=Read<int>(input,0xa0);state.axis_mapping_count=Read<int>(input,0xb0);
    if(!Array(input,0x98,4096)||!Array(input,0xa8,4096)||state.action_mapping_count<=0||state.axis_mapping_count<=0)
      return Fail(1514,"original_input_bindings_original_input_config_mappings_missing");
    constexpr uint8_t signature[]{0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48,0x89,0x7c,0x24,0x20,0x55};
    state.factory_verified=std::memcmp(reinterpret_cast<const void*>(engine::image+0x708a440),signature,sizeof(signature))==0;
    if(!state.factory_verified)return Fail(1515,"original_input_bindings_factory_entry_mismatch");
    auto manager=Read<void*>(client,0x5e0);
    if(!manager){state.stage="original_input_bindings_calling_original_factory";
      manager=engine::function<void*(*)(void*,void*,void*,void*)>(0x708a440)(client,traits,telemetry,content);++state.factory_calls;
      if(!original_players::IsA(manager,ManagerClass)||!Write(client,0x5e0,manager))return Fail(1516,"original_input_bindings_original_factory_failed");state.manager_created=true;}
    state.manager=reinterpret_cast<uintptr_t>(manager);state.action_bind_set_count=MapCount(manager,0x90);
    state.links_valid=original_players::IsA(manager,ManagerClass)&&Read<void*>(manager,0x130)==traits&&Read<void*>(manager,0x138)==telemetry&&Read<void*>(manager,0x140)==index;
    state.native_getter_matches=engine::function<void*(*)(const void*)>(0x708a710)(world)==manager;
    if(!state.links_valid||state.action_bind_set_count<=0||!state.native_getter_matches)return Fail(1517,"original_input_bindings_factory_readback_failed");
    auto down=engine::Find(L"/Script/Engine.PlayerController.IsInputKeyDown");auto analog=engine::Find(L"/Script/Engine.PlayerController.GetInputAnalogKeyState");
    state.key_down_parms=Read<uint16_t>(down,0xbe);state.analog_parms=Read<uint16_t>(analog,0xbe);
    state.initialized=true;state.error=0;state.stage="original_input_bindings_original_defaults_available";
    // Semantic queries validate prompts; this list never creates any binding.
    constexpr const wchar_t* samples[]{L"PrimaryTriggerDown",L"SecondaryTriggerDown",L"Activate_Primary",L"Activate_Secondary",L"Activate_Melee",L"UseObject",L"OutlinesFriendly",L"ObservePlayer1",L"ToggleShop",L"InGameMenu",L"Reload"};
    state.all_sample_bind_sets_found=true;state.sample_count=static_cast<unsigned>(std::size(samples));
    for(unsigned n=0;n<state.sample_count;++n){QueryImpl(samples[n],0,0,state.samples[n],false);state.all_sample_bind_sets_found&=state.samples[n].bind_set_found;}
    return true;
}
inline bool Initialize(void* clientGI){__try{return InitializeImpl(clientGI);}__except(RecordException(GetExceptionInformation())){return false;}}
inline bool LocalPCFail(unsigned error,const char* stage){localInput.error=error;localInput.stage=stage;return false;}
inline bool LocalPCIdentity(){
    auto pc=original_players::activeController;auto pawn=original_players::activePawn;auto player=original_players::localPlayer;
    return Thread()&&state.initialized&&original_players::IsA(pc,original_players::ShooterControllerClass)
      &&original_players::IsA(pawn,original_players::ShooterCharacterClass)&&original_players::IsA(player,0x3b6e680)
      &&Read<void*>(pc,0x510)==player&&Read<void*>(player,0x38)==pc&&Read<void*>(pc,0x4b0)==pawn&&Read<void*>(pawn,0x4a0)==pc;
}
inline bool InitializeLocalPCImpl(){
    if(!LocalPCIdentity())return LocalPCFail(1541,"original_local_pc_input_identity_invalid");
    auto pc=original_players::activeController;auto pawn=original_players::activePawn;
    if(localInput.initialized)return localInput.controller==reinterpret_cast<uintptr_t>(pc)&&localInput.pawn==reinterpret_cast<uintptr_t>(pawn);
    localInput.controller=reinterpret_cast<uintptr_t>(pc);localInput.pawn=reinterpret_cast<uintptr_t>(pawn);
    auto table=Read<void**>(pc,0);
    constexpr uint8_t init[]{0x48,0x89,0x5c,0x24,0x10,0x57,0x48,0x81,0xec,0xc0,0x00,0x00,0x00,0x48,0x8b,0x05};
    constexpr uint8_t apply[]{0x48,0x89,0x5c,0x24,0x10,0x57,0x48,0x83,0xec,0x50,0x48,0x8b,0xd9,0x33,0xff,0x48};
    constexpr uint8_t mouse[]{0x40,0x53,0x41,0x57,0x48,0x81,0xec,0xb8,0x00,0x00,0x00,0x48,0x8b,0x81,0x70,0x01};
    constexpr uint8_t clear[]{0x8b,0x81,0x40,0x07,0x00,0x00,0x01,0x41,0x04,0x48,0x81,0xc1,0x38,0x07,0x00,0x00};
    constexpr uint8_t assignAxis[]{0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0x48,0x3b,0xca};
    localInput.native_entries_verified=Read<void*>(table,0xfe8)==reinterpret_cast<void*>(engine::image+0x781cbd0)
      &&Read<void*>(table,0xed8)==reinterpret_cast<void*>(engine::image+0x78af080)
      &&std::memcmp(reinterpret_cast<const void*>(engine::image+0x781cbd0),init,sizeof(init))==0
      &&std::memcmp(reinterpret_cast<const void*>(engine::image+0x78b0740),apply,sizeof(apply))==0
      &&std::memcmp(reinterpret_cast<const void*>(engine::image+0x70a5850),mouse,sizeof(mouse))==0
      &&std::memcmp(reinterpret_cast<const void*>(engine::image+0x70aa200),clear,sizeof(clear))==0
      &&std::memcmp(reinterpret_cast<const void*>(engine::image+0x18cc380),assignAxis,sizeof(assignAxis))==0;
    if(!localInput.native_entries_verified)return LocalPCFail(1542,"original_local_pc_input_native_entries_changed");
    // Native SetPlayer493e2a0 normally already calls this exact virtual. Supply
    // it only if its input objects are absent; never replace an existing object.
    if(!Read<void*>(pc,0x5e8)||!Read<void*>(pc,0x230)){
      if(Read<void*>(pc,0x5e8)&&!original_players::IsA(Read<void*>(pc,0x5e8),0x64f9c00))
        return LocalPCFail(1543,"original_local_pc_input_wrong_existing_player_input_class");
      engine::function<void(*)(void*)>(0x781cbd0)(pc);++localInput.initialize_calls;
    }
    auto input=Read<void*>(pc,0x5e8);localInput.player_input=reinterpret_cast<uintptr_t>(input);
    localInput.controller_component=reinterpret_cast<uintptr_t>(Read<void*>(pc,0x230));
    localInput.pawn_component=reinterpret_cast<uintptr_t>(Read<void*>(pawn,0x230));
    localInput.player_input_is_ares=original_players::IsA(input,0x64f9c00);
    localInput.owner_valid=Read<void*>(input,0x28)==pc;
    if(!localInput.player_input_is_ares||!localInput.owner_valid||!localInput.controller_component
      ||!original_players::IsA(reinterpret_cast<void*>(localInput.pawn_component),0x6492f30))
      return LocalPCFail(1544,"original_local_pc_input_objects_not_ready_after_client_restart");
    // Original title binding application copies the real current bind layers,
    // modifiers and tap/hold behavior into this original AresPlayerInput.
    engine::function<void(*)(void*)>(0x78b0740)(pc);++localInput.apply_calls;localInput.bindings_applied=true;
    localInput.initialized=true;localInput.error=0;localInput.stage="original_local_pc_input_ready";return true;
}
inline int LocalPCException(EXCEPTION_POINTERS* p){const auto at=reinterpret_cast<uintptr_t>(p&&p->ExceptionRecord?p->ExceptionRecord->ExceptionAddress:nullptr);
    localInput.exception_rva=at>=engine::image&&at-engine::image<0x10000000?at-engine::image:0;LocalPCFail(1549,"original_local_pc_input_native_exception");return EXCEPTION_EXECUTE_HANDLER;}
inline bool InitializeLocalPC(){__try{return InitializeLocalPCImpl();}__except(LocalPCException(GetExceptionInformation())){return false;}}
inline bool UpdateImpl(float deltaSeconds,bool blockGameplay){
    ++localInput.update_calls;
    if(!localInput.initialized||!LocalPCIdentity())return LocalPCFail(1545,"original_local_pc_input_update_before_initialize");
    auto pc=original_players::activeController;auto pawn=original_players::activePawn;
    if(localInput.controller!=reinterpret_cast<uintptr_t>(pc)||localInput.pawn!=reinterpret_cast<uintptr_t>(pawn)
      ||Read<void*>(pc,0x5e8)!=reinterpret_cast<void*>(localInput.player_input))return LocalPCFail(1546,"original_local_pc_input_pair_changed");
    if(engine::function<int(*)(void*)>(0x3efc820)(pawn)!=0)return true;
    auto input=Read<void*>(pawn,0x230);auto inputState=Read<void*>(pawn,0xba0);
    if(input!=reinterpret_cast<void*>(localInput.pawn_component)||!original_players::IsA(input,0x6492f30)
      ||Read<void*>(input,0x170)!=pawn||!original_players::IsA(inputState,0x6494370))return LocalPCFail(1546,"original_local_pc_input_component_changed");
    localInput.yaw_samples=Read<int>(input,0x1ea0);localInput.pitch_samples=Read<int>(input,0x1eb0);
    localInput.raw_actions=Read<int>(input,0x1000);localInput.rotation_before=Read<engine::Vec>(pc,0x4e8);
    if(blockGameplay){
      // These are game-character buffers, separate from PC/Slate/CommonUI.
      // Keep buffer178 intact so the caller can submit Gun/Jett releases.
      if(!Array(input,0xff8,1048576)||!Array(input,0x1740,1048576)||!Array(input,0x1e88,1048576)
        ||!Array(input,0x1e98,1048576)||!Array(input,0x1ea8,1048576))
        return LocalPCFail(1550,"original_local_pc_blocked_buffers_invalid");
      for(size_t offset:{size_t(0x8c0),size_t(0x1008),size_t(0x1750)}){
        engine::function<void(*)(void*)>(0x70aa200)(static_cast<uint8_t*>(input)+offset);++localInput.discard_calls;}
      // Raw axis arrays are TArray<float>, not FAresInputStateBuffer. The real
      // axis callbacks70a5650/5750 use this exact native copy-assignment leaf
      // to replace the whole array every frame. Assign an empty local array
      // to discard a last mouse sample if UIOnly stops those game callbacks.
      struct EmptyAxis{void* data=nullptr;int count=0,capacity=0;} empty;
      static_assert(sizeof(EmptyAxis)==16);
      for(size_t offset:{size_t(0x1e98),size_t(0x1ea8)}){
        engine::function<void*(*)(void*,const void*)>(0x18cc380)(static_cast<uint8_t*>(input)+offset,&empty);++localInput.raw_axis_clear_calls;}
      ++localInput.blocked_updates;localInput.rotation_after=Read<engine::Vec>(pc,0x4e8);
      localInput.raw_events=Read<int>(input,0x1748);localInput.trailing_events=Read<int>(input,0x1e90);
      localInput.error=0;localInput.stage="original_local_pc_game_input_discarded_for_ui";return true;
    }
    if(localInput.yaw_samples<0||localInput.yaw_samples>4096||localInput.pitch_samples!=localInput.yaw_samples
      ||Read<int>(input,0x1ea4)<localInput.yaw_samples||Read<int>(input,0x1eb4)<localInput.pitch_samples
      ||(localInput.yaw_samples&&(!Read<void*>(input,0x1e98)||!Read<void*>(input,0x1ea8)))
      ||!Array(input,0xff8,4096)||!Array(input,0x1740,4096)||!Array(input,0x1e88,4096))
      return LocalPCFail(1547,"original_local_pc_input_raw_buffers_invalid");
    if(!std::isfinite(deltaSeconds)||deltaSeconds<0||deltaSeconds>1)return LocalPCFail(1548,"original_local_pc_input_invalid_frame_delta");
    if(deltaSeconds>0&&(localInput.yaw_samples||localInput.raw_actions)){
      // Only the original mouse/action producer omitted by Standalone's early
      // Tick return. It applies native sensitivity, AddYaw/AddPitch, native
      // UpdateRotation and emits the original subframe view/action records.
      engine::function<void(*)(void*,float)>(0x70a5850)(input,deltaSeconds);
      ++localInput.mouse_producer_calls;localInput.mouse_samples+=static_cast<uint64_t>(localInput.yaw_samples);
      // This is the original Tick's source-buffer clear through its native leaf.
      engine::function<void(*)(void*)>(0x70aa200)(static_cast<uint8_t*>(input)+0x8c0);
    }
    localInput.raw_events=Read<int>(input,0x1748);localInput.trailing_events=Read<int>(input,0x1e90);
    for(size_t offset:{size_t(0x1008),size_t(0x1750)}){
      const int n=Read<int>(input,offset+0x740);
      if(n){engine::function<void(*)(void*,void*)>(0x70a7110)(inputState,static_cast<uint8_t*>(input)+offset);
        localInput.mouse_events_submitted+=static_cast<uint64_t>(n);}
    }
    localInput.rotation_after=Read<engine::Vec>(pc,0x4e8);
    localInput.rotation_changed|=!CloseDirection(localInput.rotation_before,localInput.rotation_after);
    localInput.error=0;localInput.stage="original_local_pc_input_mouse_transport_updated";return true;
}
inline bool Update(float deltaSeconds,bool blockGameplay=false){__try{return UpdateImpl(deltaSeconds,blockGameplay);}__except(LocalPCException(GetExceptionInformation())){return false;}}
inline void JsonString(FILE* file,const char* value){std::fputc('"',file);for(const unsigned char* p=reinterpret_cast<const unsigned char*>(value);*p;++p){if(*p=='"'||*p=='\\')std::fputc('\\',file);if(*p>=32)std::fputc(*p,file);else std::fprintf(file,"\\u%04x",*p);}std::fputc('"',file);}
inline bool WriteDiagnostics(const wchar_t* path){if(Thread())ObserveViewInput();FILE* file=nullptr;if(!path||_wfopen_s(&file,path,L"wb")||!file)return false;const auto& s=state;
    std::fprintf(file,"{\"schema_version\":2,\"stage\":\"%s\",\"error\":%u,\"manual_key_table\":false,\"online_initialization_called\":false,\"telemetry_init_called\":false,\"settings_commit_context_ready\":%s",s.stage,s.error,settingsCommit.ready?"true":"false");
#define INPUT_BIND_BOOL(k) std::fprintf(file,",\"" #k "\":%s",s.k?"true":"false")
#define INPUT_BIND_INT(k) std::fprintf(file,",\"" #k "\":%lld",static_cast<long long>(s.k))
    INPUT_BIND_BOOL(initialized);INPUT_BIND_BOOL(factory_verified);INPUT_BIND_BOOL(traits_created);INPUT_BIND_BOOL(traits_map_ready);
    INPUT_BIND_BOOL(telemetry_created);INPUT_BIND_BOOL(telemetry_local_only);INPUT_BIND_BOOL(empty_identity_verified);
    INPUT_BIND_BOOL(content_manager_created);INPUT_BIND_BOOL(content_index_shared);INPUT_BIND_BOOL(manager_created);INPUT_BIND_BOOL(links_valid);
    INPUT_BIND_BOOL(native_getter_matches);INPUT_BIND_BOOL(all_sample_bind_sets_found);
    INPUT_BIND_INT(client);INPUT_BIND_INT(game_instance);INPUT_BIND_INT(world);INPUT_BIND_INT(traits_type);INPUT_BIND_INT(traits);INPUT_BIND_INT(telemetry);INPUT_BIND_INT(rso);
    INPUT_BIND_INT(content_manager);INPUT_BIND_INT(content_index);INPUT_BIND_INT(manager);INPUT_BIND_INT(input_settings_cdo);INPUT_BIND_INT(exception_rva);
    INPUT_BIND_INT(action_mapping_count);INPUT_BIND_INT(axis_mapping_count);INPUT_BIND_INT(traits_class_count);INPUT_BIND_INT(traits_map_count);INPUT_BIND_INT(action_bind_set_count);
    INPUT_BIND_INT(factory_calls);INPUT_BIND_INT(queries);INPUT_BIND_INT(key_down_parms);INPUT_BIND_INT(analog_parms);
    std::fprintf(file,",\"semantic_binding_queries\":[");
    for(unsigned n=0;n<s.sample_count;++n){if(n)std::fputc(',',file);const auto& b=s.samples[n];std::fprintf(file,"{\"action\":");JsonString(file,b.action);std::fprintf(file,",\"key\":");JsonString(file,b.key_name);
      std::fprintf(file,",\"bind_set_found\":%s,\"binding_found\":%s,\"bind_set\":%llu,\"axis_direction\":%u,\"index\":%d,\"modifiers\":[%u,%u,%u,%u,%u],\"tap_hold_type\":%u}",b.bind_set_found?"true":"false",b.found?"true":"false",static_cast<unsigned long long>(b.bind_set),b.axis_direction,b.bind_index,b.shift,b.ctrl,b.alt,b.cmd,b.gamepad_shift,b.tap_hold_type);}
    const auto& v=view;
    std::fprintf(file,"],\"standalone_view_transport\":{\"stage\":\"%s\",\"error\":%u,\"sampler_rva\":\"0x70a5420\",\"submission_rva\":\"0x70a7110\",\"event_type\":5,\"original_tick_consumes_event\":true,\"direct_aim_state_write\":false",v.stage,v.error);
#define INPUT_VIEW_BOOL(k) std::fprintf(file,",\"" #k "\":%s",v.k?"true":"false")
#define INPUT_VIEW_INT(k) std::fprintf(file,",\"" #k "\":%lld",static_cast<long long>(v.k))
    INPUT_VIEW_BOOL(identity_valid);INPUT_VIEW_BOOL(native_entries_verified);INPUT_VIEW_BOOL(have_submission);INPUT_VIEW_BOOL(aim_valid);
    INPUT_VIEW_BOOL(last_call_unchanged);INPUT_VIEW_BOOL(observed_last_direction);INPUT_VIEW_BOOL(queued_view_observed);
    INPUT_VIEW_INT(pawn);INPUT_VIEW_INT(controller);INPUT_VIEW_INT(input);INPUT_VIEW_INT(input_state);INPUT_VIEW_INT(exception_rva);
    INPUT_VIEW_INT(calls);INPUT_VIEW_INT(submissions);INPUT_VIEW_INT(unchanged);INPUT_VIEW_INT(observed_matches);
    INPUT_VIEW_INT(pending_before);INPUT_VIEW_INT(pending_after);INPUT_VIEW_INT(last_pending_count);INPUT_VIEW_INT(history_capacity);INPUT_VIEW_INT(history_count);
    std::fprintf(file,",\"native_sample\":[%.17g,%.17g,%.17g],\"last_submitted\":[%.17g,%.17g,%.17g],\"observed_aim\":[%.17g,%.17g,%.17g]}",
      v.native_sample.x,v.native_sample.y,v.native_sample.z,v.last_submitted.x,v.last_submitted.y,v.last_submitted.z,v.observed_aim.x,v.observed_aim.y,v.observed_aim.z);
    const auto& l=localInput;
    std::fprintf(file,",\"local_pc_input\":{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"player_input_is_ares\":%s,\"owner_valid\":%s,\"native_entries_verified\":%s,\"bindings_applied\":%s,\"rotation_changed\":%s",
      l.stage,l.error,l.initialized?"true":"false",l.player_input_is_ares?"true":"false",l.owner_valid?"true":"false",l.native_entries_verified?"true":"false",l.bindings_applied?"true":"false",l.rotation_changed?"true":"false");
#define INPUT_LOCAL_INT(k) std::fprintf(file,",\"" #k "\":%lld",static_cast<long long>(l.k))
    INPUT_LOCAL_INT(controller);INPUT_LOCAL_INT(pawn);INPUT_LOCAL_INT(player_input);INPUT_LOCAL_INT(controller_component);INPUT_LOCAL_INT(pawn_component);INPUT_LOCAL_INT(exception_rva);
    INPUT_LOCAL_INT(initialize_calls);INPUT_LOCAL_INT(apply_calls);INPUT_LOCAL_INT(update_calls);INPUT_LOCAL_INT(mouse_producer_calls);INPUT_LOCAL_INT(mouse_samples);INPUT_LOCAL_INT(mouse_events_submitted);
    INPUT_LOCAL_INT(blocked_updates);INPUT_LOCAL_INT(discard_calls);INPUT_LOCAL_INT(raw_axis_clear_calls);
    INPUT_LOCAL_INT(yaw_samples);INPUT_LOCAL_INT(pitch_samples);INPUT_LOCAL_INT(raw_actions);INPUT_LOCAL_INT(raw_events);INPUT_LOCAL_INT(trailing_events);
    std::fprintf(file,",\"rotation_before\":[%.17g,%.17g,%.17g],\"rotation_after\":[%.17g,%.17g,%.17g]}}\n",
      l.rotation_before.x,l.rotation_before.y,l.rotation_before.z,l.rotation_after.x,l.rotation_after.y,l.rotation_after.z);std::fclose(file);
#undef INPUT_LOCAL_INT
#undef INPUT_VIEW_BOOL
#undef INPUT_VIEW_INT
#undef INPUT_BIND_BOOL
#undef INPUT_BIND_INT
    return true;
}
}
