# 可选的脚步经验距离证据

这是给地图概率层的**弱距离观测**，不替代方向模型，也不能把声音响度直接当成真实距离。模型只在校准过的脚步家族、输出设备、音量与空间音频设置保持一致时适用；遮挡、其他地图/站位、其他声音和改动音量都可能使它失效。

## 接口

```python
from range_audio import RangeAudioModel
model = RangeAudioModel.load('range-model.npz')
result = model.predict(
    event_window, estimated_relative_deg,
    event_class='auto', context_verified=True,
    background_samples=independent_background,
)
if result['informative']:
    factor = np.interp(
        np.log(np.maximum(node_horizontal_distance_cm / 100, .01)),
        result['log_distance_grid'], result['likelihood'])
    node_likelihood *= factor
```

`context_verified` 只表示调用者确认音频链未变，不表示知道事件类型。`event_class='auto'` 使用模型自己的相对谱形/包络类型判别与域检查；低得分返回未知。仅做明确脚步实验时才可以显式传 `'footstep'`。枪声等其他类别明确拒绝。

`background_samples` 应是独立安静片段，不能把含有目标脚步的音频当背景。缺省时使用校准初始背景。输出 `log_distance_grid` 的单位是自然对数米；似然至少为 0.70、最大为 1，供外层保守相乘。`median_m` 和 `interval_m` 是模型估计与启发区间，不是保证包含真实坐标的统计置信区间。未知/域外/上下文未确认时返回全 1 的距离似然，不能收窄地图区域。

推理仅用 NumPy；训练才懒加载 scikit-learn。打包桌面程序可排除 sklearn 和 torch，仍能读取 NPZ 推理。

## 建模与数据

新实验在 Ascent 的合法地面点上调用完整 event4 角色脚步路径，监听器固定。校准目标为约 2/4/6/10/15/20 米、三个方位、各两份录音；实际坐标决定标签，实际距离约 1.886～20 米。独立 holdout 使用中间距离与不同方位；12 米一个目标方位没有合法候选，录制器明确跳过，没有补造样本。

- 36 份校准录音、108 次触发中，音频 onset 检测能关联 91 个事件，分布于 32 份录音。4 份较弱录音未形成可用检测，不能忽略这个远场局限。
- 每个录音前 300 ms 用作本地背景频谱估计；声能特征先扣除背景。另有归一化谱形、时间包络及方位的 sin/cos 项。
- 距离是对 log(m) 的 Ridge 经验回归。正则强度只在完整校准录音之间进行分组交叉验证选择；不使用 holdout，也不使用既有五条轨迹真值。
- 类型判别不输入绝对音量或方位；使用 95 个原换弹事件及场景背景作负例，再以另一换弹事件的 19 个独立录音事件确定严格拒绝门限 0.98。负例最大脚步得分为 0.029。这个判别范围仍很窄，不是通用游戏音效分类器。
- 剩余误差与相似样本距离分散度决定区间；log 标准差不小于 0.45，并保留 70% 平坦似然底。

模型冻结文件：`validation/range-ascent/range-model.npz`，约 17.6 KB。SHA-256：`f2ecffb09506fff2152c83e0e84cd67cc87fb64cddd14713aaaf04d9086715c3`。

## 独立中间距离 holdout

7 份录音、21 次触发全部由纯音频检测器检测并关联。推理方位来自已冻结的音频方向神经网络，**没有把真实 source 方位喂给距离模型**。

| 模式 | 返回距离 | 绝对误差中位 | 绝对误差90分位 | 中位乘性误差 | 启发区间覆盖 |
|---|---:|---:|---:|---:|---:|
| 自动类型判别 | 20/21 | 2.12 m | 3.49 m | 1.36 倍 | 17/20（85%） |
| 已知脚步实验 | 21/21 | 2.11 m | 3.42 m | 1.36 倍 | 18/21（85.7%） |

误差仍随方位/路径变化：例如约 4.85 米的一组被估为约 1.73～2.58 米。区间也会漏掉真值，因此应只作为地图滤波的弱因子，不能直接画一个确定位置。系统/游戏音量数值未由录制器读出，只确认实验期间没有主动修改；模型不能自动可靠识别所有音量变化。

完整结果：`validation/range-ascent/holdout.json`。训练记录：`training.json`。原始声学数据：`.local/sound-locator-recordings/ascent-range-1/recordings.json`。既有五条轨迹没有用于模型训练、门限选择或距离回归；应由外层进行独立位置验证。

## 检查

4 项单元测试通过：上下文与事件类别要求、宽区间和弱似然底、模型保存/加载、静音/未知类型/域外拒绝、分类特征的整体增益不变性与距离特征的增益敏感性。

```powershell
python -m pytest sound_locator/tests/test_range_audio.py -q
python sound_locator/range_audio.py evaluate <录音目录> <range-model.npz> <报告.json> --direction-model <learned-model.npz>
```

省略 `--direction-model` 的评估使用真实方位，报告会明确标为 oracle bearing，仅可用于诊断，不能当作实际推理性能。
