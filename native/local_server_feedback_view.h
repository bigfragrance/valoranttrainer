#pragma once
#include "local_server_feedback_ledger.h"
#include "local_server_feedback_pending.h"
#include "local_server_match.h"
#include "original_hud.h"
#include "original_inventory_slots.h"
#include "original_jett_priming.h"
#include <array>
#include <cstdint>
#include <cstring>

// Only the owning client's two real coordinated HUD widgets are intercepted.
// Global KillEvent ordinals, PendingKillRowInfos and combat scores stay native.
// A real native IsEarlierKillNumber query gates the one visual callback; the
// original monotonic Register method then restores the true cumulative ordinal.
// Non-native BP internal calls bypass UFunction.E0: no global VM hook is used.
namespace local_server_feedback_view {struct Pending;}
namespace local_server_feedback_pending_view {
inline bool Handle(size_t index,void* function,void* parameters);
inline void Remember(const local_server_feedback_view::Pending&,const local_server_feedback_wire::Record&);
inline void Poll();inline std::string Json();
}
namespace local_server_feedback_view {
namespace wire=local_server_feedback_wire;
namespace policy=local_server_feedback_pending;
using Weak=original_inventory_slots::WeakIdentity;
using EventFunction=void(*)(void*,void*,void*);
using Kill=original_scoreboard::KillEvent;
inline constexpr size_t Entries=161,EventSlot=0x298/8,QueueCapacity=64;
inline constexpr const wchar_t* Classes[]{
    L"/Game/UI/InGame/HUD/KillCallout/KillCalloutHUDElement.KillCalloutHUDElement_C",
    L"/Game/UI/InGame/KillBanner/CommonAssets/KillBannerAnnouncement.KillBannerAnnouncement_C"};
inline constexpr const wchar_t* Methods[]{
    L"/Game/UI/InGame/HUD/KillCallout/WBP_KillCalloutHUDElement_Base.WBP_KillCalloutHUDElement_Base_C.OnAnyPlayerKilled",
    L"/Game/UI/InGame/KillBanner/CommonAssets/KillBannerAnnouncement.KillBannerAnnouncement_C.OnAnyPlayerKilled"};
struct Array {void* data=nullptr;int32_t count=0,capacity=0;};
struct RowInfo {
    void *owner=nullptr,*left=nullptr,*right=nullptr;int32_t killer=0,victim=0;
    void* response=nullptr;uint8_t ally=0,pad41[7]{};void* weapon=nullptr;
    Array assists{};uint8_t temporary=0,recovery=0,pad74[6]{};
};
struct EarlierParams {RowInfo info{};uint8_t result=0,padding[7]{};};
struct RegisterParams {void* player=nullptr;int32_t ordinal=0,padding=0;};
static_assert(sizeof(RowInfo)==80&&offsetof(RowInfo,killer)==24&&offsetof(RowInfo,assists)==56);
static_assert(sizeof(EarlierParams)==88&&offsetof(EarlierParams,result)==80&&sizeof(RegisterParams)==16);
struct Hook {
    void* object=nullptr;Weak identity{};void** parent=nullptr;EventFunction original=nullptr;
    void* table[Entries+1]{};uint32_t kind=0;bool installed=false,published=false,busy=false;
    int pendingOffset=-1,poolOffset=-1;
};
struct Pending {
    bool used=false;size_t hook=0;uint64_t arrival=0;policy::State state{};Kill kill{};
    std::array<Weak,4> refs{};wire::Key key{};void* widget=nullptr;Weak widgetId{};
};
struct Receipt {
    uint64_t sequence=0,token=0;uint32_t kind=0,killerEpoch=0,victimEpoch=0;
    int32_t player=-1,ordinal=0,window=0,requested=0,victimWindow=0,observed=-1,maximum=-1;
    bool mapped=false,highestExact=false,pendingPreserved=false,poolValid=false,bannerReadback=false;
};
struct State {
    bool bound=false,closed=false,disabled=false;DWORD gameThread=0;
    uint64_t callbacks[2]{},mapped[2]{},forwarded[2]{},queued=0,waits=0,dropped=0,fallbacks=0;
    uint64_t unsupported=0,overflows=0,orderDeferrals=0,orderRestores=0,restoreFailures=0;
    uint64_t reentrant=0,receipts=0,arrival=0,lastPoll=0,lastCache=0,metadataRetries=0;
    uint16_t lastExpectedBytes=0,lastActualBytes=0;uint32_t lastExpectedFlags=0,lastActualFlags=0;
    unsigned error=0;const char* stage="feedback_view_waiting_original_own_hud";
    void* controller=nullptr;Weak controllerId{};
};
inline State state;
inline std::array<Hook,8> hooks{};
inline std::array<Pending,QueueCapacity> pending{};
inline std::array<Receipt,16> receipts{};
inline void* any[2]{},*earlier=nullptr,*registration=nullptr,*owning=nullptr,*playerId=nullptr;
inline std::array<Weak,6> methodIds{};
inline SRWLOCK snapshotLock=SRWLOCK_INIT;
inline std::string snapshotJson="{\"stage\":\"feedback_view_not_sampled\",\"runtime_acceptance_verified\":false}";
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Live(void* object,const Weak& id){return object&&original_inventory_slots::ResolveIdentity(id)==object;}
inline bool Capture(void* object,Weak& id){return object&&original_inventory_slots::CaptureIdentity(object,id)&&Live(object,id);}
// Never catch an original native exception. Restore the prior guard even
// through SEH; a nested callback keeps the outer guard rather than clearing it.
using ProtectedFunction=void(*)(void*);
inline void ProtectedCall(bool& busy,ProtectedFunction call,void* context){
    const bool previous=busy;busy=true;
    __try{call(context);}
    __finally{busy=previous;}
}
inline void* Method(size_t n){
    return n<2?any[n]:n==2?earlier:n==3?registration:n==4?owning:playerId;
}
inline bool MethodLive(void* fn){
    for(size_t n=0;n<methodIds.size();++n)if(fn&&fn==Method(n))return Live(fn,methodIds[n]);return false;
}
inline bool FunctionsLive(size_t kind){
    return kind<2&&MethodLive(any[kind])&&MethodLive(owning)&&MethodLive(playerId)
        &&(kind|| (MethodLive(earlier)&&MethodLive(registration)));
}
// Published table backing is session-lifetime storage. Weak expiration is
// not native object destruction; never recycle it while an old vptr may remain.
inline Hook* UnpublishedSlot(){for(auto& h:hooks)if(!h.published)return &h;return nullptr;}
inline Hook* PublishedCallback(void* object,void** actualTable){
    for(auto& h:hooks)if(h.published&&h.object==object&&actualTable==h.table+1)return &h;return nullptr;
}
inline bool Thread(){return state.bound&&!state.closed&&state.gameThread==GetCurrentThreadId()&&original_players::Thread();}
inline bool Fault(unsigned code,const char* stage){state.error=code;state.disabled=true;state.stage=stage;return false;}
inline bool Field(void* type,const wchar_t* name,int offset,int bytes){
    original_jett_priming::PropertyInfo info{};
    return original_jett_priming::FieldOffset(type,name,static_cast<size_t>(bytes),&info)==offset
        &&info.elementSize==bytes&&info.arrayDim==1;
}
inline bool Function(void*& value,const wchar_t* path,uint16_t bytes,uint32_t flags){
    state.lastExpectedBytes=bytes;state.lastExpectedFlags=flags;value=engine::Find(path);
    state.lastActualBytes=Read<uint16_t>(value,0xbe);state.lastActualFlags=Read<uint32_t>(value,0xb8);
    return value&&state.lastActualBytes==bytes&&state.lastActualFlags==flags
        &&Read<void*>(value,0xe0)==reinterpret_cast<void*>(engine::image+0x1cdb7c0);
}
inline bool Signature(void* fn){
    return Field(fn,L"KillerPlayerState",0,8)&&Field(fn,L"KilledPlayerState",8,8)
        &&Field(fn,L"KillNumberInRoundForKiller",16,4)&&Field(fn,L"KillNumberInRoundForKilled",20,4)
        &&Field(fn,L"Response",24,8)&&Field(fn,L"EquippableUsed",32,8)&&Field(fn,L"AssistsList",40,16);
}
inline bool Metadata(){
    if(any[0]&&FunctionsLive(0)&&FunctionsLive(1))return true;
    ++state.metadataRetries;
    void *feed=nullptr,*banner=nullptr,*order=nullptr,*registerFn=nullptr;
    if(!Function(feed,Methods[0],56,0x0c400000)||!Function(banner,Methods[1],56,0x0c400000)
       ||!Signature(feed)||!Signature(banner)
       ||!Function(order,L"/Game/UI/InGame/HUD/KillCallout/WBP_KillCalloutHUDElement_Base.WBP_KillCalloutHUDElement_Base_C.IsEarlierKillNumber",81,0x0c420000)
       ||!Field(order,L"Kill Feed Info",0,80)||!Field(order,L"Result",80,1)
       ||!Function(registerFn,L"/Game/UI/InGame/HUD/KillCallout/WBP_KillCalloutHUDElement_Base.WBP_KillCalloutHUDElement_Base_C.RegisterKillNumberForPlayer",12,0x0c020000)
       ||!Field(registerFn,L"Killer Player State",0,8)||!Field(registerFn,L"Kill Number in Round for Killer ",8,4))return false;
    auto owner=engine::Find(L"/Script/UMG.Widget.GetOwningPlayer"),id=engine::Find(L"/Script/Engine.PlayerState.GetPlayerId");
    if(!owner||Read<uint16_t>(owner,0xbe)!=8||!id||Read<uint16_t>(id,0xbe)!=4
       ||!(Read<uint32_t>(id,0xb8)&0x400)||Read<void*>(id,0xe0)!=reinterpret_cast<void*>(engine::image+0x49452a0))return false;
    std::array<Weak,6> fresh{};void* objects[]{feed,banner,order,registerFn,owner,id};
    for(size_t n=0;n<fresh.size();++n)if(!Capture(objects[n],fresh[n]))return false;
    // Replace the cache only after every new original handle is validated.
    // Weak handles add no GC root and do not alter UFunction flags or ownership.
    methodIds=fresh;any[0]=feed;any[1]=banner;earlier=order;registration=registerFn;owning=owner;playerId=id;return true;
}
inline bool Original(Hook& h,void* fn,void* parameters){
    if(!Live(h.object,h.identity)||!h.original||!MethodLive(fn))return false;h.original(h.object,fn,parameters);return true;
}
inline bool Own(Hook& h){
    if(!Live(state.controller,state.controllerId))return false;void* owner=nullptr;
    return Original(h,owning,&owner)&&owner==state.controller;
}
inline bool Earlier(Hook& h,const Kill& kill,int32_t number,bool& result){
    EarlierParams p{};p.info.owner=state.controller;p.info.left=kill.killer;p.info.right=kill.victim;
    p.info.killer=number;p.info.victim=kill.victimRoundDeaths;p.info.response=kill.response;p.info.weapon=kill.weapon;
    if(!Original(h,earlier,&p)||p.result>1)return false;result=p.result!=0;return true;
}
inline bool OrderReady(Hook& h,const Kill& kill){
    if(h.kind)return true;
    // In addition to native eligibility, keep an old/reordered event from
    // obscuring an already newer Highest entry. Such packets retain native
    // Pending/expiry semantics through the bounded unmapped fallback.
    if(kill.killerRoundKills>INT32_MAX-2)return false;bool eligible=false,beyond=false;
    return Earlier(h,kill,kill.killerRoundKills,eligible)&&eligible
        &&Earlier(h,kill,kill.killerRoundKills+2,beyond)&&!beyond;
}
inline bool ExactHighest(Hook& h,const Kill& kill){
    bool next=false,after=false;
    return kill.killerRoundKills<=INT32_MAX-2&&Earlier(h,kill,kill.killerRoundKills+1,next)&&next
        &&Earlier(h,kill,kill.killerRoundKills+2,after)&&!after;
}
inline bool ValidArray(const Array& a){return a.count>=0&&a.count<=512&&a.capacity>=a.count&&a.capacity<=4096&&(!a.count||a.data);}
inline bool PendingLive(const Pending& p){
    if(p.hook>=hooks.size())return false;const auto& h=hooks[p.hook];
    return h.installed&&h.object==p.widget&&original_inventory_slots::SameIdentity(h.identity,p.widgetId)
        &&Live(p.widget,p.widgetId)&&Live(p.kill.killer,p.refs[0])&&Live(p.kill.victim,p.refs[1])
        &&Live(p.kill.response,p.refs[2])&&(!p.kill.weapon||Live(p.kill.weapon,p.refs[3]));
}
inline void Record(Hook& h,const Pending& p,const wire::Record* committed,const Kill& display,bool highest,bool preserved,bool pool){
    auto& r=receipts[state.receipts++%receipts.size()];r={};r.kind=h.kind;r.player=p.key.killerId;r.ordinal=p.kill.killerRoundKills;
    r.requested=display.killerRoundKills;r.mapped=committed!=nullptr;r.highestExact=highest;r.pendingPreserved=preserved;r.poolValid=pool;
    if(committed){r.sequence=committed->sequence;r.token=committed->damageToken;r.killerEpoch=committed->killerEpoch;
        r.victimEpoch=committed->victimEpoch;r.window=committed->killerWindow;r.victimWindow=committed->victimWindow;}
    if(h.kind){
        const int ref=original_jett_priming::FieldOffset(Read<void*>(h.object,0x10),L"KillBanner",8);
        auto banner=ref<0?nullptr:Read<void*>(h.object,static_cast<size_t>(ref));Weak id{};
        if(Capture(banner,id)){
            const int count=original_jett_priming::FieldOffset(Read<void*>(banner,0x10),L"CurrentKillCount",4);
            const int max=original_jett_priming::FieldOffset(Read<void*>(banner,0x10),L"MaxKills",4);
            if(count>=0&&max>=0){r.observed=Read<int32_t>(banner,static_cast<size_t>(count));
                r.maximum=Read<int32_t>(banner,static_cast<size_t>(max));r.bannerReadback=true;}
        }
    }
}
inline void ForwardImpl(Pending& p,const wire::Record* committed){
    if(!policy::Consume(p.state))return;
    // Retire before invoking original code. A propagated exception can leave a
    // partial original visual; it must never put the consumed event back in queue.
    p.used=false;auto& h=hooks[p.hook];auto display=p.kill;
    Array pendingBefore{},pendingAfter{},poolAfter{};
    if(!h.kind)pendingBefore=Read<Array>(h.object,static_cast<size_t>(h.pendingOffset));
    if(committed){const auto projection=policy::Project(*committed);
        display.killerRoundKills=h.kind?projection.bannerCount:projection.feedKiller;display.victimRoundDeaths=projection.feedVictim;
    }
    const bool forwarded=Original(h,any[h.kind],&display);
    bool highest=h.kind!=0,preserved=h.kind!=0,pool=h.kind!=0;
    if(forwarded){++state.forwarded[h.kind];
        if(committed){++state.mapped[h.kind];
            if(!h.kind){RegisterParams actual{p.kill.killer,p.kill.killerRoundKills,0};
                const bool registered=Original(h,registration,&actual);highest=registered&&ExactHighest(h,p.kill);
                pendingAfter=Read<Array>(h.object,static_cast<size_t>(h.pendingOffset));
                preserved=std::memcmp(&pendingBefore,&pendingAfter,sizeof(Array))==0;
                poolAfter=Read<Array>(h.object,static_cast<size_t>(h.poolOffset));pool=ValidArray(poolAfter);
                if(highest&&preserved&&pool)++state.orderRestores;
                else{++state.restoreFailures;Fault(4954,"feedback_view_native_order_bookkeeping_readback_failed_mapping_disabled");}
            }
        }else ++state.fallbacks;
        Record(h,p,committed,display,highest,preserved,pool);
    }else ++state.dropped;
    p.used=false;
}
struct ForwardContext {Pending* packet=nullptr;const wire::Record* record=nullptr;};
inline void ForwardProtected(void* opaque){auto& c=*static_cast<ForwardContext*>(opaque);ForwardImpl(*c.packet,c.record);}
inline void Forward(Pending& p,const wire::Record* committed){
    ForwardContext context{&p,committed};ProtectedCall(hooks[p.hook].busy,ForwardProtected,&context);
}
inline void AttemptImpl(Pending& p){
    if(!p.used)return;if(!PendingLive(p)){policy::Consume(p.state);p.used=false;++state.dropped;return;}
    wire::Record committed{};const auto found=state.disabled?wire::Lookup::Missing:local_server_feedback_ledger::Lookup(p.key,committed);
    bool order=true;if(found==wire::Lookup::Found){order=OrderReady(hooks[p.hook],p.kill);if(!order)++state.orderDeferrals;}
    const auto action=policy::Decide(p.state,found,GetTickCount64(),true,order);
    if(action==policy::Action::ForwardMapped)Forward(p,&committed);
    else if(action==policy::Action::ForwardOriginal){
        // Preserve a genuine immutable commit before the native pending path
        // can outlive the transport ring. Its actual Pending/Weak binds lifetime.
        if(found==wire::Lookup::Found&&!hooks[p.hook].kind)local_server_feedback_pending_view::Remember(p,committed);
        Forward(p,nullptr);
    }
    else ++state.waits;
}
inline void AttemptProtected(void* opaque){AttemptImpl(*static_cast<Pending*>(opaque));}
inline void Attempt(Pending& p){
    if(!p.used)return;
    if(p.hook>=hooks.size()){policy::Consume(p.state);p.used=false;++state.dropped;return;}
    // Covers original ordering helpers, the visual callback, Register and its
    // exact readbacks. Synchronous reentry takes Observe's native passthrough.
    ProtectedCall(hooks[p.hook].busy,AttemptProtected,&p);
}
inline bool Id(Hook& h,void* ps,int32_t& result){
    auto process=Read<EventFunction>(Read<void**>(ps),0x298);
    if(!process||!MethodLive(playerId))return false;process(ps,playerId,&result);
    (void)h;return result>=0&&Read<int32_t>(ps,0x468)==result;
}
inline bool CapturePacket(Hook& h,size_t index,const Kill& kill,Pending& p){
    // Native FAssistRecord is 40 bytes, not a pointer array. The current local
    // authority sends zero assists. Unknown nonempty assist schemas pass through
    // unchanged rather than being copied or retained with guessed object roots.
    if(kill.assistCount||kill.assistCapacity<0||kill.killerRoundKills<=0||kill.victimRoundDeaths<0)return false;
    p={};p.used=true;p.hook=index;p.arrival=++state.arrival;p.state.queuedAt=GetTickCount64();p.kill=kill;
    p.widget=h.object;p.widgetId=h.identity;
    if(!Capture(kill.killer,p.refs[0])||!Capture(kill.victim,p.refs[1])||!Capture(kill.response,p.refs[2])
       ||(kill.weapon&&!Capture(kill.weapon,p.refs[3]))
       ||!Id(h,kill.killer,p.key.killerId)||!Id(h,kill.victim,p.key.victimId)||p.key.killerId==p.key.victimId)return false;
    p.key.killerOrdinal=kill.killerRoundKills;p.key.victimOrdinal=kill.victimRoundDeaths;
    // Empty native TArray has no payload to retain. Cumulative per-player
    // ordinals plus exact pair and victim identify the immutable receipt.
    p.kill.assists=nullptr;p.kill.assistCount=p.kill.assistCapacity=0;return true;
}
inline void Observe(void* object,void* fn,void* params){
    // This object is the actual native callback argument, not a retired raw
    // cache dereference. Its published vptr selects the correct generation even
    // if a later UObject reused the same address and got a different hook slot.
    Hook* hook=PublishedCallback(object,Read<void**>(object));
    const size_t index=hook?static_cast<size_t>(hook-hooks.data()):0;
    if(!hook||!hook->original)return;auto& h=*hook;
    if(!h.kind&&local_server_feedback_pending_view::Handle(index,fn,params))return;
    if(fn!=any[h.kind]||!params){h.original(object,fn,params);return;}
    ++state.callbacks[h.kind];
    if(!Thread()||state.disabled||!h.installed||h.busy||!FunctionsLive(h.kind)||!Own(h)){
        if(h.busy)++state.reentrant;h.original(object,fn,params);++state.forwarded[h.kind];return;
    }
    Pending local{};const auto& kill=*static_cast<const Kill*>(params);
    if(h.kind&&kill.killer!=Read<void*>(state.controller,0x470)){
        h.original(object,fn,params);++state.forwarded[h.kind];return;
    }
    if(!CapturePacket(h,index,kill,local)){++state.unsupported;h.original(object,fn,params);++state.forwarded[h.kind];return;}
    Attempt(local);if(!local.used)return;
    for(auto& p:pending)if(!p.used){p=local;++state.queued;return;}
    ++state.overflows;Forward(local,nullptr);
}
inline bool Install(size_t kind){
    if(!engine::Find(Classes[kind]))return true;
    auto object=original_hud::FindCoordinatedWidget(Classes[kind]);if(!object)return true;
    for(const auto& h:hooks)if(h.installed&&h.object==object&&Live(object,h.identity))return true;
    auto table=Read<void**>(object);const uintptr_t expected=kind?0xb2e1ea0:0xb314b20;
    if(table!=reinterpret_cast<void**>(engine::image+expected)||!original_players::Readable(table-1,(Entries+1)*8)
       ||table[EventSlot]!=reinterpret_cast<void*>(engine::image+0x1cdc850))return Fault(4951,"feedback_view_original_complete_widget_table_mismatch");
    Hook* available=UnpublishedSlot();
    if(!available)return Fault(4952,"feedback_view_widget_generation_capacity");
    auto& h=*available;h={};h.kind=static_cast<uint32_t>(kind);h.object=object;h.parent=table;h.original=reinterpret_cast<EventFunction>(table[EventSlot]);
    if(!Capture(object,h.identity)||!Own(h))return Fault(4953,"feedback_view_widget_sole_local_owner_not_verified");
    if(!kind){auto type=Read<void*>(object,0x10);
        h.pendingOffset=original_jett_priming::FieldOffset(type,L"PendingKillRowInfos",16);
        h.poolOffset=original_jett_priming::FieldOffset(type,L"KillCalloutRows",16);
        if(h.pendingOffset<0||h.poolOffset<0)return Fault(4955,"feedback_view_original_pending_and_row_pool_metadata_unavailable");
    }
    std::memcpy(h.table,table-1,sizeof(h.table));h.table[1+EventSlot]=reinterpret_cast<void*>(Observe);
    h.published=true;h.installed=true;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),h.table+1,table)!=table){
        h.installed=false;h.published=false;return Fault(4956,"feedback_view_widget_changed_before_owned_install");
    }
    return true;
}
inline void Cache(){
    local_server_match::Json out;const auto b=[](bool value){return value?"true":"false";};const auto& l=local_server_feedback_ledger::state;
    out.Add("{\"stage\":\"%s\",\"error\":%u,\"disabled\":%s,\"global_ordinals_modified\":false,\"original_kills_replayed\":0,\"timer_reimplementation\":false,\"runtime_acceptance_verified\":false,\"ledger\":{\"stage\":\"%s\",\"bound\":%s,\"commits\":%llu,\"published\":%llu,\"lookups\":%llu,\"matched\":%llu,\"missing\":%llu,\"expired\":%llu,\"rejected\":%llu,\"incomplete\":%llu,\"invalid_records\":%llu}",
        state.stage,state.error,b(state.disabled),l.stage,b(l.bound),l.nextCommit,l.published,l.lookups,l.matched,l.missing,l.expired,l.rejected,l.incomplete,l.invalidRecords);
    out.Add(",\"callbacks\":[%llu,%llu],\"mapped\":[%llu,%llu],\"forwarded\":[%llu,%llu],\"queued\":%llu,\"waits\":%llu,\"dropped\":%llu,\"fallbacks\":%llu,\"unsupported\":%llu,\"overflows\":%llu,\"order_deferrals\":%llu,\"order_restores\":%llu,\"restore_failures\":%llu,\"reentrant\":%llu,\"receipts\":[",
        state.callbacks[0],state.callbacks[1],state.mapped[0],state.mapped[1],state.forwarded[0],state.forwarded[1],state.queued,state.waits,state.dropped,state.fallbacks,state.unsupported,state.overflows,state.orderDeferrals,state.orderRestores,state.restoreFailures,state.reentrant);
    const uint64_t count=(std::min)(state.receipts,static_cast<uint64_t>(receipts.size()));
    for(uint64_t i=0;i<count;++i){const auto& r=receipts[static_cast<size_t>((state.receipts-count+i)%receipts.size())];
        out.Add("%s{\"kind\":\"%s\",\"sequence\":%llu,\"token\":%llu,\"player_id\":%d,\"killer_epoch\":%u,\"victim_epoch\":%u,\"real_ordinal\":%d,\"native_window\":%d,\"requested\":%d,\"victim_window\":%d,\"mapped\":%s,\"highest_exact_real_ordinal\":%s,\"pending_preserved\":%s,\"row_pool_valid\":%s,\"banner_readback\":%s,\"banner_count\":%d,\"banner_maximum\":%d}",
            i?",":"",r.kind?"banner":"killfeed",r.sequence,r.token,r.player,r.killerEpoch,r.victimEpoch,r.ordinal,r.window,r.requested,r.victimWindow,b(r.mapped),b(r.highestExact),b(r.pendingPreserved),b(r.poolValid),b(r.bannerReadback),r.observed,r.maximum);
    }out.Add("],\"metadata_retries\":%llu,\"last_signature\":{\"expected_bytes\":%u,\"actual_bytes\":%u,\"expected_flags\":%u,\"actual_flags\":%u}}",state.metadataRetries,static_cast<unsigned>(state.lastExpectedBytes),static_cast<unsigned>(state.lastActualBytes),state.lastExpectedFlags,state.lastActualFlags);
    // The pending adapter also serializes only its game-thread POD snapshot.
    if(!out.value.empty()&&out.value.back()=='}')out.value.pop_back();
    out.Add(",\"pending_projection\":");out.value+=local_server_feedback_pending_view::Json();out.Add("}");
    AcquireSRWLockExclusive(&snapshotLock);snapshotJson.swap(out.value);ReleaseSRWLockExclusive(&snapshotLock);
}
inline std::string Json(){AcquireSRWLockShared(&snapshotLock);const auto value=snapshotJson;ReleaseSRWLockShared(&snapshotLock);return value;}
inline void Poll(void* ownController){
    if(!local_server_feedback_ledger::Enabled()||local_server_feedback_ledger::state.host||state.closed)return;
    if(!state.bound){state.controller=ownController;if(!Capture(ownController,state.controllerId))return;
        state.gameThread=GetCurrentThreadId();state.bound=true;}
    if(!Thread()||ownController!=state.controller||!Live(state.controller,state.controllerId))return;
    const auto now=GetTickCount64();
    if(!state.disabled&&now-state.lastPoll>=100){state.lastPoll=now;
        if(!Metadata())state.stage="feedback_view_waiting_original_widget_signatures";
        else if(Install(0)&&Install(1))state.stage="feedback_view_own_original_widgets_waiting_real_native_events";
    }
    // Arrival order is preserved across every retry, including a ring slot
    // reclaimed before another older slot. No native GT wait or kill replay.
    uint64_t previous=0;
    for(size_t n=0;n<QueueCapacity;++n){Pending* oldest=nullptr;
        for(auto& p:pending)if(p.used&&p.arrival>previous&&(!oldest||p.arrival<oldest->arrival))oldest=&p;
        if(!oldest)break;previous=oldest->arrival;Attempt(*oldest);
    }
    local_server_feedback_pending_view::Poll();
    if(now-state.lastCache>=100){state.lastCache=now;Cache();}
}
inline void Close(){
    if(state.closed)return;if(state.bound&&!Thread())return;
    for(auto& p:pending)if(p.used){policy::Consume(p.state);p.used=false;++state.dropped;}
    for(auto& h:hooks)if(h.installed){
        if(Live(h.object,h.identity)&&Read<void**>(h.object)==h.table+1)
            InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(h.object),h.parent,h.table+1);
        // Retain the published table and original forwarder even if the Weak
        // expired before restoration. Late legitimate native callbacks route
        // straight to original code; this backing is never a free slot again.
        h.installed=false;
    }
    state.closed=true;state.stage="feedback_view_own_widget_entries_restored";Cache();
}
} // namespace local_server_feedback_view

#include "local_server_feedback_pending_view.h"
