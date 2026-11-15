#pragma once
#include "original_players.h"
#include <array>

// Missing offline authority assembly only. This is an explicit TRAINING slot
// policy, not a recovered claim about the absent server's default slot schema.
// The objects, their virtual queries and OnRep binding are original client code.
// No items are granted, and existing slot objects/contents are never replaced.
namespace original_inventory_slots {
inline constexpr size_t SlotCount=17,MaxInventories=32;
inline constexpr uintptr_t InventoryClass=0x6499330,ItemSlotClass=0x68f63c0,MultiItemSlotClass=0x69cf3d0;
inline constexpr const char* SlotNames[SlotCount]{"Primary","Secondary","Melee","GrenadeAbility","Ability1","Ability2","Passive","Level","Invisible","Ultimate","Unarmed","Armor","Downed","Backpack","Totem","PrimaryStorage","SecondaryStorage"};
enum class Kind:uint8_t{Single,Multi};
struct Policy {
    std::array<Kind,SlotCount> kinds{Kind::Single,Kind::Single,Kind::Single,Kind::Single,
        Kind::Single,Kind::Single,Kind::Multi,Kind::Multi,Kind::Multi,Kind::Single,
        Kind::Single,Kind::Single,Kind::Single,Kind::Multi,Kind::Single,Kind::Single,Kind::Single};
    int32_t multiCapacity=16;
};
inline Policy policy;
struct WeakIdentity{int32_t index=-1,serial=0;};
static_assert(sizeof(WeakIdentity)==8);
struct Slot {
    void *object=nullptr,*originalObject=nullptr,*outer=nullptr,*inventoryBackref=nullptr,*firstItem=nullptr;
    void *initialSingleContents=nullptr,*initialMultiData=nullptr;
    int32_t initialMultiCount=0,initialCount=0,nativeCapacity=-1,nativeCount=-1;
    uint8_t nativeType=254;
    bool created=false,reused=false,multi=false,outerValid=false,backrefValid=false,typeValid=false;
    bool queriesValid=false,firstItemMatches=false,contentsPreserved=false,emptyWhenCreated=false;
};
struct Inventory {
    void *pawn=nullptr,*object=nullptr;
    WeakIdentity pawnIdentity{},inventoryIdentity{};
    std::array<Slot,SlotCount> slots{};
    bool attempted=false,ready=false,originalInventory=false,ownerValid=false,typedOwnerValid=false,nativeNotification=false;
    uint32_t createdCount=0,reusedCount=0,verifiedCount=0;
};
struct Diagnostics {
    uint32_t error=0,inventoryCount=0,readyCount=0;
    uint64_t retiredCount=0,staleRetiredCount=0,totalVerified=0,totalVerifiedSlots=0;
    uintptr_t exceptionRva=0;
    bool restartRequired=false;
    const char* stage="original_inventory_slots_not_initialized";
    const char* exceptionOperation="none";
    const wchar_t* failedEvent=nullptr;
    uint16_t expectedEventSize=0,actualEventSize=0;
};
inline Diagnostics state;
inline std::array<Inventory,MaxInventories> inventories{};
inline const Diagnostics& State(){return state;}
inline bool Fail(uint32_t error,const char* stage){state.error=error;state.stage=stage;return false;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool IsA(void* object,uintptr_t getter){return original_players::IsA(object,getter);}
inline bool Thread(){return !state.restartRequired&&original_players::Thread();}
inline bool SameIdentity(const WeakIdentity& a,const WeakIdentity& b){return a.index==b.index&&a.serial==b.serial&&a.serial>0;}
inline void* ResolveIdentity(const WeakIdentity& identity){
    if(identity.index<0||identity.serial<=0)return nullptr;
    // Actual original FWeakObjectPtr resolver. It checks the native global
    // object array, serial and pending-kill flags; no old raw object read.
    return engine::function<void*(*)(const WeakIdentity*)>(0x1d4cfa0)(&identity);
}
inline bool CaptureIdentity(void* object,WeakIdentity& identity){
    if(!original_players::Readable(object,0x30))return false;
    identity.index=Read<int32_t>(object,0xc);
    if(identity.index<0)return false;
    // Same native serial allocator/getter and global-array address used by
    // original71695d7/71695ea. Normal engine weak identity bookkeeping only.
    identity.serial=engine::function<int32_t(*)(void*,int32_t)>(0x1cf35b0)
        (reinterpret_cast<void*>(engine::image+0xd7b87c0),identity.index);
    return identity.serial>0;
}
inline void RetireRecord(Inventory& entry,bool stale){
    if(!entry.attempted)return;
    if(state.inventoryCount)--state.inventoryCount;
    if(entry.ready&&state.readyCount)--state.readyCount;
    ++state.retiredCount;if(stale)++state.staleRetiredCount;
    // Diagnostic cache only: no engine destructor, slot write, item release,
    // actor visibility change, reference removal or UObject GC operation.
    entry={};
}
inline bool RetirePawnImpl(void* oldPawn){
    if(!Thread()||!oldPawn)return false;
    for(auto& entry:inventories){
        if(!entry.attempted||entry.pawn!=oldPawn)continue;
        const auto resolved=ResolveIdentity(entry.pawnIdentity);
        // A GC-reused address may now identify a DIFFERENT UObject. Reclaim
        // the expired cached identity without reading its old object memory.
        if(resolved!=entry.pawn){
            RetireRecord(entry,true);state.error=0;state.stage="original_inventory_expired_diagnostic_retired";return true;
        }
        WeakIdentity current{};
        if(CaptureIdentity(resolved,current)&&SameIdentity(current,entry.pawnIdentity)){
            RetireRecord(entry,false);state.error=0;state.stage="original_inventory_pawn_diagnostic_retired";return true;
        }
        return Fail(1270,"original_inventory_retire_weak_identity_mismatch");
    }
    // Repeated retirement is harmless and does not examine any engine slots.
    return true;
}
inline bool Event(void* object,const wchar_t* path,uint16_t size,void* parameters){
    auto fn=engine::Find(path);const uint16_t actual=Read<uint16_t>(fn,0xbe);
    if(!object||!fn||actual!=size){state.failedEvent=path;state.expectedEventSize=size;state.actualEventSize=actual;return false;}
    auto table=Read<void**>(object,0);auto process=Read<void*>(table,0x298);
    if(!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,fn,parameters);return true;
}
inline LONG CaptureException(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0001250U;
    const auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;state.exceptionOperation=state.stage;
    state.restartRequired=true;state.stage="original_inventory_slots_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool InspectBefore(Inventory& entry,size_t index){
    auto& slot=entry.slots[index];slot.object=Read<void*>(entry.object,0x1b8+index*8);
    slot.originalObject=slot.object;slot.reused=slot.object!=nullptr;
    if(!slot.object)return true;
    if(!IsA(slot.object,ItemSlotClass))return Fail(1251,"original_inventory_existing_slot_wrong_class");
    slot.multi=IsA(slot.object,MultiItemSlotClass);
    slot.outer=Read<void*>(slot.object,0x28);slot.inventoryBackref=Read<void*>(slot.object,0x40);
    if(slot.outer!=entry.object||slot.inventoryBackref!=entry.object||Read<uint8_t>(slot.object,0x30)!=index)
        return Fail(1252,"original_inventory_existing_slot_pairing_mismatch");
    slot.initialSingleContents=Read<void*>(slot.object,0x38);
    slot.initialMultiData=slot.multi?Read<void*>(slot.object,0x50):nullptr;
    slot.initialMultiCount=slot.multi?Read<int32_t>(slot.object,0x58):0;
    if(slot.initialMultiCount<0||slot.initialMultiCount>4096)return Fail(1253,"original_inventory_existing_multi_count_invalid");
    slot.initialCount=-1;
    if(!Event(slot.object,L"/Script/ShooterGame.ItemSlot.GetItemCount",4,&slot.initialCount))
        return Fail(1254,"original_inventory_existing_count_query_failed");
    ++entry.reusedCount;return true;
}
inline bool ConstructMissing(Inventory& entry,size_t index){
    auto& slot=entry.slots[index];if(slot.object)return true;
    const bool multi=policy.kinds[index]==Kind::Multi;
    if(policy.multiCapacity<=0||policy.multiCapacity>256)return Fail(1255,"original_inventory_training_capacity_invalid");
    struct Params{void* type;void* outer;void* result=nullptr;}
        p{engine::Class(multi?MultiItemSlotClass:ItemSlotClass),entry.object};
    if(!Event(engine::DefaultObject(engine::Class(0x43a9670)),L"/Script/Engine.GameplayStatics.SpawnObject",24,&p)||!p.result)
        return Fail(1256,"original_inventory_original_slot_constructor_failed");
    if(!IsA(p.result,ItemSlotClass)||Read<void*>(p.result,0x28)!=entry.object
       ||IsA(p.result,MultiItemSlotClass)!=multi)return Fail(1257,"original_inventory_constructed_slot_identity_mismatch");
    slot.object=p.result;slot.created=true;slot.multi=multi;
    // Only local TRAINING capacity when this missing slot is newly constructed.
    // MultiContents remains the original constructor's empty reflected array.
    if(multi&&!original_players::Write(slot.object,0x48,policy.multiCapacity))
        return Fail(1258,"original_inventory_training_multi_capacity_failed");
    if(!original_players::Write(entry.object,0x1b8+index*8,slot.object))
        return Fail(1259,"original_inventory_slot_strong_reference_failed");
    ++entry.createdCount;return true;
}
inline bool Validate(Inventory& entry,bool verifyAssemblyContents=true){
    entry.verifiedCount=0;
    for(size_t index=0;index<SlotCount;++index){
        auto& slot=entry.slots[index];
        if(Read<void*>(entry.object,0x1b8+index*8)!=slot.object||!IsA(slot.object,ItemSlotClass))
            return Fail(1260,"original_inventory_slot_identity_readback_failed");
        slot.outer=Read<void*>(slot.object,0x28);slot.inventoryBackref=Read<void*>(slot.object,0x40);
        slot.nativeType=Read<uint8_t>(slot.object,0x30);
        slot.outerValid=slot.outer==entry.object;slot.backrefValid=slot.inventoryBackref==entry.object;slot.typeValid=slot.nativeType==index;
        if(!slot.outerValid||!slot.backrefValid||!slot.typeValid)return Fail(1261,"original_inventory_original_binding_readback_failed");
        slot.nativeCapacity=slot.nativeCount=-1;
        struct First{uint8_t type,padding[7]{};void* result=nullptr;} first{static_cast<uint8_t>(index)};
        // The original error-producing GetFirstItemFromSlot path70fe4e0 now
        // reads the real slot, then dispatches its original virtual GetItem(0).
        slot.queriesValid=Event(slot.object,L"/Script/ShooterGame.ItemSlot.GetCapacity",4,&slot.nativeCapacity)
            &&Event(slot.object,L"/Script/ShooterGame.ItemSlot.GetItemCount",4,&slot.nativeCount)
            &&Event(entry.object,L"/Script/ShooterGame.AresInventory.GetFirstItemFromSlot",16,&first);
        if(!slot.queriesValid)return Fail(1262,"original_inventory_original_slot_queries_failed");
        slot.firstItem=first.result;
        void* expected=Read<void*>(slot.object,0x38);
        if(slot.multi){
            const auto count=Read<int32_t>(slot.object,0x58);auto array=Read<void**>(slot.object,0x50);
            if(count<0||count>4096||(count&&!original_players::Readable(array,static_cast<size_t>(count)*8)))
                return Fail(1263,"original_inventory_multi_array_readback_failed");
            expected=count?Read<void*>(array,0):nullptr;
        }
        slot.firstItemMatches=first.result==expected;
        if(verifyAssemblyContents){
            slot.emptyWhenCreated=!slot.created||(slot.nativeCount==0&&first.result==nullptr);
            slot.contentsPreserved=!slot.reused||(Read<void*>(slot.object,0x38)==slot.initialSingleContents
                &&slot.nativeCount==slot.initialCount&&(!slot.multi||(Read<void*>(slot.object,0x50)==slot.initialMultiData
                &&Read<int32_t>(slot.object,0x58)==slot.initialMultiCount)));
        }
        if(!slot.firstItemMatches||!slot.emptyWhenCreated||!slot.contentsPreserved||slot.nativeCapacity<0
           ||slot.nativeCount<0||slot.nativeCount>slot.nativeCapacity)
            return Fail(1264,"original_inventory_slot_contents_or_query_mismatch");
        ++entry.verifiedCount;
    }
    return entry.verifiedCount==SlotCount;
}
inline bool EnsureImpl(void* pawn){
    if(!Thread()||!IsA(pawn,original_players::ShooterCharacterClass))return Fail(1265,"original_inventory_slots_requires_original_pawn_thread");
    auto inventory=Read<void*>(pawn,0xc08);
    WeakIdentity pawnIdentity{},inventoryIdentity{};
    if(!CaptureIdentity(pawn,pawnIdentity)||ResolveIdentity(pawnIdentity)!=pawn
       ||!CaptureIdentity(inventory,inventoryIdentity)||ResolveIdentity(inventoryIdentity)!=inventory)
        return Fail(1271,"original_inventory_live_weak_identity_unavailable");
    Inventory* found=nullptr;
    for(auto& item:inventories){
        if(!item.attempted)continue;
        // Never reuse a diagnostic record based on pointer equality alone.
        // Expired identities can be swept without dereferencing freed actors.
        if(ResolveIdentity(item.pawnIdentity)!=item.pawn||ResolveIdentity(item.inventoryIdentity)!=item.object){
            RetireRecord(item,true);continue;
        }
        if(item.object==inventory&&item.pawn==pawn&&SameIdentity(item.pawnIdentity,pawnIdentity)
           &&SameIdentity(item.inventoryIdentity,inventoryIdentity)){found=&item;break;}
    }
    if(found){
        if(!found->ready)return Fail(1266,"original_inventory_partial_assembly_requires_new_session");
        return Validate(*found,false);
    }
    for(auto& item:inventories)if(!item.attempted){found=&item;break;}
    if(!found)return Fail(1267,"original_inventory_slots_diagnostic_capacity_reached");
    auto& entry=*found;entry.attempted=true;entry.pawn=pawn;entry.object=inventory;
    entry.pawnIdentity=pawnIdentity;entry.inventoryIdentity=inventoryIdentity;++state.inventoryCount;
    entry.originalInventory=IsA(inventory,InventoryClass);entry.ownerValid=Read<void*>(inventory,0xc8)==pawn;
    entry.typedOwnerValid=Read<void*>(inventory,0x2f0)==pawn;
    if(!entry.originalInventory||!entry.ownerValid||!entry.typedOwnerValid)return Fail(1268,"original_inventory_slots_original_owner_pair_missing");
    state.stage="original_inventory_slots_inspect_existing";
    for(size_t i=0;i<SlotCount;++i)if(!InspectBefore(entry,i))return false;
    state.stage="original_inventory_slots_construct_missing_training_slots";
    for(size_t i=0;i<SlotCount;++i)if(!ConstructMissing(entry,i))return false;
    state.stage="original_inventory_slots_original_binding_notification";
    // Original649e0a0 assigns all SlotType and Inventory fields and sends its
    // normal inventory-items notification. This is called after all17 strong
    // refs exist because its slot counter increments only on non-null entries.
    if(!Event(inventory,L"/Script/ShooterGame.AresInventory.OnRep_ItemSlots",0,nullptr))
        return Fail(1269,"original_inventory_slots_original_onrep_unavailable");
    entry.nativeNotification=true;state.stage="original_inventory_slots_verify_native_queries";
    if(!Validate(entry))return false;
    entry.ready=true;++state.readyCount;++state.totalVerified;state.totalVerifiedSlots+=SlotCount;
    state.error=0;state.stage="original_inventory_all17_slots_verified";return true;
}
inline bool Ensure(void* pawn){__try{return EnsureImpl(pawn);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool EnsureForPawn(void* pawn){return Ensure(pawn);}
inline bool RetirePawn(void* oldPawn){__try{return RetirePawnImpl(oldPawn);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"stage\":");original_players::Text(f,state.stage);
    std::fprintf(f,",\"error\":%u,\"inventory_count\":%u,\"ready_count\":%u,\"restart_required\":%s,\"exception_rva\":%llu,\"exception_operation\":",state.error,state.inventoryCount,state.readyCount,b(state.restartRequired),static_cast<unsigned long long>(state.exceptionRva));original_players::Text(f,state.exceptionOperation);
    std::fprintf(f,",\"retired_count\":%llu,\"stale_retired_count\":%llu,\"total_verified\":%llu,\"total_verified_slots\":%llu",static_cast<unsigned long long>(state.retiredCount),static_cast<unsigned long long>(state.staleRetiredCount),static_cast<unsigned long long>(state.totalVerified),static_cast<unsigned long long>(state.totalVerifiedSlots));
    std::fprintf(f,",\"slot_schema_source\":\"explicit_local_training_policy\",\"recovered_original_server_schema\":false,\"grants_items\":false,\"suppresses_inventory_logging\":false,\"failed_event\":");const auto failed=original_players::Utf8(state.failedEvent);original_players::Text(f,failed.c_str());
    std::fprintf(f,",\"expected_event_size\":%u,\"actual_event_size\":%u,\"inventories\":[",state.expectedEventSize,state.actualEventSize);
    bool first=true;
    for(const auto& entry:inventories){
        if(!entry.attempted)continue;if(!first)std::fputc(',',f);first=false;
        std::fprintf(f,"{\"pawn\":\"%p\",\"inventory\":\"%p\",\"pawn_object_index\":%d,\"pawn_weak_serial\":%d,\"inventory_object_index\":%d,\"inventory_weak_serial\":%d,\"ready\":%s,\"owner_valid\":%s,\"typed_owner_valid\":%s,\"original_notification\":%s,\"created\":%u,\"reused\":%u,\"verified\":%u,\"slots\":[",entry.pawn,entry.object,entry.pawnIdentity.index,entry.pawnIdentity.serial,entry.inventoryIdentity.index,entry.inventoryIdentity.serial,b(entry.ready),b(entry.ownerValid),b(entry.typedOwnerValid),b(entry.nativeNotification),entry.createdCount,entry.reusedCount,entry.verifiedCount);
        for(size_t i=0;i<SlotCount;++i){const auto& s=entry.slots[i];if(i)std::fputc(',',f);
            std::fprintf(f,"{\"index\":%zu,\"name\":\"%s\",\"created\":%s,\"reused\":%s,\"kind\":\"%s\",\"native_slot_type\":%u,\"outer_valid\":%s,\"backref_valid\":%s,\"type_valid\":%s,\"native_capacity\":%d,\"native_item_count\":%d,\"native_queries_valid\":%s,\"first_item_matches\":%s,\"existing_contents_preserved\":%s,\"new_slot_empty\":%s,\"object\":\"%p\",\"first_item\":\"%p\"}",i,SlotNames[i],b(s.created),b(s.reused),s.multi?"MultiItemSlot":"ItemSlot",static_cast<unsigned>(s.nativeType),b(s.outerValid),b(s.backrefValid),b(s.typeValid),s.nativeCapacity,s.nativeCount,b(s.queriesValid),b(s.firstItemMatches),b(s.contentsPreserved),b(s.emptyWhenCreated),s.object,s.firstItem);
        }std::fputs("]}",f);
    }
    std::fputs("]}\n",f);const bool ok=std::ferror(f)==0;std::fclose(f);return ok;
}
}
