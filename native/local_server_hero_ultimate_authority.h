#pragma once
#include "local_server_hero_abilities.h"
#include "local_server_player_state_authority.h"
#include <array>
#include <cstdio>

// Actual original PS requests only. No resource-amount-to-points conversion,
// generated request, new RPC, client receipt or cost/grant policy is supplied.
// Evidence: work/local-server-playable/hero-ultimate-authority/review-source/.
namespace local_server_hero_ultimate_authority {
namespace framework=local_server_hero_abilities;
namespace ps=local_server_player_state_authority;
using Source=framework::SourceView;
using Weak=original_inventory_slots::WeakIdentity;
using Exec=void(*)(void*,void*,void*);
inline constexpr uintptr_t UltPointsClass=0x68d2fe0;
inline constexpr size_t MaxHumans=32,QueueCapacity=32;
struct Hooks {bool (*resolve)(void*,Source&)=nullptr;bool (*validate)(const Source&)=nullptr;};
enum class Kind:uint8_t {Consume,Active};
struct Function {
    void *object=nullptr,*parameter=nullptr;Weak identity{};uint32_t flags=0;
    original_jett_priming::PropertyInfo info{};bool installed=false;
};
struct Receipt {
    bool valid=false,originalCalled=false,frameEvaluated=false,frameFinished=false;
    Kind kind=Kind::Consume;uint64_t id=0,policy=0,humanRegistration=0,psRegistration=0;
    uint64_t originalPointsVersion=0,originalActiveVersion=0,expectedPointsVersion=0,expectedActiveVersion=0,dependency=0;
    int32_t beforePoints=0,afterOriginalPoints=0,expectedPoints=0;bool beforeActive=false,afterOriginalActive=false,expectedActive=false,desiredActive=false;
    uint32_t scratch=0;Source source{};void *resource=nullptr,*slot=nullptr,*function=nullptr;Weak resourceId{},slotId{},functionId{};uint8_t slotType=255,route=0;
    ps::Publication publication{};const char* stage="ultimate_request_not_applied";
};
struct Human {
    bool used=false,retired=false,sourceReady=false,draining=false;
    void *world=nullptr,*controller=nullptr,*playerState=nullptr,*pawn=nullptr,*inventory=nullptr;
    std::array<Weak,5> identities{};unsigned hero=0;uint32_t epoch=0,activeFrames=0;
    uint64_t registration=0,frameworkRegistration=0;
    Source source{};void *resource=nullptr,*slot=nullptr;Weak resourceId{},slotId{};uint8_t slotType=255;
    std::array<Receipt,QueueCapacity> queue{};size_t head=0,count=0;Receipt last{};
};
struct State {
    bool initialized=false,installed=false,closed=false,enabled=false,practice=false,profiles=false,schema=false,restartRequired=false;
    void* world=nullptr;Weak worldId{};DWORD gameThread=0;unsigned error=0,active=0;
    uint64_t policy=1,registrations=0,requests=0,forwards=0,originalConsumes=0,boolEvaluations=0,scriptSteps=0,compiledSteps=0;
    uint64_t completedFrames=0,queued=0,waiting=0,completed=0,effectiveConsumes=0,activeChanges=0;
    uint64_t zeroConsumes=0,changedOriginal=0,cancelled=0,sourceDeferrals=0,retired=0,closes=0,partialFailures=0;
    const char *operation="none",*stage="hero_ultimate_not_initialized";
};
struct HumanDiagnostic {
    void *pawn=nullptr,*playerState=nullptr,*resource=nullptr,*slot=nullptr;uint32_t epoch=0;unsigned hero=0;
    uint64_t registration=0;bool used=false,retired=false,sourceReady=false,draining=false;size_t queued=0;
    Receipt head{},last{};
};
struct Diagnostics {State summary{};std::array<HumanDiagnostic,MaxHumans> humans{};};
struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr Profile Profiles[]{
    {0x3b866f0,20,0x09fe87de40670e75ULL},{0x36ab340,204,0xf250d2336a5eef37ULL},
    {0x1c77e20,158,0xc70a77070e050422ULL},
    {0x1c6e7d0,190,0x8bbc6c96ce3ae60eULL},
    {0x6f7e3e0,71,0x5636022b717c0729ULL},{0x6f7e430,70,0xe366246d57c069a8ULL}
};
inline State state{};inline Hooks hooks{};inline std::array<Function,3> functions{};
inline std::array<Human,MaxHumans> humans{};inline Diagnostics cached{};inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline const State& GetState(){return state;}
template<class T>inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Capture(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline bool GT(){return state.initialized&&GetCurrentThreadId()==state.gameThread&&original_players::Thread();}
inline void Cache(){
    Diagnostics d;d.summary=state;
    for(size_t n=0;n<humans.size();++n){const auto& h=humans[n];auto& out=d.humans[n];
        out.pawn=h.pawn;out.playerState=h.playerState;out.resource=h.resource;out.slot=h.slot;out.epoch=h.epoch;out.hero=h.hero;
        out.registration=h.registration;out.used=h.used;out.retired=h.retired;out.sourceReady=h.sourceReady;out.draining=h.draining;
        out.queued=h.count;out.last=h.last;if(h.count)out.head=h.queue[h.head];}
    AcquireSRWLockExclusive(&cacheLock);cached=d;ReleaseSRWLockExclusive(&cacheLock);
}
inline bool Fail(unsigned code,const char* why){state.error=code;state.stage=why;Cache();return false;}
__declspec(noreturn) inline void Fatal(unsigned code,const char* why){
    state.restartRequired=true;++state.partialFailures;Fail(code,why);
    // A changed FFrame cannot be reparsed or normally resumed. The owning
    // bridge's ordinary native exception boundary must end this owned session.
    RaiseException(0xe0008700U,EXCEPTION_NONCONTINUABLE,0,nullptr);
    TerminateProcess(GetCurrentProcess(),0xe0008700U);__assume(0);
}
inline bool Writable(const void* pointer,size_t bytes){
    const auto first=reinterpret_cast<uintptr_t>(pointer);if(!first||!bytes||bytes>UINTPTR_MAX-first)return false;
    uintptr_t at=first;const auto end=first+bytes;
    while(at<end){MEMORY_BASIC_INFORMATION q{};
        if(!VirtualQuery(reinterpret_cast<void*>(at),&q,sizeof(q))||q.State!=MEM_COMMIT||q.Type!=MEM_PRIVATE
           ||(q.Protect&(PAGE_GUARD|PAGE_NOACCESS))||(q.Protect&0xffU)!=PAGE_READWRITE)return false;
        const auto next=reinterpret_cast<uintptr_t>(q.BaseAddress)+q.RegionSize;if(next<=at)return false;at=next;}
    return true;
}
inline bool ImageCode(void* p){
    const auto address=reinterpret_cast<uintptr_t>(p);const auto pe=Read<uint32_t>(reinterpret_cast<void*>(engine::image),0x3c);
    const auto size=pe<0x1000?Read<uint32_t>(reinterpret_cast<void*>(engine::image),pe+0x50):0;
    if(!size||address<engine::image||address-engine::image>=size)return false;
    MEMORY_BASIC_INFORMATION q{};return VirtualQuery(p,&q,sizeof(q))&&q.State==MEM_COMMIT
        &&!(q.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&(q.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY));
}
inline Exec Original(size_t index){return engine::function<Exec>(index==0?0x3b866f0:0x36ab340);}
inline Exec Replacement(size_t index);
inline bool Owns(size_t index){const auto& f=functions[index];
    return f.installed&&Live(f.object,f.identity)&&Read<Exec>(f.object,0xe0)==Replacement(index)
        &&Read<uint32_t>(f.object,0xb8)==f.flags&&Read<void*>(f.object,0x28)==engine::Class(original_players::ShooterStateClass)
        &&Read<uint16_t>(f.object,0xbe)==(index?1:0);
}
inline bool SameSource(const Source& a,const Source& b){
    if(a.contextObject!=b.contextObject||a.world!=b.world||a.controller!=b.controller||a.playerState!=b.playerState
       ||a.pawn!=b.pawn||a.inventory!=b.inventory||a.ability!=b.ability||a.heroIndex!=b.heroIndex
       ||a.sourceEpoch!=b.sourceEpoch||a.registration!=b.registration||a.pendingGrant!=b.pendingGrant)return false;
    for(size_t n=0;n<a.identities.size();++n)if(!original_inventory_slots::SameIdentity(a.identities[n],b.identities[n]))return false;
    return true;
}
inline bool Base(const Human& h){
    if(!GT()||!h.used||h.retired||!h.hero||h.hero>=original_heroes::PlayableHeroCount||!h.epoch||state.closed||state.error||state.restartRequired
       ||!Live(state.world,state.worldId))return false;
    const std::array<void*,5> objects{h.world,h.controller,h.playerState,h.pawn,h.inventory};
    for(size_t n=0;n<objects.size();++n)if(!Live(objects[n],h.identities[n]))return false;
    for(const auto& p:framework::GetPlayers())if(p.used&&!p.retired&&p.authority&&!p.error&&p.hero==h.hero&&p.epoch==h.epoch
        &&p.registration==h.frameworkRegistration&&p.context.world==h.world&&p.context.controller==h.controller
        &&p.context.playerState==h.playerState&&p.context.pawn==h.pawn&&p.context.inventory==h.inventory){
        void* connection=nullptr;
        return h.world==state.world&&local_server_item_ownership::ActualHuman(h.world,h.controller,h.playerState,h.pawn,h.inventory,connection)
            &&original_players::Registered(h.playerState)&&Read<void*>(h.playerState,0xbc8)==h.pawn;}
    return false;
}
inline bool SourceValid(const Human& h,const Source& source,void* resource,void* slot,const Weak& resourceId,const Weak& slotId,uint8_t slotType){
    if(!Base(h)||!state.enabled||!state.practice||!hooks.resolve||!hooks.validate||source.pendingGrant||!source.ability
       ||source.world!=h.world||source.controller!=h.controller||source.playerState!=h.playerState||source.pawn!=h.pawn
       ||source.inventory!=h.inventory||source.heroIndex!=h.hero||source.sourceEpoch!=h.epoch||source.registration!=h.frameworkRegistration)return false;
    const std::array<void*,7> objects{source.contextObject,source.world,source.controller,source.playerState,source.pawn,source.inventory,source.ability};
    for(size_t n=0;n<objects.size();++n)if(!Live(objects[n],source.identities[n]))return false;
    Source current{};
    return Live(resource,resourceId)&&Live(slot,slotId)&&source.contextObject==resource
        &&hooks.resolve(resource,current)&&SameSource(source,current)&&framework::ValidateSource(source)&&hooks.validate(source)
        &&slotType<17&&Read<uint8_t>(source.ability,0x4a0)==slotType
        &&Read<void*>(source.inventory,0x1b8+size_t(slotType)*8)==slot&&local_server_item_ownership::ValidSlot(source.inventory,slot,slotType)
        &&local_server_item_ownership::Contains(slot,source.ability)&&Read<void*>(resource,0xc8)==source.ability
        &&original_weapon::Component(source.ability,local_server_resource_authority::ResourceClass)==resource
        &&original_players::IsA(resource,UltPointsClass)&&(Read<uint8_t>(resource,0xc2)&0x40)!=0
        &&Read<void*>(resource,0x148)==h.pawn&&Read<void*>(resource,0x150)==h.playerState
        &&Read<uintptr_t>(Read<void*>(resource),0x4d0)==engine::image+0x6f7e3e0
        &&Read<uintptr_t>(Read<void*>(resource),0x4e0)==engine::image+0x6f7e430;
}
inline ps::Source PSSource(const Human& h){return {h.world,h.controller,h.playerState,h.pawn,h.epoch};}
inline bool Version(const Human& h,ps::Observation& out){return ps::Readback(PSSource(h),out)&&out.registered&&out.sourceValid&&!out.retired;}
inline Human* Find(void* pawn,uint32_t epoch){for(auto& h:humans)if(h.used&&!h.retired&&h.pawn==pawn&&h.epoch==epoch)return &h;return nullptr;}
inline Human* FindReceipt(const Receipt& receipt){for(auto& h:humans)if(h.used&&!h.retired&&h.registration==receipt.humanRegistration&&h.pawn==receipt.source.pawn&&h.epoch==receipt.source.sourceEpoch)return &h;return nullptr;}
inline bool BindSource(Human& h){
    h.sourceReady=false;if(!Base(h)||!state.enabled||!state.practice)return true;
    Source selected{};void *resource=nullptr,*slot=nullptr;Weak resourceId{},slotId{};uint8_t slotType=255;unsigned matches=0;
    for(const auto& p:framework::GetPlayers())if(p.used&&!p.retired&&p.authority&&p.context.pawn==h.pawn&&p.epoch==h.epoch&&p.registration==h.frameworkRegistration){
        for(size_t n=0;n<p.count;++n){const auto& item=p.items[n];const auto& native=item.native;
            if(item.equipmentManaged||!native.published||native.slotType>=17||!Live(native.actor,native.actorId)||!Live(native.slot,native.slotId))continue;
            auto candidate=original_weapon::Component(native.actor,UltPointsClass);if(!candidate)continue;
            Source view{};Weak candidateId{};
            if(!Capture(candidate,candidateId)||!hooks.resolve(candidate,view)
               ||!SourceValid(h,view,candidate,native.slot,candidateId,native.slotId,native.slotType))continue;
            ++matches;selected=view;resource=candidate;slot=native.slot;resourceId=candidateId;slotId=native.slotId;slotType=native.slotType;}}
    if(matches!=1){++state.sourceDeferrals;return true;}
    if(!ps::Register(PSSource(h)))return Fail(ps::GetState().error?ps::GetState().error:8701,ps::GetState().stage);
    h.source=selected;h.resource=resource;h.slot=slot;h.resourceId=resourceId;h.slotId=slotId;h.slotType=slotType;h.sourceReady=true;return true;
}
inline bool Eligible(Human& h){return h.sourceReady&&SourceValid(h,h.source,h.resource,h.slot,h.resourceId,h.slotId,h.slotType);}
inline Human* Target(void* psObject){
    if(!GT()||!state.installed||state.closed||!state.enabled||!state.practice||state.error||state.restartRequired)return nullptr;
    for(auto& h:humans)if(h.used&&!h.retired&&h.playerState==psObject&&Eligible(h))return &h;return nullptr;
}
struct FramePlan {bool script=false;uint8_t* code=nullptr;void *field=nullptr,*locals=nullptr,*handler=nullptr;int32_t offset=0;};
inline bool Plan(size_t index,void* frame,FramePlan& out){
    out={};if(!index||!Owns(index)||!Writable(frame,0x90))return false;
    const auto& f=functions[index];uint8_t* code=nullptr;
    if(!original_players::Read(frame,0x20,code))return false;
    if(code){uint8_t opcode=0;if(!original_players::Read(code,0,opcode))return false;
        auto handler=Read<void*>(reinterpret_cast<void*>(engine::image+0xd747c40),size_t(opcode)*8);
        if(!ImageCode(handler))return false;out.script=true;out.code=code;out.handler=handler;return true;}
    auto field=Read<void*>(frame,0x88);auto locals=Read<void*>(frame,0x28);
    if(field!=f.parameter||Read<int32_t>(field,0x44)!=0||Read<int32_t>(field,0x30)!=1||Read<int32_t>(field,0x34)!=1
       ||(Read<uint64_t>(field,0x38)&0x180ULL)!=0x80ULL||!original_players::Readable(locals,1)
       ||Read<uintptr_t>(Read<void*>(field),0x110)!=engine::image+0x1c6e7d0)return false;
    out.field=field;out.locals=locals;out.offset=0;return true;
}
struct NativeFrameOps {
    static void BoolClass(){engine::function<void*(*)()>(0x1c77e20)();}
    template<class T>static T Load(const void* p,size_t offset){T out{};if(!original_players::Read(p,offset,out))Fatal(8702,"hero_ultimate_original_frame_read_failed_after_admission");return out;}
    template<class T>static void Store(void* p,size_t offset,T value){if(!original_players::Write(p,offset,value))Fatal(8703,"hero_ultimate_original_frame_write_failed_requires_new_session");}
    static void Dispatch(void* function,void* object,void* frame,void* result){reinterpret_cast<Exec>(function)(object,frame,result);}
    static void* ScriptHandler(uint8_t opcode){return Read<void*>(reinterpret_cast<void*>(engine::image+0xd747c40),size_t(opcode)*8);}
    static void Copy(void* field,void* destination,const void* locals){reinterpret_cast<void(*)(void*,void*,const void*)>(Read<void*>(Read<void*>(field),0x110))(field,destination,locals);}
};
template<class Ops>inline uint32_t EvaluateBool(void* frame,const FramePlan& plan){
    // Exact36AB340 admitted branches. The scratch is a DWORD, not one byte.
    uint32_t scratch=0;Ops::BoolClass();
    if(plan.script){auto code=Ops::template Load<uint8_t*>(frame,0x20);
        if(code!=plan.code)Fatal(8704,"hero_ultimate_original_code_changed_before_single_evaluation");
        const auto target=Ops::ScriptHandler(Ops::template Load<uint8_t>(code,0));
        if(target!=plan.handler)Fatal(8704,"hero_ultimate_original_opcode_target_changed_before_single_evaluation");
        Ops::Store(frame,0x20,code+1);
        Ops::Dispatch(target,Ops::template Load<void*>(frame,0x18),frame,&scratch);
    }else{
        if(Ops::template Load<void*>(frame,0x88)!=plan.field)Fatal(8705,"hero_ultimate_original_parameter_chain_changed_before_single_evaluation");
        if(Ops::template Load<void*>(frame,0x28)!=plan.locals||Ops::template Load<int32_t>(plan.field,0x44)!=plan.offset)
            Fatal(8705,"hero_ultimate_original_locals_changed_before_single_evaluation");
        Ops::Store(frame,0x88,Ops::template Load<void*>(plan.field,0x18));
        Ops::Store(frame,0x40,plan.locals);Ops::Store(frame,0x38,static_cast<uint8_t*>(plan.locals)+plan.offset);
        // Original v110 receives the LOCALS CONTAINER, not locals+offset.
        Ops::Copy(plan.field,&scratch,plan.locals);
    }
    auto end=Ops::template Load<uint8_t*>(frame,0x20);
    Ops::Store(frame,0x20,end?end+1:nullptr); // original EndFrame, no0x16 test
    return scratch;
}
inline bool Snapshot(Human& h,Receipt& out){
    ps::Observation observed;if(!Eligible(h)||!Version(h,observed))return false;
    out={};out.valid=true;out.policy=state.policy;out.humanRegistration=h.registration;out.psRegistration=observed.registration;
    out.source=h.source;out.resource=h.resource;out.slot=h.slot;out.resourceId=h.resourceId;out.slotId=h.slotId;out.slotType=h.slotType;
    out.beforePoints=out.afterOriginalPoints=out.expectedPoints=observed.points;
    out.beforeActive=out.afterOriginalActive=out.expectedActive=observed.active;
    out.originalPointsVersion=out.expectedPointsVersion=observed.fields[0].writes;
    out.originalActiveVersion=out.expectedActiveVersion=observed.fields[1].writes;return true;
}
inline bool SameOrigin(Human& h,const Receipt& receipt){
    return receipt.valid&&receipt.policy==state.policy&&h.registration==receipt.humanRegistration
        &&receipt.route<functions.size()&&Owns(receipt.route)&&functions[receipt.route].object==receipt.function
        &&original_inventory_slots::SameIdentity(functions[receipt.route].identity,receipt.functionId)
        &&SourceValid(h,receipt.source,receipt.resource,receipt.slot,receipt.resourceId,receipt.slotId,receipt.slotType);
}
inline bool Queue(Human& h,Receipt receipt){
    if(state.requests==UINT64_MAX||h.count==QueueCapacity)Fatal(8706,"hero_ultimate_observed_request_receipt_capacity_exhausted");
    receipt.id=++state.requests;receipt.stage="hero_ultimate_original_request_observed_waiting_typed_publication";
    h.queue[(h.head+h.count)%QueueCapacity]=receipt;++h.count;++state.queued;h.last=receipt;return true;
}
inline void Pop(Human& h,Receipt receipt){h.last=receipt;h.queue[h.head]={};h.head=(h.head+1)%QueueCapacity;--h.count;}
inline void AdvanceDependencies(Human& h,const Receipt& applied,const ps::Observation& after){
    for(size_t n=1;n<h.count;++n){auto& later=h.queue[(h.head+n)%QueueCapacity];
        if(!later.valid||later.id<=applied.id||later.policy!=applied.policy||later.psRegistration!=applied.psRegistration
           ||!SameSource(later.source,applied.source))continue;
        if(later.expectedPointsVersion==applied.expectedPointsVersion&&after.fields[0].writes!=applied.expectedPointsVersion){
            later.expectedPointsVersion=after.fields[0].writes;later.expectedPoints=after.points;later.dependency=applied.id;}
        if(later.expectedActiveVersion==applied.expectedActiveVersion&&after.fields[1].writes!=applied.expectedActiveVersion){
            later.expectedActiveVersion=after.fields[1].writes;later.expectedActive=after.active;later.dependency=applied.id;}}
}
inline bool Drain(Human& h){
    if(h.draining||!h.used||h.retired)return true;h.draining=true;
    __try{
        for(unsigned budget=0;budget<8&&h.count;++budget){auto receipt=h.queue[h.head];ps::Observation before;
            if(!SameOrigin(h,receipt)||!Version(h,before)||before.registration!=receipt.psRegistration
               ||before.fields[0].writes!=receipt.expectedPointsVersion||before.fields[1].writes!=receipt.expectedActiveVersion
               ||before.points!=receipt.expectedPoints||before.active!=receipt.expectedActive){
                receipt.stage="hero_ultimate_old_request_source_or_typed_write_version_changed_no_replay";++state.cancelled;Pop(h,receipt);continue;}
            receipt.publication=receipt.kind==Kind::Consume?ps::SetPoints(PSSource(h),0):ps::SetActive(PSSource(h),receipt.desiredActive);
            if(receipt.publication.status==ps::Status::Failed){h.last=receipt;Fatal(receipt.publication.error?receipt.publication.error:8707,ps::GetState().stage);}
            if(!receipt.publication.Ready()){
                if(receipt.publication.recordWritten||receipt.publication.notified||receipt.publication.nativePublished)
                    Fatal(8708,"hero_ultimate_waiting_publication_has_unproven_partial_effect");
                receipt.stage="hero_ultimate_observed_request_waiting_actual_native_ps_transaction";h.queue[h.head]=receipt;h.last=receipt;++state.waiting;break;}
            ps::Observation after;
            if(!SameOrigin(h,receipt)||!Version(h,after)||after.registration!=receipt.psRegistration
               ||(receipt.kind==Kind::Consume?after.points!=0:after.active!=receipt.desiredActive))
                Fatal(8709,"hero_ultimate_ready_publication_current_source_or_value_not_observed");
            const uint64_t pointWrites=receipt.kind==Kind::Consume&&receipt.publication.recordWritten?1U:0U;
            const uint64_t activeWrites=receipt.kind==Kind::Active&&receipt.publication.recordWritten?1U:0U;
            if(after.fields[0].writes!=receipt.expectedPointsVersion+pointWrites
               ||after.fields[1].writes!=receipt.expectedActiveVersion+activeWrites)
                Fatal(8709,"hero_ultimate_ready_publication_has_unattributed_typed_write_version_change");
            ++state.completed;if(receipt.kind==Kind::Consume&&receipt.publication.recordWritten)++state.effectiveConsumes;
            if(receipt.kind==Kind::Active&&receipt.publication.recordWritten)++state.activeChanges;
            receipt.stage="hero_ultimate_typed_ps_native_ready_remote_delivery_unverified";
            AdvanceDependencies(h,receipt,after);Pop(h,receipt);
        }
    }__finally{h.draining=false;}
    return true;
}
inline void ExecuteImpl(size_t index,void* object,void* frame,void* result){
    auto h=Target(object);Receipt origin;
    if(!h||!Owns(index)||!Snapshot(*h,origin)){++state.forwards;Original(index)(object,frame,result);return;}
    FramePlan plan;
    if(index&&!Plan(index,frame,plan)){++state.forwards;Original(index)(object,frame,result);return;}
    origin.route=static_cast<uint8_t>(index);origin.function=functions[index].object;origin.functionId=functions[index].identity;
    const auto registration=h->registration;++h->activeFrames;
    __try{
        if(index==0){
            origin.kind=Kind::Consume;Original(index)(object,frame,result);++state.originalConsumes;
            origin.originalCalled=true;origin.frameFinished=true;++state.completedFrames;
            ps::Observation after;
            if(!SameOrigin(*h,origin)||!Version(*h,after)||after.registration!=origin.psRegistration){++state.cancelled;return;}
            origin.afterOriginalPoints=after.points;origin.afterOriginalActive=after.active;
            if(after.points!=origin.beforePoints||after.fields[0].writes!=origin.originalPointsVersion){++state.changedOriginal;h->last=origin;return;}
            if(origin.beforePoints<=0){++state.zeroConsumes;h->last=origin;return;}
        }else{
            origin.kind=Kind::Active;origin.scratch=EvaluateBool<NativeFrameOps>(frame,plan);
            origin.frameEvaluated=origin.frameFinished=true;++state.boolEvaluations;++state.completedFrames;
            if(plan.script)++state.scriptSteps;else ++state.compiledSteps;
            // Argument evaluation can legitimately execute other requests.
            // Preserve the original source identity, then capture post-eval
            // typed versions, matching a setter body AFTER its arguments.
            ps::Observation after;
            if(!SameOrigin(*h,origin)||!Version(*h,after)||after.registration!=origin.psRegistration){++state.cancelled;return;}
            origin.afterOriginalPoints=origin.expectedPoints=after.points;origin.afterOriginalActive=origin.expectedActive=after.active;
            origin.originalPointsVersion=origin.expectedPointsVersion=after.fields[0].writes;
            origin.originalActiveVersion=origin.expectedActiveVersion=after.fields[1].writes;
            origin.desiredActive=origin.scratch!=0;
        }
        if(h->registration!=registration||!SameOrigin(*h,origin)||!Owns(index)){++state.cancelled;return;}
        Queue(*h,origin);Drain(*h);
    }__finally{if(h->registration==registration&&h->activeFrames)--h->activeFrames;}
}
inline void Execute(size_t index,void* object,void* frame,void* result){
    // Off-GT calls do not update counters, caches, queues or any shared state.
    // The original immutable native body is safe after our own bindings retire.
    if(GetCurrentThreadId()!=state.gameThread){Original(index)(object,frame,result);return;}
    ++state.active;__try{ExecuteImpl(index,object,frame,result);}__finally{--state.active;Cache();}
}
inline void ConsumeExec(void* o,void* f,void* r){Execute(0,o,f,r);}
inline void ActiveExec(void* o,void* f,void* r){Execute(1,o,f,r);}
inline void UltimateActiveExec(void* o,void* f,void* r){Execute(2,o,f,r);}
inline Exec Replacement(size_t index){return index==0?&ConsumeExec:index==1?&ActiveExec:&UltimateActiveExec;}
inline bool Verify(){
    if(state.profiles&&state.schema)return true;
    for(const auto& p:Profiles){auto code=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(code,p.bytes))return Fail(8710,"hero_ultimate_original_profile_unreadable");
        uint64_t digest=0xcbf29ce484222325ULL;for(size_t n=0;n<p.bytes;++n)digest=(digest^code[n])*0x100000001b3ULL;
        if(digest!=p.fnv)return Fail(8711,"hero_ultimate_original_profile_mismatch");}
    state.profiles=true;
    const wchar_t* names[]{L"/Script/ShooterGame.ShooterPlayerState.AuthConsumeUltimatePoints",L"/Script/ShooterGame.ShooterPlayerState.AuthSetUltActive",L"/Script/ShooterGame.ShooterPlayerState.AuthSetUltimateActive"};
    const wchar_t* params[]{nullptr,L"UltActive",L"bNewUltimateActive"};
    for(size_t n=0;n<functions.size();++n){auto& f=functions[n];f.object=engine::Find(names[n]);
        f.flags=Read<uint32_t>(f.object,0xb8);
        if(!Capture(f.object,f.identity)||Read<void*>(f.object,0x28)!=engine::Class(original_players::ShooterStateClass)
           ||Read<Exec>(f.object,0xe0)!=Original(n)||Read<uint16_t>(f.object,0xbe)!=(n?1:0)||(f.flags&0x400U)==0
           ||!Writable(static_cast<uint8_t*>(f.object)+0xe0,8))return Fail(8712,"hero_ultimate_original_native_ufunction_identity_or_metadata_mismatch");
        if(n){auto& p=f.info;
            if(original_jett_priming::FieldOffset(f.object,params[n],1,&p)!=0||p.arrayDim!=1||p.elementSize!=1
               ||(Read<uint64_t>(p.field,0x38)&0x580ULL)!=0x80ULL||p.boolFieldSize!=1||p.boolByteOffset!=0||p.boolByteMask!=1||p.boolFieldMask!=0xff
               ||Read<uintptr_t>(Read<void*>(p.field),0x110)!=engine::image+0x1c6e7d0)
                return Fail(8713,"hero_ultimate_original_native_bool_parameter_schema_mismatch");f.parameter=p.field;}}
    if(!framework::loadout::Field(engine::Class(UltPointsClass),L"OwningCharacter",0x148,8,1,0)
       ||!framework::loadout::Field(engine::Class(UltPointsClass),L"OwningPlayerState",0x150,8,1,0))
        return Fail(8714,"hero_ultimate_original_resource_owner_metadata_mismatch");
    state.schema=true;return true;
}
inline bool Init(void* world,const Hooks& value){
    if(state.initialized)return GT()&&state.installed&&!state.error&&!state.restartRequired&&state.world==world&&!state.closed
        &&hooks.resolve==value.resolve&&hooks.validate==value.validate;
    if(!original_players::Thread())return false; // no off-GT shared-state write
    if(!world||!engine::image||!value.resolve||!value.validate
       ||!framework::GetState().initialized||!framework::GetState().host||framework::GetState().world!=world
       ||!ps::GetState().initialized||ps::GetState().world!=world||ps::GetState().closed||ps::GetState().error||ps::GetState().restartRequired)
        return Fail(8715,"hero_ultimate_requires_actual_host_framework_ps_publisher_and_fenced_hooks");
    state.gameThread=GetCurrentThreadId();state.world=world;hooks=value;
    if(!Capture(world,state.worldId)||!Verify())return false;
    state.initialized=true;
    for(size_t n=0;n<functions.size();++n){auto& f=functions[n];
        if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(f.object)+0xe0),
            reinterpret_cast<void*>(Replacement(n)),reinterpret_cast<void*>(Original(n)))!=reinterpret_cast<void*>(Original(n)))
        {
            // Roll back only entries still owned by this incomplete install.
            // A foreign conflict is retained in the ledger for Close; it is
            // never overwritten and repeated Init cannot report success.
            for(size_t k=n;k>0;--k){auto& earlier=functions[k-1];if(!earlier.installed)continue;
                if(Live(earlier.object,earlier.identity)&&Writable(static_cast<uint8_t*>(earlier.object)+0xe0,8)
                   &&InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(earlier.object)+0xe0),
                     reinterpret_cast<void*>(Original(k-1)),reinterpret_cast<void*>(Replacement(k-1)))==reinterpret_cast<void*>(Replacement(k-1)))earlier.installed=false;}
            return Fail(8716,"hero_ultimate_original_native_exec_install_conflict");
        }f.installed=true;}
    state.installed=true;state.stage="hero_ultimate_original_requests_bound_policy_disabled";Cache();return true;
}
inline bool SetPolicy(bool enabled,bool practice){
    if(!GT()||state.closed||state.error)return false;
    if(state.enabled!=enabled||state.practice!=practice){if(state.policy==UINT64_MAX)return Fail(8717,"hero_ultimate_policy_generation_exhausted");
        ++state.policy;for(auto& h:humans){state.cancelled+=h.count;h.head=h.count=0;h.sourceReady=false;}}
    state.enabled=enabled;state.practice=practice;state.stage=enabled&&practice?"hero_ultimate_nonjett_practice_policy_enabled":"hero_ultimate_original_forward_policy";Cache();return true;
}
inline bool RegisterHuman(void* pawn,uint32_t epoch){
    if(!GT()||!pawn||!epoch||state.closed||state.error)return false;
    if(Find(pawn,epoch))return true;
    for(const auto& p:framework::GetPlayers())if(p.used&&!p.retired&&p.authority&&!p.error&&p.registration&&p.context.sourceReady
        &&p.context.epoch==p.epoch&&p.context.pawn==pawn&&p.epoch==epoch){
        if(!p.hero)return true;
        Human* selected=nullptr;for(auto& h:humans)if(!h.used&&!h.activeFrames&&!h.draining){selected=&h;break;}
        if(!selected||state.registrations==UINT64_MAX)return Fail(8718,"hero_ultimate_human_registration_capacity_exhausted");
        auto& h=*selected;h={};const auto& c=p.context;
        h.world=c.world;h.controller=c.controller;h.playerState=c.playerState;h.pawn=c.pawn;h.inventory=c.inventory;
        h.hero=p.hero;h.epoch=epoch;h.frameworkRegistration=p.registration;h.registration=++state.registrations;h.used=true;
        const std::array<void*,5> objects{h.world,h.controller,h.playerState,h.pawn,h.inventory};
        for(size_t n=0;n<objects.size();++n)if(!Capture(objects[n],h.identities[n]))return Fail(8719,"hero_ultimate_human_original_weak_identity_missing");
        if(!Base(h))return Fail(8720,"hero_ultimate_requires_current_registered_actual_human");
        state.stage="hero_ultimate_human_registered_resource_source_pending";Cache();return true;}
    return Fail(8721,"hero_ultimate_framework_human_epoch_not_registered");
}
inline bool Poll(){
    if(!GT()||state.closed||state.error||state.restartRequired)return false;
    for(size_t n=0;n<functions.size();++n)if(!Owns(n))return Fail(8722,"hero_ultimate_owned_original_native_binding_changed");
    for(auto& h:humans)if(h.used&&!h.retired){if(!BindSource(h)||!Drain(h))return false;}
    Cache();return true;
}
inline bool Retire(void* pawn,uint32_t epoch){
    if(!GT()||state.closed)return false;auto h=Find(pawn,epoch);if(!h)return true;
    // Called AFTER coordinator admission is fenced. Only our stored generation
    // is retired; no new source admission or native write is attempted.
    state.cancelled+=h->count;h->head=h->count=0;h->sourceReady=false;h->retired=true;h->used=false;++state.retired;Cache();return true;
}
inline bool Close(){
    if(!state.initialized||state.closed)return true;if(!GT())return false;
    if(state.active)return Fail(8723,"hero_ultimate_close_requires_idle_original_game_thread");
    for(const auto& h:humans)if(h.used||h.activeFrames||h.draining)return Fail(8724,"hero_ultimate_close_requires_own_sources_retired");
    for(size_t n=functions.size();n>0;--n){auto& f=functions[n-1];if(!f.installed)continue;
        if(!Owns(n-1)||!Writable(static_cast<uint8_t*>(f.object)+0xe0,8)
           ||InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(f.object)+0xe0),
             reinterpret_cast<void*>(Original(n-1)),reinterpret_cast<void*>(Replacement(n-1)))!=reinterpret_cast<void*>(Replacement(n-1)))
            return Fail(8725,"hero_ultimate_original_native_exec_restore_conflict");f.installed=false;}
    state.installed=false;state.closed=true;++state.closes;state.stage="hero_ultimate_original_execs_restored_pending_receipts_retired";Cache();return true;
}
inline void WriteReceipt(FILE* f,const Receipt& r){
    auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"id\":%llu,\"kind\":\"%s\",\"stage\":\"%s\",\"valid\":%s,\"original_called\":%s,\"frame_evaluated\":%s,\"frame_finished\":%s,\"before_points\":%d,\"after_original_points\":%d,\"scratch_uint32\":%u,\"desired_active\":%s,\"policy\":%llu,\"human_registration\":%llu,\"ps_registration\":%llu,\"original_points_version\":%llu,\"expected_points_version\":%llu,\"original_active_version\":%llu,\"expected_active_version\":%llu,\"dependency_request\":%llu,\"publication\":",
        static_cast<unsigned long long>(r.id),r.kind==Kind::Consume?"consume":"active",r.stage,b(r.valid),b(r.originalCalled),b(r.frameEvaluated),b(r.frameFinished),r.beforePoints,r.afterOriginalPoints,r.scratch,b(r.desiredActive),static_cast<unsigned long long>(r.policy),static_cast<unsigned long long>(r.humanRegistration),static_cast<unsigned long long>(r.psRegistration),static_cast<unsigned long long>(r.originalPointsVersion),static_cast<unsigned long long>(r.expectedPointsVersion),static_cast<unsigned long long>(r.originalActiveVersion),static_cast<unsigned long long>(r.expectedActiveVersion),static_cast<unsigned long long>(r.dependency));
    ps::WritePublication(f,r.publication);
    std::fprintf(f,",\"route\":%u,\"function\":\"%p\",\"function_weak_index\":%d,\"function_weak_serial\":%d,\"expected_points\":%d,\"before_active\":%s,\"after_original_active\":%s,\"expected_active\":%s,\"resource\":\"%p\",\"slot\":\"%p\",\"slot_type\":%u,\"source\":{\"context\":\"%p\",\"world\":\"%p\",\"controller\":\"%p\",\"player_state\":\"%p\",\"pawn\":\"%p\",\"inventory\":\"%p\",\"ability\":\"%p\",\"hero\":%u,\"epoch\":%u,\"registration\":%llu,\"pending_grant\":%s,\"weak_identities\":[",
        r.route,r.function,r.functionId.index,r.functionId.serial,r.expectedPoints,b(r.beforeActive),b(r.afterOriginalActive),b(r.expectedActive),r.resource,r.slot,r.slotType,r.source.contextObject,r.source.world,r.source.controller,r.source.playerState,r.source.pawn,r.source.inventory,r.source.ability,r.source.heroIndex,r.source.sourceEpoch,static_cast<unsigned long long>(r.source.registration),b(r.source.pendingGrant));
    for(size_t n=0;n<r.source.identities.size();++n)std::fprintf(f,"%s{\"index\":%d,\"serial\":%d}",n?",":"",r.source.identities[n].index,r.source.identities[n].serial);
    std::fputs("]}}",f);
}
inline void Write(FILE* f){
    if(!f)return;Diagnostics d;AcquireSRWLockShared(&cacheLock);d=cached;ReleaseSRWLockShared(&cacheLock);
    const auto& s=d.summary;auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"installed\":%s,\"closed\":%s,\"enabled\":%s,\"practice\":%s,\"profiles\":%s,\"schema\":%s,\"restart_required\":%s,\"active\":%u,\"requests\":%llu,\"forwards\":%llu,\"original_consumes\":%llu,\"bool_evaluations\":%llu,\"script_steps\":%llu,\"compiled_steps\":%llu,\"completed_frames\":%llu,\"queued\":%llu,\"waiting\":%llu,\"completed\":%llu,\"effective_consumes\":%llu,\"active_changes\":%llu,\"zero_consumes\":%llu,\"changed_original\":%llu,\"cancelled\":%llu,\"source_deferrals\":%llu,\"retired\":%llu,\"closes\":%llu,\"partial_failures\":%llu,\"humans\":[",
        s.stage,s.error,b(s.initialized),b(s.installed),b(s.closed),b(s.enabled),b(s.practice),b(s.profiles),b(s.schema),b(s.restartRequired),s.active,static_cast<unsigned long long>(s.requests),static_cast<unsigned long long>(s.forwards),static_cast<unsigned long long>(s.originalConsumes),static_cast<unsigned long long>(s.boolEvaluations),static_cast<unsigned long long>(s.scriptSteps),static_cast<unsigned long long>(s.compiledSteps),static_cast<unsigned long long>(s.completedFrames),static_cast<unsigned long long>(s.queued),static_cast<unsigned long long>(s.waiting),static_cast<unsigned long long>(s.completed),static_cast<unsigned long long>(s.effectiveConsumes),static_cast<unsigned long long>(s.activeChanges),static_cast<unsigned long long>(s.zeroConsumes),static_cast<unsigned long long>(s.changedOriginal),static_cast<unsigned long long>(s.cancelled),static_cast<unsigned long long>(s.sourceDeferrals),static_cast<unsigned long long>(s.retired),static_cast<unsigned long long>(s.closes),static_cast<unsigned long long>(s.partialFailures));
    bool first=true;for(const auto& h:d.humans){if(!h.registration)continue;
        std::fprintf(f,"%s{\"pawn\":\"%p\",\"player_state\":\"%p\",\"resource\":\"%p\",\"epoch\":%u,\"hero\":%u,\"registration\":%llu,\"used\":%s,\"retired\":%s,\"source_ready\":%s,\"draining\":%s,\"queued\":%zu,\"head\":",
            first?"":",",h.pawn,h.playerState,h.resource,h.epoch,h.hero,static_cast<unsigned long long>(h.registration),b(h.used),b(h.retired),b(h.sourceReady),b(h.draining),h.queued);first=false;
        WriteReceipt(f,h.head);std::fputs(",\"last\":",f);WriteReceipt(f,h.last);std::fputc('}',f);}
    std::fputs("],\"resource_event_amount_used_as_points\":false,\"off_game_thread_state_writes\":false,\"original_image_written\":false,\"delivery_verified\":false,\"native_runtime_verified\":false}",f);
}
}
