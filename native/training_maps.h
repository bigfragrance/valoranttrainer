#pragma once
#include "original_players.h"
#include <array>
#include <cstring>
#include <cwchar>
#include <iterator>

// Pinned, game-thread-only original LevelStreaming lifecycle adapter.
// Evidence: work/training-map-minimap-static/streaming-api.json and
// reports/training-maps.json. No container edits, fake loaded flags, synchronous
// flush, native callback replacement, or arbitrary user-supplied package loads.
namespace training_maps {
enum class Action : uint8_t { Preserve, LoadCommon, LoadFFA, HideOtherMode, HideBarrier };
struct LayerSpec {const wchar_t* package;Action action;bool alwaysLoaded;};
struct MapSpec {const char* id;const wchar_t* worldPath;const LayerSpec* layers;size_t count;bool supportsFFA;};

// BEGIN GENERATED ORIGINAL MAP CATALOG
// Source: reports/training-maps.json; exact original LevelStreaming package names.
inline constexpr LayerSpec Layers_ascent[] = {
    {L"/Game/Maps/Ascent/Ascent_KillVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Lighting", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_DefPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Gameplay_Dynamic", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_BVPawn", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_VistaAtk", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_Mid", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Audio_Spatial", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_A", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_B", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_Atk", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_Def", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Volumes", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_AtkPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_DefPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_AtkPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_BVProjectile", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Callout_Volumes", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_Env_VFX", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_APathMid", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_VistaB", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_VistaDef", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_Activations", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Audio", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_Vista", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Gameplay", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_NavMesh", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_Birthday", Action::LoadCommon, true},
    {L"/Game/Maps/SharedSubLevels/AuxiliaryZones/AuxiliaryZones", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Art_VistaA", Action::LoadCommon, true},
    {L"/Game/Maps/Ascent/Ascent_Mode_BombMode", Action::HideOtherMode, false},
    {L"/Game/Maps/Ascent/Ascent_Mode_SiteRush", Action::HideOtherMode, false},
    {L"/Game/Maps/Ascent/Ascent_FortCollins", Action::HideOtherMode, false},
    {L"/Game/Maps/Ascent/Ascent_Mode_QuickSpike", Action::HideOtherMode, false},
    {L"/Game/Maps/Ascent/Ascent_Mode_FFA", Action::LoadFFA, false},
    {L"/Game/Maps/Ascent/Ascent_Camera", Action::Preserve, false},
    {L"/Game/Maps/Ascent/Ascent_Mode_Alt8", Action::HideOtherMode, false},
    {L"/Game/Maps/Ascent/Ascent_TeamSpawnPoints", Action::HideOtherMode, false},
    {L"/Game/Maps/Ascent/Ascent_Lanes", Action::Preserve, false},
    {L"/Game/Maps/Ascent/Ascent_Greybox", Action::Preserve, false},
    {L"/Game/Maps/Ascent/Ascent_GreyboxChanges", Action::Preserve, false},
    {L"/Game/Maps/Ascent/Ascent_LightingBlockers", Action::Preserve, false},
    {L"/Game/Maps/Ascent/Ascent_Profiling", Action::Preserve, false},
};
inline constexpr LayerSpec Layers_bonsai[] = {
    {L"/Game/Maps/Bonsai/Bonsai_Art_VistaB", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Volumes", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_BVProjectile", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_DefPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_DefPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_AtkPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Callout_Volumes", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_B", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_AtkPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_AtkSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_VistaAtk", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_VistaA", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Gameplay", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_VistaDef", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_MidVent", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_BTower", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_Sewer", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_MidConstruction", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Audio_Spatial", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Audio", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_ATower", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_BVPawn", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Lighting", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_NavMesh", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_KillVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_ObserverCameras", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_Birthday", Action::LoadCommon, true},
    {L"/Game/Maps/SharedSubLevels/AuxiliaryZones/AuxiliaryZones", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_Activations", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_HappyStore", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_VFX", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Skybox", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_A", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_Mid", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Art_DefSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Gameplay_Dynamic", Action::LoadCommon, true},
    {L"/Game/Maps/Bonsai/Bonsai_Yamtest", Action::Preserve, false},
    {L"/Game/Maps/Bonsai/Bonsai_Greybox", Action::Preserve, false},
    {L"/Game/Maps/Bonsai/Bonsai_FortCollins", Action::HideOtherMode, false},
    {L"/Game/Maps/Bonsai/Bonsai_Mode_BombMode", Action::HideOtherMode, false},
    {L"/Game/Maps/Bonsai/Bonsai_Mode_SiteRush", Action::HideOtherMode, false},
    {L"/Game/Maps/Bonsai/Bonsai_Mode_QuickSpike", Action::HideOtherMode, false},
    {L"/Game/Maps/Bonsai/Bonsai_Mode_FFA", Action::LoadFFA, false},
    {L"/Game/Maps/Bonsai/Bonsai_TeamSpawnPoints", Action::HideOtherMode, false},
    {L"/Game/Maps/Bonsai/Bonsai_Mode_Alt8", Action::HideOtherMode, false},
    {L"/Game/Maps/Bonsai/Bonsai_Camera", Action::Preserve, false},
    {L"/Game/Maps/Bonsai/Bonsai_Lanes", Action::Preserve, false},
    {L"/Game/Maps/Bonsai/Bonsai_Profiling", Action::Preserve, false},
    {L"/Game/Maps/Bonsai/Bonsai_SpawnBarrierVisuals", Action::HideBarrier, false},
    {L"/Game/Maps/Bonsai/Bonsai_GreyboxReference", Action::Preserve, false},
};
inline constexpr LayerSpec Layers_canyon[] = {
    {L"/Game/Maps/Canyon/Canyon_Gameplay", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_BSiteTunnel", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_AtkPathToBSite", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_AtkSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_BackPath", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_BackPathtoA", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_Asite", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_VistaA", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_Audio_Spatial", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_MidCollider", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_VFX", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_BV_Projectile", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_BVPawn", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_CalloutVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_KillVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_ObserverCameras", Action::LoadCommon, true},
    {L"/Game/Maps/SharedSubLevels/AuxiliaryZones/AuxiliaryZones", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_BSite", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_Mid", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_NavMesh", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_Lighting", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_BackPathToB", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_Audio", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_AtkPathToASite", Action::LoadCommon, true},
    {L"/Game/Maps/Canyon/Canyon_GreyboxReference", Action::Preserve, false},
    {L"/Game/Maps/Canyon/Canyon_Mode_FFA", Action::LoadFFA, false},
    {L"/Game/Maps/Canyon/Canyon_Greybox", Action::Preserve, false},
    {L"/Game/Maps/Canyon/Canyon_ASidePath", Action::Preserve, false},
    {L"/Game/Maps/Canyon/Canyon_Mode_SpikeRush", Action::HideOtherMode, false},
    {L"/Game/Maps/Canyon/Canyon_Mode_Alt8", Action::HideOtherMode, false},
    {L"/Game/Maps/Canyon/Canyon_TeamSpawnPoints", Action::HideOtherMode, false},
    {L"/Game/Maps/Canyon/Canyon_BombGameMode_Only", Action::HideOtherMode, false},
    {L"/Game/Maps/Canyon/Canyon_Lanes", Action::Preserve, false},
    {L"/Game/Maps/Canyon/Canyon_APathNoMid", Action::Preserve, false},
};
inline constexpr LayerSpec Layers_duality[] = {
    {L"/Game/Maps/Duality/Duality_Audio", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_A", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Volumes", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_Def", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_Vista", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_AtkPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_AtkPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_DefPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_DefPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_Mid", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_TPArrivalRooms", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_Signage", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Gameplay", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_BVPawn", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_MidPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_MidPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_Lantern", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_Destructibles", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_BVProjectile", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_CalloutVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_KillVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_ObserverCameras", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_Birthday", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_BathHouse", Action::LoadCommon, true},
    {L"/Game/Maps/SharedSubLevels/AuxiliaryZones/AuxiliaryZones", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_B", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Lighting", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_NavMesh", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Audio_Spatial", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_VFX", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Art_Atk", Action::LoadCommon, true},
    {L"/Game/Maps/Duality/Duality_Mode_FFA", Action::LoadFFA, false},
    {L"/Game/Maps/Duality/Duality_SiteRushGameMode_Only", Action::HideOtherMode, false},
    {L"/Game/Maps/Duality/Duality_BombGameMode_Only", Action::HideOtherMode, false},
    {L"/Game/Maps/Duality/Duality_QuickSpikeGameMode_Only", Action::HideOtherMode, false},
    {L"/Game/Maps/Duality/Duality_Mode_Alt8", Action::HideOtherMode, false},
    {L"/Game/Maps/Duality/Duality_TeamSpawnPoints", Action::HideOtherMode, false},
    {L"/Game/Maps/Duality/Duality_Camera", Action::Preserve, false},
    {L"/Game/Maps/Duality/Duality_FortCollins", Action::HideOtherMode, false},
    {L"/Game/Maps/Duality/Duality_Greybox", Action::Preserve, false},
    {L"/Game/Maps/Duality/Duality_GreyboxReference", Action::Preserve, false},
    {L"/Game/Maps/Duality/Duality_Skybox", Action::Preserve, false},
    {L"/Game/Maps/Duality/Duality_Profiling", Action::Preserve, false},
    {L"/Game/Maps/Duality/Duality_SpawnBarrier", Action::HideBarrier, false},
};
inline constexpr LayerSpec Layers_foxtrot[] = {
    {L"/Game/Maps/Foxtrot/Foxtrot_ObserverCameras", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_AtkPath_A", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_DefPath_B", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_Vista", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_AtkPath_B", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_A_Tunnel", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_DefPath_Mid", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Audio", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Audio_Spatial", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_Workshop", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_VistaShip", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Gameplay", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_VFX", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_MidLower", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_MidUpper", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_CalloutVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_KillVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_BVProjectile", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_Birthday", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Lighting", Action::LoadCommon, true},
    {L"/Game/Maps/SharedSubLevels/AuxiliaryZones/AuxiliaryZones", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_BVPawn", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_NavMesh", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_A", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_B", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_Mid", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_DefSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_Art_AtkSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Foxtrot/Foxtrot_GreyboxReference", Action::Preserve, false},
    {L"/Game/Maps/Foxtrot/Foxtrot_Greybox", Action::Preserve, false},
    {L"/Game/Maps/Foxtrot/Foxtrot_Mode_BombMode", Action::HideOtherMode, false},
    {L"/Game/Maps/Foxtrot/Foxtrot_Mode_SpikeRush", Action::HideOtherMode, false},
    {L"/Game/Maps/Foxtrot/Foxtrot_Mode_FFA", Action::LoadFFA, false},
    {L"/Game/Maps/Foxtrot/Foxtrot_TeamSpawnPoints", Action::HideOtherMode, false},
    {L"/Game/Maps/Foxtrot_Copy/Foxtrot_Update_2025", Action::Preserve, false},
    {L"/Game/Maps/Foxtrot/Foxtrot_DesignChanges", Action::Preserve, false},
    {L"/Game/Maps/Foxtrot/Foxtrot_Lanes", Action::Preserve, false},
};
inline constexpr LayerSpec Layers_infinity[] = {
    {L"/Game/Maps/Infinity/Infinity_Lighting", Action::LoadCommon, true},
    {L"/Game/Maps/SharedSubLevels/AuxiliaryZones/AuxiliaryZones", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_DefPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_ASiteVista", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_AtkSpawnVista", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_BSiteVista", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_DefSpawnVista", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_Mid", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_VFX", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_MidAtk", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_MidDef", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Audio", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_AtkSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Audio_Spatial", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_Signs", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Global_Decals", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Barriers", Action::HideBarrier, true},
    {L"/Game/Maps/Infinity/Infinity_BV_Pawn", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_CalloutVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_BV_Projectile", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Navmesh", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_KillVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_EsportsCameras", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Gameplay", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_DefSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_ASite", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_BSite", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_DefPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_AtkPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Art_AtkPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Infinity/Infinity_Cameras", Action::Preserve, false},
    {L"/Game/Maps/Infinity/Infinity_Design_Blockout", Action::Preserve, false},
    {L"/Game/Maps/Infinity/Infinity_Modes_FFA", Action::LoadFFA, false},
    {L"/Game/Maps/Infinity/Infinity_Modes_SpikeRush", Action::HideOtherMode, false},
    {L"/Game/Maps/Infinity/Infinity_Art_Blockout", Action::Preserve, false},
    {L"/Game/Maps/Infinity/Infinity_Temp_Removed", Action::Preserve, false},
    {L"/Game/Maps/Infinity/Infinity_Lanes", Action::Preserve, false},
    {L"/Game/Maps/Infinity/Infinity_Mode_Alt8", Action::HideOtherMode, false},
    {L"/Game/Maps/Infinity/Infinity_TeamSpawnPoints", Action::HideOtherMode, false},
    {L"/Game/Maps/Infinity/Infinity_Modes_BombMode", Action::HideOtherMode, false},
};
inline constexpr LayerSpec Layers_jam[] = {
    {L"/Game/Maps/Jam/Jam_BVPawn", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Gameplay", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_BSite", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_CSite", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_DefPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_DefPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_DefPathC", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_DefSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_DefVista", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_ArtDefPathCExterior", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_DoorBakeRooms", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_VFX", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/JamEsportsCameras", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_CBConnector", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_AtkVista", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_ASiteVista", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Audio_Spatial", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Audio", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_BVProjectile", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_CalloutVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_KillVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/SharedSubLevels/AuxiliaryZones/AuxiliaryZones", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Lighting", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Navmesh", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_ASite", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_AtkPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_AtkPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_AtkPathC", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Art_AtkSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Jam/Jam_Greybox_Reference", Action::Preserve, false},
    {L"/Game/Maps/Jam/Jam_Cameras", Action::Preserve, false},
    {L"/Game/Maps/Jam/Jam_Mode_FFA", Action::LoadFFA, false},
    {L"/Game/Maps/Jam/Jam_Mode_SpikeRush", Action::HideOtherMode, false},
    {L"/Game/Maps/Jam/Jam_Mode_TeamSpawnPoints", Action::HideOtherMode, false},
    {L"/Game/Maps/Jam/Jam_FortCollins", Action::HideOtherMode, false},
    {L"/Game/Maps/Jam/Jam_Greybox_For_Art", Action::Preserve, false},
    {L"/Game/Maps/Jam/Jam_Changes_2023", Action::Preserve, false},
    {L"/Game/Maps/Jam_Copy/Jam_Copy_V4_09", Action::Preserve, false},
    {L"/Game/Maps/Jam/Jam_Mode_BombMode", Action::HideOtherMode, false},
};
inline constexpr LayerSpec Layers_juliett[] = {
    {L"/Game/Maps/SharedSubLevels/AuxiliaryZones/AuxiliaryZones", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_AtkSpawnVista", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_DefSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_AtkPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_Mid", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_BSite", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_ASite", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_DefPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_DefPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_DefSpawnVista", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_ASiteVista", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Barriers", Action::HideBarrier, true},
    {L"/Game/Maps/Juliett/Juliett_Art_ConnectorToB", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_AtkPathB_Interior", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_BSiteVista", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_VFX", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Audio_Spatial", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Audio", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_AtkPathToDefA", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_CalloutVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_KillVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_Activations", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_BVPawn", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_MantisStore", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_LightingTest", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_BVProjectile", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Gameplay", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_AtkSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Navmesh", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Cameras", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Art_AtkPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Juliett_Greybox_Ref", Action::Preserve, false},
    {L"/Game/Maps/Juliett/Juliett_Lighting", Action::Preserve, false},
    {L"/Game/Maps/Juliett/Juliett_FortCollins", Action::HideOtherMode, false},
    {L"/Game/Maps/Juliett/Juliett_TeamSpawnPoints", Action::HideOtherMode, false},
    {L"/Game/Maps/Juliett/Juliett_Modes_BombMode", Action::HideOtherMode, false},
    {L"/Game/Maps/Juliett/Juliett_Greybox_Art", Action::Preserve, false},
    {L"/Game/Maps/Juliett/Juliett_Modes_FFA", Action::LoadFFA, false},
    {L"/Game/Maps/Juliett/Juliett_Modes_SpikeRush", Action::HideOtherMode, false},
    {L"/Game/Maps/Juliett/Juliett_Mode_Alt8", Action::HideOtherMode, false},
    {L"/Game/Maps/Juliett/Juliett_Lanes", Action::Preserve, false},
};
inline constexpr LayerSpec Layers_pitt[] = {
    {L"/Game/Maps/Pitt/Pitt_Greybox", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_Blockmesh", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_ASite", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_Mid", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_Vista", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_DefPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_DefPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_AtkVista", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_DefVista", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_BVista", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Audio", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Audio_Spatial", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Lighting", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_AtkPathMid", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_AtkPathMidToA", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_VFX", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_Signage", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Navmesh", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_EsportsCameras", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_PearlActivation", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Kill_Volumes", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_CalloutVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_BVProjectile", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_BVPawn", Action::LoadCommon, true},
    {L"/Game/Maps/SharedSubLevels/AuxiliaryZones/AuxiliaryZones", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Gameplay", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_DefSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_BSite", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_AtkSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_AtkPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Art_AtkPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Pitt/Pitt_Mode_FFA", Action::LoadFFA, false},
    {L"/Game/Maps/Pitt/Pitt_Mode_BombMode", Action::HideOtherMode, false},
    {L"/Game/Maps/Pitt/Pitt_GreyboxRefrence", Action::Preserve, false},
    {L"/Game/Maps/Pitt/Pitt_Mode_SpikeRush", Action::HideOtherMode, false},
    {L"/Game/Maps/Pitt/Pitt_TeamSpawnPoints", Action::HideOtherMode, false},
    {L"/Game/Maps/Pitt/Pitt_Cameras", Action::Preserve, false},
    {L"/Game/Maps/Pitt_Copy/Pitt_Live_Update_6", Action::Preserve, false},
    {L"/Game/Maps/Pitt/Pitt_Lanes", Action::Preserve, false},
    {L"/Game/Maps/Pitt/Pitt_Mode_Alt8", Action::HideOtherMode, false},
};
inline constexpr LayerSpec Layers_plummet[] = {
    {L"/Game/Maps/Plummet/Plummet_Art_ALink", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_BVProjectile", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_EsportsCameras", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Design_SpawnBarriers", Action::HideBarrier, true},
    {L"/Game/Maps/Plummet/Plummet_Art_PreroundSequences", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Audio_Spatial", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Sound", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_BVPawn", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Art_ASite", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Art_AtkPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Art_AtkPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Art_AtkSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Art_AtkVista", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Art_BLink", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Art_BottomMid", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Art_BSite", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Art_DefPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Gameplay", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Art_DefPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Art_DefSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Art_DefVista", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Art_TopMid", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_CalloutVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Lighting", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_Navmesh", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_VFX", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_LiveStory_Permanent", Action::LoadCommon, true},
    {L"/Game/Maps/SharedSubLevels/AuxiliaryZones/AuxiliaryZones", Action::LoadCommon, true},
    {L"/Game/Maps/Plummet/Plummet_KillVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Juliett/Plummet_FortCollins", Action::HideOtherMode, false},
    {L"/Game/Maps/Plummet/Plummet_TeamSpawnPoints", Action::HideOtherMode, false},
    {L"/Game/Maps/Plummet/Plummet_Design_Greybox_Backup", Action::Preserve, false},
    {L"/Game/Maps/Plummet/Plummet_Art_Cameras", Action::Preserve, false},
    {L"/Game/Maps/Plummet/Plummet_Mode_FFA", Action::LoadFFA, false},
    {L"/Game/Maps/Plummet/Plummet_Mode_BombMode", Action::HideOtherMode, false},
    {L"/Game/Maps/Plummet/Plummet_Mode_Spikerush", Action::HideOtherMode, false},
};
inline constexpr LayerSpec Layers_port[] = {
    {L"/Game/Maps/Port/Port_Art_SnowAll", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_Art_Birthday", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_Art_AtkPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_Art_DefPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_Art_VistaA", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_KillVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_BVPawn", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_BVProjectile", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_CalloutVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_Audio_Spatial", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_Audio", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_Art_VFX", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_NavMesh", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_Art_VistaB", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_ObserverCameras", Action::LoadCommon, true},
    {L"/Game/Maps/SharedSubLevels/AuxiliaryZones/AuxiliaryZones", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_Art_A", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_Art_AtkSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_Art_B", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_Art_DefSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_Art_Mid", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_Art_Vista", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_Gameplay", Action::LoadCommon, true},
    {L"/Game/Maps/Port/Port_Lanes", Action::Preserve, false},
    {L"/Game/Maps/Port/Port_GreyboxReference", Action::Preserve, false},
    {L"/Game/Maps/Port/Port_TeamSpawnPoints", Action::HideOtherMode, false},
    {L"/Game/Maps/Port/Port_Lighting", Action::Preserve, false},
    {L"/Game/Maps/Port/Port_Mode_Alt8_Volumes", Action::HideOtherMode, false},
    {L"/Game/Maps/Port/Port_Mode_BombMode", Action::HideOtherMode, false},
    {L"/Game/Maps/Port/Port_Mode_SpikeRush", Action::HideOtherMode, false},
    {L"/Game/Maps/Port/Port_GreyboxChanges", Action::Preserve, false},
    {L"/Game/Maps/Port/Port_Mode_Alt8", Action::HideOtherMode, false},
    {L"/Game/Maps/Port/Port_Mode_FFA", Action::LoadFFA, false},
    {L"/Game/Maps/Port/Port_Greybox", Action::Preserve, false},
    {L"/Game/Maps/Port/Port_Art_Snowball", Action::Preserve, false},
    {L"/Game/Maps/Port/Port_LightingSnowball", Action::Preserve, false},
};
inline constexpr LayerSpec Layers_rook[] = {
    {L"/Game/Maps/Rook/Rook_Gameplay", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_CalloutVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_BLink", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_BSite", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_DefPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_DefPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_DefPathMid", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_DefSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_Mid", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_VFX", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_VistaA", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Navmesh", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_KillVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_VistaB", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_VistaDef", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Activations", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_TruckSequence", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_AtkSpawn", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Audio", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Audio_Spatial", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Ground", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Lighting_ReflectionProbes", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_Mid_Castle", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Lighting", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_Murals", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_AtkSpawn_Props", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_Crystals", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_BVPawn", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_BVProjectile", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_Signage", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_Decals", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_VistaAtk", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_SaltSkirts", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_LiveStory_Permanent", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_ASiteExterior", Action::LoadCommon, true},
    {L"/Game/Maps/SharedSubLevels/AuxiliaryZones/AuxiliaryZones", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_ALink", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_ASite", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_AtkPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_AtkPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_AtkPathMid", Action::LoadCommon, true},
    {L"/Game/Maps/Rook/Rook_Art_KeyArtCameras", Action::Preserve, false},
    {L"/Game/Maps/Rook/Rook_Mode_FFA", Action::LoadFFA, false},
    {L"/Game/Maps/Rook/Rook_Mode_Alt8", Action::HideOtherMode, false},
    {L"/Game/Maps/Rook/Rook_Lanes", Action::Preserve, false},
    {L"/Game/Maps/Rook/Rook_Design_Blockout_Update2026", Action::Preserve, false},
    {L"/Game/Maps/Rook/Rook_Design_Blockout", Action::Preserve, false},
    {L"/Game/Maps/Rook/Rook_Design_Blockout_Walls", Action::Preserve, false},
    {L"/Game/Maps/Rook/Rook_Art_CharacterScale", Action::Preserve, false},
    {L"/Game/Maps/Rook/Rook_Art_Cameras", Action::Preserve, false},
    {L"/Game/Maps/Rook/Rook_Mode_SpikeRush", Action::HideOtherMode, false},
    {L"/Game/Maps/Rook/Rook_Mode_BombMode", Action::HideOtherMode, false},
    {L"/Game/Maps/Rook/Rook_Cameras", Action::Preserve, false},
    {L"/Game/Maps/Rook/Rook_FortCollins", Action::HideOtherMode, false},
};
inline constexpr LayerSpec Layers_triad[] = {
    {L"/Game/Maps/Triad/Triad_Gameplay", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_ObserverCameras", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Art_B", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Audio_Spatial", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Art_Atk", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Art_Def", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Art_Basalt", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Art_Vista", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Art_AtkPathA", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Art_AtkPathB", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Art_AtkPathC", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Art_Vista_Destruction", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_BVProjectile", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Art_C_Destruction", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_VFX_Destruction", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_LightingDestruction", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_BVPawn", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Callout_Volumes", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_KillVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Art_Birthday", Action::LoadCommon, true},
    {L"/Game/Maps/SharedSubLevels/AuxiliaryZones/AuxiliaryZones", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Art_Activations", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Lighting", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_NavMesh", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Art_Destruction", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Volumes", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Audio", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Art_Skybox", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_Art_A", Action::LoadCommon, true},
    {L"/Game/Maps/Triad/Triad_VFX", Action::Preserve, false},
    {L"/Game/Maps/Triad/Triad_Mode_BombMode", Action::HideOtherMode, false},
    {L"/Game/Maps/Triad/Triad_Mode_Alt8", Action::HideOtherMode, false},
    {L"/Game/Maps/Triad/Triad_TeamSpawnPoints", Action::HideOtherMode, false},
    {L"/Game/Maps/Triad/Triad_BotTrainingMatch/Triad_AIMapMarkup", Action::Preserve, false},
    {L"/Game/Maps/Triad/Triad_BotTrainingMatch/Triad_AI_EngageDistanceCache", Action::Preserve, false},
    {L"/Game/Maps/Triad/Triad_Mode_FFA_EngageDistanceCache", Action::LoadFFA, false},
    {L"/Game/Maps/Triad/Triad_BTIL/Triad_AI_EngageDistanceCache_BTIL", Action::Preserve, false},
    {L"/Game/Maps/Triad/Triad_BTIL/Triad_AIMarkupMap_BTIL", Action::Preserve, false},
    {L"/Game/Maps/Triad/Triad_FortCollins", Action::HideOtherMode, false},
    {L"/Game/Maps/Triad/Triad_Mode_SiteRush", Action::HideOtherMode, false},
    {L"/Game/Maps/Triad/Triad_Mode_QuickSpike", Action::HideOtherMode, false},
    {L"/Game/Maps/Triad/Triad_Mode_FFA", Action::LoadFFA, false},
    {L"/Game/Maps/Triad/Triad_Alt5", Action::Preserve, false},
    {L"/Game/Maps/Triad/Triad_Mode_Alt6", Action::HideOtherMode, false},
    {L"/Game/Maps/Triad/Triad_Art_Working", Action::Preserve, false},
    {L"/Game/Maps/Triad/Triad_Art_C", Action::Preserve, false},
    {L"/Game/Maps/Triad/Triad_Mode_Alt4", Action::HideOtherMode, false},
    {L"/Game/Maps/Triad/Triad_GreyboxReference", Action::Preserve, false},
    {L"/Game/Maps/Triad/Triad_Greybox", Action::Preserve, false},
    {L"/Game/Maps/Triad/Triad_Profiling", Action::Preserve, false},
    {L"/Game/Maps/Triad/Triad_Inactive", Action::Preserve, false},
};
inline constexpr LayerSpec Layers_duel1[] = {
    {L"/Game/Maps/Duel/Duel_1/Skirmish_A_Art_ActivationsA", Action::LoadCommon, true},
    {L"/Game/Maps/Duel/Duel_1/Skirmish_A_Art_FloorA", Action::LoadCommon, true},
    {L"/Game/Maps/Duel/Duel_1/Skirmish_A_BV_KillVolumes", Action::LoadCommon, true},
    {L"/Game/Maps/SharedSubLevels/AuxiliaryZones/AuxiliaryZones", Action::LoadCommon, true},
    {L"/Game/Maps/Duel/Duel_1/Skirmish_A_Art_FoliageA", Action::LoadCommon, true},
    {L"/Game/Maps/Duel/Duel_1/Skirmish_A_Art_FrameA", Action::LoadCommon, true},
    {L"/Game/Maps/Duel/Duel_1/Skirmish_A_Art_LightingA", Action::LoadCommon, true},
    {L"/Game/Maps/Duel/Duel_1/Skirmish_A_Art_RoofA", Action::LoadCommon, true},
    {L"/Game/Maps/Duel/Duel_1/Skirmish_A_Layout", Action::LoadCommon, true},
    {L"/Game/Maps/Duel/Duel_1/Skirmish_A_Art_VFX", Action::LoadCommon, true},
    {L"/Game/Maps/Duel/Duel_1/Skirmish_A_Audio", Action::LoadCommon, true},
    {L"/Game/Maps/Duel/Duel_1/Skirmish_A_BV_Pawn", Action::LoadCommon, true},
    {L"/Game/Maps/Duel/Duel_1/Skirmish_A_Navmesh", Action::LoadCommon, false},
};
inline constexpr MapSpec Catalog[] = {
    {"ascent", L"/Game/Maps/Ascent/Ascent", Layers_ascent, std::size(Layers_ascent), true},
    {"bonsai", L"/Game/Maps/Bonsai/Bonsai", Layers_bonsai, std::size(Layers_bonsai), true},
    {"canyon", L"/Game/Maps/Canyon/Canyon", Layers_canyon, std::size(Layers_canyon), true},
    {"duality", L"/Game/Maps/Duality/Duality", Layers_duality, std::size(Layers_duality), true},
    {"foxtrot", L"/Game/Maps/Foxtrot/Foxtrot", Layers_foxtrot, std::size(Layers_foxtrot), true},
    {"infinity", L"/Game/Maps/Infinity/Infinity", Layers_infinity, std::size(Layers_infinity), true},
    {"jam", L"/Game/Maps/Jam/Jam", Layers_jam, std::size(Layers_jam), true},
    {"juliett", L"/Game/Maps/Juliett/Juliett", Layers_juliett, std::size(Layers_juliett), true},
    {"pitt", L"/Game/Maps/Pitt/Pitt", Layers_pitt, std::size(Layers_pitt), true},
    {"plummet", L"/Game/Maps/Plummet/Plummet", Layers_plummet, std::size(Layers_plummet), true},
    {"port", L"/Game/Maps/Port/Port", Layers_port, std::size(Layers_port), true},
    {"rook", L"/Game/Maps/Rook/Rook", Layers_rook, std::size(Layers_rook), true},
    {"triad", L"/Game/Maps/Triad/Triad", Layers_triad, std::size(Layers_triad), true},
    {"duel1", L"/Game/Maps/Duel/Duel_1/Skirmish_A", Layers_duel1, std::size(Layers_duel1), false},
};
// END GENERATED ORIGINAL MAP CATALOG

inline constexpr size_t MaxLayers=128;
struct Name {uint32_t comparison=0,display=0,number=0;};
struct WeakIdentity {int32_t index=-1,serial=0;};
static_assert(sizeof(Name)==12&&sizeof(WeakIdentity)==8);
struct Layer {
    const LayerSpec* spec=nullptr;void* object=nullptr;void* loadedLevel=nullptr;
    WeakIdentity identity{};Name name{};
    bool matched=false,loaded=false,visible=false,shouldLoad=false,desiredVisible=false;
    bool satisfied=false,identityLive=false,unloadSupported=false;
    uint32_t loadRequests=0,visibilityRequests=0;
};
struct Diagnostics {
    bool initialized=false,ready=false,restartRequired=false,worldIdentityLive=false;
    unsigned error=0;const char* stage="training_maps_not_initialized";
    const MapSpec* map=nullptr;void* world=nullptr;WeakIdentity worldIdentity{};
    char observedWorldPackage[256]{};
    size_t layerCount=0,required=0,missing=0,satisfied=0,ffaCount=0,ffaVisible=0;
    size_t commonCount=0,commonVisible=0,hiddenModes=0,hiddenBarriers=0;
    uint64_t polls=0,loadRequests=0,visibilityRequests=0;
    uintptr_t exceptionRva=0;const wchar_t* failedEvent=L"";
    uint16_t expectedSize=0,actualSize=0;
    uintptr_t expectedExecRva=0,actualExecRva=0;bool functionNative=false;
    std::array<Layer,MaxLayers> layers{};
};
inline Diagnostics diagnostics{};
inline ULONGLONG nextPoll=0;
inline const Diagnostics& State(){return diagnostics;}
inline bool Ready(){return diagnostics.initialized&&diagnostics.ready&&!diagnostics.restartRequired;}
template<class T> inline T Value(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline bool Fail(unsigned code,const char* stage){diagnostics.error=code;diagnostics.stage=stage;diagnostics.ready=false;return false;}
inline bool Thread(){return !diagnostics.restartRequired&&original_players::Thread();}
inline bool Same(const Name& a,const Name& b){return a.comparison==b.comparison&&a.number==b.number;}
inline void* Resolve(const WeakIdentity& id){return id.index>=0&&id.serial>0?engine::function<void*(*)(const WeakIdentity*)>(0x1d4cfa0)(&id):nullptr;}
inline bool Capture(void* object,WeakIdentity& id){
    id.index=Value<int32_t>(object,0xc);if(!object||id.index<0)return false;
    id.serial=engine::function<int32_t(*)(void*,int32_t)>(0x1cf35b0)
        (reinterpret_cast<void*>(engine::image+0xd7b87c0),id.index);
    return Resolve(id)==object;
}
struct Binding {const wchar_t* name;uint16_t size;uintptr_t exec;void* function=nullptr;};
inline Binding methods[]={
    {L"/Script/Engine.GameplayStatics.GetStreamingLevel",32,0x43eb1d0},
    {L"/Script/Engine.LevelStreaming.GetWorldAssetPackageFName",12,0x463fcc0},
    {L"/Script/Engine.LevelStreaming.SetShouldBeLoaded",1,0x4640060},
    {L"/Script/Engine.LevelStreaming.SetShouldBeVisible",1,0x4640160},
    {L"/Script/Engine.LevelStreaming.IsLevelLoaded",1,0x463fbf0},
    {L"/Script/Engine.LevelStreaming.IsLevelVisible",1,0x463fc20},
    {L"/Script/Engine.LevelStreaming.ShouldBeLoaded",1,0x463fc80},
    {L"/Script/Engine.LevelStreaming.GetLoadedLevel",8,0x463fc60}
};
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* parameters){
    Binding* binding=nullptr;for(auto& candidate:methods)if(std::wcscmp(name,candidate.name)==0){binding=&candidate;break;}
    if(!binding||binding->size!=size)return false;
    if(!binding->function)binding->function=engine::Find(name);
    auto fn=binding->function;const auto actual=Value<uint16_t>(fn,0xbe);
    const auto exec=Value<uintptr_t>(fn,0xe0);
    const bool native=(Value<uint32_t>(fn,0xb8)&0x400)!=0;
    diagnostics.functionNative=native;diagnostics.expectedExecRva=binding->exec;
    diagnostics.actualExecRva=exec>=engine::image?exec-engine::image:exec;
    diagnostics.expectedSize=size;diagnostics.actualSize=actual;
    if(!object||!fn||actual!=size||!native||exec!=engine::image+binding->exec){
        diagnostics.failedEvent=name;diagnostics.expectedSize=size;diagnostics.actualSize=actual;
        return false;
    }
    const auto table=Value<void*>(object,0);const auto process=Value<void*>(table,0x298);
    if(!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,fn,parameters);return true;
}
inline bool ReadBool(void* object,const wchar_t* method,bool& value){uint8_t result=0;if(!Event(object,method,1,&result))return false;value=result!=0;return true;}
inline bool IsLoad(Action action){return action==Action::LoadCommon||action==Action::LoadFFA;}
inline bool IsHide(Action action){return action==Action::HideOtherMode||action==Action::HideBarrier;}
inline const MapSpec* Find(const char* id){if(id)for(const auto& map:Catalog)if(std::strcmp(id,map.id)==0)return &map;return nullptr;}

inline bool WorldPackage(void* world,char (&utf8)[256],wchar_t (&wide)[256]){
    // Same pinned UObject FName/Outer ABI and original FName::ToString used by
    // the existing native weapon diagnostics. The string is released normally.
    void* current=world;void* visited[32]{};size_t count=0;
    for(;count<32;++count){
        for(size_t n=0;n<count;++n)if(visited[n]==current)return false;
        visited[count]=current;auto outer=Value<void*>(current,0x28);if(!outer)break;current=outer;
    }
    if(count==32)return false;
    struct String {wchar_t* data=nullptr;int32_t count=0,capacity=0;} text;
    engine::function<void*(*)(const void*,void*)>(0x1a7bf60)(static_cast<uint8_t*>(current)+0x18,&text);
    bool valid=text.data&&text.count>0&&text.count<=256&&text.capacity>=text.count;
    if(valid){
        std::memcpy(wide,text.data,static_cast<size_t>(text.count)*sizeof(wchar_t));
        valid=WideCharToMultiByte(CP_UTF8,0,text.data,text.count,utf8,256,nullptr,nullptr)>0;
    }
    engine::function<void(*)(void*)>(0x18a2f10)(&text);return valid;
}
inline bool WorldContains(void* world,void* object){
    // Original GetStreamingLevel43fcef0 iterates exactly this native array.
    auto data=Value<void**>(world,0x90);const auto count=Value<int32_t>(world,0x98),capacity=Value<int32_t>(world,0x9c);
    if(count<0||count>4096||capacity<count||capacity>8192||(count&&!data))return false;
    for(int32_t n=0;n<count;++n)if(Value<void*>(data,static_cast<size_t>(n)*8)==object)return true;
    return false;
}
inline void* Lookup(Layer& layer){
    struct Parameters {void* world;Name package;uint32_t pad=0;void* result=nullptr;} p{diagnostics.world,layer.name};
    static_assert(sizeof(Parameters)==32&&offsetof(Parameters,result)==24);
    auto receiver=engine::DefaultObject(engine::Class(0x43a9670));
    if(!Event(receiver,L"/Script/Engine.GameplayStatics.GetStreamingLevel",32,&p))return nullptr;
    if(!p.result)return nullptr;
    if(!original_players::IsA(p.result,0x3e270d0)||!WorldContains(diagnostics.world,p.result))return nullptr;
    if(!original_players::IsA(p.result,layer.spec->alwaysLoaded?0x3e271a0:0x3e27300))return nullptr;
    Name actual{};
    if(!Event(p.result,L"/Script/Engine.LevelStreaming.GetWorldAssetPackageFName",12,&actual)||!Same(actual,layer.name))return nullptr;
    return p.result;
}
inline bool Observe(Layer& layer){
    if(!layer.object||Resolve(layer.identity)!=layer.object){layer.identityLive=false;return false;}
    layer.identityLive=true;
    if(!ReadBool(layer.object,L"/Script/Engine.LevelStreaming.IsLevelLoaded",layer.loaded)
       ||!ReadBool(layer.object,L"/Script/Engine.LevelStreaming.IsLevelVisible",layer.visible)
       ||!ReadBool(layer.object,L"/Script/Engine.LevelStreaming.ShouldBeLoaded",layer.shouldLoad)
       ||!Event(layer.object,L"/Script/Engine.LevelStreaming.GetLoadedLevel",8,&layer.loadedLevel))return false;
    // Read only. Native SetShouldBeVisible4647630 writes this exact bit. No
    // ShouldBeVisible UFunction exists in this build; none is fabricated.
    layer.desiredVisible=(Value<uint8_t>(layer.object,0xf8)&0x08)!=0;
    return true;
}
inline bool Apply(Layer& layer){
    const auto action=layer.spec->action;if(action==Action::Preserve)return true;
    const bool load=IsLoad(action),visible=load;
    if((load||layer.unloadSupported)&&layer.shouldLoad!=load){
        uint8_t value=load?1:0;
        if(!Event(layer.object,L"/Script/Engine.LevelStreaming.SetShouldBeLoaded",1,&value))return false;
        ++layer.loadRequests;++diagnostics.loadRequests;
    }
    if(layer.desiredVisible!=visible){
        uint8_t value=visible?1:0;
        if(!Event(layer.object,L"/Script/Engine.LevelStreaming.SetShouldBeVisible",1,&value))return false;
        ++layer.visibilityRequests;++diagnostics.visibilityRequests;
    }
    return true;
}
inline bool PollImpl(bool force=false){
    if(!Thread()||!diagnostics.initialized)return false;
    const auto now=GetTickCount64();if(!force&&now<nextPoll)return true;nextPoll=now+200;
    ++diagnostics.polls;diagnostics.ready=false;
    diagnostics.worldIdentityLive=Resolve(diagnostics.worldIdentity)==diagnostics.world;
    if(!diagnostics.worldIdentityLive)return Fail(2901,"training_maps_world_identity_expired");
    diagnostics.required=diagnostics.missing=diagnostics.satisfied=0;
    diagnostics.ffaVisible=diagnostics.commonVisible=diagnostics.hiddenModes=diagnostics.hiddenBarriers=0;
    // Resolve all required original objects before changing any of them.
    for(size_t n=0;n<diagnostics.layerCount;++n){auto& layer=diagnostics.layers[n];
        const bool required=layer.spec->action!=Action::Preserve;
        if(required)++diagnostics.required;
        if(!layer.object||Resolve(layer.identity)!=layer.object){
            layer.object=Lookup(layer);layer.matched=layer.object!=nullptr;
            if(layer.object&&!Capture(layer.object,layer.identity))return Fail(2902,"training_maps_streaming_identity_failed");
        }
        if(required&&!layer.object)++diagnostics.missing;
    }
    if(diagnostics.missing){diagnostics.stage="training_maps_waiting_original_streaming_objects";return true;}
    for(size_t n=0;n<diagnostics.layerCount;++n){auto& layer=diagnostics.layers[n];
        if(!layer.object)continue;
        if(!Observe(layer))return Fail(2903,"training_maps_original_query_or_identity_failed");
        if(!Apply(layer)||!Observe(layer))return Fail(2904,"training_maps_original_streaming_request_failed");
        const auto action=layer.spec->action;
        layer.satisfied=action==Action::Preserve||(IsLoad(action)?(layer.loaded&&layer.visible&&layer.loadedLevel):
            (!layer.visible&&(!layer.unloadSupported||!layer.shouldLoad)));
        if(action!=Action::Preserve&&layer.satisfied)++diagnostics.satisfied;
        if(action==Action::LoadFFA&&layer.loaded&&layer.visible)++diagnostics.ffaVisible;
        if(action==Action::LoadCommon&&layer.loaded&&layer.visible)++diagnostics.commonVisible;
        if(action==Action::HideOtherMode&&!layer.visible)++diagnostics.hiddenModes;
        if(action==Action::HideBarrier&&!layer.visible)++diagnostics.hiddenBarriers;
    }
    diagnostics.ready=diagnostics.required==diagnostics.satisfied&&(!diagnostics.map->supportsFFA||diagnostics.ffaVisible==diagnostics.ffaCount);
    diagnostics.error=0;diagnostics.stage=diagnostics.ready?"training_maps_original_layers_ready":"training_maps_waiting_original_loading_visibility";
    return true;
}
inline bool InitializeImpl(void* world,const char* mapId){
    if(!Thread())return false;
    const auto spec=Find(mapId);if(!spec)return Fail(2905,"training_maps_map_id_not_in_original_catalog");
    if(spec->count>MaxLayers)return Fail(2906,"training_maps_catalog_capacity_exceeded");
    if(!original_players::IsA(world,0x3122cf0))return Fail(2907,"training_maps_requires_real_world");
    if(diagnostics.initialized&&diagnostics.world==world&&diagnostics.map==spec)return PollImpl(true);
    diagnostics={};diagnostics.map=spec;diagnostics.world=world;diagnostics.layerCount=spec->count;nextPoll=0;
    wchar_t observed[256]{};
    if(!WorldPackage(world,diagnostics.observedWorldPackage,observed)||_wcsicmp(observed,spec->worldPath)!=0)
        return Fail(2908,"training_maps_current_world_does_not_match_catalog");
    if(!Capture(world,diagnostics.worldIdentity))return Fail(2909,"training_maps_world_weak_identity_failed");
    if(!engine::Class(0x3e270d0)||!engine::Class(0x43a9670))return Fail(2911,"training_maps_original_native_classes_unavailable");
    for(size_t n=0;n<spec->count;++n){auto& layer=diagnostics.layers[n];layer.spec=&spec->layers[n];
        engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&layer.name,layer.spec->package,1);
        layer.unloadSupported=!layer.spec->alwaysLoaded;
        if(layer.spec->action==Action::LoadFFA)++diagnostics.ffaCount;
        if(layer.spec->action==Action::LoadCommon)++diagnostics.commonCount;
    }
    if(spec->supportsFFA&&!diagnostics.ffaCount)return Fail(2910,"training_maps_original_ffa_layer_missing_from_catalog");
    diagnostics.initialized=true;diagnostics.stage="training_maps_original_layer_requests";return PollImpl(true);
}
inline LONG CaptureException(EXCEPTION_POINTERS* e){
    diagnostics.error=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0002911U;
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=at>=engine::image?at-engine::image:at;diagnostics.restartRequired=true;diagnostics.ready=false;
    diagnostics.stage="training_maps_native_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* world,const char* mapId){__try{return InitializeImpl(world,mapId);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool Poll(){__try{return PollImpl();}__except(CaptureException(GetExceptionInformation())){return false;}}
inline const char* ActionName(Action action){
    switch(action){case Action::LoadCommon:return "load_common";case Action::LoadFFA:return "load_ffa";
        case Action::HideOtherMode:return "hide_other_mode";case Action::HideBarrier:return "hide_barrier";default:return "preserve";}
}
inline void JsonString(FILE* file,const wchar_t* value){
    std::fputc('"',file);if(value)for(const wchar_t* p=value;*p;++p){const unsigned c=static_cast<unsigned>(*p);
        if(c=='"'||c=='\\'){std::fputc('\\',file);std::fputc(static_cast<int>(c),file);}
        else if(c>=32&&c<127)std::fputc(static_cast<int>(c),file);else std::fprintf(file,"\\u%04x",c&0xffff);}
    std::fputc('"',file);
}
inline bool WriteDiagnostics(const wchar_t* path){
    FILE* file=nullptr;if(!path||_wfopen_s(&file,path,L"wb")||!file)return false;
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"ready\":%s,\"restart_required\":%s,\"world_identity_live\":%s,\"map_id\":\"%s\",\"world_package\":\"%s\",\"polls\":%llu,\"required\":%zu,\"missing\":%zu,\"satisfied\":%zu,\"ffa_count\":%zu,\"ffa_visible\":%zu,\"common_count\":%zu,\"common_visible\":%zu,\"hidden_modes\":%zu,\"hidden_barriers\":%zu,\"load_requests\":%llu,\"visibility_requests\":%llu,\"exception_rva\":%llu,\"failed_event\":",
        diagnostics.stage,diagnostics.error,b(diagnostics.initialized),b(Ready()),b(diagnostics.restartRequired),b(diagnostics.worldIdentityLive),
        diagnostics.map?diagnostics.map->id:"none",diagnostics.observedWorldPackage,
        static_cast<unsigned long long>(diagnostics.polls),diagnostics.required,diagnostics.missing,diagnostics.satisfied,
        diagnostics.ffaCount,diagnostics.ffaVisible,diagnostics.commonCount,diagnostics.commonVisible,diagnostics.hiddenModes,diagnostics.hiddenBarriers,
        static_cast<unsigned long long>(diagnostics.loadRequests),static_cast<unsigned long long>(diagnostics.visibilityRequests),static_cast<unsigned long long>(diagnostics.exceptionRva));
    JsonString(file,diagnostics.failedEvent);
    std::fprintf(file,",\"expected_size\":%u,\"actual_size\":%u,\"expected_exec_rva\":%llu,\"actual_exec_rva\":%llu,\"function_native\":%s,\"layers\":[",
        diagnostics.expectedSize,diagnostics.actualSize,static_cast<unsigned long long>(diagnostics.expectedExecRva),static_cast<unsigned long long>(diagnostics.actualExecRva),b(diagnostics.functionNative));
    for(size_t n=0;n<diagnostics.layerCount;++n){const auto& layer=diagnostics.layers[n];if(n)std::fputc(',',file);
        std::fputs("{\"package\":",file);JsonString(file,layer.spec?layer.spec->package:L"");
        std::fprintf(file,",\"action\":\"%s\",\"object\":%llu,\"loaded_level\":%llu,\"matched\":%s,\"identity_live\":%s,\"loaded\":%s,\"visible\":%s,\"should_load\":%s,\"desired_visible\":%s,\"unload_supported\":%s,\"satisfied\":%s,\"load_requests\":%u,\"visibility_requests\":%u}",
            ActionName(layer.spec?layer.spec->action:Action::Preserve),static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(layer.object)),static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(layer.loadedLevel)),
            b(layer.matched),b(layer.identityLive),b(layer.loaded),b(layer.visible),b(layer.shouldLoad),b(layer.desiredVisible),b(layer.unloadSupported),b(layer.satisfied),layer.loadRequests,layer.visibilityRequests);
    }
    std::fputs("]}\n",file);const bool good=std::ferror(file)==0;std::fclose(file);return good;
}
} // namespace training_maps
