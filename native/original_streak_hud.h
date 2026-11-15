#pragma once
#include "original_inventory_slots.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

// Observe the shipped timed-streak FXC and its real UMG widget. The original
// Comp_TimedKillStreak owns spawning, timers, text, animation and visibility.
// This adapter never creates a widget, writes HUD state or changes a view gate.
namespace original_streak_hud {
inline constexpr const wchar_t* EffectPath=L"/Game/GameModes/FXC/FXC_GameMode_FFA_KillStreak.FXC_GameMode_FFA_KillStreak_C";
inline constexpr const wchar_t* ComponentPath=L"/Game/Characters/Components/Comp_FXC_HUD.Comp_FXC_HUD_C";
inline constexpr const wchar_t* WidgetPath=L"/Game/UI/InGame/HUD/GameInfoHUD/FFA/FFAKillstreak_Burndown.FFAKillstreak_Burndown_C";
inline constexpr size_t Capacity=256,HistoryCapacity=64,TextCapacity=128,SegmentCapacity=32;
using WeakIdentity=original_inventory_slots::WeakIdentity;
struct Name {uint32_t comparison=0,display=0,number=0;};
struct Property {void *field=nullptr,*fieldClass=nullptr;int32_t offset=-1,bytes=0;uint8_t byteOffset=0,byteMask=0,fieldMask=0;};
struct Layout {
    Property component{},target{},perspective{},hudElement{},source{},allowThird{},active{},hudClass{};
    Property duration{},timer{},bar{},barPercent{},text{},division{};
};
struct Methods {
    void *actors=nullptr,*owner=nullptr,*viewport=nullptr,*visible=nullptr,*visibility=nullptr,*tick=nullptr,*parent=nullptr;
    void *remaining=nullptr,*text=nullptr,*textToString=nullptr,*childCount=nullptr,*childAt=nullptr;
};
struct Observation {
    void *effect=nullptr,*target=nullptr,*component=nullptr,*source=nullptr,*widget=nullptr,*owner=nullptr,*parent=nullptr,*playerState=nullptr;
    void *chargeBar=nullptr,*textBlock=nullptr,*divisionBox=nullptr;
    int32_t rosterIndex=-1,segmentChildren=0,visibleSegments=0;
    uint8_t visibility=255,segmentVisibility[SegmentCapacity]{};
    bool effectLive=false,componentLive=false,widgetLive=false,stopped=false,firstPerson=false;
    bool sourceCachedFirstPerson=false,currentSpectatedSource=false,knownSpectatedCache=false;
    bool actualOwnSource=false,sameLocalPlayerState=false,allowThirdPerson=false,hudActive=false,tickEnabled=false;
    bool inViewport=false,visible=false,correctWidgetClass=false,correctHUDClass=false,ownerIsActualLocal=false;
    bool textQueried=false,textTruncated=false,segmentsQueried=false,timerQueried=false;
    double duration=0;float percent=0,timerRemaining=-1;uint64_t timer=0;
    wchar_t displayedText[TextCapacity]{};
};
struct Window {
    uint64_t sequence=0,atMs=0;
    uint32_t effects=0,ownWidgets=0,ownVisible=0,foreignWidgets=0,foreignInViewport=0;
    uint32_t spectatedWidgets=0,cachedSpectatedWidgets=0;
    double ownDuration=0;float ownPercent=0,ownTimerRemaining=-1;
    int32_t ownSegmentChildren=0,ownVisibleSegments=0;
    void* ownWidget=nullptr;void* ownOwningPlayer=nullptr;
    wchar_t ownDisplayedText[TextCapacity]{};
};
struct Diagnostics {
    bool initialized=false,profileValid=false,restartRequired=false,actualLocalPlayer=false;
    bool originalAllowThirdPersonFalse=false,noForeignHUD=true;
    unsigned error=0;const char* stage="original_streak_hud_not_initialized";
    const wchar_t* failedEvent=nullptr;uint16_t expectedEventSize=0,actualEventSize=0;
    DWORD gameThread=0;uintptr_t exceptionRva=0;
    void *world=nullptr,*effectClass=nullptr,*componentClass=nullptr,*widgetClass=nullptr;
    void *gameplay=nullptr,*timerLibrary=nullptr,*textLibrary=nullptr;
    uint64_t samples=0,lastPollMs=0,textReads=0,textDestructors=0,stringFrees=0,arrayFrees=0;
    // This module only observes. Sampling cadence never drives widget tick,
    // charge percentage, native timer, visibility, text or FXC lifecycle.
    uint32_t inspectionCadenceMs=1000;
    uint64_t pollCalls=0,throttledPolls=0,forcedSamples=0,enumerations=0,actorsEnumerated=0;
    uint32_t effects=0,observations=0,ownWidgets=0,ownVisible=0,foreignWidgets=0,foreignInViewport=0;
    uint32_t spectatedWidgets=0,cachedSpectatedWidgets=0;
    uint32_t maxOwnVisible=0,maxForeignWidgets=0,maxForeignInViewport=0,unresolvedIdentities=0;
    std::array<Observation,Capacity> rows{};
    std::array<Window,HistoryCapacity> history{};
};
inline Diagnostics state;
inline Layout layout;
inline Methods methods;
inline const Diagnostics& State(){return state;}
inline bool SetInspectionCadence(uint32_t milliseconds){
    if(milliseconds<100||milliseconds>10000)return false;
    state.inspectionCadenceMs=milliseconds;return true;
}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;return false;}
inline bool Thread(){return state.gameThread==GetCurrentThreadId()&&original_players::Thread()&&!state.restartRequired;}
inline bool Event(void* object,void* function,void* params){
    auto process=Read<void*>(Read<void**>(object,0),0x298);if(!object||!function||!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,function,params);return true;
}
inline bool Bind(void*& result,const wchar_t* name,uint16_t bytes,uintptr_t native=0){
    result=engine::Find(name);const uint16_t actual=Read<uint16_t>(result,0xbe);
    if(result&&actual==bytes&&(!native||((Read<uint32_t>(result,0xb8)&0x400)!=0
       &&Read<uintptr_t>(result,0xe0)==engine::image+native)))return true;
    state.failedEvent=name;state.expectedEventSize=bytes;state.actualEventSize=actual;return false;
}
inline Property Field(void* type,const wchar_t* name,int32_t bytes,bool boolean=false){
    Property p;Name wanted{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&wanted,name,1);
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&wanted,3);
    auto cls=Read<void*>(field,8);const auto flags=Read<uint64_t>(cls,0x18);
    const int32_t offset=Read<int32_t>(field,0x44),size=Read<int32_t>(type,0x60);
    if(!field||(flags&0x8001)!=0x8001||Read<uint64_t>(field,0x20)!=Read<uint64_t>(&wanted,0)
       ||Read<int32_t>(field,0x30)!=1||Read<int32_t>(field,0x34)!=bytes||offset<0||size<bytes||offset>size-bytes)return p;
    if(boolean){
        if((flags&0x28001)!=0x28001||Read<uint8_t>(field,0x78)!=1)return p;
        p.byteOffset=Read<uint8_t>(field,0x79);p.byteMask=Read<uint8_t>(field,0x7a);p.fieldMask=Read<uint8_t>(field,0x7b);
        if(p.byteOffset!=0||!p.byteMask||!p.fieldMask)return Property{};
    }
    p.field=field;p.fieldClass=cls;p.offset=offset;p.bytes=bytes;return p;
}
inline bool Flag(void* object,const Property& p){return p.field&&(Read<uint8_t>(object,static_cast<size_t>(p.offset)+p.byteOffset)&p.fieldMask)!=0;}
inline void* Object(void* object,const Property& p){return p.field?Read<void*>(object,static_cast<size_t>(p.offset)):nullptr;}
inline bool Live(void* object){WeakIdentity identity{};return object&&original_inventory_slots::CaptureIdentity(object,identity)
    &&original_inventory_slots::ResolveIdentity(identity)==object;}
inline void* Load(void* base,const wchar_t* name){
    return engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t,void*)>(0x1d0c6e0)(base,nullptr,name,nullptr,0,nullptr);
}
inline bool ActualLocal(){
    auto pc=original_players::activeController,lp=original_players::localPlayer;
    auto gi=Read<void*>(state.world,0x1d8),locals=Read<void*>(gi,0x40);
    return pc&&lp&&state.world==original_players::world&&Read<int32_t>(gi,0x48)==1&&Read<int32_t>(gi,0x4c)>=1
        &&Read<void*>(locals,0)==lp&&Read<void*>(lp,0x38)==pc&&Read<void*>(pc,0x510)==lp;
}
// DLL-only provenance: never retain an engine reference or refresh an old
// target weak identity. A former source gets no current-feedback permission.
using SpectatedSourceCheck=bool(*)(void* world,const Observation& row);
struct SourceBinding {
    SpectatedSourceCheck check=nullptr;void *world=nullptr,*controller=nullptr,*hud=nullptr;
    std::array<WeakIdentity,3> identities{};
};
struct SpectatedRecord {
    bool used=false; // effect, component, historical pawn/PS, widget, owner, HUD, world
    std::array<void*,8> objects{};std::array<WeakIdentity,8> identities{};
};
inline SourceBinding sourceBinding{};
inline std::array<SpectatedRecord,Capacity> spectatedRecords{};
inline bool KnownLive(void* object,const WeakIdentity& id){return object&&original_inventory_slots::ResolveIdentity(id)==object;}
inline bool SameWeak(const WeakIdentity&a,const WeakIdentity&b){return a.index==b.index&&a.serial==b.serial;}
inline bool BindingLive(){
    const auto&b=sourceBinding;
    return b.check&&b.world==state.world&&b.controller==original_players::activeController&&ActualLocal()
        &&KnownLive(b.world,b.identities[0])&&KnownLive(b.controller,b.identities[1])&&KnownLive(b.hud,b.identities[2])
        &&Read<void*>(b.controller,0x520)==b.hud&&Read<void*>(b.hud,0x468)==b.controller;
}
inline bool BindSpectatedSource(SpectatedSourceCheck check,void*world,void*controller,void*hud){
    if(!state.initialized||!Thread()||!check||world!=state.world||controller!=original_players::activeController||!ActualLocal()
       ||!original_players::IsA(hud,0x6cae7d0)||Read<void*>(controller,0x520)!=hud||Read<void*>(hud,0x468)!=controller)
        return Fail(3932,"original_streak_hud_source_binding_requires_actual_local_hud");
    SourceBinding next;next.check=check;next.world=world;next.controller=controller;next.hud=hud;
    const std::array<void*,3>objects{{world,controller,hud}};
    for(size_t n=0;n<objects.size();++n)if(!original_inventory_slots::CaptureIdentity(objects[n],next.identities[n])
       ||!KnownLive(objects[n],next.identities[n]))return Fail(3932,"original_streak_hud_source_binding_identity_unavailable");
    bool same=sourceBinding.check==check&&sourceBinding.world==world&&sourceBinding.controller==controller&&sourceBinding.hud==hud;
    for(size_t n=0;n<objects.size();++n)same=same&&SameWeak(sourceBinding.identities[n],next.identities[n]);
    if(!same)spectatedRecords={};sourceBinding=next;return true;
}
inline bool RecordSpectated(const Observation&row){
    SpectatedRecord next;next.used=true;next.objects={{row.effect,row.component,row.target,row.playerState,row.widget,row.owner,sourceBinding.hud,state.world}};
    for(size_t n=0;n<next.objects.size();++n)if(!original_inventory_slots::CaptureIdentity(next.objects[n],next.identities[n])
       ||!KnownLive(next.objects[n],next.identities[n]))return false;
    SpectatedRecord*slot=nullptr;
    for(auto&old:spectatedRecords){
        if(old.used&&old.objects[4]==row.widget){
            // The same native FXC/component/widget cannot silently promote a
            // recorded old subject to a new pawn/PS life at a reused address.
            if(SameWeak(old.identities[4],next.identities[4])&&SameWeak(old.identities[0],next.identities[0])
               &&SameWeak(old.identities[1],next.identities[1])
               &&(!SameWeak(old.identities[2],next.identities[2])||!SameWeak(old.identities[3],next.identities[3])))return false;
            slot=&old;break;
        }
        if(!slot&&(!old.used||!KnownLive(old.objects[0],old.identities[0])||!KnownLive(old.objects[1],old.identities[1])
           ||!KnownLive(old.objects[4],old.identities[4])))slot=&old;
    }
    if(!slot)return false;*slot=next;return true;
}
inline bool KnownSpectatedCache(const Observation&row){
    if(!BindingLive()||!row.ownerIsActualLocal||row.parent||row.inViewport||row.visible)return false;
    for(const auto&old:spectatedRecords){
        if(!old.used||old.objects[0]!=row.effect||old.objects[1]!=row.component||old.objects[4]!=row.widget
           ||old.objects[5]!=row.owner||old.objects[6]!=sourceBinding.hud||old.objects[7]!=state.world)continue;
        bool live=true;for(auto n:{0U,1U,4U,5U,6U,7U})live=live&&KnownLive(old.objects[n],old.identities[n]);
        if(!live)continue;
        // Retired pawn/PS identities are historical evidence only. Do not
        // resolve a new life from their old raw address or grant feedback.
        const bool sameSource=row.target==old.objects[2]&&row.source==old.objects[2]
            &&(!KnownLive(old.objects[2],old.identities[2])||row.playerState==old.objects[3]);
        const bool stoppedCleared=row.stopped&&!row.hudActive&&!row.tickEnabled
            &&(!row.target||row.target==old.objects[2])&&(!row.source||row.source==old.objects[2]);
        if(sameSource||stoppedCleared)return true;
    }return false;
}
enum class WidgetSource:uint8_t {Local,Spectated,KnownCache,Foreign};
inline WidgetSource ClassifyWidget(Observation&row){
    row.currentSpectatedSource=row.knownSpectatedCache=false;
    if(row.ownerIsActualLocal&&Read<uint8_t>(row.target,0x224)!=1&&(row.actualOwnSource||row.sameLocalPlayerState))return WidgetSource::Local;
    if(BindingLive()&&row.ownerIsActualLocal&&row.target&&row.source==row.target
       &&sourceBinding.check(state.world,row)&&BindingLive()&&RecordSpectated(row)){
        row.currentSpectatedSource=true;return WidgetSource::Spectated;
    }
    if(KnownSpectatedCache(row)){row.knownSpectatedCache=true;return WidgetSource::KnownCache;}
    return WidgetSource::Foreign;
}
inline bool InitializeImpl(void* world){
    if(state.initialized)return state.world==world&&Thread();
    if(!world||world!=original_players::world||!original_players::Thread())return Fail(3920,"original_streak_hud_world_or_thread_invalid");
    state.world=world;state.gameThread=GetCurrentThreadId();
    auto effect=engine::Class(0x648ada0),component=engine::Class(0x63abb50);
    auto widget=engine::Find(L"/Script/UMG.UserWidget");
    state.effectClass=Load(effect,EffectPath);state.componentClass=Load(component,ComponentPath);state.widgetClass=Load(widget,WidgetPath);
    if(!original_players::Derived(state.effectClass,effect)||!original_players::Derived(state.componentClass,component)
       ||!original_players::Derived(state.widgetClass,widget))return Fail(3921,"original_streak_hud_original_classes_missing");
    const bool bound=Bind(methods.actors,L"/Script/Engine.GameplayStatics.GetAllActorsOfClass",32,0x43eece0)
        &&Bind(methods.owner,L"/Script/UMG.Widget.GetOwningPlayer",8,0x3bbeb60)
        &&Bind(methods.viewport,L"/Script/UMG.Widget.IsInViewport",1,0x3bc1be0)
        &&Bind(methods.visible,L"/Script/UMG.Widget.IsVisible",1,0x3bc1670)
        &&Bind(methods.visibility,L"/Script/UMG.Widget.GetVisibility",1,0x3bc1640)
        &&Bind(methods.tick,L"/Script/Engine.ActorComponent.IsComponentTickEnabled",1,0x4129570)
        &&Bind(methods.remaining,L"/Script/Engine.KismetSystemLibrary.K2_GetTimerRemainingTimeHandle",20,0x45cc560)
        &&Bind(methods.text,L"/Script/UMG.TextBlock.GetText",24,0x3bb39f0)
        &&Bind(methods.textToString,L"/Script/Engine.KismetTextLibrary.Conv_TextToString",40,0x3e25250)
        &&Bind(methods.childCount,L"/Script/UMG.PanelWidget.GetChildrenCount",4)
        &&Bind(methods.childAt,L"/Script/UMG.PanelWidget.GetChildAt",16,0x3b96370)
        &&Bind(methods.parent,L"/Script/UMG.Widget.GetParent",8);
    if(!bound)return Fail(3922,"original_streak_hud_read_only_ufunction_profile_mismatch");
    layout.component=Field(state.effectClass,L"Comp_FXC_HUD",8);layout.target=Field(effect,L"OriginalTarget",8);
    layout.perspective=Field(effect,L"bCurrentPerspectiveIsFirstPerson",1,true);
    layout.hudElement=Field(state.componentClass,L"HUD_Element",8);layout.source=Field(state.componentClass,L"ShooterCharacter",8);
    layout.allowThird=Field(state.componentClass,L"AllowThirdPerson",1,true);layout.active=Field(state.componentClass,L"HUD_Active",1,true);
    layout.hudClass=Field(state.componentClass,L"HUD_Class",8);layout.duration=Field(state.widgetClass,L"BuffDuration",8);
    layout.timer=Field(state.widgetClass,L"StreakDurationTimer",8);layout.bar=Field(state.widgetClass,L"ChargeBar",8);
    layout.text=Field(state.widgetClass,L"KillStreakText",8);layout.division=Field(state.widgetClass,L"DivisionBox",8);
    layout.barPercent=Field(engine::Find(L"/Script/UMG.ProgressBar"),L"Percent",4);
    state.profileValid=layout.component.field&&layout.target.offset==0x620&&layout.perspective.offset==0x55d
        &&layout.hudElement.field&&layout.source.field&&layout.allowThird.field&&layout.active.field&&layout.hudClass.field
        &&layout.duration.field&&layout.timer.field&&layout.bar.field&&layout.barPercent.field&&layout.text.field&&layout.division.field;
    if(!state.profileValid)return Fail(3923,"original_streak_hud_reflected_property_profile_mismatch");
    state.originalAllowThirdPersonFalse=!Flag(engine::DefaultObject(state.componentClass),layout.allowThird);
    if(!state.originalAllowThirdPersonFalse)return Fail(3924,"original_streak_hud_original_first_person_default_changed");
    state.gameplay=engine::DefaultObject(engine::Class(0x43a9670));state.timerLibrary=Read<void*>(methods.remaining,0x28);
    state.timerLibrary=engine::DefaultObject(state.timerLibrary);state.textLibrary=engine::DefaultObject(engine::Class(0x3e1ec70));
    state.actualLocalPlayer=ActualLocal();
    if(!state.gameplay||!state.timerLibrary||!state.textLibrary||!state.actualLocalPlayer)return Fail(3925,"original_streak_hud_actual_local_player_missing");
    state.initialized=true;state.error=0;state.stage="original_streak_hud_waiting_for_original_effect";return true;
}
inline bool Text(Observation& o){
    struct Value {alignas(8) uint8_t text[24]{};wchar_t* string=nullptr;int32_t count=0,capacity=0;} value;
    static_assert(sizeof(Value)==40&&offsetof(Value,string)==24);
    if(!original_players::IsA(o.textBlock,0x3bb25c0))return false;
    const bool got=Event(o.textBlock,methods.text,&value);
    const bool converted=got&&Event(state.textLibrary,methods.textToString,&value);
    o.textQueried=converted&&value.count>=0&&value.capacity>=value.count&&value.count<=4096
        &&(!value.count||original_players::Readable(value.string,static_cast<size_t>(value.count)*sizeof(wchar_t)));
    if(o.textQueried&&value.count>0){
        const int32_t length=value.count-1;o.textTruncated=length>=static_cast<int32_t>(TextCapacity);
        const int32_t count=o.textTruncated?static_cast<int32_t>(TextCapacity)-1:length;
        for(int32_t i=0;i<count;++i)o.displayedText[i]=value.string[i];o.displayedText[count]=0;
    }
    if(value.string){engine::function<void(*)(void*)>(0x1915720)(value.string);++state.stringFrees;}
    if(got){engine::function<void(*)(void*)>(0x18a31e0)(&value);++state.textDestructors;}
    ++state.textReads;return o.textQueried;
}
inline bool Segments(Observation& o){
    if(!o.divisionBox||!Event(o.divisionBox,methods.childCount,&o.segmentChildren)
       ||o.segmentChildren<0||o.segmentChildren>static_cast<int32_t>(SegmentCapacity))return false;
    for(int32_t i=0;i<o.segmentChildren;++i){
        struct Child {int32_t index=0,padding=0;void* result=nullptr;} child{i};
        static_assert(sizeof(Child)==16);
        if(!Event(o.divisionBox,methods.childAt,&child)||!child.result||!Event(child.result,methods.visibility,&o.segmentVisibility[i]))return false;
        const uint8_t visibility=o.segmentVisibility[i];if(visibility==0||visibility==3||visibility==4)++o.visibleSegments;
    }
    o.segmentsQueried=true;return true;
}
inline bool Observe(void* actor,Observation& o){
    o.effect=actor;o.effectLive=Live(actor);if(!o.effectLive){++state.unresolvedIdentities;return true;}
    o.target=Object(actor,layout.target);o.firstPerson=Flag(actor,layout.perspective);o.stopped=Read<uint8_t>(actor,0x5fc)!=0;
    o.sourceCachedFirstPerson=Read<uint8_t>(o.target,0xf54)!=0;o.playerState=Read<void*>(o.target,0x488);
    for(size_t i=0;i<original_players::players.size();++i)if(original_players::players[i].character==o.target){o.rosterIndex=static_cast<int32_t>(i);break;}
    o.actualOwnSource=o.target==original_players::activePawn&&Read<void*>(o.target,0x4a0)==original_players::activeController;
    const auto local=original_players::localIndex;
    o.sameLocalPlayerState=local>=0&&static_cast<size_t>(local)<original_players::players.size()
        &&Read<void*>(o.target,0x488)==original_players::players[static_cast<size_t>(local)].state;
    o.component=Object(actor,layout.component);o.componentLive=Live(o.component);
    if(!o.componentLive||Read<void*>(o.component,0x10)!=state.componentClass||Read<void*>(o.component,0xc8)!=actor)
        return Fail(3926,"original_streak_hud_effect_component_identity_invalid");
    o.source=Object(o.component,layout.source);o.allowThirdPerson=Flag(o.component,layout.allowThird);
    o.hudActive=Flag(o.component,layout.active);o.correctHUDClass=Object(o.component,layout.hudClass)==state.widgetClass;
    uint8_t tick=0;if(!Event(o.component,methods.tick,&tick))return Fail(3927,"original_streak_hud_tick_observation_failed");o.tickEnabled=tick!=0;
    if(!o.correctHUDClass||o.allowThirdPerson)return Fail(3928,"original_streak_hud_shipped_first_person_profile_changed");
    o.widget=Object(o.component,layout.hudElement);if(!o.widget)return true;
    o.widgetLive=Live(o.widget);o.correctWidgetClass=Read<void*>(o.widget,0x10)==state.widgetClass;
    if(!o.widgetLive||!o.correctWidgetClass)return Fail(3929,"original_streak_hud_original_widget_identity_invalid");
    uint8_t viewport=0,visible=0;
    if(!Event(o.widget,methods.owner,&o.owner)||!Event(o.widget,methods.viewport,&viewport)
       ||!Event(o.widget,methods.visible,&visible)||!Event(o.widget,methods.visibility,&o.visibility)||!Event(o.widget,methods.parent,&o.parent))
        return Fail(3930,"original_streak_hud_original_screen_queries_failed");
    o.inViewport=viewport!=0;o.visible=visible!=0;o.ownerIsActualLocal=o.owner==original_players::activeController;
    o.duration=Read<double>(o.widget,static_cast<size_t>(layout.duration.offset));o.timer=Read<uint64_t>(o.widget,static_cast<size_t>(layout.timer.offset));
    o.chargeBar=Object(o.widget,layout.bar);o.percent=Read<float>(o.chargeBar,static_cast<size_t>(layout.barPercent.offset));
    o.textBlock=Object(o.widget,layout.text);o.divisionBox=Object(o.widget,layout.division);
    struct Timer {void* world;uint64_t handle;float result=-1;uint32_t padding=0;} timer{state.world,o.timer};
    static_assert(sizeof(Timer)==24&&offsetof(Timer,result)==16);
    o.timerQueried=Event(state.timerLibrary,methods.remaining,&timer)&&std::isfinite(timer.result);o.timerRemaining=timer.result;
    return std::isfinite(o.duration)&&std::isfinite(o.percent)&&o.timerQueried&&Text(o)&&Segments(o)
        ?true:Fail(3931,"original_streak_hud_original_widget_content_query_failed");
}
inline bool SampleImpl(bool force){
    if(!state.initialized||!Thread())return Fail(3932,"original_streak_hud_sample_requires_original_game_thread");
    ++state.pollCalls;
    const uint64_t now=GetTickCount64();
    if(!force&&now-state.lastPollMs<state.inspectionCadenceMs){++state.throttledPolls;return true;}
    if(force)++state.forcedSamples;state.lastPollMs=now;
    state.actualLocalPlayer=ActualLocal();if(!state.actualLocalPlayer)return Fail(3933,"original_streak_hud_actual_local_player_changed");
    struct Query {void* world;void* type;void** actors=nullptr;int32_t count=0,capacity=0;} query{state.world,state.effectClass};
    static_assert(sizeof(Query)==32);
    ++state.enumerations;
    if(!Event(state.gameplay,methods.actors,&query))return Fail(3934,"original_streak_hud_original_effect_enumeration_failed");
    bool ok=query.count>=0&&query.count<=static_cast<int32_t>(Capacity)&&query.capacity>=query.count
        &&(!query.count||original_players::Readable(query.actors,static_cast<size_t>(query.count)*sizeof(void*)));
    state.effects=ok?static_cast<uint32_t>(query.count):0;state.observations=0;state.ownWidgets=0;state.ownVisible=0;
    if(ok)state.actorsEnumerated+=static_cast<uint64_t>(query.count);
    state.foreignWidgets=0;state.foreignInViewport=0;state.spectatedWidgets=state.cachedSpectatedWidgets=0;
    if(ok){
        for(int32_t i=0;i<query.count;++i){
            auto& row=state.rows[static_cast<size_t>(i)];row={};++state.observations;
            if(!Observe(query.actors[i],row)){ok=false;break;}
            if(row.widgetLive){
                const auto source=ClassifyWidget(row);
                if(source==WidgetSource::Local){++state.ownWidgets;if(row.inViewport&&row.visible)++state.ownVisible;}
                else if(source==WidgetSource::Spectated)++state.spectatedWidgets;
                else if(source==WidgetSource::KnownCache)++state.cachedSpectatedWidgets;
                else{++state.foreignWidgets;if(row.inViewport)++state.foreignInViewport;}
            }
        }
    }
    if(query.actors){engine::function<void(*)(void*)>(0x1915720)(query.actors);++state.arrayFrees;}
    if(!ok)return state.error?false:Fail(3935,"original_streak_hud_effect_array_not_bounded");
    if(state.ownVisible>state.maxOwnVisible)state.maxOwnVisible=state.ownVisible;
    if(state.foreignWidgets>state.maxForeignWidgets)state.maxForeignWidgets=state.foreignWidgets;
    if(state.foreignInViewport>state.maxForeignInViewport)state.maxForeignInViewport=state.foreignInViewport;
    state.noForeignHUD=state.foreignWidgets==0&&state.foreignInViewport==0;
    auto& window=state.history[static_cast<size_t>(state.samples%HistoryCapacity)];window={};window.sequence=++state.samples;window.atMs=now;
    window.effects=state.effects;window.ownWidgets=state.ownWidgets;window.ownVisible=state.ownVisible;
    window.foreignWidgets=state.foreignWidgets;window.foreignInViewport=state.foreignInViewport;
    window.spectatedWidgets=state.spectatedWidgets;window.cachedSpectatedWidgets=state.cachedSpectatedWidgets;
    for(uint32_t i=0;i<state.observations;++i){const auto& row=state.rows[i];if(row.actualOwnSource&&row.ownerIsActualLocal&&row.widgetLive){
        window.ownDuration=row.duration;window.ownPercent=row.percent;window.ownTimerRemaining=row.timerRemaining;
        window.ownSegmentChildren=row.segmentChildren;window.ownVisibleSegments=row.visibleSegments;
        window.ownWidget=row.widget;window.ownOwningPlayer=row.owner;
        std::memcpy(window.ownDisplayedText,row.displayedText,sizeof(window.ownDisplayedText));break;}}
    state.error=0;state.stage=state.effects?"original_streak_hud_original_effects_observed":"original_streak_hud_waiting_for_original_effect";
    return state.noForeignHUD?true:Fail(3936,"original_streak_hud_foreign_source_created_player_hud");
}
inline int Exception(EXCEPTION_POINTERS* p){
    state.restartRequired=true;state.exceptionRva=reinterpret_cast<uintptr_t>(p&&p->ExceptionRecord?p->ExceptionRecord->ExceptionAddress:nullptr);
    if(state.exceptionRva>=engine::image)state.exceptionRva-=engine::image;
    Fail(3937,"original_streak_hud_native_observation_exception");return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* world){__try{return InitializeImpl(world);}__except(Exception(GetExceptionInformation())){return false;}}
inline bool Sample(bool force=false){__try{return SampleImpl(force);}__except(Exception(GetExceptionInformation())){return false;}}
inline bool Poll(){return Sample(false);}
inline void JsonText(FILE* file,const wchar_t* text){
    char utf8[TextCapacity*4]{};const int count=WideCharToMultiByte(CP_UTF8,0,text,-1,utf8,static_cast<int>(sizeof(utf8)),nullptr,nullptr);
    std::fputc('"',file);if(count>0)for(int i=0;i<count-1;++i){const auto ch=static_cast<unsigned char>(utf8[i]);
        if(ch=='"'||ch=='\\')std::fputc('\\',file);if(ch<32)std::fprintf(file,"\\u%04x",static_cast<unsigned>(ch));else std::fputc(ch,file);}
    std::fputc('"',file);
}
inline void WriteDiagnostics(const wchar_t* path){
    FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return;
    std::fprintf(file,"{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"profile_valid\":%s,\"restart_required\":%s,"
        "\"actual_local_player\":%s,\"original_allow_third_person_false\":%s,\"no_foreign_hud\":%s,\"exception_rva\":\"0x%llx\","
        "\"samples\":%llu,\"effects\":%u,\"own_widgets\":%u,\"own_visible\":%u,\"foreign_widgets\":%u,\"foreign_in_viewport\":%u,"
        "\"max_own_visible\":%u,\"max_foreign_widgets\":%u,\"max_foreign_in_viewport\":%u,\"unresolved_identities\":%u,"
        "\"text_reads\":%llu,\"text_destructors\":%llu,\"string_frees\":%llu,\"array_frees\":%llu",
        state.stage,state.error,state.initialized?"true":"false",state.profileValid?"true":"false",state.restartRequired?"true":"false",
        state.actualLocalPlayer?"true":"false",state.originalAllowThirdPersonFalse?"true":"false",state.noForeignHUD?"true":"false",
        static_cast<unsigned long long>(state.exceptionRva),static_cast<unsigned long long>(state.samples),state.effects,state.ownWidgets,state.ownVisible,
        state.foreignWidgets,state.foreignInViewport,state.maxOwnVisible,state.maxForeignWidgets,state.maxForeignInViewport,state.unresolvedIdentities,
        static_cast<unsigned long long>(state.textReads),static_cast<unsigned long long>(state.textDestructors),
        static_cast<unsigned long long>(state.stringFrees),static_cast<unsigned long long>(state.arrayFrees));
    std::fprintf(file,",\"spectated_widgets\":%u,\"cached_spectated_widgets\":%u,\"rows\":[",state.spectatedWidgets,state.cachedSpectatedWidgets);
    for(uint32_t i=0;i<state.observations&&i<Capacity;++i){const auto& o=state.rows[i];if(i)std::fputc(',',file);
        std::fprintf(file,"{\"effect\":\"%p\",\"target\":\"%p\",\"component\":\"%p\",\"source\":\"%p\",\"widget\":\"%p\",\"owning_player\":\"%p\","
            "\"roster_index\":%d,\"actual_own_source\":%s,\"same_local_player_state\":%s,\"first_person\":%s,\"source_cached_first_person\":%s,\"stopped\":%s,\"allow_third_person\":%s,"
            "\"hud_active\":%s,\"component_tick_enabled\":%s,\"widget_live\":%s,\"original_widget_class\":%s,\"owner_is_actual_local\":%s,"
            "\"in_viewport\":%s,\"visible\":%s,\"visibility\":%u,\"buff_duration\":%.9g,\"timer_handle\":\"0x%llx\",\"timer_remaining\":%.9g,\"charge_percent\":%.9g,"
            "\"text_queried\":%s,\"text_truncated\":%s,\"displayed_text\":",
            o.effect,o.target,o.component,o.source,o.widget,o.owner,o.rosterIndex,o.actualOwnSource?"true":"false",o.sameLocalPlayerState?"true":"false",
            o.firstPerson?"true":"false",o.sourceCachedFirstPerson?"true":"false",o.stopped?"true":"false",o.allowThirdPerson?"true":"false",o.hudActive?"true":"false",o.tickEnabled?"true":"false",
            o.widgetLive?"true":"false",o.correctWidgetClass?"true":"false",o.ownerIsActualLocal?"true":"false",o.inViewport?"true":"false",o.visible?"true":"false",
            static_cast<unsigned>(o.visibility),o.duration,static_cast<unsigned long long>(o.timer),static_cast<double>(o.timerRemaining),static_cast<double>(o.percent),
            o.textQueried?"true":"false",o.textTruncated?"true":"false");JsonText(file,o.displayedText);
        std::fprintf(file,",\"current_spectated_source\":%s,\"known_spectated_cache\":%s,\"parent\":\"%p\"",o.currentSpectatedSource?"true":"false",o.knownSpectatedCache?"true":"false",o.parent);
        std::fprintf(file,",\"segment_children\":%d,\"visible_segment_children\":%d,\"segments_queried\":%s,\"segment_visibility\":[",
            o.segmentChildren,o.visibleSegments,o.segmentsQueried?"true":"false");
        for(int32_t j=0;j<o.segmentChildren&&j<static_cast<int32_t>(SegmentCapacity);++j)std::fprintf(file,"%s%u",j?",":"",static_cast<unsigned>(o.segmentVisibility[j]));
        std::fputs("]}",file);
    }
    std::fputs("],\"history\":[",file);const uint64_t first=state.samples>HistoryCapacity?state.samples-HistoryCapacity:0;
    for(uint64_t i=first;i<state.samples;++i){const auto& w=state.history[static_cast<size_t>(i%HistoryCapacity)];
        std::fprintf(file,"%s{\"sequence\":%llu,\"at_ms\":%llu,\"effects\":%u,\"own_widgets\":%u,\"own_visible\":%u,\"foreign_widgets\":%u,\"foreign_in_viewport\":%u,"
            "\"own_buff_duration\":%.9g,\"own_charge_percent\":%.9g,\"own_timer_remaining\":%.9g,\"own_widget\":\"%p\",\"own_owning_player\":\"%p\","
            "\"own_segment_children\":%d,\"own_visible_segment_children\":%d,\"own_displayed_text\":",i==first?"":",",
            static_cast<unsigned long long>(w.sequence),static_cast<unsigned long long>(w.atMs),w.effects,w.ownWidgets,w.ownVisible,w.foreignWidgets,w.foreignInViewport,
            w.ownDuration,static_cast<double>(w.ownPercent),static_cast<double>(w.ownTimerRemaining),w.ownWidget,w.ownOwningPlayer,
            w.ownSegmentChildren,w.ownVisibleSegments);JsonText(file,w.ownDisplayedText);std::fputc('}',file);}
    std::fprintf(file,"],\"inspection_cadence_ms\":%u,\"poll_calls\":%llu,\"throttled_polls\":%llu,\"forced_samples\":%llu,\"enumerations\":%llu,\"actors_enumerated\":%llu,\"sampling_controls_gameplay_or_ui\":false}\n",
        state.inspectionCadenceMs,static_cast<unsigned long long>(state.pollCalls),static_cast<unsigned long long>(state.throttledPolls),static_cast<unsigned long long>(state.forcedSamples),static_cast<unsigned long long>(state.enumerations),static_cast<unsigned long long>(state.actorsEnumerated));std::fclose(file);
}
} // namespace original_streak_hud
