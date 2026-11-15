#pragma once
#include "humanlike_bot_types.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace humanlike_bot_weapon_feedback {
inline constexpr size_t MaxContexts=16;
enum class SelectionReason:uint8_t {Unavailable,IncompletePrimaryQueries,IncompleteNativeState,NoPrimary,UniquePrimary,ActivePrimary,CommonPrimary,AmbiguousDependencies};
struct Candidate {
    bool valid=false,primaryKnown=false,primary=false,activeKnown=false,active=false;
    bool canFireKnown=false,canFire=false;
    bool behaviorKnown=false,automatic=false,nativeBurst=false;
    uintptr_t resource=0,stability=0;
};
struct Selection {
    bool valid=false,canFireKnown=false,canFire=false;
    bool behaviorKnown=false,automatic=false,nativeBurst=false;
    size_t slot=MaxContexts,primaryCount=0,activeCount=0;
    SelectionReason reason=SelectionReason::Unavailable;
};
// Query Primary/Pressed from every real registered context. An old shot's slot
// or the first attached component never supplies primary/active-state proof.
inline Selection SelectPrimary(const std::array<Candidate,MaxContexts>& rows,size_t count){
    Selection result{};if(!count||count>rows.size())return result;
    std::array<size_t,MaxContexts> primary{},active{};
    for(size_t n=0;n<count;++n){const auto& c=rows[n];
        if(!c.valid||!c.primaryKnown){result.reason=SelectionReason::IncompletePrimaryQueries;return result;}
        if(!c.primary)continue;
        if(!c.activeKnown){result.reason=SelectionReason::IncompleteNativeState;return result;}
        primary[result.primaryCount++]=n;
        if(c.activeKnown&&c.active)active[result.activeCount++]=n;
    }
    if(!result.primaryCount){result.reason=SelectionReason::NoPrimary;return result;}
    const bool useActive=result.activeCount!=0;
    const auto& chosen=useActive?active:primary;
    const size_t choices=useActive?result.activeCount:result.primaryCount;
    result.slot=chosen[0];const auto& first=rows[result.slot];
    if(!first.resource||!first.stability){result.reason=SelectionReason::AmbiguousDependencies;return result;}
    result.canFireKnown=first.canFireKnown;result.canFire=first.canFire;
    result.behaviorKnown=first.behaviorKnown;result.automatic=first.automatic;result.nativeBurst=first.nativeBurst;
    for(size_t n=1;n<choices;++n){const auto& c=rows[chosen[n]];
        if(c.resource!=first.resource||c.stability!=first.stability){result.reason=SelectionReason::AmbiguousDependencies;return result;}
        if(!c.canFireKnown||c.canFire!=first.canFire)result.canFireKnown=false;
        if(!c.behaviorKnown||c.automatic!=first.automatic||c.nativeBurst!=first.nativeBurst)result.behaviorKnown=false;
    }
    result.valid=true;
    result.reason=choices==1?(useActive?SelectionReason::ActivePrimary:SelectionReason::UniquePrimary):SelectionReason::CommonPrimary;
    return result;
}
// Diagnostic result of original711ead0 only. This is a native refire delay,
// not observed shot spacing, weapon recovery, or a second shot-count source.
inline bool NativeCadenceContextProven(const Selection& selection){
    return selection.valid&&selection.slot<MaxContexts
        &&(selection.activeCount==1||(!selection.activeCount&&selection.primaryCount==1));
}
inline bool NativeRefireIntervalValid(float rate,float modifier,float interval){
    if(!std::isfinite(rate)||!std::isfinite(modifier)||!std::isfinite(interval)
       ||rate<=0||modifier<=0||interval<=0)return false;
    const auto product=rate*modifier;
    if(!std::isfinite(product)||product<=0)return false;
    const auto expected=1.f/product;
    if(!std::isfinite(expected)||expected<=0)return false;
    return std::abs(expected-interval)<=(std::max)(1.e-6f,std::abs(expected)*1.e-6f);
}
struct NativeValues {
    bool sourceValid=false,stable=false;
    humanlike_bot::WeaponKind kind=humanlike_bot::WeaponKind::Unknown;
    uint64_t binding=0;uint16_t catalogWeaponIndex=0xffff;
    bool ammoQueried=false,reserveQueried=false,canFireKnown=false,canFire=false;
    bool stabilityQueried=false,stabilityMovementMatches=false,stabilityHolderKnown=false,reloadObserved=false;
    bool behaviorKnown=false,automatic=false,nativeBurst=false,triggerHeldKnown=false,triggerHeld=false;
    int32_t magazine=-1,maximum=-1,reserve=-1;
    float firingError=0,movementError=0,pitch=0,yaw=0;
    double sampledAtSeconds=0;
};
inline humanlike_bot::WeaponKind Kind(uint8_t actualCatalogCategory){
    using K=humanlike_bot::WeaponKind;
    switch(actualCatalogCategory){
    case 1:return K::Machinegun;case 2:return K::Rifle;case 3:return K::Shotgun;
    case 4:return K::Pistol;case 5:return K::SMG;case 6:return K::Sniper;case 7:return K::Melee;
    default:return K::Unknown;
    }
}
inline humanlike_bot::WeaponReadback MakeReadback(const NativeValues& n){
    humanlike_bot::WeaponReadback out{};
    if(!n.sourceValid||!n.stable||!n.binding||!std::isfinite(n.sampledAtSeconds)||n.sampledAtSeconds<0)return out;
    out.valid=true;out.binding=n.binding;out.kind=n.kind;out.catalogWeaponIndex=n.catalogWeaponIndex;out.sampledAtSeconds=n.sampledAtSeconds;
    if(n.ammoQueried&&n.magazine>=0&&n.maximum>=0&&n.magazine<=n.maximum){
        out.ammoKnown=true;out.magazine=n.magazine;out.magazineMaximum=n.maximum;
    }
    if(n.reserveQueried&&n.reserve>=0){out.reserveKnown=true;out.reserve=n.reserve;}
    out.canFireKnown=n.canFireKnown;out.canFire=n.canFireKnown&&n.canFire;
    out.fireBehaviorKnown=n.behaviorKnown;out.automatic=n.behaviorKnown&&n.automatic;out.nativeBurst=n.behaviorKnown&&n.nativeBurst;
    out.triggerHeldKnown=n.triggerHeldKnown;out.triggerHeld=n.triggerHeldKnown&&n.triggerHeld;
    // A positive typed current ReloadState is evidence. Its absence is not
    // asserted to exclude every authored intro/cycle/exit reload phase.
    if(n.reloadObserved){out.reloadKnown=true;out.reloading=true;}
    if(n.stabilityQueried&&n.stabilityMovementMatches&&n.stabilityHolderKnown){
        if(std::isfinite(n.firingError)&&std::isfinite(n.movementError)&&n.firingError>=0&&n.movementError>=0){
            out.recoveryKnown=true;out.firingErrorDegrees=n.firingError;out.movementErrorDegrees=n.movementError;
        }
        if(std::isfinite(n.pitch)&&std::isfinite(n.yaw)){
            out.recoilKnown=true;out.pitchRecoilDegrees=n.pitch;out.yawRecoilDegrees=n.yaw;
        }
    }
    // Only the single installed origin/geometry owner may set this after
    // verifying the actual per-life/binding event feed. No ammo/time proxy.
    out.shotFeedAvailable=false;
    return out;
}
} // namespace humanlike_bot_weapon_feedback
