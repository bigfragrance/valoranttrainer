# Active work checkpoint (2026-11-09)

This is an engineering checkpoint, not a completed delivery or additional user authorization.

## User's current request

Build an international VALORANT audio-localization program using known player minimap position and heading, Stereo + in-game HRTF. User explicitly authorized launching the game and playing sounds at known coordinates. Latest steering: lower error with a different approach/neural networks, no fixed source-count cap, test real maps (Ascent), use map geometry and moving-footstep trajectories. Assume all camera turn angles are known. Do not return to Duel for acceptance.

## Completed

- V1 `core.py`, `app.py`, `capture.py`, `cli.py`; old executable `outputs/SoundLocator`. V1 report/README are historical and need to be superseded in final docs/launcher.
- Original-client static evidence: `docs/client-evidence.md`; native test entry `native/sound_locator_fixture.h`, command19 guarded stage9; original full `AresSoundHandler.OnCharacterGroundSound` event4 actually produces footstep transients in Ascent. Direct default footstep PostAtLocation failed to prove target sound and is not accepted evidence.
- Ascent recordings: `.local/sound-locator-recordings/ascent-footstep-24` (24 calibration +24 heldout clips); `ascent-footstep-final-unseen` (8 clips, 24 triggers, angles off grid).
- Learned direction model `validation/learned-ascent-onset/learned-model.npz` frozen SHA256 `4d1abd10b37a25b6436817d0998d7fe02049831d1a04a7db3576f0355b38a91e`. Pure NumPy inference; Torch only training. Independent unseen-angle result: 24/24 detected; median2.5°, P90 7.5°, mean3.96°. Same DirectionField baseline P90 49.5°, mean11.25°. See `docs/learned-audio.md`.
- `event_audio.py` audio-only onsets, 160ms windows, about140ms algorithmic wait. No truth input.
- `direction_field.py` continuous3° angular evidence, no top-K=3; TF mixture components; per-STFT-frame exact yaw alignment when turning. `scene.py` combines audio + listener pose + map only. `evaluate_scene.py` separately reads source truth.
- Ascent full sampled graph `maps/ascent.json`, 13911 nodes /18772edges. 36288 original nav projections, 7125 path calls,6816 accepted original complete paths, no guessed wall edges. `map_geometry.py`, mapPNG, graphpreview and receipts. WorldUV: u=y/14336+.8138950893; v=-x/14336+.5732421875; heading=yaw. Multiple floors retained, sampled coverage is not a complete collision/acoustic map.
- Trajectories (controlled moving sound points, actors not naturally walking): `.local/sound-locator-recordings/ascent-trajectories-3/{moving_source,turning_listener,two_sources,translating_listener}` each24s. Files audio.wav, listener_pose.jsonl, source_truth.jsonl, metadata.json. 53/53/102/53 footstep triggers. Pose~21Hz; first scene a299ms gap, translating one duplicated timestamp conflict. GUI conservatively skips unsupported events.
- Independent noncollinear listener session `.local/sound-locator-recordings/ascent-noncollinear-1/noncollinear_listener_motion`:53 triggers,511poses;2.7768m effective baseline,25.466° bearing span. Pose supplied20Hz; originalTeleportTo discrete movement explicitly not natural walking.
- Old particle PF failed: accurate bearing but representative points26–30m wrong, regions lost near source. Preserved raw failed reports `validation/ascent-tracking/*particle-baseline*` and other original outputs.
- New `graph_belief.py` full-node positive support, original sparse transitions, stationary/moving models, density correction, multimodal candidate_regions. Fixed listener retains broad regions and `representative_is_source_location=false`; no fake range. Noncollinear replay representative median1.126m, final~.53m, but large posterior tails remain and position_concentrated false. See `validation/graph-belief/README.md`. Root integrated it as **default** MultiSourceTracker method="graph" in tracking.py; method="particle" preserves baseline. Need rerun complete current integration and report precise limits.
- `map_app.py`: new Chinese desktop Ascent UI, live loopback + timestamped poseJSON, silent recording replay, dynamic unlimited-list track hypotheses, hides point/path if position not concentrated, shows possible regions/angles. Headless lifecycle/pose tests. `build_map_app.py` builds independent outputs/SoundMapLocator with map and direction models, excludes Torch.
- Latest whole `python -m pytest sound_locator/tests -q`: **58 passed** after graph default integration, before new range module.
- CUA actual source UI opened PID316164/window74981176. `mapApp` is CUA binding. Successfully replayed turning_listener, visibly completed52events/24s. This running process still had older particle module loaded; final package needs reopen with graph implementation. CUA completion_summary sent for this part. Old map GUI currently remains open. Prior V1 GUI capture/start/stop verified and closed.

## Active agents / next work

- `/root/audio_evidence` reactivated for **new Ascent distance calibration**. It had restored original default training DLL/build/source snapshot and no game process, but now is rebuilding/using archived fixture for about44x2s clips: calibration2/4/6/10/15/20m×3bearings×2clips; heldout3/5/8/12m×2bearings. Exact original ground/capsule checks, real horizontal distance labels. After recording must stop own game and restore original DLL again. Docs `docs/ascent-recording-evidence.md`, `.local/sound-locator-final-native-build`, restore record.
- `/root/learned_audio` reactivated for optional `range_audio.py`: empirical weak log-distance likelihood plus OOD/footstep class rejection, using new distance calibration ONLY. Do not use trajectory source_truth to train. Proposed API RangeAudioModel.load().predict(event_window,relative_deg,event_class='auto',context_verified=True) -> status/log_distance_grid (log meters)/likelihood/median_m/interval_m/domain_score. If auto classification cannot be justified, explicit footstep experiment mode, default off; do not feed truth event labels into inference.
- `/root/map_geometry` finished graph_belief and full graph. May message for support.
- `/root/map_ui` finished UI modifications and8headless tests. May reactivate to add optional range controls if needed, but no game-focus during recording.

## Remaining integration/delivery

1. Receive new range data/model, evaluate heldout and existing trajectories without truth leakage. Range is optional/weak and limited to actual footstep domain/settings, never universal loudness=distance.
2. Integrate accepted range likelihood into DirectionObservation and GraphBeliefTrack (new optional field), SceneAnalyzer and GUI carefully; keep candidate uncertainties and no hard count cap. Re-run meaningful tests.
3. Run full current graph scene predictions/evaluations for all5 scenes. Report bearing errors separately from position representative/region size; best-candidate and closest-track metrics are optimistic and not identity proof. Overlapping bursts may merge. User's literal infinite simultaneous recovery is physically unidentifiable; dynamic list has no fixed cap and 8-source TF-disjoint synthetic test passed, actual mixture validation only2 sources so far.
4. Rewrite README/current validation report for V2, preserve V1 findings as historical. Update root launcher `启动声音方向分析.cmd` (still V1) and/or add `启动声音地图.cmd` to new app.
5. Build `python sound_locator/build_map_app.py` after final code stable. Inspect bundled exe UI and actual replay/live capture; do not leave only source working. Ship models/map/reports and concise usage. CUA finish protocol requires completion_summary immediately after observed final UI outcome.
6. Verify owned game stopped and original training module restored; source fixtures remain for reproducibility. No official-account login/online-match validation has occurred. Real tests are original international assets in owned offline Ascent environment; actors don't actually walk, emitters move on ground-validated paths. HRTF setting true readback is not proof of complete DSP chain.

No goal tool was created, no user permission is pending. Do not conclude every sound or reliable exact moving-source positions have been solved. Continue practical improvement and candid validation.

## Delivery checkpoint update

Later work completed range_audio calibration/holdout and weak-range integration. Current range model is validation/range-ascent/range-model.npz, SHA256 f2ecffb09506fff2152c83e0e84cd67cc87fb64cddd14713aaaf04d9086715c3. Range defaults OFF in UI; when enabled auto event/OOD gating and same-chain acknowledgement apply. Separate background.wav is used for replay if available. Multi-component events do not share one summed-energy distance estimate.

All68 tests passed. GraphBelief is the default. Current integrated predictions/reports are validation/ascent-range-tracking; moving source representative median3.82m/P90 11.78m, two sources4.35m/P90 12.52m, turning static source.81m/P90 2.97m, noncollinear listener1.13m/P90 2.37m. These are closest-hypothesis diagnostic metrics, not confirmed identity; all these scenes still have broad regions and no position_concentrated frames. Exact movement trajectories remain unproven.

New package outputs/SoundMapLocator (156.8MiB) built with all three frozen models and Ascent graph. Root启动声音地图.cmd and启动声音方向分析.cmd now target it. Packaged executable actually ran live loopback for~19s, then completed24s moving_source replay with range and candidate-path options,51events; no exception. Current package window7149912 / PID280156 was left open displaying this completed replay. CUA completion summary was sent. Source old GUI closed. Original training DLL verified restored to7c2b2c75f0b970699440f3071c3862121dab9e7149541427a48c9b3d351352e1, no owned game remains. All subagents finished. README andvalidation/Ascent_V2_验证报告.md are current V2; README_V1.md preserves old documentation.
