#pragma once
#include <array>
#include <cstdint>

// Original Aud_FunctionLibrary, resource SHA25670c199db...6af86. Each listed
// Static function has one IsLocallyControlled query and the original Perspective
// FirstPerson/ThirdPersonAlly/ThirdPersonEnemy branches. No playback is replaced.
namespace original_audio_view_consumers {
inline constexpr const wchar_t* ClassPath=L"/Game/Audio/Core/Aud_FunctionLibrary.Aud_FunctionLibrary_C";
struct Profile {const char* name;const wchar_t* path;int32_t scriptBytes;uint32_t queryResume;};
inline constexpr std::array<Profile,5> Profiles{{
    {"Aud_GetPerspectiveActorTeamPerspective",L"/Game/Audio/Core/Aud_FunctionLibrary.Aud_FunctionLibrary_C.Aud_GetPerspectiveActorTeamPerspective",1264,201},
    {"Aud_PlayOnObjectWithPerspectiveAndTeamSwitch",L"/Game/Audio/Core/Aud_FunctionLibrary.Aud_FunctionLibrary_C.Aud_PlayOnObjectWithPerspectiveAndTeamSwitch",1584,398},
    {"Aud_PlayOnObjectWithPerspectiveAndTeamSwitch_Callback",L"/Game/Audio/Core/Aud_FunctionLibrary.Aud_FunctionLibrary_C.Aud_PlayOnObjectWithPerspectiveAndTeamSwitch_Callback",1593,398},
    {"Aud_PlayOnObjectWithPerspectiveVO",L"/Game/Audio/Core/Aud_FunctionLibrary.Aud_FunctionLibrary_C.Aud_PlayOnObjectWithPerspectiveVO",1716,398},
    {"Aud_PlayOnObjectWithPerspectiveVO_Callback",L"/Game/Audio/Core/Aud_FunctionLibrary.Aud_FunctionLibrary_C.Aud_PlayOnObjectWithPerspectiveVO_Callback",1725,398}
}};
}
