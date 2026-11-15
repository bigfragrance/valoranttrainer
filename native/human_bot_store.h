#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "human_bot_wire.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

// Only private resolved bot configuration lives here. No Pawn, engine field,
// original clock, saved move, network ACK or input API is read or written.
namespace human_bot_store {
using Profile=humanlike_bot_config::Profile;
using Header=human_bot_wire::Header;
using Record=human_bot_wire::Record;
using Operation=human_bot_wire::Operation;
inline constexpr uint64_t StageIdleTimeoutMs=5000,StageTotalTimeoutMs=30000;
enum class Outcome:uint32_t {Applied=0,Pending=1,Invalid=2,Unavailable=3,Failed=4};
struct Reply {Outcome outcome=Outcome::Invalid;const char* message="human_bot_invalid";};
struct Transaction {
    bool active=false;Header header{};uint32_t nextPart=0,receivedProfiles=0;
    uint64_t startedAtMs=0,lastProgressAtMs=0,runningChecksum=0;
};
struct Status {
    bool loaded=false,configured=false,initialPresent=false,initialPending=false,legacyFallback=false,closed=false;
    bool consumerAcknowledged=false;
    uint32_t expectedCount=0,initialRevision=0,initialSeed=0,effectiveRevision=0;
    uint64_t effectiveFingerprint=0,commits=0,acceptedParts=0,rejectedCommands=0,expiredTransactions=0;
    uint64_t lastExpiredTransaction=0;
    const char* stage="human_bot_configuration_not_loaded";
};
// The same production class is exercised with own byte buffers in tests.
// Its global accessors below additionally enforce the original host GT owner.
class Store {
    std::array<std::array<Record,human_bot_wire::MaxProfiles>,2> banks{};
    unsigned activeBank=0;
    Header activeHeader{};
    Header admittedInitial{};
    Status status{};
    Transaction pending{};
    bool Records(const Header& h,const uint8_t* bytes,unsigned bank,uint32_t first,uint32_t count){
        auto& destination=banks[bank];
        for(uint32_t n=0;n<count;++n){
            const auto index=first+n;auto& record=destination[index];
            std::memcpy(&record,bytes+size_t(n)*human_bot_wire::RecordBytes,sizeof(record));
            if(record.rosterIndex!=index||(index==0?record.stableBotId!=0:record.stableBotId==0)
               ||!humanlike_bot_config::Validate(record.profile)
               ||record.profileFingerprint!=human_bot_wire::Fingerprint(record))return false;
            if(index&&!humanlike_bot_config::SameGlobalOnly(destination[0].profile,record.profile))return false;
            for(uint32_t previous=0;previous<index;++previous)
                if(destination[previous].stableBotId==record.stableBotId)return false;
            // A live configuration update cannot rename this pair's roster.
            if(status.configured&&banks[activeBank][index].stableBotId!=record.stableBotId)return false;
        }
        return first+count<=h.profileCount;
    }
    Reply Reject(const char* text,bool discard=true){
        ++status.rejectedCommands;status.stage=text;if(discard)pending={};
        return {Outcome::Invalid,text};
    }
    void Activate(const Header& h,unsigned bank,const char* stage){
        activeHeader=h;activeBank=bank;status.configured=true;status.legacyFallback=false;
        status.stage=stage;++status.commits;
        // Keep the previous actual consumer revision/fingerprint until Root
        // acknowledges this exact new configuration at its normal decision tick.
    }
public:
    const Status& State()const{return status;}
    const Header& ActiveHeader()const{return activeHeader;}
    const Transaction& Pending()const{return pending;}
    bool Enabled()const{return status.configured&&(activeHeader.flags&human_bot_wire::Enabled)!=0&&!status.closed;}
    bool Effective()const{return status.configured&&status.consumerAcknowledged
        &&status.effectiveRevision==activeHeader.revision&&status.effectiveFingerprint==activeHeader.contentChecksum&&!status.closed;}
    const Record* Find(uint32_t index)const{
        return status.configured&&!status.closed&&index<activeHeader.profileCount?&banks[activeBank][index]:nullptr;
    }
    bool Legacy(uint32_t count,uint32_t revision,uint32_t seed){
        if(status.loaded||status.closed||count==0||count>human_bot_wire::MaxProfiles)return false;
        status.loaded=status.legacyFallback=true;status.expectedCount=count;
        status.initialRevision=revision;status.initialSeed=seed;
        status.stage="human_bot_legacy_sidecar_absent_no_new_consumer_claim";return true;
    }
    Reply Initial(const uint8_t* data,uint32_t bytes,uint32_t count,uint32_t revision,uint32_t seed){
        if(status.loaded||status.closed)return Reject("human_bot_initial_already_loaded");
        status.initialPresent=true;Header h;
        if(!human_bot_wire::Shape(data,bytes,true,h))return Reject("human_bot_initial_wire_shape_invalid");
        if(h.profileCount!=count||h.revision!=revision||h.seed!=seed)return Reject("human_bot_initial_pair_configuration_mismatch");
        if(!Records(h,data+human_bot_wire::HeaderBytes,0,0,h.profileCount))return Reject("human_bot_initial_profile_invalid");
        const auto checksum=human_bot_wire::Hash(human_bot_wire::ContentPrefix(h),data+human_bot_wire::HeaderBytes,h.payloadBytes);
        if(checksum!=h.contentChecksum)return Reject("human_bot_initial_checksum_mismatch");
        status.loaded=status.initialPending=true;status.expectedCount=count;status.initialRevision=revision;status.initialSeed=seed;
        admittedInitial=h;status.stage="human_bot_initial_validated_waiting_host_game_thread";
        return {Outcome::Pending,status.stage};
    }
    bool ActivateInitial(){
        if(!status.loaded||status.closed)return false;
        if(!status.initialPending)return true;
        Activate(admittedInitial,0,"human_bot_initial_received_consumer_pending");status.initialPending=false;return true;
    }
    bool Expire(uint64_t now){
        if(!pending.active)return false;
        const bool invalidClock=now<pending.startedAtMs||now<pending.lastProgressAtMs;
        if(!invalidClock&&now-pending.startedAtMs<StageTotalTimeoutMs&&now-pending.lastProgressAtMs<StageIdleTimeoutMs)return false;
        status.lastExpiredTransaction=pending.header.transactionId;++status.expiredTransactions;
        status.stage=invalidClock?"human_bot_stage_wall_time_invalid":"human_bot_stage_timeout_old_configuration_preserved";
        pending={};return true;
    }
    Reply Handle(const uint8_t* data,uint32_t bytes,uint64_t now){
        if(!status.loaded||status.closed||status.initialPending)return {Outcome::Unavailable,"human_bot_store_not_accepting_commands"};
        Expire(now);Header h;
        if(!human_bot_wire::Shape(data,bytes,false,h))return Reject("human_bot_command_wire_shape_invalid");
        if(h.profileCount!=status.expectedCount)return Reject("human_bot_population_requires_new_owned_pair");
        if((status.configured&&h.revision<=activeHeader.revision)||(!status.configured&&h.revision<status.initialRevision))
            return Reject("human_bot_configuration_revision_is_stale",false);
        if(h.transactionId==status.lastExpiredTransaction)return Reject("human_bot_stage_transaction_expired");
        const auto operation=static_cast<Operation>(h.operation);
        if(operation==Operation::Abort){
            if(!pending.active||!human_bot_wire::SameBundle(pending.header,h))return Reject("human_bot_abort_has_no_matching_stage",false);
            pending={};status.stage="human_bot_stage_aborted_old_configuration_preserved";
            return {Outcome::Applied,status.stage}; // only abort, never configuration application
        }
        if(operation==Operation::Stage){
            if(!pending.active){
                if(h.partIndex||h.firstProfile)return Reject("human_bot_stage_must_begin_with_part_zero");
                pending.active=true;pending.header=h;pending.startedAtMs=pending.lastProgressAtMs=now;
                pending.runningChecksum=human_bot_wire::ContentPrefix(h);
            }else if(h.partIndex==0&&!human_bot_wire::SameBundle(pending.header,h)){
                return {Outcome::Unavailable,"human_bot_other_transaction_in_progress_abort_or_wait"};
            }
            if(!human_bot_wire::SameBundle(pending.header,h))return Reject("human_bot_stage_bundle_identity_mismatch");
            if(h.partIndex!=pending.nextPart||h.firstProfile!=pending.receivedProfiles)
                return Reject("human_bot_stage_duplicate_missing_or_out_of_order_part");
            const auto end=h.firstProfile+h.recordsInPart;
            if((h.partIndex+1==h.partCount)!=(end==h.profileCount))return Reject("human_bot_stage_part_count_does_not_cover_roster");
            const auto inactive=1U-activeBank;
            if(!Records(h,data+human_bot_wire::HeaderBytes,inactive,h.firstProfile,h.recordsInPart))return Reject("human_bot_stage_profile_invalid");
            pending.runningChecksum=human_bot_wire::Hash(pending.runningChecksum,data+human_bot_wire::HeaderBytes,h.payloadBytes);
            ++pending.nextPart;pending.receivedProfiles=end;pending.lastProgressAtMs=now;++status.acceptedParts;
            status.stage="human_bot_stage_accepted_configuration_not_applied";
            return {Outcome::Pending,status.stage};
        }
        if(operation!=Operation::Commit)return Reject("human_bot_operation_not_supported");
        if(!pending.active)return Reject("human_bot_commit_has_no_stage");
        if(!human_bot_wire::SameBundle(pending.header,h))return Reject("human_bot_commit_bundle_identity_mismatch");
        if(pending.nextPart!=h.partCount||pending.receivedProfiles!=h.profileCount)
            return Reject("human_bot_commit_missing_profile_parts");
        if(pending.runningChecksum!=h.contentChecksum)return Reject("human_bot_commit_checksum_mismatch");
        Activate(h,1U-activeBank,"human_bot_transport_committed_consumer_pending");pending={};
        return {Outcome::Applied,status.stage};
    }
    bool AckConsumerRevision(uint32_t revision){
        if(!status.loaded||!status.configured||status.closed||revision!=activeHeader.revision)return false;
        status.consumerAcknowledged=true;status.effectiveRevision=revision;status.effectiveFingerprint=activeHeader.contentChecksum;
        status.stage="human_bot_configuration_acknowledged_by_decision_consumer";return true;
    }
    void Close(){pending={};status.closed=true;status.stage="human_bot_store_closed";}
};
inline Store store;
inline DWORD gameThread=0;
inline bool bound=false;
inline SRWLOCK summaryLock=SRWLOCK_INIT;
inline std::string summary="{\"configured\":false,\"configuration_effective\":false,\"behavior_execution_verified\":false}";
inline bool Thread(){return bound&&gameThread==GetCurrentThreadId()&&!store.State().closed;}
inline void CacheSummary(){
    const auto& s=store.State();const auto& a=store.ActiveHeader();const auto& p=store.Pending();
    char buffer[2048]{};
    const int written=snprintf(buffer,sizeof(buffer),
        "{\"wire_version\":1,\"loaded\":%s,\"configured\":%s,\"enabled\":%s,\"initial_present\":%s,\"initial_pending\":%s,\"legacy_fallback\":%s,"
        "\"stage\":\"%s\",\"profile_count\":%u,\"param_count\":%u,\"transport_applied_revision\":%u,\"transport_fingerprint\":\"%016llx\","
        "\"consumer_effective_known\":%s,\"consumer_effective_revision\":%u,\"consumer_effective_fingerprint\":\"%016llx\","
        "\"configuration_effective\":%s,\"behavior_execution_verified\":false,\"stage_active\":%s,\"stage_revision\":%u,"
        "\"stage_received_profiles\":%u,\"stage_next_part\":%u,\"commits\":%llu,\"accepted_parts\":%llu,\"rejected_commands\":%llu,\"expired_transactions\":%llu,\"profile_fingerprints\":[",
        s.loaded?"true":"false",s.configured?"true":"false",store.Enabled()?"true":"false",s.initialPresent?"true":"false",s.initialPending?"true":"false",s.legacyFallback?"true":"false",s.stage,
        s.configured?a.profileCount:0U,static_cast<unsigned>(humanlike_bot_config::ParamCount),s.configured?a.revision:0U,
        static_cast<unsigned long long>(s.configured?a.contentChecksum:0),s.consumerAcknowledged?"true":"false",s.effectiveRevision,
        static_cast<unsigned long long>(s.effectiveFingerprint),store.Effective()?"true":"false",p.active?"true":"false",p.active?p.header.revision:0U,
        p.receivedProfiles,p.nextPart,static_cast<unsigned long long>(s.commits),static_cast<unsigned long long>(s.acceptedParts),
        static_cast<unsigned long long>(s.rejectedCommands),static_cast<unsigned long long>(s.expiredTransactions));
    if(written<0||static_cast<size_t>(written)>=sizeof(buffer))return;
    std::string next(buffer,static_cast<size_t>(written));
    if(s.configured&&!s.closed)for(uint32_t n=0;n<a.profileCount;++n){
        char item[48]{};snprintf(item,sizeof(item),"%s\"%016llx\"",n?",":"",static_cast<unsigned long long>(store.Find(n)->profileFingerprint));next+=item;
    }
    next+="],\"profile_fingerprints_scope\":\"transport_applied\",\"stable_bot_ids\":[";
    if(s.configured&&!s.closed)for(uint32_t n=0;n<a.profileCount;++n){
        char item[32]{};snprintf(item,sizeof(item),"%s%u",n?",":"",store.Find(n)->stableBotId);next+=item;
    }
    next+="],\"schema_digest\":\"";
    for(const auto byte:humanlike_bot_config::SchemaDigest){char encoded[3]{};snprintf(encoded,sizeof(encoded),"%02x",unsigned(byte));next+=encoded;}
    next+="\"}";
    AcquireSRWLockExclusive(&summaryLock);summary.swap(next);ReleaseSRWLockExclusive(&summaryLock);
}
struct SummaryReadLock {
    SummaryReadLock(){AcquireSRWLockShared(&summaryLock);}
    ~SummaryReadLock(){ReleaseSRWLockShared(&summaryLock);}
};
inline std::string SummaryJson(){SummaryReadLock lock;return summary;}
inline bool BindGameThread(DWORD expected){
    if(!expected||expected!=GetCurrentThreadId()||!store.State().loaded||store.State().closed)return false;
    if(bound)return gameThread==expected;
    gameThread=expected;bound=true;
    if(!store.ActivateInitial()){bound=false;gameThread=0;return false;}
    CacheSummary();return true;
}
inline const Record* FindRecord(uint32_t index){return Thread()?store.Find(index):nullptr;}
inline const Profile* FindProfile(uint32_t index){const auto r=FindRecord(index);return r?&r->profile:nullptr;}
inline const Header* Configuration(){return Thread()&&store.State().configured?&store.ActiveHeader():nullptr;}
inline bool Enabled(){return Thread()&&store.Enabled();}
inline bool GetProfile(uint32_t index,const Profile*& profile,uint32_t& revision){
    profile=nullptr;revision=0;const auto record=FindRecord(index);if(!record)return false;
    profile=&record->profile;revision=store.ActiveHeader().revision;return true;
}
inline bool AckConsumerRevision(uint32_t revision){
    if(!Thread()||!store.AckConsumerRevision(revision))return false;CacheSummary();return true;
}
inline Reply Handle(const uint8_t* bytes,uint32_t count){
    if(!Thread())return {Outcome::Unavailable,"human_bot_original_host_game_thread_required"};
    const auto reply=store.Handle(bytes,count,GetTickCount64());CacheSummary();return reply;
}
inline void Poll(){if(Thread()&&store.Expire(GetTickCount64()))CacheSummary();}
inline void Close(){if(!bound||gameThread==GetCurrentThreadId()){store.Close();CacheSummary();bound=false;}}
} // namespace human_bot_store
