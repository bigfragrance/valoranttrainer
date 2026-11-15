# 原客户端离线录音校准入口

该入口在项目自有的离线 stage9 客户端中播放安装资源中的音效，并通过 Windows WASAPI loopback 录音。初期使用 `AkAudioEvent.PostAtLocation`，后续 Ascent 脚步实验改用原 `AresSoundHandler.OnCharacterGroundSound`。它不代表线上官方服务器对局验证。

## 启动

在没有项目游戏进程运行时构建：

```powershell
python tools/build_native_module.py
```

一次三个角度的短检查：

```powershell
python tools/sound_locator_fixture.py --map ascent --event 4 --listener-point=4720.668457,-4450.544434,402.298492 --world-source=4448.901367,-4893.875977,402.298492 --rotate-listener --angles=-90,0,90 --sets 1 --seconds-per-angle 2.5 --output .local/sound-locator-recordings/example
```

默认设备编号 34 是本次机器上 Realtek 立体声 48 kHz 的 loopback；设备编号会变化，应在另一台机器或设备变化后重新确认。输出目录必须是项目 `.local` 下尚不存在的目录。`--active` 可指定明确自有 stage9 的 `result.active.json`，此时脚本不启动/关闭该会话。

`--event` 白名单：0 为 AK 第三人称换弹 MagOut，1 为第三人称 MagIn_End，2/3 为直接路径的原 HardSoleBoot_Run / QuietSneaker_Run。早期 event2 虽接受事件，但未证明有可辨识脚步输出。**event4 才是后续 Ascent 的原 ground-sound handler 路径**：HardSoleBoot_Run＋Wushu_Run 两个原资源事件，Concrete=1、ThirdPersonEnemy、VolumeMultiplier=1。它调用原 handler 的 SurfaceType/Perspective/Occlusion 和 PostAkEvent 分支；不假定覆盖所有英雄、材质或皮肤。完整事件路径写入每次收据。

`--distance` 单位 cm；默认 1000。`--angles` 是相对 listener 的水平角度（0° 正前、+90° 右侧、-90° 左侧、180° 后方）。`--sets 2` 将第 0 轮标记 calibration、第 1 轮标记 holdout；同一音效类别的留出录音不证明对其他音效类别泛化。

## 受限运行接口

新增命令编号 19，只有 `training_session` 已就绪且 `testActionsAllowed` 为真才接受。普通 stage8 和 local-server 路径不启用此 fixture。Payload 为 `<II4f`（24 字节）：operation、event、angle、distance、height、yaw。operation 0 读取，1 播放，2 保存原 HRTF 设置并启用，3 恢复，4 设置原 Controller 朝向。所有调用在游戏线程执行，坐标/枚举/大小经过检查。

Python 入口使用独立配置副本：当前默认 Ascent、两个参与者、bot 暂停、玩家无敌。它通过原 stage9 的短时输入协议维持零移动和指定朝向，不发送桌面键鼠。静态校准固定一个经过原 Ground/Capsule 验证的地面声源点，转动 listener 采样相对方位；源位置不绕圈穿墙。event4 的发声高度为原胶囊中心（地面加原 capsuleHalfHeight），对应原 FXC_GroundSound 使用 Character.GetActorLocation 的方式。录音前后读取原玩家位置，记录是否发生漂移。

HRTF 的参数布局来自固定客户端元数据：GetBoolSetting 为 2 字节，SetBoolSetting 为 3 字节，EnableHRTF 枚举值 55。设置通过原 AresSettingsManager 方法调用并读回；保存即时漫游服务标志为 false，退出时恢复原值。设置读回仍不等于已经实测该音效的 HRTF DSP。

## 证据与限制

每个 WAV 对应 `recordings.json` 中的角度、事件、采样数、采样率、RMS、峰值、录音状态、文件哈希和逐次原生收据。收据包含实际 listener、相机、世界声源、PlayingID、HRTF 设置读回、主机相对时刻等。`background.wav` 是同会话的背景录音。

非零 PlayingID 只是事件被接受；非零 WAV 也可能包含背景音。录音器自身的 `passed` 仅定义为有非零数据、玩家稳定且无音频状态错误，**不代表定位正确或声音来源已经唯一归因**。定位误差应以独立 holdout 录音比较，而不是校准集回代。

WASAPI 录制的是整个输出设备，其他应用音频会混入。每次发送命令的 host_offset 是主机侧时间，与音频帧有缓冲延迟，适合粗定位事件窗口，不是样本级同步。

event0–3 直接调用原 Wwise event 的空间播放路径，未经过角色脚步动画、SurfaceType/Perspective 切换、AresAudioComponent 的遮挡逻辑。event4 经原 handler，但事件选择、Concrete 表面和敌方视角是显式受控参数，未驱动真实 NPC 动画/步态。这两类验证必须分开。HRTF 设置 true 不能证明该事件的 Wwise State/bus/plugin 实际执行了 HRTF，当前仍未提取 DSP 系数或完整证明该链。

## Ascent 连续轨迹

`tools/sound_locator_ascent.py` 生成 `moving_source`、`turning_listener`、`two_sources`、`translating_listener` 场景。另有 `noncollinear_listener_motion` 使用 `--listener-route` 提供的原 NavGraph 路线。源采用受控世界点序列调用原 ground-sound handler，**不是 NPC 实际行走**。每个地面点与连续声源路径段都经过原 Ground/Capsule/ValidateGroundSegment 检查；每个 listener 站位经过 Ground/Capsule 和原 TeleportTo 验证。

扩展请求为 `<II4f3dII`（56 字节），增加世界坐标、受控 source_id、重置路径验证标志；operation5 是合法地面上的 listener 位置设置。该 fixture 的 source_id 存储仅用于受控实验的路径校验，不是定位算法的声源数限制。命令20由独立 `sound_locator_nav_export.h` 导出 Ascent 原导航投影及完整路径，只有 ready stage9 / Ascent 接受。

每个轨迹目录包含：

- `audio.wav`：连续 48 kHz 双声道录音；`background.wav` 是背景而非静音。
- `listener_pose.jsonl`：`t_audio_s`、采集帧、相机 `world_x/world_y/world_z`（cm）、`yaw/pitch`（度）、玩家位置。目标采样25Hz，报告实际数量和最大间隔，可能有加载引起的缺口。
- `source_truth.jsonl`：仅供评估的 `source_id`、`t_audio_s`、原 `position`、实际 handler 收据。推理不得读取它。
- `metadata.json`：录音、采样和实验限制；平移实验还包含评估用的实际基线与几何 bearing 跨度。

listener 平移目前使用沿原导航路线的离散原 TeleportTo，非自然走路，不能以相邻跳变推断自然运动速度。沿声源同一直线平移的对照几乎没有方位视差，不能据此宣称测得距离；非共线场景需要单独评估。地图、姿态与音频可以作为推理输入，source truth 与含源坐标的评估元数据必须隔离。

构建前的默认模块备份位于 `.local/sound-locator-original-module-backup`。实录结束后默认模块已恢复，最终测试构建归档在 `.local/sound-locator-final-native-build`；再次录制前须先构建。现有校验器原先拒绝当前源树的 `*.generated.h`，此次将 `tools/native_module.py` 的允许模式收窄扩展为可选 `.generated`；仍只接受 `native/` 下规定扩展名的安全文件名。完整实录记录见 [ascent-recording-evidence.md](ascent-recording-evidence.md)。
