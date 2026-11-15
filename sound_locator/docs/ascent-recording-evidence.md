# Ascent 原客户端实录记录

2026-11-09。使用项目自有离线 stage9 的原国际服客户端副本和原安装资源，不进行账号登录，不等同于官方服务器对局。后续测试全部改为 Ascent，早期斗牛数据仅保留作既有证据。

## 实际发声路径

原 `AresSoundHandler.OnCharacterGroundSound` 接受两个原资源事件 HardSoleBoot_Run 和 Wushu_Run，显式设置 Concrete=1、ThirdPersonEnemy、VolumeMultiplier=1。原 AudioHandlerActor 接着处理 SurfaceType/Perspective/Occlusion 并调用 PostAkEvent。该入口返回 bool，不返回 PlayingID，因此收据标记 `handler_accepted=true`、`playing_id_available=false`；其中数值 0 不是失败的 PlayingID，更不是伪造的接受编号。

每次发声前通过原地面和胶囊检查。移动源相邻点还通过原 `ValidateSegment`（胶囊扫掠和沿段支撑地面检查）。源坐标取地面加本角色原胶囊半高；本地试验段地面 z400，发声胶囊中心 z498。事件选择和材质是受控参数，不包含真实 NPC 的动画步态或任意角色自动 ChooseSounds。

HRTF 原设置 API 启用并读回 true，退出时恢复。尚未完整证明此原事件的 Wwise State/bus/plugin HRTF DSP 链，也没有提取 HRTF 滤波器系数。

## 数据集

以下目录均在 `F:\valoranttrainer\.local\sound-locator-recordings`：

| 目录 | 内容 |
| --- | --- |
| `ascent-footstep-probe1` | 固定合法地面声源，listener 转向 -90/0/+90，3 份录音；触发后出现明确左右方向瞬态 |
| `ascent-footstep-24` | 24 个相对角度 × calibration / holdout 两轮，共48份独立录音 |
| `ascent-footstep-final-unseen` | 冻结模型后采8个未见角度，24次原脚步请求；采集者未计算方向预测或误差 |
| `ascent-trajectories-3/moving_source` | listener 固定，受控移动世界点脚步；53事件、500姿态 |
| `ascent-trajectories-3/turning_listener` | source 固定，listener 转头；53事件、512姿态 |
| `ascent-trajectories-3/two_sources` | 两个受控世界点源交错/重叠；102事件、512姿态 |
| `ascent-trajectories-3/translating_listener` | listener沿声源共线方向离散原TeleportTo移动，对照组；53事件、511姿态 |
| `ascent-noncollinear-1/noncollinear_listener_motion` | listener沿原导航折线路径离散原TeleportTo平移，固定source；53事件、511姿态 |
| `ascent-range-1` | 独立可选距离模型数据：36校准＋7留出clip，每份3次完整脚步，共129事件；实际约1.89–20m |

全部轨迹用 WASAPI Realtek stereo 48 kHz loopback 实录，无音频采集状态错误。原 handler 和 ground 验证在所有已记录源事件中均成功。姿态目标25Hz，前三组实际平均约20.81–21.35Hz；最大间隔分别为0.297s、0.047s、0.062s，第一组有首次事件加载缺口。非共线组最大姿态间隔0.094s，不能把采样流当作严格无间断的理想25Hz。

## 位置与视差

初期提出的 focus 站位被当前原地面/胶囊检查拒绝，失败会话 `ascent-trajectories-2` 未产生轨迹音频。随后使用原导航 node747 `(4482,-4268,410)`，经原 Ground/Capsule/Teleport 验证成功，实际地面落在 z400。

源受控路线是既有实测的 cover↔open：`(4720.668457,-4450.544434,402.298492)` ↔ `(4448.901367,-4893.875977,402.298492)`，本次每一步再次检查地面及连通段。它只是发声点随时间移动，演员本身没有沿路线行走。

共线 listener 对照：在实际脚步发声时刻，listener 平移基线223.0cm，但固定源的几何 bearing 跨度仅0.00045°，不足以通过方位三角定位距离。最初无声阶段有一帧旧相机位置，不能用全pose跨度399cm代替有效发声时的几何基线。

非共线 listener 使用原 NavGraph 六点折线，见 `sound_locator/maps/ascent-listener-noncollinear.json`。每个瞬移落点由原地面/胶囊和TeleportTo再次验证。按实际发声时收据，基线 **277.68cm**，固定源几何 bearing 跨度 **25.466°**。这些是受控场景几何量，不是定位器的测量精度；定位误差由主程序独立评估。

## 数据隔离

推理输入仅为每场景的 `audio.wav`、`listener_pose.jsonl` 和全图地图/导航数据。`source_truth.jsonl` 包含每步实际世界发声位置、source_id和时刻，仅用于评估；不要输入模型。`metadata.json` 可能含基线、固定源等评估信息，也不可整体当推理输入。

姿态的 world_x/y/z 是相机位置，另有 actor_position。真值 position 是发声胶囊中心，ground_feet 是地面坐标；NavGraph 节点约高于地面10cm。计算位置误差时应使用XY或正确统一高度基准。

## 构建与恢复

新增/接入代码：`native/sound_locator_fixture.h`、`native/training_session.h` 中只在ready stage9接受的命令19；独立地图代理的 `sound_locator_nav_export.h` 接入命令20。录制器为 `tools/sound_locator_fixture.py` 和 `tools/sound_locator_ascent.py`。`tools/native_module.py` 的安全文件名规则增加可选 `.generated`，以接受当前源树原本已有的四个生成头文件。

MSVC构建通过，实际启动/身份检查、原设置读回、原几何验证、WASAPI录音和进程清理均完成。测试结束后无VALORANT进程，默认训练DLL/build/source-snapshot已恢复原备份且 `verified_build()` 通过。

最终测试构建及完整源快照归档在 `.local/sound-locator-final-native-build`，含 `restore-record.json`。测试DLL SHA-256：`553503c674187b2f15b97294e2f3c838d811b2b57451b62b78af181c9ad07764`。恢复后默认DLL：`7c2b2c75f0b970699440f3071c3862121dab9e7149541427a48c9b3d351352e1`。若再次录制，须先构建包含fixture的新模块；恢复后的旧默认DLL不含命令19/20。

## 独立距离校准补充

使用 `tools/sound_locator_range.py` 在固定的合法 node747 站位录制，source从原 NavGraph 的同连通分量、同楼层选取，并再次通过原 Ground/Capsule 验证。距离采用每次原收据中 source−listener 的实际水平长度，不把目标2/4/6/10/15/20m强制当成实际值。

校准覆盖6个目标距离、3个方向，每个距离/方向录两份约2.05s clip，各3次间隔0.55s脚步，得到36份、108次事件。留出使用约3/5/8/12m及不同方位；12m目标-75°没有候选通过检查，明确跳过，最终7份、21次事件。每clip第一事件在0.4s之后，至少保留0.3s当地背景；另有初始 `background.wav`。每个新点有一次记录在point_checks中的原生点验证发声，位于训练clip之外。

所有片段保持同一设备、HRTF设置与事件参数；系统音量和游戏音效音量的数值未读出，标为unknown，采集者未修改它们。原handler遮挡路径启用，但具体遮挡系数未读出，不从地图或音量凭空填写。环境背景随时间变化且采样按距离递增，学习阶段应使用各clip局部背景，不应把背景变化当距离；响度也不能直接等于距离。

`ascent-range-1/data-audit.json` 核对43份WAV的48kHz立体声、样本数与哈希，以及129个原handler/ground/HRTF收据，全部通过。listener位置全程未变，距离标签最大浮点舍入差约0.000055cm。这是完整性与几何检查，不保证所有远处脚步可辨认，更不表示距离模型已经准确。

此次追加录制复用了归档测试DLL，完成后再次正常停止游戏，将原默认模块从新的range备份恢复，`verified_build()`通过。恢复收据在 `ascent-range-1/module-restore.json`；默认DLL仍为上列原哈希。旧轨迹真值没有用于此次采集或距离模型拟合。
