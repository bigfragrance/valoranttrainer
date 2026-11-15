// Included inside local_server_native_test after original input helpers.
// This explicit fixture never selects C, emits a skill event or ticks a skill.
inline bool IsSkillPrimary(){return state.marker.scenario==uint32_t(Scenario::PhoenixCPrimary);}
inline bool SkillOwnerFence(){
    if(!state.bound||GetCurrentThreadId()!=original_players::gameThread)return false;
    for(const auto& id:state.identities)if(!id.object||Resolve(id.identity)!=id.object)return false;
    const auto& ids=state.identities;
    if(state.world!=ids[0].object||state.pc!=ids[1].object||state.ps!=ids[2].object||state.pawn!=ids[3].object
       ||state.localPlayer!=ids[4].object||state.gameInstance!=ids[5].object||state.driver!=ids[6].object||state.connection!=ids[7].object
       ||state.input!=ids[8].object||state.inputState!=ids[9].object||state.inventory!=ids[10].object
       ||Read<uint8_t>(state.pc,0x224)!=2||Read<uint8_t>(state.pawn,0x224)!=2
       ||Read<void*>(state.pc,0x4b0)!=state.pawn||Read<void*>(state.pawn,0x4a0)!=state.pc
       ||Read<void*>(state.pc,0x470)!=state.ps||Read<void*>(state.pawn,0x488)!=state.ps||Read<void*>(state.ps,0x210)!=state.pc
       ||Read<void*>(state.pawn,0xba0)!=ids[9].object||Read<void*>(state.pawn,0xc08)!=ids[10].object
       ||Read<void*>(state.input,0xc8)!=state.pawn||Read<void*>(state.input,0x170)!=state.pawn
       ||Read<void*>(state.inputState,0xc8)!=state.pawn||Read<void*>(state.inputState,0x148)!=state.pawn
       ||Read<void*>(state.inventory,0xc8)!=state.pawn||Read<void*>(state.inventory,0x2f0)!=state.pawn
       ||Read<void*>(state.pc,0x510)!=ids[4].object||Read<void*>(ids[4].object,0x38)!=state.pc
       ||Read<void*>(state.world,0x1d8)!=state.gameInstance||Read<void*>(state.world,0x40)!=state.driver
       ||Read<void*>(state.driver,0x178)!=state.world||Read<void*>(state.driver,0xc0)!=state.connection||Read<void*>(state.connection,0x60)!=state.driver
       ||Read<int32_t>(ids[7].object,0x13c)!=3||Read<void*>(ids[7].object,0x38)!=state.pc||Read<void*>(ids[7].object,0xa0)!=state.pc)return false;
    const auto& client=local_server_client::GetState();
    return client.localPairVerified&&client.nativeClient&&client.world==state.world&&client.controller==state.pc
        &&client.pawn==state.pawn&&client.playerState==state.ps&&original_players::activePawn==state.pawn
        &&original_players::activeController==state.pc;
}
inline bool SkillCleanup(){
    if(!state.primaryHeldRequested)return true;
    if(SkillOwnerFence())return ForcePrimary(false);
    // Retiring only this fixture's private old-binding intention is not an
    // engine release, native death observation or input-consumption proof.
    state.primaryHeldRequested=false;state.skill.privateIntentRetired=true;state.skill.releaseUnavailable=true;
    return true;
}
inline bool SkillAbort(uint32_t error,const char* stage){
    if(!SkillCleanup())state.skill.releaseUnavailable=true;
    return Fail(error,stage);
}
inline bool SkillSnapshotReady(const local_server_hero_skill_readback::Sample& s){
    if(!s.valid||s.host||s.hero!=1||s.epoch==0||s.frameworkRegistration==0||!s.selectedAuthored||s.role!=2||s.slot!=3
       ||!s.ability.weakKnown||!s.ability.classNameKnown
       ||std::strcmp(s.ability.className,"Ability_Phoenix_Q_FireballWall_Production_C")!=0
       ||s.machineOverflow||s.machineCount>s.machines.size())return false;
    const auto primary=Read<void*>(reinterpret_cast<void*>(s.ability.pointer),0xe00);
    for(size_t i=0;i<s.machineCount;++i){const auto& m=s.machines[i];
        if(m.object.pointer==reinterpret_cast<uintptr_t>(primary)&&m.valid&&m.enabled&&m.links&&m.currentKnown
           &&m.object.weakKnown&&m.current.pointer&&m.current.nameKnown&&m.current.classNameKnown
           &&std::strcmp(m.current.name,"RespondToEventState_ThrowProjectile")==0
           &&std::strcmp(m.current.className,"RespondToEventStateComponent")==0)return true;
    }return false;
}
inline bool SkillSample(local_server_hero_skill_readback::Sample& sample){
    namespace fw=local_server_hero_abilities;
    const fw::Player* selected=nullptr;
    for(const auto& p:fw::GetPlayers())if(p.used&&!p.retired&&!p.authority&&p.context.world==state.world
       &&p.context.controller==state.pc&&p.context.playerState==state.ps&&p.context.pawn==state.pawn){
        if(selected)return false;selected=&p;
    }
    const auto& reader=local_server_hero_skill_readback::GetState();
    if(!selected||selected->hero!=1||!local_server_hero_skill_readback::GT()||reader.busy)return false;
    if(!local_server_hero_skill_readback::GuardedSample(state.pawn,selected->epoch,sample)||!sample.valid)return false;
    ++state.skill.samples;
    return sample.pawn.pointer==reinterpret_cast<uintptr_t>(state.pawn)
        &&sample.playerState.pointer==reinterpret_cast<uintptr_t>(state.ps)&&sample.inventory.pointer==reinterpret_cast<uintptr_t>(state.inventory)
        &&sample.epoch==selected->epoch&&sample.frameworkRegistration==selected->registration;
}
inline bool SkillMethods(){
    auto& s=state.skill;if(s.metadata){return Resolve(s.alive.identity)==s.alive.object;}
    constexpr Profile aliveProfile{0x6c62390,56,0x29153e2fde2f0f5eULL};
    if(!ProfileMatches(aliveProfile))return SkillAbort(4970,"native_phoenix_primary_original_alive_profile_changed");
    if(!Function(s.alive.object,L"/Script/ShooterGame.ShooterCharacter.IsAlive",1,0x54020401,0x6c62390)
       ||Read<void*>(s.alive.object,0x28)!=engine::Class(original_players::ShooterCharacterClass)
       ||!Identity(s.alive.object,s.alive.identity))return SkillAbort(4970,"native_phoenix_primary_original_alive_metadata_unavailable");
    s.metadata=true;return true;
}
inline bool SkillAlive(){uint8_t alive=255;return Event(state.pawn,state.skill.alive.object,&alive)&&alive==1;}
inline bool SkillBind(const local_server_hero_skill_readback::Sample& sample){
    auto& s=state.skill;
    const auto ability=reinterpret_cast<void*>(sample.ability.pointer),machine=Read<void*>(ability,0xe00);
    const auto selected=Read<void*>(machine,0x1b0);
    bool sameSampledState=false;
    for(size_t i=0;i<sample.machineCount&&i<sample.machines.size();++i){const auto& m=sample.machines[i];
        if(m.object.pointer==reinterpret_cast<uintptr_t>(machine)&&m.current.pointer==reinterpret_cast<uintptr_t>(selected))sameSampledState=true;}
    if(!sameSampledState||Read<void*>(ability,0x10)!=reinterpret_cast<void*>(sample.ability.type))return false;
    s.ability.object=ability;s.machine.object=machine;s.respond.object=selected;
    if(!Identity(ability,s.ability.identity)||!Identity(machine,s.machine.identity)||!Identity(selected,s.respond.identity)
       ||Read<void*>(state.inventory,0x280)!=ability||Read<void*>(state.inventory,0x268)!=ability
       ||Read<void*>(ability,0x4a8)!=state.pawn||Read<void*>(ability,0x210)!=state.pawn
       ||Read<void*>(machine,0xc8)!=ability||Read<void*>(machine,0x1b0)!=selected
       ||Read<void*>(selected,0xc8)!=ability||Read<void*>(selected,0xe0)!=machine)return false;
    s.epoch=sample.epoch;s.registration=sample.frameworkRegistration;s.selectionBound=true;
    s.beforeInputKnown=sample.inputKnown;s.beforeAccepted=sample.acceptedInputIndex;
    local_server_hero_skill_readback::Sample verify{};
    return SkillSample(verify)&&SkillSnapshotReady(verify)&&verify.epoch==s.epoch&&verify.frameworkRegistration==s.registration
        &&verify.ability.pointer==sample.ability.pointer&&Read<void*>(machine,0x1b0)==selected;
}
inline bool TickSkillPrimary(void* world,void* pc,void* ps,void* pawn,float delta){
    auto& s=state.skill;const uint64_t now=GetTickCount64();
    if(!s.startedAtMs)s.startedAtMs=now;
    if(now<s.startedAtMs)return SkillAbort(4971,"native_phoenix_primary_wall_clock_regressed");
    if(!ValidateOwner(world,pc,ps,pawn))return SkillAbort(state.error,state.stage);
    if(!state.bound||state.ownershipPassedMask!=0x1fff||!SkillOwnerFence()){
        if(state.primaryHeldRequested)return SkillAbort(4972,"native_phoenix_primary_current_owner_lost");
        if(now-s.startedAtMs>=SkillPreparationMs)return SkillAbort(4973,"native_phoenix_primary_owner_preparation_timeout");
        return Wait("native_phoenix_primary_waiting_real_role2_owner");
    }
    if(!ResolveMethods()||!SkillMethods())return SkillAbort(state.error?state.error:4970,state.stage);
    if(state.stopRequested)return SkillAbort(4912,"native_phoenix_primary_explicit_stop_released_or_retired");
    if(now-s.startedAtMs>=SkillTotalMs)return SkillAbort(4913,"native_phoenix_primary_total_timeout_released_or_retired");
    if(s.selectionBound&&!SkillAlive())return SkillAbort(4974,"native_phoenix_primary_original_life_or_query_changed");
    local_server_hero_skill_readback::Sample sample{};const bool sampled=SkillSample(sample);
    if(s.selectionBound&&(!sampled||sample.epoch!=s.epoch||sample.frameworkRegistration!=s.registration))
        return SkillAbort(4974,"native_phoenix_primary_framework_life_or_source_changed");
    if(!QueryPrimary()||state.primaryNow>1)return SkillAbort(4975,"native_phoenix_primary_original_input_query_unavailable");
    if(s.selectionBound&&sampled){s.afterSampled=true;s.afterInputKnown=sample.inputKnown;s.afterAccepted=sample.acceptedInputIndex;}
    state.phaseSeconds+=delta;
    if(state.phase==Phase::ShootHeld){
        state.primaryHeldObserved=state.primaryHeldObserved||state.primaryNow==1;
        if(now-s.pressedAtMs<500)return Wait("native_phoenix_primary_holding_original_input_500ms");
        if(!ForcePrimary(false))return SkillAbort(state.error,state.stage);
        s.releasedAtMs=now;state.phase=Phase::ShootReleased;state.phaseSeconds=0;
        return Wait("native_phoenix_primary_release_waiting_original_query");
    }
    if(state.phase==Phase::ShootReleased){
        if(state.primaryNow==0){state.releaseObserved=true;
            if(!state.primaryHeldObserved)return SkillAbort(4976,"native_phoenix_primary_press_not_consumed_release_observed");
            Complete();state.stage="native_phoenix_primary_input_complete_skill_execution_unverified";return true;
        }
        if(now-s.releasedAtMs>=SkillReleaseMs)return SkillAbort(4915,"native_phoenix_primary_release_not_observed");
        return true;
    }
    state.phase=Phase::Warming;state.primaryBefore=state.primaryNow;
    const bool ready=sampled&&SkillSnapshotReady(sample)&&state.current==reinterpret_cast<void*>(sample.ability.pointer)
        &&Read<void*>(state.inventory,0x268)==state.current&&state.primaryNow==0&&SkillAlive();
    state.readinessPassedMask=ready?1:0;
    if(!ready){s.readyAtMs=0;state.readySeconds=0;}
    else{if(!s.readyAtMs)s.readyAtMs=now;state.readySeconds=double(now-s.readyAtMs)/1000.;}
    if(!ready||now-s.readyAtMs<state.marker.warmupMs){
        if(now-s.startedAtMs>=SkillPreparationMs)return SkillAbort(4977,"native_phoenix_primary_external_C_selection_not_ready");
        return Wait("native_phoenix_primary_waiting_external_Phoenix_C_slot3_RespondToEventState");
    }
    if(!SkillBind(sample)||!SkillOwnerFence()||!ForcePrimary(true))return SkillAbort(4978,"native_phoenix_primary_prepress_current_proof_changed");
    s.pressedAtMs=now;state.phase=Phase::ShootHeld;state.phaseSeconds=0;
    return Wait("native_phoenix_primary_original_press_submitted_not_cast_proof");
}
inline void WriteSkillPrimary(FILE* file,const State& all){
    const auto& s=all.skill;const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,"\"phoenix_c_primary\":{\"scope\":\"explicit_original_primary_input_only\",\"active\":%s,\"selection_bound\":%s,\"epoch\":%u,\"registration\":%llu,\"samples\":%llu,\"preparation_budget_ms\":30000,\"total_budget_ms\":45000,\"hold_ms\":500,\"release_proof_budget_ms\":10000,\"started_at_ms\":%llu,\"pressed_at_ms\":%llu,\"released_at_ms\":%llu,\"before_accepted_index_known\":%s,\"before_accepted_index\":%d,\"after_accepted_index_known\":%s,\"after_accepted_index\":%d,\"after_sampled\":%s,\"private_intent_retired\":%s,\"release_unavailable\":%s,\"private_retirement_is_native_release_proof\":false,\"item\":\"%p\",\"machine\":\"%p\",\"prepress_state\":\"%p\",\"expected_slot\":3,\"expected_class\":\"Ability_Phoenix_Q_FireballWall_Production_C\",\"expected_state\":\"RespondToEventState_ThrowProjectile\",\"automatic_skill_selection_calls\":0,\"manual_skill_tick_calls\":0,\"cast_or_resource_success_verified\":false},",
        b(all.marker.scenario==5),b(s.selectionBound),s.epoch,static_cast<unsigned long long>(s.registration),static_cast<unsigned long long>(s.samples),
        static_cast<unsigned long long>(s.startedAtMs),static_cast<unsigned long long>(s.pressedAtMs),static_cast<unsigned long long>(s.releasedAtMs),b(s.beforeInputKnown),s.beforeAccepted,b(s.afterInputKnown),s.afterAccepted,b(s.afterSampled),b(s.privateIntentRetired),b(s.releaseUnavailable),s.ability.object,s.machine.object,s.respond.object);
}
