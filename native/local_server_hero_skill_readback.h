#pragma once
#include "local_server_hero_abilities.h"
#include "local_server_input.h"
#include "local_server_input_lifecycle.h"
#include "local_server_client.h"
#include "local_server_hero_skill_readback_data.h"
#include "local_server_hero_skill_readback_profiles.h"

// Only the current registered human's original selected authored item is read.
// No event/exec hook, native simulation/input call, resource predicate, actor
// factory, weak serial allocator, replicated setter or UObject writer exists.
namespace local_server_hero_skill_readback {
namespace fw=local_server_hero_abilities;
using Weak=original_inventory_slots::WeakIdentity;
struct State {
    bool initialized=false,host=false,closed=false,busy=false,profiles=false,schema=false;
    bool sharedOwnerProfiles=false,inputSchema=false,machineSchema=false,resourceSchema=false,chargeSchema=false,signatureSchema=false,ultimateSchema=false;
    DWORD thread=0;void* world=nullptr;Weak worldId{};uint64_t polls=0,faults=0,lastAt=0,serial=0;
    uint32_t lastException=0;uint64_t nameFormats=0;const char* stage="hero_skill_readback_not_initialized";
    uintptr_t getterFunction=0,getterExec=0,getterOwner=0;uint32_t getterFlags=0;uint16_t getterBytes=0;bool getterMetadata=false;
};
struct NativeString {wchar_t* data=nullptr;int32_t count=0,capacity=0;};
struct Name {uint32_t words[3]{};};
struct NativeArray {void* data=nullptr;int32_t count=0,capacity=0;};
static_assert(sizeof(NativeString)==16&&sizeof(Name)==12&&sizeof(NativeArray)==16);
inline State state{};inline Sample latest{};inline History history{};
inline SRWLOCK lock=SRWLOCK_INIT;
inline std::string json="{\"stage\":\"hero_skill_readback_not_initialized\",\"skill_execution_verified\":false}";
struct LabelCache {Object label{};Name name{},className{};};
inline std::array<LabelCache,128> labelCache{};inline size_t labelNext=0;
struct FieldDiagnostic {int32_t expectedOffset=0,actualOffset=-1,expectedSize=0,actualSize=0,expectedDim=0,actualDim=0;bool valid=false;char name[96]{};};
inline std::array<FieldDiagnostic,24> fields{};inline size_t fieldCount=0;
inline const State& GetState(){return state;}
template<class T>inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool GT(){return state.initialized&&!state.closed&&state.thread==GetCurrentThreadId()&&original_players::Thread();}
inline bool Field(const wchar_t* owner,const wchar_t* name,int32_t offset,int32_t size,int32_t dim=1,bool structurePrefix=false){
    auto type=engine::Find(owner);original_jett_priming::PropertyInfo info{};
    const auto actual=original_jett_priming::FieldOffset(type,name,static_cast<size_t>(size),&info);
    const bool valid=actual==offset&&info.arrayDim==dim&&(structurePrefix?(info.elementSize>=size&&info.elementSize<=64):info.elementSize==size);
    if(fieldCount<fields.size()){
        auto& d=fields[fieldCount++];d={};d.expectedOffset=offset;d.actualOffset=actual;d.expectedSize=size;d.actualSize=info.elementSize;
        d.expectedDim=dim;d.actualDim=info.arrayDim;d.valid=valid;
        WideCharToMultiByte(CP_UTF8,0,name,-1,d.name,static_cast<int>(sizeof(d.name)),nullptr,nullptr);
    }return valid;
}
inline bool Verify(){
    for(const auto& p:local_server_hero_skill_readback_source::Profiles){
        const auto bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes))return false;
        uint64_t hash=0xcbf29ce484222325ULL;for(size_t n=0;n<p.bytes;++n)hash=(hash^bytes[n])*0x100000001b3ULL;
        if(hash!=p.fnv)return false;
    }
    state.profiles=true;
    // The existing publication module verifies these only on HOST Prepare.
    // A receiver must verify the same complete retained bodies privately;
    // never call a host producer to acquire an observation permission flag.
    for(const auto& p:local_server_item_ownership::Profiles){
        const auto code=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(code,p.bytes))return false;
        uint64_t hash=0xcbf29ce484222325ULL;
        for(size_t n=0;n<p.bytes;++n)hash=(hash^code[n])*0x100000001b3ULL;
        if(hash!=p.fnv)return false;
    }
    state.sharedOwnerProfiles=true;
    fieldCount=0;
    auto currentGetter=engine::Find(L"/Script/ShooterGame.StateMachineComponent.GetCurrentState");
    state.getterFunction=reinterpret_cast<uintptr_t>(currentGetter);state.getterExec=Read<uintptr_t>(currentGetter,0xe0);
    state.getterOwner=Read<uintptr_t>(currentGetter,0x28);state.getterFlags=Read<uint32_t>(currentGetter,0xb8);state.getterBytes=Read<uint16_t>(currentGetter,0xbe);
    state.getterMetadata=currentGetter&&state.getterBytes==8&&state.getterFlags==0x54020401U
        &&state.getterOwner==reinterpret_cast<uintptr_t>(engine::Find(L"/Script/ShooterGame.StateMachineComponent"))
        &&state.getterExec==engine::image+0x6d25830;
    state.inputSchema=Field(L"/Script/ShooterGame.AresInventory",L"CurrentEquippable",0x280,8)
        &&Field(L"/Script/ShooterGame.AresInputStateComponent",L"CurrentEquippableInputStates",0x265,1,15)
        &&Field(L"/Script/ShooterGame.AresItem",L"MyPawn",0x4a8,8);
    state.machineSchema=Field(L"/Script/ShooterGame.StateMachineComponent",L"AuthCurrentState",0x1c8,8,1,true)
        &&Field(L"/Script/ShooterGame.StateMachineComponent",L"ClientPredictedCurrentState",0x1b0,8,1,true)
        &&Field(L"/Script/ShooterGame.StateMachineComponent",L"CurrentSubframeWorldTimeSeconds",0x1e0,4)
        &&Field(L"/Script/ShooterGame.EquippableStateMachineComponent",L"bStateMachineEnabled",0x348,1);
    state.resourceSchema=Field(L"/Script/ShooterGame.ResourceComponent",L"PredictedResourceAmount",0x10c,4)
        &&Field(L"/Script/ShooterGame.ResourceComponent",L"AuthResourceAmount",0x110,4);
    state.chargeSchema=Field(L"/Script/ShooterGame.EquipmentChargeComponent",L"MaxCharges",0x188,4);
    state.signatureSchema=Field(L"/Script/ShooterGame.SignatureAbilityResourceComponent",L"NumSignatureChargesPerUse",0x1a8,4)
        &&Field(L"/Script/ShooterGame.SignatureAbilityResourceComponent",L"AuthSignatureChargeAmount",0x1ac,4);
    state.ultimateSchema=Field(L"/Script/ShooterGame.UltPointsComponent",L"OwningCharacter",0x148,8)
        &&Field(L"/Script/ShooterGame.UltPointsComponent",L"OwningPlayerState",0x150,8)
        &&Field(L"/Script/ShooterGame.ShooterPlayerState",L"NumUltimatePoints",0xbdc,4);
    state.schema=state.inputSchema&&state.machineSchema&&state.getterMetadata;return true;
}
inline bool FormatName(const Name& name,char (&out)[96]){
    NativeString text{};bool valid=false;++state.nameFormats;
    __try{
        engine::function<void*(*)(const void*,void*)>(0x1a7bf60)(&name,&text);
        if(text.data&&text.count>0&&text.count<=1024&&text.capacity>=text.count){
            const auto count=WideCharToMultiByte(CP_UTF8,0,text.data,text.count-1,out,95,nullptr,nullptr);
            if(count>0){out[count]=0;valid=true;}
        }
    }__finally{engine::function<void(*)(void*)>(0x18a2f10)(&text);}
    return valid;
}
inline Object Label(void* object,const Weak* borrowed=nullptr){
    Object out{};out.pointer=reinterpret_cast<uintptr_t>(object);if(!object)return out;
    if(borrowed){if(!Live(object,*borrowed))return out;out.weakKnown=true;out.index=borrowed->index;out.serial=borrowed->serial;}
    out.type=reinterpret_cast<uintptr_t>(Read<void*>(object,0x10));const auto name=Read<Name>(object,0x18);
    const auto className=out.type?Read<Name>(reinterpret_cast<void*>(out.type),0x18):Name{};
    for(const auto& entry:labelCache)if(entry.label.pointer==out.pointer&&entry.label.type==out.type
        &&std::memcmp(&entry.name,&name,sizeof(Name))==0&&std::memcmp(&entry.className,&className,sizeof(Name))==0){
        out.nameKnown=entry.label.nameKnown;out.classNameKnown=entry.label.classNameKnown;
        std::memcpy(out.name,entry.label.name,sizeof(out.name));std::memcpy(out.className,entry.label.className,sizeof(out.className));return out;
    }
    out.nameKnown=FormatName(name,out.name);if(out.type)out.classNameKnown=FormatName(className,out.className);
    labelCache[labelNext]={out,name,className};labelNext=(labelNext+1)%labelCache.size();
    return out;
}
inline bool Current(const fw::Player& p,void* pawn,uint32_t epoch){
    const auto& c=p.context;
    const auto& f=fw::GetState();
    if(!GT()||!state.profiles||!state.schema||!state.sharedOwnerProfiles
       ||!f.initialized||f.error||f.restartRequired||f.world!=state.world||f.host!=state.host
       ||!Live(state.world,state.worldId)||!p.used||p.retired||p.error||!p.hero||p.hero>=original_heroes::PlayableHeroCount||!p.registration
       ||p.epoch!=epoch||c.epoch!=epoch||p.authority!=state.host||c.pawn!=pawn||c.world!=state.world||!c.sourceReady
       ||!Live(c.world,c.worldId)||!Live(c.pawn,c.pawnId)||!Live(c.controller,c.controllerId)
       ||!Live(c.playerState,c.playerStateId)||!Live(c.inventory,c.inventoryId)||!Live(p.heroClass,p.heroClassId)
       ||Read<void*>(c.controller,0x470)!=c.playerState||Read<void*>(c.controller,0x4b0)!=pawn
       ||Read<void*>(pawn,0x488)!=c.playerState||Read<void*>(pawn,0x4a0)!=c.controller
       ||Read<void*>(pawn,0xc08)!=c.inventory||Read<void*>(c.inventory,0xc8)!=pawn||Read<void*>(c.inventory,0x2f0)!=pawn
       ||Read<uint8_t>(pawn,0x224)!=(state.host?3:2)||Read<uint8_t>(c.controller,0x224)!=(state.host?3:2)
       ||local_server_item_ownership::ActorWorld(pawn)!=state.world||local_server_item_ownership::ActorWorld(c.controller)!=state.world
       ||local_server_item_ownership::ActorWorld(c.playerState)!=state.world)return false;
    if(state.host){void* connection=nullptr;return local_server_item_ownership::ActualHuman(c.world,c.controller,c.playerState,pawn,c.inventory,connection);}
    const auto& client=local_server_client::GetState();
    return client.localPairVerified&&client.nativeClient&&client.world==c.world&&client.controller==c.controller
        &&client.pawn==pawn&&client.playerState==c.playerState
        &&original_players::activePawn==pawn&&original_players::activeController==c.controller&&original_players::localPlayer
        &&Read<void*>(c.controller,0x510)==original_players::localPlayer&&Read<void*>(original_players::localPlayer,0x38)==c.controller;
}
inline bool ItemCurrent(const fw::Player& p,const fw::Item& i){
    const auto& n=i.native;const auto& c=p.context;
    return i.assembled&&n.published&&n.ownerReady&&n.slotType<17&&Live(n.actor,n.actorId)&&Live(n.slot,n.slotId)
        &&Live(n.type,n.typeId)&&Read<void*>(n.actor,0x10)==n.type&&Read<void*>(n.actor,0x4a8)==c.pawn
        &&Read<void*>(n.actor,0x210)==c.pawn&&Read<uint8_t>(n.actor,0x224)==(state.host?3:2)
        &&Read<void*>(c.inventory,0x280)==n.actor&&Read<void*>(c.inventory,0x1b8+size_t(n.slotType)*8)==n.slot
        &&Read<void*>(n.slot,0x28)==c.inventory&&Read<void*>(n.slot,0x40)==c.inventory&&Read<uint8_t>(n.slot,0x30)==n.slotType
        &&local_server_item_ownership::ActorWorld(n.actor)==state.world&&local_server_item_ownership::Contains(n.slot,n.actor);
}
inline bool BorrowInput(const fw::Player& p,void*& input,Weak& id){
    input=nullptr;id={};const auto& c=p.context;
    if(state.host){const auto& in=local_server_input::GetState();
        if(in.pawn!=c.pawn||in.pc!=c.controller||in.world!=c.world||in.epochRetired||in.error
           ||in.pawnIdentity.index!=c.pawnId.index||in.pawnIdentity.serial!=c.pawnId.serial)return false;
        input=in.inputState;id={in.inputIdentity.index,in.inputIdentity.serial};
    }else{const auto& in=local_server_input_lifecycle::GetState();
        if(!in.initialized||in.closed||in.closing||in.error||in.epoch.pawn!=c.pawn||in.epoch.pc!=c.controller||in.epoch.world!=c.world
           ||in.epoch.pawnId.index!=c.pawnId.index||in.epoch.pawnId.serial!=c.pawnId.serial)return false;
        input=in.epoch.input;id=in.epoch.inputId;
    }
    return Live(input,id)&&Read<void*>(c.pawn,0xba0)==input&&Read<void*>(input,0xc8)==c.pawn&&Read<void*>(input,0x148)==c.pawn;
}
inline void ReadResource(const fw::Player& p,const fw::Item& item,Sample& out){
    out.resourceAmbiguous=item.resources>1;if(item.resources!=1||!Live(item.resource,item.resourceId)||Read<void*>(item.resource,0xc8)!=item.native.actor)return;
    auto& r=out.resource;r.object=Label(item.resource,&item.resourceId);
    if(original_players::IsA(item.resource,0x68d2fe0)){
        r.kind=3;
        if(state.ultimateSchema&&Read<void*>(item.resource,0x148)==p.context.pawn&&Read<void*>(item.resource,0x150)==p.context.playerState){
            r.ultimatePointsKnown=true;r.ultimatePoints=Read<int32_t>(p.context.playerState,0xbdc);
        }return;
    }
    if(!original_players::IsA(item.resource,local_server_resource_authority::ResourceClass)||!state.resourceSchema)return;
    r.kind=1;r.currentKnown=r.authorityKnown=true;r.current=Read<int32_t>(item.resource,0x10c);r.authority=Read<int32_t>(item.resource,0x110);
    if(original_players::IsA(item.resource,local_server_resource_authority::EquipmentChargeClass)&&state.chargeSchema){
        r.configuredMaximumKnown=true;r.configuredMaximum=Read<int32_t>(item.resource,0x188);
    }
    if(original_players::IsA(item.resource,local_server_resource_authority::SignatureClass)&&state.signatureSchema){
        r.kind=2;r.signatureKnown=true;r.signaturePerUse=Read<int32_t>(item.resource,0x1a8);r.signatureProgress=Read<int32_t>(item.resource,0x1ac);
    }
}
inline bool SampleImpl(void* pawn,uint32_t epoch,Sample& out){
    out={};out.atMs=GetTickCount64();out.host=state.host;out.epoch=epoch;out.stage="hero_skill_waiting_current_framework_human";
    const fw::Player* player=nullptr;
    for(const auto& p:fw::GetPlayers())if(p.context.pawn==pawn&&p.epoch==epoch&&!p.retired&&p.used){if(player)return false;player=&p;}
    if(!player||!Current(*player,pawn,epoch)||player->count>player->items.size())return false;
    const auto& p=*player;const auto& c=p.context;const auto registration=p.registration;
    out.hero=p.hero;out.frameworkRegistration=registration;out.pawn=Label(pawn,&c.pawnId);out.playerState=Label(c.playerState,&c.playerStateId);out.inventory=Label(c.inventory,&c.inventoryId);
    const auto selected=Read<void*>(c.inventory,0x280);out.ability.pointer=reinterpret_cast<uintptr_t>(selected);
    void* input=nullptr;Weak inputId{};
    if(BorrowInput(p,input,inputId)){
        out.input=Label(input,&inputId);out.inputKnown=true;out.acceptedInputIndex=Read<int32_t>(input,0x120c);
        out.primaryHeld=Read<uint8_t>(input,0x265);out.heldKnown=out.primaryHeld<=1;
    }
    const fw::Item* item=nullptr;
    for(size_t n=0;n<p.count;++n)if(p.items[n].native.actor==selected){if(item)return false;item=&p.items[n];}
    if(!item){
        if(out.inputKnown&&(!Live(input,inputId)||Read<void*>(pawn,0xba0)!=input||Read<void*>(input,0xc8)!=pawn
           ||Read<int32_t>(input,0x120c)!=out.acceptedInputIndex||Read<uint8_t>(input,0x265)!=out.primaryHeld))return false;
        out.valid=Current(p,pawn,epoch)&&p.registration==registration&&Read<void*>(c.inventory,0x280)==selected;out.stage="hero_skill_selected_item_not_in_authored_framework";return out.valid;
    }
    if(!ItemCurrent(p,*item))return false;
    const auto& n=item->native;out.ability=Label(n.actor,&n.actorId);out.selectedAuthored=true;out.slot=n.slotType;out.role=Read<uint8_t>(n.actor,0x224);
    if(n.componentCount>n.components.size())return false;
    for(size_t k=0;k<n.componentCount;++k){auto machine=n.components[k];
        if(!Live(machine,n.componentIds[k])||Read<void*>(machine,0xc8)!=n.actor)return false;
        if(!original_players::IsA(machine,0x67722c0))continue;
        ++out.observedMachineCount;if(out.machineCount==MaxMachines){out.machineOverflow=true;continue;}
        auto& m=out.machines[out.machineCount++];m.object=Label(machine,&n.componentIds[k]);
        m.enabled=Read<uint8_t>(machine,0x348)!=0;
        const auto current=Read<void*>(machine,state.host?0x1c8:0x1b0);
        if(current&&Read<void*>(current,0xc8)==n.actor&&Read<void*>(current,0xe0)==machine){
            const Weak* borrowed=nullptr;if(n.selectedState==current&&Live(current,n.selectedStateId))borrowed=&n.selectedStateId;
            m.current=Label(current,borrowed);m.currentKnown=m.links=true;
        }
        m.subframe=Read<float>(machine,0x1e0);m.subframeKnown=std::isfinite(m.subframe)&&m.subframe>=0;
        const auto pending=Read<NativeArray>(machine,0x380);m.pendingCount=pending.count;m.pendingCapacity=pending.capacity;
        m.pendingKnown=QueueShape(reinterpret_cast<uintptr_t>(pending.data),pending.count,pending.capacity);
        if(m.pendingKnown&&pending.count){m.pendingFirstRaw=Read<uint64_t>(pending.data);m.pendingFirstKnown=true;}
        m.valid=Live(machine,n.componentIds[k])&&Read<void*>(machine,0xc8)==n.actor
            &&Read<void*>(machine,state.host?0x1c8:0x1b0)==current;
        if(m.currentKnown)m.valid=m.valid&&Read<void*>(current,0xc8)==n.actor&&Read<void*>(current,0xe0)==machine;
        if(!m.valid)return false;
    }
    ReadResource(p,*item,out);
    if(out.resource.object.pointer&&(!Live(item->resource,item->resourceId)||Read<void*>(item->resource,0xc8)!=n.actor))return false;
    if(out.inputKnown&&(!Live(input,inputId)||Read<void*>(pawn,0xba0)!=input||Read<void*>(input,0xc8)!=pawn
       ||Read<int32_t>(input,0x120c)!=out.acceptedInputIndex||Read<uint8_t>(input,0x265)!=out.primaryHeld))return false;
    out.valid=Current(p,pawn,epoch)&&p.registration==registration&&ItemCurrent(p,*item);
    out.stage=out.valid?"hero_skill_actual_selected_native_snapshot":"hero_skill_tuple_changed_during_sample";return out.valid;
}
inline LONG Fault(EXCEPTION_POINTERS* error){
    ++state.faults;state.lastException=error&&error->ExceptionRecord?error->ExceptionRecord->ExceptionCode:0;
    state.stage="hero_skill_diagnostic_sample_fault_no_game_error";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool GuardedVerify(){__try{return Verify();}__except(Fault(GetExceptionInformation())){return false;}}
inline bool GuardedSample(void* pawn,uint32_t epoch,Sample& out){
    __try{return SampleImpl(pawn,epoch,out);}__except(Fault(GetExceptionInformation())){out.valid=false;out.stage="hero_skill_diagnostic_sample_fault_no_game_error";return false;}
}
inline void Publish(){
    auto next=Format(latest,history,state.stage,state.initialized&&!state.closed,state.host,state.profiles,state.schema,state.polls,state.faults,state.lastException);
    next.pop_back();Print(next,",\"native_name_format_calls\":%llu,\"field_schema\":[",static_cast<unsigned long long>(state.nameFormats));
    for(size_t n=0;n<fieldCount;++n){if(n)Add(next,",");const auto& f=fields[n];Add(next,"{\"name\":");String(next,f.name);
        Print(next,",\"valid\":%s,\"expected_offset\":%d,\"actual_offset\":%d,\"expected_size\":%d,\"actual_size\":%d,\"expected_dim\":%d,\"actual_dim\":%d}",
            Bool(f.valid),f.expectedOffset,f.actualOffset,f.expectedSize,f.actualSize,f.expectedDim,f.actualDim);}
    Print(next,"],\"state_getter_metadata\":{\"valid\":%s,\"function\":\"%llx\",\"exec\":\"%llx\",\"owner\":\"%llx\",\"flags\":%u,\"parameter_bytes\":%u}",
        Bool(state.getterMetadata),static_cast<unsigned long long>(state.getterFunction),static_cast<unsigned long long>(state.getterExec),static_cast<unsigned long long>(state.getterOwner),state.getterFlags,unsigned(state.getterBytes));
    Print(next,",\"shared_profiles_verified_locally\":%s",Bool(state.sharedOwnerProfiles));
    Add(next,",\"state_struct_complete_layout_known\":false,\"state_struct_first_pointer_only\":true,\"shared_item_ownership_profiles_required\":true}");
    AcquireSRWLockExclusive(&lock);json.swap(next);ReleaseSRWLockExclusive(&lock);
}
inline bool Init(void* world,bool host){
    if(!original_players::Thread())return false;
    if(state.initialized)return state.world==world&&state.host==host&&!state.closed;
    const auto& f=fw::GetState();state.world=world;state.host=host;state.thread=GetCurrentThreadId();state.worldId=f.worldId;
    state.initialized=f.initialized&&f.world==world&&f.host==host;
    if(!state.initialized){state.stage="hero_skill_framework_world_not_ready";Publish();return false;}
    const bool ok=GuardedVerify();if(ok&&!Live(world,state.worldId))state.schema=false;
    state.stage=ok&&state.schema?"hero_skill_readback_ready_not_execution_proof":"hero_skill_readback_profile_or_schema_unknown";Publish();return true;
}
inline void Poll(void* pawn,uint32_t epoch){
    if(!GT()||state.busy)return;const auto now=GetTickCount64();if(state.lastAt&&now>=state.lastAt&&now-state.lastAt<25)return;
    state.lastAt=now;++state.polls;
    if(!state.profiles||!state.schema){latest={};latest.atMs=now;latest.host=state.host;latest.stage="hero_skill_readback_profile_or_schema_unknown";state.stage=latest.stage;Publish();return;}
    state.busy=true;Sample next{};
    const bool valid=state.profiles&&state.schema&&GuardedSample(pawn,epoch,next);state.busy=false;
    if(!valid){const auto stage=next.stage;Invalidate(next,stage,now,state.host,epoch);}
    next.valid=valid;next.serial=++state.serial;latest=next;Remember(history,latest);state.stage=latest.stage;Publish();
}
inline void Retire(void* pawn,uint32_t epoch){
    if(!GT())return;if(RetireSample(latest,reinterpret_cast<uintptr_t>(pawn),epoch)){state.stage=latest.stage;Publish();}
}
inline void Close(){
    if(!state.initialized||GetCurrentThreadId()!=state.thread)return;
    state.closed=true;latest.valid=false;latest.stage="hero_skill_readback_closed";state.stage=latest.stage;Publish();
}
inline std::string Json(){AcquireSRWLockShared(&lock);const auto result=json;ReleaseSRWLockShared(&lock);return result;}
} // namespace local_server_hero_skill_readback
