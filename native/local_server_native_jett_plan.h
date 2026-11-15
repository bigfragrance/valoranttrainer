#pragma once
#include <cstdint>

// Decision-only part of the explicit development input fixture. Production
// callers supply observations of original state; no predicted motion or skill
// result is produced here. Native release readback is required before advancing.
namespace local_server_native_jett_plan {
enum class Skill:uint8_t {Cloudburst,Updraft,Tailwind,BladeStorm};
enum class Phase:uint8_t {Prepare,FirstHeld,FirstReleased,WaitPrimed,SecondHeld,SecondReleased,
    WaitEquipped,PrimaryHeld,PrimaryReleased,Settle,Complete,Failed};
enum class Action:uint8_t {None,ActivationPress,ActivationRelease,PrimaryPress,PrimaryRelease,Complete,Abort};
struct Observation {
    uint64_t now=0;
    bool ready=false,activationReleased=false,primed=false,equipped=false,primaryReleased=false;
    bool primaryContinue=true;
};
// Original Force*Input enqueues an input command. A getter reflects it only
// after the normal original input consumer has run; same-call equality is not
// a setter postcondition. Confirmation must occur in a later fixture GT sample.
enum class Readback:uint8_t {None,Waiting,Confirmed,Expired};
struct PendingInput {
    bool pending=false,primary=false,held=false;
    uint64_t atMs=0,tick=0;
    static constexpr uint32_t BudgetMs=10000;
};
inline void Submitted(PendingInput& pending,bool primary,bool held,uint64_t tick,uint64_t now){
    pending={true,primary,held,now,tick};
}
inline Readback Observe(PendingInput& pending,uint8_t value,uint64_t tick,uint64_t now){
    if(!pending.pending)return Readback::None;
    if(now<pending.atMs||tick<pending.tick||now-pending.atMs>=PendingInput::BudgetMs)return Readback::Expired;
    if(tick==pending.tick||value>1||value!=uint8_t(pending.held?1:0))return Readback::Waiting;
    pending.pending=false;return Readback::Confirmed;
}
// Byte ctor1C79DF0/common1C6DC00 and Enum ctor1BED7C0 plus their original
// LinkInternal slots1C88AB0/1BEEA70 produce these exact final runtime flags.
inline constexpr uint64_t LinkedByteOrEnumFlags(uint64_t declared){return declared|0x0008001040000200ULL;}
struct FieldShape {
    bool checked=false,found=false,generic=false,nameMatches=false,bounds=false,enumRequired=false;
    int32_t expectedOffset=-1,actualOffset=-1,actualSize=0,actualDim=0;
    uint64_t declaredFlags=0,expectedFlags=0,actualFlags=0,castFlags=0;
    uintptr_t field=0,expectedEnum=0,actualEnum=0,enumGetter=0;
    uint32_t mismatch=0;
};
inline uint32_t FieldMismatch(const FieldShape& p){
    uint32_t mask=0;
    if(!p.found)mask|=1;if(!p.generic)mask|=2;if(!p.nameMatches)mask|=4;
    if(p.actualOffset!=p.expectedOffset)mask|=8;if(p.actualDim!=1)mask|=16;
    if(p.actualSize!=1)mask|=32;if(!p.bounds)mask|=64;
    if(p.actualFlags!=p.expectedFlags)mask|=128;
    if((p.enumRequired&&!p.expectedEnum)||p.actualEnum!=p.expectedEnum)mask|=256;
    return mask;
}
struct Plan {
    Skill skill=Skill::Cloudburst;Phase phase=Phase::Prepare;
    uint64_t started=0,phaseAt=0,warmAt=0,last=0;
    uint32_t warmupMs=500;
    static constexpr uint32_t HoldMs=500,PrimaryMs=500,SettleMs=2000,TotalMs=45000,PrepareMs=30000;
};
inline Action Next(Plan& p,const Observation& o){
    if(p.phase==Phase::Complete||p.phase==Phase::Failed)return Action::None;
    if(o.now<p.last||o.now<p.started||o.now-p.started>=Plan::TotalMs){p.phase=Phase::Failed;return Action::Abort;}
    p.last=o.now;
    const auto move=[&](Phase phase,Action action){p.phase=phase;p.phaseAt=o.now;return action;};
    switch(p.phase){
    case Phase::Prepare:
        if(!o.ready||!o.activationReleased||!o.primaryReleased)p.warmAt=0;
        else if(!p.warmAt)p.warmAt=o.now;
        if(p.warmAt&&o.now-p.warmAt>=p.warmupMs)return move(Phase::FirstHeld,Action::ActivationPress);
        if(o.now-p.started>=Plan::PrepareMs)return move(Phase::Failed,Action::Abort);
        return Action::None;
    case Phase::FirstHeld:
        return o.now-p.phaseAt>=Plan::HoldMs?move(Phase::FirstReleased,Action::ActivationRelease):Action::None;
    case Phase::FirstReleased:
        if(!o.activationReleased)return Action::None;
        if(p.skill==Skill::Tailwind)return move(Phase::WaitPrimed,Action::None);
        if(p.skill==Skill::BladeStorm)return move(Phase::WaitEquipped,Action::None);
        return move(Phase::Settle,Action::None);
    case Phase::WaitPrimed:
        return o.primed&&o.activationReleased?move(Phase::SecondHeld,Action::ActivationPress):Action::None;
    case Phase::SecondHeld:
        return o.now-p.phaseAt>=Plan::HoldMs?move(Phase::SecondReleased,Action::ActivationRelease):Action::None;
    case Phase::SecondReleased:
        return o.activationReleased?move(Phase::Settle,Action::None):Action::None;
    case Phase::WaitEquipped:
        // Require actual current+desired X and a native settling interval.
        return o.equipped&&o.primaryReleased&&o.now-p.phaseAt>=1000
            ?move(Phase::PrimaryHeld,Action::PrimaryPress):Action::None;
    case Phase::PrimaryHeld:
        return !o.primaryContinue||o.now-p.phaseAt>=Plan::PrimaryMs?move(Phase::PrimaryReleased,Action::PrimaryRelease):Action::None;
    case Phase::PrimaryReleased:
        return o.primaryReleased?move(Phase::Settle,Action::None):Action::None;
    case Phase::Settle:
        return o.activationReleased&&o.primaryReleased
            &&o.now-p.phaseAt>=(p.skill==Skill::Cloudburst?6000U:Plan::SettleMs)
            ?move(Phase::Complete,Action::Complete):Action::None;
    default:return Action::None;
    }
}
} // namespace local_server_native_jett_plan
