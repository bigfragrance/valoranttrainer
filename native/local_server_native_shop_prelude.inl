// Included inside local_server_native_test. Only explicit Scenario12 calls this.
// No second Tick, input-stream writer, correction producer or authority action.
// SHOP_PRELUDE_GENERATED_PROFILES_BEGIN
inline constexpr size_t MaximumShopPreludeSwitchReceipts=local_server_native_shop_plan::MaximumCycles*3;
inline constexpr Profile ShopPreludeProfiles[]{
    {0x649fd30,393,0xf35ee280a17f21f7ULL},
    {0x70fc220,543,0xc3a47d325f5746b1ULL},
    {0x70a4d30,213,0x7ac764b4b030092ULL},
    {0x6cd9e70,472,0x3ad05bbbad720ad8ULL},
    {0x7a8e9e0,93,0x18a1729cb14cd4eULL},
    {0x6cd8d70,167,0x325071e1d6e01f7dULL},
    {0x6499330,174,0x2423cf21a48d620cULL},
    {0x64a7370,47,0x7565ad1a3aeac49fULL},
    {0x646e9a0,47,0xd3e53c5feb94e836ULL},
    {0x6773ce0,174,0xf5ad7ac9decb5713ULL},
    {0x6dc7840,174,0x9bc35236285d2003ULL},
    {0x6c62390,56,0x29153e2fde2f0f5eULL},
    {0x70a4530,1859,0x6256e5a01ed4f880ULL},
};
// SHOP_PRELUDE_GENERATED_PROFILES_END
using ShopPreludeField=local_server_native_shop_snapshot::Field;
struct ShopPreludeState {
    local_server_native_shop_plan::Plan plan{};
    bool methodsReady=false,modelReady=false,ownerBound=false,notObserved=false,releaseUnavailable=false;
    bool transition=false,stateKnown=false,oldPrelude=false,purchaseSubmitted=false,purchaseReturned=false,purchaseTransitionProven=false;
    bool pendingHeaderRead=false,pendingRecordsRead=false,purchaseReceiptCaptured=false,purchaseReturnObserved=false;
    bool beforeInputRead=false,afterInputRead=false;
    uint64_t nonce=0,samples=0,switchCalls=0,switchQueueProofs=0,pendingProofs=0,purchaseCalls=0,purchaseAtMs=0;
    uint64_t openedForPurchase=0,preambleLostOnOpen=0;
    Bound pawn{},pc{},model{},purchasable{},targetClass{},switchMethod{},purchaseMethod{},aliveMethod{};
    std::array<Bound,3> slots{};std::array<ShopPreludeField,4> fields{};
    std::array<std::array<uint8_t,40>,3> pendingRecords{};
    std::array<std::array<uint8_t,40>,3> purchasePendingRecords{};
    using SwitchReceipt=local_server_native_shop_snapshot::SwitchReceipt;
    std::array<SwitchReceipt,MaximumShopPreludeSwitchReceipts> switchReceipts{};
    uint32_t switchReceiptCount=0;
    int32_t pendingCount=-1,pendingCapacity=-1,oldIndex=-1,beforeInputIndex=-1,afterInputIndex=-1;
    int32_t switchQueueBefore=-1,switchQueueAfter=-1;uint8_t currentSlot=255,preambleSlot=255;
    int32_t purchasePendingCount=-1,purchasePendingCapacity=-1,purchaseOldIndex=-1;uint8_t purchasePreambleSlot=255;
    void *machine=nullptr,*selectedState=nullptr,*pendingPointer=nullptr;
    void *purchasePendingPointer=nullptr,*purchaseMachine=nullptr,*purchaseSelectedState=nullptr;
    Weak machineIdentity{},selectedIdentity{},purchaseMachineIdentity{},purchaseStateIdentity{};
    uint64_t purchaseTick=0;
    char targetLabel[24]{};
    const char* result="waiting_explicit_fixture";
};
inline ShopPreludeState shopPrelude;
inline local_server_native_shop_snapshot::Cache shopPreludeCache;
inline uint64_t shopPreludePublications=0; // Mutated only on the game thread.
inline std::atomic<bool> shopPreludeStopRequested{false};
inline bool IsShopPrelude(){return state.marker.scenario==12;}
inline void ConfigureShopPreludeSnapshot(uint64_t nonce){
    // Init runs before Observe starts. This publishes config only, never engine
    // state; all unread evidence stays explicitly not_observed/null/-1.
    shopPreludeCache.Store(local_server_native_shop_snapshot::Configured(nonce));
}
inline void PublishShopPrelude(local_server_native_shop_snapshot::Exit exit){
    if(!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread||!IsShopPrelude())return;
    namespace snap=local_server_native_shop_snapshot;
    snap::Snapshot next{};next.enabled=next.published=true;next.exit=exit;
    next.publication=++shopPreludePublications;next.publisherThread=GetCurrentThreadId();
    auto& f=next.fixture;const auto& s=state;
    f.scenario=s.marker.scenario;f.nonce=s.marker.nonce;f.error=s.error;f.phase=unsigned(s.phase);
    f.initialized=s.initialized;f.configured=s.configured;f.cliMatched=s.cliMatched;f.armed=s.armed;
    f.profilesVerified=s.profilesVerified;f.metadataVerified=s.metadataVerified;f.bound=s.bound;
    f.finished=s.finished;f.failed=s.failed;f.actionsCompleted=s.actionsCompleted;f.stopRequested=s.stopRequested;
    f.ticks=s.ticks;f.elapsedSeconds=s.elapsedSeconds;f.ownershipPassedMask=s.ownershipPassedMask;
    f.readinessPassedMask=s.readinessPassedMask;f.expectedThread=s.expectedThread;f.actualThread=s.actualThread;
    f.profileFailureRva=s.profileFailureRva;f.shopOpenKnown=s.shopOpenKnown;f.shopOpen=s.shopOpen;
    f.shopOpenSubmitted=s.shopOpenSubmitted;f.shopCloseSubmitted=s.shopCloseSubmitted;
    std::snprintf(f.stage,sizeof(f.stage),"%s",s.stage);std::snprintf(f.operation,sizeof(f.operation),"%s",s.operation);
    auto& d=next.prelude;const auto& p=shopPrelude;
    d.phase=unsigned(p.plan.phase);d.cycles=p.plan.cycles;d.switchReceiptCount=p.switchReceiptCount;
    d.methodsReady=p.methodsReady;d.modelReady=p.modelReady;d.ownerBound=p.ownerBound;
    d.notObserved=p.notObserved;d.releaseUnavailable=p.releaseUnavailable;
    d.transition=p.transition;d.stateKnown=p.stateKnown;d.oldPrelude=p.oldPrelude;
    d.pendingHeaderRead=p.pendingHeaderRead;d.pendingRecordsRead=p.pendingRecordsRead;
    d.purchaseReceiptCaptured=p.purchaseReceiptCaptured;d.purchaseReturnObserved=p.purchaseReturnObserved;
    d.purchaseReturned=p.purchaseReturned;d.purchaseSubmitted=p.purchaseSubmitted;
    d.purchaseTransitionProven=p.purchaseTransitionProven;d.beforeInputRead=p.beforeInputRead;d.afterInputRead=p.afterInputRead;
    d.samples=p.samples;d.switchCalls=p.switchCalls;d.switchQueueProofs=p.switchQueueProofs;d.pendingProofs=p.pendingProofs;
    d.purchaseCalls=p.purchaseCalls;d.purchaseAtMs=p.purchaseAtMs;d.purchaseTick=p.purchaseTick;
    d.openedForPurchase=p.openedForPurchase;d.preambleLostOnOpen=p.preambleLostOnOpen;
    d.pendingCount=p.pendingCount;d.pendingCapacity=p.pendingCapacity;d.oldIndex=p.oldIndex;
    d.beforeInputIndex=p.beforeInputIndex;d.afterInputIndex=p.afterInputIndex;
    d.currentSlot=p.currentSlot;d.preambleSlot=p.preambleSlot;d.pendingPointer=p.pendingPointer;
    d.purchasePendingCount=p.purchasePendingCount;d.purchasePendingCapacity=p.purchasePendingCapacity;
    d.purchaseOldIndex=p.purchaseOldIndex;d.purchasePreambleSlot=p.purchasePreambleSlot;
    d.purchasePendingPointer=p.purchasePendingPointer;d.purchaseMachine=p.purchaseMachine;d.purchaseSelectedState=p.purchaseSelectedState;
    d.purchaseMachineIdentity={p.purchaseMachineIdentity.index,p.purchaseMachineIdentity.serial};
    d.purchaseStateIdentity={p.purchaseStateIdentity.index,p.purchaseStateIdentity.serial};
    d.model={p.model.object,{p.model.identity.index,p.model.identity.serial}};
    d.purchasable={p.purchasable.object,{p.purchasable.identity.index,p.purchasable.identity.serial}};
    d.targetClass={p.targetClass.object,{p.targetClass.identity.index,p.targetClass.identity.serial}};
    d.fields=p.fields;d.pendingRecords=p.pendingRecords;d.purchasePendingRecords=p.purchasePendingRecords;d.switchReceipts=p.switchReceipts;
    std::memcpy(d.targetLabel,p.targetLabel,sizeof(d.targetLabel));std::snprintf(d.result,sizeof(d.result),"%s",p.result);
    if(exit==snap::Exit::Exception){d.notObserved=true;f.actionsCompleted=false;
        std::snprintf(d.result,sizeof(d.result),"%s","not_observed_game_thread_exception");}
    // All engine dispatch/reads have returned before this POD-only lock scope.
    shopPreludeCache.Store(next);
}
template<class T>inline bool ShopPreludeRead(const void* object,size_t offset,T& out){
    if(!object)return false;const auto bytes=static_cast<const uint8_t*>(object)+offset;
    if(!original_players::Readable(bytes,sizeof(T)))return false;
    std::memcpy(&out,bytes,sizeof(T));return true;
}
inline bool ShopPreludeOwnerFence(){
    const auto& p=shopPrelude;
    return GetCurrentThreadId()==original_players::gameThread&&p.ownerBound
        &&Resolve(p.pawn.identity)==p.pawn.object&&Resolve(p.pc.identity)==p.pc.object
        &&state.pawn==p.pawn.object&&state.pc==p.pc.object
        &&Read<void*>(p.pc.object,0x4b0)==p.pawn.object&&Read<void*>(p.pawn.object,0x4a0)==p.pc.object;
}
inline bool ShopPreludeClose(){
    if(!ShopPreludeOwnerFence()){shopPrelude.releaseUnavailable=true;return false;}
    if(!QueryShop())return false;
    return !state.shopOpen||Toggle(false);
}
inline bool ShopPreludeNotObserved(const char* reason){
    auto& p=shopPrelude;p.notObserved=true;p.result=reason;
    if(ShopPreludeOwnerFence()){if(!ShopPreludeClose())return false;}
    else if(state.shopOpenSubmitted)p.releaseUnavailable=true;
    state.finished=true;state.actionsCompleted=false;state.phase=Phase::Complete;
    state.stage="native_shop_prelude_condition_not_observed";return true;
}
inline bool ShopPreludeFailure(const char* reason){
    if(ShopPreludeOwnerFence())ShopPreludeClose();
    return Fail(5190,reason);
}
inline bool ShopPreludeFieldAt(size_t index,void* function,const wchar_t* name,int32_t offset,uintptr_t enumGetter,bool boolean){
    auto& d=shopPrelude.fields[index];d={};d.observed=true;original_jett_priming::PropertyInfo p{};
    original_jett_priming::FieldOffset(function,name,1,&p);
    d.field=p.field;d.offset=p.offset;d.size=p.elementSize;d.dim=p.arrayDim;d.castFlags=p.castFlags;
    d.flags=Read<uint64_t>(p.field,0x38);
    if(!p.field)d.mismatch|=1;if(!p.genericProperty||!p.nameMatches||!p.boundsValid)d.mismatch|=2;
    if(p.offset!=offset||p.elementSize!=1||p.arrayDim!=1)d.mismatch|=4;
    const uint64_t declared=boolean?0x10000000000580ULL:0x10000000000080ULL;
    const uint64_t linked=declared|0x0008001040000200ULL;
    if(d.flags!=linked)d.mismatch|=8;
    if(boolean){if(p.castFlags!=0x28001||p.boolFieldSize!=1||p.boolByteOffset!=0
            ||p.boolByteMask!=1||p.boolFieldMask!=255)d.mismatch|=16;
    }else{
        d.expectedEnum=engine::function<void*(*)()>(enumGetter)();d.actualEnum=Read<void*>(p.field,0x80);
        if(!d.expectedEnum||d.actualEnum!=d.expectedEnum)d.mismatch|=32;
    }
    return d.mismatch==0;
}
inline bool ShopPreludeMethods(){
    auto& p=shopPrelude;
    if(p.methodsReady){
        if(Resolve(p.switchMethod.identity)==p.switchMethod.object&&Resolve(p.purchaseMethod.identity)==p.purchaseMethod.object
           &&Resolve(p.aliveMethod.identity)==p.aliveMethod.object)return true;
        return ShopPreludeFailure("native_shop_prelude_original_method_weak_changed");
    }
    for(const auto& profile:ShopPreludeProfiles)if(!ProfileMatches(profile))return ShopPreludeFailure("native_shop_prelude_original_profile_changed");
    if(!Function(p.switchMethod.object,L"/Script/ShooterGame.AresInventory.ForceActivateSlotInput",2,0x04020401,0x649fd30)
       ||!Function(p.purchaseMethod.object,L"/Script/ShooterGame.ShopPurchasableModel.TryPurchase",2,0x04020401,0x6cd9e70)
       ||!Function(p.aliveMethod.object,L"/Script/ShooterGame.ShooterCharacter.IsAlive",1,0x54020401,0x6c62390))return false;
    if(Read<void*>(p.switchMethod.object,0x28)!=engine::Class(0x6499330)
       ||Read<void*>(p.purchaseMethod.object,0x28)!=engine::Class(0x6cd8d70)
       ||Read<void*>(p.aliveMethod.object,0x28)!=engine::Class(original_players::ShooterCharacterClass))
        return ShopPreludeFailure("native_shop_prelude_original_method_owner_changed");
    if(!ShopPreludeFieldAt(0,p.switchMethod.object,L"ItemSlot",0,0x64a7370,false)
       ||!ShopPreludeFieldAt(1,p.switchMethod.object,L"ActivationType",1,0x646e9a0,false)
       ||!ShopPreludeFieldAt(2,p.purchaseMethod.object,L"TargetSlot",0,0x64a7370,false)
       ||!ShopPreludeFieldAt(3,p.purchaseMethod.object,L"ReturnValue",1,0,true))
        return ShopPreludeFailure("native_shop_prelude_original_parameter_contract_changed");
    if(!Identity(p.switchMethod.object,p.switchMethod.identity)||!Identity(p.purchaseMethod.object,p.purchaseMethod.identity)
       ||!Identity(p.aliveMethod.object,p.aliveMethod.identity))return ShopPreludeFailure("native_shop_prelude_method_weak_unavailable");
    p.methodsReady=true;return true;
}
inline bool ShopPreludeSlots(){
    auto& p=shopPrelude;p.currentSlot=255;
    for(size_t n=0;n<p.slots.size();++n){
        auto slot=Read<void*>(state.inventory,0x1b8+n*8);auto actor=Read<void*>(slot,0x38);
        if(!original_players::IsA(slot,0x68f63c0)||original_players::IsA(slot,0x69cf3d0)
           ||Read<void*>(slot,0x40)!=state.inventory||Read<uint8_t>(slot,0x30)!=static_cast<uint8_t>(n)
           ||!original_players::IsA(actor,0x645c660)||Read<uint8_t>(actor,0x224)!=2
           ||Read<void*>(actor,0x210)!=state.pawn||Read<void*>(actor,0x4a8)!=state.pawn
           ||!Identity(actor,p.slots[n].identity))return false;
        p.slots[n].object=actor;if(actor==state.current)p.currentSlot=static_cast<uint8_t>(n);
    }
    return true;
}
inline bool ShopPreludeSelectModel(){
    auto& p=shopPrelude;
    if(p.modelReady)return Resolve(p.model.identity)==p.model.object&&Resolve(p.purchasable.identity)==p.purchasable.object
        &&Resolve(p.targetClass.identity)==p.targetClass.object&&Read<void*>(p.model.object,0x1c0)==state.playerInfo;
    if(!p.slots[0].object)return false;
    const all_weapons::Definition *phantom=nullptr,*vandal=nullptr;
    for(const auto& d:all_weapons::definitions)if(d.identityVerified&&d.runtimeClass&&d.equipmentSlot==0&&d.label){
        if(std::strcmp(d.label,"Phantom")==0)phantom=&d;else if(std::strcmp(d.label,"Vandal")==0)vandal=&d;
    }
    const auto target=phantom&&Read<void*>(p.slots[0].object,0x10)==phantom->runtimeClass?vandal:phantom;
    if(!target)return false;
    auto groups=Read<Array>(state.coordinator,0x98);
    if(groups.count<1||groups.count>16||groups.capacity<groups.count||!groups.data
       ||!original_players::Readable(groups.data,size_t(groups.count)*16))return false;
    for(int32_t g=0;g<groups.count;++g){auto models=Read<Array>(groups.data,size_t(g)*16);
        if(models.count<0||models.count>256||models.capacity<models.count||(models.count&&!models.data)
           ||(models.count&&!original_players::Readable(models.data,size_t(models.count)*8)))return false;
        for(int32_t n=0;n<models.count;++n){auto model=models.data[n];
            if(!original_players::IsA(model,0x6cd8d70)||Read<void*>(model,0x1c0)!=state.playerInfo)continue;
            auto item=Read<void*>(model,0x1c8);
            if(!original_players::IsA(item,0x64f3360)||Read<void*>(item,0xa0)!=target->runtimeClass)continue;
            if(!Identity(model,p.model.identity)||!Identity(item,p.purchasable.identity)||!Identity(target->runtimeClass,p.targetClass.identity))return false;
            p.model.object=model;p.purchasable.object=item;p.targetClass.object=target->runtimeClass;
            std::snprintf(p.targetLabel,sizeof(p.targetLabel),"%s",target->label);p.modelReady=true;return true;
        }
    }
    return false;
}
inline bool ShopPreludeTransition(){
    auto& p=shopPrelude;p.stateKnown=false;p.transition=false;
    p.machineIdentity={};p.selectedIdentity={};
    p.machine=Read<void*>(state.current,0xe00);p.selectedState=Read<void*>(p.machine,0x1b0);
    Weak machine{},selected{};
    if(!original_players::IsA(p.machine,0x67722c0)||!Identity(p.machine,machine)||!Identity(p.selectedState,selected)
       ||Read<void*>(p.machine,0xc8)!=state.current||Read<void*>(p.selectedState,0xc8)!=state.current
       ||Read<void*>(p.selectedState,0xe0)!=p.machine||Read<uint8_t>(p.machine,0x348)!=1)return false;
    p.machineIdentity=machine;p.selectedIdentity=selected;p.stateKnown=true;
    p.transition=original_players::IsA(p.selectedState,0x6773ce0)||original_players::IsA(p.selectedState,0x6dc7840);
    return p.transition;
}
inline bool ShopPreludePending(){
    auto& p=shopPrelude;p.oldPrelude=false;p.preambleSlot=255;
    p.pendingHeaderRead=p.pendingRecordsRead=false;p.pendingRecords={};p.pendingPointer=nullptr;
    p.pendingCount=p.pendingCapacity=p.oldIndex=-1;int32_t auth=-1,clientInput=-1;
    if(!ShopPreludeRead(state.input,0x25f0,p.pendingPointer)||!ShopPreludeRead(state.input,0x25f8,p.pendingCount)
       ||!ShopPreludeRead(state.input,0x25fc,p.pendingCapacity)||!ShopPreludeRead(state.inventory,0x2f8,p.oldIndex)
       ||!ShopPreludeRead(state.inventory,0x328,auth)||!ShopPreludeRead(state.inputState,0x260,clientInput))return false;
    p.pendingHeaderRead=true;
    if(p.pendingCount<3||p.pendingCount>8192||p.pendingCapacity<p.pendingCount||p.pendingCapacity>8192
       ||!original_players::Readable(p.pendingPointer,120)||p.oldIndex<0
       ||auth!=p.oldIndex||clientInput!=p.oldIndex)return false;
    for(size_t n=0;n<3;++n)std::memcpy(p.pendingRecords[n].data(),static_cast<const uint8_t*>(p.pendingPointer)+n*40,40);
    p.pendingRecordsRead=true;
    const auto& a=p.pendingRecords[0];const auto& b=p.pendingRecords[1];const auto& c=p.pendingRecords[2];
    const auto index=Read<int32_t>(a.data(),8);const auto item=Read<Weak>(b.data(),8);const float deadline=Read<float>(c.data(),8);
    if(a[0]!=6||b[0]!=4||c[0]!=7||index!=p.oldIndex||!std::isfinite(deadline)||deadline<0)return false;
    for(size_t n=0;n<p.slots.size();++n)if(SameWeak(item,p.slots[n].identity)&&Resolve(item)==p.slots[n].object){p.preambleSlot=static_cast<uint8_t>(n);break;}
    // The pending4 may be the previous owned knife/sidearm, not currentPrimary.
    if(p.preambleSlot==255)return false;
    ++p.pendingProofs;p.oldPrelude=true;return true;
}
inline bool ShopPreludeSwitch(uint8_t slot){
    auto& p=shopPrelude;if(!ShopPreludeOwnerFence()||slot>2)return false;
    const auto before=Read<int32_t>(state.input,0x8b8);p.switchQueueBefore=before;
    struct Params{uint8_t slot=0,activation=0;} args{slot,0};static_assert(sizeof(Params)==2);
    const bool dispatched=Event(state.inventory,p.switchMethod.object,&args);++p.switchCalls;
    const auto after=Read<int32_t>(state.input,0x8b8);p.switchQueueAfter=after;
    auto data=Read<const uint8_t*>(state.input,0x8b0);const auto capacity=Read<int32_t>(state.input,0x8bc);
    ShopPreludeState::SwitchReceipt* receipt=nullptr;
    if(p.switchReceiptCount<p.switchReceipts.size()){receipt=&p.switchReceipts[p.switchReceiptCount++];
        *receipt={slot,state.ticks,GetTickCount64(),before,after,false};}
    if(!dispatched||before<0||after!=before+2||after>8192||capacity<after||!data
       ||!original_players::Readable(data+size_t(before)*40,80))return false;
    for(int32_t n=0;n<2;++n){auto event=data+size_t(before+n)*40;
        if(event[0]!=1||(event[0x15]&0x1f)!=slot||((event[0x15]>>5)&1)!=uint8_t(n==0)
           ||(event[0x14]&0x3f)!=0)return false;
    }
    if(receipt)receipt->queued=true;++p.switchQueueProofs;return true;
}
inline bool ShopPreludeFreezePurchase(){
    auto& p=shopPrelude;if(p.purchaseReceiptCaptured)return false;
    p.purchasePendingRecords=p.pendingRecords;p.purchasePendingCount=p.pendingCount;p.purchasePendingCapacity=p.pendingCapacity;
    p.purchaseOldIndex=p.oldIndex;p.purchasePreambleSlot=p.preambleSlot;p.purchaseTransitionProven=p.stateKnown&&p.transition;
    p.purchasePendingPointer=p.pendingPointer;p.purchaseMachine=p.machine;p.purchaseSelectedState=p.selectedState;
    p.purchaseMachineIdentity=p.machineIdentity;p.purchaseStateIdentity=p.selectedIdentity;
    p.purchaseTick=state.ticks;p.purchaseReceiptCaptured=true;return true;
}
inline bool ShopPreludeBuy(){
    auto& p=shopPrelude;if(p.purchaseReceiptCaptured||!ShopPreludeOwnerFence()||!ShopPreludeSelectModel())return false;
    if(!QueryShop())return false;
    if(!state.shopOpen){if(!Toggle(true)||!QueryShop())return false;++p.openedForPurchase;}
    // Recheck after real original UI dispatch. Never buy from a stale copied
    // pending predicate if opening the menu changed the actual producer state.
    state.current=Read<void*>(state.inventory,0x280);
    if(!state.shopOpen||!ShopPreludeSlots()||p.currentSlot!=0||!ShopPreludeTransition()||!ShopPreludePending()){
        ++p.preambleLostOnOpen;return false;
    }
    if(Resolve(p.model.identity)!=p.model.object||Read<void*>(p.model.object,0x1c0)!=state.playerInfo)return false;
    p.beforeInputRead=ShopPreludeRead(state.inputState,0x260,p.beforeInputIndex);
    if(!p.beforeInputRead||!ShopPreludeFreezePurchase())return false;
    struct Params{uint8_t slot=0,result=0;} args;static_assert(sizeof(Params)==2);
    ++p.purchaseCalls;p.purchaseAtMs=GetTickCount64();state.operation="original_shop_model_try_purchase_during_native_pending_prelude";
    const bool dispatched=Event(p.model.object,p.purchaseMethod.object,&args);state.operation="none";
    p.purchaseReturnObserved=dispatched;p.purchaseReturned=dispatched&&args.result==1;p.purchaseSubmitted=p.purchaseReturned;
    p.afterInputRead=ShopPreludeRead(state.inputState,0x260,p.afterInputIndex);
    return p.purchaseSubmitted;
}
inline bool TickShopPrelude(float){
    namespace plan=local_server_native_shop_plan;auto& p=shopPrelude;
    if(!p.ownerBound){p.pawn={state.pawn,state.identities[3].identity};p.pc={state.pc,state.identities[1].identity};
        p.ownerBound=true;p.nonce=state.marker.nonce;p.result="warming_original_shop_and_native_inputs";}
    if(!ShopPreludeOwnerFence())return ShopPreludeNotObserved("owned_life_retired_before_reproduction");
    if(!ShopPreludeMethods())return false;
    uint8_t alive=0;if(!Event(state.pawn,p.aliveMethod.object,&alive))return false;
    if(alive!=1)return ShopPreludeNotObserved("native_death_before_reproduction");
    if(!QueryShop()||!ShopReady())return false;
    const bool slots=ShopPreludeSlots();const bool models=slots&&ShopPreludeSelectModel();
    ShopPreludeTransition();if(slots)ShopPreludePending();else{p.oldPrelude=false;p.pendingHeaderRead=p.pendingRecordsRead=false;}
    const bool idle=PredictedIdle();const bool target=state.current&&p.targetClass.object&&Read<void*>(state.current,0x10)==p.targetClass.object;
    plan::Observation o;o.shopOpenKnown=state.shopOpenKnown;o.shopOpen=state.shopOpen;o.modelsReady=models;
    o.slotsReady=slots;o.idle=idle;o.currentSlotKnown=p.currentSlot!=255;o.currentSlot=p.currentSlot;
    o.transition=p.stateKnown&&p.transition;o.oldPrelude=p.oldPrelude;o.targetHeld=target;
    ++p.samples;const auto now=GetTickCount64();const auto action=plan::Tick(p.plan,o,now,state.ticks,state.marker.warmupMs,state.marker.totalMs);
    switch(action){
    case plan::Action::None:return true;
    case plan::Action::OpenWarm:return Toggle(true);
    case plan::Action::CloseWarm:case plan::Action::CloseRetry:case plan::Action::ClosePurchase:return ShopPreludeClose();
    case plan::Action::Secondary:case plan::Action::Melee:case plan::Action::Primary:{
        const uint8_t slot=action==plan::Action::Secondary?1:action==plan::Action::Melee?2:0;
        if(!ShopPreludeSwitch(slot))return ShopPreludeNotObserved("original_slot_press_release_not_observed");return true;}
    case plan::Action::OpenAndPurchase:{const bool submitted=ShopPreludeBuy();if(state.error)return false;
        if(p.purchaseReceiptCaptured&&!submitted)return ShopPreludeNotObserved("original_purchase_rejected_or_return_unobserved");
        plan::PurchaseResult(p.plan,submitted,now,state.ticks);return true;}
    case plan::Action::NotObserved:return ShopPreludeNotObserved(p.purchaseSubmitted?"purchase_submitted_native_settle_not_observed":"native_transition_and_old_pending_prelude_not_observed");
    case plan::Action::Complete:p.result="original_purchase_and_native_local_settle_observed_external_host_required";return Complete();
    }
    return true;
}
inline bool WriteShopPrelude(FILE* file){
    const auto snapshot=shopPreludeCache.Copy();
    return local_server_native_shop_snapshot::Write(file,snapshot);
}
