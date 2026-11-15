# 国际服 VALORANT 声音方向估计：客户端证据与边界

核对日期：2026-11-08。用户配置：Stereo（立体声）＋游戏内 HRTF 开启。

## 已核对的材料

本次只读检查原安装的 `F:\Riot Games\VALORANT\live\ShooterGame\Binaries\Win64\VALORANT-Win64-Shipping.exe`。原安装与项目 `client` 副本均为 239,005,680 字节，SHA-256 均为 `147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f`。这固定了此次静态结论的对象；不能把它外推到任何未来版本。

REA 成功识别该文件为 x86-64 PE。除此之外，本次用只读字节扫描保留了 [client-byte-evidence.json](client-byte-evidence.json)，其中的偏移都是文件偏移，不是运行时地址。以下字符串确实存在：

| 字符串 | 文件偏移 | 能支持的结论 |
| --- | --- | --- |
| `SPATIALIZATION_HRTF` | `0xbd20f90` | 存在 HRTF 空间化标识 |
| `EAresBoolSettingName::EnableHRTF` | `0xc2bad10` | 存在 HRTF 设置枚举项 |
| `UHRTFSettingsBoolViewModel` | `0xc3eedb0` | 存在 HRTF 设置视图模型标识 |
| `?SetDefaultListeners@SoundEngine@AK@@YA?AW4AKRESULT@@PEB_KI@Z` | `0xd324bd4` | 存在 Wwise 默认监听器接口符号 |

此外有 Wwise v2022.1 构建路径等线索。字符串只能证明字节存在，不能单独证明某函数可达、当前设置生效、某个音效使用了 HRTF，或真实听感。

## 原项目可复核的声音路径

本次重新运行 `python tools/verify_npc_audio_evidence.py`，退出码 0。返回结果为 10 个原始包、130 个完整解码函数、5 个纯音频 profile、3 个观战音频 profile、8 个原生字节 profile，以及 18 个符号栈约束用例通过。该检查针对保留的资源和字节证据，不启动游戏，也不验证声学结果；结果明确标记 `runtime_game_executed=false`、`acoustic_acceptance=false`。没有把历史资源记录中的 `source_blocks_verified` 当成本次重扫了整个现安装 Paks 的证明。

以下来源是当前磁盘上的研究代码与保留解码，不是 Riot 公开源代码：

- [audio_feedback.h](../../native/audio_feedback.h) 使用 `/Script/AkAudio.AkAudioEvent.PostAtLocation`，参数包含位置和朝向。其自身也明确指出，非零 PlayingID 只代表事件接受，不证明已经有可听输出。
- [original_audio_listeners.h](../../native/original_audio_listeners.h) 记录默认监听器集合、监听器所属相机/控制器、位置与主空间监听器。这支持“声源和 listener 的相对关系参与音频路径”的接口层判断，不提供 HRTF 滤波系数。
- [AudioHandlerActor-flow.txt](../../work/original-npc-audio-routing/AudioHandlerActor-flow.txt) 的 `TriggerCharacterGroundSound` 创建或复用世界声源，设置 SurfaceType、Perspective、Occlusion、VolumeMultiplier 和衰减缩放；其 `TriggerCharacterEquippableSound` 也设置 Perspective 与 Occlusion。
- 同一解码文件中的 `TriggerHUDKillConfirmSound` 经 `Aud_Play2d` 播放击杀提示。这是一条明确的二维播放路径，因此不能给“每一种声音”都赋予世界方向。
- [perspective-values.json](../../work/original-npc-audio-routing/perspective-values.json) 和 [original_audio_view_consumers.h](../../native/original_audio_view_consumers.h) 区分 FirstPerson、ThirdPersonEnemy、ThirdPersonAlly。不同视角的声音不能默认使用同一个实测模板。

这些路径说明声音类别、表面、视角、音量与遮挡会参与声效处理。具体声学影响及其幅度仍需实际录音验证。此次没有证明混响、遮挡是否会使感知方向绕到门口，也没有给出其具体传播公式。

## 官方公开说明能确认什么

Riot 在 2021 年的 [2.06 更新说明](https://playvalorant.com/en-us/news/game-updates/valorant-patch-notes-2-06/) 中描述了固定人体测量模型的 HRTF；发布时列出的范围是脚步、换弹和死亡竞赛复活，并建议避免叠加其他三维音效。这是发布时范围，**不能据此宣称当前版本仍只处理这三类声音**。

Riot 的 [8.0 更新说明](https://playvalorant.com/en-us/news/game-updates/valorant-patch-notes-8-0/) 解释了左右声道的时间与频谱差异，并区分内置 HRTF 与 Auto-Detect＋Windows 空间音频路线。用户已选定内置 HRTF，因此应保持这条音频链稳定；更换设备、均衡器、声道处理或空间音效后，原录音模板应重新验证。官方页面没有提供可直接用于逆运算的完整 HRTF 系数表。

## 为什么“位置＋朝向”不足以还原每一个声源

一个简化的短时频域模型是：

`L(f,t) = Σ H_L(f,θᵢ,φᵢ,rᵢ,环境) Sᵢ(f,t) + N_L(f,t)`

`R(f,t) = Σ H_R(f,θᵢ,φᵢ,rᵢ,环境) Sᵢ(f,t) + N_R(f,t)`

这只是建立估计器的模型，不是此次逆向取得的游戏实现。输出只有两个混合通道，未知量包括多个源波形、方位、距离、仰角和环境。没有声源先验或稀疏性假设时，通常不存在唯一逆解。两个源在同一时频单元重叠、弱声被强声掩蔽、无空间方向的 UI/语音、或前后双耳特征相近时，都可能没有唯一方向。

因此，小地图位置和朝向解决的是**坐标变换**，并没有增加用于分离未知波形的音频通道。方向候选来自音频估计；位置只是方向射线的起点。响度还受音效本身、音量、距离衰减和遮挡影响，不能直接当成米制距离。

若约定小地图 x 向右、y 向下，朝向 ψ 从地图上方顺时针增加，相对方位 θ 从正前顺时针增加，则：

`α = (ψ + θ) mod 360°`

`ray(s) = (x₀, y₀) + s × (sin α, −cos α), s ≥ 0`

这给出地图上的方向射线，不是唯一声源坐标。旋转小地图时还要统一地图旋转角；输入朝向应对应实际听音视角。俯仰角或观战视角变化也可能影响模板有效性。

## 程序实现应遵守的证据边界

1. 对单一主导声音，可用左右相位/时间差、频率相关电平比等特征匹配实测方位模板。多个频带各由一个源主导时，可以得到多个候选峰；不能保证候选数等于实际人数或声源数。
2. 未取得该客户端的完整 HRTF 滤波器或足够覆盖的真实校准录音前，不能宣称具有准确的 360° 定位。通用头部半径和声速模型不能代替游戏 HRTF 实测。
3. 模板匹配分数不是经过统计校准的正确概率。需要保留“未知”“前后歧义”“混合/低匹配”等结果，不强制输出一个看似精确的角度。
4. 校准应使用固定音频设置、已知角度、稳定 listener 和单一声源，覆盖脚步/换弹/枪声等实际目标类别、多个距离和场景。验证集应与建模录音分开，并专门测重叠声音、静音、二维提示音和设置变化。
5. 本次静态研究没有运行游戏、没有测得真实录音定位误差，也没有取得各声效当前 HRTF 覆盖率。程序的合成数据测试只能证明实现内部行为，不能代替国际服实录验收。

## REA 检索状态

原安装文件已成功打开，选中的分析器为 Ghidra 12.1.4。首次 `search_strings` 查询（`HRTF|Wwise|SpatialAudio|DefaultListener|PostAtLocation`）在 300 秒后由工具调用层超时，未返回 REA 字符串 Evidence；没有重试全量分析，也没有把普通字节扫描伪装成 REA Evidence。本轮没有获得原生 DSP 反编译结果。随后 `close_binary({})` 成功返回，关闭了本次打开的目标及其分析资源。

这不影响已经完成的原安装指纹、只读字节扫描和保留资源一致性核对，但限制了此次静态分析深度。真实 HRTF 系数和各类声音覆盖范围仍为未解决项。

后续用户授权了实际启动与录音。2026-11-09 的离线原客户端实测已扩展至 Ascent，通过原 OnCharacterGroundSound 录制脚步、方向校准及连续声源/监听器轨迹；具体证据、参数和限制见 [native-fixture.md](native-fixture.md) 及 `.local/sound-locator-recordings/ascent-*`。上文“未运行游戏/声学未验收”描述的是最初静态阶段，不应外推到这些后续实录。实录结果仍不能等同于线上官方服务器或全部音效场景验证。
