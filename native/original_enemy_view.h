#pragma once
#include "original_players.h"
#include <array>

// Original NPC presentation policy for this standalone trainer. Native Engine
// PlayerController::IsLocalController treats every standalone ShooterPC as
// local, including PCs with no UPlayer. The shipped character already exposes
// Set3PMeshVisible for this case: it selects the original full3P render/AnimBP
// path through the original IsFirstPerson selector and UpdatePawnMeshes.
// No controller role, ownership, camera, hidden flag, mesh, material, pose,
// animation mode, component tick or renderer flag is written by this adapter.
// Evidence: work/duel1-original-enemy-view.
namespace original_enemy_view {
struct Entry {
    bool bound=false,npc=false,hasPlayer=false,inLocalPlayers=false,localOwnershipValid=false;
    bool originalFirstPersonBefore=false,firstPerson=false,original3PFlagBefore=false,original3PFlag=false;
    bool applied=false,rolesPreserved=false,coreHidden=true,cosmeticHidden=true,armsHidden=true;
    bool gun1PHidden=true,gun3PHidden=true;
    bool fogRelevant=false,initialMeshPolicyComputed=false,renderAllowed=false;
    bool hide1POverride=false,hide3POverride=false,alive=false;
    float attributeHide1P=0,attributeHide3P=0,attributeForce3P=0;
    uint8_t roleBefore=0,roleAfter=0,remoteRoleBefore=0,remoteRoleAfter=0;
    uint32_t setterCalls=0,samples=0;
    uint64_t corePoseFrame=0,cosmeticPoseFrame=0;
    float coreLastRendered=-1000,cosmeticLastRendered=-1000;
    void *pawn=nullptr,*controller=nullptr,*player=nullptr,*gameInstance=nullptr;
    void *core=nullptr,*cosmetic=nullptr,*arms=nullptr,*weapon=nullptr,*gun1P=nullptr,*gun3P=nullptr;
    void *corePrimitive=nullptr,*cosmeticPrimitive=nullptr,*gun3PPrimitive=nullptr;
};
struct Diagnostics {
    unsigned error=0,boundNPCs=0,setterCalls=0,samples=0;
    bool restartRequired=false,localUntouched=true;
    uintptr_t exceptionRva=0;
    const char* stage="original_enemy_view_not_applied";
    const char* operation="none";
};
inline Diagnostics diagnostics;
inline std::array<Entry,original_players::MaxPlayers> entries{};
inline const Diagnostics& State(){return diagnostics;}
template<class T> inline T Value(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline bool Fail(unsigned code,const char* text){diagnostics.error=code;diagnostics.stage=text;return false;}
inline bool Thread(){return !diagnostics.restartRequired&&original_players::Thread();}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* parameters){
    auto method=engine::Find(name);
    if(!object||!method||Value<uint16_t>(method,0xbe)!=size)return false;
    auto table=Value<void**>(object,0);auto process=Value<void*>(table,0x298);
    if(!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,method,parameters);return true;
}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    diagnostics.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0001430U;
    const auto at=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=at>=engine::image?at-engine::image:at;
    diagnostics.restartRequired=true;diagnostics.stage="original_enemy_view_exception_requires_new_session";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool QueryLocalMembership(Entry& entry){
    entry.player=Value<void*>(entry.controller,0x510);entry.hasPlayer=entry.player!=nullptr;
    entry.gameInstance=Value<void*>(original_players::world,0x1d8);
    if(!original_players::IsA(entry.gameInstance,0x647e8a0))return Fail(1430,"enemy_view_requires_original_game_instance");
    auto localPlayers=Value<void**>(entry.gameInstance,0x40);
    const int32_t count=Value<int32_t>(entry.gameInstance,0x48),capacity=Value<int32_t>(entry.gameInstance,0x4c);
    if(count<1||count>64||capacity<count||!original_players::Readable(localPlayers,static_cast<size_t>(count)*8))
        return Fail(1431,"enemy_view_original_local_player_list_invalid");
    entry.inLocalPlayers=false;entry.localOwnershipValid=false;
    for(int32_t i=0;i<count;++i){
        auto local=Value<void*>(localPlayers,static_cast<size_t>(i)*8);
        if(!original_players::IsA(local,0x3b6e680))return Fail(1432,"enemy_view_original_local_player_entry_invalid");
        auto controller=Value<void*>(local,0x38);
        if(controller==entry.controller||local==entry.player)entry.inLocalPlayers=true;
        if(local==original_players::localPlayer&&controller==original_players::activeController
           &&Value<void*>(controller,0x510)==local)entry.localOwnershipValid=true;
    }
    return entry.localOwnershipValid?true:Fail(1433,"enemy_view_actual_local_player_ownership_invalid");
}
inline bool Hidden(void* component){return !component||(Value<uint8_t>(component,0x251)&2)!=0;}
inline bool SampleImpl(size_t index){
    if(!Thread()||index>=entries.size()||!entries[index].bound)return false;
    auto& entry=entries[index];
    if(!QueryLocalMembership(entry))return false;
    uint8_t first=0;
    if(!Event(entry.pawn,L"/Script/ShooterGame.ShooterCharacter.IsFirstPerson",1,&first))
        return Fail(1434,"enemy_view_original_first_person_query_failed");
    entry.firstPerson=first!=0;entry.original3PFlag=Value<uint8_t>(entry.pawn,0x1270)!=0;
    entry.roleAfter=Value<uint8_t>(entry.pawn,0x224);entry.remoteRoleAfter=Value<uint8_t>(entry.pawn,0x128);
    entry.rolesPreserved=entry.roleBefore==entry.roleAfter&&entry.remoteRoleBefore==entry.remoteRoleAfter;
    entry.core=Value<void*>(entry.pawn,0x4f0);entry.cosmetic=Value<void*>(entry.pawn,0xf40);entry.arms=Value<void*>(entry.pawn,0xf30);
    auto fog=Value<void*>(entry.pawn,0xa68);entry.fogRelevant=!fog||Value<uint8_t>(fog,0x1e1)!=0;
    entry.initialMeshPolicyComputed=Value<uint8_t>(entry.pawn,0xf59)!=0;entry.renderAllowed=Value<uint8_t>(entry.pawn,0x1442)!=0;
    entry.hide1POverride=Value<uint8_t>(entry.pawn,0x1410)!=0;entry.hide3POverride=Value<uint8_t>(entry.pawn,0x1411)!=0;
    auto abilities=Value<void*>(entry.pawn,0xa20);auto attributes=Value<void*>(abilities,0x13a8);
    entry.attributeHide1P=Value<float>(attributes,0x174);entry.attributeHide3P=Value<float>(attributes,0x184);entry.attributeForce3P=Value<float>(attributes,0x564);
    entry.alive=Value<uint8_t>(Value<void*>(entry.pawn,0xc70),0x1f9)!=0;
    entry.coreHidden=Hidden(entry.core);entry.cosmeticHidden=Hidden(entry.cosmetic);entry.armsHidden=Hidden(entry.arms);
    entry.corePrimitive=Value<void*>(entry.core,0x690);entry.cosmeticPrimitive=Value<void*>(entry.cosmetic,0x690);
    entry.corePoseFrame=Value<uint64_t>(entry.core,0xd78);entry.cosmeticPoseFrame=Value<uint64_t>(entry.cosmetic,0xd78);
    entry.coreLastRendered=Value<float>(entry.core,0x4ac);entry.cosmeticLastRendered=Value<float>(entry.cosmetic,0x4ac);
    auto inventory=Value<void*>(entry.pawn,0xc08);entry.weapon=Value<void*>(inventory,0x280);
    entry.gun1P=Value<void*>(entry.weapon,0xde0);entry.gun3P=Value<void*>(entry.weapon,0xdf0);
    entry.gun1PHidden=Hidden(entry.gun1P);entry.gun3PHidden=Hidden(entry.gun3P);entry.gun3PPrimitive=Value<void*>(entry.gun3P,0x690);
    ++entry.samples;++diagnostics.samples;
    if(entry.hasPlayer||entry.inLocalPlayers||!entry.rolesPreserved)return Fail(1435,"enemy_view_local_membership_or_role_changed");
    return true;
}
inline bool Sample(size_t index){
    __try{return SampleImpl(index);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool ApplyImpl(size_t index){
    if(!Thread()||index>=entries.size()||static_cast<int>(index)==original_players::localIndex)
        return Fail(1436,"enemy_view_requires_non_local_roster_entry");
    auto& roster=original_players::players[index];
    if(!roster.ready||roster.local||!roster.registered||!original_players::IsA(roster.character,original_players::ShooterCharacterClass)
       ||!original_players::IsA(roster.controller,original_players::ShooterControllerClass)
       ||Value<void*>(roster.controller,0x4b0)!=roster.character||Value<void*>(roster.character,0x4a0)!=roster.controller)
        return Fail(1437,"enemy_view_original_player_pair_not_ready");
    auto& entry=entries[index];
    if(!entry.bound||entry.pawn!=roster.character){
        const bool firstBinding=!entry.bound;
        entry={};entry.bound=true;entry.pawn=roster.character;entry.controller=roster.controller;
        entry.roleBefore=Value<uint8_t>(entry.pawn,0x224);entry.remoteRoleBefore=Value<uint8_t>(entry.pawn,0x128);
        entry.original3PFlagBefore=Value<uint8_t>(entry.pawn,0x1270)!=0;
        if(firstBinding)++diagnostics.boundNPCs;
    }
    diagnostics.operation="enemy_view_validate_actual_local_player_membership";
    if(!QueryLocalMembership(entry))return false;
    if(entry.hasPlayer||entry.inLocalPlayers)return Fail(1438,"enemy_view_refuses_actual_local_or_connected_player");
    entry.npc=true;
    auto table=Value<void**>(entry.pawn,0);
    if(Value<void*>(table,0xa88)!=reinterpret_cast<void*>(engine::image+0x7884460))
        return Fail(1439,"enemy_view_original_first_person_selector_changed");
    uint8_t before=0;
    if(!Event(entry.pawn,L"/Script/ShooterGame.ShooterCharacter.IsFirstPerson",1,&before))
        return Fail(1434,"enemy_view_original_first_person_query_failed");
    if(!entry.setterCalls)entry.originalFirstPersonBefore=before!=0;
    const auto activePawn=original_players::activePawn,activeController=original_players::activeController;
    const uint8_t local3P=Value<uint8_t>(activePawn,0x1270);
    if(before||!Value<uint8_t>(entry.pawn,0x1270)){
        // Exact original API: C++ and reflected bool payload are one byte. It
        // writes its own1270 selector and immediately runs native786eef0,
        // including original equippable70f0250 and OnUpdateCharacterMeshes.
        uint8_t enabled=1;diagnostics.operation="enemy_view_original_set_3p_mesh_visible";
        if(!Event(entry.pawn,L"/Script/ShooterGame.ShooterCharacter.Set3PMeshVisible",1,&enabled))
            return Fail(1440,"enemy_view_original_set_3p_api_unavailable");
        ++entry.setterCalls;++diagnostics.setterCalls;
    }
    diagnostics.localUntouched=original_players::activePawn==activePawn&&original_players::activeController==activeController
        &&Value<uint8_t>(activePawn,0x1270)==local3P;
    if(!diagnostics.localUntouched)return Fail(1441,"enemy_view_actual_local_selection_changed");
    if(!SampleImpl(index))return false;
    entry.applied=entry.original3PFlag&&!entry.firstPerson;
    if(!entry.applied)return Fail(1442,"enemy_view_original_third_person_selection_not_observed");
    diagnostics.error=0;diagnostics.stage="original_enemy_third_person_selected_pending_render_observation";diagnostics.operation="none";return true;
}
inline bool Apply(size_t index){
    __try{return ApplyImpl(index);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool ApplyAllNPCs(){
    if(!Thread())return false;
    for(size_t index=0;index<original_players::MaxPlayers;++index){
        const auto& entry=original_players::players[index];
        if(entry.ready&&!entry.local&&static_cast<int>(index)!=original_players::localIndex)
            if(!Apply(index))return false;
    }
    return true;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    const auto b=[](bool value){return value?"true":"false";};
    std::fputs("{\"schema_version\":1,\"stage\":",f);original_players::Text(f,diagnostics.stage);
    std::fprintf(f,",\"error\":%u,\"bound_npcs\":%u,\"setter_calls\":%u,\"samples\":%u,\"local_untouched\":%s,\"restart_required\":%s,\"exception_rva\":%llu,\"operation\":",diagnostics.error,diagnostics.boundNPCs,diagnostics.setterCalls,diagnostics.samples,b(diagnostics.localUntouched),b(diagnostics.restartRequired),static_cast<unsigned long long>(diagnostics.exceptionRva));original_players::Text(f,diagnostics.operation);
    std::fputs(",\"entries\":[",f);bool first=true;
    for(size_t index=0;index<entries.size();++index){const auto& e=entries[index];if(!e.bound)continue;
        std::fprintf(f,"%s{\"roster_index\":%zu,\"npc\":%s,\"has_player\":%s,\"in_original_local_players\":%s,\"actual_local_ownership_valid\":%s,\"original_first_person_before\":%s,\"first_person\":%s,\"original_3p_flag_before\":%s,\"original_3p_flag\":%s,\"applied\":%s,\"roles_preserved\":%s,\"role_before\":%u,\"role_after\":%u,\"remote_role_before\":%u,\"remote_role_after\":%u,\"setter_calls\":%u,\"samples\":%u",first?"":",",index,b(e.npc),b(e.hasPlayer),b(e.inLocalPlayers),b(e.localOwnershipValid),b(e.originalFirstPersonBefore),b(e.firstPerson),b(e.original3PFlagBefore),b(e.original3PFlag),b(e.applied),b(e.rolesPreserved),static_cast<unsigned>(e.roleBefore),static_cast<unsigned>(e.roleAfter),static_cast<unsigned>(e.remoteRoleBefore),static_cast<unsigned>(e.remoteRoleAfter),e.setterCalls,e.samples);first=false;
        std::fprintf(f,",\"core_hidden\":%s,\"cosmetic_hidden\":%s,\"arms_hidden\":%s,\"gun1p_hidden\":%s,\"gun3p_hidden\":%s,\"core_pose_frame\":%llu,\"cosmetic_pose_frame\":%llu,\"core_last_rendered\":%.9g,\"cosmetic_last_rendered\":%.9g",b(e.coreHidden),b(e.cosmeticHidden),b(e.armsHidden),b(e.gun1PHidden),b(e.gun3PHidden),static_cast<unsigned long long>(e.corePoseFrame),static_cast<unsigned long long>(e.cosmeticPoseFrame),e.coreLastRendered,e.cosmeticLastRendered);
        std::fprintf(f,",\"fow_relevant\":%s,\"initial_mesh_policy_computed\":%s,\"render_allowed\":%s,\"hide_1p_override\":%s,\"hide_3p_override\":%s,\"alive\":%s,\"attribute_hide_1p\":%.9g,\"attribute_hide_3p\":%.9g,\"attribute_force_3p\":%.9g",b(e.fogRelevant),b(e.initialMeshPolicyComputed),b(e.renderAllowed),b(e.hide1POverride),b(e.hide3POverride),b(e.alive),e.attributeHide1P,e.attributeHide3P,e.attributeForce3P);
        std::fprintf(f,",\"pawn\":\"%p\",\"controller\":\"%p\",\"player\":\"%p\",\"core_raw_primitive690\":\"%p\",\"cosmetic_raw_primitive690\":\"%p\",\"gun3p_raw_primitive690\":\"%p\"}",e.pawn,e.controller,e.player,e.corePrimitive,e.cosmeticPrimitive,e.gun3PPrimitive);
    }
    std::fputs("],\"original_api\":\"ShooterCharacter.Set3PMeshVisible(true)\",\"render_or_pose_values_written\":false,\"runtime_pixels_verified\":false}\n",f);
    const bool ok=std::ferror(f)==0;std::fclose(f);return ok;
}
}
