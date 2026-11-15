#pragma once
#include "original_scope.h"

// A late local attachment has missed FP_Core's original equippable-change block.
// Resume that complete original block once, using only its already initialized
// original persistent UberGraphFrame. No Blueprint local, LastEquippable,
// AnimationStateComponent transform, animation class, or mesh pose is written.
// Evidence: reports/duel1-original-ads-refresh.json.
namespace original_ads_refresh {
inline constexpr const wchar_t* kType=L"/Game/Characters/_Core/1P/FP_Core_AnimGraph.FP_Core_AnimGraph_C";
inline constexpr const wchar_t* kGraph=L"/Game/Characters/_Core/1P/FP_Core_AnimGraph.FP_Core_AnimGraph_C.ExecuteUbergraph_FP_Core_AnimGraph";
struct Diagnostics {
    unsigned error=0,generation=0;
    uint64_t samples=0,originalCalls=0,completedCalls=0,pendingFrameSamples=0;
    bool profileValid=false,persistentFrameValid=false,originalLocalPawnValid=false;
    bool currentWeaponValid=false,attempted=false,consumed=false,bpScopeMatches=false;
    bool frameUnchanged=false,ownerUnchanged=false,lastEquippableUnchanged=false;
    void *world=nullptr,*pawn=nullptr,*weapon=nullptr,*scope=nullptr,*core=nullptr,*instance=nullptr;
    void *type=nullptr,*graph=nullptr,*frame=nullptr,*nativeFrame=nullptr,*localPawn=nullptr;
    int frameOffset=-1,localPawnOffset=-1,pawnOwnerOffset=-1,lastEquippableOffset=-1,bpScopeOffset=-1;
    const char* stage="original_ads_refresh_not_requested";
};
inline Diagnostics state;
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* object,size_t offset){return original_scope::Read<T>(object,offset);}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;return false;}
inline bool Same(const engine::Transform& a,const engine::Transform& b){
    bool same=std::abs(a.translation.x-b.translation.x)<1e-5
        &&std::abs(a.translation.y-b.translation.y)<1e-5&&std::abs(a.translation.z-b.translation.z)<1e-5
        &&std::abs(a.scale.x-b.scale.x)<1e-5&&std::abs(a.scale.y-b.scale.y)<1e-5&&std::abs(a.scale.z-b.scale.z)<1e-5;
    for(unsigned i=0;i<4;++i)same=same&&std::abs(a.rotation[i]-b.rotation[i])<1e-5;
    return same;
}
inline bool Profile(){
    state.type=engine::Find(kType);state.graph=engine::Find(kGraph);
    if(!state.type||!state.graph||Read<void*>(state.instance,0x10)!=state.type
        ||!original_players::IsA(state.type,0x3863340)||Read<void*>(state.graph,0x28)!=state.type)
        return false;
    state.frameOffset=original_scope::FieldOffset(state.type,L"UberGraphFrame");
    state.localPawnOffset=original_scope::FieldOffset(state.graph,L"K2Node_DynamicCast_AsShooter_Character");
    state.pawnOwnerOffset=original_scope::FieldOffset(state.type,L"ShooterCharacterOwner");
    state.lastEquippableOffset=original_scope::FieldOffset(state.type,L"LastEquippable");
    state.bpScopeOffset=original_scope::FieldOffset(state.type,L"ScopeTransformComponentSpace");
    auto frameField=Read<void*>(state.type,0x290);auto script=Read<uint8_t*>(state.graph,0x68);
    auto classTable=Read<void*>(state.type,0);
    const auto flags=Read<uint32_t>(state.graph,0xb8);
    // The original ProcessEvent obtains this exact persistent frame at1cdc98d
    // through class.v3c0. Its non-null branch skips local zero/init/destruction.
    // Therefore the real normal animation update's Pawn local remains valid.
    state.profileValid=state.frameOffset>=0&&state.localPawnOffset>=0&&state.pawnOwnerOffset>=0
        &&state.lastEquippableOffset>=0&&state.bpScopeOffset>=0
        &&state.bpScopeOffset+static_cast<int>(sizeof(engine::Transform))<=Read<int>(state.type,0x60)
        &&frameField&&Read<int>(frameField,0x44)==state.frameOffset
        &&Read<void*>(state.type,0x298)==state.graph
        &&Read<void*>(classTable,0x3c0)==reinterpret_cast<void*>(engine::image+0x3f41d30)
        &&(flags&0x8000)!=0&&(flags&0x400)==0&&Read<uint16_t>(state.graph,0xbe)==4
        &&Read<int>(state.graph,0x70)==10036&&script
        &&Read<uint8_t>(script,0)==0x4c&&Read<uint32_t>(script,1)==10033
        &&Read<uint8_t>(script,5)==0x4e&&Read<uint8_t>(script,7563)==0
        &&Read<int>(Read<void*>(script,7564),0x44)==state.localPawnOffset
        &&Read<uint8_t>(script,7985)==0x4c&&Read<uint32_t>(script,7986)==9744
        &&Read<uint8_t>(script,7990)==0x5f&&Read<uint8_t>(script,8303)==0x0f
        &&Read<uint8_t>(script,8498)==0x0f&&Read<uint8_t>(script,8569)==0x4d
        &&Read<uint8_t>(script,9744)==0x5f&&Read<uint8_t>(script,10020)==0x4d
        &&Read<uint8_t>(script,10033)==4&&Read<uint8_t>(script,10035)==0x53;
    return state.profileValid;
}
inline bool ObserveConsumed(){
    const auto& observed=original_scope::State();
    engine::Transform bpScope{};
    state.bpScopeMatches=state.bpScopeOffset>=0
        &&original_players::Read(state.instance,static_cast<size_t>(state.bpScopeOffset),bpScope)
        &&Same(observed.scopeTransform,bpScope);
    state.consumed=observed.scope_present&&observed.scope_transform_read&&!observed.scope_transform_identity
        &&observed.gun_to_scope_read&&!observed.gun_to_scope_identity
        &&Same(observed.scopeTransform,observed.gunToScope)&&state.bpScopeMatches
        &&Read<void*>(observed.animationState,0xcf0)==state.weapon
        &&Read<void*>(state.instance,static_cast<size_t>(state.lastEquippableOffset))==state.weapon;
    return state.consumed;
}
inline bool Update(void* world,void* pawn,void* weapon){
    if(!engine::image||original_players::gameThread!=GetCurrentThreadId())
        return Fail(1871,"original_ads_refresh_wrong_game_thread");
    ++state.samples;if(state.error)return false;
    const auto& factory=original_scope::FactoryState();
    if(!factory.scopeReady){state.stage="original_ads_refresh_waiting_for_original_scope";return true;}
    if(!original_scope::Probe(world,pawn,weapon))return Fail(1872,"original_ads_refresh_scope_probe_failed");
    const auto& observed=original_scope::State();
    auto core=Read<void*>(pawn,0xf30);auto instance=Read<void*>(core,0xa70);
    if(state.generation!=factory.generation||state.instance!=instance||state.scope!=observed.scope){
        const auto samples=state.samples,calls=state.originalCalls,completed=state.completedCalls,pending=state.pendingFrameSamples;
        state=Diagnostics{};state.samples=samples;state.originalCalls=calls;state.completedCalls=completed;state.pendingFrameSamples=pending;
        state.generation=factory.generation;state.world=world;state.pawn=pawn;state.weapon=weapon;
        state.scope=observed.scope;state.core=core;state.instance=instance;
    }
    if(world!=factory.world||pawn!=factory.pawn||weapon!=factory.weapon||pawn!=original_weapon::pawn
        ||weapon!=original_weapon::weapon||Read<void*>(core,0xc8)!=pawn
        ||Read<void*>(instance,0x28)!=core||observed.coreInstance!=instance
        ||Read<void*>(instance,0x390)!=observed.animationState
        ||engine::function<int(*)(void*)>(0x3efc820)(pawn)!=0)
        return Fail(1873,"original_ads_refresh_actual_local_owner_mismatch");
    if(!state.profileValid&&!Profile())return Fail(1874,"original_ads_refresh_original_graph_profile_mismatch");
    auto inventory=Read<void*>(pawn,0xc08);
    state.currentWeaponValid=Read<void*>(inventory,0x280)==weapon;
    // Abilities may temporarily be the actual equippable. Let the original
    // normal weapon-change update handle those transitions before catching up.
    if(!state.currentWeaponValid||Read<void*>(instance,static_cast<size_t>(state.lastEquippableOffset))!=weapon
        ||Read<void*>(observed.animationState,0xcf0)!=weapon){
        state.stage="original_ads_refresh_waiting_for_original_weapon_equip";return true;
    }
    if(ObserveConsumed()){state.stage="original_ads_original_scope_alignment_consumed";return true;}
    if(state.attempted)return Fail(1875,"original_ads_refresh_original_graph_did_not_consume_scope");
    state.frame=Read<void*>(instance,static_cast<size_t>(state.frameOffset));
    // Exact same original class virtual used by UObject.ProcessEvent. This is
    // a frame lookup, not an animation/socket/pose evaluation or frame write.
    auto table=Read<void*>(state.type,0);
    using FrameGetter=void*(*)(void*,void*,void*);
    auto getter=Read<FrameGetter>(table,0x3c0);
    state.nativeFrame=getter(state.type,instance,state.graph);
    state.persistentFrameValid=state.frame&&state.frame==state.nativeFrame;
    state.localPawn=Read<void*>(state.frame,static_cast<size_t>(state.localPawnOffset));
    state.originalLocalPawnValid=state.persistentFrameValid&&state.localPawn==pawn
        &&Read<void*>(instance,static_cast<size_t>(state.pawnOwnerOffset))==pawn;
    if(!state.originalLocalPawnValid){
        ++state.pendingFrameSamples;state.stage="original_ads_refresh_waiting_for_original_animation_frame";return true;
    }
    int32_t originalEquipEntry=7985;state.attempted=true;++state.originalCalls;
    // 7985 pushes9744 and performs every original Scope lookup/validity check.
    // 8303 fills BP ScopeTransform;8498 fills native GunToScope;9744 performs
    // original equipped-gun classification and LastEquippable assignment.
    // The entry's only preexisting input is the validated original Pawn local.
    if(!original_scope::Event(instance,kGraph,4,&originalEquipEntry))
        return Fail(1876,"original_ads_refresh_original_equippable_graph_failed");
    ++state.completedCalls;
    state.frameUnchanged=Read<void*>(instance,static_cast<size_t>(state.frameOffset))==state.frame;
    state.ownerUnchanged=Read<void*>(core,0xc8)==pawn&&Read<void*>(core,0xa70)==instance
        &&Read<void*>(instance,0x28)==core&&Read<void*>(instance,static_cast<size_t>(state.pawnOwnerOffset))==pawn;
    state.lastEquippableUnchanged=Read<void*>(inventory,0x280)==weapon
        &&Read<void*>(instance,static_cast<size_t>(state.lastEquippableOffset))==weapon;
    if(!state.frameUnchanged||!state.ownerUnchanged||!state.lastEquippableUnchanged)
        return Fail(1877,"original_ads_refresh_original_graph_owner_postcondition_failed");
    if(!original_scope::Probe(world,pawn,weapon)||!ObserveConsumed())
        return Fail(1875,"original_ads_refresh_original_graph_did_not_consume_scope");
    state.stage="original_ads_original_scope_alignment_consumed";return true;
}
inline bool WriteDiagnostics(const wchar_t* path){
    FILE* f=nullptr;if(!path||_wfopen_s(&f,path,L"wb")||!f)return false;
    std::fprintf(f,"{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"generation\":%u,\"samples\":%llu,\"original_calls\":%llu,\"completed_calls\":%llu,\"pending_frame_samples\":%llu,\"profile_valid\":%d,\"persistent_frame_valid\":%d,\"original_local_pawn_valid\":%d,\"current_weapon_valid\":%d,\"attempted\":%d,\"animation_consumed\":%d,\"blueprint_scope_matches\":%d,\"frame_unchanged\":%d,\"owner_unchanged\":%d,\"last_equippable_unchanged\":%d,\"instance\":\"%p\",\"graph\":\"%p\",\"frame\":\"%p\",\"original_frame_lookup\":\"%p\",\"local_pawn\":\"%p\",\"frame_offset\":%d,\"local_pawn_offset\":%d,\"pawn_owner_offset\":%d,\"last_equippable_offset\":%d,\"blueprint_scope_offset\":%d,\"original_graph_entry\":7985,\"manual_animation_or_scope_writes\":false,\"diagnostic_socket_or_bone_getters_called\":false}\n",
        state.stage,state.error,state.generation,static_cast<unsigned long long>(state.samples),static_cast<unsigned long long>(state.originalCalls),
        static_cast<unsigned long long>(state.completedCalls),static_cast<unsigned long long>(state.pendingFrameSamples),state.profileValid,
        state.persistentFrameValid,state.originalLocalPawnValid,state.currentWeaponValid,state.attempted,state.consumed,state.bpScopeMatches,
        state.frameUnchanged,state.ownerUnchanged,state.lastEquippableUnchanged,state.instance,state.graph,state.frame,state.nativeFrame,
        state.localPawn,state.frameOffset,state.localPawnOffset,state.pawnOwnerOffset,state.lastEquippableOffset,state.bpScopeOffset);
    std::fclose(f);return true;
}
}
