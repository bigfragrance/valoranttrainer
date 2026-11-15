// Included inside local_server_native_test after its original input helpers.
// Explicit marker + matching native CLI token are the only activation path.
inline bool IsJettXTarget(){return state.marker.scenario==uint32_t(Scenario::JettXTarget);}
inline bool IsJettSkill(){return state.marker.scenario>=6&&state.marker.scenario<=10;}
inline uint32_t JettSkillIndex(){return IsJettXTarget()?3:state.marker.scenario-6;}
inline bool JettQueryActivation(){
    auto& j=state.jett;struct Query{uint8_t slot,result=255;} q{j.slot};
    if(!j.methodsReady||!Event(state.pawn,j.currentActivation.object,&q)||q.result>1)return false;
    j.activationNow=q.result;++j.activationQueries;return true;
}
inline bool JettParameter(size_t index,void* function,const wchar_t* name,int32_t offset,uint64_t declaredFlags,uintptr_t enumGetter){
    auto& d=state.jett.parameterChecks[index];d={};d.checked=true;d.expectedOffset=offset;
    d.declaredFlags=declaredFlags;d.expectedFlags=local_server_native_jett_plan::LinkedByteOrEnumFlags(declaredFlags);
    d.enumGetter=enumGetter;d.enumRequired=enumGetter!=0;
    original_jett_priming::PropertyInfo p{};
    original_jett_priming::FieldOffset(function,name,1,&p);
    d.field=reinterpret_cast<uintptr_t>(p.field);d.found=p.field!=nullptr;d.generic=p.genericProperty;
    d.nameMatches=p.nameMatches;d.bounds=p.boundsValid;d.actualOffset=p.offset;
    d.actualDim=p.arrayDim;d.actualSize=p.elementSize;d.castFlags=p.castFlags;d.actualFlags=Read<uint64_t>(p.field,0x38);
    if(enumGetter)d.expectedEnum=reinterpret_cast<uintptr_t>(engine::function<void*(*)()>(enumGetter)());
    // Native Enum ctor1BED7C0 stores UEnum at +80; Byte ctor1C79DF0
    // stores its optional UEnum at +78. IndexInSlot declares nullptr.
    if(p.field)d.actualEnum=Read<uintptr_t>(p.field,enumGetter?0x80:0x78);
    d.mismatch=local_server_native_jett_plan::FieldMismatch(d);return d.mismatch==0;
}
inline bool JettFields(){
    auto& j=state.jett;j.fieldRejectMask=0;
    const std::array<bool,6> result{
        JettParameter(0,j.activation.object,L"ItemSlot",0,0x10000000000080ULL,0x64a7370),
        JettParameter(1,j.activation.object,L"InputState",1,0x10000000000080ULL,0x6493750),
        JettParameter(2,j.activation.object,L"ActivationType",2,0x10000000000080ULL,0x646e9a0),
        JettParameter(3,j.activation.object,L"IndexInSlot",3,0x10000000000082ULL,0),
        JettParameter(4,j.currentActivation.object,L"ItemSlot",0,0x10000000000080ULL,0x64a7370),
        JettParameter(5,j.currentActivation.object,L"ReturnValue",1,0x10000000000580ULL,0x6493750)};
    for(size_t n=0;n<result.size();++n)if(!result[n])j.fieldRejectMask|=uint32_t(1)<<n;
    j.fieldsVerified=j.fieldRejectMask==0;
    return j.fieldsVerified;
}
inline bool JettQueue(int32_t& count){
    auto data=Read<void*>(state.input,0x8b0);count=Read<int32_t>(state.input,0x8b8);
    const auto capacity=Read<int32_t>(state.input,0x8bc);
    return count>=0&&count<=8192&&capacity>=count
        &&(!count||original_players::Readable(data,size_t(count)*40));
}
inline bool JettQueuedActivation(int32_t before,int32_t after,uint8_t slot,bool held){
    if(after!=before+1)return false;
    auto data=Read<const uint8_t*>(state.input,0x8b0);if(!data)return false;
    const auto command=data+size_t(before)*40;
    // Native 70A4D30 creates this exact command. These are observations of its
    // actual queue, never a synthesized command or a success substitute.
    return command[0]==1&&(command[0x15]&0x1f)==slot&&((command[0x15]>>5)&1)==uint8_t(held?1:0)
        &&(command[0x14]&7)==0&&((command[0x14]>>3)&7)==0;
}
inline bool JettActivate(bool held){
    auto& j=state.jett;if(!SkillOwnerFence()||!j.bound||!j.methodsReady)return false;
    if(Resolve(j.activation.identity)!=j.activation.object||Resolve(j.currentActivation.identity)!=j.currentActivation.object)return false;
    struct Input{uint8_t slot,value,type,index;} input{j.slot,uint8_t(held?1:0),0,0};
    static_assert(sizeof(Input)==4&&offsetof(Input,slot)==0&&offsetof(Input,value)==1
        &&offsetof(Input,type)==2&&offsetof(Input,index)==3);
    int32_t queueBefore=-1,queueAfter=-1;const bool beforeKnown=JettQueue(queueBefore);
    state.operation=held?"original_jett_activation_press":"original_jett_activation_release";
    // Set intent before dispatch so an exceptional return can still release it.
    j.activationHeld=held;++j.activationCalls;
    const bool dispatched=Event(state.pawn,j.activation.object,&input);
    state.operation="none";
    const bool afterKnown=JettQueue(queueAfter);
    const bool query=dispatched&&JettQueryActivation();
    const bool queued=beforeKnown&&afterKnown&&JettQueuedActivation(queueBefore,queueAfter,j.slot,held);
    if(held){j.pressQueueKnown=beforeKnown&&afterKnown;j.pressQueueBefore=queueBefore;j.pressQueueAfter=queueAfter;
        j.pressQueuedMatches=queued;j.pressImmediate=query?j.activationNow:255;}
    else{j.releaseQueueKnown=beforeKnown&&afterKnown;j.releaseQueueBefore=queueBefore;j.releaseQueueAfter=queueAfter;
        j.releaseQueuedMatches=queued;j.releaseImmediate=query?j.activationNow:255;}
    if(!dispatched||!query)return false;
    local_server_native_jett_plan::Submitted(j.pending,false,held,state.ticks,GetTickCount64());
    return true;
}
inline bool JettCleanup(){
    auto& j=state.jett;bool okay=true;
    if(!SkillOwnerFence()){
        if(j.activationHeld||state.primaryHeldRequested){j.privateIntentRetired=true;j.releaseUnavailable=true;}
        j.activationHeld=false;state.primaryHeldRequested=false;return true;
    }
    if(j.activationHeld&&!JettActivate(false)){j.releaseUnavailable=true;okay=false;}
    if(state.primaryHeldRequested&&!ForcePrimary(false)){j.releaseUnavailable=true;okay=false;}
    // Cleanup dispatch is not a consumed release. A terminal fixture cannot
    // prove the original future consumer ran, so leave that distinction clear.
    if(j.pending.pending)j.releaseUnavailable=true;
    return okay;
}
inline bool JettAbort(uint32_t error,const char* stage){JettCleanup();return Fail(error,stage);}
inline bool JettMethods(){
    auto& j=state.jett;if(j.methodsReady)return Resolve(j.activation.identity)==j.activation.object
        &&Resolve(j.currentActivation.identity)==j.currentActivation.object;
    for(const auto& profile:JettProfiles)if(!ProfileMatches(profile))return JettAbort(4980,"native_jett_original_activation_profile_changed");
    if(!Function(j.activation.object,L"/Script/ShooterGame.ShooterCharacter.ForceActivationInputState",4,0x04020401,0x6c6b5e0)
       ||!Function(j.currentActivation.object,L"/Script/ShooterGame.ShooterCharacter.GetCurrentActivationInputState",2,0x54020401,0x6c6a350)
       ||Read<void*>(j.activation.object,0x28)!=engine::Class(original_players::ShooterCharacterClass)
       ||Read<void*>(j.currentActivation.object,0x28)!=engine::Class(original_players::ShooterCharacterClass)
       ||!Identity(j.activation.object,j.activation.identity)||!Identity(j.currentActivation.object,j.currentActivation.identity)
       ||!JettFields()||!SkillMethods())return JettAbort(4980,"native_jett_original_input_metadata_unavailable");
    j.methodsReady=true;return true;
}
inline bool JettSample(){
    auto& j=state.jett;auto p=local_server_abilities::Find(state.pawn);
    if(!p||p->authority||!p->used||!p->ready||!p->sourceEpoch||!p->registration
       ||p->controller!=state.pc||p->playerState!=state.ps||p->inventory!=state.inventory
       ||!local_server_abilities::Source(*p))return false;
    const auto index=JettSkillIndex();auto& ability=p->skills[index];
    if(!ability.ready||ability.role!=2||ability.slotType>=17
       ||!local_server_abilities::Live(ability.actor,ability.identity)
       ||Read<void*>(ability.actor,0x210)!=state.pawn||Read<void*>(ability.actor,0x4a8)!=state.pawn
       ||Read<void*>(state.inventory,0x1b8+size_t(ability.slotType)*8)!=ability.slot
       ||Read<void*>(ability.slot,0x38)!=ability.actor||Read<void*>(ability.slot,0x40)!=state.inventory)return false;
    if(j.bound&&(j.epoch!=p->sourceEpoch||j.registration!=p->registration||j.ability.object!=ability.actor
       ||Resolve(j.ability.identity)!=ability.actor||j.slot!=ability.slotType))return false;
    if(!j.bound){
        j.ability.object=ability.actor;if(!Identity(ability.actor,j.ability.identity))return false;
        j.epoch=p->sourceEpoch;j.registration=p->registration;j.slot=ability.slotType;j.bound=true;
        j.plan.skill=static_cast<local_server_native_jett_plan::Skill>(index);j.chargeBefore=ability.charges;
        j.transitionsBefore=ability.transitions;
    }
    j.chargeAfter=ability.charges;j.transitionsAfter=ability.transitions;j.primed=false;
    if(index==2){
        auto priming=local_server_abilities::ObjectField(ability.actor,L"StateMachine_Priming");
        auto primed=local_server_abilities::ObjectField(ability.actor,L"Prime_Primed");
        // Role 2 selects the actual native ClientPredictedCurrentState.
        j.primed=p->primingReady&&priming&&primed&&Read<void*>(priming,0xc8)==ability.actor
            &&Read<void*>(primed,0xc8)==ability.actor&&Read<void*>(primed,0xe0)==priming
            &&Read<uint8_t>(priming,0x348)&&Read<void*>(priming,0x1b0)==primed;
    }
    j.equipped=Read<void*>(state.inventory,0x280)==ability.actor&&Read<void*>(state.inventory,0x268)==ability.actor;
    j.primedObserved=j.primedObserved||j.primed;j.equippedObserved=j.equippedObserved||j.equipped;
    ++j.samples;return true;
}
// This original static query is required only by the separate opt-in scenario.
// Generated storage24 includes padding; Bool setter1A9EC90 writes Return@16,
// so the logical native UFunction ParmsSize is17. No team or property flags change.
inline bool JettTargetEnemy(void* pawn){
    auto& t=state.jettTarget;if(!IsJettXTarget()||!SkillOwnerFence())return false;
    if(!t.metadataVerified){
        constexpr Profile profiles[]{{0x65a9a20,400,0x74c938fcc4e0c808ULL},{0x65a0b40,174,0xf6bf30faee074ad0ULL}};
        for(const auto& profile:profiles)if(!ProfileMatches(profile)){
            state.profileFailureRva=profile.rva;return Fail(4990,"native_jett_target_original_enemy_profile_changed");}
        if(!Function(t.enemyFunction.object,L"/Script/ShooterGame.BaseTeamComponent.IsEnemy",17,0x14022401,0x65a9a20)
           ||Read<void*>(t.enemyFunction.object,0x28)!=engine::Class(0x65a0b40)
           ||!Identity(t.enemyFunction.object,t.enemyFunction.identity))return Fail(4990,"native_jett_target_original_enemy_metadata_unavailable");
        t.teamDefault.object=engine::DefaultObject(engine::Class(0x65a0b40));
        if(!original_players::IsA(t.teamDefault.object,0x65a0b40)||!Identity(t.teamDefault.object,t.teamDefault.identity))
            return Fail(4990,"native_jett_target_original_enemy_default_unavailable");
        constexpr const wchar_t* names[]{L"SourceObject",L"TargetObject",L"ReturnValue"};
        constexpr int32_t offsets[]{0,8,16},sizes[]{8,8,1};t.fieldRejectMask=0;
        for(size_t n=0;n<t.enemyFields.size();++n){auto& p=t.enemyFields[n];
            const auto offset=original_jett_priming::FieldOffset(t.enemyFunction.object,names[n],static_cast<size_t>(sizes[n]),&p);
            const bool shape=offset==offsets[n]&&p.arrayDim==1&&p.elementSize==sizes[n]
                &&(n!=2||(p.castFlags==0x28001&&p.boolFieldSize==1&&p.boolByteOffset==0&&p.boolByteMask==1&&p.boolFieldMask==255));
            if(!shape)t.fieldRejectMask|=uint32_t(1)<<n;
        }
        if(t.fieldRejectMask)return Fail(4990,"native_jett_target_original_enemy_parameter_shape_changed");
        t.metadataVerified=true;
    }
    if(Resolve(t.enemyFunction.identity)!=t.enemyFunction.object||Resolve(t.teamDefault.identity)!=t.teamDefault.object)
        return Fail(4990,"native_jett_target_original_enemy_identity_changed");
    struct Query{void* source;void* target;uint8_t result=255,padding[7]{};} query{state.pawn,pawn};
    static_assert(sizeof(Query)==24&&offsetof(Query,target)==8&&offsetof(Query,result)==16);
    if(!Event(t.teamDefault.object,t.enemyFunction.object,&query)||query.result>1)
        return Fail(4990,"native_jett_target_original_enemy_query_failed");
    ++t.enemyQueries;t.lastEnemyValue=query.result;if(!query.result)++t.enemyRejected;return query.result==1;
}
// Explicit X-only binding, independent from BattleMethods/BattleSchema.
// It never calls a null BattleFunction(BoneIndex), evaluates a new pose, or
// substitutes ActorLocation plus an invented head-height offset.
inline bool JettTargetHeadMethods(){
    auto& h=state.jettHead;if(!IsJettXTarget()||!SkillOwnerFence())return false;
    if(h.metadataVerified){
        if(Resolve(h.boneIndex.identity)==h.boneIndex.object)return true;
        h.method.weakVerified=false;h.method.failureMask|=32;return Fail(4992,"native_jett_head_original_bone_index_identity_changed");
    }
    constexpr Profile profiles[]{{0x3ea8d10,339,0xeacecc50ad5b87adULL},{0x3ea3e80,180,0xce6ea80a609ee232ULL}};
    for(const auto& p:profiles)if(!ProfileMatches(p)){
        state.profileFailureRva=p.rva;return Fail(4992,"native_jett_head_original_profile_changed");}
    h.profilesVerified=true;auto& m=h.method;m={};m.checked=true;m.expectedBytes=16;
    m.expectedFlags=0x54020403;m.expectedExec=0x3ea8d10;m.ownerGetter=0x3ea3e80;
    m.function=engine::Find(L"/Script/Engine.SkinnedMeshComponent.GetBoneIndex");m.found=m.function!=nullptr;
    m.actualBytes=Read<uint16_t>(m.function,0xbe);m.actualFlags=Read<uint32_t>(m.function,0xb8);
    const auto exec=Read<uintptr_t>(m.function,0xe0);m.actualExec=exec>=engine::image?exec-engine::image:exec;
    m.actualOwner=Read<void*>(m.function,0x28);m.expectedOwner=m.found?engine::Class(m.ownerGetter):nullptr;
    if(!m.found)m.failureMask|=1;if(m.actualBytes!=m.expectedBytes)m.failureMask|=2;
    if(m.actualFlags!=m.expectedFlags)m.failureMask|=4;if(!m.expectedOwner||m.actualOwner!=m.expectedOwner)m.failureMask|=8;
    if(exec!=engine::image+m.expectedExec)m.failureMask|=16;
    if(m.failureMask)return Fail(4992,"native_jett_head_original_bone_index_metadata_changed");
    h.boneIndex.object=m.function;m.weakVerified=Identity(m.function,h.boneIndex.identity);
    if(!m.weakVerified){m.failureMask|=32;return Fail(4992,"native_jett_head_original_bone_index_identity_unavailable");}
    constexpr const wchar_t* names[]{L"BoneName",L"ReturnValue"};constexpr int32_t offsets[]{0,12},sizes[]{12,4};
    for(size_t n=0;n<h.parameters.size();++n){auto& p=h.parameters[n];
        if(original_jett_priming::FieldOffset(m.function,names[n],static_cast<size_t>(sizes[n]),&p)!=offsets[n]
           ||p.arrayDim!=1||p.elementSize!=sizes[n])h.fieldRejectMask|=uint32_t(1)<<n;
    }
    if(original_jett_priming::FieldOffset(engine::Class(original_players::ShooterCharacterClass),L"HeadDamageBoneNames",16,&h.namesField)!=0x11a8
       ||h.namesField.arrayDim!=1||h.namesField.elementSize!=16)h.fieldRejectMask|=4;
    if(h.fieldRejectMask)return Fail(4992,"native_jett_head_original_bone_parameter_or_names_schema_changed");
    h.metadataVerified=true;return true;
}
inline bool JettTargetTransformHead(const engine::Transform& bone,const engine::Transform& world,engine::Vec& result){
    const double qx=world.rotation[0],qy=world.rotation[1],qz=world.rotation[2],qw=world.rotation[3];
    const double norm=qx*qx+qy*qy+qz*qz+qw*qw;
    if(!std::isfinite(norm)||std::abs(norm-1.)>.01||!original_players::Finite(world.translation)
       ||!original_players::Finite(world.scale)||!original_players::Finite(bone.translation)
       ||std::abs(world.scale.x)<.01||std::abs(world.scale.y)<.01||std::abs(world.scale.z)<.01
       ||std::abs(world.scale.x)>100||std::abs(world.scale.y)>100||std::abs(world.scale.z)>100)return false;
    const engine::Vec v{bone.translation.x*world.scale.x,bone.translation.y*world.scale.y,bone.translation.z*world.scale.z};
    const engine::Vec t{2.*(qy*v.z-qz*v.y),2.*(qz*v.x-qx*v.z),2.*(qx*v.y-qy*v.x)};
    const engine::Vec value{world.translation.x+v.x+qw*t.x+qy*t.z-qz*t.y,
        world.translation.y+v.y+qw*t.y+qz*t.x-qx*t.z,world.translation.z+v.z+qw*t.z+qx*t.y-qy*t.x};
    if(!original_players::Finite(value))return false;result=value;return true;
}
inline bool JettTargetHeadPoint(void* pawn,engine::Vec& point){
    auto& h=state.jettHead;h.pointValid=false;h.stage="native_jett_head_waiting_actual_completed_pose";++h.queries;
    if(!JettTargetHeadMethods())return false;
    Bound pawnId{pawn};if(!Identity(pawn,pawnId.identity))return false;
    auto mesh=Read<void*>(pawn,0x4f0);Bound meshId{mesh},assetId{};
    if(!original_players::IsA(mesh,0x385f750)||Read<void*>(mesh,0xc8)!=pawn||!Identity(mesh,meshId.identity))return false;
    auto asset=Read<void*>(mesh,0x710);assetId.object=asset;if(!asset||!Identity(asset,assetId.identity))return false;
    h.pawn=pawnId;h.mesh=meshId;h.asset=assetId;
    if(Resolve(Read<Weak>(mesh,0x718))){++h.leaderPending;h.stage="native_jett_head_real_leader_mapping_pending";return false;}
    struct Names{BattleName* data=nullptr;int32_t count=0,capacity=0;};const auto names=Read<Names>(pawn,0x11a8);
    if(!names.data||names.count<1||names.count>32||names.capacity<names.count||names.capacity>128
       ||!original_players::Readable(names.data,static_cast<size_t>(names.count)*sizeof(BattleName)))return false;
    for(int32_t n=0;n<names.count;++n){
        struct Index{BattleName name{};int32_t value=-1;} query{names.data[n],-1};static_assert(sizeof(Index)==16&&offsetof(Index,value)==12);
        if(!query.name.words[0]||!Event(mesh,h.boneIndex.object,&query)||query.value<0)continue;
        const auto frame=Read<uint64_t>(mesh,0xd78);const auto buffer=Read<int32_t>(mesh,0x7a8);
        if(!frame||buffer<0||buffer>1)continue;
        const size_t at=0x760+static_cast<size_t>(buffer)*16;auto data=Read<void*>(mesh,at);
        const auto count=Read<int32_t>(mesh,at+8),capacity=Read<int32_t>(mesh,at+12);
        if(!data||count<1||count>4096||capacity<count||capacity>8192||query.value>=count)continue;
        engine::Transform bone{},world{},after{};engine::Vec result{};
        if(!original_players::Read(data,static_cast<size_t>(query.value)*sizeof(bone),bone)
           ||!original_players::Read(mesh,0x300,world)||!JettTargetTransformHead(bone,world,result))continue;
        if(Resolve(pawnId.identity)!=pawn||Read<void*>(pawn,0x4f0)!=mesh||Read<void*>(mesh,0xc8)!=pawn
           ||Resolve(meshId.identity)!=mesh||Resolve(assetId.identity)!=asset||Read<void*>(mesh,0x710)!=asset
           ||Read<uint64_t>(mesh,0xd78)!=frame||Read<int32_t>(mesh,0x7a8)!=buffer||Read<void*>(mesh,at)!=data
           ||Read<int32_t>(mesh,at+8)!=count||!original_players::Read(mesh,0x300,after)
           ||std::memcmp(&world,&after,sizeof(world))!=0||Resolve(Read<Weak>(mesh,0x718)))continue;
        point=result;h.headPoint=result;h.componentBone=bone.translation;h.poseFrame=frame;h.poseBuffer=buffer;h.poseCount=count;
        h.bone=query.value;h.boneName=query.name;h.pointValid=true;++h.proved;h.stage="native_jett_head_actual_damage_bone_world_point_observed";return true;
    }
    ++h.posePending;return false;
}
inline bool TickJettSkill(void* world,void* pc,void* ps,void* pawn,float){
    namespace plan=local_server_native_jett_plan;auto& j=state.jett;const auto now=GetTickCount64();
    if(!j.startedAt){j.startedAt=now;j.plan.started=now;j.plan.last=now;j.plan.warmupMs=state.marker.warmupMs;}
    if(!ValidateOwner(world,pc,ps,pawn))return JettAbort(state.error,state.stage);
    if(!state.bound||state.ownershipPassedMask!=0x1fff||!SkillOwnerFence()){
        if(j.bound||j.activationHeld||state.primaryHeldRequested)return JettAbort(4981,"native_jett_current_owner_lost");
        if(now-j.startedAt>=plan::Plan::PrepareMs)return JettAbort(4982,"native_jett_owner_preparation_timeout");
        return Wait("native_jett_waiting_actual_role2_owner");
    }
    if(!ResolveMethods()||!JettMethods())return JettAbort(state.error?state.error:4980,state.stage);
    if(state.stopRequested)return JettAbort(4912,"native_jett_explicit_stop_released_or_retired");
    if(now<j.startedAt||now-j.startedAt>=plan::Plan::TotalMs)return JettAbort(4985,"native_jett_total_input_budget_expired");
    const bool sampled=JettSample();
    if(j.bound&&(!sampled||!SkillAlive()))return JettAbort(4983,"native_jett_actual_life_or_registered_source_changed");
    if(!sampled){
        if(now-j.startedAt>=plan::Plan::PrepareMs)return JettAbort(4982,"native_jett_ready_graph_preparation_timeout");
        return Wait("native_jett_waiting_original_replicated_skill_graph");
    }
    if(!JettQueryActivation()||!QueryPrimary()||state.primaryNow>1)return JettAbort(4984,"native_jett_original_input_query_unavailable");
    if(j.pending.pending){
        const bool primary=j.pending.primary,held=j.pending.held;++j.pendingSamples;
        const auto receipt=plan::Observe(j.pending,primary?state.primaryNow:j.activationNow,state.ticks,now);
        if(receipt==plan::Readback::Expired)return JettAbort(4986,"native_jett_original_input_readback_timeout");
        if(receipt!=plan::Readback::Confirmed)return Wait(primary?"native_jett_waiting_original_primary_readback"
            :"native_jett_waiting_original_activation_readback");
        if(primary){j.primaryReadbackTick=state.ticks;
            if(held){state.primaryHeldObserved=true;j.primaryPressObserved=true;}
            else{state.releaseObserved=true;j.primaryReleaseObserved=true;}}
        else{j.activationReadbackTick=state.ticks;
            if(held)++j.activationPressReadbacks;else ++j.activationReleaseReadbacks;}
        // Hold duration starts at an observed native press, rather than its
        // earlier queue submission. Release is never sent before this receipt.
        if(held)j.plan.phaseAt=now;
    }
    bool primaryContinue=true;
    if(IsJettXTarget()){
        auto& t=state.jettTarget;
        const bool acquiring=j.plan.phase==plan::Phase::WaitEquipped&&j.equipped;
        const bool holding=j.plan.phase==plan::Phase::PrimaryHeld;
        if(acquiring||holding){
            const auto readbacks=state.targetAimReadbacks;const auto aim=AimTarget();++t.aimSamples;t.lastAimResult=uint8_t(aim);
            if(aim==AimResult::Failure||state.error)return JettAbort(state.error?state.error:4991,"native_jett_target_original_aim_failed");
            if(acquiring){
                // Cached 10ms aim readiness cannot authorize this press. The
                // original controller readback must have succeeded in this GT sample.
                if(aim!=AimResult::Ready||state.targetAimReadbacks==readbacks)
                    return Wait("native_jett_target_waiting_visible_enemy_and_fresh_native_rotation_readback");
                t.readyAtTick=state.ticks;
            }else if(aim!=AimResult::Ready){
                // Keep the same body during the burst; use the existing original
                // release/receipt plan when it dies, retires or becomes obstructed.
                primaryContinue=false;t.holdLost=true;t.deadRelease=aim==AimResult::Dead;
            }
        }
    }
    const plan::Observation o{now,true,j.activationNow==0,j.primed,j.equipped,state.primaryNow==0,primaryContinue};
    const auto action=plan::Next(j.plan,o);
    bool okay=true;
    switch(action){
    case plan::Action::ActivationPress:okay=JettActivate(true);break;
    case plan::Action::ActivationRelease:okay=JettActivate(false);break;
    case plan::Action::PrimaryPress:
        if(IsJettXTarget()){
            auto& t=state.jettTarget;
            const bool exact=t.readyAtTick==state.ticks&&state.targetSelected&&state.targetVisible&&state.targetAlive
                &&Resolve(state.targetPawnId)==state.targetPawn&&Resolve(state.targetPSId)==state.targetPS
                &&Resolve(state.targetGSId)==state.targetGS&&JettTargetEnemy(state.targetPawn);
            if(!exact)return JettAbort(state.error?state.error:4991,"native_jett_target_prepress_enemy_source_changed");
            t.prePrimaryReady=true;t.enemyAtPress=true;t.primaryAtTick=state.ticks;t.readbacksAtPress=state.targetAimReadbacks;
            t.pawnAtPress={state.targetPawn,state.targetPawnId};t.psAtPress={state.targetPS,state.targetPSId};t.gsAtPress={state.targetGS,state.targetGSId};
        }
        okay=j.equipped&&SkillOwnerFence()&&ForcePrimary(true);
        if(okay)plan::Submitted(j.pending,true,true,state.ticks,now);break;
    case plan::Action::PrimaryRelease:
        okay=SkillOwnerFence()&&ForcePrimary(false);
        if(okay)plan::Submitted(j.pending,true,false,state.ticks,now);break;
    case plan::Action::Abort:return JettAbort(4985,"native_jett_bounded_input_plan_timed_out");
    case plan::Action::Complete:
        j.inputComplete=true;Complete();state.stage="native_jett_input_complete_native_skill_results_unverified";return true;
    default:break;
    }
    if(!okay)return JettAbort(4986,"native_jett_original_input_dispatch_or_readback_failed");
    state.phase=Phase::Warming;return Wait("native_jett_original_input_plan_observing_native_state");
}
inline void WriteJettFieldChecks(FILE* file,const State& all){
    const auto& j=all.jett;const auto b=[](bool v){return v?"true":"false";};
    constexpr const char* names[]{"ForceActivationInputState.ItemSlot","ForceActivationInputState.InputState",
        "ForceActivationInputState.ActivationType","ForceActivationInputState.IndexInSlot",
        "GetCurrentActivationInputState.ItemSlot","GetCurrentActivationInputState.ReturnValue"};
    std::fprintf(file,"\"jett_input_fields\":{\"reject_mask\":%u,\"strict_final_flags\":true,\"reason_bits\":{\"missing\":1,\"not_property\":2,\"name\":4,\"offset\":8,\"array_dim\":16,\"element_size\":32,\"bounds\":64,\"final_flags\":128,\"enum_identity\":256},\"parameters\":[",j.fieldRejectMask);
    for(size_t n=0;n<j.parameterChecks.size();++n){const auto& p=j.parameterChecks[n];
        std::fprintf(file,"%s{\"name\":\"%s\",\"checked\":%s,\"found\":%s,\"mismatch\":%u,\"field\":\"%p\",\"expected_offset\":%d,\"actual_offset\":%d,\"expected_size\":1,\"actual_size\":%d,\"expected_array_dim\":1,\"actual_array_dim\":%d,\"declared_flags\":\"%016llx\",\"expected_final_flags\":\"%016llx\",\"actual_final_flags\":\"%016llx\",\"field_cast_flags\":\"%016llx\",\"enum_getter_rva\":%llu,\"expected_enum\":\"%p\",\"actual_enum\":\"%p\"}",
            n?",":"",names[n],b(p.checked),b(p.found),p.mismatch,reinterpret_cast<void*>(p.field),p.expectedOffset,p.actualOffset,
            p.actualSize,p.actualDim,static_cast<unsigned long long>(p.declaredFlags),static_cast<unsigned long long>(p.expectedFlags),
            static_cast<unsigned long long>(p.actualFlags),static_cast<unsigned long long>(p.castFlags),static_cast<unsigned long long>(p.enumGetter),
            reinterpret_cast<void*>(p.expectedEnum),reinterpret_cast<void*>(p.actualEnum));
    }
    std::fputs("]},",file);
}
inline void WriteJettHead(FILE* file,const State& all){
    const auto& h=all.jettHead;const auto b=[](bool v){return v?"true":"false";};const auto& m=h.method;
    std::fprintf(file,"\"jett_head_target\":{\"enabled\":%s,\"stage\":\"%s\",\"profiles_verified\":%s,\"metadata_verified\":%s,\"field_reject_mask\":%u,\"point_valid\":%s,\"queries\":%llu,\"proved\":%llu,\"leader_pending\":%llu,\"pose_pending\":%llu,\"pawn\":\"%p\",\"mesh\":\"%p\",\"asset\":\"%p\",\"bone_index\":%d,\"bone_name\":[%u,%u,%u],\"pose_frame\":%llu,\"pose_buffer\":%d,\"pose_count\":%d,\"root_observed\":%s,\"root_point\":[%.9g,%.9g,%.9g],\"head_point\":[%.9g,%.9g,%.9g],\"component_head_translation\":[%.9g,%.9g,%.9g],\"bone_method\":{\"checked\":%s,\"found\":%s,\"weak_verified\":%s,\"failure_mask\":%u,\"expected_bytes\":%u,\"actual_bytes\":%u,\"expected_flags\":%u,\"actual_flags\":%u,\"expected_exec_rva\":%llu,\"actual_exec_rva\":%llu,\"expected_owner\":\"%p\",\"actual_owner\":\"%p\"},\"parameter_fields\":[",
        b(all.marker.scenario==uint32_t(Scenario::JettXTarget)),h.stage,b(h.profilesVerified),b(h.metadataVerified),h.fieldRejectMask,b(h.pointValid),
        static_cast<unsigned long long>(h.queries),static_cast<unsigned long long>(h.proved),static_cast<unsigned long long>(h.leaderPending),static_cast<unsigned long long>(h.posePending),
        h.pawn.object,h.mesh.object,h.asset.object,h.bone,h.boneName.words[0],h.boneName.words[1],h.boneName.words[2],
        static_cast<unsigned long long>(h.poseFrame),h.poseBuffer,h.poseCount,b(h.rootObserved),h.rootPoint.x,h.rootPoint.y,h.rootPoint.z,
        h.headPoint.x,h.headPoint.y,h.headPoint.z,h.componentBone.x,h.componentBone.y,h.componentBone.z,
        b(m.checked),b(m.found),b(m.weakVerified),m.failureMask,unsigned(m.expectedBytes),unsigned(m.actualBytes),m.expectedFlags,m.actualFlags,
        static_cast<unsigned long long>(m.expectedExec),static_cast<unsigned long long>(m.actualExec),m.expectedOwner,m.actualOwner);
    const original_jett_priming::PropertyInfo* fields[]{&h.parameters[0],&h.parameters[1],&h.namesField};
    constexpr int32_t offsets[]{0,12,0x11a8},sizes[]{12,4,16};
    for(size_t n=0;n<3;++n){const auto& p=*fields[n];std::fprintf(file,"%s{\"found\":%s,\"expected_offset\":%d,\"actual_offset\":%d,\"expected_size\":%d,\"actual_size\":%d,\"array_dim\":%d,\"name_matches\":%s,\"bounds_valid\":%s}",
        n?",":"",b(p.field!=nullptr),offsets[n],p.offset,sizes[n],p.elementSize,p.arrayDim,b(p.nameMatches),b(p.boundsValid));}
    std::fprintf(file,"],\"los\":{\"attempted\":%s,\"returned\":%s,\"queries\":%llu,\"at_tick\":%llu,\"pawn\":\"%p\",\"pawn_weak\":[%d,%d],\"start\":[%.9g,%.9g,%.9g],\"end\":[%.9g,%.9g,%.9g],\"length\":%.9g,\"return_value\":%u,\"blocking\":%s,\"distance_finite\":%s,\"distance\":%.9g,\"clear\":%s,\"hit_actor_weak\":[%d,%d],\"hit_component_weak\":[%d,%d],\"native_out_hit_240b_hex\":\"",
        b(h.losAttempted),b(h.losReturned),static_cast<unsigned long long>(h.losQueries),static_cast<unsigned long long>(h.losAtTick),h.losPawn,h.losPawnId.index,h.losPawnId.serial,
        h.losStart.x,h.losStart.y,h.losStart.z,h.losEnd.x,h.losEnd.y,h.losEnd.z,h.losLength,unsigned(h.losReturnValue),b(h.losBlocking),b(h.losDistanceFinite),h.losDistance,b(h.losClear),
        h.hitActor.index,h.hitActor.serial,h.hitComponent.index,h.hitComponent.serial);
    for(auto byte:h.hitBytes)std::fprintf(file,"%02x",unsigned(byte));
    std::fputs("\"},\"pose_or_transform_writes\":0,\"pose_evaluation_calls\":0,\"native_hit_or_damage_verified\":false},",file);
}
inline void WriteJettTarget(FILE* file,const State& all){
    const auto& t=all.jettTarget;const auto b=[](bool v){return v?"true":"false";};
    std::fprintf(file,"\"jett_target\":{\"enabled\":%s,\"scope\":\"explicit_jett_x_target_original_aim_and_input_only\",\"enemy_metadata_verified\":%s,\"field_reject_mask\":%u,\"pre_primary_ready\":%s,\"enemy_at_press\":%s,\"enemy_queries\":%llu,\"enemy_rejected\":%llu,\"last_enemy_value\":%u,\"aim_samples\":%llu,\"last_aim_result\":%u,\"ready_at_tick\":%llu,\"primary_at_tick\":%llu,\"rotation_readbacks_at_press\":%llu,\"hold_lost\":%s,\"dead_release\":%s,\"pawn_at_press\":\"%p\",\"ps_at_press\":\"%p\",\"gs_at_press\":\"%p\",\"pawn_weak_at_press\":[%d,%d],\"ps_weak_at_press\":[%d,%d],\"enemy_parameter_fields\":[",
        b(all.marker.scenario==uint32_t(Scenario::JettXTarget)),b(t.metadataVerified),t.fieldRejectMask,b(t.prePrimaryReady),b(t.enemyAtPress),
        static_cast<unsigned long long>(t.enemyQueries),static_cast<unsigned long long>(t.enemyRejected),unsigned(t.lastEnemyValue),
        static_cast<unsigned long long>(t.aimSamples),unsigned(t.lastAimResult),static_cast<unsigned long long>(t.readyAtTick),
        static_cast<unsigned long long>(t.primaryAtTick),static_cast<unsigned long long>(t.readbacksAtPress),b(t.holdLost),b(t.deadRelease),
        t.pawnAtPress.object,t.psAtPress.object,t.gsAtPress.object,t.pawnAtPress.identity.index,t.pawnAtPress.identity.serial,
        t.psAtPress.identity.index,t.psAtPress.identity.serial);
    constexpr int32_t offsets[]{0,8,16},sizes[]{8,8,1};
    for(size_t n=0;n<t.enemyFields.size();++n){const auto& p=t.enemyFields[n];
        std::fprintf(file,"%s{\"found\":%s,\"expected_offset\":%d,\"actual_offset\":%d,\"expected_size\":%d,\"actual_size\":%d,\"actual_array_dim\":%d,\"name_matches\":%s,\"bounds_valid\":%s,\"bool_field_size\":%u,\"bool_byte_offset\":%u,\"bool_byte_mask\":%u,\"bool_field_mask\":%u}",
            n?",":"",b(p.field!=nullptr),offsets[n],p.offset,sizes[n],p.elementSize,p.arrayDim,b(p.nameMatches),b(p.boundsValid),
            unsigned(p.boolFieldSize),unsigned(p.boolByteOffset),unsigned(p.boolByteMask),unsigned(p.boolFieldMask));
    }
    std::fputs("],\"target_hit_verified\":false,\"damage_verified\":false,\"transform_or_health_writes\":0},",file);
}
inline void WriteJettSkill(FILE* file,const State& all){
    const auto& j=all.jett;const auto b=[](bool value){return value?"true":"false";};
    WriteJettFieldChecks(file,all);WriteJettHead(file,all);WriteJettTarget(file,all);
    std::fprintf(file,"\"jett_input_receipt\":{\"fields_verified\":%s,\"pending\":%s,\"pending_primary\":%s,\"pending_held\":%s,\"submitted_tick\":%llu,\"activation_readback_tick\":%llu,\"primary_readback_tick\":%llu,\"pending_samples\":%llu,\"readback_budget_ms\":10000,\"press_immediate_value\":%u,\"release_immediate_value\":%u,\"press_queue_known\":%s,\"press_queue_before\":%d,\"press_queue_after\":%d,\"press_queued_command_matches\":%s,\"release_queue_known\":%s,\"release_queue_before\":%d,\"release_queue_after\":%d,\"release_queued_command_matches\":%s,\"dispatch_is_consumption_proof\":false},",
        b(j.fieldsVerified),b(j.pending.pending),b(j.pending.primary),b(j.pending.held),static_cast<unsigned long long>(j.pending.tick),
        static_cast<unsigned long long>(j.activationReadbackTick),static_cast<unsigned long long>(j.primaryReadbackTick),
        static_cast<unsigned long long>(j.pendingSamples),unsigned(j.pressImmediate),unsigned(j.releaseImmediate),b(j.pressQueueKnown),
        j.pressQueueBefore,j.pressQueueAfter,b(j.pressQueuedMatches),b(j.releaseQueueKnown),j.releaseQueueBefore,j.releaseQueueAfter,b(j.releaseQueuedMatches));
    std::fprintf(file,"\"jett_skill\":{\"active\":%s,\"bound\":%s,\"input_complete\":%s,\"epoch\":%u,\"registration\":%llu,\"slot\":%u,\"phase\":%u,\"samples\":%llu,\"activation_calls\":%llu,\"activation_queries\":%llu,\"activation_press_readbacks\":%llu,\"activation_release_readbacks\":%llu,\"activation_now\":%u,\"activation_held\":%s,\"actual_primed\":%s,\"actual_x_equipped\":%s,\"primed_observed\":%s,\"x_equipped_observed\":%s,\"primary_press_observed\":%s,\"primary_release_observed\":%s,\"charge_before\":%d,\"charge_after\":%d,\"transitions_before\":%llu,\"transitions_after\":%llu,\"private_intent_retired\":%s,\"release_unavailable\":%s,\"physical_input_verified\":false,\"cast_verified\":false,\"server_damage_verified\":false,\"manual_skill_ticks\":0},",
        b(all.marker.scenario>=6&&all.marker.scenario<=10),b(j.bound),b(j.inputComplete),j.epoch,
        static_cast<unsigned long long>(j.registration),unsigned(j.slot),unsigned(j.plan.phase),static_cast<unsigned long long>(j.samples),
        static_cast<unsigned long long>(j.activationCalls),static_cast<unsigned long long>(j.activationQueries),
        static_cast<unsigned long long>(j.activationPressReadbacks),static_cast<unsigned long long>(j.activationReleaseReadbacks),
        unsigned(j.activationNow),b(j.activationHeld),b(j.primed),b(j.equipped),b(j.primedObserved),b(j.equippedObserved),b(j.primaryPressObserved),b(j.primaryReleaseObserved),
        j.chargeBefore,j.chargeAfter,static_cast<unsigned long long>(j.transitionsBefore),static_cast<unsigned long long>(j.transitionsAfter),
        b(j.privateIntentRetired),b(j.releaseUnavailable));
}
