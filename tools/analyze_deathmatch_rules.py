"""Read-only original FFA/Deathmatch package and authority evidence survey.

Source containers and the pinned image are read-only. Outputs stay under F:;
the target is never launched and no process, account or network is accessed.
"""
from __future__ import annotations
import sys
sys.dont_write_bytecode = True
from pathlib import Path
import argparse
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import analyze_training_maps as package_reader


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def integration_evidence():
    feedback_path = ROOT / 'reports/deathmatch-feedback-readonly.json'
    feedback = json.loads(feedback_path.read_text(encoding='utf-8'))
    header = ROOT / 'native/deathmatch_rules.h'
    return {
        'rules_header': str(header), 'rules_header_sha256': sha(header),
        'game_thread_apis': {
            'Initialize': 'Initialize(world, Config, Hooks); original mode/factory must already exist.',
            'Poll': 'Poll(GetTickCount64()/1000.0); same monotonic clock as initialization.',
            'Bind': 'Bind(index), including the real local participant; preserves slot generation across removal/re-add.',
            'Unregister': 'Unregister(index) / Unbind(index) before root destroys the real bot Pawn/PC/PS or removes GS.PlayerArray. A pending native damage transaction must finish first.',
            'policy': 'CanDamage(source,victim), AbilitiesAllowed(), BotFireAllowed(), SetConfig(Config).',
            'combat': 'BeginNativeDamage(globalCombatToken,source,victim,equippable,now,DeathData&) -> original ApplyPoint -> EndNativeDamage(success,Result). Begin/End must be paired; no independent score commit.',
            'feedback_view': 'MapKillBannerParameters(BannerParameters&) rewrites only a copy of the active local original Manual48 presentation parameters; original ProcessEvent is forwarded exactly once by the view adapter.',
            'snapshots': 'State(), Player(index), Active(), Remaining(), Leader(), WriteDiagnostics(path).'
        },
        'respawn_hooks': {
            'selectSpawn': 'bool(index,nextSpawnEpoch,SpawnSelection&): FFA requires a real enabled original map anchor; position explicitly distinguishes feet from original Pawn root.',
            'beforeRespawn': 'bool(index,oldPawn,isLocal): release native bot input and retire all old gun/shot/slot observers while identities are still valid.',
            'afterSpawn': 'bool(index,oldPawn,newPawn,isLocal): rebind true inventory/weapon, input, camera, local Jett, HUD, armor and all-source combat. Return accepts native asynchronous equip request; readiness is independently gated by original gun and physics readback.',
            'matchEnded': 'void(winnerIndex,EndReason); winner=-1 for tied time limit.'
        },
        'configuration': {'ffa_kills': 40, 'ffa_seconds': 540,
                          'respawn_delay_seconds_range': [0, 15], 'respawn_delay_seconds_default': 1.5,
                          'streak_timeout_seconds_range': [0.5, 30], 'streak_timeout_seconds_default': 5,
                          'streak_timeout_origin': 'Explicit local policy; original template timer value not established.',
                          'practice_bots_return_fire_default': False, 'local_invincibility_default': False,
                          'ordinary_dm_abilities': False},
        'real_mode_types': {
            'player_state': '/Game/GameModes/Deathmatch/DeathmatchPlayerState.DeathmatchPlayerState_C',
            'ffa_base_state': '/Game/GameModes/Common/FFAPlayerState.FFAPlayerState_C',
            'team_template': 'FFATeam_GEN_VARIABLE, native FFATeamComponent; no BombTeam red/blue field assignment.',
            'game_state': '/Game/GameModes/Deathmatch/DeathmatchGameState.DeathmatchGameState_C',
            'controller': '/Game/GameModes/Deathmatch/DeathmatchController.DeathmatchController_C',
            'owner_exclusive_info': '/Game/GameModes/Deathmatch/DeathmatchPlayerInfo.DeathmatchPlayerInfo_C',
            'native_ffa_team_class_getter_rva': '0x6849cb0',
            'native_base_team_class_getter_rva': '0x65a0b40',
            'native_pair_validation': '/Script/ShooterGame.BaseTeamComponent.IsEnemy, ParmsSize17; all distinct living participants must be enemies.'
        },
        'feedback': {'report': str(feedback_path), 'report_sha256': sha(feedback_path),
                     'conclusions': feedback['conclusions'], 'apis': feedback['apis'],
                     'global_kill_ordinal_preserved': True, 'cumulative_score_cleared': False,
                     'display_kill_count_max': 6, 'display_max_kills': 'max(5,min(streak,6))'},
        'armor_integration': {
            'header': str(ROOT / 'native/original_armor.h'),
            'header_sha256': sha(ROOT / 'native/original_armor.h'),
            'damage_header_sha256': sha(ROOT / 'native/original_damage.h'),
            'factory_evidence': str(ROOT / 'work/original-armor-runtime/grant-evidence.json'),
            'factory_evidence_sha256': sha(ROOT / 'work/original-armor-runtime/grant-evidence.json'),
            'native_data_and_partition_audit': str(ROOT / 'reports/original-armor-native-readonly.json'),
            'native_data_and_partition_audit_sha256': sha(ROOT / 'reports/original-armor-native-readonly.json'),
            'apis': 'Initialize(world), GrantHeavy(index[,pawn]), Sample(index), Armor(index), Retire(index), WriteDiagnostics(path).',
            'root_lifecycle': 'Ordinary DM grants actual HeavyArmor to every original participant. Retire(index) before respawn/removal, then GrantHeavy for the new native Pawn. Practice may keep its original unarmored behavior.',
            'native_reuse': 'HeavyArmor actor and SCS section, original AresItem owner callback, original BasicArmorItem Auth/Client ownership events, original attach sorting/HUD augmentation, native per-section clamping/nonlethal logic and the same original PointDamageResponse/OnDeath.',
            'local_authority': 'Ordinary single Health+Shield partition reads each actual damage type CDO Penetration. Base data has Health target0 amount0.333299994; no copied66% constant. The local authority supplies native LifeChange records after original section calculation.',
            'original_outer_server_partition_recovered': False,
            'zero_shield_alive_policy': 'Local authority uses life>0 for the section alive bit; character alive is derived only from the native health calculation. Original notify consumers copy the supplied section bit; the original authority derivation was not recovered.',
            'unsupported_schema': 'Active nonstandard/overheal sections, multiple shield sections, nonunit Health/Shield/section scales or nonstandard penetration require separate original evidence and are rejected.',
            'native_readbacks': 'ShooterCharacter.GetHealth/GetMaxHealth/GetShield/GetMaxShield and DamageableComponent.GetRawHealth/GetRawShield return the real original floats.',
            'runtime_verified': False
        },
        'integration_limits': [
            'No new header was compiled and no target was executed for this analysis. Runtime confirmation of local death, possession, armor and every rebind is still required.',
            'V2 rules is the sole match-score authority; do not run the old original_training scoring loop or its local-only shot queue in parallel.',
            'HeavyArmor and ordinary Health+Shield local authority are implemented for integration, but armor grant, fractional body damage, lethal head damage and respawn armor readback have not been run for this task.',
            'Original weapon/loadout restoration is owned by the weapon module. Its SaveLoadout/RestoreLoadout/OnPawnReplaced must preserve primary/secondary/melee definitions captured while alive; rules never walks a corpse inventory to reconstruct them.',
            'Map anchor capsule centers are not proved ground feet. Original nav projection and capsule clearance remain required in the root spawn selector.',
            'A failed partially completed native respawn requires the root session failure path; no invented alive flags, substitute animation or proxy hitbox is used.'
        ]
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=Path('F:/Riot Games/VALORANT/live/ShooterGame/Content/Paks'))
    parser.add_argument('--output', type=Path, default=ROOT / 'reports/deathmatch-rules.json')
    parser.add_argument('--use-retained', action='store_true', help='Reuse and hash-check the already exported original package records; no source container reread.')
    args = parser.parse_args()
    if args.source.resolve().drive.upper() != 'F:' or args.output.resolve().drive.upper() != 'F:':
        raise ValueError('This survey is scoped to the F: original source and F: outputs')
    work = ROOT / 'work/deathmatch-rules'
    work.mkdir(parents=True, exist_ok=True)
    package_reader.WORK = work
    descriptions, errors = [], []
    if args.use_retained:
        descriptions = json.loads((work / 'original-packages.json').read_text(encoding='utf-8'))
        for description in descriptions:
            if sha(Path(description['raw'])) != description['sha256']:
                raise ValueError('Retained original package hash mismatch: ' + description['package'])
    else:
        source = package_reader.Source(args.source)
        packages = sorted({package_reader.package_name(filename)
                           for (name, extension), (_, filename, _) in source.index.items()
                           if extension == '.uasset' and (name.startswith('/game/gamemodes/deathmatch/')
                               or name.startswith('/game/gamemodes/common/ffa')
                               or ('deathmatch' in name and 'gamestate' in name))})
        for name in packages:
            try:
                descriptions.append(source.describe(name))
            except Exception as error:
                errors.append({'package': name, 'error': str(error)})
        (work / 'original-packages.json').write_text(json.dumps(descriptions, indent=2), encoding='utf-8')
    catalog_path = ROOT / 'work/client-analysis/continue-world/base-bootstrap/class-catalog.json'
    catalog = json.loads(catalog_path.read_text(encoding='utf-8'))
    class_names = ('ShooterGameMode', 'ShooterGameState', 'ShooterPlayerState', 'KillStreakComponent',
                   'DamageableComponent', 'DamageSectionComponent', 'OwnerExclusivePlayerInfo',
                   'BasicCombatStatsComponent', 'BombTeamComponent', 'NumberedTeamComponent',
                   'SimpleTeamComponent', 'SpawnPointPlayerSpawner', 'ShooterTeamStart')
    classes = [c for c in catalog['classes'] if c['name'] in class_names]
    summary = []
    for description in descriptions:
        summary.append({k: description[k] for k in ('package', 'raw', 'sha256', 'container', 'chunk_id', 'source_blocks_verified', 'imports', 'names')}
                       | {'exports': [{k: e.get(k) for k in ('name', 'type', 'super', 'property_slots', 'properties_start')}
                                      for e in description['exports']]})
    result = {
        'schema_version': 2, 'stage': 'original_mode_assets_native_feedback_and_local_rules_static_review',
        'source': str(args.source), 'source_modified': False, 'target_started': False,
        'process_accessed': False, 'accounts_accessed': False, 'network_used': False,
        'image_sha256': catalog['image_sha256'], 'catalog_sha256': sha(catalog_path),
        'packages': summary, 'package_errors': errors, 'native_classes': classes,
        'local_authority_boundary': [
            'Original DamageResponse, Health/Alive replication handling, death montage, original character construction and possession are reused.',
            'The missing local server selects opponents, clocks, score limit40/time540, respawn scheduling, and supplies exactly-once native combat-result records.',
            'Every registered combatant, including the local player, is a native damage target. Invincibility is an explicit option, not a permanent player exception.',
            'Original shot direction/origin, spread, recoil, gun state machine and native damage tuning remain outside this module.',
            'Feedback counters and resets are separate from original aggregate kills/deaths. No HUD total is cleared to simulate a streak reset.'
        ],
        'current_factory_integration': 'Root owns explicit freeForAll factory selection, the original FFA/DM states and team template, mode-specific controller/GameState/Info, and leaves practice defaults intact. This report does not claim runtime acceptance of that integration.',
        'integration': integration_evidence(),
        'validation': {'static': 'Source-bound package hashes, complete native feedback bodies/Blueprint call layouts, header API/source review. No compile or game run.', 'runtime_verified': False}
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps({'output': str(args.output.resolve()), 'package_count': len(summary),
                      'packages': [x['package'] for x in summary], 'errors': errors}))


if __name__ == '__main__':
    main()
