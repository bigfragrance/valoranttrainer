#pragma once
#include <cstdint>
#include <array>

// Sole caller: explicit Scenario12 inside the existing client fixture Tick.
// A wall deadline bounds the test only. No simulation clock is created.
namespace local_server_native_shop_plan {
enum class Phase:uint8_t {WarmOpen,WarmModels,WarmClose,WarmIdle,WaitSecondary,WaitMelee,WaitPrimary,
    RetryClose,PurchaseClose,Settle,Complete,NotObserved};
enum class Action:uint8_t {None,OpenWarm,CloseWarm,Secondary,Melee,Primary,OpenAndPurchase,CloseRetry,ClosePurchase,Complete,NotObserved};
inline constexpr uint32_t MaximumCycles=12,SwitchWindowMs=500,SettleMs=10000,PostPurchaseObserveMs=5000;
struct Observation {
    bool shopOpenKnown=false,shopOpen=false,modelsReady=false,slotsReady=false,idle=false;
    bool currentSlotKnown=false,transition=false,oldPrelude=false,targetHeld=false;
    uint8_t currentSlot=255;
};
struct Plan {
    Phase phase=Phase::WarmOpen;
    uint64_t started=0,phaseAt=0,warmAt=0,lastActionTick=0;
    uint32_t cycles=0;
    bool startedKnown=false,purchaseDispatched=false,notObserved=false;
};
inline void Move(Plan& p,Phase phase,uint64_t now,uint64_t tick){p.phase=phase;p.phaseAt=now;p.lastActionTick=tick;}
inline Action Miss(Plan& p){p.notObserved=true;p.phase=Phase::NotObserved;return Action::NotObserved;}
inline void PurchaseResult(Plan& p,bool dispatched,uint64_t now,uint64_t tick){
    p.purchaseDispatched=dispatched;
    Move(p,dispatched?Phase::PurchaseClose:Phase::RetryClose,now,tick);
}
inline Action Tick(Plan& p,const Observation& o,uint64_t now,uint64_t tick,uint32_t warmupMs,uint32_t totalMs){
    if(p.phase==Phase::Complete||p.phase==Phase::NotObserved)return Action::None;
    if(!p.startedKnown){p.startedKnown=true;p.started=p.phaseAt=now;}
    if(now<p.started||now-p.started>=totalMs)return Miss(p);
    if(tick<=p.lastActionTick)return Action::None;
    if(p.phase==Phase::WarmOpen){
        if(!o.shopOpenKnown)return Action::None;
        Move(p,Phase::WarmModels,now,tick);return o.shopOpen?Action::None:Action::OpenWarm;
    }
    if(p.phase==Phase::WarmModels){
        if(!o.shopOpen||!o.modelsReady)return Action::None;
        Move(p,Phase::WarmClose,now,tick);return Action::CloseWarm;
    }
    if(p.phase==Phase::WarmClose){
        if(!o.shopOpenKnown||o.shopOpen)return Action::None;
        Move(p,Phase::WarmIdle,now,tick);return Action::None;
    }
    if(p.phase==Phase::WarmIdle){
        if(!o.slotsReady||!o.idle||o.shopOpen){p.warmAt=0;return Action::None;}
        if(!p.warmAt)p.warmAt=now;
        if(now-p.warmAt<warmupMs)return Action::None;
        ++p.cycles;Move(p,Phase::WaitSecondary,now,tick);return Action::Secondary;
    }
    if(p.phase==Phase::WaitSecondary||p.phase==Phase::WaitMelee||p.phase==Phase::WaitPrimary){
        const uint8_t wanted=p.phase==Phase::WaitSecondary?1:p.phase==Phase::WaitMelee?2:0;
        if(o.currentSlotKnown&&o.currentSlot==wanted){
            if(p.phase==Phase::WaitSecondary){Move(p,Phase::WaitMelee,now,tick);return Action::Melee;}
            if(p.phase==Phase::WaitMelee){Move(p,Phase::WaitPrimary,now,tick);return Action::Primary;}
            // Both facts must be observed on this real native sample. A timer,
            // request, name or queued activation never substitutes for either.
            if(o.transition&&o.oldPrelude){p.lastActionTick=tick;return Action::OpenAndPurchase;}
        }
        if(now-p.phaseAt<SwitchWindowMs)return Action::None;
        if(p.cycles>=MaximumCycles)return Miss(p);
        ++p.cycles;Move(p,Phase::WaitSecondary,now,tick);return Action::Secondary;
    }
    if(p.phase==Phase::RetryClose){
        if(o.shopOpenKnown&&o.shopOpen){p.lastActionTick=tick;return Action::CloseRetry;}
        if(!o.shopOpenKnown)return Action::None;
        if(p.cycles>=MaximumCycles)return Miss(p);
        ++p.cycles;Move(p,Phase::WaitSecondary,now,tick);return Action::Secondary;
    }
    if(p.phase==Phase::PurchaseClose){
        if(!o.shopOpenKnown)return Action::None;
        if(o.shopOpen){p.lastActionTick=tick;return Action::ClosePurchase;}
        Move(p,Phase::Settle,now,tick);return Action::None;
    }
    if(p.phase==Phase::Settle){
        if(o.targetHeld&&o.idle&&!o.shopOpen&&now-p.phaseAt>=PostPurchaseObserveMs){p.phase=Phase::Complete;return Action::Complete;}
        if(now-p.phaseAt>=SettleMs)return Miss(p);
    }
    return Action::None;
}
} // namespace local_server_native_shop_plan
