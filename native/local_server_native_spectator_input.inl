// Included inside local_server_native_test. Explicit case15 only; no normal input polling.
namespace spectator_input_fixture {
inline constexpr Profile InputProfiles[]{
    {0x708d250,64,0x9d17b23f8be16d3aULL},
    {0x18a31e0,64,0x6239e48fb5f39fb0ULL},
    {0x64ea580,543,0x72cb308376334f70ULL},
    {0x78228f0,64,0xff8c81b72fcfcb63ULL},
    {0x78579e0,64,0x93ba7a62c136b88bULL},
    {0x7858510,64,0x45926dedf0bfeb89ULL},
    {0x7858520,64,0xcff0b6ab09893a9fULL},
    {0x7857610,96,0xf57b5a41355878b9ULL},
    {0x6bd6fe0,174,0xc4ac570947090eabULL},
    {0x708a710,1572,0xbed3e3581bbc0519ULL},
};
inline constexpr const char* ContextGuardNames[]{"game_thread","world","owned_local_pc","own_player_state","own_pawn","local_player","driver_world","connection","in_order_owner","specific_owner","input_bindings","profiles","pre_getter_weak","transition_present","transition_weak","transition_type","transition_owner","transition_ack_property","frozen_transition_identity","ack_values"};
inline constexpr uint64_t ContextRequiredMask=(uint64_t(1)<<std::size(ContextGuardNames))-1;
inline constexpr const char* BindingGuardNames[]{"initialized","snapshot_links_valid","owned_context_profiles","current_game_instance","ares_client_chain","manager_owner_link","traits_link","telemetry_local_link","content_index_link","content_provider_link","participant_weak","current_world_getter"};
inline constexpr uint32_t BindingRequiredMask=(uint32_t(1)<<std::size(BindingGuardNames))-1;
struct AbiFieldInfo {
    bool found=false,readable=false,nameMatches=false;void* next=nullptr;
    int32_t offset=-1,bytes=-1,dimensions=-1;uint64_t flags=0,castFlags=0,ownerBits=0;
    uint32_t actualName[3]{},expectedName[3]{};uint8_t boolSize=0,boolOffset=0,boolByteMask=0,boolFieldMask=0;
};
struct InputAbiInfo {
    bool checked=false,found=false,readable=false,execMatches=false,outerMatches=false,receiverDerived=false;
    bool functionWeak=false,ownerWeak=false,keyStructMatches=false,keySchema=false,pressedSchema=false,childChainMatches=false,schemaMatches=false;
    uint16_t parms=0,returnOffset=0;uint8_t numParms=0;uint32_t flags=0;
    int32_t propertiesSize=0,alignment=0,keyStructBytes=0,keyStructAlignment=0;uintptr_t execRva=0;
    Weak functionIdentity{},ownerIdentity{};AbiFieldInfo key{},pressed{};
};
struct Receipt {
    bool dispatchedDown=false,dispatchedUp=false,requestObserved=false,ackObserved=false,cameraObserved=false;
    bool releaseObserved=false;uint32_t downCalls=0,upCalls=0;
    uint64_t pressedAtMs=0,completedAtMs=0,cameraSampleAtMs=0;
    int32_t requestBefore=-1,ackBefore=-1,requestAfter=-1,ackAfter=-1,fromId=-1,toId=-1;
    Weak fromPawn{},toPawn{};uint64_t targetGeneration=0;
    original_input_bindings::Binding binding{};
};
struct Evidence {
    InputAbiInfo inputAbi{};
    bool bound=false,metadata=false,profiles=false,completed=false,skipped=false,releaseUnavailable=false;
    uint8_t step=0;uint64_t wallStartMs=0,readyAtMs=0,nextReadyAtMs=0;
    const char* reason="waiting_native_remote_spectator";
    uint64_t contextWaitStartedAtMs=0,contextReadyAtMs=0,modeWaitStartedAtMs=0;
    uint64_t contextChecks=0,contextKnownMask=0,contextPassedMask=0,transitionGetterCalls=0;
    uint32_t firstFailedGuard=UINT32_MAX,ownerValidMask=0;int32_t ownerFailureIndex=-1;
    bool transitionCacheWasEmpty=false,transitionGetterReturned=false;
    uint32_t bindingKnownMask=0,bindingPassedMask=0,bindingFirstFailedGuard=UINT32_MAX;
    bool bindingInitialized=false,bindingLinksValid=false,creationWorldMatchesCurrent=false;
    bool currentWorldGetterKnown=false,currentWorldGetterMatches=false;uint64_t currentWorldGetterCalls=0;
    std::array<Bound,9> bindingOwners{};
    std::array<Bound,11> owners{};Bound method{};
    void* transition=nullptr;int32_t request=-1,ack=-1,baselineId=-1;
    Weak baselinePawn{},baselineState{};uint64_t baselineGeneration=0;
    bool inOrderActive=false,specificActive=false,shortcutsKnown=false,shortcutsAllowed=false;
    std::array<original_input_bindings::Binding,2> bindings{};
    std::array<Receipt,2> receipts{};
};
inline Evidence evidence;
struct Snapshot {
    Evidence value{};uint64_t nonce=0;uint32_t error=0,bindIndex=0;
    bool configured=false,cliMatched=false,armed=false,finished=false,failed=false,actionsCompleted=false;
    const char* stage="spectator_input_not_published";
};
inline Snapshot published;
inline SRWLOCK publishedLock=SRWLOCK_INIT;
inline std::atomic<bool> stopRequested{false};
inline void Publish(){
    Snapshot value{};value.value=evidence;value.nonce=state.marker.nonce;value.error=state.error;
    value.bindIndex=state.marker.reserved[0];value.configured=state.configured;value.cliMatched=state.cliMatched;
    value.armed=state.armed;value.finished=state.finished;value.failed=state.failed;value.actionsCompleted=state.actionsCompleted;value.stage=state.stage;
    AcquireSRWLockExclusive(&publishedLock);published=value;ReleaseSRWLockExclusive(&publishedLock);
}
inline bool BoundNow(void* value,Bound& out){out.object=value;return Identity(value,out.identity);}
inline bool Live(const Bound& value){return value.object&&Resolve(value.identity)==value.object;}
inline bool Finish(const char* reason,bool completed=false){
    evidence.reason=reason;evidence.completed=completed;evidence.skipped=!completed;
    state.finished=true;state.actionsCompleted=completed;state.phase=Phase::Complete;state.stage=reason;return true;
}
inline bool Ack(){
    if(!Live(evidence.owners[5])||!Live(evidence.owners[7])||evidence.owners[7].object!=evidence.transition
       ||!original_players::IsA(evidence.transition,0x6bd6fe0)||Read<void*>(evidence.transition,0xc8)!=state.pc)return false;
    evidence.request=Read<int32_t>(evidence.transition,0x110);evidence.ack=Read<int32_t>(evidence.transition,0x114);
    return evidence.request>=0&&evidence.request<INT32_MAX&&evidence.ack>=0;
}
inline bool BindingsContext(void* world,void* manager){
    const auto& source=original_input_bindings::State();
    evidence.bindingKnownMask=0;evidence.bindingPassedMask=0;evidence.bindingFirstFailedGuard=UINT32_MAX;
    evidence.bindingInitialized=source.initialized;evidence.bindingLinksValid=source.links_valid;
    evidence.creationWorldMatchesCurrent=source.world==reinterpret_cast<uintptr_t>(world);
    evidence.currentWorldGetterKnown=false;evidence.currentWorldGetterMatches=false;
    const auto pass=[](uint32_t bit,bool value){
        evidence.bindingKnownMask|=uint32_t(1)<<bit;
        if(value)evidence.bindingPassedMask|=uint32_t(1)<<bit;
        else if(evidence.bindingFirstFailedGuard==UINT32_MAX)evidence.bindingFirstFailedGuard=bit;
        return value;
    };
    pass(0,evidence.bindingInitialized);pass(1,evidence.bindingLinksValid);
    if(!pass(2,(evidence.contextPassedMask&0x3ff)==0x3ff&&evidence.profiles))return false;
    if((evidence.bindingPassedMask&3)!=3)return false;
    const auto gi=Read<void*>(world,0x1d8),client=reinterpret_cast<void*>(source.client);
    const auto traits=reinterpret_cast<void*>(source.traits),telemetry=reinterpret_cast<void*>(source.telemetry);
    const auto index=reinterpret_cast<void*>(source.content_index),content=reinterpret_cast<void*>(source.content_manager);
    const auto settings=Read<void*>(client,0x378),rso=Read<void*>(client,0x750);
    pass(3,gi&&gi==reinterpret_cast<void*>(source.game_instance)&&original_players::IsA(gi,0x647e8a0));
    pass(4,original_players::IsA(client,original_input_bindings::ClientClass)&&Read<void*>(gi,0x2b8)==client
        &&Read<void*>(client,0x2c8)==gi&&Read<void*>(client,0x28)==gi);
    pass(5,original_players::IsA(manager,original_input_bindings::ManagerClass)&&Read<void*>(client,0x5e0)==manager
        &&Read<void*>(manager,0x28)==client);
    pass(6,original_players::IsA(traits,original_input_bindings::TraitsClass)&&Read<void*>(client,0x5d8)==traits&&Read<void*>(manager,0x130)==traits);
    pass(7,original_players::IsA(telemetry,0x6a81250)&&Read<void*>(client,0x5c0)==telemetry&&Read<void*>(manager,0x138)==telemetry
        &&source.telemetry_local_only&&!Read<void*>(telemetry,0xf0)&&!Read<void*>(telemetry,0xf8)&&!Read<uint16_t>(telemetry,0x78)
        &&original_players::IsA(settings,0x6510860)&&Read<uint8_t>(settings,0x890)
        &&original_players::IsA(rso,0x6bd2cb0)&&rso==reinterpret_cast<void*>(source.rso)
        &&Read<void*>(telemetry,0xe0)==settings&&Read<void*>(telemetry,0xe8)==rso);
    pass(8,original_players::IsA(index,0x66711f0)&&Read<void*>(gi,0x2c8)==index&&Read<void*>(manager,0x140)==index&&Read<uint8_t>(index,0xf8));
    pass(9,original_players::IsA(content,0x667b420)&&Read<void*>(client,0x308)==content&&Read<void*>(content,0xa0)==index);
    if((evidence.bindingPassedMask&0x3ff)!=0x3ff)return false;
    void* objects[]{gi,client,manager,traits,telemetry,index,content,settings,rso};
    std::array<Bound,9> candidate{};bool valid=true;
    Bound currentWorld{};
    valid=evidence.bound?(world==evidence.owners[0].object&&Live(evidence.owners[0])):BoundNow(world,currentWorld);
    for(size_t i=0;i<std::size(objects);++i){
        const bool live=evidence.bound?(objects[i]==evidence.bindingOwners[i].object&&Live(evidence.bindingOwners[i])):BoundNow(objects[i],candidate[i]);
        valid=valid&&live;
    }
    if(!pass(10,valid))return false;
    // Query the current world through the complete original getter. Never
    // reinitialize the provider or dereference its creation-world snapshot.
    ++evidence.currentWorldGetterCalls;evidence.currentWorldGetterKnown=true;
    evidence.currentWorldGetterMatches=engine::function<void*(*)(const void*)>(0x708a710)(world)==manager;
    if(!pass(11,evidence.currentWorldGetterMatches))return false;
    if(!evidence.bound)evidence.bindingOwners=candidate;
    return true;
}
inline bool Context(void* world,void* pc,void* ps){
    ++evidence.contextChecks;evidence.contextKnownMask=0;evidence.contextPassedMask=0;
    evidence.firstFailedGuard=UINT32_MAX;evidence.ownerFailureIndex=-1;evidence.ownerValidMask=0;
    const auto pass=[](uint32_t bit,bool value){
        evidence.contextKnownMask|=uint64_t(1)<<bit;
        if(value)evidence.contextPassedMask|=uint64_t(1)<<bit;
        else if(evidence.firstFailedGuard==UINT32_MAX)evidence.firstFailedGuard=bit;
        return value;
    };
    const auto local=Read<void*>(pc,0x510),driver=Read<void*>(world,0x40),connection=Read<void*>(driver,0xc0);
    const auto order=Read<void*>(pc,0xc70),specific=Read<void*>(pc,0xc78);
    const auto manager=reinterpret_cast<void*>(original_input_bindings::State().manager);
    pass(0,GetCurrentThreadId()==original_players::gameThread);
    pass(1,world&&original_players::world==world);
    pass(2,original_players::activeController==pc&&original_players::IsA(pc,original_players::ShooterControllerClass)&&Read<uint8_t>(pc,0x224)==2);
    pass(3,original_players::IsA(ps,original_players::ShooterStateClass)&&Read<void*>(pc,0x470)==ps&&Read<void*>(ps,0x210)==pc);
    pass(4,original_players::IsA(state.pawn,original_players::ShooterCharacterClass)&&Read<uint8_t>(state.pawn,0x224)==2
        &&Read<void*>(pc,0x4b0)==state.pawn&&Read<void*>(state.pawn,0x4a0)==pc&&Read<void*>(state.pawn,0x488)==ps);
    pass(5,original_players::IsA(local,0x6cb4740)&&Read<void*>(local,0x38)==pc);
    pass(6,driver&&Read<void*>(driver,0x178)==world);
    pass(7,connection&&Read<void*>(connection,0x60)==driver&&Read<void*>(connection,0x38)==pc
        &&Read<void*>(connection,0xa0)==pc&&Read<int32_t>(connection,0x13c)==3);
    pass(8,original_players::IsA(order,0x6d0fa80)&&Read<void*>(order,0xc8)==pc);
    pass(9,original_players::IsA(specific,0x6d103d0)&&Read<void*>(specific,0xc8)==pc);
    pass(10,BindingsContext(world,manager));
    pass(11,evidence.profiles); // Includes the complete native transition getter and class getter.
    if((evidence.contextPassedMask&0xfff)!=0xfff)return false;
    void* objects[]{world,pc,ps,local,connection,order,specific,nullptr,manager,driver,state.pawn};
    std::array<Bound,11> candidate{};bool baseWeak=true;
    for(size_t i=0;i<std::size(objects);++i){
        if(i==7)continue;
        const bool valid=evidence.bound?(objects[i]==evidence.owners[i].object&&Live(evidence.owners[i])):BoundNow(objects[i],candidate[i]);
        if(valid)evidence.ownerValidMask|=uint32_t(1)<<i;
        else{baseWeak=false;if(evidence.ownerFailureIndex<0)evidence.ownerFailureIndex=static_cast<int32_t>(i);}
    }
    if(!pass(12,baseWeak))return false;
    // Original complete getter queries the PC's existing component when its
    // private e8 cache is empty. Only the original getter may populate that cache.
    evidence.transitionCacheWasEmpty|=Read<void*>(order,0xe8)==nullptr;++evidence.transitionGetterCalls;
    auto transition=engine::function<void*(*)(void*)>(0x7857610)(order);
    evidence.transitionGetterReturned=transition!=nullptr;
    if(!pass(13,transition!=nullptr))return false;
    if(!pass(14,BoundNow(transition,candidate[7]))){evidence.ownerFailureIndex=7;return false;}
    evidence.ownerValidMask|=uint32_t(1)<<7;
    if(!pass(15,original_players::IsA(transition,0x6bd6fe0)))return false;
    if(!pass(16,Read<void*>(transition,0xc8)==pc))return false;
    if(!pass(17,local_server_match::Property(Read<void*>(transition,0x10),L"AuthSpectateRequestAck",4,0x114)!=nullptr))return false;
    if(!pass(18,!evidence.bound||(transition==evidence.owners[7].object&&SameWeak(candidate[7].identity,evidence.owners[7].identity)
        &&Live(evidence.owners[7])))){evidence.ownerFailureIndex=7;return false;}
    const auto request=Read<int32_t>(transition,0x110),ack=Read<int32_t>(transition,0x114);
    if(!pass(19,request>=0&&request<INT32_MAX&&ack>=0))return false;
    if(!evidence.bound)evidence.owners=candidate;
    evidence.transition=transition;evidence.request=request;evidence.ack=ack;
    evidence.inOrderActive=(Read<uint8_t>(order,0xc1)&0x20)!=0;
    evidence.specificActive=(Read<uint8_t>(specific,0xc1)&0x20)!=0;
    return true;
}
inline bool Camera(const local_server_spectator_view::View& view,void* pc){
    return local_server_spectator_view::Fresh(view)&&view.controller==pc&&view.localOwnerVerified
        &&view.mode==4&&view.enabled&&view.nativeSpectatingKnown&&view.nativeSpectating&&view.active
        &&view.targetCurrentLife&&view.targetAliveKnown&&view.targetAlive&&view.nativePlayerId>0
        &&view.targetState!=state.ps&&view.cameraTargetConfirmed&&view.poseQueriesKnown;
}
inline bool Compatible(const original_input_bindings::Binding& binding){
    return binding.found&&binding.bind_set_found&&binding.key_down_queried&&!binding.base_key_down
        &&!binding.shift&&!binding.ctrl&&!binding.alt&&!binding.cmd&&!binding.gamepad_shift&&!binding.tap_hold_type;
}
inline bool SameBinding(const original_input_bindings::Binding& a,const original_input_bindings::Binding& b){
    return Compatible(a)&&Compatible(b)&&a.bind_index==b.bind_index&&a.axis_direction==b.axis_direction
        &&a.bind_set==b.bind_set&&std::memcmp(a.key_name_words,b.key_name_words,sizeof(a.key_name_words))==0;
}
inline bool DispatchKey(const uint8_t* chord,bool pressed){
    // The borrowed FKey32 is complete and stays live through both events.
    alignas(8) uint8_t parameters[40]{};std::memcpy(parameters,chord,32);parameters[32]=pressed?1:0;
    __try{return Event(state.pc,evidence.method.object,parameters);}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline bool WithChord(const original_input_bindings::Binding& binding,const uint8_t* chord,void* context){
    auto& receipt=*static_cast<Receipt*>(context);
    if(!chord||!SameBinding(receipt.binding,binding)||!Live(evidence.method)||!Live(evidence.owners[1]))return false;
    receipt.binding=binding;receipt.pressedAtMs=GetTickCount64();
    ++receipt.downCalls;receipt.dispatchedDown=DispatchKey(chord,true);
    // Always release the same original key, even when the down dispatch failed.
    if(Live(evidence.owners[1])){++receipt.upCalls;receipt.dispatchedUp=DispatchKey(chord,false);}
    evidence.releaseUnavailable=!receipt.dispatchedUp;
    return receipt.dispatchedDown&&receipt.dispatchedUp;
}
inline bool Click(size_t step,const local_server_spectator_view::View& view){
    auto& receipt=evidence.receipts[step];receipt.binding=evidence.bindings[step];
    receipt.requestBefore=evidence.request;receipt.ackBefore=evidence.ack;receipt.fromId=view.nativePlayerId;
    receipt.fromPawn={view.targetIdentity.index,view.targetIdentity.serial};
    original_input_bindings::Binding actual{};
    const bool result=original_input_bindings::QueryWithNativeChord(step?L"SecondaryTriggerDown":L"PrimaryTriggerDown",0,
        static_cast<int>(state.marker.reserved[0]),actual,WithChord,&receipt);
    if(!result)return Finish(receipt.downCalls?"spectator_input_dispatch_or_release_unavailable":"spectator_input_binding_changed_or_unsupported");
    if(!Ack())return Finish("spectator_input_ack_identity_unavailable");
    return true;
}
inline void* AbiField(void* function,const wchar_t* name,uint32_t (&words)[3]){
    alignas(8) uint8_t nativeName[16]{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(nativeName,name,1);
    std::memcpy(words,nativeName,sizeof(words));
    return engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(function,nativeName,3);
}
inline AbiFieldInfo ObserveAbiField(void* field,const uint32_t (&name)[3]){
    AbiFieldInfo value{};value.found=field!=nullptr;value.readable=field&&original_players::Readable(field,0x80);
    if(!value.readable)return value;
    value.dimensions=Read<int32_t>(field,0x30);value.bytes=Read<int32_t>(field,0x34);value.offset=Read<int32_t>(field,0x44);
    value.flags=Read<uint64_t>(field,0x38);value.castFlags=Read<uint64_t>(Read<void*>(field,8),0x18);
    value.next=Read<void*>(field,0x18);value.ownerBits=Read<uint64_t>(field,0x10);
    std::memcpy(value.expectedName,name,sizeof(value.expectedName));
    for(size_t i=0;i<3;++i)value.actualName[i]=Read<uint32_t>(field,0x20+i*4);
    value.nameMatches=std::memcmp(value.actualName,value.expectedName,sizeof(value.actualName))==0;
    if((value.castFlags&0x28001ULL)==0x28001ULL){
        value.boolSize=Read<uint8_t>(field,0x78);value.boolOffset=Read<uint8_t>(field,0x79);
        value.boolByteMask=Read<uint8_t>(field,0x7a);value.boolFieldMask=Read<uint8_t>(field,0x7b);
    }
    return value;
}
inline bool BindInputAbi(){
    auto& a=evidence.inputAbi;a={};a.checked=true;
    const auto owner=engine::Class(0x64f1fb0); // Exact original AresPlayerController declaration owner.
    const auto function=engine::Find(L"/Script/ShooterGame.AresPlayerController.SimulateInputKey");
    a.found=function!=nullptr;a.readable=function&&original_players::Readable(function,0xe8);
    if(a.readable){
        a.parms=Read<uint16_t>(function,0xbe);a.propertiesSize=Read<int32_t>(function,0x60);a.alignment=Read<int32_t>(function,0x64);
        a.numParms=Read<uint8_t>(function,0xbc);a.returnOffset=Read<uint16_t>(function,0xc0);a.flags=Read<uint32_t>(function,0xb8);
        const auto exec=Read<uintptr_t>(function,0xe0);a.execMatches=exec==engine::image+0x64ea580;
        a.execRva=exec>=engine::image&&exec-engine::image<0x10000000?exec-engine::image:0;
        a.outerMatches=owner&&Read<void*>(function,0x28)==owner;
        a.receiverDerived=owner&&original_players::Derived(Read<void*>(state.pc,0x10),owner);
        Bound fn{},type{};a.functionWeak=BoundNow(function,fn);a.ownerWeak=BoundNow(owner,type);
        a.functionIdentity=fn.identity;a.ownerIdentity=type.identity;
        uint32_t keyName[3]{},pressedName[3]{};
        auto key=AbiField(function,L"Key",keyName),pressed=AbiField(function,L"bPressed",pressedName);
        a.key=ObserveAbiField(key,keyName);a.pressed=ObserveAbiField(pressed,pressedName);
        const auto keyStruct=Read<void*>(key,0x78);const auto expectedKeyStruct=engine::Find(L"/Script/InputCore.Key");
        a.keyStructMatches=expectedKeyStruct&&keyStruct==expectedKeyStruct;
        a.keyStructBytes=Read<int32_t>(keyStruct,0x60);a.keyStructAlignment=Read<int32_t>(keyStruct,0x64);
        a.childChainMatches=Read<void*>(function,0x58)==key&&a.key.next==pressed&&a.pressed.next==nullptr;
        constexpr uint64_t KeyFlags=0x18000000000080ULL,PressedFlags=0x18001040000280ULL;
        a.keySchema=a.key.readable&&a.key.nameMatches&&a.key.castFlags==0x108001ULL
            &&a.key.dimensions==1&&a.key.bytes==32&&a.key.offset==0&&a.key.flags==KeyFlags
            &&a.keyStructMatches&&a.keyStructBytes==32&&a.keyStructAlignment==8;
        a.pressedSchema=a.pressed.readable&&a.pressed.nameMatches&&a.pressed.castFlags==0x28001ULL
            &&a.pressed.dimensions==1&&a.pressed.bytes==1&&a.pressed.offset==32&&a.pressed.flags==PressedFlags
            &&a.pressed.boolSize==1&&a.pressed.boolOffset==0&&a.pressed.boolByteMask==1&&a.pressed.boolFieldMask==0xff;
        a.schemaMatches=a.parms==33&&a.propertiesSize==40&&a.alignment==1&&a.numParms==2&&a.returnOffset==UINT16_MAX
            &&a.flags==0x04020401&&a.execMatches&&a.outerMatches&&a.receiverDerived&&a.functionWeak&&a.ownerWeak
            &&a.keySchema&&a.pressedSchema&&a.childChainMatches;
    }
    // Exact linked input5 ABI: Parms33, Properties40, MinAlignment1.
    // Caller storage remains the complete alignas(8) FKey32+bool buffer40.
    if(!Function(evidence.method.object,L"/Script/ShooterGame.AresPlayerController.SimulateInputKey",33,0x04020401,0x64ea580))return false;
    if(!a.schemaMatches)return Fail(4905,"spectator_input_full_linked_schema_changed");
    if(evidence.method.object!=function||Resolve(a.functionIdentity)!=function||Resolve(a.ownerIdentity)!=owner)
        return Fail(4905,"spectator_input_function_or_owner_identity_changed");
    return true;
}
inline void WriteAbiField(FILE* file,const AbiFieldInfo& p){
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"found\":%s,\"readable\":%s,\"name_matches\":%s,\"name_words\":[%u,%u,%u],\"expected_name_words\":[%u,%u,%u],\"offset\":%d,\"bytes\":%d,\"dimensions\":%d,\"flags\":%llu,\"cast_flags\":%llu,\"owner_bits\":%llu,\"bool_layout_raw\":",
        b(p.found),b(p.readable),b(p.nameMatches),p.actualName[0],p.actualName[1],p.actualName[2],p.expectedName[0],p.expectedName[1],p.expectedName[2],p.offset,p.bytes,p.dimensions,
        static_cast<unsigned long long>(p.flags),static_cast<unsigned long long>(p.castFlags),static_cast<unsigned long long>(p.ownerBits));
    if((p.castFlags&0x28001ULL)==0x28001ULL)std::fprintf(file,"[%u,%u,%u,%u]}",unsigned(p.boolSize),unsigned(p.boolOffset),unsigned(p.boolByteMask),unsigned(p.boolFieldMask));
    else std::fputs("null}",file);
}
inline void WriteInputAbi(FILE* file,const InputAbiInfo& a,uint32_t nativeInputCalls){
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,",\"input_abi\":{\"probe_only\":false,\"checked\":%s,\"found\":%s,\"readable\":%s,\"actual_parms_size\":%u,\"actual_properties_size\":%d,\"actual_min_alignment\":%d,\"actual_num_parms\":%u,\"actual_flags\":%u,\"actual_exec_rva\":%llu,\"actual_return_offset\":%u,\"outer_matches\":%s,\"receiver_derived\":%s,\"function_weak\":[%d,%d],\"owner_weak\":[%d,%d],\"function_weak_valid\":%s,\"owner_weak_valid\":%s,\"key_struct_matches\":%s,\"key_struct_bytes\":%d,\"key_struct_alignment\":%d,\"key_schema_matches\":%s,\"pressed_schema_matches\":%s,\"child_chain_matches\":%s,\"schema_matches\":%s,\"current_guard_parms_size\":33,\"borrowed_call_buffer_bytes\":40,\"metadata_writes\":0,\"native_input_calls\":%u,\"native_input_calls_scope\":\"SimulateInputKey_dispatch_attempts\",\"key\":",
        b(a.checked),b(a.found),b(a.readable),unsigned(a.parms),a.propertiesSize,a.alignment,unsigned(a.numParms),a.flags,static_cast<unsigned long long>(a.execRva),unsigned(a.returnOffset),
        b(a.outerMatches),b(a.receiverDerived),a.functionIdentity.index,a.functionIdentity.serial,a.ownerIdentity.index,a.ownerIdentity.serial,b(a.functionWeak),b(a.ownerWeak),b(a.keyStructMatches),a.keyStructBytes,a.keyStructAlignment,
        b(a.keySchema),b(a.pressedSchema),b(a.childChainMatches),b(a.schemaMatches),nativeInputCalls);
    WriteAbiField(file,a.key);std::fputs(",\"b_pressed\":",file);WriteAbiField(file,a.pressed);std::fputc('}',file);
}
inline bool Tick(void* world,void* pc,void* ps,void* pawn){
    const auto now=GetTickCount64();if(!evidence.wallStartMs)evidence.wallStartMs=now;
    state.world=world;state.pc=pc;state.ps=ps;state.pawn=pawn;
    if(stopRequested.exchange(false,std::memory_order_relaxed)||state.stopRequested)return Finish("spectator_input_explicit_stop");
    if(now-evidence.wallStartMs>=state.marker.totalMs)return Finish("spectator_input_total_deadline_not_observed");
    if(!evidence.profiles){
        for(const auto& profile:InputProfiles)if(!ProfileMatches(profile)){
            state.profileFailureRva=profile.rva;return Fail(4901,"spectator_input_original_profile_changed");
        }
        evidence.profiles=true;
    }
    if(!evidence.contextWaitStartedAtMs)evidence.contextWaitStartedAtMs=now;
    if(!Context(world,pc,ps)){
        if(evidence.bound)return Finish("spectator_input_owner_or_transition_lifetime_changed");
        evidence.readyAtMs=0;
        const auto waitingSince=evidence.modeWaitStartedAtMs?evidence.modeWaitStartedAtMs:evidence.contextWaitStartedAtMs;
        if(now-waitingSince>=state.marker.waitMs)return Finish("spectator_input_owned_context_unavailable");
        return Wait("spectator_input_waiting_owned_local_context");
    }
    if(!evidence.contextReadyAtMs){evidence.contextReadyAtMs=now;evidence.modeWaitStartedAtMs=now;}
    auto view=local_server_spectator_view::Snapshot(); // Existing cadence only; no extra query/update.
    evidence.shortcutsKnown=view.shortcutsKnown;evidence.shortcutsAllowed=view.shortcutsAllowed;
    if(!evidence.bound){
        if(!Camera(view,pc)||!evidence.inOrderActive||evidence.ack!=evidence.request){
            evidence.readyAtMs=0;
            if(now-evidence.modeWaitStartedAtMs>=state.marker.waitMs)return Finish("spectator_input_native_remote_mode_or_action_stack_unavailable");
            return Wait("spectator_input_waiting_active_native_remote_view_and_settled_ack");
        }
        if(!evidence.readyAtMs)evidence.readyAtMs=now;
        if(now-evidence.readyAtMs<state.marker.warmupMs)return true;
        if(!BindInputAbi())return false;
        if(!BoundNow(evidence.method.object,evidence.method)
           ||!original_players::Derived(Read<void*>(pc,0x10),Read<void*>(evidence.method.object,0x28)))
            return Finish("spectator_input_original_method_owner_unavailable");
        evidence.metadata=true;
        for(size_t i=0;i<2;++i)if(!original_input_bindings::Query(i?L"SecondaryTriggerDown":L"PrimaryTriggerDown",0,
            static_cast<int>(state.marker.reserved[0]),evidence.bindings[i],true)||!Compatible(evidence.bindings[i]))
            return Finish("spectator_input_missing_combined_held_or_noninstant_binding_unsupported");
        if(!std::memcmp(evidence.bindings[0].key_name_words,evidence.bindings[1].key_name_words,12))
            return Finish("spectator_input_same_primary_secondary_key_unsupported");
        evidence.baselineId=view.nativePlayerId;evidence.baselinePawn={view.targetIdentity.index,view.targetIdentity.serial};
        evidence.baselineState={view.targetStateIdentity.index,view.targetStateIdentity.serial};evidence.baselineGeneration=view.targetGeneration;
        evidence.bound=true;state.bound=true;state.metadataVerified=true;evidence.reason="waiting_next_and_previous_receipts";
    }
    if(evidence.step==0){if(!Click(0,view)||state.finished)return !state.error;evidence.step=1;return Wait("spectator_input_wait_next_original_rpc_ack_camera");}
    if(evidence.step==2){
        if(now<evidence.nextReadyAtMs)return true;
        if(!Camera(view,pc)||view.nativePlayerId!=evidence.receipts[0].toId
           ||view.targetIdentity.index!=evidence.receipts[0].toPawn.index||view.targetIdentity.serial!=evidence.receipts[0].toPawn.serial
           ||evidence.ack!=evidence.request)
            return Finish("spectator_input_next_target_changed_before_previous");
        if(!Click(1,view)||state.finished)return !state.error;evidence.step=3;return Wait("spectator_input_wait_previous_original_rpc_ack_camera");
    }
    const size_t current=evidence.step==1?0:1;auto& receipt=evidence.receipts[current];
    receipt.requestAfter=evidence.request;receipt.ackAfter=evidence.ack;
    if(evidence.request>receipt.requestBefore+1)return Finish("spectator_input_concurrent_native_requests_interference");
    receipt.requestObserved=evidence.request==receipt.requestBefore+1;
    receipt.ackObserved=receipt.requestObserved&&evidence.ack>receipt.ackBefore&&evidence.ack>=evidence.request;
    if(now-receipt.pressedAtMs>=state.marker.waitMs)return Finish("spectator_input_request_ack_or_camera_not_observed");
    const bool expected=current?view.nativePlayerId==evidence.baselineId:view.nativePlayerId!=receipt.fromId;
    if(!receipt.ackObserved||!Camera(view,pc)||view.sampledAtMs<=receipt.pressedAtMs||!expected)return true;
    if(current&&(view.targetIdentity.index!=evidence.baselinePawn.index||view.targetIdentity.serial!=evidence.baselinePawn.serial
       ||view.targetStateIdentity.index!=evidence.baselineState.index||view.targetStateIdentity.serial!=evidence.baselineState.serial))
        return Finish("spectator_input_baseline_life_changed");
    // Re-query the real key only to observe release, never to synthesize another event.
    original_input_bindings::Binding released{};
    if(!original_input_bindings::Query(current?L"SecondaryTriggerDown":L"PrimaryTriggerDown",0,
       static_cast<int>(state.marker.reserved[0]),released,true)||!SameBinding(receipt.binding,released))
        return Finish("spectator_input_release_or_binding_readback_unavailable");
    receipt.releaseObserved=true;receipt.cameraObserved=true;receipt.completedAtMs=now;
    receipt.cameraSampleAtMs=view.sampledAtMs;receipt.toId=view.nativePlayerId;
    receipt.toPawn={view.targetIdentity.index,view.targetIdentity.serial};receipt.targetGeneration=view.targetGeneration;
    if(!current){evidence.step=2;evidence.nextReadyAtMs=now+500;return Wait("spectator_input_native_rate_limit_cooldown");}
    evidence.step=4;return Finish("spectator_input_client_keys_ack_camera_complete_host_rpc_correlation_required",true);
}
inline void Write(FILE* file){
    AcquireSRWLockShared(&publishedLock);const auto snapshot=published;ReleaseSRWLockShared(&publishedLock);
    const auto& proof=snapshot.value;
    const auto b=[](bool value){return value?"true":"false";};const auto u=[](uint64_t value){return static_cast<unsigned long long>(value);};
    std::fprintf(file,"{\"schema_version\":1,\"scenario\":15,\"nonce\":%llu,\"configured\":%s,\"cli_matched\":%s,\"armed\":%s,\"finished\":%s,\"failed\":%s,\"error\":%u,\"actions_completed\":%s,\"stage\":\"%s\",\"spectator_input\":{\"scope\":\"client_original_keys_replicated_ack_and_cached_camera_only\",\"step\":%u,\"skipped_or_not_observed\":%s,\"reason\":\"%s\",\"bound\":%s,\"metadata_verified\":%s,\"bind_index\":%u,\"axis_direction\":0,\"wall_start_ms\":%llu,\"maximum_wall_ms\":45000,\"baseline_id\":%d,\"baseline_pawn_weak\":[%d,%d],\"baseline_state_weak\":[%d,%d],\"in_order_active\":%s,\"specific_active\":%s,\"shortcuts_known\":%s,\"shortcuts_allowed\":%s,\"release_unavailable\":%s,\"server_rpc_verified\":false,\"host_rpc_correlation_required\":true,\"physical_os_input_verified\":false,\"direct_request_calls\":0,\"camera_updates\":0,\"target_writes\":0,\"key_source_rva\":\"0x708d250\",\"shared_release_rva\":\"0x18a31e0\",\"owners\":[",
        u(snapshot.nonce),b(snapshot.configured),b(snapshot.cliMatched),b(snapshot.armed),b(snapshot.finished),b(snapshot.failed),snapshot.error,b(snapshot.actionsCompleted),snapshot.stage,
        unsigned(proof.step),b(proof.skipped),proof.reason,b(proof.bound),b(proof.metadata),snapshot.bindIndex,u(proof.wallStartMs),proof.baselineId,
        proof.baselinePawn.index,proof.baselinePawn.serial,proof.baselineState.index,proof.baselineState.serial,b(proof.inOrderActive),b(proof.specificActive),b(proof.shortcutsKnown),b(proof.shortcutsAllowed),b(proof.releaseUnavailable));
    for(size_t i=0;i<proof.owners.size();++i)std::fprintf(file,"%s[%d,%d]",i?",":"",proof.owners[i].identity.index,proof.owners[i].identity.serial);
    std::fprintf(file,"],\"context\":{\"required_mask\":%llu,\"known_mask\":%llu,\"passed_mask\":%llu,\"failed_mask\":%llu,\"missing_mask\":%llu,\"first_failed_guard\":\"%s\",\"owner_valid_mask\":%u,\"owner_failure_index\":%d,\"checks\":%llu,\"wait_started_at_ms\":%llu,\"ready_at_ms\":%llu,\"mode_wait_started_at_ms\":%llu,\"transition_getter_rva\":\"0x7857610\",\"transition_getter_calls\":%llu,\"transition_cache_was_empty\":%s,\"transition_getter_returned\":%s,\"manual_cache_writes\":0,\"components_created\":0},\"receipts\":[",
        u(ContextRequiredMask),u(proof.contextKnownMask),u(proof.contextPassedMask),u(proof.contextKnownMask&~proof.contextPassedMask),u(ContextRequiredMask&~proof.contextPassedMask),
        proof.firstFailedGuard<std::size(ContextGuardNames)?ContextGuardNames[proof.firstFailedGuard]:"none",proof.ownerValidMask,proof.ownerFailureIndex,u(proof.contextChecks),
        u(proof.contextWaitStartedAtMs),u(proof.contextReadyAtMs),u(proof.modeWaitStartedAtMs),u(proof.transitionGetterCalls),b(proof.transitionCacheWasEmpty),b(proof.transitionGetterReturned));
    for(size_t i=0;i<2;++i){const auto& r=proof.receipts[i];const auto& key=r.binding;
        std::fprintf(file,"%s{\"action\":\"%s\",\"direction\":%u,\"bind_index\":%d,\"key_name_words\":[%u,%u,%u],\"modifiers\":[%u,%u,%u,%u,%u],\"tap_hold_type\":%u,\"down_calls\":%u,\"up_calls\":%u,\"down_dispatched\":%s,\"up_dispatched\":%s,\"release_observed\":%s,\"request_observed\":%s,\"ack_observed\":%s,\"camera_observed\":%s,\"pressed_at_ms\":%llu,\"completed_at_ms\":%llu,\"camera_sample_at_ms\":%llu,\"request_before\":%d,\"ack_before\":%d,\"request_after\":%d,\"ack_after\":%d,\"from_id\":%d,\"to_id\":%d,\"from_pawn_weak\":[%d,%d],\"to_pawn_weak\":[%d,%d],\"target_generation\":%llu}",
            i?",":"",i?"SecondaryTriggerDown":"PrimaryTriggerDown",unsigned(i),key.bind_index,key.key_name_words[0],key.key_name_words[1],key.key_name_words[2],
            unsigned(key.shift),unsigned(key.ctrl),unsigned(key.alt),unsigned(key.cmd),unsigned(key.gamepad_shift),unsigned(key.tap_hold_type),r.downCalls,r.upCalls,
            b(r.dispatchedDown),b(r.dispatchedUp),b(r.releaseObserved),b(r.requestObserved),b(r.ackObserved),b(r.cameraObserved),u(r.pressedAtMs),u(r.completedAtMs),u(r.cameraSampleAtMs),
            r.requestBefore,r.ackBefore,r.requestAfter,r.ackAfter,r.fromId,r.toId,r.fromPawn.index,r.fromPawn.serial,r.toPawn.index,r.toPawn.serial,u(r.targetGeneration));
    }
    std::fprintf(file,"],\"binding_source\":{\"required_mask\":%u,\"known_mask\":%u,\"passed_mask\":%u,\"failed_mask\":%u,\"missing_mask\":%u,\"first_failed_guard\":\"%s\",\"initialized\":%s,\"snapshot_links_valid\":%s,\"creation_world_matches_current\":%s,\"current_world_getter_known\":%s,\"current_world_getter_matches\":%s,\"current_world_getter_calls\":%llu,\"getter_rva\":\"0x708a710\",\"initialize_calls\":0,\"owners\":[",
        BindingRequiredMask,proof.bindingKnownMask,proof.bindingPassedMask,proof.bindingKnownMask&~proof.bindingPassedMask,BindingRequiredMask&~proof.bindingPassedMask,
        proof.bindingFirstFailedGuard<std::size(BindingGuardNames)?BindingGuardNames[proof.bindingFirstFailedGuard]:"none",b(proof.bindingInitialized),b(proof.bindingLinksValid),
        b(proof.creationWorldMatchesCurrent),b(proof.currentWorldGetterKnown),b(proof.currentWorldGetterMatches),u(proof.currentWorldGetterCalls));
    for(size_t i=0;i<proof.bindingOwners.size();++i)std::fprintf(file,"%s[%d,%d]",i?",":"",proof.bindingOwners[i].identity.index,proof.bindingOwners[i].identity.serial);
    std::fputs("]}",file);WriteInputAbi(file,proof.inputAbi,proof.receipts[0].downCalls+proof.receipts[0].upCalls+proof.receipts[1].downCalls+proof.receipts[1].upCalls);std::fputs("}}",file);
}
} // namespace spectator_input_fixture
inline bool IsSpectatorInput(){return state.marker.scenario==uint32_t(Scenario::SpectatorInput);}
inline bool SpectatorInputCleanup(){spectator_input_fixture::stopRequested.store(true,std::memory_order_relaxed);return true;}
inline bool StopSpectatorInput(){spectator_input_fixture::stopRequested.store(true,std::memory_order_relaxed);return true;} // No engine dispatch from a worker Stop.
inline bool TickSpectatorInput(void* world,void* pc,void* ps,void* pawn){
    bool returned=false,result=false;
    __try{result=spectator_input_fixture::Tick(world,pc,ps,pawn);returned=true;}
    __finally{
        if(!returned)spectator_input_fixture::Finish("spectator_input_exception_not_observed");
        spectator_input_fixture::Publish();
    }
    return result;
}
inline void WriteSpectatorInputState(FILE* file){spectator_input_fixture::Write(file);}
