# Ascent 原脚步事件的神经网络方向头

本模块只负责单源主导事件的双耳方向得分。地图位置、轨迹、已知转头角的时序融合，以及混合声音的多个分量由外层处理。网络没有声源个数输出，也不等于任意数量声音的分离器。

## 可用文件与 API

最终冻结模型：`sound_locator/validation/learned-ascent-onset/learned-model.npz`（281,607 字节）。SHA-256：`4d1abd10b37a25b6436817d0998d7fe02049831d1a04a7db3576f0355b38a91e`。

```python
from learned_audio import LearnedAudioModel
from event_audio import detect_events

model = LearnedAudioModel.load(model_path)
for event in detect_events(stereo_48k):
    window = stereo_48k[event['start_sample']:event['end_sample']]
    result = model.predict(window, heading_deg=known_heading_at_window_center)
    # relative_angles_deg: 0, 5, ..., 355
    # world_angles_deg: (relative angle + known heading) % 360
    # direction_scores: full circular score field, not a calibrated probability
    # status / quality / normalized_entropy: rejection and evidence-quality fields
```

推理只需要 NumPy/现有 SciPy 音频特征，不加载 Torch。训练时才懒加载 Torch，明确使用 CPU，设置两个计算线程。本机重复 200 次事件窗推理平均约 0.62 ms；该数值不包括音频等待和界面刷新。

模型输出为 72 个方向得分，不做 top-K 截断。已知 heading 只做坐标变换，不能把单一窗口变成独立的三角定位观测。相机在 160 ms 内明显转动时，整窗特征可能混合多个相对角；外层应按已知每帧角度校正传统声学分量，或仅在转角很小时使用这个网络。

## 纯音频事件检测

`detect_events(stereo_48k)` 返回 `start_sample / center_sample / end_sample / onset_sample / strength`。检测器不接收事件回执或声源坐标。

- 10 ms 双声道 RMS 块；以前最多 500 ms 的中位 RMS 作为背景。
- 当前块超过背景 2.5 倍、绝对 -50 dBFS 门限，且比前块上升超过 1.4 倍时触发。
- 200 ms 不应期，固定 160 ms 分类窗，其中 20 ms 在检测起点之前。
- 只返回已完整收到的窗口；约在检测起点后 140 ms 才能消费，不是零延迟。
- 同时或近同时的不同声源可能合并成一个 acoustic burst，不能因此只保留一个声源；窗口内所有可信的声学分量仍需交给外层跟踪。
- 尚未构成脚步类别识别器。枪声、环境瞬态等也可能触发，当前模型只在以下条件下验收。

## 网络与数据隔离

对每个 FFT 频点提取右减左 ILD、复相位差的实部/虚部及相干程度，共 1004 个相对特征。没有把绝对单耳频谱作为方向输入。隐藏层是 64-ReLU、64-ReLU，输出 72 个角度得分；训练使用 10° 平滑的圆周目标。

Ascent 完整角色脚步路径通过原 `OnCharacterGroundSound` 调用，固定监听器/声源位置并转动相机，采集 24 个方向。24 个完整校准录音按 18/6 分为训练/选 epoch，绝不把同一录音的重叠窗口分到不同集合。以每份校准录音发声前的 160 ms 片段作均匀得分负例，减少记住背景方向的风险。选出 130 epochs 后，用全部 24 个校准录音重训一次。24 个另录 holdout 不用于 epoch 选择；后续独立 session 的 8 个未见角度最终组完全不参加训练或调参。

事件关联只用于评估/标注：检测到的 onset 与回执时刻差落在 -20～250 ms 内时一对一关联。检测过程和分类窗本身完全由音频决定。回执不是精确硬件声学起点，因此该关联仍有时间测量限制。

## 结果

### Ascent 开发 holdout（独立录音、已见角度）

72 次原脚步触发全部检测到；共检测 78 个 burst，多出 6 个未关联瞬态。

| 方法 | 返回方向的关联事件 | 首选中位误差 | 90 分位误差 | 平均误差 | 全部触发中误差≤30° |
|---|---:|---:|---:|---:|---:|
| 神经网络 | 72/72 | 0° | 9.5° | 5.21° | 70/72（97.22%） |
| DirectionField | 72/72 | 3° | 150° | 35.25° | 51/72（70.83%） |

神经网络仍有两次 100°/110° 大误判，不能只引用中位数。多出的 6 个瞬态中，网络对 4 个不返回方向，2 个返回方向；它们可能是层音或环境瞬态，尚不能归因为新声源。24 个 holdout 发声前背景片段全部不返回方向。

### 冻结后的 Ascent 未见角度最终组

8 个独立录音、24 次触发，24 次均检测到，没有多余检测；角度相对校准网格偏移 7.5°。模型文件 SHA-256 在此次评估前后相同。

| 方法 | 首选中位误差 | 90 分位误差 | 平均误差 | 全部触发中误差≤30° |
|---|---:|---:|---:|---:|
| 神经网络 | 2.5° | 7.5° | 3.96° | 24/24 |
| DirectionField | 1.5° | 49.5° | 11.25° | 21/24 |

网络减少了本组大角度误判，但中位误差并不优于 DirectionField。只有 8 份录音，不能把 24/24 表述为通用“100%准确率”。这个试验没有覆盖其他地图、表面、人物、距离、高低差、转头动态和重叠多源；这些需要独立轨迹实验。

旧斗牛换弹录音仅用于早期方法开发，不作为本次 Ascent 验收依据。

逐事件结果在 `validation/learned-ascent-onset/event-comparison.json` 和 `final-unseen-event-comparison.json`。训练数据来源 `.local/sound-locator-recordings/ascent-footstep-24`；最终组来源 `.local/sound-locator-recordings/ascent-footstep-final-unseen`。

## 检查与复现

9 项测试通过：圆周目标环绕、归一化、完整角度场、模型读写、已知 heading 变换、音量整体缩放不变性、左右互换符号、静音/单声道/不相干噪声拒绝、无信息模型拒绝、多个音频 burst 和未完整窗口处理等。

```powershell
python -m pytest sound_locator/tests/test_learned_audio.py sound_locator/tests/test_event_audio.py -q
python sound_locator/learned_audio.py train .local/sound-locator-recordings/ascent-footstep-24 <新输出目录> --epochs 300
python sound_locator/evaluate_learned_events.py <录音目录> <模型.npz> <声学校准profile.npz> <报告.json> --split all
```

模型的 softmax 数值不是经过统计校准的正确概率，也不是“有这个人”的概率。不能通过强制取最大值替代不确定性和地图轨迹验证。
