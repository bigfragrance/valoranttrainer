#pragma once
#include "original_movement.h"
#include "original_simulation.h"
#include "original_shot_observer.h"

// Explicit test protocol for the wrapper-owned original player. This is not an
// OS input injector. Only stages7/9 consume commands; commands expire in .5s.
// Metrics and rays are observations of the original code, never shot generation.
namespace original_probe {
inline constexpr uint32_t Magic=0x3152504f;
inline constexpr size_t MetricsSequence=768,ShotRing=1024,ShotCapacity=32;
struct Input { bool active=false;uint32_t flags=0;float forward=0,right=0;bool jump=false,walk=false,crouch=false; };
struct Aim { bool active=false;uint32_t sequence=0;engine::Vec rotation{}; };
inline uint64_t shotCount=0;
template<class T> inline T Read(const uint8_t* p,size_t offset){T v{};std::memcpy(&v,p+offset,sizeof(v));return v;}
template<class T> inline void Write(uint8_t* p,size_t offset,const T& v){std::memcpy(p+offset,&v,sizeof(v));}
inline Input TestInput(const uint8_t* shared,uint64_t stage,double now){
    Input result;
    if(!shared||(stage!=7&&stage!=9))return result;
    const auto sequence=Read<uint32_t>(shared,20);
    if(!sequence||(sequence&1))return result;
    uint8_t copy[32]{};std::memcpy(copy,shared+20,sizeof(copy));MemoryBarrier();
    if(Read<uint32_t>(shared,20)!=sequence)return result;
    const auto flags=Read<uint32_t>(copy,4),movement=Read<uint32_t>(copy,16);
    const auto forward=Read<float>(copy,8),right=Read<float>(copy,12);
    const auto expiry=Read<double>(copy,20);
    const uint32_t allowed=stage==9?1023U:15U;
    if(flags&~allowed||movement&~7U||!std::isfinite(forward)||!std::isfinite(right)
       ||std::abs(forward)>1||std::abs(right)>1||!std::isfinite(expiry)
       ||expiry<now||expiry-now>.5)return result;
    result.active=true;result.flags=flags;result.forward=forward;result.right=right;
    result.jump=(movement&1)!=0;result.walk=(movement&2)!=0;result.crouch=(movement&4)!=0;
    return result;
}
inline Aim TestAim(const uint8_t* shared,uint64_t stage,double now){
    Aim result;
    if(!shared||stage!=9||Read<uint32_t>(shared,3584)!=0x314d4941)return result;
    const auto sequence=Read<uint32_t>(shared,3588);
    if(!sequence||(sequence&1))return result;
    uint8_t copy[24]{};std::memcpy(copy,shared+3592,sizeof(copy));MemoryBarrier();
    if(Read<uint32_t>(shared,3588)!=sequence)return result;
    const auto pitch=Read<double>(copy,0),yaw=Read<double>(copy,8),expiry=Read<double>(copy,16);
    if(!std::isfinite(pitch)||!std::isfinite(yaw)||!std::isfinite(expiry)
       ||std::abs(pitch)>89||std::abs(yaw)>36000||expiry<now||expiry-now>.5)return result;
    result.active=true;result.sequence=sequence;result.rotation={pitch,yaw,0};return result;
}
inline bool Publish(uint8_t* shared,void* pawn,void* controller,uint64_t stage,uint32_t inputs){
    if(!shared||!original_players::Thread()||!pawn||!controller)return false;
    if(!original_movement::Sample()||!original_simulation::Refresh())return false;
    original_weapon::RefreshDiagnostics();
    engine::Vec location{},velocity{},camera{},rotation{};
    auto cameraManager=original_players::Value<void*>(controller,0x528);
    if(!engine::Event(pawn,L"/Script/Engine.Actor.K2_GetActorLocation",&location)
       ||!engine::Event(pawn,L"/Script/Engine.Actor.GetVelocity",&velocity)
       ||!engine::Event(cameraManager,L"/Script/Engine.PlayerCameraManager.GetCameraLocation",&camera)
       ||!engine::Event(cameraManager,L"/Script/Engine.PlayerCameraManager.GetCameraRotation",&rotation))return false;
    const auto& m=original_movement::State();const auto& w=original_weapon::State();
    const auto& s=original_simulation::State();const auto& shots=original_shots::State();
    float pitchRecoil=w.pitch_recoil,yawRecoil=w.yaw_recoil;
    int actualAmmo=w.ammo,maximumAmmo=w.max_ammo;
    // A gun may choose a derived firing state (e.g. Outlaw hip/zoom selectors).
    // Read the state already returned by its real state machine; telemetry
    // must not mistake the first inherited component for the active one.
    auto firing=reinterpret_cast<void*>(w.original_current_state_pointer);
    if(original_players::IsA(firing,0x684d4c0)
       &&original_players::Value<void*>(firing,0xc8)==original_weapon::weapon){
        auto stability=original_players::Value<void*>(firing,0x490);
        if(original_players::IsA(stability,0x6d1e460)){
            engine::Event(stability,L"/Script/ShooterGame.StabilityComponent.GetPitchRecoil",&pitchRecoil);
            engine::Event(stability,L"/Script/ShooterGame.StabilityComponent.GetYawRecoil",&yawRecoil);
        }
        auto resource=original_players::Value<void*>(firing,0x498);
        if(original_players::IsA(resource,0x63c7e00)){
            engine::Event(resource,L"/Script/ShooterGame.AmmoComponent.GetAmmo",&actualAmmo);
            maximumAmmo=original_players::Value<int>(resource,0x130);
        }
    }
    const double now=static_cast<double>(GetTickCount64())/1000.0;
    InterlockedIncrement(reinterpret_cast<volatile LONG*>(shared+MetricsSequence));
    Write(shared,776,now);Write(shared,784,location);Write(shared,808,velocity);
    Write(shared,832,camera);Write(shared,856,rotation);
    Write(shared,880,pitchRecoil);Write(shared,884,yawRecoil);
    Write(shared,888,actualAmmo);Write(shared,892,maximumAmmo);
    Write(shared,896,s.perform_movement_calls);Write(shared,904,shots.rays);
    Write(shared,912,m.actual_capsule_half_height);Write(shared,916,m.jump_velocity);
    Write(shared,920,m.current_max_speed);Write<uint32_t>(shared,924,m.movement_type);
    Write(shared,928,s.clock_current_step);
    Write<uint32_t>(shared,932,(w.ready?1U:0U)|(m.jump_held?2U:0U)|(m.walk_held?4U:0U)
                                 |(m.crouch_held?8U:0U)|(m.movement_disabled?16U:0U));
    Write(shared,936,shots.error);Write(shared,940,inputs);
    Write(shared,944,m.original_defaults.jump.maxJumpHeight);
    Write(shared,948,m.original_defaults.jump.jumpTotalTime);
    Write(shared,952,m.original_defaults.capsuleHalfHeight);
    Write(shared,956,m.equipped_movement_ratio);
    original_shots::Shot shot;
    while(original_shots::Pop(shot)){
        Write(shared,ShotRing+(shotCount%ShotCapacity)*sizeof(shot),shot);++shotCount;
    }
    Write(shared,960,shotCount);Write<uint32_t>(shared,968,ShotCapacity);
    Write(shared,972,shots.hooks);Write(shared,976,shots.shot_callbacks);
    Write(shared,980,shots.initial_snapshots);Write(shared,984,shots.invalid_payloads);
    float fov=0;engine::Event(cameraManager,L"/Script/Engine.PlayerCameraManager.GetFOVAngle",&fov);
    Write(shared,988,fov);
    Write<uint32_t>(shared,992,(w.primary_pressed_observed?1U:0U)|(w.primary_down_observed?2U:0U)
                                  |(w.reload_pressed_observed?4U:0U));
    Write<uint32_t>(shared,996,w.input_state_primary);
    void *animation=nullptr,*montage=nullptr;
    if(original_weapon::mesh1p)engine::Event(original_weapon::mesh1p,L"/Script/Engine.SkeletalMeshComponent.GetAnimInstance",&animation);
    if(animation)engine::Event(animation,L"/Script/Engine.AnimInstance.GetCurrentActiveMontage",&montage);
    Write<uint64_t>(shared,1000,reinterpret_cast<uintptr_t>(animation));
    Write<uint64_t>(shared,1008,reinterpret_cast<uintptr_t>(montage));
    MemoryBarrier();InterlockedIncrement(reinterpret_cast<volatile LONG*>(shared+MetricsSequence));
    // Advertise this protocol only after a complete original-player sample.
    Write<uint32_t>(shared,56,static_cast<uint32_t>(stage));Write(shared,760,Magic);
    MemoryBarrier();InterlockedExchange(reinterpret_cast<volatile LONG*>(shared+12),1);
    return true;
}
}
