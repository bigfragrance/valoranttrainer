#pragma once
#include "presentation.h"
#include <array>
#include <cstddef>
#include <limits>

// Pinned-image ABI and original package types: reports/duel1-jett-abilities.json.
// Local training rules, not the missing ShooterGame authority/state machinery.
// Call every function on the owned game's game thread. OnShots follows
// presentation::Update so original skill poses take priority over rifle poses.
namespace jett {
enum Input : uint32_t { SmokeKey=64, UpdraftKey=128, DashKey=256, UltimateKey=512 };
struct Options {
    double smokeCooldown=0.8, updraftCooldown=1.0, dashCooldown=1.0;
    double smokeSeconds=4.5, smokeRadius=350, projectileSpeed=2000, projectileSeconds=1.0;
    double updraftSpeed=900, dashSpeed=2400, dashSeconds=0.25;
};
struct SkillState {
    bool ready=false, ultimate_active=false, request_burst=false, inside_smoke=false;
    uint32_t error=0, smoke_casts=0, updraft_casts=0, dash_casts=0, ultimate_casts=0;
    uint32_t knife_shots=0, knives=5, active_smokes=0, effect_failures=0;
    double smoke_ready_at=0, updraft_ready_at=0, dash_ready_at=0, weapon_blocked_until=0;
    const char* stage="abilities_not_initialized";
};
inline Options options;
inline SkillState state;
inline const SkillState& State() { return state; }
inline void *world=nullptr, *controller=nullptr, *pawn=nullptr;
inline void *smokeParticle=nullptr, *smokeProjectile=nullptr, *smokeOutro=nullptr;
inline void *updraftParticle=nullptr, *dashParticle=nullptr, *knifeBurst=nullptr;
inline void *knifeActor=nullptr, *knifeComponent=nullptr;
inline void *knifeIdle=nullptr, *knifeEquip=nullptr, *armsIdle=nullptr, *armsEquip=nullptr;
inline std::array<void*,5> armsAttack{}, knifeAttack{};
inline void *smokePose=nullptr, *updraftPose=nullptr, *dashPose=nullptr;
inline uint32_t previousInput=0, lastShots=0, lastKills=0;
inline double lastNow=-1, dashUntil=0, poseUntil=0;
inline double dashForward=1, dashRight=0;
inline bool countersInitialized=false, poseActive=false;
struct Smoke {
    void* component=nullptr;
    engine::Vec position{}, direction{};
    double born=0, expires=0;
    bool flying=false;
};
inline std::array<Smoke,8> smokes{};
struct TimedEffect { void* component=nullptr; double expires=0; };
inline std::array<TimedEffect,32> effects{};

template<class T> inline void Put(uint8_t* p,size_t offset,const T& value) { std::memcpy(p+offset,&value,sizeof(value)); }
inline bool Finite(const engine::Vec& v) { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z); }
inline bool Fail(uint32_t code,const char* stage) { state.error=code; state.stage=stage; return false; }
inline void* Load(uintptr_t getter,const wchar_t* path) { return engine::Load(engine::Class(getter),path); }
inline bool Launch(const engine::Vec& velocity,bool xyOverride,bool zOverride) {
    alignas(8) uint8_t p[0x20]{};
    Put(p,0,velocity);p[0x18]=xyOverride?1:0;p[0x19]=zOverride?1:0;
    return pawn&&Finite(velocity)&&engine::Event(pawn,L"/Script/Engine.Character.LaunchCharacter",p);
}
inline bool MoveParticle(void* component,const engine::Vec& position) {
    alignas(8) uint8_t p[0x118]{};Put(p,0,position);p[0x110]=1;
    return engine::Event(component,L"/Script/Engine.SceneComponent.K2_SetWorldLocation",p);
}
inline bool DestroyParticle(void*& component) {
    if(!component)return true;
    // Passing the component itself satisfies the native ownership check.
    void* receiver=component;
    const bool ok=engine::Event(receiver,L"/Script/Engine.ActorComponent.K2_DestroyComponent",&receiver);
    component=nullptr;return ok;
}
inline void* Emitter(void* particle,const engine::Vec& position,const engine::Vec& rotation,bool persistent=false) {
    if(!particle)return nullptr;
    alignas(8) uint8_t p[0x68]{};
    Put(p,0,pawn);Put(p,8,particle);Put(p,0x10,position);Put(p,0x28,rotation);
    const engine::Vec scale{1,1,1};Put(p,0x40,scale);
    // Explicit lifetime for every component: an original FXC may normally stop
    // a looping particle, so bAutoDestroy alone does not bound its lifetime.
    p[0x58]=0;p[0x59]=0;p[0x5a]=1; // no pool: stored pointers are never recycled
    auto receiver=engine::DefaultObject(engine::Class(0x43a9670));
    if(!engine::Event(receiver,L"/Script/Engine.GameplayStatics.SpawnEmitterAtLocation",p))return nullptr;
    void* result=nullptr;std::memcpy(&result,p+0x60,8);
    if(!result)++state.effect_failures;
    else if(!persistent) {
        TimedEffect* slot=nullptr;
        for(auto& effect:effects)if(!effect.component){slot=&effect;break;}
        if(!slot){slot=&effects[0];for(auto& effect:effects)if(effect.expires<slot->expires)slot=&effect;DestroyParticle(slot->component);}
        *slot={result,(std::max)(lastNow,0.0)+2.0};
    }
    return result;
}
inline void Pose(void* arms,void* blades,double now,bool loop=false) {
    if(presentation::ready&&arms) {
        presentation::Play(presentation::core,arms,loop);
        presentation::animationActive=false;
    }
    if(blades&&knifeComponent)presentation::Play(knifeComponent,blades,loop);
    poseActive=!loop;
    const auto length=arms?presentation::Length(arms):0.0f;
    poseUntil=now+(length>0?length:0.35);
}
inline void SetKnifeCount(uint32_t count) {
    state.knives=(std::min)(count,5u);
    for(uint32_t i=0;i<5;++i) {
        alignas(4) uint8_t p[0x10]{};
        wchar_t bone[]{L'K',L'n',L'i',L'f',L'e',static_cast<wchar_t>(L'1'+i),L'\0'};
        presentation::Name(p,bone);
        engine::Event(knifeComponent,i<5-state.knives?L"/Script/Engine.SkinnedMeshComponent.HideBoneByName":L"/Script/Engine.SkinnedMeshComponent.UnHideBoneByName",p);
    }
}
inline void SetDashInput(double forward,double right) {
    if(!std::isfinite(forward)||!std::isfinite(right))return;
    const double length=std::hypot(forward,right);
    dashForward=length>0.01?forward/length:1;dashRight=length>0.01?right/length:0;
}
inline bool Initialize(void* inWorld,void* inController,void* inPawn) {
    if(state.ready)return world==inWorld&&controller==inController&&pawn==inPawn;
    world=inWorld;controller=inController;pawn=inPawn;state.error=0;
    if(!world||!controller||!pawn||!presentation::ready)return Fail(301,"ability_player_or_presentation_missing");
    state.stage="loading_original_jett_ability_assets";
    constexpr uintptr_t particle=0x3e58380, animation=0x39e55b0;
    smokeParticle=Load(particle,L"/Game/Characters/Wushu/S0/VFX/Ability_C/Wushu_4_SmokeBall_ALLY.Wushu_4_SmokeBall_ALLY");
    smokeProjectile=Load(particle,L"/Game/Characters/Wushu/S0/VFX/Ability_C/Wushu_4_SmokeBall_Proj.Wushu_4_SmokeBall_Proj");
    smokeOutro=Load(particle,L"/Game/Characters/Wushu/S0/VFX/Ability_C/Wushu_4_SmokeBall_Outro.Wushu_4_SmokeBall_Outro");
    updraftParticle=Load(particle,L"/Game/Characters/Wushu/S0/VFX/Ability_Q/Wushu_Q_3P_PopUp.Wushu_Q_3P_PopUp");
    dashParticle=Load(particle,L"/Game/Characters/Wushu/S0/VFX/Ability_E/Wushu_E_3p_Forward.Wushu_E_3p_Forward");
    knifeBurst=Load(particle,L"/Game/Characters/Wushu/S0/VFX/Ability_X/Wushu_X_1P_DaggerBurst.Wushu_X_1P_DaggerBurst");
    armsIdle=Load(animation,L"/Game/Characters/Wushu/S0/Ability_X/1P/Anims/FP_Wushu_S0_X_Idle.FP_Wushu_S0_X_Idle");
    armsEquip=Load(animation,L"/Game/Characters/Wushu/S0/Ability_X/1P/Anims/FP_Wushu_S0_X_Equip.FP_Wushu_S0_X_Equip");
    knifeIdle=Load(animation,L"/Game/Characters/Wushu/S0/Ability_X/1P/Anims/AB_Wushu_S0_X_Idle.AB_Wushu_S0_X_Idle");
    knifeEquip=Load(animation,L"/Game/Characters/Wushu/S0/Ability_X/1P/Anims/AB_Wushu_S0_X_Equip.AB_Wushu_S0_X_Equip");
    const wchar_t* fp[]{
      L"/Game/Characters/Wushu/S0/Ability_X/1P/Anims/FP_Wushu_S0_X_Attack01.FP_Wushu_S0_X_Attack01",
      L"/Game/Characters/Wushu/S0/Ability_X/1P/Anims/FP_Wushu_S0_X_Attack02.FP_Wushu_S0_X_Attack02",
      L"/Game/Characters/Wushu/S0/Ability_X/1P/Anims/FP_Wushu_S0_X_Attack03.FP_Wushu_S0_X_Attack03",
      L"/Game/Characters/Wushu/S0/Ability_X/1P/Anims/FP_Wushu_S0_X_Attack04.FP_Wushu_S0_X_Attack04",
      L"/Game/Characters/Wushu/S0/Ability_X/1P/Anims/FP_Wushu_S0_X_Attack05.FP_Wushu_S0_X_Attack05"};
    const wchar_t* ab[]{
      L"/Game/Characters/Wushu/S0/Ability_X/1P/Anims/AB_Wushu_S0_X_Attack01.AB_Wushu_S0_X_Attack01",
      L"/Game/Characters/Wushu/S0/Ability_X/1P/Anims/AB_Wushu_S0_X_Attack02.AB_Wushu_S0_X_Attack02",
      L"/Game/Characters/Wushu/S0/Ability_X/1P/Anims/AB_Wushu_S0_X_Attack03.AB_Wushu_S0_X_Attack03",
      L"/Game/Characters/Wushu/S0/Ability_X/1P/Anims/AB_Wushu_S0_X_Attack04.AB_Wushu_S0_X_Attack04",
      L"/Game/Characters/Wushu/S0/Ability_X/1P/Anims/AB_Wushu_S0_X_Attack05.AB_Wushu_S0_X_Attack05"};
    for(size_t i=0;i<5;++i){armsAttack[i]=Load(animation,fp[i]);knifeAttack[i]=Load(animation,ab[i]);if(!armsAttack[i]||!knifeAttack[i])return Fail(302,"jett_knife_attack_animation_missing");}
    smokePose=Load(animation,L"/Game/Characters/Wushu/S0/Ability_4/1P/Anims/FP_Wushu_S0_4_Smoke_Grenade_intro.FP_Wushu_S0_4_Smoke_Grenade_Intro");
    updraftPose=Load(animation,L"/Game/Characters/Wushu/S0/Ability_Q/1P/Anims/FP_Wushu_S0_Q_CycloneBoost_Activate.FP_Wushu_S0_Q_CycloneBoost_Activate");
    dashPose=Load(animation,L"/Game/Characters/Wushu/S0/Ability_E/1P/Anims/FP_Wushu_S0_E_DashN.FP_Wushu_S0_E_DashN");
    auto mesh=Load(0x3815ac0,L"/Game/Characters/Wushu/S0/Ability_X/1P/Models/AB_Wushu_S0_X_Skelmesh.AB_Wushu_S0_X_Skelmesh");
    if(!smokeParticle||!smokeProjectile||!smokeOutro||!updraftParticle||!dashParticle||!knifeBurst||!mesh||!armsIdle||!armsEquip||!knifeIdle||!knifeEquip||!smokePose||!updraftPose||!dashPose)return Fail(303,"jett_ability_asset_missing");
    knifeActor=engine::Spawn(world,{0,0,0},pawn);if(!knifeActor)return Fail(304,"jett_knife_actor_failed");
    knifeComponent=engine::AddSkeletal(knifeActor);
    if(!knifeComponent||!engine::SetSkeletal(knifeComponent,mesh)||!engine::DisableCollision(knifeComponent)||!presentation::SingleNode(knifeComponent)||!presentation::Play(knifeComponent,knifeIdle,true))return Fail(305,"jett_knife_component_failed");
    *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(knifeComponent)+0x92c)=0;
    engine::Hidden(knifeActor,true);SetKnifeCount(5);
    state.ready=true;state.stage="jett_local_abilities_ready";return true;
}
inline void SetUltimate(bool active,double now) {
    if(!state.ready)return;
    state.ultimate_active=active;engine::Hidden(knifeActor,!active);engine::Hidden(presentation::gunActor,active);
    if(active){++state.ultimate_casts;SetKnifeCount(5);Pose(armsEquip,knifeEquip,now);state.weapon_blocked_until=now+0.65;}
    else Pose(presentation::idle,nullptr,now,true);
}
inline void RetireSmoke(Smoke& smoke,bool outro) {
    if(smoke.component&&outro&&!smoke.flying)Emitter(smokeOutro,smoke.position,{0,0,0});
    if(!DestroyParticle(smoke.component))++state.effect_failures;
    smoke={};
}
inline bool CastSmoke(double now,const engine::Vec& camera,const engine::Vec& rotation) {
    Smoke* slot=nullptr;
    for(auto& s:smokes)if(!s.component){slot=&s;break;}
    if(!slot){slot=&smokes[0];for(auto& s:smokes)if(s.born<slot->born)slot=&s;RetireSmoke(*slot,true);}
    const auto direction=engine::Forward(rotation);
    // Start at the camera and trace every advance, including the first frame.
    auto component=Emitter(smokeProjectile,camera,rotation,true);if(!component)return false;
    *slot={component,camera,direction,now,now+options.projectileSeconds,true};
    ++state.smoke_casts;state.smoke_ready_at=now+options.smokeCooldown;
    Pose(smokePose,nullptr,now);state.weapon_blocked_until=now+0.25;return true;
}
inline double DistanceSquared(const engine::Vec& a,const engine::Vec& b) { const auto d=engine::Add(a,engine::Mul(b,-1));return d.x*d.x+d.y*d.y+d.z*d.z; }
inline bool SegmentObscured(const engine::Vec& start,const engine::Vec& end) {
    const auto delta=engine::Add(end,engine::Mul(start,-1));const double d2=DistanceSquared(end,start);
    for(const auto& s:smokes)if(s.component&&!s.flying) {
        const auto q=engine::Add(s.position,engine::Mul(start,-1));
        const double t=d2>0?(std::max)(0.0,(std::min)(1.0,(q.x*delta.x+q.y*delta.y+q.z*delta.z)/d2)):0;
        if(DistanceSquared(s.position,engine::Add(start,engine::Mul(delta,t)))<options.smokeRadius*options.smokeRadius)return true;
    }
    return false;
}
inline void Update(double now,const engine::Vec& camera,const engine::Vec& rotation,uint32_t input) {
    if(!state.ready||!std::isfinite(now)||!Finite(camera)||!Finite(rotation))return;
    const double dt=lastNow<0?0:(std::max)(0.0,(std::min)(now-lastNow,0.05));lastNow=now;
    for(auto& effect:effects)if(effect.component&&now>=effect.expires)DestroyParticle(effect.component);
    const uint32_t pressed=input&~previousInput;previousInput=input;
    state.request_burst=false;
    if(dashUntil>0&&now>=dashUntil){Launch({0,0,0},true,false);dashUntil=0;}
    state.active_smokes=0;state.inside_smoke=false;
    for(auto& s:smokes)if(s.component) {
        if(s.flying) {
            if(input&SmokeKey)s.direction=engine::Forward(rotation);
            const double step=options.projectileSpeed*dt;double hitDistance=0;
            const int hit=engine::Trace(pawn,s.position,s.direction,step+5,hitDistance);
            if(hit<0){RetireSmoke(s,false);++state.effect_failures;continue;}
            const double travel=hit==1?(std::max)(0.0,(std::min)(step,hitDistance-5)):step;
            s.position=engine::Add(s.position,engine::Mul(s.direction,travel));
            if(hit==1||now>=s.expires) {
                DestroyParticle(s.component);s.component=Emitter(smokeParticle,s.position,{0,0,0},true);s.flying=false;s.expires=now+options.smokeSeconds;
            } else if(!MoveParticle(s.component,s.position)){RetireSmoke(s,false);++state.effect_failures;continue;}
        } else if(now>=s.expires){RetireSmoke(s,true);continue;}
        if(s.component&&!s.flying){++state.active_smokes;if(DistanceSquared(camera,s.position)<options.smokeRadius*options.smokeRadius)state.inside_smoke=true;}
    }
    if(!(input&8))return; // no skills while application is unfocused
    if((pressed&UltimateKey)&&now>=state.weapon_blocked_until)SetUltimate(!state.ultimate_active,now);
    if((pressed&SmokeKey)&&now>=state.smoke_ready_at)CastSmoke(now,camera,rotation);
    if((pressed&UpdraftKey)&&now>=state.updraft_ready_at&&Launch({0,0,options.updraftSpeed},false,true)) {
        ++state.updraft_casts;state.updraft_ready_at=now+options.updraftCooldown;
        Emitter(updraftParticle,engine::Add(camera,{0,0,-150}),{0,rotation.y,0});Pose(updraftPose,nullptr,now);state.weapon_blocked_until=now+0.35;
    }
    if((pressed&DashKey)&&now>=state.dash_ready_at) {
        const auto direction=engine::Add(engine::Mul(engine::Forward({0,rotation.y,0}),dashForward),engine::Mul(engine::Right(rotation),dashRight));
        // CharacterMovement performs the real capsule collision sweep.
        if(Launch(engine::Mul(direction,options.dashSpeed),true,true)) {
            ++state.dash_casts;state.dash_ready_at=now+options.dashCooldown;dashUntil=now+options.dashSeconds;
            Emitter(dashParticle,engine::Add(camera,{0,0,-70}),{0,rotation.y,0});Pose(dashPose,nullptr,now);state.weapon_blocked_until=dashUntil;
        }
    }
    if(state.ultimate_active&&(pressed&4))state.request_burst=true;
}
inline void AlignKnife() {
    engine::Transform target,root;
    if(!presentation::Socket(presentation::core,presentation::options.weaponSocket,0,target)||!presentation::Socket(knifeComponent,L"Root",2,root))return;
    const auto r=presentation::Rotation(root);const presentation::Quat inverse{-r.x,-r.y,-r.z,r.w};
    const auto q=presentation::Multiply(presentation::Rotation(target),inverse);
    const auto location=engine::Add(target.translation,engine::Mul(presentation::Rotate(q,root.translation),-1));
    if(!engine::SetTransform(knifeActor,location,presentation::Rotator(q)))++state.effect_failures;
}
inline void OnShots(double now,uint32_t shots,uint32_t kills,const engine::Vec& camera,const engine::Vec& rotation,
                    uint32_t authoritativeKnives=std::numeric_limits<uint32_t>::max()) {
    if(!state.ready)return;
    if(!countersInitialized){lastShots=shots;lastKills=kills;countersInitialized=true;}
    if(state.ultimate_active) {
        if(shots>lastShots) {
            const uint32_t count=(std::min)(shots-lastShots,5u), index=(std::min)(5-state.knives,4u);
            Pose(armsAttack[index],knifeAttack[index],now);
            Emitter(knifeBurst,engine::Add(camera,engine::Mul(engine::Forward(rotation),55)),rotation);
            state.knife_shots+=count;SetKnifeCount(state.knives>count?state.knives-count:0);
        }
        if(kills>lastKills)SetKnifeCount(5);
        if(authoritativeKnives<=5&&authoritativeKnives!=state.knives)SetKnifeCount(authoritativeKnives);
        AlignKnife();
    }
    if(poseActive&&now>=poseUntil)Pose(state.ultimate_active?armsIdle:presentation::idle,state.ultimate_active?knifeIdle:nullptr,now,true);
    // Rifle feedback might have set its idle timer before this hook; disable it
    // only while a skill owns the arms. Keep ordinary rifle behavior otherwise.
    if(state.ultimate_active||poseActive)presentation::animationActive=false;
    lastShots=shots;lastKills=kills;
}
inline void Shutdown() {
    for(auto& s:smokes)RetireSmoke(s,false);
    for(auto& effect:effects)DestroyParticle(effect.component);
    if(knifeActor)engine::Event(knifeActor,L"/Script/Engine.Actor.K2_DestroyActor",nullptr);
    if(presentation::ready)engine::Hidden(presentation::gunActor,false);
    knifeActor=nullptr;knifeComponent=nullptr;state.ready=false;state.ultimate_active=false;
}
}
