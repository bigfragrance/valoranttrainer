#pragma once
#include "local_server_feedback_row_match.h"
#include <cmath>
#include <cstring>

// Included after the owning widget adapter. Original pending sorting, expiry,
// row creation, animation and removal execute unchanged, exactly once. Only a
// strictly identified real newly appended row receives two visual-only calls.
namespace local_server_feedback_pending_view {
namespace view=local_server_feedback_view;
namespace match=local_server_feedback_row_match;
namespace wire=local_server_feedback_wire;
using Weak=view::Weak;
using Row=view::RowInfo;
using Array=view::Array;
using Hook=view::Hook;
struct Timed {Row row{};float expiry=0;uint32_t padding=0;};
static_assert(sizeof(Timed)==88&&alignof(Timed)==8&&offsetof(Timed,expiry)==80);
struct Method {void* object=nullptr;Weak identity{};};
enum MethodRole:size_t {Timer,Text,Vfx,Count,Child,Visibility,MethodCount};
inline constexpr const wchar_t* Paths[]{
    L"/Game/UI/InGame/HUD/KillCallout/WBP_KillCalloutHUDElement_Base.WBP_KillCalloutHUDElement_Base_C.EventTickPendingKillRows",
    L"/Game/UI/InGame/HUD/KillCallout/KillFeed_Assists/WBP_KillFeedRow_Base.WBP_KillFeedRow_Base_C.SetTextForKillNumber",
    L"/Game/UI/InGame/HUD/KillCallout/KillFeed_Assists/WBP_KillFeedRow_Base.WBP_KillFeedRow_Base_C.UpdateVFXVisibility",
    L"/Script/UMG.PanelWidget.GetChildrenCount",L"/Script/UMG.PanelWidget.GetChildAt",L"/Script/UMG.Widget.GetVisibility"};
inline constexpr const wchar_t* RowNames[]{
    L"OwningPlayer_3_361529BD405A88DC93FB5390E366B0B4",L"LeftPlayerState_9_B3CF0D5E424FE9D9A566E580429632D3",
    L"RightPlayerState_11_53DE82B84913E95183A716A36AA4B577",L"KillNumberInRoundForKiller_13_44B38B8E43DAE4CA31C663A0F63B723C",
    L"KillNumberInRoundForVictim_31_CCD4A2E74B36D60E7EEC98B3891E833D",L"DamageResponse_15_A6F68D6947762DA4CB967B8C634882EC",
    L"bWasAllyRes_19_7DFB202F45A964FD63F334BDAFD948A8",L"EquippableUsed_22_793FB6124858143BDAA56F933AADA733",
    L"AssistsList_26_0C92D62844C93DDFB2E8BC9A5A221CA3",L"bWasTemporaryDeath_28_36D834AB48CDE62C29B6C893C7C63808",
    L"bWasRecovery_33_0C60F131465A698060DD7B8B4E584337"};
struct Captured {
    match::Snapshot rows{};void* box=nullptr;Weak boxId{};
    std::array<std::array<Weak,4>,match::Capacity> refs{};
};
struct Receipt {
    uint64_t sequence=0,token=0;int32_t player=-1,victim=-1,ordinal=0,window=0,victimWindow=0;
    match::Object row{};uint8_t numerals=255,fire=255;bool attempted=false,completed=false,readback=false;
};
struct Retained {
    bool used=false,attached=false;void* widget=nullptr;Weak widgetId{};
    view::Kill kill{};std::array<Weak,2> players{};
    wire::Record record{};wire::Binding binding{};
};
struct State {
    bool ready=false;const char* stage="pending_visual_waiting_original_metadata";
    uint64_t nextMetadata=0,callbacks=0,originalAttempts=0,originalReturns=0,snapshotRejects=0;
    uint64_t unchanged=0,ambiguous=0,reentrant=0,paired=0,attempts=0,projected=0,duplicate=0,stale=0;
    uint64_t ledgerMissing=0,ledgerExpired=0,methodFailures=0,receiptCount=0,readbacks=0;
    uint64_t retained=0,attached=0,retainedLookups=0,retired=0,retentionFull=0,retentionConflicts=0;
    uint32_t lastPlan=0;size_t beforePending=0,afterPending=0,beforeChildren=0,afterChildren=0;
    int pendingOffset=-1,verticalOffset=-1,poolOffset=-1,numeralsOffset=-1,fireOffset=-1;
    void *actualFeedClass=nullptr,*rowWidgetClass=nullptr;Weak actualFeedClassId{},rowWidgetClassId{};
};
inline State state;
inline std::array<Method,MethodCount> methods{};
inline std::array<Receipt,16> receipts{};
inline std::array<Retained,match::Capacity> retained{};
inline SRWLOCK lock=SRWLOCK_INIT;
inline std::string cached="{\"ready\":false,\"stage\":\"pending_visual_not_sampled\"}";
template<class T> inline T Read(const void* object,size_t offset=0){return view::Read<T>(object,offset);}
inline bool Live(const Method& m){return view::Live(m.object,m.identity);}
inline bool MethodsLive(){for(const auto& m:methods)if(!Live(m))return false;return true;}
inline void* Property(void* type,const wchar_t* name,int offset,int bytes){
    original_jett_priming::PropertyInfo field{};
    return original_jett_priming::FieldOffset(type,name,static_cast<size_t>(bytes),&field)==offset
        &&field.elementSize==bytes&&field.arrayDim==1?field.field:nullptr;
}
inline uint64_t Cast(void* property){return Read<uint64_t>(Read<void*>(property,8),0x18);}
inline bool Bind(size_t n,uint16_t bytes,uint32_t flags,uintptr_t exec){
    auto object=engine::Find(Paths[n]);
    if(!object||Read<uint16_t>(object,0xbe)!=bytes||Read<uint32_t>(object,0xb8)!=flags
       ||Read<void*>(object,0xe0)!=reinterpret_cast<void*>(engine::image+exec))return false;
    methods[n].object=object;return view::Capture(object,methods[n].identity);
}
inline Hook* Feed(){for(auto& h:view::hooks)if(h.installed&&!h.kind&&view::Live(h.object,h.identity))return &h;return nullptr;}
inline bool Metadata(Hook& h){
    if(!Bind(Timer,0,0x0c000000,0x1cdb7c0)||!Bind(Text,4,0x0c080000,0x1cdb7c0)
       ||!view::Field(methods[Text].object,L"KillNumber",0,4)
       ||!Bind(Vfx,8,0x0c020000,0x1cdb7c0)||!view::Field(methods[Vfx].object,L"KillerNumber",0,4)
       ||!view::Field(methods[Vfx].object,L"VictimNumber",4,4)
       ||!Bind(Count,4,0x54020403,0x3b964a0)||!view::Field(methods[Count].object,L"ReturnValue",0,4)
       ||!Bind(Child,16,0x54020403,0x3b96370)||!view::Field(methods[Child].object,L"Index",0,4)
       ||!view::Field(methods[Child].object,L"ReturnValue",8,8))return false;
    // Existing original streak observer pins this read-only native getter.
    auto visibility=engine::Find(Paths[Visibility]);
    if(!visibility||Read<uint16_t>(visibility,0xbe)!=1||!(Read<uint32_t>(visibility,0xb8)&0x400)
       ||Read<void*>(visibility,0xe0)!=reinterpret_cast<void*>(engine::image+0x3bc1640)
       ||!view::Field(visibility,L"ReturnValue",0,1)||!view::Capture(visibility,methods[Visibility].identity))return false;
    methods[Visibility].object=visibility;
    auto actual=Read<void*>(h.object,0x10);auto pending=Property(actual,L"PendingKillRowInfos",h.pendingOffset,16);
    auto inner=Read<void*>(pending,0x80),structure=Read<void*>(inner,0x78);
    auto rowProperty=Property(structure,L"RowInfo_6_0BFCC48B427F1A7ADCFD21A260E1C1DF",0,80);
    auto row=Read<void*>(rowProperty,0x78);
    // PropertiesSize records field end (84 and74); FProperty.ElementSize88/80
    // records aligned storage. Only the latter defines the real array stride.
    if((Cast(pending)&0x208001)!=0x208001||(Cast(inner)&0x108001)!=0x108001
       ||Read<int32_t>(inner,0x34)!=sizeof(Timed)||Read<int32_t>(structure,0x60)!=84
       ||!Property(structure,L"ExpiryTime_5_94B4E9B7471CDFAB147CC28397FBF680",80,4)
       ||!rowProperty||Read<int32_t>(row,0x60)!=74)return false;
    constexpr int offsets[]{0,8,16,24,28,32,40,48,56,72,73},sizes[]{8,8,8,4,4,8,1,8,16,1,1};
    for(size_t n=0;n<11;++n)if(!Property(row,RowNames[n],offsets[n],sizes[n]))return false;
    const int vertical=original_jett_priming::FieldOffset(actual,L"VerticalBoxForKillRows",8);
    auto rowClass=engine::Find(L"/Game/UI/InGame/HUD/KillCallout/KillFeed_Assists/KillFeedRow.KillFeedRow_C");
    const int numerals=original_jett_priming::FieldOffset(rowClass,L"Numerals",8);
    const int fire=original_jett_priming::FieldOffset(rowClass,L"OnFireVFX",8);
    if(vertical<0||numerals<0||fire<0||!view::Capture(actual,state.actualFeedClassId)
       ||!view::Capture(rowClass,state.rowWidgetClassId))return false;
    state.actualFeedClass=actual;state.rowWidgetClass=rowClass;state.pendingOffset=h.pendingOffset;
    state.poolOffset=h.poolOffset;state.verticalOffset=vertical;state.numeralsOffset=numerals;state.fireOffset=fire;return true;
}
inline bool Call(void* object,const Weak& identity,size_t method,void* parameters){
    if(method>=methods.size()||!view::Live(object,identity)||!Live(methods[method]))return false;
    if(!original_players::Derived(Read<void*>(object,0x10),Read<void*>(methods[method].object,0x28)))return false;
    auto process=Read<view::EventFunction>(Read<void**>(object),0x298);
    if(process!=reinterpret_cast<view::EventFunction>(engine::image+0x1cdc850))return false;
    process(object,methods[method].object,parameters);return true;
}
inline bool Capture(Hook& h,Captured& snapshot){
    if(!view::Live(h.object,h.identity)||!view::Live(state.actualFeedClass,state.actualFeedClassId)
       ||Read<void*>(h.object,0x10)!=state.actualFeedClass)return false;
    const auto array=Read<Array>(h.object,static_cast<size_t>(state.pendingOffset));
    if(!view::ValidArray(array)||array.count>static_cast<int32_t>(match::Capacity)
       ||(array.count&&((reinterpret_cast<uintptr_t>(array.data)%alignof(Timed))
          ||!original_players::Readable(array.data,static_cast<size_t>(array.count)*sizeof(Timed)))))return false;
    snapshot.rows.sources=static_cast<size_t>(array.count);
    for(int32_t n=0;n<array.count;++n){Timed value{};std::memcpy(&value,static_cast<const uint8_t*>(array.data)+static_cast<size_t>(n)*sizeof(Timed),sizeof(value));
        const auto& r=value.row;auto& s=snapshot.rows.pending[static_cast<size_t>(n)];auto& refs=snapshot.refs[static_cast<size_t>(n)];
        if(r.owner!=view::state.controller||r.assists.count||r.assists.capacity<0||r.ally||r.temporary||r.recovery
           ||r.killer<=0||r.victim<0||!std::isfinite(value.expiry)
           ||!view::Capture(r.left,refs[0])||!view::Capture(r.right,refs[1])
           ||!view::Id(h,r.left,s.key.killerId)||!view::Id(h,r.right,s.key.victimId))return false;
        s.owner=reinterpret_cast<uintptr_t>(r.owner);s.left=reinterpret_cast<uintptr_t>(r.left);s.right=reinterpret_cast<uintptr_t>(r.right);
        s.response=reinterpret_cast<uintptr_t>(r.response);s.weapon=reinterpret_cast<uintptr_t>(r.weapon);
        s.killerOrdinal=r.killer;s.victimOrdinal=r.victim;s.key.killerOrdinal=r.killer;s.key.victimOrdinal=r.victim;
        s.leftIndex=refs[0].index;s.leftSerial=refs[0].serial;s.rightIndex=refs[1].index;s.rightSerial=refs[1].serial;
        std::memcpy(&s.expiryBits,&value.expiry,sizeof(value.expiry));s.ally=r.ally;s.temporary=r.temporary;s.recovery=r.recovery;
    }
    snapshot.box=Read<void*>(h.object,static_cast<size_t>(state.verticalOffset));
    if(!view::Capture(snapshot.box,snapshot.boxId))return false;int32_t count=-1;
    if(!Call(snapshot.box,snapshot.boxId,Count,&count)||count<0||count>static_cast<int32_t>(match::Capacity))return false;
    snapshot.rows.children=static_cast<size_t>(count);
    for(int32_t n=0;n<count;++n){struct ChildParams{int32_t index=0,pad=0;void* object=nullptr;} child{n,0,nullptr};
        static_assert(sizeof(ChildParams)==16&&offsetof(ChildParams,object)==8);
        Weak identity{};if(!Call(snapshot.box,snapshot.boxId,Child,&child)||!view::Capture(child.object,identity))return false;
        snapshot.rows.rows[static_cast<size_t>(n)]={reinterpret_cast<uintptr_t>(child.object),identity.index,identity.serial};
    }
    return true;
}
inline bool SourceLive(const Captured& snapshot,size_t index){
    const auto& s=snapshot.rows.pending[index];const auto& refs=snapshot.refs[index];
    return view::Live(reinterpret_cast<void*>(s.left),refs[0])&&view::Live(reinterpret_cast<void*>(s.right),refs[1]);
}
inline bool BindingCurrent(const wire::Binding& a){
    const auto& b=local_server_feedback_ledger::state.binding;
    return local_server_feedback_ledger::state.bound&&wire::ValidBinding(a)
        &&a.pairDigest==b.pairDigest&&a.rosterDigest==b.rosterDigest&&a.hostStarted==b.hostStarted
        &&a.hostPid==b.hostPid&&a.clientPid==b.clientPid&&a.rosterGeneration==b.rosterGeneration&&a.rosterCount==b.rosterCount;
}
inline bool Matches(const Retained& value,const match::Source& source){
    return wire::Matches(value.record,source.key)&&reinterpret_cast<uintptr_t>(value.kill.killer)==source.left
        &&reinterpret_cast<uintptr_t>(value.kill.victim)==source.right&&reinterpret_cast<uintptr_t>(value.kill.response)==source.response
        &&reinterpret_cast<uintptr_t>(value.kill.weapon)==source.weapon
        &&value.players[0].index==source.leftIndex&&value.players[0].serial==source.leftSerial
        &&value.players[1].index==source.rightIndex&&value.players[1].serial==source.rightSerial;
}
inline void Remember(const view::Pending& packet,const wire::Record& record){
    if(packet.hook>=view::hooks.size()||view::hooks[packet.hook].kind||!wire::ValidRecord(record)
       ||!wire::Matches(record,packet.key)||!BindingCurrent(local_server_feedback_ledger::state.binding))return;
    Retained* available=nullptr;
    for(auto& old:retained){
        if(old.used&&(!view::Live(old.widget,old.widgetId)||!BindingCurrent(old.binding))){old.used=false;++state.retired;}
        if(old.used&&old.widget==packet.widget&&original_inventory_slots::SameIdentity(old.widgetId,packet.widgetId)
           &&wire::Matches(old.record,packet.key)){
            if(old.record.sequence!=record.sequence||old.record.checksum!=record.checksum)++state.retentionConflicts;
            return;
        }
        if(!old.used&&!available)available=&old;
    }
    if(!available){++state.retentionFull;return;}
    *available={};available->used=true;available->widget=packet.widget;available->widgetId=packet.widgetId;
    available->kill=packet.kill;available->players={packet.refs[0],packet.refs[1]};
    available->record=record;available->binding=local_server_feedback_ledger::state.binding;++state.retained;
}
inline void ObservePending(Hook& hook,const Captured& snapshot,bool after){
    for(auto& old:retained)if(old.used){
        if(!BindingCurrent(old.binding)||!view::Live(old.widget,old.widgetId)){old.used=false;++state.retired;continue;}
        if(old.widget!=hook.object||!original_inventory_slots::SameIdentity(old.widgetId,hook.identity))continue;
        bool present=false;for(size_t n=0;n<snapshot.rows.sources;++n)if(Matches(old,snapshot.rows.pending[n])){present=true;break;}
        if(present&&!old.attached){old.attached=true;++state.attached;}
        // No clock extends the receipt: its retention is bounded by the real
        // native Pending item and original owning widget generation only.
        if(!present&&(after||old.attached)){old.used=false;++state.retired;}
    }
}
inline bool RetainedRecord(Hook& hook,const match::Source& source,wire::Record& record){
    for(const auto& old:retained)if(old.used&&old.attached&&old.widget==hook.object
        &&original_inventory_slots::SameIdentity(old.widgetId,hook.identity)&&BindingCurrent(old.binding)
        &&Matches(old,source)&&wire::ValidRecord(old.record)){
        record=old.record;++state.retainedLookups;return true;
    }
    return false;
}
inline bool Seen(const match::Source& source,const match::Object& row){
    for(const auto& r:receipts)if(r.attempted&&match::Same(r.row,row)&&r.player==source.key.killerId&&r.victim==source.key.victimId&&r.ordinal==source.killerOrdinal)return true;return false;
}
inline bool PoolContains(Hook& h,void* row){
    const auto pool=Read<Array>(h.object,static_cast<size_t>(state.poolOffset));
    if(!view::ValidArray(pool)||(pool.count&&!original_players::Readable(pool.data,static_cast<size_t>(pool.count)*sizeof(void*))))return false;
    for(int32_t n=0;n<pool.count;++n)if(static_cast<void**>(pool.data)[n]==row)return true;return false;
}
inline void Project(Hook& h,const Captured& before,size_t sourceIndex,const match::Object& row){
    const auto& source=before.rows.pending[sourceIndex];
    auto object=reinterpret_cast<void*>(row.address);const Weak identity{row.index,row.serial};
    if(!SourceLive(before,sourceIndex)||!view::Live(object,identity)||!view::Live(state.rowWidgetClass,state.rowWidgetClassId)
       ||!original_players::Derived(Read<void*>(object,0x10),state.rowWidgetClass)||!PoolContains(h,object)){
        ++state.stale;return;
    }
    void* owner=nullptr;
    if(!view::MethodLive(view::owning)){++state.methodFailures;return;}
    auto event=Read<view::EventFunction>(Read<void**>(object),0x298);
    if(event!=reinterpret_cast<view::EventFunction>(engine::image+0x1cdc850)){++state.methodFailures;return;}
    event(object,view::owning,&owner);if(owner!=view::state.controller){++state.stale;return;}
    if(Seen(source,row)){++state.duplicate;return;}
    wire::Record record{};const auto found=RetainedRecord(h,source,record)?wire::Lookup::Found:local_server_feedback_ledger::Lookup(source.key,record);
    if(found!=wire::Lookup::Found){if(found==wire::Lookup::Expired)++state.ledgerExpired;else ++state.ledgerMissing;return;}
    auto& receipt=receipts[state.receiptCount++%receipts.size()];receipt={};receipt.sequence=record.sequence;receipt.token=record.damageToken;
    receipt.player=record.killerId;receipt.victim=record.victimId;receipt.ordinal=record.killerOrdinal;
    receipt.window=record.killerWindow;receipt.victimWindow=record.victimWindow;receipt.row=row;receipt.attempted=true;++state.attempts;
    // Marked before the first original visual call. An exceptional partial
    // display is never replayed or turned into another original kill callback.
    int32_t number=record.killerWindow;struct VfxParams{int32_t killer=0,victim=0;} vfx{record.killerWindow,record.victimWindow};
    if(!Call(object,identity,Text,&number)||!Call(object,identity,Vfx,&vfx)){++state.methodFailures;return;}
    receipt.completed=true;++state.projected;
    auto numeral=Read<void*>(object,static_cast<size_t>(state.numeralsOffset)),fire=Read<void*>(object,static_cast<size_t>(state.fireOffset));
    Weak numeralId{},fireId{};
    if(view::Capture(numeral,numeralId)&&view::Capture(fire,fireId)
       &&Call(numeral,numeralId,Visibility,&receipt.numerals)&&Call(fire,fireId,Visibility,&receipt.fire)){
        receipt.readback=receipt.numerals==(record.killerWindow>2?3:1)&&receipt.fire==(record.killerWindow>=3?3:1);
        if(receipt.readback)++state.readbacks;
    }
}
struct Context {Hook* hook=nullptr;void* function=nullptr;void* parameters=nullptr;};
inline void Process(void* opaque){
    auto& context=*static_cast<Context*>(opaque);auto& h=*context.hook;Captured before{},after{};
    const auto reentrant=view::state.reentrant+state.reentrant;const bool validBefore=Capture(h,before);
    if(validBefore)ObservePending(h,before,false);
    ++state.originalAttempts;h.original(h.object,context.function,context.parameters);++state.originalReturns;
    if(!validBefore||!Capture(h,after)||before.box!=after.box
       ||!original_inventory_slots::SameIdentity(before.boxId,after.boxId)){
        ++state.snapshotRejects;state.stage="pending_visual_original_tick_forwarded_snapshot_unknown";return;
    }
    state.beforePending=before.rows.sources;state.afterPending=after.rows.sources;
    state.beforeChildren=before.rows.children;state.afterChildren=after.rows.children;
    if(reentrant!=view::state.reentrant+state.reentrant){++state.ambiguous;state.stage="pending_visual_nested_callback_not_associated";return;}
    const auto plan=match::Associate(before.rows,after.rows);state.lastPlan=static_cast<uint32_t>(plan.status);
    if(plan.status==match::Status::NoChange){++state.unchanged;ObservePending(h,after,true);return;}
    if(plan.status!=match::Status::Mapped){++state.ambiguous;ObservePending(h,after,true);state.stage="pending_visual_native_row_association_not_unique";return;}
    state.paired+=plan.count;
    for(size_t n=0;n<plan.count;++n)Project(h,before,plan.pairs[n].source,after.rows.rows[plan.pairs[n].child]);
    ObservePending(h,after,true);
    state.stage="pending_visual_only_actual_native_new_rows_projected";
}
inline bool Handle(size_t index,void* function,void* parameters){
    if(!state.ready||!Live(methods[Timer])||function!=methods[Timer].object||index>=view::hooks.size())return false;
    auto& h=view::hooks[index];
    if(h.busy){++state.reentrant;return false;}
    if(!view::Thread()||view::state.disabled||!h.installed||h.kind||!MethodsLive()||!view::Own(h))return false;
    ++state.callbacks;Context context{&h,function,parameters};view::ProtectedCall(h.busy,Process,&context);return true;
}
inline void Cache(){
    local_server_match::Json out;const auto b=[](bool x){return x?"true":"false";};
    out.Add("{\"ready\":%s,\"stage\":\"%s\",\"original_pending_modified\":false,\"highest_written_by_projection\":false,\"original_kill_replays\":0,\"populate_calls\":0,\"play_in_calls\":0,\"timer_decisions_reimplemented\":false,\"callbacks\":%llu,\"original_attempts\":%llu,\"original_returns\":%llu,\"snapshot_rejects\":%llu,\"unchanged\":%llu,\"ambiguous\":%llu,\"reentrant\":%llu,\"paired\":%llu,\"attempts\":%llu,\"projected\":%llu,\"readbacks\":%llu,\"duplicate_visuals_skipped\":%llu,\"stale\":%llu,\"ledger_missing\":%llu,\"ledger_expired\":%llu,\"method_failures\":%llu,\"last_plan\":%u,\"before_pending\":%zu,\"after_pending\":%zu,\"before_children\":%zu,\"after_children\":%zu,\"receipts\":[",
        b(state.ready),state.stage,state.callbacks,state.originalAttempts,state.originalReturns,state.snapshotRejects,state.unchanged,state.ambiguous,state.reentrant,state.paired,state.attempts,state.projected,state.readbacks,state.duplicate,state.stale,state.ledgerMissing,state.ledgerExpired,state.methodFailures,state.lastPlan,state.beforePending,state.afterPending,state.beforeChildren,state.afterChildren);
    const auto count=(std::min)(state.receiptCount,static_cast<uint64_t>(receipts.size()));
    for(uint64_t n=0;n<count;++n){const auto& r=receipts[static_cast<size_t>((state.receiptCount-count+n)%receipts.size())];
        out.Add("%s{\"sequence\":%llu,\"token\":%llu,\"player_id\":%d,\"victim_id\":%d,\"real_ordinal\":%d,\"native_window\":%d,\"victim_window\":%d,\"row\":\"%p\",\"weak_index\":%d,\"weak_serial\":%d,\"attempted\":%s,\"completed\":%s,\"readback\":%s,\"numerals_visibility\":%u,\"fire_visibility\":%u}",n?",":"",r.sequence,r.token,r.player,r.victim,r.ordinal,r.window,r.victimWindow,reinterpret_cast<void*>(r.row.address),r.row.index,r.row.serial,b(r.attempted),b(r.completed),b(r.readback),static_cast<unsigned>(r.numerals),static_cast<unsigned>(r.fire));
    }
    out.Add("],\"retained\":%llu,\"attached_to_real_pending\":%llu,\"retained_lookups\":%llu,\"retired_receipts\":%llu,\"retention_full\":%llu,\"retention_conflicts\":%llu}",state.retained,state.attached,state.retainedLookups,state.retired,state.retentionFull,state.retentionConflicts);
    AcquireSRWLockExclusive(&lock);cached.swap(out.value);ReleaseSRWLockExclusive(&lock);
}
inline std::string Json(){AcquireSRWLockShared(&lock);const auto copy=cached;ReleaseSRWLockShared(&lock);return copy;}
inline void Poll(){
    if(!view::Thread())return;const auto now=GetTickCount64();if(now<state.nextMetadata)return;state.nextMetadata=now+100;
    auto h=Feed();
    if(!h||!view::Live(state.actualFeedClass,state.actualFeedClassId)||Read<void*>(h->object,0x10)!=state.actualFeedClass||!MethodsLive()){
        state.ready=h&&Metadata(*h);state.stage=state.ready?"pending_visual_original_metadata_verified_waiting_native_tick":"pending_visual_waiting_exact_original_metadata";
    }
    Cache();
}
} // namespace local_server_feedback_pending_view
