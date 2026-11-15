#pragma once
#include "humanlike_bot_sensor_settings.h"
#include "humanlike_bot_point_impacts.h"
#include <array>
#include <cmath>

// Optional project-visible-contact model. It does not assert a native hit marker,
// audible hit confirm, enemy health knowledge, or a complete-origin Miss.
namespace humanlike_bot_visible_impacts {
namespace core=humanlike_bot_sensors;
namespace dto=humanlike_bot;
namespace cfg=humanlike_bot_config;
namespace impacts=humanlike_bot_point_impacts;
using ProfileReader=const cfg::Profile*(*)(size_t);
struct Seen {uint64_t binding=0,origin=0;dto::TrackId track=0;};
struct Observer {
    uint64_t cursor=0,seenCursor=0;bool pending=false;impacts::ImpactReceipt receipt{};
    dto::OwnLife own{};std::array<Seen,64> seen{};
};
struct Diagnostics {uint64_t read=0,accepted=0,deferred=0,expired=0,rejected=0,duplicates=0,gaps=0,rays=0;};
class Bridge {
    std::array<Observer,core::MaxPlayers> observers_{};uint32_t next_=1;
    ProfileReader reader_=nullptr;uint64_t activation_=0;Diagnostics diagnostics_{};
public:
    void Reset(ProfileReader reader){for(auto&observer:observers_)observer={};next_=1;reader_=reader;
        activation_=impacts::writeSerial;diagnostics_={};}
    const Diagnostics& Stats()const{return diagnostics_;}
    void Pump(core::Provider&provider,double now,core::Budget&budget,const core::Hooks&hooks){
        if(!reader_||!impacts::diagnostics.installed||!std::isfinite(now))return;
        const uint32_t start=next_;next_=1+next_%(core::MaxPlayers-1);
        for(uint32_t step=0;step<core::MaxPlayers-1;++step){const uint32_t index=1+(start-1+step)%(core::MaxPlayers-1);
            const auto*frame=provider.Frame(index);const auto*profile=reader_(index);
            if(!frame||!profile||!frame->self.valid||!frame->self.alive||!frame->self.viewValid||!frame->self.eyeValid)continue;
            auto&observer=observers_[index];
            if(!core::Same(observer.own,frame->self.life)){const auto cursor=observer.cursor;observer={};observer.own=frame->self.life;
                observer.cursor=(std::max)(cursor,activation_);}
            const auto settings=core::SettingsFromProfile(*profile);if(!core::Valid(settings))continue;
            const double maximumAge=cfg::Get(*profile,cfg::Param::VisualStaleMs)/1000.;
            uint32_t read=0;bool gap=false;
            while(read<settings.eventBudget){
                if(!observer.pending){if(!impacts::Next(observer.cursor,observer.receipt,gap))break;observer.pending=true;++diagnostics_.read;}
                ++read;const auto&receipt=observer.receipt;
                if(!provider.OwnSubjectMatches(index,receipt.source)
                   ||!receipt.actualGeometryPaired||!receipt.nativePointResponseVerified||!receipt.nativeCommitVerified){
                    observer.pending=false;++diagnostics_.rejected;continue;}
                if(receipt.acquiredAtSeconds>now||now-receipt.acquiredAtSeconds>maximumAge){observer.pending=false;++diagnostics_.expired;continue;}
                dto::TrackId track=0;
                if(!provider.AcquiredVisibleTrack(index,receipt.victim,receipt.acquiredAtSeconds,now,maximumAge,track)){
                    observer.pending=false;++diagnostics_.rejected;continue;}
                bool duplicate=false;for(const auto&seen:observer.seen)duplicate=duplicate||(seen.binding==receipt.binding&&seen.origin==receipt.nativeOrigin&&seen.track==track);
                if(duplicate){observer.pending=false;++diagnostics_.duplicates;continue;}
                core::Pose self{};self.valid=self.viewValid=true;self.eye=frame->self.eye;self.view=frame->self.view;
                self.nativeFovKnown=frame->self.nativeViewFovKnown;self.nativeHorizontalFovDegrees=frame->self.nativeHorizontalFovDegrees;
                bool peripheral=false;if(!core::InFov(self,receipt.impact,settings,peripheral)){observer.pending=false;++diagnostics_.rejected;continue;}
                if(!budget.rays){provider.FlagBudgetDeferred(index);++diagnostics_.deferred;break;}
                const auto ray=provider.TraceBudgeted(hooks,index,self.eye,receipt.impact,budget);++diagnostics_.rays;
                if(ray==core::Ray::Unknown){provider.FlagBudgetDeferred(index);++diagnostics_.deferred;break;}
                if(ray!=core::Ray::Clear){observer.pending=false;++diagnostics_.rejected;continue;}
                dto::Feedback feedback{};feedback.kind=dto::Feedback::Kind::Hit;feedback.life=frame->self.life;
                feedback.serial=core::FeedbackSerial(receipt.serial,2);feedback.acquiredAtSeconds=receipt.acquiredAtSeconds;
                feedback.valid=true;feedback.nativeFeedbackObserved=false;feedback.alreadyKnownTrack=track;
                feedback.binding=receipt.binding;feedback.nativeOrigin=receipt.nativeOrigin;
                feedback.provenance=dto::Feedback::Provenance::ProjectVisibleNativeContact;
                feedback.nativeContactObserved=feedback.visibleImpactProven=true;feedback.confidence=1;
                if(!provider.AppendFeedback(index,feedback)){++diagnostics_.deferred;break;}
                observer.seen[observer.seenCursor++%observer.seen.size()]={receipt.binding,receipt.nativeOrigin,track};
                observer.pending=false;++diagnostics_.accepted;
            }
            if(gap){provider.FlagEventGap(index);++diagnostics_.gaps;} // absence/gap is never a Miss
        }
    }
    static void Pass(core::Provider&provider,double now,core::Budget&budget,const core::Hooks&hooks,void*context){
        if(context)static_cast<Bridge*>(context)->Pump(provider,now,budget,hooks);
    }
};
} // namespace humanlike_bot_visible_impacts
