#pragma once
#include "original_players.h"
#include <cmath>

// Use the shipped ColorBlindMode setting and palette. The original settings
// widget follows the same GetAccessibilityColor -> ColorToLinearColor ->
// SetOutlineColorsForRender path. There is no custom palette/shader, periodic
// setting enforcement, renderer-global write or per-component color override.
// Static evidence: work/duel1-original-enemy-colors.
namespace original_enemy_colors {
inline constexpr uint8_t ColorBlindSetting=0x0c;
inline constexpr int32_t YellowMode=2; // EAresColorBlindMode::Deuteranopia.
struct Color {
    uint8_t blue=0,green=0,red=0,alpha=0; // Native FColor is BGRA in memory.
};
struct LinearColor {float red=0,green=0,blue=0,alpha=0;};
static_assert(sizeof(Color)==4&&sizeof(LinearColor)==16);
struct Diagnostics {
    bool attempted=false,applied=false,managerReady=false,dataReady=false;
    bool setterCalled=false,initialSettingVerified=false,paletteRead=false;
    bool linearConverted=false,rendererDispatched=false,rendererMatchesPalette=false;
    bool nativeYellow=false,requestedSettingCurrentlyActive=false,restartRequired=false;
    int32_t previousSetting=-1,currentSetting=-1,minSetting=0,maxSetting=0;
    uint8_t nativeMode=0;
    uint32_t setterCalls=0,rendererCalls=0,samples=0;
    unsigned error=0;
    uintptr_t exceptionRva=0;
    const char* stage="original_enemy_colors_not_applied";
    const char* operation="none";
    const wchar_t* failedEvent=L"";
    uint16_t expectedParmsSize=0,actualParmsSize=0;
    void *world=nullptr,*settings=nullptr,*settingData=nullptr;
    Color ally{},enemy{};
    LinearColor allyLinear{},enemyLinear{},rendererAlly{},rendererEnemy{};
};
inline Diagnostics diagnostics;
inline const Diagnostics& State(){return diagnostics;}
template<class T> inline T Value(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline bool Fail(unsigned code,const char* text){diagnostics.error=code;diagnostics.stage=text;return false;}
inline bool Thread(){return !diagnostics.restartRequired&&original_players::Thread();}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* parameters){
    auto method=engine::Find(name);
    const uint16_t actual=Value<uint16_t>(method,0xbe);
    if(!object||!method||actual!=size){
        diagnostics.failedEvent=name;diagnostics.expectedParmsSize=size;diagnostics.actualParmsSize=actual;return false;
    }
    auto table=Value<void**>(object,0);auto process=Value<void*>(table,0x298);
    if(!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,method,parameters);return true;
}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    diagnostics.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0001450U;
    const auto at=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=at>=engine::image?at-engine::image:at;
    diagnostics.restartRequired=true;diagnostics.stage="original_enemy_colors_exception_requires_new_session";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool GetSetting(int32_t& result){
    struct Query {uint8_t setting=ColorBlindSetting;uint8_t padding[3]{};int32_t value=-1;} query;
    static_assert(sizeof(Query)==8);
    if(!Event(diagnostics.settings,L"/Script/ShooterGame.AresSettingsManager.GetIntSetting",8,&query))return false;
    result=query.value;return true;
}
inline bool Convert(const Color& color,LinearColor& output){
    struct Conversion {Color input;LinearColor result;} parameters{color,{}};
    static_assert(sizeof(Conversion)==20&&offsetof(Conversion,result)==4);
    if(!Event(engine::DefaultObject(engine::Class(0x4526580)),
        L"/Script/Engine.KismetMathLibrary.Conv_ColorToLinearColor",20,&parameters))return false;
    output=parameters.result;
    return std::isfinite(output.red)&&std::isfinite(output.green)&&std::isfinite(output.blue)&&std::isfinite(output.alpha);
}
inline bool Near(const LinearColor& a,const LinearColor& b){
    return std::fabs(a.red-b.red)<.00001f&&std::fabs(a.green-b.green)<.00001f
        &&std::fabs(a.blue-b.blue)<.00001f&&std::fabs(a.alpha-b.alpha)<.00001f;
}
inline bool ReadPalette(){
    auto library=engine::DefaultObject(engine::Class(0x6c14c60));
    struct ModeQuery {void* world;uint8_t result=0;uint8_t padding[7]{};} mode{diagnostics.world};
    struct ColorQuery {void* world;Color result{};uint8_t padding[4]{};} ally{diagnostics.world},enemy{diagnostics.world};
    static_assert(sizeof(ModeQuery)==16&&sizeof(ColorQuery)==16);
    // C++ storage is16; original UFunction.ParmsSize ends at the returned byte
    // (mode9) or FColor (color12), without the storage's trailing padding.
    if(!Event(library,L"/Script/ShooterGame.ShooterBlueprintLibrary.GetAccessibilityColorBlindMode",9,&mode)
       ||!Event(library,L"/Script/ShooterGame.ShooterBlueprintLibrary.GetAccessibilityAllyColor",12,&ally)
       ||!Event(library,L"/Script/ShooterGame.ShooterBlueprintLibrary.GetAccessibilityEnemyColor",12,&enemy))
        return Fail(1455,"original_enemy_accessibility_palette_query_failed");
    diagnostics.nativeMode=mode.result;diagnostics.ally=ally.result;diagnostics.enemy=enemy.result;
    diagnostics.paletteRead=true;
    if(!Convert(ally.result,diagnostics.allyLinear)||!Convert(enemy.result,diagnostics.enemyLinear))
        return Fail(1456,"original_enemy_palette_linear_conversion_failed");
    diagnostics.linearConverted=true;
    // Read the actual original palette, including its configured opacity.
    // The observed shipped palette returns yellow RGB with alpha128.
    diagnostics.nativeYellow=enemy.result.red>=240&&enemy.result.green>=240
        &&enemy.result.blue<=30&&enemy.result.alpha>0;
    return true;
}
inline bool SampleImpl(){
    if(!Thread()||!diagnostics.applied)return false;
    if(!GetSetting(diagnostics.currentSetting)||!ReadPalette())
        return Fail(1457,"original_enemy_color_setting_readback_failed");
    diagnostics.requestedSettingCurrentlyActive=diagnostics.currentSetting==YellowMode
        &&diagnostics.nativeMode==static_cast<uint8_t>(YellowMode);
    // Read-only proof of the original renderer palette. Native3007590 writes
    // these exact globals, directly or through its original render command.
    // A game-thread dispatch may precede that render command by one frame.
    diagnostics.rendererAlly=Value<LinearColor>(reinterpret_cast<void*>(engine::image),0xd711430);
    diagnostics.rendererEnemy=Value<LinearColor>(reinterpret_cast<void*>(engine::image),0xd711488);
    diagnostics.rendererMatchesPalette=Near(diagnostics.rendererAlly,diagnostics.allyLinear)
        &&Near(diagnostics.rendererEnemy,diagnostics.enemyLinear);
    ++diagnostics.samples;return true;
}
inline bool Sample(){
    __try{return SampleImpl();}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool ApplyImpl(void* world){
    if(!Thread()||world!=original_players::world||!world)return Fail(1450,"original_enemy_colors_requires_original_world");
    if(diagnostics.applied){
        if(diagnostics.world!=world)return Fail(1451,"original_enemy_colors_world_changed_requires_new_session");
        // Keep a user's later original settings selection. This adapter selects
        // the requested initial mode once; it never enforces colors each frame.
        return SampleImpl();
    }
    diagnostics.attempted=true;diagnostics.world=world;
    struct ManagerQuery {void* world;void* result=nullptr;} manager{world};
    if(!Event(engine::DefaultObject(engine::Class(0x6cc9ff0)),
        L"/Script/ShooterGame.ShooterSettingsFacade.GetAresSettingsManager",16,&manager)
       ||!original_players::IsA(manager.result,0x6510860))
        return Fail(1452,"original_enemy_original_settings_manager_required");
    diagnostics.settings=manager.result;
    const int32_t count=Value<int32_t>(manager.result,0xf8),capacity=Value<int32_t>(manager.result,0xfc);
    diagnostics.managerReady=Value<uint8_t>(manager.result,0x890)!=0&&count>0&&count<=4096
        &&capacity>=count&&Value<void*>(manager.result,0xf0)!=nullptr;
    if(!diagnostics.managerReady)return Fail(1453,"original_enemy_colors_waiting_original_settings");
    struct DataQuery {uint8_t setting=ColorBlindSetting;uint8_t padding[7]{};void* result=nullptr;} data;
    static_assert(sizeof(DataQuery)==16&&offsetof(DataQuery,result)==8);
    if(!Event(manager.result,L"/Script/ShooterGame.AresSettingsManager.GetIntData",16,&data)
       ||!original_players::IsA(data.result,0x6bf6cc0))
        return Fail(1454,"original_enemy_colorblind_setting_data_missing");
    diagnostics.settingData=data.result;
    diagnostics.minSetting=Value<int32_t>(data.result,0x124);diagnostics.maxSetting=Value<int32_t>(data.result,0x128);
    diagnostics.dataReady=Value<uint8_t>(data.result,0x120)==ColorBlindSetting
        &&diagnostics.minSetting<=YellowMode&&diagnostics.maxSetting>=YellowMode;
    if(!diagnostics.dataReady||!GetSetting(diagnostics.previousSetting))
        return Fail(1454,"original_enemy_colorblind_setting_data_invalid");
    if(diagnostics.previousSetting!=YellowMode){
        struct Setting {uint8_t setting=ColorBlindSetting;uint8_t padding[3]{};int32_t value=YellowMode;uint8_t saveToRoamingImmediately=0;uint8_t trailing[3]{};} setting;
        static_assert(sizeof(Setting)==12&&offsetof(Setting,value)==4&&offsetof(Setting,saveToRoamingImmediately)==8);
        diagnostics.operation="original_enemy_set_original_colorblind_mode";
        // Original setter75e63d0 owns local setting maps/notification/persistence.
        // The roaming-service option is explicitly false; no account is used.
        if(!Event(manager.result,L"/Script/ShooterGame.AresSettingsManager.SetIntSetting",9,&setting))
            return Fail(1458,"original_enemy_original_colorblind_setter_unavailable");
        diagnostics.setterCalled=true;++diagnostics.setterCalls;
    }
    if(!GetSetting(diagnostics.currentSetting)||!ReadPalette())return false;
    diagnostics.initialSettingVerified=diagnostics.currentSetting==YellowMode
        &&diagnostics.nativeMode==static_cast<uint8_t>(YellowMode)&&diagnostics.nativeYellow;
    if(!diagnostics.initialSettingVerified)return Fail(1459,"original_enemy_original_yellow_palette_not_observed");
    struct Colors {void* world;LinearColor ally,enemy;} colors{world,diagnostics.allyLinear,diagnostics.enemyLinear};
    static_assert(sizeof(Colors)==40&&offsetof(Colors,enemy)==24);
    diagnostics.operation="original_enemy_publish_original_accessibility_palette";
    if(!Event(engine::DefaultObject(engine::Class(0x2a64b40)),
        L"/Script/Renderer.AresOutlineRendering.SetOutlineColorsForRender",40,&colors))
        return Fail(1460,"original_enemy_original_renderer_palette_unavailable");
    diagnostics.rendererDispatched=true;++diagnostics.rendererCalls;diagnostics.applied=true;
    if(!SampleImpl())return false;
    diagnostics.error=0;diagnostics.stage="original_enemy_original_yellow_setting_and_palette_applied";
    diagnostics.operation="none";return true;
}
inline bool Apply(void* world){
    __try{return ApplyImpl(world);}__except(Capture(GetExceptionInformation())){return false;}
}
inline void PrintLinear(FILE* file,const LinearColor& color){
    std::fprintf(file,"[%.9g,%.9g,%.9g,%.9g]",color.red,color.green,color.blue,color.alpha);
}
inline void PrintColor(FILE* file,const Color& color){
    std::fprintf(file,"[%u,%u,%u,%u]",static_cast<unsigned>(color.red),static_cast<unsigned>(color.green),static_cast<unsigned>(color.blue),static_cast<unsigned>(color.alpha));
}
inline void PrintDiagnostics(FILE* file){
    const auto& d=diagnostics;const auto b=[](bool value){return value?"true":"false";};
    std::fputs("{\"stage\":",file);original_players::Text(file,d.stage);
    std::fprintf(file,",\"error\":%u,\"restart_required\":%s,\"exception_rva\":%llu,\"operation\":",d.error,b(d.restartRequired),static_cast<unsigned long long>(d.exceptionRva));original_players::Text(file,d.operation);
    std::fputs(",\"failed_event\":",file);const auto method=original_players::Utf8(d.failedEvent);original_players::Text(file,method.c_str());
    std::fprintf(file,",\"expected_parms_size\":%u,\"actual_parms_size\":%u,\"applied\":%s,\"manager_ready\":%s,\"data_ready\":%s,\"setter_called\":%s,\"initial_setting_verified\":%s,\"palette_read\":%s,\"linear_converted\":%s,\"renderer_dispatched\":%s,\"renderer_matches_palette\":%s,\"native_yellow\":%s,\"requested_setting_currently_active\":%s",static_cast<unsigned>(d.expectedParmsSize),static_cast<unsigned>(d.actualParmsSize),b(d.applied),b(d.managerReady),b(d.dataReady),b(d.setterCalled),b(d.initialSettingVerified),b(d.paletteRead),b(d.linearConverted),b(d.rendererDispatched),b(d.rendererMatchesPalette),b(d.nativeYellow),b(d.requestedSettingCurrentlyActive));
    std::fprintf(file,",\"setting_enum\":%u,\"requested_setting\":%d,\"previous_setting\":%d,\"current_setting\":%d,\"native_mode\":%u,\"minimum_setting\":%d,\"maximum_setting\":%d,\"setter_calls\":%u,\"renderer_calls\":%u,\"samples\":%u,\"world\":\"%p\",\"settings_manager\":\"%p\",\"setting_data\":\"%p\",\"ally_rgba\":",static_cast<unsigned>(ColorBlindSetting),YellowMode,d.previousSetting,d.currentSetting,static_cast<unsigned>(d.nativeMode),d.minSetting,d.maxSetting,d.setterCalls,d.rendererCalls,d.samples,d.world,d.settings,d.settingData);
    PrintColor(file,d.ally);std::fputs(",\"enemy_rgba\":",file);PrintColor(file,d.enemy);
    std::fputs(",\"ally_linear\":",file);PrintLinear(file,d.allyLinear);
    std::fputs(",\"enemy_linear\":",file);PrintLinear(file,d.enemyLinear);
    std::fputs(",\"renderer_ally\":",file);PrintLinear(file,d.rendererAlly);
    std::fputs(",\"renderer_enemy\":",file);PrintLinear(file,d.rendererEnemy);
    std::fputs(",\"runtime_pixels_verified\":false}",file);
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    PrintDiagnostics(file);std::fputc('\n',file);
    const bool result=std::ferror(file)==0;std::fclose(file);return result;
}
}
