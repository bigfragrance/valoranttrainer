#pragma once
#include "original_damage.h"

// Read the actual firing component's projectile class and its original SCS
// damage template. Evaluate the shipped curves with the shipped native curve
// evaluator and native regional-damage helper. No hard-coded AK damage table.
// This supplies the local authority amount; no projectile/shot is spawned here.
namespace original_damage_tuning {
inline constexpr uintptr_t ProjectileClass=0x644d990,DamageComponentClass=0x6708820;
struct Calculation {
    bool valid=false,originalRegionVerified=false;
    void* damageType=nullptr;
    uint8_t region=0;
    double distance=0;
    float baseDamage=0,regionalMultiplier=0,attributeMultiplier=0,totalDamage=0;
    float originalCurveMaximum=0,falloffMultiplier=0;
    bool nativeFalloffFeature=false;
};
struct Diagnostics {
    bool ready=false,realFiringClass=false,templateFound=false,strongClassReference=false;
    bool restartRequired=false;
    unsigned error=0,calculations=0;
    uintptr_t exceptionRva=0;
    const char* stage="original_damage_tuning_not_initialized";
    void *firing=nullptr,*projectileClass=nullptr,*templateClass=nullptr,*damageTemplate=nullptr,*damageType=nullptr;
    Calculation last;
};
inline Diagnostics diagnostics;
inline DWORD gameThread=0;
inline const Diagnostics& State(){return diagnostics;}
template<class T> inline T Read(const void* pointer,size_t offset){return original_players::Value<T>(pointer,offset);}
inline bool Fail(unsigned error,const char* stage){diagnostics.error=error;diagnostics.stage=stage;return false;}
inline bool Thread(){return gameThread==GetCurrentThreadId()&&!diagnostics.restartRequired;}
inline LONG Capture(EXCEPTION_POINTERS* e){
    diagnostics.error=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0001170U;
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=at>=engine::image?at-engine::image:at;
    diagnostics.restartRequired=true;diagnostics.stage="original_damage_tuning_native_exception_requires_new_session";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline void* RuntimeRichCurve(void* runtimeCurve){
    if(!original_players::Readable(runtimeCurve,0x88))return nullptr;
    auto external=Read<void*>(runtimeCurve,0x80);
    if(external){
        if(!original_players::IsA(external,0x4291b10))return nullptr;
        return static_cast<uint8_t*>(external)+0x38;
    }
    return runtimeCurve;
}
inline bool InitializeImpl(void* actualFiring){
    if(!original_players::Thread()||!original_players::IsA(actualFiring,0x684d4c0))
        return Fail(1170,"original_damage_tuning_requires_real_firing_component");
    if(gameThread&&gameThread!=GetCurrentThreadId())return Fail(1170,"original_damage_tuning_wrong_thread");
    if(diagnostics.restartRequired)return false;
    gameThread=GetCurrentThreadId();
    diagnostics.firing=actualFiring;
    // ProjectileTuning.ProjectileFired is the actual reflected strong UClass
    // field at FiringState+550. It retains class, inherited template and curves.
    diagnostics.projectileClass=Read<void*>(actualFiring,0x550);
    diagnostics.realFiringClass=original_players::Derived(diagnostics.projectileClass,engine::Class(ProjectileClass));
    if(!diagnostics.realFiringClass)return Fail(1171,"original_damage_tuning_original_projectile_class_missing");
    diagnostics.strongClassReference=Read<void*>(actualFiring,0x550)==diagnostics.projectileClass;
    diagnostics.damageTemplate=nullptr;diagnostics.templateClass=nullptr;
    for(void* type=diagnostics.projectileClass;type;type=Read<void*>(type,0x48)){
        auto object=engine::function<void*(*)(void*,void*,const wchar_t*,bool)>(0x1d07dd0)
            (engine::Class(DamageComponentClass),type,L"DamageProjectileEffectComponent_GEN_VARIABLE",false);
        if(original_players::IsA(object,DamageComponentClass)){
            diagnostics.damageTemplate=object;diagnostics.templateClass=type;break;
        }
    }
    diagnostics.templateFound=diagnostics.damageTemplate!=nullptr;
    if(!diagnostics.templateFound)return Fail(1172,"original_damage_tuning_original_scs_template_missing");
    // DamageTuning begins+f0; its native RuntimeFloatCurves are88 bytes each.
    // The original DamageType UClass follows four curves at component+310.
    diagnostics.damageType=Read<void*>(diagnostics.damageTemplate,0x310);
    if(!original_players::Derived(diagnostics.damageType,engine::Class(original_damage::DamageTypeClass)))
        return Fail(1173,"original_damage_tuning_original_damage_type_missing");
    auto baseCurve=RuntimeRichCurve(static_cast<uint8_t*>(diagnostics.damageTemplate)+0xf0);
    if(!baseCurve)return Fail(1174,"original_damage_tuning_original_base_curve_missing");
    const float base=engine::function<float(*)(void*,float,float)>(0x42c8060)(baseCurve,0.0f,0.0f);
    if(!std::isfinite(base)||base<=0||base>1000000)return Fail(1175,"original_damage_tuning_original_base_curve_invalid");
    diagnostics.ready=true;diagnostics.error=0;diagnostics.stage="original_damage_tuning_original_projectile_data_ready";return true;
}
inline bool Initialize(void* actualFiring){
    __try{return InitializeImpl(actualFiring);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool CalculateImpl(const original_shots::Shot& shot,const original_damage::Hit& hit,Calculation& result){
    result={};
    if(!Thread()||!diagnostics.ready||!hit.hit||!hit.component||!original_players::Finite(shot.start))
        return Fail(1176,"original_damage_tuning_calculation_context_invalid");
    if(Read<void*>(diagnostics.firing,0x550)!=diagnostics.projectileClass)
        return Fail(1177,"original_damage_tuning_projectile_changed_reinitialize");
    const auto impact=Read<engine::Vec>(hit.nativeHit,0x28);
    if(!original_players::Finite(impact))return Fail(1178,"original_damage_tuning_native_impact_invalid");
    const auto x=impact.x-shot.start.x,y=impact.y-shot.start.y,z=impact.z-shot.start.z;
    result.distance=std::sqrt(x*x+y*y+z*z);
    if(!std::isfinite(result.distance)||result.distance>1000000)
        return Fail(1178,"original_damage_tuning_native_distance_invalid");
    auto rich=RuntimeRichCurve(static_cast<uint8_t*>(diagnostics.damageTemplate)+0xf0);
    if(!rich)return Fail(1174,"original_damage_tuning_original_base_curve_missing");
    // Same original evaluator and arguments as native damage function78c7050.
    result.baseDamage=engine::function<float(*)(void*,float,float)>(0x42c8060)
        (rich,static_cast<float>(result.distance),0.0f);
    result.regionalMultiplier=engine::function<float(*)(void*,const engine::Vec*,const uint8_t*,uint8_t*)>(0x78c6de0)
        (diagnostics.damageTemplate,&shot.start,hit.nativeHit,&result.region);
    result.originalRegionVerified=result.region==hit.region&&result.region<=2;
    if(!result.originalRegionVerified)return Fail(1179,"original_damage_tuning_native_region_mismatch");
    auto sourcePawn=Read<void*>(diagnostics.firing,0x4c8);
    if(!original_players::IsA(sourcePawn,original_players::ShooterCharacterClass))
        return Fail(1180,"original_damage_tuning_original_firing_owner_missing");
    auto abilities=Read<void*>(sourcePawn,0xa20);auto attributes=Read<void*>(abilities,0x13a8);
    if(!original_players::IsA(attributes,0x63e58b0))return Fail(1181,"original_damage_tuning_source_attributes_missing");
    // Exact source attribute CurrentValue offsets used by original78c7331..
    // The original region helper above respects the target disable-multiplier
    // attribute. This adapter covers unobstructed, nonpenetrating trainer hits.
    const size_t offsets[]{0x914,0x904,0x924};
    auto victim=Read<void*>(hit.component,0xc8);
    auto victimAbilities=Read<void*>(victim,0xa20);
    auto victimAttributes=Read<void*>(victimAbilities,0x13a8);
    const auto epsilon=Read<float>(reinterpret_cast<void*>(engine::image),0xc8d0a0c);
    const bool regionalDisabled=original_players::IsA(victimAttributes,0x63e58b0)
        &&std::abs(Read<float>(victimAttributes,0x764))>epsilon;
    result.attributeMultiplier=regionalDisabled?1.0f:Read<float>(attributes,offsets[result.region]);
    result.totalDamage=result.baseDamage*result.regionalMultiplier*result.attributeMultiplier;
    float minimum=0;
    engine::function<void(*)(void*,float*,float*)>(0x42c66f0)(rich,&minimum,&result.originalCurveMaximum);
    if(!std::isfinite(result.baseDamage)||result.baseDamage<0||!std::isfinite(result.regionalMultiplier)
       ||result.regionalMultiplier<0||!std::isfinite(result.attributeMultiplier)||result.attributeMultiplier<0
       ||!std::isfinite(result.totalDamage)||result.totalDamage<0||result.totalDamage>1000000
       ||!std::isfinite(result.originalCurveMaximum))return Fail(1182,"original_damage_tuning_native_result_invalid");
    result.falloffMultiplier=result.originalCurveMaximum>0?result.totalDamage/result.originalCurveMaximum:0;
    result.nativeFalloffFeature=engine::function<bool(*)(void*,uint8_t)>(0x6e08cf0)(original_players::world,70);
    if(result.nativeFalloffFeature){
        // Original hit-confirm normalization from78c73db/78c73f2; the constants
        // themselves are read from the pinned image's original data segment.
        const auto scale=Read<float>(reinterpret_cast<void*>(engine::image),0xc8d0c2c);
        const auto average=Read<float>(reinterpret_cast<void*>(engine::image),0xc8d11cc);
        const auto clamped=(std::max)(0.0f,(std::min)(1.0f,result.totalDamage*scale));
        result.falloffMultiplier=(result.falloffMultiplier+clamped)*average;
    }
    result.damageType=diagnostics.damageType;result.valid=true;
    diagnostics.last=result;++diagnostics.calculations;diagnostics.error=0;
    diagnostics.stage="original_damage_tuning_original_curves_evaluated";return true;
}
inline bool Calculate(const original_shots::Shot& shot,const original_damage::Hit& hit,Calculation& result){
    __try{return CalculateImpl(shot,hit,result);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    const auto b=[](bool v){return v?"true":"false";};const auto& s=diagnostics;const auto& c=s.last;
    std::fprintf(f,"{\"schema_version\":1,\"stage\":");original_players::Text(f,s.stage);
    std::fprintf(f,",\"error\":%u,\"ready\":%s,\"real_firing_class\":%s,\"template_found\":%s,\"strong_class_reference\":%s,\"restart_required\":%s,\"exception_rva\":%llu,\"calculations\":%u,\"firing\":\"%p\",\"projectile_class\":\"%p\",\"template_class\":\"%p\",\"damage_template\":\"%p\",\"damage_type\":\"%p\",\"last\":{\"valid\":%s,\"original_region_verified\":%s,\"region\":%u,\"distance_cm\":%.9g,\"base_damage\":%.9g,\"regional_multiplier\":%.9g,\"attribute_multiplier\":%.9g,\"total_damage\":%.9g,\"original_curve_maximum\":%.9g,\"falloff_multiplier\":%.9g,\"native_falloff_feature\":%s},\"wall_penetration_claimed\":false,\"runtime_verified\":false}\n",
        s.error,b(s.ready),b(s.realFiringClass),b(s.templateFound),b(s.strongClassReference),b(s.restartRequired),
        static_cast<unsigned long long>(s.exceptionRva),s.calculations,s.firing,s.projectileClass,s.templateClass,s.damageTemplate,s.damageType,
        b(c.valid),b(c.originalRegionVerified),static_cast<unsigned>(c.region),c.distance,c.baseDamage,c.regionalMultiplier,c.attributeMultiplier,
        c.totalDamage,c.originalCurveMaximum,c.falloffMultiplier,b(c.nativeFalloffFeature));
    const bool okay=std::ferror(f)==0;std::fclose(f);return okay;
}
}
