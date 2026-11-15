#pragma once
#include "original_hud.h"
#include "local_server_match.h"
#include <array>
#include <string>

// Passive evidence for the original kill-display call boundary. All UObject
// queries run on the existing client game thread. Writers copy cached POD.
namespace local_server_ui_vm_probe {
struct PropertySnapshot {
    void *field=nullptr,*fieldClass=nullptr,*ref78=nullptr,*ref80=nullptr;
    uint64_t castFlags=0;int32_t offset=-1,bytes=0,dimensions=0,ownerBytes=0;
};
struct FunctionSnapshot {
    void *object=nullptr,*outer=nullptr,*exec=nullptr;uint32_t flags=0;
    uint16_t parameterBytes=0;int32_t propertiesBytes=0;
    std::array<PropertySnapshot,2> fields{};
};
struct Snapshot {
    uint64_t sampledAt=0;uintptr_t image=0;std::array<uintptr_t,4> vm{};
    std::array<bool,4> vmKnown{};
    void *function=nullptr,*outer=nullptr,*exec=nullptr;
    uint32_t flags=0,exception=0;uint16_t parameterBytes=0;int32_t propertiesBytes=0;
    std::array<void*,2> widgets{},tables{};
    std::array<FunctionSnapshot,6> pendingMethods{};
    PropertySnapshot pendingProperty{},inner78{},inner80{},verticalProperty{};
    void *timestampStruct=nullptr,*rowStruct=nullptr,*verticalBox=nullptr,*verticalClass=nullptr;
    int32_t timestampBytes=0,rowBytes=0,pendingCount=-1,pendingCapacity=-1;
    std::array<PropertySnapshot,2> timestampFields{};
    std::array<PropertySnapshot,11> rowFields{};
};
inline Snapshot snapshot{};
inline uint64_t nextSampleAt=0;
inline SRWLOCK lock=SRWLOCK_INIT;
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline constexpr const wchar_t* PendingPaths[]{
    L"/Game/UI/InGame/HUD/KillCallout/WBP_KillCalloutHUDElement_Base.WBP_KillCalloutHUDElement_Base_C.EventTickPendingKillRows",
    L"/Game/UI/InGame/HUD/KillCallout/KillFeed_Assists/WBP_KillFeedRow_Base.WBP_KillFeedRow_Base_C.SetTextForKillNumber",
    L"/Game/UI/InGame/HUD/KillCallout/KillFeed_Assists/WBP_KillFeedRow_Base.WBP_KillFeedRow_Base_C.UpdateVFXVisibility",
    L"/Game/UI/InGame/HUD/KillCallout/KillFeed_Assists/WBP_KillFeedParticpant_Base.WBP_KillFeedParticpant_Base_C.UpdateOnFireVFXVisibility",
    L"/Script/UMG.PanelWidget.GetChildrenCount",L"/Script/UMG.PanelWidget.GetChildAt"};
inline constexpr const wchar_t* PendingParamNames[][2]{
    {nullptr,nullptr},{L"KillNumber",nullptr},{L"KillerNumber",L"VictimNumber"},
    {L"NumberOfKillsInRound",nullptr},{L"ReturnValue",nullptr},{L"Index",L"ReturnValue"}};
inline constexpr const wchar_t* TimestampNames[]{
    L"RowInfo_6_0BFCC48B427F1A7ADCFD21A260E1C1DF",L"ExpiryTime_5_94B4E9B7471CDFAB147CC28397FBF680"};
inline constexpr const wchar_t* RowNames[]{
    L"OwningPlayer_3_361529BD405A88DC93FB5390E366B0B4",L"LeftPlayerState_9_B3CF0D5E424FE9D9A566E580429632D3",
    L"RightPlayerState_11_53DE82B84913E95183A716A36AA4B577",L"KillNumberInRoundForKiller_13_44B38B8E43DAE4CA31C663A0F63B723C",
    L"KillNumberInRoundForVictim_31_CCD4A2E74B36D60E7EEC98B3891E833D",L"DamageResponse_15_A6F68D6947762DA4CB967B8C634882EC",
    L"bWasAllyRes_19_7DFB202F45A964FD63F334BDAFD948A8",L"EquippableUsed_22_793FB6124858143BDAA56F933AADA733",
    L"AssistsList_26_0C92D62844C93DDFB2E8BC9A5A221CA3",L"bWasTemporaryDeath_28_36D834AB48CDE62C29B6C893C7C63808",
    L"bWasRecovery_33_0C60F131465A698060DD7B8B4E584337"};
inline PropertySnapshot Describe(void* field,int32_t ownerBytes=0){
    PropertySnapshot p{};p.field=field;p.ownerBytes=ownerBytes;if(!field)return p;
    p.fieldClass=Read<void*>(field,8);p.castFlags=Read<uint64_t>(p.fieldClass,0x18);
    p.offset=Read<int32_t>(field,0x44);p.bytes=Read<int32_t>(field,0x34);p.dimensions=Read<int32_t>(field,0x30);
    p.ref78=Read<void*>(field,0x78);p.ref80=Read<void*>(field,0x80);return p;
}
inline PropertySnapshot Property(void* type,const wchar_t* text){
    if(!type||!text)return {};struct Name{uint32_t words[3]{};} name;
    // Existing-name lookup only; no FName-pool insertion, object load or UI call.
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&name,text,0);
    if(!name.words[0]&&!name.words[1])return {};
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&name,3);
    if(!field||Read<uint64_t>(field,0x20)!=Read<uint64_t>(&name))return {};
    return Describe(field,Read<int32_t>(type,0x60));
}
inline void CapturePendingMetadata(Snapshot& value){
    for(size_t n=0;n<value.pendingMethods.size();++n){auto& m=value.pendingMethods[n];
        m.object=engine::Find(PendingPaths[n]);if(!m.object)continue;
        m.outer=Read<void*>(m.object,0x28);m.exec=Read<void*>(m.object,0xe0);m.flags=Read<uint32_t>(m.object,0xb8);
        m.parameterBytes=Read<uint16_t>(m.object,0xbe);m.propertiesBytes=Read<int32_t>(m.object,0x60);
        for(size_t k=0;k<m.fields.size();++k)if(PendingParamNames[n][k])m.fields[k]=Property(m.object,PendingParamNames[n][k]);
    }
    auto feed=value.widgets[0];if(!feed)return;auto type=Read<void*>(feed,0x10);
    value.pendingProperty=Property(type,L"PendingKillRowInfos");value.verticalProperty=Property(type,L"VerticalBoxForKillRows");
    // Retain both candidates explicitly. Interpretation below requires the
    // real FArrayProperty.Inner+80 and StructProperty.Struct+78 profile.
    value.inner78=Describe(value.pendingProperty.ref78);value.inner80=Describe(value.pendingProperty.ref80);
    const auto& inner=value.inner80;
    if((inner.castFlags&0x8001)==0x8001&&inner.bytes>0&&inner.bytes<=512){
        auto structure=inner.ref78;const auto scriptStruct=engine::Find(L"/Script/CoreUObject.ScriptStruct");
        if(structure&&scriptStruct&&original_players::Derived(Read<void*>(structure,0x10),scriptStruct)){
            value.timestampStruct=structure;value.timestampBytes=Read<int32_t>(structure,0x60);
            for(size_t n=0;n<value.timestampFields.size();++n)value.timestampFields[n]=Property(structure,TimestampNames[n]);
            auto row=value.timestampFields[0].ref78;
            if(row&&original_players::Derived(Read<void*>(row,0x10),scriptStruct)){
                value.rowStruct=row;value.rowBytes=Read<int32_t>(row,0x60);
                for(size_t n=0;n<value.rowFields.size();++n)value.rowFields[n]=Property(row,RowNames[n]);
            }
        }
    }
    if(value.pendingProperty.offset>=0&&value.pendingProperty.bytes==16
       &&value.pendingProperty.offset+16<=value.pendingProperty.ownerBytes){
        value.pendingCount=Read<int32_t>(feed,static_cast<size_t>(value.pendingProperty.offset)+8);
        value.pendingCapacity=Read<int32_t>(feed,static_cast<size_t>(value.pendingProperty.offset)+12);
    }
    if(value.verticalProperty.offset>=0&&value.verticalProperty.bytes==8
       &&value.verticalProperty.offset+8<=value.verticalProperty.ownerBytes){
        value.verticalBox=Read<void*>(feed,static_cast<size_t>(value.verticalProperty.offset));
        value.verticalClass=Read<void*>(value.verticalBox,0x10);
    }
}
inline void CaptureImpl(void* world,Snapshot& value){
    value.sampledAt=GetTickCount64();value.image=engine::image;
    constexpr size_t opcodes[]{0x1b,0x1c,0x45,0x46};
    for(size_t n=0;n<value.vm.size();++n)
        value.vmKnown[n]=original_players::Read(reinterpret_cast<void*>(engine::image),0xd747c40+opcodes[n]*8,value.vm[n]);
    value.function=engine::Find(L"/Game/UI/InGame/HUD/KillCallout/KillFeed_Assists/WBP_KillFeedRow_Base.WBP_KillFeedRow_Base_C.PopulateKillFeedRow");
    if(value.function){
        value.outer=Read<void*>(value.function,0x28);value.exec=Read<void*>(value.function,0xe0);
        value.flags=Read<uint32_t>(value.function,0xb8);value.parameterBytes=Read<uint16_t>(value.function,0xbe);
        value.propertiesBytes=Read<int32_t>(value.function,0x60);
    }
    // Do not load an absent widget class or instantiate any HUD for a probe.
    if(original_hud::world==world&&original_hud::diagnostics.coordinator){
        constexpr const wchar_t* paths[]{
            L"/Game/UI/InGame/HUD/KillCallout/KillCalloutHUDElement.KillCalloutHUDElement_C",
            L"/Game/UI/InGame/KillBanner/CommonAssets/KillBannerAnnouncement.KillBannerAnnouncement_C"};
        for(size_t n=0;n<value.widgets.size();++n)if(engine::Find(paths[n])){
            value.widgets[n]=original_hud::FindCoordinatedWidget(paths[n]);
            if(original_hud::IsA(value.widgets[n],0x3b0f3c0))value.tables[n]=Read<void*>(value.widgets[n]);
        }
    }
    CapturePendingMetadata(value);
}
inline void Capture(void* world){
    if(!world||!engine::image||original_players::gameThread!=GetCurrentThreadId())return;
    const auto now=GetTickCount64();if(now<nextSampleAt)return;nextSampleAt=now+1000;
    Snapshot value{};
    __try{CaptureImpl(world,value);}
    __except(EXCEPTION_EXECUTE_HANDLER){value.exception=GetExceptionCode();}
    AcquireSRWLockExclusive(&lock);snapshot=value;ReleaseSRWLockExclusive(&lock);
}
inline std::string Json(){
    AcquireSRWLockShared(&lock);const auto value=snapshot;ReleaseSRWLockShared(&lock);
    local_server_match::Json out;
    const auto rva=[&](uintptr_t address){return static_cast<unsigned long long>(address>=value.image&&address-value.image<0xe000000?address-value.image:0);};
    out.Add("{\"sampled_at_ms\":%llu,\"exception\":%u,\"native_writes\":0,\"vm_targets\":[",static_cast<unsigned long long>(value.sampledAt),value.exception);
    constexpr unsigned opcodes[]{0x1b,0x1c,0x45,0x46};
    for(size_t n=0;n<value.vm.size();++n)
        out.Add("%s{\"opcode\":%u,\"known\":%s,\"pointer\":\"%p\",\"rva\":%llu}",n?",":"",opcodes[n],value.vmKnown[n]?"true":"false",reinterpret_cast<void*>(value.vm[n]),rva(value.vm[n]));
    out.Add("],\"populate\":{\"object\":\"%p\",\"outer28\":\"%p\",\"exec\":\"%p\",\"exec_rva\":%llu,\"flags\":%u,\"parameter_bytes\":%u,\"properties_bytes\":%d},\"widgets\":[",
        value.function,value.outer,value.exec,rva(reinterpret_cast<uintptr_t>(value.exec)),value.flags,static_cast<unsigned>(value.parameterBytes),value.propertiesBytes);
    for(size_t n=0;n<value.widgets.size();++n)
        out.Add("%s{\"kind\":\"%s\",\"object\":\"%p\",\"table\":\"%p\",\"table_rva\":%llu}",n?",":"",n?"kill_banner":"kill_callout",value.widgets[n],value.tables[n],rva(reinterpret_cast<uintptr_t>(value.tables[n])));
    out.Add("],\"pending_metadata\":{\"scope\":\"passive_original_fields_no_ui_invocation\",\"functions\":[");
    const auto property=[&](const PropertySnapshot& p){out.Add("{\"field\":\"%p\",\"field_class\":\"%p\",\"cast_flags\":%llu,\"offset\":%d,\"bytes\":%d,\"dimensions\":%d,\"owner_bytes\":%d,\"ref78\":\"%p\",\"ref80\":\"%p\"}",p.field,p.fieldClass,static_cast<unsigned long long>(p.castFlags),p.offset,p.bytes,p.dimensions,p.ownerBytes,p.ref78,p.ref80);};
    constexpr const char* roles[]{"pending_timer_event","row_text","row_vfx","participant_vfx","panel_count","panel_child"};
    for(size_t n=0;n<value.pendingMethods.size();++n){const auto& m=value.pendingMethods[n];
        out.Add("%s{\"role\":\"%s\",\"object\":\"%p\",\"outer28\":\"%p\",\"exec_rva\":%llu,\"flags\":%u,\"parameter_bytes\":%u,\"properties_bytes\":%d,\"fields\":[",n?",":"",roles[n],m.object,m.outer,rva(reinterpret_cast<uintptr_t>(m.exec)),m.flags,static_cast<unsigned>(m.parameterBytes),m.propertiesBytes);
        for(size_t k=0;k<m.fields.size();++k){if(k)out.Add(",");property(m.fields[k]);}out.Add("]}");
    }
    out.Add("],\"pending_property\":");property(value.pendingProperty);out.Add(",\"inner78\":");property(value.inner78);out.Add(",\"inner80\":");property(value.inner80);
    out.Add(",\"vertical_property\":");property(value.verticalProperty);
    out.Add(",\"timestamp_struct\":\"%p\",\"timestamp_bytes\":%d,\"row_struct\":\"%p\",\"row_bytes\":%d,\"pending_count\":%d,\"pending_capacity\":%d,\"vertical_box\":\"%p\",\"vertical_class\":\"%p\",\"timestamp_fields\":[",value.timestampStruct,value.timestampBytes,value.rowStruct,value.rowBytes,value.pendingCount,value.pendingCapacity,value.verticalBox,value.verticalClass);
    for(size_t n=0;n<value.timestampFields.size();++n){if(n)out.Add(",");property(value.timestampFields[n]);}out.Add("],\"row_fields\":[");
    for(size_t n=0;n<value.rowFields.size();++n){if(n)out.Add(",");property(value.rowFields[n]);}out.Add("]}}");return out.value;
}
}
