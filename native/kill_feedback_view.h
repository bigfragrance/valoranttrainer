#pragma once
#include "deathmatch_rules.h"
#include "original_hud.h"
#include "original_jett_priming.h"

// The original announcement alone receives a display ordinal. Global combat
// events and BasicCombatStats keep their actual aggregate ordinals and scores.
// Original Blueprint code, audio, skin banner and animations run once normally.
namespace kill_feedback_view {
inline constexpr uintptr_t TableRva=0xb2e1ea0;
inline constexpr size_t TableEntries=161;
inline constexpr const wchar_t* AnyKillMethod=L"/Game/UI/InGame/KillBanner/CommonAssets/KillBannerAnnouncement.KillBannerAnnouncement_C.OnAnyPlayerKilled";
using ProcessEvent=void(*)(void*,void*,void*);
struct Hook {void* object=nullptr;void** parent=nullptr;ProcessEvent original=nullptr;original_effects::WeakIdentity identity{};void* table[TableEntries+1]{};};
inline Hook hooks[8]{};
inline void *manual=nullptr,*anyKill=nullptr;
inline uint64_t calls=0,manualMaps=0,announcementMaps=0;
inline unsigned error=0;
inline const char* stage="kill_feedback_waiting_original_announcement";
struct BannerObservation {
    void* banner=nullptr;original_effects::WeakIdentity identity{};
    int referenceOffset=-1,countOffset=-1,maxOffset=-1;
    bool fieldsAvailable=false,visibilityAvailable=false,visible=false;
    int32_t count=-1,maximum=-1;
};
struct DisplayRecord {
    const char* origin="none";bool mapped=false,readback=false,visible=false;
    int32_t incoming=0,requested=0,observed=-1,maximum=-1;
    uint32_t logicalStreak=0,matchKills=0;
};
inline BannerObservation bannerObservation;
inline std::array<DisplayRecord,128> displayRecords{};
inline uint64_t displayRecordCount=0;
inline void SampleBanner(void* announcement){
    auto& b=bannerObservation;
    if(b.referenceOffset<0)b.referenceOffset=original_jett_priming::FieldOffset(original_players::Value<void*>(announcement,0x10),L"KillBanner",8);
    if(b.referenceOffset<0)return;
    auto actual=original_players::Value<void*>(announcement,b.referenceOffset);
    if(!actual)return;
    if(actual!=b.banner||original_effects::ResolveIdentity(b.identity)!=actual){
        b.banner=actual;b.fieldsAvailable=false;
        if(!original_effects::CaptureIdentity(actual,b.identity))return;
        auto type=original_players::Value<void*>(actual,0x10);
        b.countOffset=original_jett_priming::FieldOffset(type,L"CurrentKillCount",4);
        b.maxOffset=original_jett_priming::FieldOffset(type,L"MaxKills",4);
    }
    if(b.countOffset<0||b.maxOffset<0)return;
    b.count=original_players::Value<int32_t>(actual,b.countOffset);
    b.maximum=original_players::Value<int32_t>(actual,b.maxOffset);b.fieldsAvailable=true;
    uint8_t visible=0;b.visibilityAvailable=deathmatch_rules::Event(actual,L"/Script/UMG.Widget.IsVisible",1,&visible);
    b.visible=b.visibilityAvailable&&visible;
}
inline void RecordDisplay(void* object,const char* origin,bool mapped,int32_t incoming,int32_t requested){
    SampleBanner(object);const auto& b=bannerObservation;
    auto& r=displayRecords[displayRecordCount++%displayRecords.size()];
    r={origin,mapped,b.fieldsAvailable,b.visible,incoming,requested,b.count,b.maximum};
    if(auto p=deathmatch_rules::Player(0)){r.logicalStreak=p->streak;r.matchKills=p->matchKills;}
}
inline Hook* Find(void* object){for(auto& h:hooks)if(h.object==object)return &h;return nullptr;}
inline void Observe(void* object,void* fn,void* parameters){
    auto h=Find(object);if(!h||!h->original)return;
    ++calls;
    if(parameters&&fn==manual){
        auto copy=*static_cast<deathmatch_rules::BannerParameters*>(parameters);
        const int32_t incoming=copy.killCount;
        const bool mapped=deathmatch_rules::MapKillBannerParameters(copy);
        if(mapped)++manualMaps;
        h->original(object,fn,&copy);
        if(!copy.preview)RecordDisplay(object,"manual",mapped,incoming,copy.killCount);
        return;
    }
    if(parameters&&fn==anyKill){
        auto copy=*static_cast<original_scoreboard::KillEvent*>(parameters);
        deathmatch_rules::BannerParameters display;
        display.killCount=copy.killerRoundKills;display.maxKills=(std::max)(5,display.killCount);display.equippable=copy.weapon;
        if(deathmatch_rules::MapKillBannerParameters(display)){
            const int32_t incoming=copy.killerRoundKills;
            copy.killerRoundKills=display.killCount;++announcementMaps;h->original(object,fn,&copy);
            RecordDisplay(object,"announcement",true,incoming,copy.killerRoundKills);return;
        }
        auto local=deathmatch_rules::Player(0);
        if(local&&copy.killer==local->playerState){h->original(object,fn,parameters);RecordDisplay(object,"announcement",false,copy.killerRoundKills,copy.killerRoundKills);return;}
    }
    h->original(object,fn,parameters);
}
inline bool Poll(){
    if(!original_players::Thread())return false;
    auto widget=original_hud::FindCoordinatedWidget(deathmatch_rules::AnnouncementClass);
    if(!widget)return true;
    if(auto h=Find(widget)){SampleBanner(widget);return original_effects::ResolveIdentity(h->identity)==widget;}
    manual=engine::Find(deathmatch_rules::ManualBannerMethod);anyKill=engine::Find(AnyKillMethod);
    if(original_players::Value<uint16_t>(manual,0xbe)!=48||original_players::Value<uint16_t>(anyKill,0xbe)!=56){error=2510;stage="kill_feedback_original_presentation_signature_changed";return false;}
    auto actual=original_players::Value<void**>(widget,0);
    auto expected=reinterpret_cast<void**>(engine::image+TableRva);
    if(actual!=expected||!original_players::Readable(actual-1,(TableEntries+1)*8)
       ||actual[0x298/8]!=reinterpret_cast<void*>(engine::image+0x1cdc850)){error=2511;stage="kill_feedback_complete_widget_vtable_mismatch";return false;}
    Hook* available=nullptr;for(auto& h:hooks)if(!h.object){available=&h;break;}
    if(!available){error=2512;stage="kill_feedback_widget_generation_capacity";return false;}
    auto& h=*available;h.object=widget;h.parent=actual;h.original=reinterpret_cast<ProcessEvent>(actual[0x298/8]);
    if(!original_effects::CaptureIdentity(widget,h.identity))return false;
    std::memcpy(h.table,actual-1,sizeof(h.table));h.table[1+0x298/8]=reinterpret_cast<void*>(Observe);
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(widget),h.table+1,actual)!=actual){error=2513;stage="kill_feedback_instance_changed_during_bind";return false;}
    stage="kill_feedback_original_presentation_observed";return true;
}
inline bool WriteDiagnostics(const wchar_t* path){
    FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    std::fprintf(f,"{\"stage\":\"%s\",\"error\":%u,\"calls\":%llu,\"manual_maps\":%llu,\"announcement_maps\":%llu,\"global_ordinal_modified\":false,\"native_visual_callback_forwarded_once\":true",stage,error,
        static_cast<unsigned long long>(calls),static_cast<unsigned long long>(manualMaps),static_cast<unsigned long long>(announcementMaps));
    const auto& b=bannerObservation;
    std::fprintf(f,",\"original_banner_fields_available\":%s,\"original_banner_count\":%d,\"original_banner_maximum\":%d,\"original_banner_visibility_available\":%s,\"original_banner_visible\":%s,\"display_record_count\":%llu,\"display_records\":[",b.fieldsAvailable?"true":"false",b.count,b.maximum,b.visibilityAvailable?"true":"false",b.visible?"true":"false",static_cast<unsigned long long>(displayRecordCount));
    const uint64_t count=(std::min)(displayRecordCount,static_cast<uint64_t>(displayRecords.size()));
    for(uint64_t i=0;i<count;++i){const auto& r=displayRecords[(displayRecordCount-count+i)%displayRecords.size()];
        std::fprintf(f,"%s{\"origin\":\"%s\",\"mapped\":%s,\"readback\":%s,\"visible\":%s,\"incoming\":%d,\"requested\":%d,\"observed\":%d,\"maximum\":%d,\"logical_streak\":%u,\"match_kills\":%u}",i?",":"",r.origin,r.mapped?"true":"false",r.readback?"true":"false",r.visible?"true":"false",r.incoming,r.requested,r.observed,r.maximum,r.logicalStreak,r.matchKills);}
    std::fputs("]}",f);
    std::fclose(f);return true;
}
}
