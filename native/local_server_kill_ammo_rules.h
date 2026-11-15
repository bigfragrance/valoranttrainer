#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

// Pure queue/life rules. No engine identity, score, resource or input mutation.
namespace local_server_kill_ammo_rules {
enum class Seen {New,Duplicate,Conflict,Full,Invalid};
struct Commit {uint64_t token=0;size_t source=0,victim=0;};
template<size_t Capacity> struct Ledger {
    static_assert(Capacity>0);
    std::array<Commit,Capacity> rows{};size_t count=0;
    Seen Lookup(const Commit& key)const{
        if(!key.token||key.source==key.victim)return Seen::Invalid;
        for(size_t n=0;n<count;++n)if(rows[n].token==key.token)
            return rows[n].source==key.source&&rows[n].victim==key.victim?Seen::Duplicate:Seen::Conflict;
        return count==Capacity?Seen::Full:Seen::New;
    }
    Seen Insert(const Commit& key){const auto result=Lookup(key);if(result==Seen::New)rows[count++]=key;return result;}
};
enum class Action {Cancel,Wait,Refill};
struct Gate {
    bool active=false,infiniteAmmo=false,sameLife=false,nativeAlive=false;
    bool gunOwned=false,registrationReady=false;
};
inline Action Decide(const Gate& gate,double now,double due){
    if(!gate.active||gate.infiniteAmmo||!gate.sameLife||!gate.nativeAlive||!gate.gunOwned)return Action::Cancel;
    if(!gate.registrationReady||now<due)return Action::Wait;
    return Action::Refill;
}
} // namespace local_server_kill_ammo_rules
