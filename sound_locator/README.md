# 声迹 V2：Ascent 声音方向、位置候选与路径

Windows 桌面研究程序。输入是立体声音频，以及准确、带时间戳的玩家位置和视角；使用实录神经网络、逐短时帧转角对齐和原游戏导航图，维护多个声源的**位置与轨迹假设**。没有固定的三个声源上限，但不能保证从两个混合声道中分离任意数量、任意重叠的声音。

已在国际服原客户端的 **Ascent 离线环境**启动游戏，使用正常脚步声音入口实录验证。验收采用 Ascent；旧斗牛结果只保留为历史研究。

## 打开程序

双击项目根目录 `启动声音地图.cmd`，或打开 `outputs/SoundMapLocator/SoundMapLocator.exe`。源码入口为 `python sound_locator/map_app.py`。

### 最快查看实际录音

点击“回放录音会话”，选择以下任一目录：

```text
F:\valoranttrainer\.local\sound-locator-recordings\ascent-trajectories-3\moving_source
F:\valoranttrainer\.local\sound-locator-recordings\ascent-trajectories-3\turning_listener
F:\valoranttrainer\.local\sound-locator-recordings\ascent-trajectories-3\two_sources
F:\valoranttrainer\.local\sound-locator-recordings\ascent-noncollinear-1\noncollinear_listener_motion
```

回放只分析声音，不播放音频。推理读取 `audio.wav`、`listener_pose.jsonl`，可选背景录音；**不读取 `source_truth.jsonl`**。后者只用于独立评估。程序不把已知真值画成自己的预测结果。

### 实时使用

1. 保持游戏 **Stereo + 内置 HRTF**，选择游戏实际输出的双声道设备。
2. 用“接入姿态 JSON”连续提供位置和视角，再点击“实时采集”。这里按你的要求假设转角已知；程序没有自动识别小地图或读取游戏内存。
3. 手动地图点击和朝向输入只适合静止位置或演示。走动、转头时，必须更新同步姿态，否则地图投影不成立。
4. 默认启用 Ascent 脚步方向网络。其他地图、材质、声音种类、音频设备和设置尚未全面验证。
5. “同一音频设置：启用脚步距离实验”默认关闭。它只在自动类型/域检查通过时提供弱距离线索；设备、音量、HRTF 等必须与校准一致。未知音效、多成分混音或不匹配样本可能被拒绝。

系统回环捕获所选输出上的全部声音，包括语音聊天、视频和其他应用。它不是进程级游戏音轨。

## 位置与视角接口

持续原子更新本地 JSON，建议至少 20 Hz；`timestamp` 使用实际采样时刻的 Unix 秒：

```json
{"timestamp": 1794200000.123, "world_x": 4482.0, "world_y": -4268.0, "world_z": 576.0, "yaw": -85.0}
```

世界坐标单位为厘米，yaw 是 Unreal 水平角。也可提供 Ascent 小地图归一化坐标：

```json
{"timestamp": 1794200000.123, "x": 0.52, "y": 0.26, "heading_deg": 275.0}
```

示例时间戳不是实时值。地图 x 向右、y 向下，朝向从上方顺时针计。超过 1 秒的实时姿态拒绝使用；事件窗口跨超过 150 ms 的姿态缺口或冲突时间戳时，GUI 会跳过并报告，避免假装精确同步。WASAPI 交付时间与姿态时钟仍是近似对齐，未做到硬件同步。

Ascent 原坐标变换：`u=y/14336+0.8138950893`，`v=-x/14336+0.5732421875`。使用完整、未裁剪的原小地图。

## 怎样理解显示

- 彩色候选区域、编号都是假设，不能等同于已确认的敌人或人数。
- 只转头可以消除部分前后歧义，不能单独测出距离。监听器横向移动产生的视差、地图结构和弱距离模型才可能缩小位置范围。
- 未收敛时不把任意代表节点画成精确位置；候选范围不是经过统计校准的置信区间。
- 可选“显示候选路径（可能多解）”显示沿原导航图连接的历史假设；断连、过长或楼层不明时不画，虚线不是已确认的真实移动轨迹。
- 持续脚步由时序模型关联，保留静止和移动两种解释；转头按每个短时音频帧对齐。相同方向或同时起声的多个声音仍可能合并，交叉轨迹也可能交换编号。
- 原导航图限制移动路径，但不等于完整墙体/声学传播模型；高低差、门、遮挡和未采样狭窄区域仍有误差。

## 实测结果与范围

Ascent 独立未见角度组：8 份录音、24 次脚步全部检测到，方向网络首选误差中位 **2.5°**、90 分位 **7.5°**。同批模板方法的90分位为49.5°。这不是通用“100%准确率”。

连续移动、转头、两源重叠和监听器平移均已实际录音。位置仍可能多解；加入独立脚步距离模型后，受控场景的位置主峰诊断误差明显减小，但移动/双源尾部误差仍可超过十米，界面继续保留不确定区域。

详细结果：[Ascent V2 验证报告](validation/Ascent_V2_验证报告.md)。方法：[方向网络](docs/learned-audio.md)、[弱距离模型](docs/range-audio.md)、[地图数据](maps/README.md)、[真实客户端录音证据](docs/ascent-recording-evidence.md)。

## 开发与复现

```powershell
python -m pytest sound_locator/tests -q
python sound_locator/build_map_app.py
python sound_locator/scene.py <会话目录> --profile sound_locator/validation/ascent-footstep-baseline/profile.npz --neural sound_locator/validation/learned-ascent-onset/learned-model.npz --out scene.json
```

同音频链的可选距离实验加 `--range-model sound_locator/validation/range-ascent/range-model.npz --same-audio-chain`。推理参数没有声源真值路径。评估单独调用 `evaluate_scene.py`。

原客户端测试接口仅允许明确的自有 stage9 会话；日常训练 DLL 已恢复，研究源文件与测试构建留存。后续重录前须按 [native-fixture.md](docs/native-fixture.md) 重新准备对应测试构建。

旧程序和旧实验保留在 `app.py`、`core.py`、[README_V1.md](README_V1.md) 与旧验证目录，不能用它们替代当前版本验收。
