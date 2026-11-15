#pragma once
#include "humanlike_bot_types.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace humanlike_bot::math {
inline constexpr double Pi=3.1415926535897932384626433832795;
inline bool Finite(float x){return std::isfinite(x);}
inline bool Finite(double x){return std::isfinite(x);}
inline bool Finite(Vec3 p){return Finite(p.x)&&Finite(p.y)&&Finite(p.z);}
inline bool Finite(Rotation p){return Finite(p.pitch)&&Finite(p.yaw);}
inline Vec3 Add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3 Sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec3 Mul(Vec3 a,float f){return {a.x*f,a.y*f,a.z*f};}
inline float Dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline float Length(Vec3 p){return std::sqrt(Dot(p,p));}
inline float Horizontal(Vec3 p){return std::sqrt(p.x*p.x+p.y*p.y);}
inline float Distance(Vec3 a,Vec3 b){return Length(Sub(a,b));}
inline Vec3 Unit(Vec3 p){const auto n=Length(p);return Finite(n)&&n>0?Mul(p,1/n):Vec3{};}
inline Vec3 FlatUnit(Vec3 p){p.z=0;return Unit(p);}
inline float Clamp(float value,float lo,float hi){return (std::min)((std::max)(value,lo),hi);}
inline float Yaw(float value){if(!Finite(value))return 0;value=std::remainder(value,360.0f);return value;}
inline Rotation Normalize(Rotation r){return {Clamp(r.pitch,-90,90),Yaw(r.yaw)};}
inline Rotation Look(Vec3 from,Vec3 to){const auto d=Sub(to,from);return Normalize({static_cast<float>(std::atan2(d.z,Horizontal(d))*180/Pi),static_cast<float>(std::atan2(d.y,d.x)*180/Pi)});}
inline Vec3 Forward(Rotation r){const double p=r.pitch*Pi/180,y=r.yaw*Pi/180;return {static_cast<float>(std::cos(p)*std::cos(y)),static_cast<float>(std::cos(p)*std::sin(y)),static_cast<float>(std::sin(p))};}
inline float Angle(Rotation a,Rotation b){return std::hypot(a.pitch-b.pitch,Yaw(a.yaw-b.yaw));}
inline Rotation Step(Rotation from,Rotation goal,float maximumDegrees,float smoothing=1){
    float p=goal.pitch-from.pitch,y=Yaw(goal.yaw-from.yaw);const auto length=std::hypot(p,y);
    if(length<=0||maximumDegrees<=0)return Normalize(from);
    const auto k=(std::min)(1.0f,maximumDegrees/length)*Clamp(smoothing,0,1);
    return Normalize({from.pitch+p*k,from.yaw+y*k});
}
inline bool Same(OwnLife a,OwnLife b){return a.stableBotId==b.stableBotId&&a.spawnEpoch==b.spawnEpoch&&a.token==b.token;}
inline uint64_t Mix(uint64_t value){value+=0x9e3779b97f4a7c15ULL;value=(value^(value>>30))*0xbf58476d1ce4e5b9ULL;value=(value^(value>>27))*0x94d049bb133111ebULL;return value^(value>>31);}
struct Rng {
    uint64_t state=1;
    void Seed(uint64_t seed){state=Mix(seed);}
    uint64_t Next(){state+=0x9e3779b97f4a7c15ULL;return Mix(state);}
    float Unit(){return static_cast<float>((Next()>>40)*(1.0/16777216.0));}
    float Signed(){return Unit()*2-1;}
    bool Chance(float probability){return Unit()<Clamp(probability,0,1);}
    uint32_t Inclusive(uint32_t lo,uint32_t hi){if(hi<=lo)return lo;return lo+static_cast<uint32_t>(Next()%(static_cast<uint64_t>(hi)-lo+1));}
};
} // namespace humanlike_bot::math
