# 本机训练控制台

双击 `F:\valoranttrainer\启动训练控制台.cmd`，打开 <http://127.0.0.1:8790>。点击“开始训练”。PowerShell 也可运行 `.\dev.ps1 play`。重复打开入口会复用本项目的控制台。

控制台只监听本机。当前为项目运行版，使用本机已有 Python 和匹配的国际服资源；运行日志、配置及缓存保存在 F 盘。国际服安装目录保持只读，不进行登录。

## 操作

WASD 移动，鼠标转向，空格跳跃，Shift 静步，Ctrl 下蹲，R 换弹，左右键射击/开镜。1/2/3 与滚轮沿用原装备切换，B 打开原购买页，Esc 打开原设置，F6 切换无限子弹，F10 退出训练。架枪模式保留捷风技能；个人乱斗关闭技能。

默认装备捷风和已保存的冠军狂徒外观。武器、皮肤和特效选择会保存在 `.local/controller/config.json`。原购买页沿用原购买间隔；WebUI 的切枪请求实时提交，原装备动画需要正常完成。

架枪中 C 为烟雾、Q 为起飞，E 先准备再按一次冲刺，X 为飞刀。飞刀左键单发，左键击杀回补；右键释放剩余飞刀，击杀不回补。技能使用原资源与状态机。

## 架枪练习

在原小地图上依次选择起点、露出点和撤回点，再保存路线。可保存多条路线、调整权重、单独禁用；新出生的机器人从选中路线的起点开始首次拉出。默认两条斗牛一横移路线用于直接开始练习，可按掩体重新选点。

“地图 Z”用于楼层高度；进入当前地图后自动取玩家高度。客户端运行时，选点会调用原导航、地面和站立胶囊检查。路线检查逐段验证闭环，不可走的路线会显示原因。

拉出方式支持跳拉、大拉、小拉、蹲拉、Peek 和随机。路线样式选“跟随全局”时使用右侧设置。实际频率也受路线长度和原动作时间影响。机器人默认只拉出，可开启还击和玩家无敌；“暂停机器人”便于选点。

## 武器和外观

支持当前资源中的 20 把枪及近战、1,433 个皮肤条目、2,723 个等级条目和 2,972 个炫彩条目。选择武器后搜索皮肤，选择等级和炫彩并应用；加载和原装备流程结束后生效。

选皮肤页提供原二维枪图大图和列表缩略图，等级、炫彩切换后会同步刷新。没有单独炫彩图时会明确标出回退来源；原资源缺图时保留外观应用功能。图片按需读取，缓存保留在 F 盘项目中，动态特效仍在游戏内查看。

特殊效果按当前外观显示。冠军描边可遵循原领先规则、强制开启或关闭。支持的冠军皮肤还可调整击杀递进效果；该数值不改变战绩。其他特效覆盖在对应射击、换弹或检视动作触发时生效。设置、请求和实际生效状态分别保留。

## 人机个人乱斗

在“人机乱斗”选择人数和地图，调整反应时间、拉枪速度、准度及身法的均值与同局差异。每个机器人拥有自己的能力参数，同局死亡后保留；行为噪声继续变化。地图和人数变更会重新建立本次离线场景。

当前资源有 30 位可玩英雄，每局最多 30 人。你使用捷风，机器人从其余 29 位随机且不重复分配；重开重新抽取，同局死亡保持英雄。顶部沿用原个人乱斗排名条，显示每个人短时间内的连杀数，并按当前连杀数从左到右排序；连杀时限由原版组件和计时器控制，随连杀阶段变化，超时或死亡后归零，并显示原版连杀窗口 HUD。

先到 40 杀或 9 分钟结束，死亡后重生。使用原重甲、出生保护、击杀补弹和回血包资源及表现，补齐本地权威入口。连续击杀保留连杀反馈，间隔超时或自己死亡后重置，总击杀继续累计。

地图列表标出运行验证状态。逐图检查覆盖原关卡加载、四名真实英雄出生和短程导航移动，未遍历全部角落与地图机关。原射击、后坐力、动画、皮肤和 UI 由客户端执行；缺失的本地生成、计分、伤害权威记录由项目补写。

## 诊断

控制台日志在 `.local/controller/logs`，各次运行入口和结果在 `.local/training-sessions`；原客户端日志位于对应 `.local/native-baseline/<时间>/runtime.log`。自动验证的逐项结果在 `.local/feature-verification`，地图结果在 `.local/map-verification/results.json`。

## 本地服务器开发候选

拟人人机日常入口是根目录 **`启动本地服务器控制台.cmd`**，或 `.\dev.ps1 play-local`，使用 `http://127.0.0.1:8792` 和独立持久设置 `.local/controller/local-server-candidate/humanlike-bots.json`。首次为 schema3、12 人 Ascent、拟人人机启用，无限子弹继承原保存偏好；后续启动不重置已保存设置。详见 [入口、配置及验收范围](LOCAL_SERVER_LAUNCH.md)。下列 8791、`settings-fixtures` 和直接配对命令保留作独立开发诊断，不是该日常入口的设置来源。

候选后端需单独启用，当前默认训练入口保持原流程。运行 `python -B tools/training_server.py --backend local-server-candidate --port 8791`，再打开 <http://127.0.0.1:8791>。页面会标明“开发候选 · 尚未通过可玩验收”；已有其他后端的端口不会被替换。

2026-11-04 起，候选控制台启动 host 时会明确选择原 `ShooterGameMode` 及原公共移动时钟。直接运行训练配对时应加 `--interactive --original-host-mode`，例如 `python -B tools/run_local_server.py --interactive --original-host-mode --controller-settings .local/local-server/settings-fixtures/candidate-webui-practice-minimal.json`。原 DefaultPawn 网络证明仍保留独立入口。

候选设置保存在 `.local/controller/local-server-candidate/config.json`。首次使用时从原 `.local/controller/config.json` 复制已保存偏好及 revision，之后各自保存，不再自动同步。候选的启动快照、控制台日志和服务记录分别位于该候选目录的 `sessions/`、`logs/` 和 `server-<端口>.json`，配对本身仍位于 `.local/local-server/pair-<时间>/`。

临时最小测试可运行 `python -B tools/training_server.py --backend local-server-candidate --port 8792 --settings-file .local/local-server/settings-fixtures/candidate-webui-practice-minimal.json`。`--config` 是 `--settings-file` 的别名；现有完整设置 JSON 会直接使用，新文件首次从原偏好初始化。候选设置参数仅接受本项目 `.local` 内的独立 JSON，并拒绝原控制台的设置与记录目录。`GET /api/identity` 返回实际 `settings_file`，相同端口只有后端及设置文件都一致时才会复用；早先仍共享原设置的候选服务需重启后再使用该入口。

候选接受目录中的斗牛一架枪模式，以及全部 `standard_5v5` 地图的架枪与人机个人乱斗配置。启动前要求候选构建快照包含完整 Initial/V2 控制、运行时与比赛实现，旧的网络阶段模块无法直接启动该训练路径。地图、模式与人数变更会重启本次配对；切换训练模式保留所选 5v5 地图。目录外地图、诊断地图及斗牛一乱斗会在保存前拒绝。接入范围与实际验证结果分别显示。

点击开始后，由一个本地监督进程启动配对的 host/client，最长 24 小时。两端使用所选地图的独立 training bootstrap；host 绑定本机地址并完成运行时、登录工厂初始化后，才允许启动 client。停止按钮请求该配对会话收尾，客户端先退出，随后关闭 host 的原网络驱动。客户端 F10 退出也会触发配对收尾。host 使用 `-nosound`，画面、菜单输入与音频由 client 承担。

候选沿用现有设置、路线、武器、皮肤和特效格式。配置与装备命令发送到已核验身份的 host V2 控制通道，原菜单显示命令发送到配对 client；API 的 `acknowledged` 表示是否已收到游戏线程回执，装备及特效的实际结果仍以状态快照为准。host 监听前读取配对目录的 `session-initial.bin`。

每次配对的身份记录、进度与收尾报告保存在 `.local/local-server/pair-<时间>/`，其中 `verification.json` 分别记录原网络阶段检查和进程收尾检查。控制通道存在、收到回执或进程正常退出均不表示候选已通过可玩验收。

有界检查可使用 `python -B tools/run_local_server.py --controller-settings <本项目.local下的设置文件> --observe-seconds 20`。它读取同样的地图、模式、路线与出生点 Initial，并让两端使用 `--map <地图ID> --mode training`。不提供设置文件时保留原来的斗牛一导航阶段检查；独立 `proof` 后端始终限定斗牛一导航，不继承候选训练模式。候选训练路径只加载候选模块。

地图结果独立读取 `.local/local-server/map-validation/results.json`，不继承旧训练器的 `training-maps.json` 运行验证结果。索引采用 `schema_version: 1`、`backend: local-server-candidate`、`source: local-server-map-validation-v1`；`cases` 条目按 `map_id`、`mode`、`module_sha256` 和 `evidence` 指向本项目 `.local/local-server/` 内的检查报告。报告需使用 `schema_version: 1`、`kind: local-server-map-case`、`network_flavor: playable`，带相同后端、地图、模式及当前模块哈希，只有 `passed: true` 且非空 `checks` 全为 `true` 才显示该地图与模式检查通过。没有匹配报告、新模块构建或仅有旧训练器报告时均显示尚未验证；该结果不代替完整可玩验收。
