# 开发状态 · 2026-11-01

完整验收范围见 [完成条件](REQUIREMENTS.md)。用户目标为继续直到完成，目前保持 active。

## 后续复核：配置覆盖

正常启动对照约 6–7 秒进入 Init/平台初始化。增加与旧探针相同的只读文件锁、job、暂停创建后恢复，仍可正常到达；使用不写代码段的硬件执行断点仍未命中。这些结果支持捕获方式影响启动，但尚未证明具体模块机制。

新只读观察器使用本工具创建的正常游戏进程句柄读取限定配置结构，不附加其他进程、不修改游戏内存。重新检查发现 `GEngineIni` 正常保持类别键 `Engine`，不能用它判断 INI 绝对路径是否被忽略。

Engine 固定记录的 destination filename 实测就是传入的本项目 `BaseEngine.ini`。静态证据显示，该版本只有 GameUserSettings、RiotUserSettings、RiotLocalMachine 允许 destination 文件合并；Engine 走 source hierarchy。这解释了参数解析成功但未采用目标文件值的现象。

实际 source-path 表包含 `../../../ShooterGame/Config/UserEngine.ini`。已临时创建一个独立标记 section，运行后确认没有读入 Engine 配置（同时验证观察器可以找到既有 `/Script/Engine.Engine` section）；测试后原文件状态已恢复。因此不能把路径出现在表中当作 loose 文件加载成功。

后续继续解析客户端原有 `IniBootstrap=` 归档入口的实际格式，以完整保留原配置并设置必要的启动项。尚未把该入口称为已可用。

## 目标与当前结论

继续研究原游戏客户端 `VALORANT-Win64-Shipping.exe` 的无账号启动、原生关卡加载、角色和武器初始化，最终目标为斗牛 1–5 随机拉出目标的训练环境。禁止账号登录。早期 Panda3D 原型保留，开发停止。

尚未实现原客户端练枪环境。当前重点是引擎类配置如何进入实际创建路径；地图、可操作角色、武器及机器人仍需独立运行验收。

## 已完成的项目整理

- 全部 3,104 个源文件迁入，初始逐文件 SHA-256 对照通过；再次检查源目录，3,104 个原文件均未改变。
- 固定分析副本继续匹配 SHA-256 `147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f`，239,005,680 字节。
- 修正核心研究脚本、C++ 构建与输出路径，新增 `dev.ps1`、项目配置和本地依赖记录。
- 新增结构化运行日志分析器，输出带行号的世界、引擎/GI 和平台失败证据；不将启动请求、退出码或认证管理器完成状态当成游戏完成。
- 新增带版本/登录状态检查的原生配置捕获包装器。18 项离线测试通过。

## 从旧目录接续的最新运行证据

资源修复后的历史校验覆盖 17 个容器、635,840 个物理块，`total_mismatches=0`、`complete=true`。该记录不含 TOC 数字签名认证，本次未重扫整个安装。

`work/client-analysis/base-engine-textconfig-runtime.log` 的关键位置：

| 行 | 实际观察 |
| --- | --- |
| 138、140 | 世界 URL 为 `/Game/Maps/Init/Init`，并非请求中的 `Skirmish_A` |
| 172、173 | 游戏内部 `RiotGamesApiClient` 报 `LaunchRiotClient`，整体进入 `InitFailure` |
| 181 | 对象路径包含 `AresGameEngine`、`BPAresGameInstance_C`、`BPAresClientGameInstance_C` |

对应启动记录包含 `-EngineINI` 和 `-textconfig`，进程退出码为 0；以上证据仍不支持基础引擎配置已达到预期、目标地图已进入或账号已登录。重新生成的分析见 `reports/native-startup.json`。

## 本轮原生验证

配置观察点位于 `0x4dbc088` 的配置读取返回后、`0x4dbc0b9` 的引擎类装载之前。配置返回 FString 位于该点栈的 `RSP+0x60`，`GEngineIni` 为 RVA `0xd76ca38`。本机固定副本重新反汇编确认调用目标为 `0x19df8c0`，section/key 为 `/Script/Engine.Engine`、`GameEngine`。

调试方式初测在 `stub.dll+0x2c4140` 收到第一机会断点，原捕获器直接中止。按照 [Windows ContinueDebugEvent 文档](https://learn.microsoft.com/windows/win32/api/debugapi/nf-debugapi-continuedebugevent) 修正路由后，三个第一机会断点交给程序自身的异常处理器，90 秒内仍未到达目标配置点；已按时结束本次进程。该结果不证明具体保护机制，也未得到实际配置值。

接续实现短时配置快照方法：复用既有暂停启动/固定版本校验框架，在同一配置点只采样返回值并停住主启动线程，由控制器记录后结束自己创建的进程。运行结果在 `.local/native-probe/`，以 `captured`、`stopped_before_engine_load` 和清理结果共同判定，不能以构建成功代替捕获成功。

快照方法首次实测 15.257 秒后返回 `not_reached`、`deadline_without_capture_marker`；未得到引擎类或配置文件名。自建游戏进程已退出，私有分配随进程回收，安装 EXE SHA-256 前后相同，未留下游戏或启动器进程。该次测试期间源码有并行的小修，记录只用于其已记录 EXE 哈希对应的结果；最终源码随后重新构建通过，未声称已完成最终版运行验收。包装器现已加入测试前后源文件/头文件/EXE 哈希比对，避免此类版本归属不清。

REA 单指令接口在本轮 300 秒超时，原生分析会话已关闭；本轮静态复核使用本地固定副本、PE/Capstone 及已有 Ghidra 导出，未把失败的 REA 调用描述为有效分析证据。

## 接续检查点

1. 获取实际 `GEngineIni` 文件名和返回的 `GameEngine` 类；区分文件名覆盖未选择、配置集被重载和后续类替换。
2. 跟踪 `0x19e4060` 文件名解析及 `0x19e6120 → 0x19f5580 → 0x19f57e0` 配置装载链。
3. 核对 `0x43a4780` 的 GameInstance 类选择及 `0x439a8e0` 基础类路径。历史 `stage2-candidate.json` 仍只是静态候选，未视为已完成运行实现。
4. 在实际进入原场景后，分别验证原生角色、武器、伤害与机器人生成；三个已有 SpawnBot 空入口不能作为可用生成器。

Riot 启动器不是本轮主分析对象；`LaunchRiotClient` 是游戏内部启动依赖的失败证据。`minimum-platform-init` 的已知字段解析同样不构成已验证的无账号进图功能。
