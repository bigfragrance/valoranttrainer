# 本地宿主与客户端迁移

新增的 ShooterGame 候选使用独立 DLL、构建快照和双端目录。原训练器及已通过的 DefaultPawn 网络样机继续保留；默认训练器入口尚未切换。

2026-11-04 的训练候选已接入真实 `ShooterGameMode`、唯一的原 GameState 工厂与原固定步/移动时钟生命周期。Ascent 八人局中已记录到本地玩家 6 次真实击杀和 10 次成功重生。NPC 现在保留各自物理时钟，经原映射器发送同一公共时间域的快照，客户端继续使用原解码与插值；修复了瞄着敌人却打空的明显位置错位。逐项证据见 [命中同步记录](../work/local-server-playable/hit-sync-repair/mode9-gameplay-evidence.json)。该记录与完整对局结束、进程收尾的验收分开，全部英雄技能仍不由这项结果认证。

后续原玩法九分钟回归已通过：119 次击杀、118 次重生、130 个旧装备 Actor 销毁后原弱身份失效，客户端装备通道保持在 7–10 个，无主 Owner 为 0。清理先按旧 Pawn/epoch 捕获装备，再退休输入、特效和技能观察器，最后调用原 Actor 销毁路径并重生；新生命和真实转移装备独立保护。原 `GameMode.EndMatch` 调用一次并观察到实际 inactive，双端 exit0、原驱动销毁、无 Critical Error。客户端结算 UI 和全图网格逐对象消失仍不在该验证范围内。见 [完整运行摘要](../work/local-server-playable/hit-sync-repair/death-weapon-retirement/mode13-verified-summary.json)。

| 路径 | 用途 |
| --- | --- |
| `.local/native-module/trainer_bridge.dll` | 原训练器对照构建 |
| `.local/local-network/module/network_probe.dll` | DefaultPawn 原网络协议、Actor 复制与 AI 移动验证 |
| `.local/local-server/module/local_server_bridge.dll` | ShooterGame 宿主及客户端候选 |
| `native/local_server_gameplay.h` | 将原服务、GameState 和英雄生命周期接入网络世界 |
| `tools/local_server_module.py` | 候选构建、快照校验、进程归属和模块加载 |
| `tools/run_local_server.py` | 有界双端运行、阶段判断及清理 |

这里的 `playable` 是选择候选模块的名称。该名称和报告中的 `passed` 均不表示完整训练功能已通过。报告始终保留 `playable_verified: false`，并分别列出实际观察到的阶段。

## 运行

在项目根目录执行：

```powershell
python -X utf8 tools/local_server_module.py
python -X utf8 tools/run_local_server.py --transport ip --observe-seconds 35
```

第一条只构建 `.local/local-server/module` 中的新 DLL。第二条依次启动本项目拥有的宿主和客户端，绑定 `127.0.0.1:17777`，完成有限时长观察后退出。可用 `--port` 更改端口，`--observe-seconds` 支持 5–75 秒；`--bootstrap` 可指定项目内另一份经过校验的斗牛一配置归档。

最新会话由 `.local/local-server/latest-session.json` 指向。会话目录包含 `verification.json`、双方 runner 报告、运行日志，以及 `host/bridge-state.json` 和 `client/bridge-state.json`。失败阶段保留在 `error`、`playable_error`、`playable_stage` 和 `failure_stage` 中。

默认 `run_native_baseline.py --network-probe` 仍选择 `proof`。只有显式传入 `--network-flavor playable` 才会选择新模块。构建校验、原 Popen 身份发布、对端预检和加载使用同一个选择。原有样机命令见 [LOCAL_SERVER_PROOF.md](LOCAL_SERVER_PROOF.md)。

## 当前迁移边界

候选源码接入以下顺序；源码存在本身不算运行通过，具体结果以每次 `verification.json` 为准。

1. 在原离线 GameInstance 生命周期内初始化本地服务、设置、内容、渲染和特效依赖。
2. 宿主创建原 ShooterGame GameState，并设置网络登录所用的控制器和 PlayerState 类型。
3. 采用原网络登录实际生成的控制器及 PlayerState，验证它们与 NetConnection 的双向归属。
4. 为该连接生成权威捷风，在原 BeginPlay 前关联 PlayerState，调用原 Possess，并建立原装备槽。
5. 客户端通过原 ClientTravel 连接；从原连接和复制通道观测世界切换及自己的英雄。

原安装目录继续只读使用。运行配置、构建、缓存、日志与临时文件写入本项目 F 盘目录。这里不包含账号登录、账号状态修改或安全校验绕过。

## 阶段验收

`native_gates` 分别记录控制连接、自己的 PlayerState/Pawn 复制、宿主 Shooter 世界、权威英雄生成与占有、客户端英雄到达。`shooter_hero_round_trip_proven` 要求同一组双端观测同时满足以下条件：

- 宿主本地服务及 Shooter 世界就绪。
- 登录连接达到原 State=3、LoginState=3，并且控制器/PlayerState/Pawn 恰好对应候选报告中的权威英雄链，角色均为 Authority。
- 客户端已离开自己的临时启动世界，没有待切图 NetGame 或本地权威 GameMode；其原 LocalPlayer/控制器/ServerConnection 归属正确，收到自己的 PlayerState 和 Role2 英雄。
- 双方没有原模块或玩法阶段错误。

完整 `passed` 还要求完成请求的观察时长，双方按请求退出，原驱动清理成功，运行日志存在且无 Critical Error，旧训练器/网络样机的 DLL 和清单保持一致，新候选 DLL/清单也保持一致。中途退出、缺失日志、清理超时或强制终止均不能通过。

这一步不认证原英雄输入发送、服务器处理、ACK、校正/回放、射击伤害、死亡重生、技能和完整原 HUD。DefaultPawn 的 AI 轨迹复制证据也不能代替这些门槛。

## 构建身份与测试

`local_server_module.py` 对固定原镜像生成 28 个底层原函数 profile，并保留原 `original_*.h` 中各自的运行时版本检查。构建清单捕获入口 CPP 的递归引号 include 闭包及构建脚本；校验读取捕获快照，而不把之后工作区中的源码变化归入旧构建。快照漏项、额外宣称未参与编译的源码、路径越界、profile 不一致和 DLL 变化均会拒绝加载。

每次发布的进程归属同时包含 flavor、会话 ID、原 Popen 创建身份、镜像路径及哈希、DLL 哈希。`proof` 与 `playable` 即使出现相同 DLL 哈希，也不能互相作为对端豁免；原有未带 flavor 的 proof 记录只兼容 proof 路径。

Python 测试命令：

```powershell
python -X utf8 -m unittest discover -s tests -v
```

本次新增 34 项测试，完整 225 项通过。测试使用临时文件、合成状态和 mock 编译/进程句柄；不启动游戏，也不作为原生运行验收证据。结构化迁移清单见 [local-server-migration-inventory.json](../reports/local-server-migration-inventory.json)。
