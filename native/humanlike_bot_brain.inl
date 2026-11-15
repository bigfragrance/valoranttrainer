#pragma once
namespace humanlike_bot {
inline bool Brain::Initialize(const cfg::Profile& profile,uint64_t matchSeed,uint32_t stableBotId,uint32_t revision){
    if(!stableBotId||stableBotId>=cfg::MaxRosterProfiles||!cfg::Validate(profile))return false;
    profile_=profile;seed_=matchSeed;stableBotId_=stableBotId;configured_=true;
    life_={};snapshot_={};enabled_=true;releasePending_=profileChanged_=false;
    lastNow_=lifeAt_=lastPerception_=lastTraceAt_=lastAttentionAt_=-1;traceWrite_=0;traces_.fill({});
    for(auto& t:tracks_)t={};Interrupt();ResetTactics();informationAngles_.fill({});viewCommand_={};
    snapshot_.configured=true;snapshot_.revision=revision;snapshot_.reason=Reason::None;
    for(auto& d:dependencies_)d.reset();return true;
}
inline ProfileUpdate Brain::ApplyProfile(const cfg::Profile& profile,uint32_t revision){
    ProfileUpdate result{};result.revision=revision;
    if(!configured_||!cfg::Validate(profile))return result;
    std::bitset<cfg::ParamCount> changed;
    for(size_t n=0;n<cfg::ParamCount;++n)if(profile.values[n]!=profile_.values[n])changed.set(n);
    result.accepted=true;result.changed=changed.any();
    if(result.changed){
        for(size_t n=0;n<dependencies_.size();++n)if((changed&dependencies_[n]).any())result.interruptedDomains|=uint32_t(1)<<n;
        const auto hit=[&](Domain d){return (result.interruptedDomains&(uint32_t(1)<<static_cast<unsigned>(d)))!=0;};
        if(hit(Domain::Aim)||hit(Domain::Fire)||hit(Domain::Motion)){
            Interrupt(hit(Domain::Aim),hit(Domain::Motion),hit(Domain::Fire)||hit(Domain::Aim)||hit(Domain::Motion));
            releasePending_=true;profileChanged_=true;
        }
        if(hit(Domain::Attention)){attention_=Attention::Normal;attentionUntil_=0;}
        if(hit(Domain::Decision)&&navigationEstimate_.requested){
            CancelEstimatedNavigation();nextDecisionAt_=0;releasePending_=true;profileChanged_=true;
        }
        if(changed.test(cfg::Index(Param::AimHeadWeight))||changed.test(cfg::Index(Param::AimBodyWeight)))for(auto& t:tracks_)t.reselectPoint=true;
    }
    profile_=profile;snapshot_.revision=revision;return result;
}
inline ProfileUpdate Brain::ApplyConfiguration(const cfg::Profile& profile,uint32_t revision,bool enabled){
    const auto result=ApplyProfile(profile,revision);if(result.accepted)SetEnabled(enabled);return result;
}
inline bool Brain::Reseed(uint64_t matchSeed){
    if(!configured_||seed_==matchSeed)return false;
    seed_=matchSeed;rng_.Seed(math::Mix(seed_)^math::Mix(stableBotId_)^math::Mix(life_.spawnEpoch));
    reactionEvents_.fill(0);advanceGaitEvent_=0;prefireHandled_.fill(0);
    // An explicit behavior-seed change ends old random action plans while
    // preserving acquired information, native identity and resolved ability.
    Interrupt();CancelEstimatedNavigation();soundHandledSerial_=0;combatPair_={};
    nextMotionChoiceAt_=nextDecisionAt_=0;releasePending_=true;
    return true;
}
inline bool Brain::BeginLife(OwnLife life){
    if(!configured_||!stableBotId_||life.stableBotId!=stableBotId_||!life.spawnEpoch||!life.token)return false;
    if(life_.token){if(life.spawnEpoch<life_.spawnEpoch||(life.spawnEpoch==life_.spawnEpoch&&life.token!=life_.token))return false;
        if(math::Same(life_,life))return true;}
    life_=life;snapshot_.life=life;snapshot_.peekPlan={};snapshot_.lastEndedPeekPlan={};
    viewCommand_={};lastRequestedFire_=false;
    snapshot_.lastIntentJump=snapshot_.lastIntentCrouch=snapshot_.lastIntentWalk=false;snapshot_.lastIntentRelease=true;lifeAt_=-1;lastNow_=-1;lastPerception_=-1;lastAttentionAt_=-1;
    for(auto& t:tracks_)t={};for(auto& a:angles_)a={};informationAngles_.fill({});
    seenShots_.fill({});seenHurt_.fill(0);seenSounds_.fill(0);seenFeedback_.fill(0);
    nextShotKey_=nextHurtKey_=nextSoundKey_=nextFeedbackKey_=0;
    Interrupt();attention_=Attention::Normal;attentionUntil_=0;lastHurtAt_=lastSoundAt_=lastKillAt_=-1;
    hurtDirectionKnown_=soundDirectionKnown_=recentHit_=recentMiss_=seenAnyTarget_=hurtAcknowledged_=soundAcknowledged_=false;
    hurtConfidence_=soundConfidence_=0;holdUntil_=searchUntil_=angleUntil_=nextDecisionAt_=0;relocateAt_=hitDecisionAt_=-1;
    patrolRouteId_=preaimAngleId_=0;
    nextCrouchAllowedAt_=nextMotionChoiceAt_=0;lastNeutralAt_=-1;previousMoveSign_=1;
    lastIssuedSerial_=lastObservedConsumedSerial_=0;
    previousAimTarget_=0;relocationGoalKnown_=false;hurtFlank_=soundFlank_=false;soundEventAt_=-1;ResetTactics();
    reactionEvents_.fill(0);hurtReaction_=soundReaction_={};advanceGait_={};advanceGaitEvent_=0;eventActivity_=Activity::Respawn;
    snapshot_.lastReactionContext=cfg::ReactionContext::Count;snapshot_.lastReactionEvent=0;
    snapshot_.lastReactionHesitationMs=snapshot_.lastReactionYawErrorDeg=snapshot_.lastReactionPitchErrorDeg=0;
    profileChanged_=false;
    rng_.Seed(math::Mix(seed_)^math::Mix(stableBotId_)^math::Mix(life.spawnEpoch));
    // Native token protects identity, but is deliberately not an RNG seed.
    // Runtime object allocation/serial order must not change reproducible skill.
    activity_=Activity::Respawn;releasePending_=true;return true;
}
inline void Brain::Interrupt(bool resetAim,bool resetMotion,bool resetFire){
    if(resetAim){aim_={};viewCommand_.commandKnown=false;viewCommand_.spent=true;dependencies_[static_cast<size_t>(Domain::Aim)].reset();}
    if(resetMotion){motion_={};advanceGait_={};dependencies_[static_cast<size_t>(Domain::Motion)].reset();}
    if(resetFire){fire_={};estimatedPrefire_={};dependencies_[static_cast<size_t>(Domain::Fire)].reset();}
}
inline Intent Brain::Neutral(const OwnSnapshot& own,Reason reason){
    if(fire_.estimateKey)fire_={};estimatedPrefire_={};
    viewCommand_.commandKnown=false;viewCommand_.spent=true;
    CancelEstimatedNavigation();
    if(reason==Reason::Dead||reason==Reason::Retired||reason==Reason::Paused||reason==Reason::Disabled
       ||reason==Reason::OwnStateUnavailable||reason==Reason::InvalidOwnLife||reason==Reason::InvalidTime
       ||reason==Reason::PermissionRejected)ResetTactics();
    Intent result{};result.serial=++snapshot_.requestedIntents;result.life=own.life;result.rotation=math::Finite(own.view)?math::Normalize(own.view):Rotation{};
    if(math::Finite(own.nowSeconds))lastNeutralAt_=own.nowSeconds;
    snapshot_.reason=reason;snapshot_.nativeIntentConsumptionKnown=false;return result;
}
inline Brain::Track* Brain::Find(TrackId id){for(auto& t:tracks_)if(t.used&&t.id==id)return &t;return nullptr;}
inline const Brain::Track* Brain::Find(TrackId id)const{for(const auto& t:tracks_)if(t.used&&t.id==id)return &t;return nullptr;}
inline MemoryView Brain::Memory(TrackId id)const{
    MemoryView view{};const auto t=Find(id);if(!t)return view;
    view.used=t->used;view.visible=t->visible;view.retired=t->retired;view.track=t->id;view.point=t->point;view.velocity=t->velocity;
    view.confidence=t->confidence;view.uncertaintyCm=t->uncertainty;view.lastSeenSeconds=t->lastSeen;
    view.firstEvidenceSeconds=t->firstEvidence;view.recognizedSeconds=t->recognized;view.exposure=t->exposure;return view;
}
inline bool Brain::ReadShootingView(const OwnSnapshot& own,Rotation& view)const{
    if(own.shootingViewRequired){
        const auto at=own.shootingViewSampledAtSeconds;
        if(!own.shootingViewKnown||!math::Finite(own.shootingView)||!math::Finite(at)||at<0||at>own.nowSeconds
           ||own.nowSeconds-at>cfg::Get(profile_,Param::OwnSnapshotMaxAgeMs)*.001)return false;
        view=own.shootingView;
    }else{
        view=own.view;
        if(own.weapon.recoilKnown&&math::Finite(own.weapon.pitchRecoilDegrees)&&math::Finite(own.weapon.yawRecoilDegrees)){
            view.pitch+=own.weapon.pitchRecoilDegrees;view.yaw+=own.weapon.yawRecoilDegrees;
        }
    }
    return math::Finite(view);
}
inline CombatReadback Brain::ReadCombat(const OwnSnapshot& own)const{
    CombatReadback value{};
    const auto maximumAge=cfg::Get(profile_,Param::OwnSnapshotMaxAgeMs)*.001;
    if(!configured_||!math::Same(life_,own.life)||!own.valid||!own.alive||!own.eyeValid||!own.viewValid
       ||!math::Finite(own.eye)||!math::Finite(own.view)||!math::Finite(own.nowSeconds)
       ||!math::Finite(own.sampledAtSeconds)||own.sampledAtSeconds<0||own.sampledAtSeconds>own.nowSeconds
       ||own.nowSeconds-own.sampledAtSeconds>maximumAge)return value;
    value.current=true;value.now=own.nowSeconds;value.ownSampleAt=own.sampledAtSeconds;
    value.requestedFire=lastRequestedFire_;value.historyReady=aim_.historyReady;
    value.window=fire_.window;value.holding=fire_.holding;value.needRelease=fire_.needRelease;
    value.noticeAt=aim_.noticeAt;value.actionAt=aim_.startAt;value.nextFireAt=fire_.nextAt;
    value.observedView=own.view;value.shootingViewRequired=own.shootingViewRequired;
    value.shootingViewAt=own.shootingViewSampledAtSeconds;
    value.shootingViewKnown=ReadShootingView(own,value.gateView);
    value.toleranceDegrees=cfg::Get(profile_,fire_.mode==FireMode::MovingClose?Param::MovingFireToleranceDeg:Param::FireAlignmentToleranceDeg);
    const auto* target=Find(aim_.target);
    if(target&&target->used&&target->id&&math::Finite(target->point)){
        value.targetKnown=true;value.target=target->id;value.targetVisible=target->visible;
        value.targetRetired=target->retired||target->lifeRetired;value.point=target->point;value.pointKind=target->pointKind;
        value.headAvailableAtSelection=target->headAvailableAtSelection;
        value.targetObservedAt=target->lastSeen;
        const auto bearing=math::Look(own.eye,target->point);
        value.observedErrorDegrees=math::Angle(value.observedView,bearing);
        value.gateErrorDegrees=math::Angle(value.gateView,bearing);
    }
    return value;
}
inline cfg::Distance Brain::Band(const OwnSnapshot& own,float distanceCm){
    const auto w=static_cast<cfg::Weapon>(own.weapon.catalogWeaponIndex);
    if(!cfg::ValidWeapon(w))return cfg::Distance::Far;
    return distanceCm<=P(cfg::WeaponNearDistance(w))?cfg::Distance::Near:distanceCm<=P(cfg::WeaponFarDistance(w))?cfg::Distance::Mid:cfg::Distance::Far;
}
inline float Brain::Tune(const OwnSnapshot& own,cfg::WeaponTuning tuning){
    const auto t=Find(aim_.target);const auto distance=t?TargetDistance(own,*t):P(Param::FireFarDistanceCm);
    const auto w=static_cast<cfg::Weapon>(own.weapon.catalogWeaponIndex);
    if(!cfg::ValidWeapon(w))return 0;
    return P(cfg::Tuning(w,Band(own,distance),tuning));
}
inline Param Brain::PeekParameter(PeekKind kind,unsigned field)const{
    if(kind==PeekKind::None||field>=cfg::PeekFieldCount)return Param::Count;
    return cfg::PeekParam(static_cast<cfg::Peek>(static_cast<unsigned>(kind)-1),static_cast<cfg::PeekField>(field));
}
inline Brain::ReactionVariation Brain::SampleReaction(cfg::ReactionContext context){
    const auto index=static_cast<size_t>(context);ReactionVariation result{};
    if(index>=reactionEvents_.size())return result;
    const auto event=++reactionEvents_[index];math::Rng noise{};
    // Per-context event stream excludes native object tokens and the generic
    // behavior RNG. Fixed draws keep hesitation and angular error independent.
    noise.Seed(math::Mix(seed_)^math::Mix(stableBotId_)^math::Mix(life_.spawnEpoch)
        ^math::Mix(0x4852454143540000ULL+index)^math::Mix(event));
    const auto hesitateDraw=noise.Unit(),errorDraw=noise.Unit(),yawDraw=noise.Signed(),pitchDraw=noise.Signed();
    const auto field=[&](cfg::ReactionVariationField value){return P(cfg::ReactionVariation(context,value));};
    const auto hesitate=field(cfg::ReactionVariationField::HesitateProbability),error=field(cfg::ReactionVariationField::ErrorProbability);
    if(hesitateDraw<hesitate)result.hesitationSeconds=field(cfg::ReactionVariationField::HesitateMs)*.001f;
    if(errorDraw<error){const auto degrees=field(cfg::ReactionVariationField::ErrorDeg);result.yawError=yawDraw*degrees;result.pitchError=pitchDraw*degrees;}
    snapshot_.lastReactionContext=context;snapshot_.lastReactionEvent=event;
    snapshot_.lastReactionHesitationMs=result.hesitationSeconds*1000;
    snapshot_.lastReactionYawErrorDeg=result.yawError;snapshot_.lastReactionPitchErrorDeg=result.pitchError;
    return result;
}
inline bool Brain::AdvanceWalk(double now){
    const auto probability=P(Param::AdvanceWalkProbability);
    if(probability<=0){advanceGait_={};return false;}
    const auto hold=Milliseconds(Param::AdvanceGaitHoldMs),jitter=Milliseconds(Param::AdvanceGaitJitterMs);
    if(!advanceGait_.known||now>=advanceGait_.until){
        math::Rng noise{};noise.Seed(math::Mix(seed_)^math::Mix(stableBotId_)^math::Mix(life_.spawnEpoch)
            ^math::Mix(0x4847414954000000ULL)^math::Mix(++advanceGaitEvent_));
        advanceGait_.known=true;advanceGait_.walk=noise.Unit()<probability;
        advanceGait_.until=now+hold+noise.Unit()*jitter;
    }
    return advanceGait_.walk;
}
inline bool Brain::ObserveView(const OwnSnapshot& own,Reason& reason){
    const auto at=own.viewSampleTimeKnown?own.viewSampledAtSeconds:own.sampledAtSeconds;
    // Explicit invalid precise provenance must never fall back to another clock.
    if(!math::Finite(at)||at<0||at>own.nowSeconds){viewCommand_.commandKnown=false;viewCommand_.spent=true;reason=Reason::InvalidTime;return false;}
    if(own.nowSeconds-at>Milliseconds(Param::OwnSnapshotMaxAgeMs)){viewCommand_.commandKnown=false;viewCommand_.spent=true;reason=Reason::OwnStateUnavailable;return false;}
    auto& view=viewCommand_;
    if(view.sampleAt>=0&&view.exactTime==own.viewSampleTimeKnown&&at<view.sampleAt){
        view.commandKnown=false;view.spent=true;reason=Reason::InvalidTime;return false;
    }
    if(view.sampleAt<0||view.exactTime!=own.viewSampleTimeKnown){
        view={};view.sampleAt=view.intervalStart=at;view.reference=own.view;view.exactTime=own.viewSampleTimeKnown;view.spent=false;
    }else if(at>view.sampleAt){
        view.intervalStart=view.sampleAt;view.sampleAt=at;view.reference=own.view;view.spent=false;view.commandKnown=false;
    }else if(math::Angle(view.reference,own.view)>.00001f){
        // A changed legacy view without a new acquisition time has no proven
        // interval. Keep the observation but do not manufacture a turn budget.
        view.reference=own.view;view.commandKnown=false;view.spent=true;
    }
    return true;
}
inline double Brain::ViewDelta(double notBefore)const{
    if(viewCommand_.spent||viewCommand_.sampleAt<0)return 0;
    const auto start=(std::max)(viewCommand_.intervalStart,(std::max)(lastNeutralAt_,notBefore));
    return (std::max)(0.0,viewCommand_.sampleAt-start);
}
inline bool Brain::ReplayView(const OwnSnapshot& own,TrackId target,bool preaim,Intent& intent){
    auto& view=viewCommand_;if(!view.spent)return false;
    // Reissue an absolute command, never promote it to observed own.view.
    // Plan changes hold the actual acquired orientation until a new sample.
    if(!view.commandKnown||view.target!=target||view.preaim!=preaim){
        view.command=math::Normalize(own.view);view.target=target;view.preaim=preaim;view.commandKnown=true;
    }
    intent.rotationRequested=true;intent.rotation=view.command;return true;
}
inline void Brain::StepView(const OwnSnapshot& own,Rotation goal,float speed,double dt,float smoothing,TrackId target,bool preaim,Intent& intent){
    if(ReplayView(own,target,preaim,intent))return;
    intent.rotationRequested=true;intent.rotation=math::Step(own.view,math::Normalize(goal),speed*static_cast<float>(dt),smoothing);
    viewCommand_.command=intent.rotation;viewCommand_.target=target;viewCommand_.preaim=preaim;
    viewCommand_.commandKnown=viewCommand_.spent=true;
}
inline bool Brain::FlankBearing(const OwnSnapshot& own,float bearing){
    if(!own.viewValid||!math::Finite(own.view)||!math::Finite(bearing))return false;
    auto horizontal=P(Param::VisualFovHorizontal);
    if(own.nativeViewFovKnown&&math::Finite(own.nativeHorizontalFovDegrees)&&own.nativeHorizontalFovDegrees>0)
        horizontal=(std::min)(horizontal,own.nativeHorizontalFovDegrees);
    // Side/back is a yaw relation, never a large pitch in a forward corner.
    return std::abs(math::Yaw(bearing-own.view.yaw))>horizontal*.5f;
}
inline double Brain::DirectionActionAt(double eventAt,double noticedAt,double baselineActionAt,bool flank,Param decide,Param start,double extraStart){
    if(!flank)return baselineActionAt;
    // Perception, judgement and motor start are shared stages of one stimulus.
    // Take each stage's maximum; do not sum two complete reaction pipelines.
    const auto notice=(std::max)(noticedAt,eventAt+Milliseconds(Param::ReactionFlankPerceiveMs));
    const auto judgement=(std::max)(Milliseconds(decide),Milliseconds(Param::ReactionFlankDecideMs));
    const auto motor=(std::max)(extraStart,static_cast<double>((std::max)(Milliseconds(start),Milliseconds(Param::ReactionFlankStartMs))));
    return (std::max)(baselineActionAt,notice+judgement+motor);
}
inline void Brain::Receive(const SensorFrame& f){
    Scope(Domain::Fire);
    if(f.shotsOverflow){fire_.holding=false;fire_.window=false;releasePending_=true;ReasonNow(Reason::ShotOverflow);}
    for(size_t n=0;n<f.shotCount;++n){const auto& s=f.shots[n];
        if(!s.valid||!s.actualOriginGeometryPaired||!math::Same(s.life,life_)||!s.nativeOrigin||!s.binding||!s.serial
           ||!math::Finite(s.acquiredAtSeconds)||s.acquiredAtSeconds>f.self.nowSeconds
           ||f.self.nowSeconds-s.acquiredAtSeconds>Milliseconds(Param::ShotReceiptMaxAgeMs))continue;
        bool duplicate=false;for(const auto& key:seenShots_)if(math::Same(key.life,s.life)&&key.binding==s.binding&&key.origin==s.nativeOrigin){duplicate=true;break;}
        if(duplicate){++snapshot_.duplicateShotReceipts;continue;}
        seenShots_[nextShotKey_++%seenShots_.size()]={s.life,s.binding,s.nativeOrigin};++snapshot_.actualShots;
        if(fire_.window&&fire_.binding==s.binding&&s.acquiredAtSeconds>=fire_.startedAt){++fire_.count;fire_.lastActualAt=s.acquiredAtSeconds;
            if(f.self.weapon.fireBehaviorKnown&&!f.self.weapon.automatic&&!f.self.weapon.nativeBurst){fire_.needRelease=true;fire_.releaseRequestedAt=-1;}}
    }
    Scope(Domain::Decision);
    for(size_t n=0;n<f.feedbackCount;++n){const auto& e=f.feedback[n];
        if(!e.valid||!math::Same(e.life,life_)||!e.serial||!math::Finite(e.acquiredAtSeconds)||e.acquiredAtSeconds>f.self.nowSeconds
           ||!math::Finite(e.confidence)||e.confidence<0||e.confidence>1||!math::Finite(e.angularUncertaintyDeg)||e.angularUncertaintyDeg<0)continue;
        bool paired=false;for(const auto& s:seenShots_)if(math::Same(s.life,life_)&&s.binding==e.binding&&s.origin==e.nativeOrigin&&e.nativeOrigin)paired=true;
        const auto acquired=Find(e.alreadyKnownTrack);
        const bool visibleModel=e.kind==Feedback::Kind::Hit&&e.provenance==Feedback::Provenance::ProjectVisibleNativeContact
            &&!e.nativeFeedbackObserved&&e.nativeContactObserved&&e.visibleImpactProven&&paired&&acquired&&!acquired->retired;
        if(!e.nativeFeedbackObserved&&!visibleModel)continue;
        if(e.kind==Feedback::Kind::Hit&&!paired)continue;
        if(e.kind==Feedback::Kind::Miss&&(!paired||!e.wholeOriginComplete||!e.perceivedMissProven||!acquired||acquired->retired||!acquired->visible||e.alreadyKnownTrack!=aim_.target))continue;
        bool duplicate=false;for(const auto key:seenFeedback_)duplicate=duplicate||key==e.serial;if(duplicate)continue;
        seenFeedback_[nextFeedbackKey_++%seenFeedback_.size()]=e.serial;
        if(e.kind==Feedback::Kind::Kill){
            lastKillAt_=e.acquiredAtSeconds;relocationGoalKnown_=false;Interrupt(true,false,true);
            if(auto t=Find(e.alreadyKnownTrack)){t->retired=t->lifeRetired=true;t->visible=false;t->confidence=0;}
            relocateAt_=rng_.Chance(P(Param::KillRelocateProbability))?lastKillAt_+Milliseconds(Param::KillRelocateDelayMs):-1;
            activity_=Activity::Search;searchUntil_=f.self.nowSeconds+Milliseconds(Param::SearchDwellMs);
        }else if(e.kind==Feedback::Kind::Hit){
            snapshot_.feedbackProvenance=visibleModel?Feedback::Provenance::ProjectVisibleNativeContact:Feedback::Provenance::NativePerceptible;
            snapshot_.feedbackConfidence=e.confidence;snapshot_.feedbackAngularUncertaintyDeg=e.angularUncertaintyDeg;
            if(rng_.Chance(e.confidence*(1-P(Param::HitMisreadProbability)))){recentHit_=true;hitDecisionAt_=e.acquiredAtSeconds+Milliseconds(Param::HitConfirmMs);}
        }else if(e.kind==Feedback::Kind::Miss){
            if(paired)recentMiss_=true;
        }else if(e.kind==Feedback::Kind::OwnDeath){Interrupt();releasePending_=true;activity_=Activity::Dead;}
        else if(e.kind==Feedback::Kind::WeaponChanged){Interrupt(false,false,true);releasePending_=true;}
    }
    Scope(Domain::Attention);
    for(size_t n=0;n<f.hurtCount;++n){const auto& h=f.hurt[n];
        if(!h.valid||!math::Same(h.life,life_)||!h.serial||!math::Finite(h.acquiredAtSeconds)||h.acquiredAtSeconds>f.self.nowSeconds
           ||!math::Finite(h.ownHealthLoss)||!math::Finite(h.ownShieldLoss)||h.ownHealthLoss<0||h.ownShieldLoss<0||h.ownHealthLoss+h.ownShieldLoss<=0||!math::Finite(h.confidence))continue;
        bool duplicate=false;for(const auto key:seenHurt_)duplicate=duplicate||key==h.serial;if(duplicate)continue;
        seenHurt_[nextHurtKey_++%seenHurt_.size()]=h.serial;lastHurtAt_=h.acquiredAtSeconds;
        const auto notice=(std::max)(Milliseconds(Param::HurtNoticeMs),Milliseconds(Param::ReactionHurtPerceiveMs));
        hurtNoticeAt_=lastHurtAt_+notice;hurtDirectionAt_=lastHurtAt_+(std::max)(notice,Milliseconds(Param::HurtDirectionMs));hurtAcknowledged_=false;
        hurtActionAt_=hurtNoticeAt_+Milliseconds(Param::ReactionHurtDecideMs)+(std::max)(Milliseconds(Param::ReactionHurtStartMs),Milliseconds(Param::HurtActionAdjustMs));
        hurtDirectionKnown_=h.directionKnown&&math::Finite(h.bearingDegrees)&&math::Finite(h.bearingUncertaintyDegrees);
        hurtBearing_=h.bearingDegrees;hurtConfidence_=h.confidence;
        hurtFlank_=hurtDirectionKnown_&&FlankBearing(f.self,hurtBearing_);
        hurtReaction_=SampleReaction(hurtFlank_?cfg::ReactionContext::Flank:cfg::ReactionContext::Hurt);
        hurtActionAt_+=hurtReaction_.hesitationSeconds;
        if(hurtDirectionKnown_)hurtBearing_=math::Yaw(hurtBearing_+hurtReaction_.yawError);
        lastSalientAt_=f.self.nowSeconds;
    }
    Scope(Domain::Decision);
    if(!f.hearingAvailable){
        lastSoundAt_=-1;soundDirectionKnown_=false;soundEstimate_={};
        for(auto&e:combatEstimates_)if(e.heard)e={};
        if(navigationEstimate_.usesHearing)CancelEstimatedNavigation();
    }
    for(size_t n=0;n<f.soundCount;++n){const auto& s=f.sounds[n];
        if(!f.hearingAvailable||!s.valid||s.ownSound||!s.serial||!math::Finite(s.acquiredAtSeconds)||s.acquiredAtSeconds>f.self.nowSeconds
           ||!math::Finite(s.confidence)||s.confidence<0||s.confidence>1
           ||s.acquiredAtSeconds<lifeAt_
           ||f.self.nowSeconds-s.acquiredAtSeconds>=Milliseconds(Param::HearingStimulusTtlMs)
           ||(s.directionKnown&&(!math::Finite(s.bearingDegrees)||!math::Finite(s.bearingUncertaintyDegrees)||s.bearingUncertaintyDegrees<0||s.bearingUncertaintyDegrees>180))
           ||(s.evidenceTimeKnown&&(!math::Finite(s.firstEvidenceAtSeconds)||s.firstEvidenceAtSeconds>s.acquiredAtSeconds))
           ||(s.distanceBandKnown&&(!math::Finite(s.minimumDistanceCm)||!math::Finite(s.maximumDistanceCm)||s.minimumDistanceCm<0||s.maximumDistanceCm<s.minimumDistanceCm)))continue;
        bool duplicate=false;for(const auto key:seenSounds_)duplicate=duplicate||key==s.serial;if(duplicate)continue;
        seenSounds_[nextSoundKey_++%seenSounds_.size()]=s.serial;lastSoundAt_=s.acquiredAtSeconds;lastSoundKind_=s.kind;
        soundDirectionKnown_=s.directionKnown&&math::Finite(s.bearingDegrees);soundBearing_=s.bearingDegrees;soundConfidence_=s.confidence;
        soundDistance_=s.distanceBandKnown?(s.minimumDistanceCm+s.maximumDistanceCm)*.5f:0;
        soundEventAt_=s.evidenceTimeKnown?s.firstEvidenceAtSeconds:s.acquiredAtSeconds;
        soundFlank_=soundDirectionKnown_&&FlankBearing(f.self,soundBearing_);
        soundNoticeAt_=s.evidenceTimeKnown?(std::max)(s.acquiredAtSeconds,s.firstEvidenceAtSeconds+Milliseconds(Param::ReactionHearingPerceiveMs)):s.acquiredAtSeconds;
        soundDecisionAt_=soundNoticeAt_+Milliseconds(Param::ReactionHearingDecideMs)+Milliseconds(Param::ReactionHearingStartMs);soundAcknowledged_=false;
        soundReaction_=SampleReaction(soundFlank_?cfg::ReactionContext::Flank:cfg::ReactionContext::Hearing);
        soundDecisionAt_+=soundReaction_.hesitationSeconds;
        if(soundDirectionKnown_)soundBearing_=math::Yaw(soundBearing_+soundReaction_.yawError);
        lastSalientAt_=f.self.nowSeconds;
        const auto settings=TacticSettings();
        auto perceived=s;
        if(soundDirectionKnown_){perceived.bearingDegrees=soundBearing_;
            perceived.bearingUncertaintyDegrees=(std::min)(180.0f,s.bearingUncertaintyDegrees+std::abs(soundReaction_.yawError));}
        const auto incoming=humanlike_bot_tactics_navigation::FromSound(perceived,f.self,settings);
        const bool sameEpisode=humanlike_bot_tactics_navigation::Current(soundEstimate_,f.self.nowSeconds,settings)
            &&humanlike_bot_tactics_navigation::Current(incoming,f.self.nowSeconds,settings)
            &&soundEstimate_.gunshot==incoming.gunshot
            &&math::Horizontal(math::Sub(soundEstimate_.point,incoming.point))<=humanlike_bot_tactics_navigation::Radius(soundEstimate_,f.self.nowSeconds,settings)
                +humanlike_bot_tactics_navigation::Radius(incoming,f.self.nowSeconds,settings);
        // Continuous fire at the same uncertain origin must not perpetually
        // push the initial hearing-reaction deadline forward on every serial.
        if(!sameEpisode){soundApproachReadyAt_=soundDecisionAt_;++prefireSoundEpisode_;prefireSoundAt_=incoming.availableAt;}
        prefireSoundProvenance_=s.evidence!=SoundEvidence::Unknown&&static_cast<unsigned>(s.evidence)<=static_cast<unsigned>(SoundEvidence::ActualAudioPostLogicalPropagation);
        soundEstimate_=incoming;
        if(soundEstimate_.gunshot)humanlike_bot_tactics_navigation::Insert(combatEstimates_,soundEstimate_,f.self.nowSeconds,settings,nextHypothesis_);
    }
    for(size_t n=0;n<f.retiredTrackCount;++n){const auto& r=f.retiredTracks[n];if(!r.valid||!r.track||!math::Finite(r.learnedAtSeconds)||r.learnedAtSeconds>f.self.nowSeconds)continue;
        if(auto t=Find(r.track)){t->retired=t->lifeRetired=true;t->visible=false;t->confidence=0;}
        if(aim_.target==r.track){Interrupt();releasePending_=true;ReasonNow(Reason::TargetRetired);}
    }
}
inline humanlike_bot_tactics_navigation::Settings Brain::TacticSettings(){
    return {Milliseconds(Param::HearingStimulusTtlMs),Milliseconds(Param::MemoryConfidenceHalfLifeMs),
        P(Param::MemoryConfidenceDecayPerS),P(Param::MemoryUncertaintyGrowthCmS),P(Param::MemoryMinimumConfidence)};
}
inline void Brain::ResetTactics(){
    combatEstimates_.fill({});soundEstimate_={};combatPair_={};CancelEstimatedNavigation();
    nextHypothesis_=nextNavigationEstimate_=soundHandledSerial_=0;soundApproachReadyAt_=0;
    previousHoldId_=0;nextHoldChoiceAt_=0;failedHold_={};failedSound_={};
    estimatedPrefire_={};prefireHandled_.fill(0);nextPrefire_=prefireSoundEpisode_=prefireAngleEpisode_=0;prefireSoundAt_=-1;prefireSoundProvenance_=false;
    lastSoundAt_=-1;soundDirectionKnown_=false;
}
inline bool Brain::StartEstimatedNavigation(const SensorFrame&f,const TacticEstimate&a,const TacticEstimate*b,NavigationPurpose purpose,double readyAt){
    namespace tactic=humanlike_bot_tactics_navigation;
    const auto settings=TacticSettings();const auto now=f.self.nowSeconds;
    if(!tactic::Current(a,now,settings)||(b&&!tactic::Separated(a,*b,now,settings))||now<readyAt)return false;
    EstimatedNavigation request{};request.requested=true;request.key=++nextNavigationEstimate_;request.life=life_;
    request.purpose=purpose;request.usesHearing=a.heard||(b&&b->heard);
    request.estimatedGoal=b?math::Mul(math::Add(a.point,b->point),.5f):a.point;
    // All destinations are frozen local-floor navigation estimates. Visual
    // head/body samples and sound sectors provide no hidden floor/feet state.
    request.estimatedGoal.z=f.self.feet.z;
    request.evidenceAtSeconds=b?(std::max)(a.availableAt,b->availableAt):a.availableAt;
    request.readyAtSeconds=(std::max)(readyAt,request.evidenceAtSeconds);request.expiresAtSeconds=b?(std::min)(a.expiresAt,b->expiresAt):a.expiresAt;
    request.confidence=b?(std::min)(tactic::Confidence(a,now,settings),tactic::Confidence(*b,now,settings)):tactic::Confidence(a,now,settings);
    request.uncertaintyCm=b?(std::max)(tactic::Radius(a,now,settings),tactic::Radius(*b,now,settings))
        +math::Horizontal(math::Sub(a.point,b->point))*.5f:tactic::Radius(a,now,settings);
    if(!tactic::ValidRequest(request,f.self,now))return false;
    CancelEstimatedNavigation();navigationEstimate_=request;
    navigationFirst_=purpose==NavigationPurpose::SoundApproach?prefireSoundEpisode_:a.hypothesis;
    navigationSoundSerial_=purpose==NavigationPurpose::SoundApproach?a.serial:0;navigationSecond_=b?b->hypothesis:0;
    activity_=Activity::Search;return true;
}
inline void Brain::TacticsTick(const SensorFrame&f){
    namespace tactic=humanlike_bot_tactics_navigation;
    Scope(Domain::Decision);const auto settings=TacticSettings();const auto now=f.self.nowSeconds;
    for(auto&e:combatEstimates_){
        if(e.visualTrack){const auto t=Find(e.visualTrack);if(!t||t->lifeRetired)e={};}
        if(!tactic::Current(e,now,settings)||(!f.hearingAvailable&&e.heard))e={};
    }
    // Only already recognized, genuinely visible firing samples update this
    // cache. Hidden/deferred packets cannot create or refresh combat evidence.
    for(const auto&t:tracks_)if(t.used&&!t.retired&&t.visible&&t.firing&&t.firingAt>=0){
        TacticEstimate e{};e.valid=e.gunshot=true;e.visualTrack=t.id;e.point=t.point;e.point.z=f.self.feet.z;
        e.confidence=t.confidence;e.radiusCm=t.uncertainty;e.eventAt=t.firingAt;
        e.availableAt=(std::max)(t.firingAt,t.recognized);e.sampledAt=t.lastSeen;
        e.expiresAt=t.lastSeen+(std::min)(settings.ttl,static_cast<double>(Milliseconds(Param::MemoryDurationMs)));
        tactic::Insert(combatEstimates_,e,now,settings,nextHypothesis_);
    }
    if(failedSound_.episode&&(failedSound_.episode!=prefireSoundEpisode_||!f.hearingAvailable
       ||!tactic::Current(soundEstimate_,now,settings)
       ||ObserveNavigationDeparture(f,failedSound_.goal,failedSound_.escapeGoal,failedSound_.haveEscape)))failedSound_={};
    if(navigationEstimate_.requested&&navigationEstimate_.purpose!=NavigationPurpose::MapHold){
        // A newly acquired receipt in the same uncertain auditory episode
        // renews only its lease. Its original spatial plan remains committed.
        if(navigationEstimate_.purpose==NavigationPurpose::SoundApproach&&navigationFirst_==prefireSoundEpisode_
           &&f.hearingAvailable&&tactic::Current(soundEstimate_,now,settings)
           &&soundEstimate_.serial!=navigationSoundSerial_&&soundEstimate_.availableAt>=navigationEstimate_.evidenceAtSeconds
           &&soundEstimate_.expiresAt>navigationEstimate_.expiresAtSeconds){
            navigationEstimate_.expiresAtSeconds=soundEstimate_.expiresAt;navigationSoundSerial_=soundEstimate_.serial;
        }
        bool valid=tactic::ValidRequest(navigationEstimate_,f.self,now)&&(!navigationEstimate_.usesHearing||f.hearingAvailable);
        if(navigationEstimate_.purpose==NavigationPurpose::SoundApproach)
            valid=valid&&tactic::Current(soundEstimate_,now,settings);
        else {
            const TacticEstimate*a=nullptr,*b=nullptr;
            for(const auto&e:combatEstimates_){if(e.valid&&e.hypothesis==navigationFirst_)a=&e;if(e.valid&&e.hypothesis==navigationSecond_)b=&e;}
            valid=valid&&a&&b&&tactic::Separated(*a,*b,now,settings);
        }
        const bool matching=tactic::SameRequest(navigationEstimate_,f.space.estimatedNavigation)
            ||tactic::SoundLeaseExtension(f.space.estimatedNavigation,navigationEstimate_);
        if(!valid||(matching&&(f.space.estimatedGoalReached||f.space.estimatedGoalRejected))){
            const bool rejected=matching&&f.space.estimatedGoalRejected;
            if(rejected&&navigationEstimate_.purpose==NavigationPurpose::SoundApproach&&navigationFirst_==prefireSoundEpisode_){
                failedSound_={};failedSound_.episode=navigationFirst_;failedSound_.goal=navigationEstimate_.estimatedGoal;
            }
            CancelEstimatedNavigation();activity_=Activity::Search;searchUntil_=now+Milliseconds(Param::SearchDwellMs);
            if(rejected)ReasonNow(Reason::NavigationUnavailable);
        }
    }
    if(P(Param::ThirdPartyJoinWeight)<=0){combatPair_={};return;}
    // Cache per-source confidence/radius once; the pair scan never repeats
    // exponential decay for every pair or scans all empty-slot combinations.
    std::array<size_t,MaxSounds+MaxContacts> candidates{};size_t count=0;
    std::array<float,MaxSounds+MaxContacts> strengths{},radii{};
    for(size_t n=0;n<combatEstimates_.size();++n){const auto&e=combatEstimates_[n];
        if(!e.gunshot||!tactic::Current(e,now,settings))continue;
        strengths[n]=tactic::Confidence(e,now,settings);radii[n]=tactic::Radius(e,now,settings);candidates[count++]=n;
    }
    const TacticEstimate*a=nullptr,*b=nullptr;float best=-1;
    for(size_t n=0;n<count;++n)for(size_t m=n+1;m<count;++m){
        const auto i=candidates[n],j=candidates[m];const auto&first=combatEstimates_[i],&second=combatEstimates_[j];
        if((first.visualTrack&&first.visualTrack==second.visualTrack)
           ||math::Horizontal(math::Sub(first.point,second.point))<=radii[i]+radii[j])continue;
        const auto confidence=(std::min)(strengths[i],strengths[j]);
        if(confidence>best){best=confidence;a=&first;b=&second;}
    }
    if(!a||!b){combatPair_={};return;}
    const auto first=(std::min)(a->hypothesis,b->hypothesis),second=(std::max)(a->hypothesis,b->hypothesis);
    if(combatPair_.first!=first||combatPair_.second!=second){
        combatPair_={};combatPair_.first=first;combatPair_.second=second;
        // Share the stimulus origin with sensor acquisition; do not add the
        // third-party perceive delay again after it was already noticed.
        combatPair_.readyAt=(std::max)((std::max)(a->availableAt,b->availableAt),
            (std::max)(a->eventAt,b->eventAt)+Milliseconds(Param::ReactionThirdPartyPerceiveMs))
            +Milliseconds(Param::ReactionThirdPartyDecideMs)+Milliseconds(Param::ReactionThirdPartyStartMs);
    }
}
inline void Brain::AttentionTick(const SensorFrame& f,double dt){
    Scope(Domain::Attention);
    if(lastHurtAt_>=0&&!hurtAcknowledged_&&f.self.nowSeconds>=hurtNoticeAt_){hurtAcknowledged_=true;
        if(rng_.Chance(Inclination(Param::AttentionHurtRecoveryWeight))){attention_=Attention::Recovering;attentionUntil_=f.self.nowSeconds+Milliseconds(Param::AttentionRecoveryMs);}
        if(rng_.Chance(P(Param::AttentionStressError))){Interrupt(false,true,true);releasePending_=true;ReasonNow(Reason::ActionInterrupted);}}
    if(lastSoundAt_>=0&&!soundAcknowledged_&&f.self.nowSeconds>=soundNoticeAt_){soundAcknowledged_=true;
        if(rng_.Chance(Inclination(Param::AttentionSoundRecoveryWeight))){attention_=Attention::Recovering;attentionUntil_=f.self.nowSeconds+Milliseconds(Param::AttentionRecoveryMs);}}
    if(attention_==Attention::Distracted||attention_==Attention::Recovering||attention_==Attention::Focused){
        if(f.self.nowSeconds>=attentionUntil_){
            if(attention_==Attention::Recovering){attention_=Attention::Focused;attentionUntil_=f.self.nowSeconds+Milliseconds(Param::AttentionFocusedDurationMs);}
            else attention_=Attention::Normal;
        }
    }
    const auto idle=(std::max)(0.0,f.self.nowSeconds-lastSalientAt_);
    const float boredom=idle>=Milliseconds(Param::AttentionIdleBoredomMs)?Inclination(Param::AttentionHoldBoredomWeight):0;
    const auto probability=P(Param::AttentionLapseRatePerMinute)*static_cast<float>(dt)/60*(1+boredom);
    if(attention_==Attention::Normal&&rng_.Chance(probability)){
        attention_=Attention::Distracted;attentionUntil_=f.self.nowSeconds+(std::max)(0.0f,Milliseconds(Param::AttentionLapseDurationMs)+rng_.Signed()*Milliseconds(Param::AttentionLapseDurationJitterMs));
    }
    lastAttentionAt_=f.self.nowSeconds;
}
inline void Brain::Remember(const SensorFrame& f){
    Scope(Domain::Memory);
    if(!f.perceptionValid)return;
    std::array<TrackId,MaxContacts> previousVisible{};size_t visibleCount=0;
    for(const auto& t:tracks_)if(t.used&&t.visible&&!t.retired)previousVisible[visibleCount++]=t.id;
    for(auto& t:tracks_)if(t.used)t.visible=false;
    for(size_t n=0;n<f.contactCount;++n){const auto& c=f.contacts[n];
        if(!c.permitted||!c.recognizedBySensor||!c.track||!math::Finite(c.observedAtSeconds)||c.observedAtSeconds>f.self.nowSeconds
           ||!math::Finite(c.firstExposedAtSeconds)||!math::Finite(c.recognizedAtSeconds)||c.firstExposedAtSeconds>c.recognizedAtSeconds||c.recognizedAtSeconds>f.self.nowSeconds
           ||!math::Finite(c.confidence)||c.confidence<0||c.confidence>1)continue;
        auto t=Find(c.track);
        if(t&&t->lifeRetired)continue;
        bool wasVisible=false;
        if(t)for(size_t p=0;p<visibleCount;++p)wasVisible=wasVisible||previousVisible[p]==t->id;
        if(c.visibility!=Visibility::Visible){
            // A frozen hidden/deferred packet never creates or refreshes a
            // memory and never supplies a new hidden point/velocity.
            if(t){t->visible=false;t->deferredSight=c.visibility==Visibility::Deferred&&(wasVisible||t->deferredSight);}continue;
        }
        if(c.pointCount==0||c.pointCount>MaxVisiblePoints||f.self.nowSeconds-c.observedAtSeconds>Milliseconds(Param::VisualStaleMs))continue;
        if(t&&c.observedAtSeconds<=t->lastPacket){t->visible=!t->retired;t->deferredSight=false;
            // A backend may revoke an already perceived fire cue on source,
            // visibility or age loss without refreshing the old target point.
            if(!c.firingVisuallyObserved){t->firing=false;t->firingAt=-1;}continue;}
        const float attentional=c.confidence*(attention_==Attention::Distracted?P(Param::AttentionVisualScale):1);
        if(attentional<P(Param::MemoryMinimumConfidence))continue;
        // Head and body are two preference groups. Extra visible shoulder
        // samples improve coverage without multiplying the body preference.
        std::array<float,2> confidence{};
        for(size_t p=0;p<c.pointCount;++p)if(math::Finite(c.points[p].point)
           &&math::Finite(c.points[p].confidence)&&c.points[p].confidence>0){
            const bool head=c.points[p].kind==PointKind::NativeHead||c.points[p].kind==PointKind::Eye;
            confidence[head?0:1]+=c.points[p].confidence;
        }
        size_t selected=MaxVisiblePoints;
        const bool headBecameAvailable=t&&!t->headAvailableAtSelection&&confidence[0]>0;
        if(t&&!t->reselectPoint&&!t->retired&&!headBecameAvailable){float nearest=std::numeric_limits<float>::max();
            for(size_t p=0;p<c.pointCount;++p)if(c.points[p].kind==t->pointKind&&math::Finite(c.points[p].point)
               &&math::Finite(c.points[p].confidence)&&c.points[p].confidence>0){
                const auto distance=math::Distance(c.points[p].point,t->point);if(distance<nearest){nearest=distance;selected=p;}}
        }
        const bool selecting=selected==MaxVisiblePoints;
        std::array<float,2> groupWeight{};
        if(selecting){groupWeight[0]=confidence[0]>0?P(Param::AimHeadWeight):0;
            groupWeight[1]=confidence[1]>0?P(Param::AimBodyWeight):0;}
        float draw=rng_.Unit()*(groupWeight[0]+groupWeight[1]);
        for(size_t p=0;selected==MaxVisiblePoints&&p<c.pointCount;++p)if(math::Finite(c.points[p].point)
           &&math::Finite(c.points[p].confidence)&&c.points[p].confidence>0){
            const bool head=c.points[p].kind==PointKind::NativeHead||c.points[p].kind==PointKind::Eye;
            const auto group=head?0:1;
            const auto weight=groupWeight[group]*c.points[p].confidence/confidence[group];
            if(weight<=0)continue;draw-=weight;if(draw<=0){selected=p;break;}
        }
        if(selected==MaxVisiblePoints)continue;
        if(!t){for(auto& candidate:tracks_)if(!candidate.used||candidate.retired){t=&candidate;break;}}
        if(!t){t=&tracks_[0];for(auto& candidate:tracks_)if(candidate.lastSeen<t->lastSeen)t=&candidate;}
        const bool newTrack=!t->used||t->id!=c.track||t->retired;
        if(newTrack){*t={};t->used=true;t->id=c.track;t->firstEvidence=c.firstExposedAtSeconds;t->recognized=c.recognizedAtSeconds;
            if(attention_==Attention::Normal){Scope(Domain::Attention);attention_=Attention::Focused;attentionUntil_=f.self.nowSeconds+Milliseconds(Param::AttentionFocusedDurationMs);Scope(Domain::Memory);}}
        else {
            // A budget gap does not create a new stimulus for an already
            // recognized exposure that is still inside the sight freshness bound.
            const bool sameExposureAfterBudgetGap=t->deferredSight
                &&c.observedAtSeconds-t->lastSeen<=Milliseconds(Param::VisualStaleMs)
                &&c.firstExposedAtSeconds<=t->lastSeen&&c.recognizedAtSeconds<=t->lastSeen;
            if(!wasVisible&&!sameExposureAfterBudgetGap)t->reappearedAt=c.observedAtSeconds;}
        t->deferredSight=false;
        if(t->motionKnown&&c.motionKnown&&math::Dot(t->velocity,c.observedVelocity)<0){t->motionChangedAt=c.observedAtSeconds;t->directionReaction=SampleReaction(cfg::ReactionContext::DirectionChange);}
        if(t->postureKnown&&c.postureKnown&&(t->crouched!=c.observedCrouched||t->airborne!=c.observedAirborne)){t->postureChangedAt=c.observedAtSeconds;t->postureReaction=SampleReaction(cfg::ReactionContext::CrouchJump);}
        // Reconsider a body-only selection once the head is actually seen.
        // A deliberate body choice with an already visible head stays stable.
        if(selecting)t->headAvailableAtSelection=confidence[0]>0;
        t->previousVelocity=t->velocity;t->point=c.points[selected].point;t->pointKind=c.points[selected].kind;
        t->velocity=c.motionKnown&&math::Finite(c.observedVelocity)?c.observedVelocity:Vec3{};
        t->motionKnown=c.motionKnown;t->postureKnown=c.postureKnown;t->crouched=c.observedCrouched;t->airborne=c.observedAirborne;
        if(c.firingVisuallyObserved&&!t->firing)t->firingAt=c.observedAtSeconds;
        if(!c.firingVisuallyObserved)t->firingAt=-1;
        t->firing=c.firingVisuallyObserved;t->baseConfidence=math::Clamp(attentional,0,1);t->confidence=t->baseConfidence;
        t->exposure=c.exposure;t->lastSeen=c.observedAtSeconds;t->lastPacket=c.observedAtSeconds;t->visible=true;t->retired=false;t->reselectPoint=false;t->uncertainty=0;
        t->history[t->historyWrite++%SeenHistoryCapacity]={t->point,t->velocity,t->lastSeen,t->motionKnown};t->historyCount=(std::min)(t->historyCount+1,SeenHistoryCapacity);
        seenAnyTarget_=true;lastSalientAt_=f.self.nowSeconds;
    }
    lastPerception_=f.perceivedAtSeconds;
}
inline void Brain::Decay(double now){
    Scope(Domain::Memory);
    for(auto& t:tracks_)if(t.used&&!t.retired){
        const auto age=(std::max)(0.0,now-t.lastSeen);
        if(age>Milliseconds(Param::VisualStaleMs))t.visible=false;
        // TTL concerns lost-contact memory, not the interval between two
        // current visible samples. TTL=0 remains usable while truly visible.
        if(t.visible){t.confidence=t.baseConfidence;t.uncertainty=0;continue;}
        if(age>Milliseconds(Param::MemoryDurationMs)){t.retired=true;t.visible=false;t.confidence=0;continue;}
        const auto half=Milliseconds(Param::MemoryConfidenceHalfLifeMs);
        const auto retained=half>0?std::exp2(-age/half):(age==0?1.0:0.0);
        t.confidence=math::Clamp(static_cast<float>(t.baseConfidence*retained-age*P(Param::MemoryConfidenceDecayPerS)),0,1);
        t.uncertainty=static_cast<float>(age)*P(Param::MemoryUncertaintyGrowthCmS);
    }
}
inline float Brain::TargetScore(const SensorFrame& f,const Track& t){
    const auto distance=TargetDistance(f.self,t),angle=math::Angle(f.self.view,math::Look(f.self.eye,t.point));
    const auto range=P(Param::VisualMaxDistanceCm);float value=t.confidence*(t.visible?P(Param::DuelSeekWeight):P(Param::MemoryContinueSearchWeight));
    value+=range>0?(std::max)(0.0f,1-distance/range)*P(Param::MultiThreatWeight):0;
    value+=(1-angle/180)*P(Param::CurrentTargetStickWeight);
    if(t.firing)value+=P(Param::FlankRiskWeight);if(t.id==aim_.target)value+=P(Param::SwitchStickiness);
    return value;
}
inline Brain::Track* Brain::SelectTarget(const SensorFrame& f){
    Scope(Domain::Decision);Track* best=nullptr;float score=0;
    for(auto& t:tracks_)if(t.used&&!t.retired&&t.confidence>=P(Param::MemoryMinimumConfidence)){
        const auto value=TargetScore(f,t);if(!best||value>score){best=&t;score=value;}
    }
    return best;
}
inline bool Brain::ActionConditions(float anyMask,float allMask,const SensorFrame& f,Track* target){
    uint32_t bits=0;const auto add=[&](cfg::Condition condition,bool yes){if(yes)bits|=static_cast<uint32_t>(condition);};
    add(cfg::Condition::KnownTarget,target!=nullptr);add(cfg::Condition::UnknownAngle,f.space.angleCount>0);
    add(cfg::Condition::VisibleOpponent,target&&target->visible);add(cfg::Condition::HeardShot,lastSoundAt_>=0&&lastSoundKind_==SoundKind::Gunshot&&f.self.nowSeconds>=soundNoticeAt_&&f.self.nowSeconds-lastSoundAt_<=Milliseconds(Param::HearingStimulusTtlMs));
    add(cfg::Condition::UnderFire,lastHurtAt_>=0&&f.self.nowSeconds>=hurtNoticeAt_&&f.self.nowSeconds-lastHurtAt_<=Milliseconds(Param::HurtMemoryMs));
    add(cfg::Condition::LowAmmo,f.self.weapon.ammoKnown&&f.self.weapon.magazineMaximum>0&&static_cast<float>(f.self.weapon.magazine)/f.self.weapon.magazineMaximum<=P(Param::ReloadMagazineThreshold));
    add(cfg::Condition::LowHealth,f.self.healthKnown&&f.self.maxHealth>0&&f.self.health/f.self.maxHealth<=P(Param::LowHealthThreshold));
    add(cfg::Condition::TargetAirborne,target&&target->postureKnown&&target->airborne);
    if(target){const auto distance=TargetDistance(f.self,*target);const auto band=distance<=P(Param::FireNearDistanceCm)?cfg::Distance::Near:distance<=P(Param::FireFarDistanceCm)?cfg::Distance::Mid:cfg::Distance::Far;
        add(cfg::Condition::NearRange,band==cfg::Distance::Near);add(cfg::Condition::MidRange,band==cfg::Distance::Mid);add(cfg::Condition::FarRange,band==cfg::Distance::Far);}
    unsigned threats=0;for(const auto& t:tracks_)threats+=t.used&&!t.retired&&t.visible;
    add(cfg::Condition::MultipleThreats,threats>1);add(cfg::Condition::CoverProven,f.space.coverKnown&&f.space.retreatPathProven);
    add(cfg::Condition::InformationGoal,activity_==Activity::Information||activity_==Activity::Search);
    add(cfg::Condition::GunfightGoal,activity_==Activity::Engage);add(cfg::Condition::Roaming,activity_==Activity::Roam);
    const auto any=static_cast<uint32_t>(anyMask),all=static_cast<uint32_t>(allMask);
    return (!any||(bits&any)!=0)&&(bits&all)==all;
}
inline void Brain::Acquire(const SensorFrame& f,Track& target,bool switching){
    Scope(Domain::Aim,true);
    viewCommand_.commandKnown=false;
    if(previousAimTarget_!=target.id){recentHit_=recentMiss_=false;}
    Param perceive=Param::ReactionActivePeekPerceiveMs,decide=Param::ReactionActivePeekDecideMs,start=Param::ReactionActivePeekStartMs;
    auto context=cfg::ReactionContext::ActivePeek;
    if(eventActivity_==Activity::Hold&&motion_.peek==PeekKind::None){perceive=Param::ReactionHoldPerceiveMs;decide=Param::ReactionHoldDecideMs;start=Param::ReactionHoldStartMs;context=cfg::ReactionContext::Hold;}
    if(switching){perceive=Param::ReactionThirdPartyPerceiveMs;decide=Param::ReactionThirdPartyDecideMs;start=Param::ReactionThirdPartyStartMs;context=cfg::ReactionContext::ThirdParty;}
    const bool reappeared=!switching&&target.reappearedAt>=0;
    if(reappeared){perceive=Param::ReactionReappearPerceiveMs;decide=Param::ReactionReappearDecideMs;start=Param::ReactionReappearStartMs;context=cfg::ReactionContext::Reappear;}
    const auto bearing=math::Look(f.self.eye,target.point);
    if(FlankBearing(f.self,bearing.yaw)){
        perceive=Param::ReactionFlankPerceiveMs;decide=Param::ReactionFlankDecideMs;start=Param::ReactionFlankStartMs;
        context=cfg::ReactionContext::Flank;
    }
    aim_={};aim_.target=target.id;aim_.stage=switching?AimStage::Switch:AimStage::React;aim_.stageAt=f.self.nowSeconds;
    aim_.eventAt=(std::max)(target.firstEvidence,target.reappearedAt);
    const auto decisionScale=attention_==Attention::Distracted?P(Param::AttentionDecisionScale):1;
    // Visual recognition and scenario perception describe the same stimulus
    // edge. The motor-start constraints likewise take max, never double-add.
    aim_.noticeAt=(std::max)(target.recognized,aim_.eventAt+Milliseconds(perceive));
    if(reappeared)aim_.noticeAt=(std::max)(aim_.noticeAt,aim_.eventAt+Milliseconds(Param::TrackReacquireMs));
    aim_.decisionAt=(std::max)(f.self.nowSeconds,aim_.noticeAt)+Milliseconds(decide)*decisionScale;
    if(switching)aim_.decisionAt=(std::max)(aim_.decisionAt,f.self.nowSeconds+Milliseconds(Param::SwitchDecisionMs));
    const auto reaction=SampleReaction(context);aim_.decisionAt+=reaction.hesitationSeconds;
    aim_.startAt=aim_.decisionAt+(std::max)(Milliseconds(start),Milliseconds(Param::PrimaryDelayMs));
    const auto moving=f.self.movementValid&&math::Horizontal(f.self.velocity)>0;
    const float extra=moving?P(Param::PrimaryMovingErrorDeg):0;
    aim_.yawError=rng_.Signed()*(P(Param::PrimaryYawErrorDeg)+extra);
    aim_.pitchError=rng_.Signed()*(P(Param::PrimaryPitchErrorDeg)+extra);
    const float probability=rng_.Unit(),over=P(Param::PrimaryOvershootProbability),under=P(Param::PrimaryUndershootProbability);
    const float direction=math::Yaw(bearing.yaw-f.self.view.yaw)>=0?1.0f:-1.0f;
    if(probability<over)aim_.yawError+=direction*P(Param::PrimaryOvershootDeg);
    else if(probability<over+under)aim_.yawError-=direction*P(Param::PrimaryOvershootDeg);
    if(switching){aim_.yawError+=rng_.Signed()*P(Param::SwitchErrorDeg);aim_.pitchError+=rng_.Signed()*P(Param::SwitchErrorDeg);}
    aim_.yawError+=reaction.yawError;aim_.pitchError+=reaction.pitchError;
    aim_.jitterAt=f.self.nowSeconds;fire_.holding=false;fire_.window=false;releasePending_=switching;
    previousAimTarget_=target.id;
    if(switching)ReasonNow(Reason::TargetSwitchPending);
}
inline void Brain::AimTick(const SensorFrame& f,Track* target,double dt,Intent& intent){
    Scope(Domain::Aim);
    if(!target||target->retired||target->confidence<P(Param::MemoryMinimumConfidence)){aim_.stage=AimStage::Idle;aim_.target=0;Preaim(f,dt,intent);return;}
    // Keep legitimate memory for decisions, but once its configured lost-
    // sight hold ends, moving search looks at its route instead of tracking
    // that old point through a wall until the whole memory expires.
    if(!target->visible&&activity_==Activity::Search
       &&f.self.nowSeconds-target->lastSeen>Milliseconds(Param::MemoryLostHoldMs)){
        Preaim(f,dt,intent);return;
    }
    if(aim_.target!=target->id)Acquire(f,*target,aim_.target!=0);
    else if(target->reappearedAt>aim_.eventAt)Acquire(f,*target,false);
    if(f.self.nowSeconds<aim_.noticeAt){ReasonNow(Reason::RecognitionPending);return;}
    if(f.self.nowSeconds<aim_.decisionAt){ReasonNow(Reason::ReactionPending);return;}
    if(f.self.nowSeconds<aim_.startAt){ReasonNow(Reason::TurnPending);return;}
    if(ReplayView(f.self,target->id,false,intent))return;
    dt=ViewDelta((std::max)(aim_.startAt,aim_.stageAt));
    if(aim_.stage==AimStage::React)aim_.stage=AimStage::Primary;
    const float moving=f.self.movementValid?math::Horizontal(f.self.velocity):0;
    Vec3 point=target->point,observedVelocity=target->velocity;double observedAt=target->lastSeen;bool motionKnown=target->motionKnown;aim_.historyReady=true;
    if(aim_.stage==AimStage::Track&&target->historyCount){
        auto cutoff=f.self.nowSeconds-Milliseconds(Param::TrackDelayMs);double beforeChange=std::numeric_limits<double>::infinity();
        if(target->motionChangedAt>=0){const auto lag=(std::max)(Milliseconds(Param::TrackDirectionChangeDelayMs),
            Milliseconds(Param::ReactionDirectionChangePerceiveMs)+Milliseconds(Param::ReactionDirectionChangeDecideMs)+Milliseconds(Param::ReactionDirectionChangeStartMs))+target->directionReaction.hesitationSeconds;
            if(f.self.nowSeconds<target->motionChangedAt+lag)beforeChange=target->motionChangedAt;}
        if(target->postureChangedAt>=0&&f.self.nowSeconds<target->postureChangedAt+Milliseconds(Param::ReactionCrouchJumpPerceiveMs)+Milliseconds(Param::ReactionCrouchJumpDecideMs)+Milliseconds(Param::ReactionCrouchJumpStartMs)+target->postureReaction.hesitationSeconds)beforeChange=(std::min)(beforeChange,target->postureChangedAt);
        const SeenSample* selected=nullptr;const auto oldestIndex=(target->historyWrite-target->historyCount)%SeenHistoryCapacity;const auto& oldest=target->history[oldestIndex];
        for(size_t n=0;n<target->historyCount;++n){const auto& sample=target->history[(oldestIndex+n)%SeenHistoryCapacity];if(sample.at<=cutoff&&sample.at<beforeChange)selected=&sample;}
        if(!selected){
            if(target->historyCount==SeenHistoryCapacity&&cutoff<oldest.at){aim_.historyReady=false;ReasonNow(Reason::ObservationHistoryUnavailable);return;}
            // Startup holds its first genuinely acquired sample. Its time is
            // kept intact; no pre-acquisition history is manufactured.
            selected=&oldest;
        }
        point=selected->point;observedVelocity=selected->velocity;observedAt=selected->at;motionKnown=selected->motionKnown;
    }
    if(motionKnown&&aim_.stage==AimStage::Track){
        const auto age=(std::max)(0.0,f.self.nowSeconds-observedAt);
        const auto base=Milliseconds(Param::TrackPredictionMs);
        auto prediction=base>0?(std::min)(static_cast<double>(Milliseconds(Param::TrackPredictionMaxMs)),base+age):0;
        if(!target->visible)prediction=(std::min)(prediction,static_cast<double>(Milliseconds(Param::MemoryMaximumPredictionMs)));
        point=math::Add(point,math::Mul(observedVelocity,static_cast<float>(prediction)));
    }
    const auto ideal=math::Look(f.self.eye,point);Rotation goal=ideal;
    float speed=0,smoothing=1;
    if(aim_.stage==AimStage::Primary||aim_.stage==AimStage::Switch){
        const auto cap=aim_.stage==AimStage::Switch?P(Param::SwitchSpeedDegS):P(Param::PrimarySpeedDegS);
        const auto remaining=math::Angle(f.self.view,{ideal.pitch+aim_.pitchError,ideal.yaw+aim_.yawError});
        const auto brake=std::sqrt((std::max)(0.0f,2*P(Param::PrimaryDecelerationDegS2)*remaining));
        aim_.angularSpeed=(std::min)((std::min)(cap,brake),aim_.angularSpeed+P(Param::PrimaryAccelerationDegS2)*static_cast<float>(dt));
        speed=aim_.angularSpeed;goal.pitch+=aim_.pitchError;goal.yaw+=aim_.yawError;
        if(math::Angle(f.self.view,goal)<=P(Param::PrimaryArrivalDeg)){
            aim_.stage=math::Angle(f.self.view,ideal)>P(Param::SecondaryThresholdDeg)?AimStage::Secondary:AimStage::Micro;
            aim_.stageAt=f.self.nowSeconds;aim_.startAt=f.self.nowSeconds+Milliseconds(Param::SecondaryDelayMs);aim_.corrections=0;
            aim_.yawError=rng_.Signed()*P(Param::SecondaryYawErrorDeg);aim_.pitchError=rng_.Signed()*P(Param::SecondaryPitchErrorDeg);
        }
    }else if(aim_.stage==AimStage::Secondary){
        if(f.self.nowSeconds<aim_.startAt){ReasonNow(Reason::TurnPending);return;}
        speed=P(Param::SecondarySpeedDegS);goal.pitch+=aim_.pitchError;goal.yaw+=aim_.yawError;
        if(math::Angle(f.self.view,goal)<=P(Param::SecondaryArrivalDeg)){
            ++aim_.corrections;
            if(math::Angle(f.self.view,ideal)<=P(Param::SecondaryThresholdDeg)||aim_.corrections>=static_cast<unsigned>(P(Param::SecondaryMaxCorrections))){
                aim_.stage=AimStage::Micro;aim_.stageAt=f.self.nowSeconds;aim_.jitterAt=0;
            }else{
                aim_.yawError=rng_.Signed()*P(Param::SecondaryYawErrorDeg);aim_.pitchError=rng_.Signed()*P(Param::SecondaryPitchErrorDeg);
                aim_.startAt=f.self.nowSeconds+Milliseconds(Param::SecondaryDelayMs);
            }
        }
    }else{
        const bool hold=activity_==Activity::Hold;
        speed=hold?P(Param::HoldCorrectionSpeedDegS):aim_.stage==AimStage::Micro?P(Param::MicroSpeedDegS):P(Param::TrackSpeedDegS);
        const auto jitterMs=aim_.stage==AimStage::Micro?Param::MicroJitterIntervalMs:Param::TrackJitterIntervalMs;
        if(f.self.nowSeconds>=aim_.jitterAt){
            const float stability=hold?P(Param::HoldStability):aim_.stage==AimStage::Micro?P(Param::MicroStability):0;
            const float yaw=hold?P(Param::HoldErrorDeg):aim_.stage==AimStage::Micro?P(Param::MicroJitterDeg):P(Param::TrackYawErrorDeg);
            const float pitch=hold?P(Param::HoldErrorDeg):aim_.stage==AimStage::Micro?P(Param::MicroJitterDeg):P(Param::TrackPitchErrorDeg);
            const float moveError=moving>0&&aim_.stage==AimStage::Track?P(Param::TrackMovingErrorDeg)*P(Param::MovingAimErrorScale):0;
            // Observed cm/s is not an angular error in degrees. Estimate
            // angular displacement over our configured tracking lag using
            // only the selected acquired point/velocity and our own eye.
            // This is an uncertainty model, never knowledge of a future pose.
            float motionYawError=0,motionPitchError=0;
            if(motionKnown){
                const auto lag=Milliseconds(Param::TrackDelayMs);
                const auto sight=math::Unit(math::Sub(point,f.self.eye));
                const auto tangent=math::Sub(observedVelocity,math::Mul(sight,math::Dot(observedVelocity,sight)));
                const auto estimate=math::Look(f.self.eye,math::Add(point,math::Mul(tangent,lag)));
                const auto scale=P(Param::AimTargetMotionErrorScale);
                motionYawError=std::abs(math::Yaw(estimate.yaw-ideal.yaw))*scale;
                motionPitchError=std::abs(estimate.pitch-ideal.pitch)*scale;
            }
            aim_.yawError=rng_.Signed()*(yaw*(1-stability)+moveError+motionYawError);
            aim_.pitchError=rng_.Signed()*(pitch*(1-stability)+moveError+motionPitchError);
            aim_.jitterAt=f.self.nowSeconds+Milliseconds(jitterMs);
        }
        if(aim_.stage==AimStage::Track&&target->motionChangedAt>=0){
            const auto lag=(std::max)((std::max)(Milliseconds(Param::TrackDelayMs),Milliseconds(Param::TrackDirectionChangeDelayMs)),
                Milliseconds(Param::ReactionDirectionChangePerceiveMs)+Milliseconds(Param::ReactionDirectionChangeDecideMs)+Milliseconds(Param::ReactionDirectionChangeStartMs))+target->directionReaction.hesitationSeconds;
            if(f.self.nowSeconds<target->motionChangedAt+lag){
                ReasonNow(Reason::ReactionPending);
            }else{
                dt=(std::min)(dt,ViewDelta(target->motionChangedAt+lag));
                auto& response=target->directionReaction;
                if(dt>0&&response.correctionUntil<0)response.correctionUntil=f.self.nowSeconds+Milliseconds(Param::TrackJitterIntervalMs);
                if(response.correctionUntil>=0&&f.self.nowSeconds<response.correctionUntil){goal.yaw+=response.yawError;goal.pitch+=response.pitchError;}
            }
        }
        if(target->postureChangedAt>=0){
            const auto deadline=target->postureChangedAt+Milliseconds(Param::ReactionCrouchJumpPerceiveMs)+Milliseconds(Param::ReactionCrouchJumpDecideMs)+Milliseconds(Param::ReactionCrouchJumpStartMs)+target->postureReaction.hesitationSeconds;
            if(f.self.nowSeconds<deadline){ReasonNow(Reason::ReactionPending);speed=0;}
            else if(aim_.stage==AimStage::Track){
                dt=(std::min)(dt,ViewDelta(deadline));
                auto& response=target->postureReaction;
                if(dt>0&&response.correctionUntil<0)response.correctionUntil=f.self.nowSeconds+Milliseconds(Param::TrackJitterIntervalMs);
                if(response.correctionUntil>=0&&f.self.nowSeconds<response.correctionUntil){goal.yaw+=response.yawError;goal.pitch+=response.pitchError;}
            }
        }
        goal.pitch+=aim_.pitchError;goal.yaw+=aim_.yawError;smoothing=aim_.stage==AimStage::Track?P(Param::TrackSmoothing):1;
        if(aim_.stage==AimStage::Micro&&math::Angle(f.self.view,ideal)<=P(Param::MicroPrecisionDeg))aim_.stage=AimStage::Track;
    }
    if(f.self.weapon.recoilKnown){
        goal.pitch-=f.self.weapon.pitchRecoilDegrees*P(Param::RecoilPitchCompensation);
        goal.yaw-=f.self.weapon.yawRecoilDegrees*P(Param::RecoilYawCompensation);
        goal.pitch+=aim_.pitchError*P(Param::RecoilErrorDeg);goal.yaw+=aim_.yawError*P(Param::RecoilErrorDeg);
    }
    if(attention_==Attention::Distracted)speed*=P(Param::AttentionVisualScale);
    StepView(f.self,goal,speed,dt,smoothing,target->id,false,intent);
    aim_.requestedGoal=goal;aim_.endpointKnown=true;
}
inline humanlike_bot_hold_positions::Settings Brain::HoldSettings(){
    return {P(Param::HoldConventionalWeight),P(Param::HoldUnconventionalWeight),P(Param::HoldCoverDistanceCm),
        P(Param::MoveArrivalCm),P(Param::CoverMaximumAnnotationDistanceCm),P(Param::CoverMinimumConfidence),
        Inclination(Param::SafetyWeight),Inclination(Param::FlankRiskWeight),Inclination(Param::AngleRiskWeight),
        static_cast<uint32_t>(P(Param::CoverMaximumExposedAngles))};
}
inline bool Brain::StartMapHold(const SensorFrame&f){
    namespace hold=humanlike_bot_hold_positions;Scope(Domain::Decision);
    const auto now=f.self.nowSeconds;
    if(navigationEstimate_.requested||now<nextHoldChoiceAt_||!f.self.groundedKnown||!f.self.grounded)return false;
    const auto settings=HoldSettings();std::array<float,MaxHoldLocations> weights{};float total=0;
    for(size_t n=0;n<f.space.holdLocationCount;++n){const auto&r=f.space.holdLocations[n];
        if(r.id==failedHold_.id&&r.revision==failedHold_.revision)continue;
        weights[n]=hold::Weight(r,f.self,settings);total+=weights[n];}
    // After a completed hold prefer a different authored physical position when
    // one is eligible; one available point remains usable after search dwell.
    bool alternative=false;
    for(size_t n=0;n<f.space.holdLocationCount;++n)alternative=alternative||(weights[n]>0&&f.space.holdLocations[n].id!=previousHoldId_);
    if(alternative)for(size_t n=0;n<f.space.holdLocationCount;++n)if(f.space.holdLocations[n].id==previousHoldId_){total-=weights[n];weights[n]=0;}
    if(total<=0)return false;float draw=rng_.Unit()*total;size_t selected=MaxHoldLocations;
    for(size_t n=0;n<f.space.holdLocationCount;++n)if(weights[n]>0){selected=n;draw-=weights[n];if(draw<=0)break;}
    if(selected>=MaxHoldLocations)return false;
    const auto&location=f.space.holdLocations[selected];EstimatedNavigation request{};
    request.requested=true;request.key=++nextNavigationEstimate_;request.life=life_;request.purpose=NavigationPurpose::MapHold;
    request.knowledge=NavigationKnowledge::StaticMap;request.mapAnnotationId=location.id;request.mapRevision=location.revision;
    request.estimatedGoal=location.position;request.confidence=location.confidence;
    request.evidenceAtSeconds=request.readyAtSeconds=now;
    const auto approachUntil=now+Milliseconds(Param::PatienceMs)+Milliseconds(Param::MoveStallMs);
    // Static map knowledge remains valid for this life and annotation revision.
    // Approach progress and the arrived hold timer own its behavioral lifetime.
    request.expiresAtSeconds=0;
    if(!humanlike_bot_tactics_navigation::ValidRequest(request,f.self,now))return false;
    motion_={};navigationEstimate_=request;mapHold_={};mapHold_.active=true;mapHold_.location=location;
    mapHold_.approachUntil=approachUntil;mapHold_.approachFeet=f.self.feet;mapHold_.approachSampleAt=f.self.sampledAtSeconds;
    activity_=Activity::Hold;return true;
}
inline bool Brain::ZeroOwnMotion(const OwnSnapshot& own){
    return own.movementValid&&math::Finite(own.velocity)&&own.velocity.x==0&&own.velocity.y==0&&own.velocity.z==0
        &&own.groundedKnown&&own.grounded;
}
inline void Brain::BeginArrival(ArrivalControl& s,const OwnSnapshot& own){
    s={};s.active=true;s.releasedAt=s.lastProgressAt=own.nowSeconds;s.lastFeet=own.feet;
    s.lastSpeed=own.movementValid&&math::Finite(own.velocity)?math::Length(own.velocity):0;
}
inline void Brain::ArrivalTick(ArrivalControl& s,const OwnSnapshot& own,Vec3 goal,bool correctionPath){
    s.needPath=false;if(!s.active||s.done||s.failed)return;
    const auto now=own.nowSeconds;const auto arrival=P(Param::MoveArrivalCm);
    // TickBots observes actual actor motion every50ms. A faster decision/own
    // clock is not a new motion frame; require at least this existing period.
    const auto cadence=(std::max)(.05,static_cast<double>(Milliseconds(Param::OwnSampleMs)));
    if(s.pulseSeconds<=0)s.pulseSeconds=cadence;
    if(now-s.lastProgressAt>Milliseconds(Param::MoveStallMs)){s.failed=true;s.pulsing=false;s.zeroSince=-1;return;}
    const auto distance=math::Distance(own.feet,goal);
    const bool current=own.valid&&own.alive&&!own.paused&&!own.retired&&math::Finite(own.feet)&&math::Finite(goal)
        &&math::Finite(own.sampledAtSeconds)&&own.sampledAtSeconds<=now
        &&now-own.sampledAtSeconds<=Milliseconds(Param::OwnSnapshotMaxAgeMs);
    if(!current||!own.movementValid||!math::Finite(own.velocity)||!own.groundedKnown||!own.grounded){
        if(s.pulsing){s.pulsing=false;s.releasedAt=now;}s.zeroSince=-1;return;
    }
    if(own.sampledAtSeconds<s.lastSampleAt){s.pulsing=false;s.releasedAt=now;s.zeroSince=-1;return;}
    const auto speed=math::Length(own.velocity);
    if(own.sampledAtSeconds>s.lastSampleAt){
        if(math::Distance(own.feet,s.lastFeet)>=arrival||speed<s.lastSpeed){s.lastProgressAt=now;s.lastFeet=own.feet;}
        s.lastSpeed=speed;s.lastSampleAt=own.sampledAtSeconds;
    }
    const bool at=distance<=arrival;
    if(s.pulsing){
        if(now>=s.pulseUntil||at||!correctionPath){s.pulsing=false;s.releasedAt=now;s.zeroSince=-1;}
        else{s.needPath=true;return;}
    }
    // Do not reverse a still-coasting body merely because it drifted outside.
    // Nor may an old pre-release zero snapshot prove a consumed stop request.
    if(!ZeroOwnMotion(own)||own.sampledAtSeconds<=s.releasedAt){s.zeroSince=-1;return;}
    if(at){
        if(s.zeroSince<0)s.zeroSince=own.sampledAtSeconds;
        if(own.sampledAtSeconds>s.zeroSince&&own.sampledAtSeconds-s.zeroSince>=cadence){s.done=true;s.active=false;}
        return;
    }
    s.zeroSince=-1;s.needPath=true;
    if(s.correctionIssued){
        // Original braking strength is unknown. If a full stop did not reduce
        // the measured miss, shorten the next input pulse rather than invent
        // a cm/s setpoint or reverse a moving body.
        if(distance>=s.stoppedDistance)s.pulseSeconds*=.5;
        s.correctionIssued=false;
    }
    // A correction is one bounded native-walk input pulse after a current
    // original path/ground proof; then release and observe actual zero again.
    if(correctionPath){s.pulsing=true;s.pulseUntil=now+s.pulseSeconds;s.stoppedDistance=distance;s.correctionIssued=true;}
}
inline void Brain::RecordFailedHold(){
    failedHold_={};failedHold_.id=mapHold_.location.id;failedHold_.revision=mapHold_.location.revision;
    failedHold_.goal=mapHold_.location.position;
}
inline bool Brain::ObserveNavigationDeparture(const SensorFrame&f,Vec3 failedGoal,Vec3&escapeGoal,bool&haveEscape){
    const auto arrival=P(Param::MoveArrivalCm);
    // Keep the previously published destination until actual own arrival,
    // including the pass that publishes a following static route.
    if(haveEscape&&humanlike_bot_hold_positions::Arrived(f.self,escapeGoal,arrival))return true;
    if(activity_!=Activity::Roam||f.space.estimatedNavigation.requested||!f.space.navigationKnown
       ||!f.space.pathProven||!f.space.groundProven||!f.space.routeId||(f.space.routeId&0x40000000U)
       ||!math::Finite(f.space.routeGoal)||math::Distance(f.space.routeGoal,failedGoal)<=arrival
       ||humanlike_bot_hold_positions::Arrived(f.self,f.space.routeGoal,arrival))return false;
    escapeGoal=f.space.routeGoal;haveEscape=true;return false;
}
inline void Brain::ObserveHoldDeparture(const SensorFrame&f){
    if(failedHold_.id&&ObserveNavigationDeparture(f,failedHold_.goal,failedHold_.escapeGoal,failedHold_.haveEscape))failedHold_={};
}
inline void Brain::MapHoldTick(const SensorFrame&f){
    namespace hold=humanlike_bot_hold_positions;Scope(Domain::Decision);
    ObserveHoldDeparture(f);
    if(!mapHold_.active)return;const auto now=f.self.nowSeconds;const auto settings=HoldSettings();
    const HoldLocation*current=nullptr;
    for(size_t n=0;n<f.space.holdLocationCount;++n)if(hold::SameLocation(mapHold_.location,f.space.holdLocations[n])){current=&f.space.holdLocations[n];break;}
    const bool matching=humanlike_bot_tactics_navigation::SameRequest(navigationEstimate_,f.space.estimatedNavigation);
    const bool invalid=!current||!hold::Eligible(mapHold_.location,f.self,settings)
        ||!humanlike_bot_tactics_navigation::ValidRequest(navigationEstimate_,f.self,now)
        ||(matching&&f.space.estimatedGoalRejected);
    // Only a fresh actual own-position displacement renews the approach
    // observation window. New requests, native velocity and path generations
    // cannot renew it. The adapter still owns blocked-path retirement.
    if(!mapHold_.arrived&&matching&&f.self.movementValid&&f.self.groundedKnown&&f.self.grounded
       &&f.self.sampledAtSeconds>mapHold_.approachSampleAt){
        mapHold_.approachSampleAt=f.self.sampledAtSeconds;
        humanlike_bot_navigation_progress::Settings progressSettings{};
        progressSettings.stallSeconds=Milliseconds(Param::PatienceMs)+Milliseconds(Param::MoveStallMs);
        const auto minimumProgress=humanlike_bot_navigation_progress::MinimumProgressCm(progressSettings);
        if(math::Distance(f.self.feet,mapHold_.approachFeet)>=minimumProgress){
            mapHold_.approachFeet=f.self.feet;
            mapHold_.approachUntil=now+progressSettings.stallSeconds;
        }
    }
    const bool timedOut=!mapHold_.arrived&&now>=mapHold_.approachUntil;
    if(invalid||timedOut||(mapHold_.arrived&&now>=mapHold_.leaveAt)){
        if(invalid||timedOut)RecordFailedHold();
        const auto id=mapHold_.location.id;CancelEstimatedNavigation();previousHoldId_=id;
        nextHoldChoiceAt_=searchUntil_=now+Milliseconds(Param::SearchDwellMs);holdUntil_=0;activity_=Activity::Search;
        if(invalid||timedOut)ReasonNow(Reason::NavigationUnavailable);return;
    }
    if(!matching)return;
    if(mapHold_.arrived&&(!f.space.holdProofReady||!hold::Arrived(f.self,mapHold_.location.position,settings.arrival))){
        RecordFailedHold();CancelEstimatedNavigation();nextHoldChoiceAt_=now+Milliseconds(Param::SearchDwellMs);holdUntil_=0;
        activity_=Activity::Search;ReasonNow(Reason::CoverUnavailable);return;
    }
    if(mapHold_.arrived&&!ZeroOwnMotion(f.self)){
        mapHold_.arrived=false;mapHold_.arrivedAt=mapHold_.leaveAt=-1;holdUntil_=0;BeginArrival(mapHold_.arrival,f.self);
    }
    if(!mapHold_.arrived&&f.space.estimatedGoalReached&&f.space.holdProofReady
       &&hold::Arrived(f.self,mapHold_.location.position,settings.arrival)&&!mapHold_.arrival.active)BeginArrival(mapHold_.arrival,f.self);
    if(!mapHold_.arrived&&mapHold_.arrival.active){
        const bool correction=f.space.holdProofReady&&f.space.navigationKnown&&f.space.pathProven&&f.space.groundProven
            &&!f.space.estimatedGoalReached&&!f.space.estimatedGoalRejected;
        ArrivalTick(mapHold_.arrival,f.self,mapHold_.location.position,correction);
        if(mapHold_.arrival.failed){RecordFailedHold();CancelEstimatedNavigation();nextHoldChoiceAt_=now+Milliseconds(Param::SearchDwellMs);holdUntil_=0;
            activity_=Activity::Search;ReasonNow(Reason::Stalled);return;}
    }
    if(!mapHold_.arrived&&mapHold_.arrival.done&&f.space.holdProofReady
       &&hold::Arrived(f.self,mapHold_.location.position,settings.arrival)&&ZeroOwnMotion(f.self)){
        mapHold_.arrived=true;mapHold_.arrivedAt=now;mapHold_.leaveAt=holdUntil_=now+Milliseconds(Param::HoldDurationMs);
    }
}
inline void Brain::StopEstimatedPrefire(double now,Reason reason){
    if(fire_.estimateKey){fire_.window=fire_.holding=false;fire_.needRelease=true;fire_.releaseRequestedAt=now;
        fire_.nextAt=(std::max)(fire_.nextAt,now+Milliseconds(Param::FireResetWaitMs));fire_.estimateKey=0;}
    estimatedPrefire_={};if(reason!=Reason::None)ReasonNow(reason);
}
inline void Brain::ConsiderEstimatedPrefire(const SensorFrame&f,EstimatedAim candidate,Rotation&goal){
    namespace pre=humanlike_bot_prefire;Scope(Domain::Fire);const auto now=f.self.nowSeconds;
    estimatedPrefire_.sourceCurrent=false;
    if(!candidate.requested||!Enabled(Param::FireEnabled)||P(Param::FirePrefireProbability)<=0
       ||candidate.confidence<P(Param::FirePrefireConfidence)||!math::Finite(candidate.point)
       ||candidate.acquiredAtSeconds<lifeAt_||now-candidate.acquiredAtSeconds>Milliseconds(Param::FirePrefireMaxAgeMs)){
        StopEstimatedPrefire(now);return;
    }
    if(candidate.source==EstimatedAimSource::Heard&&!Enabled(lastSoundKind_==SoundKind::Gunshot?Param::HearingGunshotEnabled:lastSoundKind_==SoundKind::Footstep?Param::HearingFootstepEnabled:lastSoundKind_==SoundKind::Landing?Param::HearingLandingEnabled:Param::HearingOtherCombatEnabled)){
        StopEstimatedPrefire(now);return;}
    const auto&old=estimatedPrefire_.aim;
    const bool same=old.requested&&old.source==candidate.source&&old.sourceEpisode==candidate.sourceEpisode
        &&old.mapAnnotationId==candidate.mapAnnotationId&&old.mapRevision==candidate.mapRevision;
    if(same){
        if(!pre::Valid(old,f.self,now)){StopEstimatedPrefire(now);return;}
        estimatedPrefire_.sourceCurrent=true;goal=math::Look(f.self.eye,old.point);return;
    }
    StopEstimatedPrefire(now);
    const auto source=static_cast<size_t>(candidate.source);if(source>=prefireHandled_.size()||!candidate.sourceEpisode)return;
    if(prefireHandled_[source]==candidate.sourceEpisode)return;prefireHandled_[source]=candidate.sourceEpisode;
    // One opportunity draw per genuine acquired episode / selected authored
    // angle dwell, not one roll per Tick. It never advances generic skill RNG.
    math::Rng chance{};chance.Seed(math::Mix(seed_)^math::Mix(stableBotId_)^math::Mix(life_.spawnEpoch)
        ^math::Mix(0x4850524546495245ULL+source)^math::Mix(candidate.sourceEpisode));
    if(!chance.Chance(P(Param::FirePrefireProbability)))return;
    candidate.key=++nextPrefire_;candidate.life=life_;
    candidate.readyAtSeconds=(std::max)(candidate.readyAtSeconds,candidate.acquiredAtSeconds)+Milliseconds(Param::FireFirstWaitMs);
    candidate.expiresAtSeconds=(std::min)(candidate.expiresAtSeconds,candidate.acquiredAtSeconds+Milliseconds(Param::FirePrefireMaxAgeMs));
    auto aim=math::Look(f.self.eye,candidate.point);aim.yaw+=aim_.yawError;
    aim.pitch=f.self.view.pitch+(aim.pitch-f.self.view.pitch)*P(Param::PreaimHeightCorrection);
    candidate.point=math::Add(f.self.eye,math::Mul(math::Forward(aim),math::Distance(f.self.eye,candidate.point)));
    if(candidate.readyAtSeconds>=candidate.expiresAtSeconds||!pre::Valid(candidate,f.self,now))return;
    estimatedPrefire_.aim=candidate;estimatedPrefire_.sourceCurrent=true;goal=aim;
}
inline bool Brain::CanEstimatedShoot(const SensorFrame&f,FireMode mode,Reason&reason){
    const auto&r=estimatedPrefire_.aim;const auto reject=[&](Reason value){reason=value;return false;};
    if(!estimatedPrefire_.sourceCurrent||!humanlike_bot_prefire::Valid(r,f.self,f.self.nowSeconds)
       ||r.confidence<P(Param::FirePrefireConfidence))return reject(Reason::TargetLost);
    if(r.source==EstimatedAimSource::Heard&&!f.hearingAvailable)return reject(Reason::HearingUnavailable);
    if(!humanlike_bot_prefire::Proof(r,f,(std::min)(Milliseconds(Param::OwnSnapshotMaxAgeMs),Milliseconds(Param::FirePrefireMaxAgeMs))))return reject(Reason::VisualUnknown);
    if(!f.self.groundedKnown||!f.self.grounded)return reject(Reason::GroundingUnknown);
    const auto bearing=math::Look(f.self.eye,r.point);
    auto horizontal=P(Param::VisualFovHorizontal);
    if(f.self.nativeViewFovKnown&&math::Finite(f.self.nativeHorizontalFovDegrees)&&f.self.nativeHorizontalFovDegrees>0)horizontal=(std::min)(horizontal,f.self.nativeHorizontalFovDegrees);
    if(std::abs(math::Yaw(bearing.yaw-f.self.view.yaw))>horizontal*.5f||std::abs(bearing.pitch-f.self.view.pitch)>P(Param::VisualFovVertical)*.5f)return reject(Reason::OutsideFieldOfView);
    if(!f.eventFeedValid||!f.self.weapon.shotFeedAvailable)return reject(Reason::ActualShotFeedUnavailable);
    if(f.self.nowSeconds<r.readyAtSeconds)return reject(Reason::ReactionPending);
    if(!WeaponFireReady(f,math::Distance(f.self.eye,r.point),mode,reason))return false;
    return MotionAimFireReady(f,r.point,mode,reason);
}
inline void Brain::Preaim(const SensorFrame& f,double dt,Intent& intent){
    Scope(Domain::Decision);Vec3 point{};bool have=false;EstimatedAim opportunity{};
    estimatedPrefire_.sourceCurrent=false;
    double notBefore=lastNeutralAt_;
    const bool hurtLive=hurtDirectionKnown_&&lastHurtAt_>=0&&f.self.nowSeconds-lastHurtAt_<=Milliseconds(Param::HurtMemoryMs);
    const bool soundLive=soundDirectionKnown_&&lastSoundAt_>=0&&f.self.nowSeconds-lastSoundAt_<=Milliseconds(Param::HearingStimulusTtlMs);
    const bool approachingSound=f.space.navigationKnown&&f.space.pathProven&&navigationEstimate_.requested
        &&navigationEstimate_.purpose==NavigationPurpose::SoundApproach;
    auto hurtSearchAt=(std::max)(hurtDirectionAt_,hurtActionAt_-hurtReaction_.hesitationSeconds)+hurtReaction_.hesitationSeconds;
    auto soundSearchAt=soundDecisionAt_;
    if(hurtLive&&hurtFlank_)hurtSearchAt=(std::max)(hurtDirectionAt_,DirectionActionAt(lastHurtAt_,hurtNoticeAt_,hurtActionAt_-hurtReaction_.hesitationSeconds,true,
        Param::ReactionHurtDecideMs,Param::ReactionHurtStartMs,Milliseconds(Param::HurtActionAdjustMs)))+hurtReaction_.hesitationSeconds;
    if(soundLive&&soundFlank_)soundSearchAt=DirectionActionAt(soundEventAt_,soundNoticeAt_,soundDecisionAt_-soundReaction_.hesitationSeconds,true,
        Param::ReactionHearingDecideMs,Param::ReactionHearingStartMs)+soundReaction_.hesitationSeconds;
    if(hurtLive&&f.self.nowSeconds>=hurtSearchAt){
        notBefore=(std::max)(notBefore,hurtSearchAt);
        point=math::Add(f.self.eye,math::Mul(math::Forward({0,hurtBearing_}),P(Param::VisualMaxDistanceCm)*hurtConfidence_));have=true;
    }else if(soundLive&&!approachingSound&&f.self.nowSeconds>=soundSearchAt){
        notBefore=(std::max)(notBefore,soundSearchAt);
        point=math::Add(f.self.eye,math::Mul(math::Forward({0,soundBearing_}),soundDistance_));have=soundDistance_>0;
        const auto settings=TacticSettings();
        if(have&&f.hearingAvailable&&prefireSoundProvenance_&&prefireSoundEpisode_&&humanlike_bot_tactics_navigation::Current(soundEstimate_,f.self.nowSeconds,settings)){
            opportunity.requested=true;opportunity.source=EstimatedAimSource::Heard;opportunity.sourceEpisode=prefireSoundEpisode_;
            opportunity.point=soundEstimate_.point;opportunity.point.z+=f.self.eye.z-f.self.feet.z;
            opportunity.acquiredAtSeconds=prefireSoundAt_;opportunity.readyAtSeconds=(std::max)(soundApproachReadyAt_,soundSearchAt);
            opportunity.expiresAtSeconds=soundEstimate_.expiresAt;opportunity.confidence=humanlike_bot_tactics_navigation::Confidence(soundEstimate_,f.self.nowSeconds,settings);
            opportunity.uncertaintyCm=humanlike_bot_tactics_navigation::Radius(soundEstimate_,f.self.nowSeconds,settings);
        }
    }else if((hurtLive&&hurtFlank_)||(soundLive&&!approachingSound&&soundFlank_))ReasonNow(Reason::ReactionPending);
    if(!have&&motion_.peek==PeekKind::InfoRepeek&&motion_.stage!=MotionStage::Idle&&math::Finite(motion_.informationFocus)){
        point=motion_.informationFocus;have=true;
    }
    if(!have&&mapHold_.active&&mapHold_.arrived){point=mapHold_.location.focus;have=true;
        opportunity.requested=true;opportunity.source=EstimatedAimSource::AuthoredHold;opportunity.sourceEpisode=navigationEstimate_.key;
        opportunity.point=point;opportunity.mapAnnotationId=mapHold_.location.id;opportunity.mapRevision=mapHold_.location.revision;
        opportunity.acquiredAtSeconds=mapHold_.arrivedAt;opportunity.readyAtSeconds=mapHold_.arrivedAt;
        opportunity.expiresAtSeconds=mapHold_.leaveAt;opportunity.confidence=mapHold_.location.confidence;
    }
    const bool travelling=motion_.peek==PeekKind::None&&(activity_==Activity::Roam||activity_==Activity::Search||activity_==Activity::Relocate
        ||(mapHold_.active&&!mapHold_.arrived));
    if(!have&&travelling&&f.space.navigationKnown&&f.space.pathProven){
        const AngleHint* route=nullptr;
        for(size_t n=0;n<f.space.angleCount;++n){const auto&a=f.space.angles[n];
            if(a.valid&&a.staticMapKnowledge&&a.routeDirected&&math::Finite(a.focusPoint)){route=&a;break;}}
        if(route){point=route->focusPoint;have=true;
            if(preaimAngleId_!=route->id||f.self.nowSeconds>=angleUntil_){
                preaimAngleId_=route->id;angleUntil_=f.self.nowSeconds+Milliseconds(Param::SearchDwellMs);
                aim_.yawError=rng_.Signed()*(P(Param::PreaimYawErrorDeg)+P(Param::PreaimHoldJitterDeg));
                aim_.pitchError=rng_.Signed()*P(Param::PreaimHeadHeightErrorCm);
            }
            point.z+=aim_.pitchError;
        }else{
            // No invented visible angle: the next original path segment is
            // only a travel direction, never an enemy or prefire proof.
            point=f.space.nextPathPoint;point.z=f.self.eye.z;have=true;
        }
    }
    if(!have&&f.space.angleCount>0){
        const bool change=f.self.nowSeconds>=angleUntil_||searchAngle_>=f.space.angleCount
            ||f.space.angles[searchAngle_].id!=preaimAngleId_;
        if(change){
            size_t selected=MaxAngles;float total=0;std::array<float,MaxAngles> weights{};
            for(size_t n=0;n<f.space.angleCount;++n){const auto& a=f.space.angles[n];if(!a.valid||!a.staticMapKnowledge||!math::Finite(a.focusPoint))continue;
                bool recent=false;for(const auto& old:angles_)if(old.id==a.id&&old.checkedAt>=0&&f.self.nowSeconds-old.checkedAt<Milliseconds(Param::SearchAngleMemoryMs))recent=true;
                if(recent&&rng_.Chance(Inclination(Param::PreaimRetainWeight)))continue;
                if(rng_.Chance(P(Param::SearchOmitProbability))||rng_.Chance(P(Param::PreaimOmitProbability)))continue;
                weights[n]=(std::max)(0.0f,1-a.risk*Inclination(Param::SearchRiskWeight));total+=weights[n];
            }
            float draw=rng_.Unit()*total;
            for(size_t n=0;n<f.space.angleCount;++n){const auto& a=f.space.angles[n];if(!a.valid||!a.staticMapKnowledge||!math::Finite(a.focusPoint))continue;
                const float weight=weights[n];
                if(weight<=0)continue;draw-=weight;if(draw<=0&&total>0){selected=n;break;}
            }
            if(selected<MaxAngles){
                if(f.space.angleCount>1&&rng_.Chance(P(Param::PreaimWrongGuessProbability))){
                    for(size_t offset=1;offset<f.space.angleCount;++offset){const auto alternative=(selected+offset)%f.space.angleCount;
                        if(weights[alternative]>0){selected=alternative;break;}}
                }
                ++prefireAngleEpisode_;searchAngle_=static_cast<uint32_t>(selected);preaimAngleId_=f.space.angles[selected].id;
                angleUntil_=f.self.nowSeconds+Milliseconds(Param::SearchDwellMs);
                angles_[selected%angles_.size()]={f.space.angles[selected].id,f.self.nowSeconds};
                aim_.yawError=rng_.Signed()*(P(Param::PreaimYawErrorDeg)+P(Param::PreaimHoldJitterDeg));
                aim_.pitchError=rng_.Signed()*P(Param::PreaimHeadHeightErrorCm);
                preaimMargin_=P(Param::PreaimWallMarginCm)*(rng_.Chance(Inclination(Param::PreaimExpectedWideWeight))?1.0f:-1.0f);
            }
        }
        if(searchAngle_<f.space.angleCount&&f.space.angles[searchAngle_].valid&&f.space.angles[searchAngle_].staticMapKnowledge){
            point=f.space.angles[searchAngle_].focusPoint;point.z+=aim_.pitchError;have=true;
            const auto side=math::FlatUnit({-(point.y-f.self.eye.y),point.x-f.self.eye.x,0});
            point=math::Add(point,math::Mul(side,preaimMargin_));
            const auto&a=f.space.angles[searchAngle_];
            if(a.authoredStatic&&a.mapRevision){opportunity.requested=true;opportunity.source=EstimatedAimSource::AuthoredAngle;
                opportunity.sourceEpisode=prefireAngleEpisode_;opportunity.mapAnnotationId=a.id;opportunity.mapRevision=a.mapRevision;
                opportunity.point=point;opportunity.confidence=a.heightConfidence;
                opportunity.acquiredAtSeconds=angleUntil_-Milliseconds(Param::SearchDwellMs);
                opportunity.readyAtSeconds=opportunity.acquiredAtSeconds;opportunity.expiresAtSeconds=angleUntil_;}

        }
    }
    if(!have&&f.space.navigationKnown&&f.space.pathProven){point=f.space.nextPathPoint;point.z=f.self.eye.z;have=true;}
    if(!have){StopEstimatedPrefire(f.self.nowSeconds);viewCommand_.commandKnown=false;viewCommand_.spent=true;ReasonNow(Reason::NavigationUnavailable);return;}
    auto goal=math::Look(f.self.eye,point);goal.yaw+=aim_.yawError;goal.pitch=f.self.view.pitch+(goal.pitch-f.self.view.pitch)*P(Param::PreaimHeightCorrection);
    ConsiderEstimatedPrefire(f,opportunity,goal);Scope(Domain::Decision);
    dt=ViewDelta(notBefore);StepView(f.self,goal,P(Param::PreaimTransferSpeedDegS),dt,1,0,true,intent);
}
inline bool Brain::InformationMemoryReady(const SensorFrame&f,const Track&t,bool unhandled){
    const auto hold=Milliseconds(Param::MemoryLostHoldMs);
    return t.used&&t.id&&!t.retired&&!t.lifeRetired&&hold>0&&math::Finite(t.lastSeen)
        &&t.lastSeen>=0&&t.lastSeen<=f.self.nowSeconds&&f.self.nowSeconds-t.lastSeen<=hold
        &&t.confidence>=P(Param::MemoryMinimumConfidence)&&t.confidence>=P(Param::InfoRepeekMinimumConfidence)
        &&(!unhandled||t.lastSeen>t.informationHandledAt);
}
inline const AngleHint* Brain::InformationAngleReady(const SensorFrame&f){
    const auto remember=Milliseconds(Param::SearchAngleMemoryMs);
    for(size_t n=0;n<f.space.angleCount;++n){const auto&a=f.space.angles[n];
        if(!a.id||!a.valid||!a.staticMapKnowledge||!a.coverFocusProven||!math::Finite(a.focusPoint))continue;
        bool recent=false;for(const auto&old:informationAngles_)if(old.id==a.id&&old.checkedAt>=0
            &&f.self.nowSeconds>=old.checkedAt&&f.self.nowSeconds-old.checkedAt<remember)recent=true;
        if(!recent)return&a;
    }return nullptr;
}
inline bool Brain::AtPeekCover(const SensorFrame&f){
    const auto distance=math::Horizontal(math::Sub(f.self.feet,f.space.coverStart));
    return math::Finite(distance)&&distance<=P(Param::MoveArrivalCm);
}
inline bool Brain::InformationPurposeReady(const SensorFrame&f,Track*target){
    if(motion_.stage!=MotionStage::Idle||!Enabled(Param::InfoRepeekEnabled)||P(Param::InfoRepeekWeight)<=0)return false;
    if(target){if(target->visible||!InformationMemoryReady(f,*target,true))return false;}
    else if(!InformationAngleReady(f))return false;
    if(!f.self.movementValid||!f.self.groundedKnown||!f.self.grounded||!f.space.coverKnown||!f.space.groundProven
       ||!f.space.peekPathProven||!f.space.retreatPathProven||!f.space.clearanceKnown
       ||f.space.coverConfidence<P(Param::CoverMinimumConfidence)||f.space.exposureRisk>P(Param::InfoRepeekMaximumRisk))return false;
    if(!AtPeekCover(f))return false;
    if(!target&&f.space.coverConfidence<P(Param::InfoRepeekMinimumConfidence))return false;
    const auto span=math::Horizontal(math::Sub(f.space.peekOpen,f.space.coverStart));
    if(span<P(Param::InfoRepeekMinimumSpaceCm)||span<P(Param::InfoRepeekDistanceCm))return false;
    // Establish an information purpose from acquired memory and normal own
    // geometry. All other configured condition bits must already be true;
    // only this newly established goal supplies InformationGoal itself.
    const auto bit=static_cast<uint32_t>(cfg::Condition::InformationGoal);
    const auto any=static_cast<uint32_t>(P(Param::InfoRepeekConditionAny));
    const auto all=static_cast<uint32_t>(P(Param::InfoRepeekConditionAll));
    return ActionConditions(static_cast<float>((any&bit)?0U:any),static_cast<float>(all&~bit),f,target);
}
inline void Brain::Decide(const SensorFrame& f,Track* target){
    Scope(Domain::Decision);
    if(target&&target->visible)CancelEstimatedNavigation();
    if(fire_.reloadRequested){CancelEstimatedNavigation();activity_=Activity::Reload;return;}
    if(f.self.nowSeconds<nextDecisionAt_)return;
    nextDecisionAt_=f.self.nowSeconds+Milliseconds(Param::ActionSwitchMs);
    if(f.self.nowSeconds-lifeAt_<Milliseconds(Param::RespawnAlertMs)){activity_=Activity::Respawn;return;}
    unsigned threats=0;for(const auto& t:tracks_)threats+=t.used&&!t.retired&&t.visible;
    const bool underFire=lastHurtAt_>=0&&f.self.nowSeconds>=hurtActionAt_&&f.self.nowSeconds-lastHurtAt_<=Milliseconds(Param::HurtMemoryMs);
    const bool lowHealth=f.self.healthKnown&&f.self.maxHealth>0&&f.self.health/f.self.maxHealth<=P(Param::LowHealthThreshold);
    const bool lowAmmo=f.self.weapon.ammoKnown&&f.self.weapon.magazineMaximum>0&&static_cast<float>(f.self.weapon.magazine)/f.self.weapon.magazineMaximum<=P(Param::ReloadMagazineThreshold);
    const bool safeRetreat=f.space.coverKnown&&f.space.retreatPathProven&&f.space.coverConfidence>=P(Param::CoverMinimumConfidence);
    if((underFire||lowHealth||lowAmmo||threats>P(Param::ThreatTolerance))&&safeRetreat){
        const float danger=(lowHealth?P(Param::LowHealthRetreatWeight):0)+(lowAmmo?P(Param::LowAmmoRetreatWeight):0)+static_cast<float>(threats)*P(Param::MultiThreatWeight)+P(Param::SafetyWeight)+P(Param::RetreatWeight)+static_cast<float>(f.space.exposedAngleCount)*P(Param::AngleRiskWeight);
        const float duel=P(Param::DuelSeekWeight)+P(Param::WinFocusWeight);
        if(danger+duel>0&&rng_.Unit()*(danger+duel)<danger&&!rng_.Chance(P(Param::MisjudgmentProbability))){
            if(activity_!=Activity::Retreat){Interrupt(false,true,true);releasePending_=true;}
            CancelEstimatedNavigation();activity_=Activity::Retreat;return;
        }
    }
    if(underFire&&hurtDirectionKnown_&&f.self.nowSeconds>=hurtDirectionAt_){
        const float cover=P(Param::FlankCoverFirstWeight),turn=P(Param::FlankTurnFirstWeight);
        if(safeRetreat&&cover+turn>0&&rng_.Unit()*(cover+turn)<cover){CancelEstimatedNavigation();activity_=Activity::Retreat;return;}
    }
    if(lowAmmo&&f.self.weapon.reserveKnown&&f.self.weapon.reserve>0&&(!target||!target->visible||safeRetreat||f.self.weapon.magazine==0)){
        CancelEstimatedNavigation();activity_=Activity::Reload;return;
    }
    if(motion_.peek==PeekKind::InfoRepeek&&motion_.stage!=MotionStage::Idle){
        if(motion_.informationTrack&&target&&target->visible&&target->id!=motion_.informationTrack){
            Interrupt(false,true,true);releasePending_=true;activity_=Activity::Engage;ReasonNow(Reason::TargetSwitchPending);return;
        }
        activity_=Activity::Information;return;
    }
    if(target&&target->visible){
        if(activity_==Activity::Relocate){Interrupt(false,true,true);releasePending_=true;}
        relocateAt_=-1;relocationGoalKnown_=false;activity_=Activity::Engage;return;
    }
    if(relocateAt_>=0&&f.self.nowSeconds>=relocateAt_){
        relocateAt_=-1;
        if(f.space.navigationKnown&&f.space.pathProven&&f.space.groundProven&&math::Finite(f.space.routeGoal)){
            relocationGoal_=f.space.routeGoal;relocationRouteId_=f.space.routeId;relocationGoalKnown_=true;activity_=Activity::Relocate;return;
        }
        ReasonNow(Reason::NavigationUnavailable);
    }
    if(activity_==Activity::Relocate&&relocationGoalKnown_){
        if(math::Horizontal(math::Sub(f.self.feet,relocationGoal_))<=P(Param::MoveArrivalCm)){
            relocationGoalKnown_=false;activity_=Activity::Search;
        }else if(!f.space.navigationKnown||!f.space.pathProven||f.space.routeId!=relocationRouteId_){
            relocationGoalKnown_=false;activity_=Activity::Search;releasePending_=true;ReasonNow(Reason::NavigationUnavailable);return;
        }else return;
    }
    if(navigationEstimate_.requested&&navigationEstimate_.purpose==NavigationPurpose::ThirdPartyJoin){activity_=Activity::Search;return;}
    if(combatPair_.first&&combatPair_.second&&!combatPair_.decided&&f.self.nowSeconds>=combatPair_.readyAt){
        combatPair_.decided=true;
        const TacticEstimate*a=nullptr,*b=nullptr;
        for(const auto&e:combatEstimates_){if(e.valid&&e.hypothesis==combatPair_.first)a=&e;if(e.valid&&e.hypothesis==combatPair_.second)b=&e;}
        if(a&&b&&rng_.Chance(Inclination(Param::ThirdPartyJoinWeight))&&!rng_.Chance(P(Param::MisjudgmentProbability))
           &&StartEstimatedNavigation(f,*a,b,NavigationPurpose::ThirdPartyJoin,combatPair_.readyAt))return;
    }
    if(InformationPurposeReady(f,target)){CancelEstimatedNavigation();activity_=Activity::Information;return;}
    if(navigationEstimate_.requested){activity_=navigationEstimate_.purpose==NavigationPurpose::MapHold?Activity::Hold:Activity::Search;return;}
    if(target){
        activity_=f.self.nowSeconds-target->lastSeen<=Milliseconds(Param::MemoryLostHoldMs)?Activity::Hold:Activity::Search;
        return;
    }
    if(lastKillAt_>=0&&f.self.nowSeconds-lastKillAt_<=Milliseconds(Param::PatienceMs)){
        const auto search=P(Param::AfterKillSearchWeight),reload=P(Param::AfterKillReloadWeight),advance=P(Param::AfterKillAdvanceWeight),total=search+reload+advance;
        if(total>0){const auto draw=rng_.Unit()*total;
            if(draw<reload&&lowAmmo){activity_=Activity::Reload;return;}
            if(draw<reload+search){activity_=Activity::Search;return;}
        }
    }
    if(f.hearingAvailable&&lastSoundAt_>=0&&f.self.nowSeconds>=soundApproachReadyAt_
       &&f.self.nowSeconds-lastSoundAt_<Milliseconds(Param::HearingStimulusTtlMs)&&soundEstimate_.serial!=soundHandledSerial_
       &&prefireSoundEpisode_!=failedSound_.episode){
        soundHandledSerial_=soundEstimate_.serial;
        const auto approach=P(Param::SoundApproachWeight),wait=P(Param::SoundWaitWeight),avoid=P(Param::SoundAvoidWeight),total=approach+wait+avoid;
        if(total>0){const auto draw=rng_.Unit()*total;
            if(draw<avoid){activity_=safeRetreat?Activity::Retreat:Activity::Hold;return;}
            if(draw<avoid+wait){activity_=Activity::Hold;holdUntil_=(std::max)(holdUntil_,f.self.nowSeconds+Milliseconds(Param::PatienceMs));return;}
            if(StartEstimatedNavigation(f,soundEstimate_,nullptr,NavigationPurpose::SoundApproach,soundApproachReadyAt_))return;
            activity_=Activity::Hold;ReasonNow(Reason::NavigationUnavailable);return;
        }
    }
    if(f.self.nowSeconds<holdUntil_&&!rng_.Chance(P(Param::TimingMistakeProbability))){activity_=Activity::Hold;return;}
    // Commit to the current static route until arrival/replacement. Looking
    // at another angle cannot reroll this into a new stationary hold.
    if(activity_==Activity::Roam&&f.space.navigationKnown&&f.space.pathProven&&f.space.groundProven
       &&(!patrolRouteId_||patrolRouteId_==f.space.routeId)){
        patrolRouteId_=f.space.routeId;return;
    }
    const float roam=P(Param::RoamWeight)+P(Param::AdvanceWeight),hold=P(Param::DefendAreaWeight);
    if(activity_==Activity::Dead||activity_==Activity::Respawn||f.self.nowSeconds>=searchUntil_){
        if(roam+hold>0&&rng_.Unit()*(roam+hold)<hold){
            if(StartMapHold(f))return;
            if(failedHold_.id){activity_=Activity::Roam;holdUntil_=0;patrolRouteId_=f.space.pathProven?f.space.routeId:0;}
            else{activity_=Activity::Hold;holdUntil_=f.self.nowSeconds+Milliseconds(Param::HoldDurationMs);}
        }
        else {activity_=Activity::Roam;holdUntil_=0;patrolRouteId_=f.space.pathProven?f.space.routeId:0;}
    }
}
inline bool Brain::PickPeek(const SensorFrame& f,Track* target){
    Scope(Domain::Motion,true);
    bool anyEnabled=false;for(size_t n=0;n<cfg::PeekCount;++n)anyEnabled=Enabled(cfg::PeekParam(static_cast<cfg::Peek>(n),cfg::PeekField::Enabled))||anyEnabled;
    if(!anyEnabled)return false;
    if(!f.self.groundedKnown){ReasonNow(Reason::GroundingUnknown);return false;}
    if(!f.self.grounded){ReasonNow(Reason::NativeActionUnavailable);return false;}
    if(!f.space.coverKnown||!f.space.retreatPathProven||!f.space.peekPathProven||!f.space.groundProven){ReasonNow(Reason::CoverUnavailable);return false;}
    if(!f.space.clearanceKnown){ReasonNow(Reason::ClearanceUnavailable);return false;}
    // A local peek begins at its proved cover origin. Authored hold travel
    // continues through the independent MapHold navigation path.
    if(!AtPeekCover(f))return false;
    const auto available=math::Horizontal(math::Sub(f.space.peekOpen,f.space.coverStart));
    std::array<float,cfg::PeekCount> weights{};float total=0;
    for(size_t n=0;n<weights.size();++n){const auto kind=static_cast<PeekKind>(n+1);
        const auto field=[&](cfg::PeekField p){return PeekParameter(kind,static_cast<unsigned>(p));};
        if(!Enabled(field(cfg::PeekField::Enabled)))continue;
        if(!ActionConditions(P(field(cfg::PeekField::ConditionAny)),P(field(cfg::PeekField::ConditionAll)),f,target))continue;
        const auto confidence=target?target->confidence:f.space.coverConfidence;
        if(confidence<P(field(cfg::PeekField::MinimumConfidence))||f.space.exposureRisk>P(field(cfg::PeekField::MaximumRisk)))continue;
        float distance=P(field(cfg::PeekField::DistanceCm));if(kind==PeekKind::Poppin)distance+=P(Param::PoppinExtraDistanceCm);
        if(available<P(field(cfg::PeekField::MinimumSpaceCm))||distance>available)continue;
        if(kind==PeekKind::Jump){
            if(!f.self.groundedKnown){ReasonNow(Reason::GroundingUnknown);continue;}
            if(!f.self.grounded||!f.space.landingSupportKnown){ReasonNow(Reason::JumpLandingUnproven);continue;}
        }
        if(kind==PeekKind::Crouch&&!f.self.stanceValid){ReasonNow(Reason::CrouchNotObserved);continue;}
        if(kind==PeekKind::Poppin&&(!target||!target->visible)){
            // First leg may use only this bot's genuine recent visual memory.
            // Zero disables memory entry; a current visible target bypasses it.
            const auto hold=Milliseconds(Param::MemoryLostHoldMs);
            const bool recent=target&&target->used&&!target->retired&&!target->lifeRetired&&hold>0
                &&math::Finite(target->lastSeen)&&target->lastSeen>=0&&target->lastSeen<=f.self.nowSeconds
                &&f.self.nowSeconds-target->lastSeen<=hold;
            if(!recent){ReasonNow(Reason::PoppinEvidenceMissing);continue;}
        }
        if(kind==PeekKind::InfoRepeek){
            if(target?!InformationMemoryReady(f,*target,true):!InformationAngleReady(f))continue;
        }
        const auto weight=P(field(cfg::PeekField::Weight));
        // Independent use tendency: a lone low-weight enabled technique need
        // not run at every opportunity. Eligible alternatives are weighted.
        if(weight>0&&rng_.Chance(Inclination(field(cfg::PeekField::Weight)))){weights[n]=weight;total+=weight;}
    }
    if(total<=0)return false;float draw=rng_.Unit()*total;size_t selected=weights.size();
    for(size_t n=0;n<weights.size();++n){if(weights[n]<=0)continue;draw-=weights[n];if(draw<=0){selected=n;break;}}
    if(selected==weights.size())return false;
    const auto kind=static_cast<PeekKind>(selected+1);const auto field=[&](cfg::PeekField p){return PeekParameter(kind,static_cast<unsigned>(p));};
    if(rng_.Chance(P(field(cfg::PeekField::ExecutionErrorProbability)))){ReasonNow(Reason::ActionInterrupted);return false;}
    motion_={};motion_.peek=kind;motion_.stage=MotionStage::Approach;motion_.start=f.space.coverStart;motion_.retreat=f.space.retreatPoint;
    motion_.direction=math::FlatUnit(math::Sub(f.space.peekOpen,f.space.coverStart));
    motion_.distance=(std::max)(0.0f,P(field(cfg::PeekField::DistanceCm))+rng_.Signed()*P(field(cfg::PeekField::DistanceErrorCm)));
    const auto fullDistance=motion_.distance+(kind==PeekKind::Poppin?P(Param::PoppinExtraDistanceCm):0);
    if(fullDistance>available){motion_={};ReasonNow(Reason::ClearanceUnavailable);return false;}
    motion_.goal=math::Add(motion_.start,math::Mul(motion_.direction,motion_.distance));
    motion_.startedAt=motion_.stageAt=motion_.lastProgressAt=f.self.nowSeconds;motion_.lastFeet=f.self.feet;
    motion_.shotsAtPhase=static_cast<uint32_t>(snapshot_.actualShots);
    if(kind==PeekKind::InfoRepeek){
        motion_.informationProofOpen=f.space.peekOpen;activity_=Activity::Information;
        if(target){motion_.informationTrack=target->id;motion_.informationFocus=target->point;target->informationHandledAt=target->lastSeen;}
        if(const auto*angle=InformationAngleReady(f)){
            motion_.informationAngle=angle->id;if(!target)motion_.informationFocus=angle->focusPoint;
            auto*record=&informationAngles_[0];
            for(auto&old:informationAngles_){if(old.id==angle->id||!old.id){record=&old;break;}if(old.checkedAt<record->checkedAt)record=&old;}
            *record={angle->id,f.self.nowSeconds};
        }
    }
    return true;
}
inline void Brain::PickDuel(const SensorFrame& f,Track& target){
    Scope(Domain::Motion,true);
    if(!f.self.groundedKnown){ReasonNow(Reason::GroundingUnknown);return;}
    if(!f.self.grounded){ReasonNow(Reason::NativeActionUnavailable);return;}
    struct Choice {DuelStyle kind;Param enabled,weight,any,all,confidence,risk,error;};
    constexpr std::array<Choice,4> choices{{
        {DuelStyle::Strafe,Param::StrafeEnabled,Param::StrafeWeight,Param::StrafeConditionAny,Param::StrafeConditionAll,Param::StrafeMinimumConfidence,Param::StrafeMaximumRisk,Param::StrafeExecutionErrorProbability},
        {DuelStyle::DotCrouch,Param::DotCrouchEnabled,Param::DotCrouchWeight,Param::DotCrouchConditionAny,Param::DotCrouchConditionAll,Param::DotCrouchMinimumConfidence,Param::DotCrouchMaximumRisk,Param::DotCrouchExecutionErrorProbability},
        {DuelStyle::Guangxi,Param::GuangxiEnabled,Param::GuangxiWeight,Param::GuangxiConditionAny,Param::GuangxiConditionAll,Param::GuangxiMinimumConfidence,Param::GuangxiMaximumRisk,Param::GuangxiExecutionErrorProbability},
        {DuelStyle::Deadzone,Param::DeadzoneEnabled,Param::DeadzoneWeight,Param::DeadzoneConditionAny,Param::DeadzoneConditionAll,Param::DeadzoneMinimumConfidence,Param::DeadzoneMaximumRisk,Param::DeadzoneMissProbability}
    }};
    std::array<float,4> weights{};float total=0;
    for(size_t n=0;n<choices.size();++n){const auto& c=choices[n];
        if(!Enabled(c.enabled)||target.confidence<P(c.confidence)||f.space.exposureRisk>P(c.risk)
           ||!ActionConditions(P(c.any),P(c.all),f,&target))continue;
        if(c.kind==DuelStyle::DotCrouch){if(!f.self.stanceValid||f.self.nowSeconds<nextCrouchAllowedAt_)continue;}
        else if(!f.self.movementValid||!f.space.clearanceKnown||!f.space.groundProven)continue;
        if(c.kind==DuelStyle::Guangxi&&!f.self.stanceValid){ReasonNow(Reason::CrouchNotObserved);continue;}
        if(c.kind==DuelStyle::Deadzone&&(!f.self.weapon.recoveryKnown||!f.self.movementValid)){ReasonNow(Reason::WeaponRecoveryUnknown);continue;}
        if(rng_.Chance(P(c.error))||!rng_.Chance(Inclination(c.weight)))continue;
        weights[n]=P(c.weight);total+=weights[n];
    }
    if(total<=0)return;float draw=rng_.Unit()*total;size_t selected=choices.size();
    for(size_t n=0;n<weights.size();++n){if(weights[n]<=0)continue;draw-=weights[n];if(draw<=0){selected=n;break;}}
    if(selected==choices.size())return;
    motion_={};motion_.duel=choices[selected].kind;
    motion_.sign=rng_.Chance(P(Param::StrafeRepeatDirectionProbability))?previousMoveSign_:rng_.Chance(P(Param::StrafeDirectionBias))?1:-1;
    previousMoveSign_=motion_.sign;
    const auto toward=math::FlatUnit(math::Sub(target.point,f.self.eye));motion_.direction={-toward.y*motion_.sign,toward.x*motion_.sign,0};
    motion_.start=f.self.feet;motion_.startedAt=motion_.stageAt=motion_.lastProgressAt=f.self.nowSeconds;motion_.lastFeet=f.self.feet;
    motion_.shotsAtPhase=static_cast<uint32_t>(snapshot_.actualShots);
    if(motion_.duel==DuelStyle::DotCrouch){motion_.stage=MotionStage::Engage;return;}
    const bool guangxi=motion_.duel==DuelStyle::Guangxi;
    motion_.distance=(std::max)(0.0f,P(guangxi?Param::GuangxiStepCm:Param::StrafeStepCm)
        +rng_.Signed()*P(guangxi?Param::GuangxiStepJitterCm:Param::StrafeStepJitterCm));
    if(guangxi)motion_.distance*=1+rng_.Signed()*P(Param::GuangxiVariation);
    const float clearance=motion_.sign>0?f.space.rightClearanceCm:f.space.leftClearanceCm;
    if(motion_.distance>clearance){motion_={};ReasonNow(Reason::ClearanceUnavailable);return;}
    motion_.goal=math::Add(f.self.feet,math::Mul(motion_.direction,motion_.distance));motion_.stage=MotionStage::Out;
    if(guangxi)motion_.comboLeft=rng_.Inclusive(static_cast<uint32_t>(P(Param::GuangxiComboMin)),static_cast<uint32_t>(P(Param::GuangxiComboMax)));
    if(motion_.duel==DuelStyle::Strafe)motion_.goalShots=rng_.Inclusive(static_cast<uint32_t>(P(Param::StrafeShotsMin)),static_cast<uint32_t>(P(Param::StrafeShotsMax)));
    if(motion_.duel==DuelStyle::Deadzone)motion_.goalShots=rng_.Inclusive(static_cast<uint32_t>(P(Param::DeadzoneShotsMin)),static_cast<uint32_t>(P(Param::DeadzoneShotsMax)));
}
inline void Brain::MoveTo(const OwnSnapshot& own,Vec3 point,Intent& intent,bool walk){
    if(!own.movementValid||!math::Finite(point)){ReasonNow(Reason::MovementUnknown);return;}
    const auto delta=math::Sub(point,own.feet);
    if(math::Horizontal(delta)>P(Param::MoveArrivalCm)){intent.worldDirection=math::FlatUnit(delta);intent.walk=walk;}
}
inline void Brain::MotionTick(const SensorFrame& f,Track* target,Intent& intent){
    Scope(Domain::Motion);
    const auto now=f.self.nowSeconds;
    const bool advancing=!target&&motion_.stage==MotionStage::Idle
        &&activity_!=Activity::Retreat&&activity_!=Activity::Reload
        &&(navigationEstimate_.requested||activity_==Activity::Relocate||activity_==Activity::Roam||activity_==Activity::Search);
    if(!advancing)advanceGait_={};
    if((f.self.collisionKnown&&f.self.collision)||(f.self.stalledKnown&&f.self.stalled)){
        CancelEstimatedNavigation();motion_={};fire_.window=false;fire_.holding=false;releasePending_=true;ReasonNow(f.self.collision?Reason::Collision:Reason::Stalled);return;
    }
    if(motion_.peek==PeekKind::InfoRepeek&&activity_==Activity::Information){
        const auto*source=Find(motion_.informationTrack);
        if(motion_.informationTrack&&(!source||source->lifeRetired||source->retired)){
            motion_={};fire_.window=fire_.holding=false;releasePending_=true;ReasonNow(Reason::TargetRetired);return;
        }
        const auto same=[](Vec3 a,Vec3 b){return a.x==b.x&&a.y==b.y&&a.z==b.z;};
        if(!f.self.movementValid||!f.self.groundedKnown||!f.self.grounded||!f.space.groundProven||!f.space.peekPathProven||!f.space.retreatPathProven
           ||!same(f.space.coverStart,motion_.start)||!same(f.space.retreatPoint,motion_.retreat)||!same(f.space.peekOpen,motion_.informationProofOpen)){
            motion_={};fire_.window=fire_.holding=false;releasePending_=true;
            ReasonNow(!f.self.movementValid?Reason::MovementUnknown:f.self.groundedKnown?Reason::CoverUnavailable:Reason::GroundingUnknown);return;
        }
        if(source&&!InformationMemoryReady(f,*source,false)){
            motion_.informationObservedAt=-1;
            if(motion_.stage!=MotionStage::Return){motion_.stage=MotionStage::Return;motion_.stageAt=motion_.lastProgressAt=now;motion_.lastFeet=f.self.feet;}
            ReasonNow(Reason::MemoryExpired);
        }
        if(motion_.stage==MotionStage::Recover&&math::Horizontal(math::Sub(f.self.feet,motion_.retreat))>P(Param::MoveArrivalCm)){
            motion_.stage=MotionStage::Return;motion_.stageAt=motion_.lastProgressAt=now;motion_.lastFeet=f.self.feet;
        }
        if((motion_.stage==MotionStage::Return||motion_.stage==MotionStage::Recover)&&!f.space.coverKnown){
            if(now-motion_.lastProgressAt>Milliseconds(Param::MoveStallMs)){
                motion_={};fire_.window=fire_.holding=false;releasePending_=true;ReasonNow(Reason::Stalled);
            }else ReasonNow(Reason::CoverUnavailable);
            return; // wait for a current own-to-cover proof, never a stale triangle
        }
    }
    if(motion_.peek!=PeekKind::None&&activity_!=Activity::Hold&&activity_!=Activity::Engage&&activity_!=Activity::Information){
        motion_={};fire_.window=false;fire_.holding=false;releasePending_=true;ReasonNow(Reason::ActionInterrupted);return;
    }
    if(activity_==Activity::Respawn)return;
    if(navigationEstimate_.requested){
        intent.estimatedNavigation=navigationEstimate_;
        if(navigationEstimate_.purpose==NavigationPurpose::MapHold){
            if(!f.space.holdProofReady){ReasonNow(Reason::CoverUnavailable);return;}
            if(mapHold_.arrived)return;
            if(mapHold_.arrival.active){
                intent.navigationRequested=mapHold_.arrival.needPath;
                if(mapHold_.arrival.pulsing&&humanlike_bot_tactics_navigation::SameRequest(navigationEstimate_,f.space.estimatedNavigation)
                   &&f.space.navigationKnown&&f.space.pathProven&&f.space.groundProven&&!f.space.estimatedGoalReached){
                    MoveTo(f.self,f.space.nextPathPoint,intent,true);
                }
                return;
            }
        }
        const bool proofMatches=humanlike_bot_tactics_navigation::SameRequest(navigationEstimate_,f.space.estimatedNavigation)
            ||humanlike_bot_tactics_navigation::SoundLeaseExtension(f.space.estimatedNavigation,navigationEstimate_);
        if(!proofMatches
           ||f.space.estimatedGoalReached||f.space.estimatedGoalRejected
           ||!f.space.navigationKnown||!f.space.pathProven||!f.space.groundProven){ReasonNow(Reason::NavigationUnavailable);return;}
        intent.navigationRequested=true;
        const auto delta=math::Sub(f.space.nextPathPoint,f.self.feet);
        const auto step=(std::min)(P(Param::SearchStepCm),math::Horizontal(delta));
        MoveTo(f.self,math::Add(f.self.feet,math::Mul(math::FlatUnit(delta),step)),intent);
        if(advancing&&math::Horizontal(intent.worldDirection)>0)intent.walk=AdvanceWalk(now);return;
    }
    if(motion_.stage==MotionStage::Idle&&activity_==Activity::Hold&&!target&&f.self.groundedKnown&&f.self.grounded
       &&f.space.coverKnown&&f.space.retreatPathProven&&f.space.groundProven
       &&math::Horizontal(math::Sub(f.self.feet,f.space.coverStart))>P(Param::HoldCoverDistanceCm)){
        MoveTo(f.self,f.space.coverStart,intent);return;
    }
    if(activity_==Activity::Retreat||activity_==Activity::Reload){
        if(f.space.coverKnown&&f.space.retreatPathProven&&f.space.groundProven){MoveTo(f.self,f.space.retreatPoint,intent);return;}
        if(activity_==Activity::Retreat)ReasonNow(Reason::RetreatUnavailable);return;
    }
    if(activity_==Activity::Relocate||activity_==Activity::Roam||activity_==Activity::Search){
        if(f.space.estimatedNavigation.requested){ReasonNow(Reason::NavigationUnavailable);return;}
        if(activity_==Activity::Relocate&&relocationGoalKnown_&&f.space.routeId!=relocationRouteId_){
            relocationGoalKnown_=false;activity_=Activity::Search;releasePending_=true;ReasonNow(Reason::NavigationUnavailable);return;
        }
        if(!f.space.navigationKnown||!f.space.pathProven||!f.space.groundProven){ReasonNow(Reason::NavigationUnavailable);return;}
        if(activity_==Activity::Search&&now<searchUntil_)return;
        intent.navigationRequested=true;
        const auto delta=math::Sub(f.space.nextPathPoint,f.self.feet);
        const auto step=(std::min)(P(Param::SearchStepCm),math::Horizontal(delta));
        MoveTo(f.self,math::Add(f.self.feet,math::Mul(math::FlatUnit(delta),step)),intent);
        if(advancing&&math::Horizontal(intent.worldDirection)>0)intent.walk=AdvanceWalk(now);
        return;
    }
    if(!target&&motion_.stage==MotionStage::Idle){
        if(now<nextMotionChoiceAt_)return;nextMotionChoiceAt_=now+Milliseconds(Param::ActionSwitchMs);
        if(activity_==Activity::Hold||activity_==Activity::Information)PickPeek(f,nullptr);
        if(motion_.stage==MotionStage::Idle)return;
    }
    if(motion_.stage==MotionStage::Idle){
        if(now<nextMotionChoiceAt_)return;nextMotionChoiceAt_=now+Milliseconds(Param::ActionSwitchMs);
        if(activity_==Activity::Hold&&now-target->lastSeen>Milliseconds(Param::MemoryLostHoldMs))return;
        if(!PickPeek(f,target)&&target->visible)PickDuel(f,*target);
    }
    if(motion_.stage==MotionStage::Idle)return;
    if(target&&!target->visible&&now-target->lastSeen>Milliseconds(Param::MemoryLostHoldMs)){
        if(motion_.peek!=PeekKind::None){
            if(motion_.stage!=MotionStage::Return){motion_.stage=MotionStage::Return;motion_.stageAt=now;}
        }
        else {motion_={};ReasonNow(Reason::TargetLost);return;}
    }
    if(motion_.peek==PeekKind::Jump){
        const auto abort=[&](Reason reason){motion_={};fire_.window=false;fire_.holding=false;releasePending_=true;ReasonNow(reason);};
        if(motion_.jumpRequested&&!motion_.landed&&now-motion_.jumpAt>Milliseconds(Param::JumpLandingTimeoutMs)){
            abort(Reason::JumpLandingUnproven);return;
        }
        if(motion_.stage==MotionStage::Return&&!motion_.airSeen){abort(motion_.jumpRequested?Reason::JumpNotObserved:Reason::ActionInterrupted);return;}
        if(!f.self.groundedKnown){ReasonNow(Reason::GroundingUnknown);return;}
        if(!motion_.jumpRequested&&!f.self.grounded){abort(Reason::NativeActionUnavailable);return;}
        if(motion_.jumpRequested){
            if(!f.self.grounded&&f.self.sampledAtSeconds>motion_.jumpAt){
                motion_.airSeen=true;motion_.landed=false;motion_.airSampleAt=(std::max)(motion_.airSampleAt,f.self.sampledAtSeconds);
            }
            if(!motion_.landed&&motion_.airSeen&&f.self.grounded&&f.self.sampledAtSeconds>motion_.airSampleAt){
                motion_.landed=true;motion_.lastProgressAt=now;motion_.lastFeet=f.self.feet;
                if(motion_.stage!=MotionStage::Return){motion_.stage=MotionStage::Return;motion_.stageAt=now;}
            }
            if(!motion_.airSeen&&now-motion_.jumpAt>Milliseconds(Param::ActionObserveTimeoutMs)){abort(Reason::JumpNotObserved);return;}
        }
    }
    if(math::Horizontal(math::Sub(f.self.feet,motion_.lastFeet))>P(Param::MoveArrivalCm)){
        motion_.lastFeet=f.self.feet;motion_.lastProgressAt=now;
    }else if((motion_.stage==MotionStage::Out||motion_.stage==MotionStage::Return||motion_.stage==MotionStage::Approach||motion_.stage==MotionStage::Reverse)
              &&!(motion_.peek==PeekKind::Jump&&motion_.airSeen&&!motion_.landed)&&!motion_.arrival.active
              &&now-motion_.lastProgressAt>Milliseconds(Param::MoveStallMs)){
        motion_={};releasePending_=true;ReasonNow(Reason::Stalled);return;
    }
    if(motion_.peek!=PeekKind::None){
        if((motion_.stage==MotionStage::Out||motion_.stage==MotionStage::Brake||motion_.stage==MotionStage::Engage)
           &&((f.space.peekSightKnown&&f.space.peekSightClear)||(target&&target->visible)))motion_.informationSeen=true;
        if(motion_.peek==PeekKind::InfoRepeek&&!motion_.informationRepeek
           &&(motion_.stage==MotionStage::Out||motion_.stage==MotionStage::Brake)
           &&target&&(!motion_.informationTrack||target->id==motion_.informationTrack)&&target->visible&&target->lastSeen>motion_.startedAt){
            // Anonymous map/own-view exploration binds only a genuinely new
            // acquired contact. Static angle knowledge never creates a Track.
            motion_.informationTrack=target->id;motion_.informationObservedAt=target->lastSeen;
        }
        const auto field=[&](cfg::PeekField p){return PeekParameter(motion_.peek,static_cast<unsigned>(p));};
        if(motion_.peek==PeekKind::Crouch&&(motion_.stage==MotionStage::Out||motion_.stage==MotionStage::Brake||motion_.stage==MotionStage::Engage)){
            intent.crouch=true;if(motion_.crouchAt<0)motion_.crouchAt=now;
            if(f.self.stanceValid&&f.self.crouched)motion_.crouchSeen=true;
            if(!motion_.crouchSeen&&now-motion_.crouchAt>Milliseconds(Param::ActionObserveTimeoutMs)){
                intent.crouch=false;motion_.stage=MotionStage::Return;motion_.stageAt=now;releasePending_=true;ReasonNow(Reason::CrouchNotObserved);return;
            }
        }
        if(motion_.stage==MotionStage::Approach){
            MoveTo(f.self,motion_.start,intent);
            if(math::Horizontal(math::Sub(f.self.feet,motion_.start))<=P(Param::MoveArrivalCm)){motion_.stage=MotionStage::Preaim;motion_.stageAt=now;}
            return;
        }
        if(motion_.stage==MotionStage::Preaim){
            const bool coordinated=!target||math::Angle(f.self.view,math::Look(f.self.eye,target->point))<=P(Param::FireAlignmentToleranceDeg);
            if(now>=motion_.stageAt+Milliseconds(field(cfg::PeekField::PrepareMs))&&(coordinated||!rng_.Chance(P(Param::CoordinationPrecision)))){motion_.stage=MotionStage::Out;motion_.stageAt=now;motion_.lastProgressAt=now;}
            return;
        }
        if(motion_.peek==PeekKind::Jump&&(motion_.stage==MotionStage::Out||motion_.stage==MotionStage::Brake)){
            if(!motion_.jumpRequested&&now>=motion_.stageAt+Milliseconds(Param::JumpTakeoffDelayMs)){
                if(!f.self.grounded){motion_={};releasePending_=true;ReasonNow(Reason::NativeActionUnavailable);return;}
                motion_.jumpRequested=true;motion_.jumpAt=now;
            }
            intent.jump=motion_.jumpRequested&&!motion_.airSeen&&now-motion_.jumpAt<=Milliseconds(Param::JumpMaxJumpHoldMs);
        }
        if(motion_.stage==MotionStage::Out){
            MoveTo(f.self,motion_.goal,intent,motion_.peek==PeekKind::SlowCreep);
            if(motion_.peek==PeekKind::Crouch)intent.crouch=true;
            if(motion_.peek==PeekKind::Jump&&motion_.airSeen){
                intent.worldDirection=math::FlatUnit(math::Add(intent.worldDirection,math::Mul(motion_.direction,P(Param::JumpAirDirectionBias))));
            }
            if(motion_.peek==PeekKind::Poppin){
                unsigned threats=0;for(const auto& t:tracks_)threats+=t.used&&!t.retired&&t.visible;
                if(threats>P(Param::PoppinCancelThreatCount)){motion_.stage=MotionStage::Return;motion_.stageAt=now;intent.worldDirection={};releasePending_=true;ReasonNow(Reason::MultipleThreats);return;}
            }
            if(math::Horizontal(math::Sub(f.self.feet,motion_.goal))<=P(Param::MoveArrivalCm)){
                if(motion_.peek==PeekKind::Poppin&&!motion_.poppinExtended){
                    intent.worldDirection={};
                    if(target&&target->visible&&target->firing&&target->firingAt>=0&&target->confidence>=P(Param::PoppinObservedFireConfidence)
                       &&now>=target->firingAt+Milliseconds(Param::PoppinObservedFireDecisionMs)){
                        if(!rng_.Chance(P(Param::PoppinExtendSameDirectionProbability))){motion_.stage=MotionStage::Return;motion_.stageAt=now;ReasonNow(Reason::ActionInterrupted);return;}
                        motion_.poppinExtended=true;motion_.distance+=P(Param::PoppinExtraDistanceCm);motion_.goal=math::Add(motion_.start,math::Mul(motion_.direction,motion_.distance));motion_.lastProgressAt=now;return;
                    }
                    if(now-motion_.stageAt<Milliseconds(Param::PoppinMaxExposureMs)){ReasonNow(Reason::PoppinEvidenceMissing);return;}
                    motion_.stage=MotionStage::Return;motion_.stageAt=now;ReasonNow(Reason::PoppinEvidenceMissing);return;
                }
                motion_.stage=MotionStage::Brake;motion_.stageAt=now;intent.worldDirection={};
            }
            const auto maxExposure=motion_.peek==PeekKind::Poppin?Milliseconds(Param::PoppinMaxExposureMs):Milliseconds(field(cfg::PeekField::ExposureMs));
            if(maxExposure>0&&now-motion_.stageAt>maxExposure){motion_.stage=MotionStage::Return;motion_.stageAt=now;}
            return;
        }
        if(motion_.stage==MotionStage::Brake){
            if(motion_.peek==PeekKind::Jump&&!motion_.landed)return; // 2D arrival/low horizontal speed is not landing proof
            if(motion_.peek==PeekKind::Crouch)intent.crouch=true;
            const auto delay=motion_.peek==PeekKind::Poppin?Milliseconds(Param::PoppinBrakeMs):Milliseconds(field(cfg::PeekField::DecisionMs));
            if(f.self.movementValid&&math::Horizontal(f.self.velocity)<=Tune(f.self,cfg::WeaponTuning::AccurateSpeedCmS)&&now-motion_.stageAt>=delay){
                if(motion_.peek==PeekKind::InfoRepeek){
                    // The observation leg always returns before any duel.
                    // Only the later re-peek can engage a currently perceived source.
                    motion_.returnChosen=!motion_.informationRepeek||!target||!target->visible||target->id!=motion_.informationTrack;
                }else{
                    const auto engage=P(field(cfg::PeekField::EngageWeight)),back=P(field(cfg::PeekField::ReturnWeight));
                    motion_.returnChosen=motion_.peek==PeekKind::Shoulder||motion_.peek==PeekKind::Jump||engage+back<=0||rng_.Unit()*(engage+back)>=engage;
                }
                motion_.stage=motion_.returnChosen?MotionStage::Return:MotionStage::Engage;motion_.stageAt=now;
                if(motion_.peek==PeekKind::InfoRepeek&&motion_.returnChosen){motion_.lastProgressAt=now;motion_.lastFeet=f.self.feet;}
            }
            return;
        }
        if(motion_.stage==MotionStage::Engage){
            if(motion_.peek==PeekKind::Crouch)intent.crouch=true;
            if(now-motion_.stageAt>=Milliseconds(field(cfg::PeekField::ExposureMs))){
                motion_.stage=MotionStage::Return;motion_.stageAt=now;
                if(motion_.peek==PeekKind::InfoRepeek){motion_.lastProgressAt=now;motion_.lastFeet=f.self.feet;}
            }
            return;
        }
        if(motion_.peek==PeekKind::InfoRepeek&&motion_.stage==MotionStage::Recover){
            // An arrival sample is not proof that original velocity settled.
            // A later drift must reacquire this same locked retreat first.
            if(math::Horizontal(math::Sub(f.self.feet,motion_.retreat))>P(Param::MoveArrivalCm)){
                motion_.stage=MotionStage::Return;motion_.stageAt=motion_.lastProgressAt=now;motion_.lastFeet=f.self.feet;return;
            }
            if(math::Horizontal(f.self.velocity)>Tune(f.self,cfg::WeaponTuning::AccurateSpeedCmS)){
                motion_.stageAt=now;
                if(now-motion_.lastProgressAt>Milliseconds(Param::MoveStallMs)){
                    motion_={};fire_.window=fire_.holding=false;releasePending_=true;ReasonNow(Reason::Stalled);
                }return;
            }
            if(now-motion_.stageAt<Milliseconds(field(cfg::PeekField::DecisionMs)))return;
            auto*source=Find(motion_.informationTrack);
            const bool fresh=source&&motion_.informationObservedAt>motion_.startedAt&&source->lastSeen>=motion_.informationObservedAt
                &&InformationMemoryReady(f,*source,false)&&f.space.coverKnown&&f.space.clearanceKnown;
            if(source)source->informationHandledAt=(std::max)(source->informationHandledAt,source->lastSeen);
            const auto engage=P(field(cfg::PeekField::EngageWeight)),back=P(field(cfg::PeekField::ReturnWeight));
            if(!fresh||engage+back<=0||rng_.Unit()*(engage+back)>=engage){
                ReasonNow(fresh?Reason::PeekPurposeComplete:Reason::MemoryExpired);motion_={};fire_.holding=fire_.window=false;activity_=Activity::Hold;return;
            }
            motion_.informationRepeek=true;motion_.returnChosen=false;motion_.stage=MotionStage::Approach;
            motion_.stageAt=motion_.lastProgressAt=now;motion_.lastFeet=f.self.feet;return;
        }
        if(motion_.stage==MotionStage::Return){
            if(now-motion_.stageAt<Milliseconds(field(cfg::PeekField::ReturnDelayMs)))return;
            if(motion_.peek!=PeekKind::InfoRepeek){
                const bool at=humanlike_bot_hold_positions::Arrived(f.self,motion_.retreat,P(Param::MoveArrivalCm))
                    &&(motion_.peek!=PeekKind::Jump||(motion_.airSeen&&motion_.landed));
                if(at&&!motion_.arrival.active&&!motion_.arrival.done)BeginArrival(motion_.arrival,f.self);
                if(motion_.arrival.active){
                    const auto&point=f.space.retreatPoint;
                    const bool correction=f.space.coverKnown&&f.space.retreatPathProven&&f.space.groundProven
                        &&point.x==motion_.retreat.x&&point.y==motion_.retreat.y&&point.z==motion_.retreat.z;
                    ArrivalTick(motion_.arrival,f.self,motion_.retreat,correction);
                    if(motion_.arrival.failed){motion_={};fire_.holding=fire_.window=false;releasePending_=true;ReasonNow(Reason::Stalled);return;}
                    if(motion_.arrival.pulsing&&correction)MoveTo(f.self,motion_.retreat,intent,true);
                }else if(!motion_.arrival.done)MoveTo(f.self,motion_.retreat,intent);
                if(!motion_.arrival.done)return;
                ReasonNow(motion_.informationSeen?Reason::PeekPurposeComplete:Reason::VisualUnknown);
                motion_={};fire_.holding=fire_.window=false;return;
            }
            MoveTo(f.self,motion_.retreat,intent);
            if(math::Horizontal(math::Sub(f.self.feet,motion_.retreat))<=P(Param::MoveArrivalCm)
               &&(motion_.peek!=PeekKind::Jump||(motion_.airSeen&&motion_.landed&&f.self.groundedKnown&&f.self.grounded))){
                if(motion_.peek==PeekKind::InfoRepeek&&math::Horizontal(f.self.velocity)>Tune(f.self,cfg::WeaponTuning::AccurateSpeedCmS))return;
                if(motion_.peek==PeekKind::InfoRepeek&&!motion_.informationRepeek){
                    const auto*source=Find(motion_.informationTrack);
                    if(!motion_.informationTrack||(source&&InformationMemoryReady(f,*source,false))){
                        motion_.stage=MotionStage::Recover;motion_.stageAt=now;return;
                    }
                }
                if(motion_.peek==PeekKind::InfoRepeek)if(auto*source=Find(motion_.informationTrack))
                    source->informationHandledAt=(std::max)(source->informationHandledAt,source->lastSeen);
                ReasonNow(motion_.informationSeen?Reason::PeekPurposeComplete:Reason::VisualUnknown);motion_={};fire_.holding=false;fire_.window=false;
            }
            return;
        }
    }
    if(motion_.duel==DuelStyle::DotCrouch){
        const auto phase=static_cast<unsigned>(P(Param::DotCrouchTriggerPhase));
        const bool afterShot=fire_.lastActualAt>=0&&now>=fire_.lastActualAt+Milliseconds(Param::DotCrouchAfterShotDelayMs);
        const bool trigger=phase==3||(phase==2&&afterShot)||(phase==1&&aim_.stage==AimStage::Track)||(phase==0&&f.self.movementValid&&math::Horizontal(f.self.velocity)<=Tune(f.self,cfg::WeaponTuning::AccurateSpeedCmS));
        if(!motion_.crouchRequested&&trigger){motion_.crouchRequested=true;motion_.stageAt=now;motion_.crouchUntil=now+Milliseconds(Param::DotCrouchHoldMs);}
        if(motion_.crouchRequested){
            intent.crouch=now<motion_.crouchUntil;
            if(f.self.stanceValid&&f.self.crouched)motion_.crouchSeen=true;
            if(!motion_.crouchSeen&&now-motion_.stageAt>Milliseconds(Param::ActionObserveTimeoutMs)){motion_={};releasePending_=true;ReasonNow(Reason::CrouchNotObserved);return;}
            if(motion_.crouchSeen&&now>=motion_.crouchUntil+Milliseconds(Param::DotCrouchStandDelayMs)&&f.self.stanceValid&&!f.self.crouched){
                nextCrouchAllowedAt_=now+Milliseconds(Param::DotCrouchCooldownMs);motion_={};
            }
        }
        return;
    }
    if(motion_.duel==DuelStyle::Strafe){
        if(motion_.stage==MotionStage::Out){MoveTo(f.self,motion_.goal,intent);if(math::Horizontal(math::Sub(f.self.feet,motion_.goal))<=P(Param::MoveArrivalCm)){motion_.stage=MotionStage::Brake;motion_.stageAt=now;}}
        else if(motion_.stage==MotionStage::Brake&&now-motion_.stageAt>=Milliseconds(Param::StrafeBrakeMs)+Milliseconds(Param::StrafeFireDelayMs))motion_.stage=MotionStage::Engage;
        else if(motion_.stage==MotionStage::Engage&&snapshot_.actualShots-motion_.shotsAtPhase>=motion_.goalShots){motion_.stage=MotionStage::Recover;motion_.stageAt=now;}
        else if(motion_.stage==MotionStage::Recover&&now-motion_.stageAt>=Milliseconds(Param::StrafeResumeMs))motion_={};
    }else if(motion_.duel==DuelStyle::Guangxi){
        const auto traveled=math::Horizontal(math::Sub(f.self.feet,motion_.start));const auto fraction=motion_.distance>0?traveled/motion_.distance:1;
        if(fraction>=P(Param::GuangxiCrouchPhase)&&!motion_.crouchRequested){motion_.crouchRequested=true;motion_.crouchUntil=now+Milliseconds(Param::GuangxiCrouchHoldMs);}
        intent.crouch=motion_.crouchRequested&&now<motion_.crouchUntil;
        if(f.self.stanceValid&&f.self.crouched)motion_.crouchSeen=true;
        if(motion_.crouchRequested&&!motion_.crouchSeen&&now>motion_.crouchUntil+Milliseconds(Param::ActionObserveTimeoutMs)){
            motion_={};intent.crouch=false;intent.worldDirection={};releasePending_=true;ReasonNow(Reason::CrouchNotObserved);return;
        }
        MoveTo(f.self,motion_.goal,intent);
        if(traveled>=motion_.distance||now-motion_.stageAt>=Milliseconds(Param::GuangxiReverseMs)){
            if(motion_.comboLeft>0)--motion_.comboLeft;
            if(!motion_.comboLeft){intent.worldDirection={};if(!intent.crouch&&now>=motion_.crouchUntil+Milliseconds(Param::GuangxiStandDelayMs))motion_={};return;}
            if(!f.space.clearanceKnown){motion_={};ReasonNow(Reason::ClearanceUnavailable);return;}
            const bool reverse=rng_.Chance(P(Param::GuangxiVariation));if(reverse){motion_.direction=math::Mul(motion_.direction,-1);motion_.sign=-motion_.sign;}
            const auto clearance=motion_.sign>0?f.space.rightClearanceCm:f.space.leftClearanceCm;
            if(motion_.distance>clearance){motion_={};ReasonNow(Reason::ClearanceUnavailable);return;}
            motion_.start=f.self.feet;motion_.goal=math::Add(f.self.feet,math::Mul(motion_.direction,motion_.distance));motion_.stageAt=now;motion_.crouchRequested=false;
        }
    }else if(motion_.duel==DuelStyle::Deadzone){
        if(motion_.stage==MotionStage::Out){MoveTo(f.self,motion_.goal,intent);
            if(now-motion_.stageAt>=Milliseconds(Param::DeadzoneReverseMs)){
                motion_.stage=MotionStage::Reverse;motion_.stageAt=now+rng_.Signed()*Milliseconds(Param::DeadzoneTimingErrorMs);
                motion_.direction=math::Mul(motion_.direction,-1);motion_.goal=math::Add(f.self.feet,math::Mul(motion_.direction,P(Param::DeadzonePostReverseCm)));
            }
        }else if(motion_.stage==MotionStage::Reverse){
            MoveTo(f.self,motion_.goal,intent);
            if(now-motion_.stageAt>Milliseconds(Param::DeadzoneMaxWindowMs)||snapshot_.actualShots-motion_.shotsAtPhase>=motion_.goalShots){motion_={};fire_.holding=false;fire_.window=false;}
        }
    }
}
inline bool Brain::WeaponFireReady(const SensorFrame& f,float distance,FireMode mode,Reason& reason){
    Scope(Domain::Fire);const auto& self=f.self;const auto& w=self.weapon;
    const auto reject=[&](Reason value){reason=value;return false;};
    if(!w.valid||!w.binding||!cfg::ValidWeapon(static_cast<cfg::Weapon>(w.catalogWeaponIndex)))return reject(Reason::WeaponUnknown);
    if(!math::Finite(w.sampledAtSeconds)||w.sampledAtSeconds>self.nowSeconds||self.nowSeconds-w.sampledAtSeconds>Milliseconds(Param::WeaponSnapshotMaxAgeMs))return reject(Reason::WeaponUnknown);
    if(!w.ammoKnown||w.magazine<0||w.magazineMaximum<=0||w.magazine>w.magazineMaximum)return reject(Reason::AmmoUnknown);
    if(w.magazine==0)return reject(Reason::EmptyMagazine);
    // Original CanFire=true permits submitting Primary without claiming that
    // the separate, incompletely observed reload state is known to be false.
    if(!w.reloadKnown&&(!w.canFireKnown||!w.canFire))return reject(Reason::ReloadUnknown);
    if(w.reloading||fire_.reloadRequested||activity_==Activity::Reload)return reject(Reason::ReloadPending);
    if(!w.fireBehaviorKnown)return reject(Reason::NativeActionUnavailable);
    if(!self.movementValid||!math::Finite(self.velocity))return reject(Reason::MovementUnknown);
    if(!w.recoveryKnown||!math::Finite(w.firingErrorDegrees)||!math::Finite(w.movementErrorDegrees)||w.firingErrorDegrees<0||w.movementErrorDegrees<0)return reject(Reason::WeaponRecoveryUnknown);
    const auto speed=math::Horizontal(self.velocity);
    const bool continuing=fire_.window&&fire_.holding&&fire_.mode==mode;
    if(!continuing&&lastNeutralAt_>=0&&(!w.triggerHeldKnown||w.triggerHeld||w.sampledAtSeconds<=lastNeutralAt_))return reject(Reason::NativeReleaseUnproven);
    const bool moving=mode==FireMode::MovingClose;
    if(moving){
        if(Band(self,distance)!=cfg::Distance::Near||speed<P(Param::MovingCloseMinSpeedCmS))return reject(Reason::MovementTooFast);
    }else{
        auto maximum=(std::min)(P(Param::AccurateSpeedCmS),Tune(self,cfg::WeaponTuning::AccurateSpeedCmS));
        if(motion_.duel==DuelStyle::Deadzone)maximum=(std::min)(maximum,P(Param::DeadzoneMaxSpeedCmS));
        if(speed>maximum)return reject(Reason::MovementTooFast);
        if(!continuing){
            auto maximumError=(std::min)(P(Param::WeaponRecoveryErrorDeg),Tune(self,cfg::WeaponTuning::RecoveryErrorDeg));
            if(motion_.duel==DuelStyle::Deadzone)maximumError=(std::min)(maximumError,P(Param::DeadzoneMaxWeaponErrorDeg));
            if(w.firingErrorDegrees>maximumError||w.movementErrorDegrees>(std::min)(P(Param::WeaponMovementErrorDeg),Tune(self,cfg::WeaponTuning::MovementErrorDeg)))return reject(Reason::WeaponNotRecovered);
        }
    }
    if(!continuing&&(!w.canFireKnown||!w.canFire))return reject(Reason::NativeActionUnavailable);
    reason=Reason::None;return true;
}
inline bool Brain::MotionAimFireReady(const SensorFrame&f,Vec3 point,FireMode mode,Reason&reason){
    Scope(Domain::Fire);const auto&self=f.self;const bool moving=mode==FireMode::MovingClose;
    const auto reject=[&](Reason value){reason=value;return false;};
    if(motion_.peek!=PeekKind::None){
        if(motion_.peek==PeekKind::Shoulder||motion_.peek==PeekKind::Jump||motion_.stage!=MotionStage::Engage)return reject(Reason::TurnPending);
        if(motion_.peek==PeekKind::Crouch&&(!self.stanceValid||!self.crouched))return reject(Reason::CrouchNotObserved);
    }
    if((motion_.duel==DuelStyle::DotCrouch||motion_.duel==DuelStyle::Guangxi)&&motion_.crouchRequested
       &&f.self.nowSeconds<motion_.crouchUntil&&(!self.stanceValid||!self.crouched))return reject(Reason::CrouchNotObserved);
    if(motion_.duel==DuelStyle::Strafe&&motion_.stage!=MotionStage::Engage)return reject(Reason::MovementTooFast);
    if(motion_.duel==DuelStyle::Deadzone&&(motion_.stage!=MotionStage::Reverse||self.nowSeconds<motion_.stageAt))return reject(Reason::MovementTooFast);
    if(motion_.duel==DuelStyle::Guangxi){
        const auto progress=motion_.distance>0?math::Horizontal(math::Sub(self.feet,motion_.start))/motion_.distance:1;
        if(progress<P(Param::GuangxiFirePhase))return reject(Reason::TurnPending);
    }
    Rotation actual{};
    if(!ReadShootingView(self,actual))return reject(Reason::VisualUnknown);
    const auto error=math::Angle(actual,math::Look(self.eye,point));
    const auto tolerance=moving?P(Param::MovingFireToleranceDeg):P(Param::FireAlignmentToleranceDeg);
    if(error>tolerance){
        if(error>P(Param::FireUrgentToleranceDeg)||!rng_.Chance(P(Param::FireUrgentProbability)))return reject(Reason::AimOutsideTolerance);
    }
    reason=Reason::None;return true;
}
inline bool Brain::CanShoot(const SensorFrame& f,Track& target,FireMode mode,Reason& reason){
    const auto&self=f.self;const auto reject=[&](Reason value){reason=value;return false;};
    if(!WeaponFireReady(f,TargetDistance(self,target),mode,reason))return false;
    if(self.nowSeconds<aim_.startAt||aim_.stage==AimStage::React||aim_.stage==AimStage::Idle)return reject(Reason::ReactionPending);
    if(!aim_.historyReady)return reject(Reason::ObservationHistoryUnavailable);
    if(target.retired)return reject(Reason::TargetRetired);
    if(!target.visible&&!fire_.prefire&&(!fire_.window||self.nowSeconds-target.lastSeen>Milliseconds(Param::FireLostStopMs)))return reject(Reason::TargetLost);
    if(!target.visible&&target.confidence<P(Param::FirePrefireConfidence))return reject(Reason::TargetLost);
    return MotionAimFireReady(f,target.point,mode,reason);
}
inline void Brain::FireTick(const SensorFrame& f,Track* target,Intent& intent){
    Scope(Domain::Fire);const auto now=f.self.nowSeconds;const auto& w=f.self.weapon;
    const bool estimated=!target&&estimatedPrefire_.aim.requested;
    if(target&&estimatedPrefire_.aim.requested){StopEstimatedPrefire(now);return;}
    if(estimated){
        if(estimatedPrefire_.started&&!fire_.window){StopEstimatedPrefire(now);return;}
        if(humanlike_bot_prefire::Same(estimatedPrefire_.aim,f.space.estimatedAim)&&f.space.estimatedAimKnown&&!f.space.estimatedAimClear){StopEstimatedPrefire(now,Reason::VisualUnknown);return;}
        if(!estimatedPrefire_.sourceCurrent||!humanlike_bot_prefire::Valid(estimatedPrefire_.aim,f.self,now)
           ||(estimatedPrefire_.aim.source==EstimatedAimSource::Heard&&!f.hearingAvailable)){
            StopEstimatedPrefire(now,Reason::TargetLost);return;}
        intent.estimatedAim=estimatedPrefire_.aim;
    }
    if(!Enabled(Param::FireEnabled)||(!target&&!estimated)||activity_==Activity::Dead||activity_==Activity::Respawn
       ||activity_==Activity::Retreat||activity_==Activity::Reload||fire_.reloadRequested||w.reloading){StopEstimatedPrefire(now);fire_.holding=false;fire_.window=false;return;}
    if(fire_.binding&&fire_.binding!=w.binding){StopEstimatedPrefire(now);fire_={};ReasonNow(Reason::WeaponBindingChanged);return;}
    if(fire_.window&&fire_.precise&&(!w.shotFeedAvailable||!f.eventFeedValid)){
        fire_.window=false;fire_.holding=false;if(estimated){StopEstimatedPrefire(now);intent.estimatedAim={};}ReasonNow(Reason::ActualShotFeedUnavailable);return;
    }
    if(fire_.window&&fire_.precise&&fire_.count>=fire_.goal){
        fire_.holding=false;fire_.window=false;fire_.needRelease=true;fire_.releaseRequestedAt=now;fire_.lastReleaseAt=now;
        fire_.nextAt=now+(std::max)(Milliseconds(Param::FireRoundPauseMs),Tune(f.self,cfg::WeaponTuning::RoundPauseMs)*.001f);
        if(fire_.estimateKey){estimatedPrefire_={};intent.estimatedAim={};fire_.estimateKey=0;}
        ReasonNow(fire_.count>fire_.goal?Reason::NativeShotOvershoot:Reason::ShotWindowComplete);return;
    }
    if(fire_.window&&now-fire_.startedAt>Milliseconds(Param::FireSprayMaxHoldMs)){
        fire_.window=false;fire_.holding=false;fire_.needRelease=true;fire_.releaseRequestedAt=now;fire_.nextAt=now+Milliseconds(Param::FireResetWaitMs);
        if(estimated){StopEstimatedPrefire(now);intent.estimatedAim={};}ReasonNow(fire_.precise?Reason::ActualShotPending:Reason::ActualShotFeedUnavailable);return;
    }
    if(fire_.needRelease){
        fire_.holding=false;
        if(fire_.releaseRequestedAt<0)fire_.releaseRequestedAt=now;
        const bool nativeReleased=w.triggerHeldKnown&&!w.triggerHeld&&w.sampledAtSeconds>fire_.releaseRequestedAt;
        if(!nativeReleased){
            if(now-fire_.releaseRequestedAt>Milliseconds(Param::ActionObserveTimeoutMs)){fire_.window=false;ReasonNow(Reason::NativeReleaseUnproven);}
            else ReasonNow(Reason::TriggerRelease);
            return;
        }
        fire_.needRelease=false;fire_.releaseRequestedAt=-1;fire_.lastReleaseAt=now;
    }
    if(!estimated&&recentHit_&&now>=hitDecisionAt_){
        recentHit_=false;const auto keep=P(Param::HitContinueWeight)*Tune(f.self,cfg::WeaponTuning::HitContinueWeight),reset=P(Param::HitResetWeight)*Tune(f.self,cfg::WeaponTuning::HitResetWeight);
        if(keep+reset>0&&rng_.Unit()*(keep+reset)>=keep){fire_.window=false;fire_.holding=false;fire_.nextAt=now+Milliseconds(Param::FireResetWaitMs);ReasonNow(Reason::TriggerRelease);return;}
    }
    if(!estimated&&recentMiss_&&!motion_.firstMissHandled){
        recentMiss_=false;motion_.firstMissHandled=true;
        const std::array<float,4> weights{{P(Param::FirstMissContinueWeight),P(Param::FirstMissStrafeWeight),P(Param::FirstMissCrouchWeight),P(Param::FirstMissRetreatWeight)}};
        float total=0;for(auto weight:weights)total+=weight;
        float draw=rng_.Unit()*total;size_t selected=0;
        for(size_t n=0;n<weights.size();++n){if(weights[n]<=0)continue;draw-=weights[n];if(draw<=0){selected=n;break;}}
        if(total>0&&selected>0){
            fire_.window=false;fire_.holding=false;fire_.nextAt=now+Milliseconds(Param::FireResetWaitMs);
            if(selected==3){activity_=Activity::Retreat;motion_={};ReasonNow(f.space.retreatPathProven?Reason::ActionInterrupted:Reason::RetreatUnavailable);}
            else if(selected==2&&Enabled(Param::DotCrouchEnabled)&&f.self.stanceValid&&now>=nextCrouchAllowedAt_){motion_={};motion_.duel=DuelStyle::DotCrouch;motion_.stage=MotionStage::Engage;motion_.firstMissHandled=true;}
            else if(selected==1&&Enabled(Param::StrafeEnabled)){motion_={};PickDuel(f,*target);motion_.firstMissHandled=true;}
            ReasonNow(Reason::ActionInterrupted);return;
        }
    }
    if(fire_.window&&fire_.precise){
        const auto last=fire_.lastActualAt>=fire_.startedAt?fire_.lastActualAt:fire_.startedAt;
        if(now-last>Milliseconds(Param::FireShotReceiptTimeoutMs)){
            fire_.window=false;fire_.holding=false;fire_.needRelease=true;fire_.releaseRequestedAt=now;fire_.nextAt=now+Milliseconds(Param::FireResetWaitMs);if(estimated){StopEstimatedPrefire(now);intent.estimatedAim={};}ReasonNow(Reason::ActualShotPending);return;
        }
    }
    if(now<fire_.nextAt){ReasonNow(Reason::ReactionPending);return;}
    if(!fire_.window){
        const auto weapon=static_cast<cfg::Weapon>(w.catalogWeaponIndex);
        if(!w.valid||!cfg::ValidWeapon(weapon)){ReasonNow(Reason::WeaponUnknown);return;}
        const bool underFire=lastHurtAt_>=0&&now>=hurtNoticeAt_&&now-lastHurtAt_<=Milliseconds(Param::HurtMemoryMs);
        const auto band=Band(f.self,estimated?math::Distance(f.self.eye,estimatedPrefire_.aim.point):TargetDistance(f.self,*target));
        const auto context=underFire?cfg::FireContext::UnderFire:f.self.movementValid&&math::Horizontal(f.self.velocity)>P(Param::AccurateSpeedCmS)?cfg::FireContext::Moving
            :!estimated&&(target->exposure==Exposure::SampledPartial||target->exposure==Exposure::SampledHeadOnly)?cfg::FireContext::Partial:cfg::FireContext::Steady;
        std::array<float,cfg::FireModeCount> weights{};float total=0;
        for(size_t n=0;n<weights.size();++n){
            const auto mode=static_cast<FireMode>(n+1);auto weight=P(cfg::FireWeight(weapon,band,context,static_cast<cfg::FireMode>(n)));
            if(mode==FireMode::MovingClose&&(band!=cfg::Distance::Near||!f.self.movementValid||math::Horizontal(f.self.velocity)<P(Param::MovingCloseMinSpeedCmS)))weight=0;
            if((motion_.duel==DuelStyle::Strafe||motion_.duel==DuelStyle::Deadzone)&&motion_.goalShots){
                if((motion_.goalShots==1&&mode!=FireMode::Single)||(motion_.goalShots==2&&mode!=FireMode::Double)||(motion_.goalShots>2&&mode!=FireMode::Burst))weight=0;
            }
            if((!w.shotFeedAvailable||!f.eventFeedValid)&&(estimated||!(mode==FireMode::Spray&&Enabled(Param::FireAllowUncountedSpray))))weight=0;
            weights[n]=weight;total+=weight;
        }
        if(total<=0){ReasonNow(w.shotFeedAvailable?Reason::ParameterUnsupported:Reason::ActualShotFeedUnavailable);return;}
        float draw=rng_.Unit()*total;size_t selected=weights.size();
        for(size_t n=0;n<weights.size();++n){if(weights[n]<=0)continue;draw-=weights[n];if(draw<=0){selected=n;break;}}
        if(selected==weights.size()){ReasonNow(Reason::ParameterUnsupported);return;}
        const auto mode=static_cast<FireMode>(selected+1);uint32_t goal=0;
        if(mode==FireMode::Single)goal=static_cast<uint32_t>(P(Param::SingleShots));
        else if(mode==FireMode::Double)goal=static_cast<uint32_t>(P(Param::DoubleShots));
        else if(mode==FireMode::Burst){
            const auto lo=static_cast<uint32_t>((std::max)(P(Param::BurstMinShots),Tune(f.self,cfg::WeaponTuning::BurstMinShots)));
            const auto hi=static_cast<uint32_t>((std::min)(P(Param::BurstMaxShots),Tune(f.self,cfg::WeaponTuning::BurstMaxShots)));
            if(lo>hi){ReasonNow(Reason::ParameterUnsupported);return;}goal=rng_.Inclusive(lo,hi);
        }else if(mode==FireMode::Spray)goal=static_cast<uint32_t>((std::min)(P(Param::SprayMaxShots),Tune(f.self,cfg::WeaponTuning::SprayMaxShots)));
        else{
            const auto lo=static_cast<uint32_t>((std::max)(P(Param::MovingCloseMinShots),Tune(f.self,cfg::WeaponTuning::MovingMinShots)));
            const auto hi=static_cast<uint32_t>((std::min)(P(Param::MovingCloseMaxShots),Tune(f.self,cfg::WeaponTuning::MovingMaxShots)));
            if(lo>hi){ReasonNow(Reason::ParameterUnsupported);return;}goal=rng_.Inclusive(lo,hi);
        }
        if((motion_.duel==DuelStyle::Strafe||motion_.duel==DuelStyle::Deadzone)&&motion_.goalShots)goal=motion_.goalShots;
        if(w.nativeBurst&&(!w.nativeBurstSizeKnown||!w.nativeBurstSize||goal%w.nativeBurstSize)){ReasonNow(Reason::NativeBurstUnproven);return;}
        bool prefire=false;
        if(!estimated&&!target->visible){
            if(target->retired||target->confidence<P(Param::FirePrefireConfidence)||now-target->lastSeen>Milliseconds(Param::FirePrefireMaxAgeMs)||!rng_.Chance(P(Param::FirePrefireProbability))){ReasonNow(Reason::TargetLost);return;}
            prefire=true;
        }
        fire_.prefire=prefire;
        Reason reason=Reason::None;if(!(estimated?CanEstimatedShoot(f,mode,reason):CanShoot(f,*target,mode,reason))){ReasonNow(reason);return;}
        if(!fire_.hesitated&&rng_.Chance(P(Param::FireHesitateProbability))){fire_.hesitated=true;fire_.nextAt=now+Milliseconds(Param::FireHesitateMs);ReasonNow(Reason::ReactionPending);return;}
        if(!estimated&&fire_.lastReleaseAt<0&&now<aim_.startAt+Milliseconds(Param::FireFirstWaitMs)){ReasonNow(Reason::ReactionPending);return;}
        const auto nextAt=fire_.nextAt;const auto lastActual=fire_.lastActualAt;const auto lastRelease=fire_.lastReleaseAt;
        fire_={};fire_.window=true;fire_.mode=mode;fire_.binding=w.binding;fire_.goal=goal;fire_.precise=w.shotFeedAvailable&&f.eventFeedValid;fire_.prefire=prefire||estimated;
        if(estimated){fire_.estimateKey=estimatedPrefire_.aim.key;estimatedPrefire_.started=true;}
        fire_.startedAt=now;fire_.nextAt=nextAt;fire_.lastActualAt=lastActual;fire_.lastReleaseAt=lastRelease;
    }
    Reason reason=Reason::None;
    if(!(estimated?CanEstimatedShoot(f,fire_.mode,reason):CanShoot(f,*target,fire_.mode,reason))){fire_.holding=false;ReasonNow(reason);return;}
    fire_.holding=true;intent.fire=true;
}
inline void Brain::ReloadTick(const SensorFrame& f,Intent& intent){
    Scope(Domain::Fire);const auto now=f.self.nowSeconds;const auto& w=f.self.weapon;
    const auto cancel=[&](Reason reason){
        // Retire only our request. This is not proof the original FSM ended.
        fire_={};fire_.reloadBeforeAt=now;fire_.nextAt=now+Milliseconds(Param::FireResetWaitMs);
        activity_=Activity::Search;intent.reload=false;releasePending_=true;ReasonNow(reason);
    };
    const bool fresh=w.valid&&w.binding&&math::Finite(w.sampledAtSeconds)&&w.sampledAtSeconds<=now
        &&now-w.sampledAtSeconds<=Milliseconds(Param::WeaponSnapshotMaxAgeMs);
    const bool ammo=w.ammoKnown&&w.magazine>=0&&w.magazineMaximum>0&&w.magazine<=w.magazineMaximum;
    if(fire_.reloadRequested){
        fire_.window=false;fire_.holding=false;
        if(w.binding!=fire_.binding){cancel(Reason::WeaponBindingChanged);return;}
        if(!fresh){cancel(Reason::WeaponUnknown);return;}
        if(w.ammoKnown&&!ammo){cancel(Reason::AmmoUnknown);return;}
        // A cached pre-request positive is not a new execution receipt.
        if(w.reloadKnown&&w.reloading&&w.sampledAtSeconds>fire_.reloadAt
           &&w.sampledAtSeconds>fire_.reloadPositiveAt){fire_.reloadObserved=true;fire_.reloadPositiveAt=w.sampledAtSeconds;}
        if(fire_.reloadObserved){
            intent.reload=false;
            const bool later=w.sampledAtSeconds>fire_.reloadAt&&w.sampledAtSeconds>fire_.reloadPositiveAt;
            if(later&&ammo&&w.canFireKnown&&w.canFire){
                if(w.magazine>fire_.reloadBefore){
                    // Actual ammo growth + restored original Primary permission
                    // ends our wait; reloadKnown remains exactly as observed.
                    fire_.reloadRequested=fire_.reloadObserved=false;fire_.reloadBeforeAt=-1;fire_.reloadPositiveAt=-1;
                    fire_.reloadBefore=-1;fire_.needRelease=false;fire_.releaseRequestedAt=-1;activity_=Activity::Search;return;
                }
                cancel(Reason::ActionInterrupted);return; // permitted again, but no refill: not a completed reload
            }
            // Fresh positive original reload may legitimately outlast this
            // observation budget (e.g. a long native weapon animation). While
            // it remains proven, keep waiting with R released. Once positive
            // evidence stops, an unconfirmed ending has a bounded wait too.
            if(!(w.reloadKnown&&w.reloading)&&now-fire_.reloadPositiveAt>Milliseconds(Param::ReloadObserveTimeoutMs)){
                cancel(Reason::ReloadUnknown);return;
            }
            ReasonNow(Reason::ReloadPending);return;
        }
        if(now-fire_.reloadAt>Milliseconds(Param::ReloadObserveTimeoutMs)){cancel(Reason::ReloadNotObserved);return;}
        // An already requested attempt can temporarily lose ammo selection
        // while original reload is active. Do not read unknown placeholder
        // values; release R and await independent positive/native ammo facts.
        intent.reload=ammo;ReasonNow(ammo?Reason::ReloadPending:Reason::AmmoUnknown);return;
    }
    if(activity_!=Activity::Reload)return;
    if(!fresh||!cfg::ValidWeapon(static_cast<cfg::Weapon>(w.catalogWeaponIndex))){ReasonNow(Reason::WeaponUnknown);return;}
    if(!ammo){ReasonNow(Reason::AmmoUnknown);return;}
    if(w.magazine>=w.magazineMaximum){activity_=Activity::Search;fire_.reloadBeforeAt=-1;return;}
    if(!w.reserveKnown||w.reserve<=0){ReasonNow(Reason::EmptyMagazine);return;}
    if(static_cast<float>(w.magazine)/static_cast<float>(w.magazineMaximum)>P(Param::ReloadMagazineThreshold)){
        activity_=Activity::Search;fire_.reloadBeforeAt=-1;return;
    }
    if(fire_.reloadBeforeAt<0)fire_.reloadBeforeAt=now;
    if(now-fire_.reloadBeforeAt<Milliseconds(Param::ReloadDecisionMs)){ReasonNow(Reason::ReactionPending);return;}
    if(f.space.coverKnown&&f.space.retreatPathProven&&rng_.Chance(Inclination(Param::ReloadCoverWeight))
       &&math::Horizontal(math::Sub(f.self.feet,f.space.retreatPoint))>P(Param::MoveArrivalCm)){ReasonNow(Reason::RetreatUnavailable);return;}
    fire_.window=false;fire_.holding=false;fire_.mode=FireMode::None;fire_.count=fire_.goal=0;
    fire_.reloadRequested=true;fire_.reloadObserved=w.reloadKnown&&w.reloading;
    fire_.binding=w.binding;fire_.reloadAt=now;fire_.reloadBefore=w.magazine;
    fire_.reloadPositiveAt=fire_.reloadObserved?w.sampledAtSeconds:-1;
    // Trying original Reload is not a CanReload/reloading execution proof.
    intent.reload=!fire_.reloadObserved;ReasonNow(Reason::ReloadPending);
}
inline void Brain::Note(const SensorFrame& f,const Intent& intent,Reason reason){
    snapshot_.life=life_;snapshot_.activity=activity_;snapshot_.aim=aim_.stage;snapshot_.target=aim_.target;
    snapshot_.peek=motion_.peek;snapshot_.motion=motion_.stage;snapshot_.attention=attention_;snapshot_.fireMode=fire_.mode;
    snapshot_.estimatedPrefire=estimatedPrefire_.aim;snapshot_.estimatedPrefireStarted=estimatedPrefire_.started;
    snapshot_.actualShotsInWindow=fire_.count;snapshot_.windowGoalShots=fire_.goal;snapshot_.reason=reason;
    snapshot_.eventAtSeconds=aim_.eventAt;snapshot_.noticedAtSeconds=aim_.noticeAt;snapshot_.decidedAtSeconds=aim_.decisionAt;snapshot_.actionAtSeconds=aim_.startAt;
    // Diagnostic copy only: no RNG, dependency reads, input, ACK or motion writes.
    snapshot_.lastIntentJump=intent.jump;snapshot_.lastIntentCrouch=intent.crouch;
    snapshot_.lastIntentWalk=intent.walk;snapshot_.lastIntentRelease=intent.releaseAll;
    lastRequestedFire_=intent.fire;
    auto&plan=snapshot_.peekPlan;
    const bool motionActive=motion_.peek!=PeekKind::None&&motion_.stage!=MotionStage::Idle;
    if(motionActive){
        if(!math::Same(life_,f.self.life)||!math::Finite(f.self.nowSeconds)||!math::Finite(motion_.startedAt)
           ||!math::Finite(motion_.stageAt)||motion_.startedAt<0||motion_.stageAt<motion_.startedAt
           ||!math::Finite(motion_.start)||!math::Finite(motion_.goal)||!math::Finite(motion_.retreat))plan={};
        else{
            if(!plan.known||!plan.active||plan.startedAtSeconds!=motion_.startedAt||plan.peek!=motion_.peek){
                plan={};plan.known=true;plan.firstIntentSerial=intent.serial;plan.revision=snapshot_.revision;
                plan.startedAtSeconds=motion_.startedAt;
            }
            plan.active=true;plan.lastIntentSerial=intent.serial;plan.stageAtSeconds=motion_.stageAt;
            plan.observedAtSeconds=f.self.nowSeconds;plan.start=motion_.start;plan.goal=motion_.goal;plan.retreat=motion_.retreat;
            plan.peek=motion_.peek;plan.lastActiveMotion=motion_.stage;
            const auto stage=static_cast<unsigned>(motion_.stage);if(stage<16)plan.stageMaskAtNote|=uint16_t(1U<<stage);
            plan.jumpRequested=motion_.jumpRequested;plan.airSeen=motion_.airSeen;plan.landed=motion_.landed;
            plan.crouchSeen=motion_.crouchSeen;plan.informationSeen=motion_.informationSeen;
            plan.returnChosen=motion_.returnChosen;plan.poppinExtended=motion_.poppinExtended;
        }
    }else if(plan.known&&plan.active){
        plan.active=false;plan.lastIntentSerial=intent.serial;plan.stageMaskAtNote|=1U;
        plan.endedAtSeconds=math::Finite(f.self.nowSeconds)&&f.self.nowSeconds>=plan.startedAtSeconds
            &&f.self.nowSeconds>=plan.observedAtSeconds?f.self.nowSeconds:-1;
        plan.terminalOwnKnown=math::Same(life_,f.self.life)&&f.self.valid&&math::Finite(f.self.feet)
            &&plan.endedAtSeconds>=0&&math::Finite(f.self.sampledAtSeconds)&&f.self.sampledAtSeconds>=0
            &&f.self.sampledAtSeconds<=f.self.nowSeconds
            &&f.self.nowSeconds-f.self.sampledAtSeconds<=cfg::Get(profile_,Param::OwnSnapshotMaxAgeMs)*.001;
        plan.terminalOwnFeet=plan.terminalOwnKnown?f.self.feet:Vec3{};
        plan.terminalOwnSampledAtSeconds=plan.terminalOwnKnown?f.self.sampledAtSeconds:-1;
        plan.terminalGroundKnown=plan.terminalOwnKnown&&f.self.groundedKnown;
        plan.terminalGrounded=plan.terminalGroundKnown&&f.self.grounded;
    }
    const bool consumed=f.self.consumptionKnown&&lastIssuedSerial_&&f.self.lastConsumedIntentSerial==lastIssuedSerial_&&f.self.lastConsumedIntentSerial>lastObservedConsumedSerial_;
    snapshot_.nativeIntentConsumptionKnown=consumed;if(consumed)lastObservedConsumedSerial_=f.self.lastConsumedIntentSerial;
    Scope(Domain::General);bool changed=true;
    if(traceWrite_){const auto& old=traces_[(traceWrite_-1)%TraceCapacity];changed=old.target!=snapshot_.target||old.activity!=activity_||old.aim!=aim_.stage||old.peek!=motion_.peek
        ||old.motion!=motion_.stage||old.fireMode!=fire_.mode||old.attention!=attention_||old.reason!=reason||old.actualShotsInWindow!=fire_.count||consumed;}
    if(changed||lastTraceAt_<0||f.self.nowSeconds-lastTraceAt_>=Milliseconds(Param::DecisionTraceIntervalMs)){
        Trace trace{};trace.serial=++traceWrite_;trace.timeKnown=math::Finite(f.self.nowSeconds);trace.atSeconds=trace.timeKnown?f.self.nowSeconds:0;trace.target=aim_.target;
        trace.feedbackProvenance=snapshot_.feedbackProvenance;trace.feedbackConfidence=snapshot_.feedbackConfidence;trace.feedbackAngularUncertaintyDeg=snapshot_.feedbackAngularUncertaintyDeg;
        trace.requestedIntentSerial=intent.serial;trace.consumedIntentSerial=consumed?f.self.lastConsumedIntentSerial:0;
        trace.activity=activity_;trace.aim=aim_.stage;trace.peek=motion_.peek;trace.motion=motion_.stage;trace.fireMode=fire_.mode;trace.attention=attention_;trace.reason=reason;trace.actualShotsInWindow=fire_.count;
        trace.speedCmS=f.self.movementValid?math::Horizontal(f.self.velocity):0;
        if(const auto target=Find(aim_.target)){trace.confidence=target->confidence;trace.aimErrorDegrees=f.self.viewValid&&f.self.eyeValid?math::Angle(f.self.view,math::Look(f.self.eye,target->point)):0;}
        trace.inputRequested=!intent.releaseAll;trace.nativeExecutionObserved=consumed;
        traces_[(traceWrite_-1)%TraceCapacity]=trace;if(traceWrite_>TraceCapacity)++snapshot_.traceOverwritten;lastTraceAt_=f.self.nowSeconds;
    }
    if(plan.known&&plan.lastIntentSerial==intent.serial){
        if(!plan.firstTraceSerial)plan.firstTraceSerial=traceWrite_;plan.lastTraceSerial=traceWrite_;
        if(!plan.active)snapshot_.lastEndedPeekPlan=plan;
    }
    lastIssuedSerial_=intent.serial;
}
inline size_t Brain::CopyTrace(uint64_t afterSerial,Trace* destination,size_t capacity,uint64_t& missed)const{
    missed=0;if(!destination||!capacity||afterSerial>=traceWrite_)return 0;
    const auto first=traceWrite_>=TraceCapacity?traceWrite_-TraceCapacity+1:1;
    const auto wanted=afterSerial+1;if(wanted<first)missed=first-wanted;
    const auto begin=(std::max)(wanted,first);size_t copied=0;
    for(auto serial=begin;serial<=traceWrite_&&copied<capacity;++serial)destination[copied++]=traces_[(serial-1)%TraceCapacity];return copied;
}
inline Intent Brain::Tick(const SensorFrame& f){
    snapshot_.reason=Reason::None;
    const auto finish=[&](Intent value){if(!value.serial)value.serial=++snapshot_.requestedIntents;Note(f,value,snapshot_.reason);return value;};
    const auto neutral=[&](Reason reason){return finish(Neutral(f.self,reason));};
    // Slot zero is the human, even if a transport payload contains a profile.
    if(!stableBotId_||!f.self.life.stableBotId||f.self.life.stableBotId!=stableBotId_)return neutral(Reason::PermissionRejected);
    if(!configured_)return neutral(Reason::ProfileUnavailable);
    if(f.version!=ContractVersion||f.contactCount>MaxContacts||f.soundCount>MaxSounds||f.hurtCount>MaxHurtEvents||f.shotCount>MaxShotReceipts
       ||f.feedbackCount>MaxFeedback||f.retiredTrackCount>MaxRetiredTracks||f.space.angleCount>MaxAngles
       ||f.space.holdLocationCount>MaxHoldLocations)return neutral(Reason::OwnStateUnavailable);
    const auto now=f.self.nowSeconds;
    if(!math::Finite(now)||!math::Finite(f.self.sampledAtSeconds)||f.self.sampledAtSeconds>now)return neutral(Reason::InvalidTime);
    if(!math::Same(life_,f.self.life)&&!BeginLife(f.self.life))return neutral(Reason::InvalidOwnLife);
    if(lastNow_>=0&&now<lastNow_){Interrupt();releasePending_=true;return neutral(Reason::InvalidTime);}
    Scope(Domain::General);
    if(!f.self.valid||now-f.self.sampledAtSeconds>Milliseconds(Param::OwnSnapshotMaxAgeMs)||!f.self.eyeValid||!f.self.viewValid
       ||!math::Finite(f.self.eye)||!math::Finite(f.self.feet)||!math::Finite(f.self.view)){Interrupt();releasePending_=true;return neutral(Reason::OwnStateUnavailable);}
    const auto dt=lastNow_>=0?now-lastNow_:0;lastNow_=now;if(lifeAt_<0)lifeAt_=now;
    if(dt>Milliseconds(Param::MaxDecisionGapMs)){Interrupt();releasePending_=true;return neutral(Reason::InvalidTime);}
    Reason viewReason=Reason::OwnStateUnavailable;
    if(!ObserveView(f.self,viewReason)){Interrupt();releasePending_=true;return neutral(viewReason);}
    Receive(f);
    if(f.eventsOverflow){Interrupt();releasePending_=true;return neutral(Reason::ActionInterrupted);}
    if(!f.self.alive||f.self.retired){Interrupt();activity_=Activity::Dead;releasePending_=true;return neutral(f.self.retired?Reason::Retired:Reason::Dead);}
    if(f.self.paused){Interrupt();releasePending_=true;return neutral(Reason::Paused);}
    if(!enabled_){Interrupt();releasePending_=true;return neutral(Reason::Disabled);}
    if(releasePending_){releasePending_=false;const auto reason=profileChanged_?Reason::ProfileChanged:Reason::ActionInterrupted;profileChanged_=false;return neutral(reason);}
    AttentionTick(f,dt);Remember(f);Decay(now);TacticsTick(f);
    auto target=SelectTarget(f);eventActivity_=activity_;MapHoldTick(f);Decide(f,target);
    if(target&&target->id!=aim_.target)Acquire(f,*target,previousAimTarget_!=0&&previousAimTarget_!=target->id);
    if(releasePending_){releasePending_=false;return neutral(snapshot_.reason==Reason::None?Reason::ActionInterrupted:snapshot_.reason);}
    Intent intent{};intent.life=life_;intent.rotation=f.self.view;
    MotionTick(f,target,intent);
    if(releasePending_){releasePending_=false;return neutral(snapshot_.reason==Reason::None?Reason::ActionInterrupted:snapshot_.reason);}
    ReloadTick(f,intent);
    if(releasePending_){releasePending_=false;return neutral(snapshot_.reason==Reason::None?Reason::ActionInterrupted:snapshot_.reason);}
    AimTick(f,target,dt,intent);FireTick(f,target,intent);
    if(intent.reload)intent.fire=false;if(intent.jump)intent.crouch=false;
    if(!math::Finite(intent.rotation)||!math::Finite(intent.worldDirection)){Interrupt();return neutral(Reason::NativeActionUnavailable);}
    intent.rotation=math::Normalize(intent.rotation);intent.worldDirection=math::FlatUnit(intent.worldDirection);
    intent.releaseAll=!(intent.rotationRequested||math::Horizontal(intent.worldDirection)>0||intent.walk||intent.crouch||intent.jump||intent.fire||intent.reload);
    if(!target&&snapshot_.reason==Reason::None)ReasonNow(Reason::NoAcquiredTarget);
    return finish(intent);
}
} // namespace humanlike_bot
