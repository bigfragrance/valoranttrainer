# 声向：VALORANT Stereo + HRTF 方向分析

独立 Windows 桌面实验程序。读取输出设备的立体声音频，结合已知的玩家位置和朝向，输出**方向候选射线**。默认不读取游戏进程，不自动识别小地图，不把声音强度当作距离。另有仅供本项目离线训练客户端使用的受限校准测试接口。

## 启动

双击项目根目录 `启动声音方向分析.cmd`，或运行 `python sound_locator/app.py`。若已有打包版本，启动器优先打开 `outputs/SoundLocator/SoundLocator.exe`。

1. 保持游戏 **Stereo + 内置 HRTF**，Windows 空间音效关闭。改变输出设备、HRTF、均衡器、虚拟环绕、声道平衡等设置后应重新校准。
2. 选择游戏实际播放声音的输出设备，点击“开始采集”。采集的是该输出上所有程序的混音，不是仅 VALORANT。背景视频、聊天语音和其他程序会污染结果。
3. 输入 X/Y 和朝向，或接入实时位置 JSON。X/Y 是正方形地图画布的 0～1 坐标，左上角为 (0,0)，X 向右，Y 向下；朝向从地图上方顺时针计，右方 90°。
4. 可加载固定朝向的小地图图片。图片按比例居中放入正方形，坐标包含留边。移动时需更新位置，转头时需更新朝向；旋转的小地图必须先统一到固定地图坐标。
5. 未校准只报告左右线索，不输出角度。加载经过验证且与音频设备匹配的校准配置，或按下一节采集。

## 实录校准

校准采集的是左右声道的**相对传递特征**，不是从游戏中提取原始 HRTF 滤波器。

- 在安静、固定条件下，安排一个位置已知的第三人称声源。校准角度是相对你的相机朝向：0° 正前、90° 正右、180° 正后、270° 正左。
- 选择角度，点击“3 秒后录 3 秒”。倒计时方便切回游戏；录制期间保持位置、朝向和声源方向不变，重复播放同一类清晰声效。
- 建议每隔 30° 采一份，共 12 个方向。程序至少需要 8 个不同方向且最大角度间隙不超过 60° 才启用一圈匹配。这只是覆盖检查，不是精度保证。
- 可以同角度重复采样，也可导入已知方向的 stereo WAV。不同声音类型、距离、遮挡状态应分别验证；同一声音的校准不能自动保证其他声音同样准确。
- 每次校准会在 `data/calibrations/<时间>/` 保存原始 WAV、manifest 和 `profile.npz`。新建校准保留历史文件。
- 使用**另一次录制**、未参与校准的音频评估，尤其要测试相邻角度、前后对称角度、不同声音、不同距离和重叠声。不能拿训练样本的匹配率冒充准确率。

若有安静背景的独立录音，可在命令行 manifest 增加 `"background_wav": "background.wav"`，用其估计背景频谱。背景抑制也会漏掉较弱声音；环境变化后需要重新验证。程序可标注特征过于相近的多个方向，并用虚线显示无法区分的方向。

录制长时间静音、方向不明、多声源混合片段作为校准，会使配置失效。程序的信号质量门限只能排除部分问题，不能自动判定标注是否正确。

## 接入已知位置和朝向

程序支持轮询本地 JSON 文件。外部小地图定位器需要至少每秒更新，并用临时文件加原子重命名发布：

```json
{"x": 0.42, "y": 0.61, "heading_deg": 135.0, "timestamp": 1794140000.0}
```

`timestamp` 必须是采集姿态时的当前 Unix 秒，示例值不能直接当实时值。超过 1 秒或明显超前的姿态暂停地图投影。程序从最近历史姿态中选择接近音频窗口中心的一项；音频时间来自设备交付时间的近似值，并非硬件同步。短窗口中转头超过 15° 时暂缓投影。

换算公式：地图方位 `β=(heading_deg+relative_deg) mod 360`；射线 `p(t)=(x,y)+t(sinβ,-cosβ), t≥0`。只知道方向时，无法知道声源在射线上的哪一点。射线长度固定，仅用于展示。

## 命令行

从项目根目录执行：

```powershell
python sound_locator/cli.py devices
python sound_locator/cli.py calibrate path/to/manifest.json path/to/profile.npz
python sound_locator/cli.py analyze input.wav --profile path/to/profile.npz --x .5 --y .5 --heading 90 --out result.jsonl
python sound_locator/cli.py evaluate heldout/manifest.json path/to/profile.npz --out evaluation.json
python -m pytest sound_locator/tests -q
```

Manifest 格式如下，WAV 路径相对于 manifest 所在目录：

```json
{
  "metadata": {"mode": "Stereo + in-game HRTF", "description": "真实录音、环境和输出设置"},
  "clips": [
    {"angle_deg": 0, "wav": "front.wav"},
    {"angle_deg": 90, "wav": "right.wav"}
  ]
}
```

上述两行只演示格式，不满足完整一圈的校准要求。`evaluate` 面向已知方向的单源片段，报告返回覆盖率、首选角度中位误差、90 分位误差和全部有声窗口中的 30° 内比例。它不能自动证明样本独立，也不是多源识别的评估器。

## 实现与局限

- 48 kHz 双声道、1024 点 Hann FFT、256 点 hop；实时使用 160 ms 音频窗口，每 80 ms 分析一次。其他 WAV 采样率会重采样；多声道与单声道输入明确拒绝。
- 250 Hz～12 kHz 每个频点计算右减左的声级差 ILD、右乘左共轭的相位差 IPD。单一稳定声源的相对频谱能部分消掉原声音的频谱。
- 用实际样本估计各角度特征；对时间频率单元进行匹配、相干性筛选和方向投票，再抑制邻近重复候选。
- 多峰可能来自多个声源，也可能来自同一声音的前后歧义、混响或校准偏差。最多显示 3 个候选，**不是 3 个被确认的敌人**。`source_count` 和 `distance` 始终未知。
- `support` 是匹配投票分数，不是正确概率。未校准、覆盖不足、静音、完全相同双声道、双耳特征不稳定或明显失配时不输出角度。
- 无声事件、被混音掩蔽的事件无法恢复。HUD、语音、音乐和第一人称音效也可能是二维或特殊混音，不能被解释为地图上的敌方声源。程序尚无可靠的事件分类器，不能排除所有非世界声效。
- 前后、上下、距离、遮挡、声源随机变化和转头都可能影响结果。这里只估计水平角，没有高度恢复和完整音源分离。

## 已完成的验证

2026-11-08 至 11-09：27 项信号/几何测试通过，包括独立随机种子的合成角度、可分离频段的双源混合、无校准不输出精确角度、静音/单声道/不相干信号拒绝、位置过期处理、不可区分模板标注、背景抑制和配置读写。真实 Windows WASAPI loopback 成功读取 34 个块、65,280 帧，无异常，停止后线程退出。桌面界面已实际打开并验证开始/停止采集。

12 方向合成配置的本机 100 次测量平均分析耗时约 1.45 ms/窗口；这不是完整端到端延迟，也不是游戏精度。真实客户端校准测试及未通过项见 [验证报告](validation/验证报告.md)。随附校准是固定场景的换弹声实验配置，不能直接当作所有脚步、枪声和技能的通用模型。

## 声效系统调查依据

- [Riot 2.06：HRTF 初始说明](https://playvalorant.com/en-us/news/game-updates/valorant-patch-notes-2-06/)：双耳头部相关传递特征提供空间线索；当时的声效覆盖范围不能当作今天的完整清单。
- [Riot 8.0：Stereo、HRTF 与 Windows Spatial Audio](https://playvalorant.com/en-us/news/game-updates/valorant-patch-notes-8-0/)：输出方式会改变双耳处理；Mono 不保留同样的空间信息。
- [Riot 8.07：输出声道配置问题](https://playvalorant.com/en-us/news/game-updates/valorant-patch-notes-8-07/)：错误的多声道配置可能影响耳机听到的声音。
- [Microsoft WASAPI loopback](https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording)：端点回环得到系统最终混音。
- 本地客户端证据和范围：[docs/client-evidence.md](docs/client-evidence.md)。二进制中存在 HRTF/Wwise 标识；本轮没有恢复完整原始 HRTF 系数或全部音频 DSP。

最终左右输出是多个未知声源经过各自空间滤波后的叠加。位置和朝向只提供坐标变换，不能增加被混合、遮蔽或丢失的独立观测，因此无法保证唯一恢复“每一个声音”。
