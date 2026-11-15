# 本地原生网络样机

第一步已跑通底层链路：独立宿主在斗牛一加载原世界，第二个原客户端通过原 IP 网络驱动入场，收到自己的 PlayerState/Pawn，以及一个由宿主原 AIController 控制的远程 Pawn。远程人物通过原 ActorChannel 和属性编码持续同步移动。

**这是框架验证样机。测试角色使用原生 Engine.DefaultPawn；捷风、枪械、技能、原 HUD 和 ShooterCharacter 的输入确认/校正尚未迁入。现有可玩训练器保留作对照。**

## 重复运行

在 `F:\valoranttrainer` 执行：

```powershell
python -X utf8 tools/local_network_probe.py
python -X utf8 tools/run_local_native_network.py --transport ip --observe-seconds 45
```

第一条编译独立的约 208 KiB 验证 DLL，不覆盖可玩模块。第二条依次启动两个本项目拥有的进程，监听 `127.0.0.1:17777`，自动验证并退出。加载计入的总耗时约一分钟。可用 `--port` 指定另一个本机端口；默认只运行有限时间，不启动常驻服务器。

资源只读复用已有安装，所有构建、运行配置、临时文件和日志写入 F 盘。服务端和客户端各有独立配置与诊断目录。运行进程的匹配同时核对 proof id、原 Popen 身份、创建时间、镜像路径和模块哈希；独立国服进程保留。

最新运行目录由 `.local/local-network/latest-proof.json` 指向。每次保存 `verification.json`、双方 runner 报告及 `host/bridge-state.json`、`client/bridge-state.json`。汇总见 [第一步验证报告](../reports/local-native-server-first-step.json)。

## 当前实际通过的门槛

- 原驱动工厂、World/Notify、关卡集合和原 Tick 生命周期相互一致；监听前后均验证回环地址。
- 宿主真实连接达到原 LoginState=3，控制器与其 NetConnection 双向对应；客户端完成网络世界切换，并形成自己的 LocalPlayer/控制器/服务器连接关系。
- 客户端收到原 PlayerState 和自主代理 Pawn（Role2）。
- 独立远程 Pawn 由宿主 AIController 驱动，客户端通过原 OpenChannels 中的 ActorChannel 观察到模拟代理（Role1）。
- 持续移动通过原变化比较、属性编码及通道发送到客户端。45 秒重复验证中，宿主/客户端轨迹跨度约 1069/1067 厘米，249 组有时序约束的位置对照通过。
- 双端未出现 Critical Error；原驱动注销和两个自有测试进程退出通过；可玩 DLL 与构建清单哈希保持不变。
- 191 项 Python 测试通过。验收脚本要求完整观测时长、双方按请求退出、日志无 Critical Error、原驱动清理和构建哈希一致；缺失日志或中途退出均不能通过。既有 45 秒记录也通过了这些条件的离线复核，见 [完成条件复核](../reports/local-native-server-first-step-completion.json)。

位置对照只针对此 DefaultPawn 网络样机：将客户端观测点与附近时间窗口内的宿主点比较。它不是原英雄移动手感、精度、延迟或竞技同步的认证。

## 补齐了哪些服务器缺口

1. **托管装配**：调用原驱动工厂、SetWorld、FindOrAddLevelCollection 和 InitListen，补齐已证实缺失的世界/两类集合驱动链接。清理使用原 DestroyNamedNetDriver。
2. **Actor 复制遍历**：普通服务器 TickFlush 保留连接收发，却缺少 Actor 复制遍历。样机只对连接自己的控制器、状态、Pawn、共享 GameState 及测试远程 Pawn 调度，保留原关卡就绪检查、弱身份、通道创建、PreReplication 和 ReplicateActor。
3. **复制帧推进**：按原录制帧的批次边界推进当前 `NetDriver+0x4D8` 帧号，恢复原 changelist 的变化比较。对照实验中，缺少这一步时客户端停在初始位置；补齐后开始持续收到移动增量。没有额外手动 dirty，也没有写客户端角色或位置。

客户端只通过原 ClientTravel 发起连接。跨切图后的实体观察直接读取其原 ServerConnection/OpenChannels；不需要再次改写新玩家 Pawn 的 Tick 表来扫描人物。

## 参考仓库与版本边界

参考用户提供的 [andr1ww/Ares.Gameserver](https://github.com/andr1ww/Ares.Gameserver)，固定提交 `e0a2d4cfa74afee102efa2c7be16a0609453bcf7`，Apache-2.0。该仓库针对 1.08；本样机独立重新核对当前程序的接口、结构布局和原码，未采用旧地址。相关来源与许可见 [说明](../work/local-server-proof/THIRD_PARTY_NOTICES.md)。

本阶段选择原生 IpNetDriver/IpConnection，沿用原协议校验。直接使用 AresNetConnection 的早期实验在连接初始化出现本地依赖空指针；该路径尚未就绪，原失败报告保留。

## 下一阶段

将 ShooterGame 的 GameState、控制器、PlayerState、真实英雄与装备生命周期接入服务器世界；补齐原空权威入口。随后分别验证原组件位流、本地英雄输入发送、服务器处理、ACK、校正和回放，再接入射击/伤害/死亡/重生及原表现。基础 Actor 移动样机通过不能替代这些验证。

静态与运行证据分别位于 `work/local-server-proof` 和 `.local/local-network`。前一轮研究见 [LOCAL_SERVER_RESEARCH.md](LOCAL_SERVER_RESEARCH.md)。
