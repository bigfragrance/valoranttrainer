# 斗牛 1 研发入口

本文记录早期 DefaultPawn 导航对照。当前原角色、枪械和英雄敌人测试入口是项目根目录的 `.\dev.ps1 play`，当前状态见 [README](../README.md)。下方测试计数及未完成项属于当时的历史记录。

当前研发固定在斗牛 1（`/Game/Maps/Duel/Duel_1/Skirmish_A`）。

```powershell
.\dev.ps1 duel1-test
```

此入口启动原客户端的导航对照，90 秒后自动结束。它会生成独立配置、校验客户端版本和保存登录状态，持续只读记录世界、控制器、Pawn、位置和视角。已有游戏或启动器运行时拒绝启动，结束时只清理本工具创建的游戏进程。账号登录仍禁止。

可用 Python 调整观察时间，导航模式上限 180 秒：

```powershell
python tools/run_map_probe.py --map 1 --mode navigation --run --seconds 120
```

## 已验证

- 实际世界包名为 `Skirmish_A`，不是仅在命令行请求该地图。
- 对象链为 `GameModeBase → GameStateBase`，`GameInstance → LocalPlayer → PlayerController → DefaultPawn → SphereComponent`。
- 实测位置从 `[-2050, 0, 300]` cm 发生变化，与初始位置的最大距离约 382.6 cm；控制器视角也变化，已保存实际画面。
- 本轮图形窗口出现手动输入；准确按键序列未知。较大的位置/视角变化是运行观察结果，不标为自动按键测试通过。
- 47 项离线测试通过；最终 90 秒运行结束后进程清理成功，安装 EXE 前后哈希一致。

运行记录索引：`reports/duel1-navigation-validation.json`。静态布局依据：`reports/duel1-world-layout.json`；默认 Pawn 创建和原角色限制：`reports/duel1-player-static.json`。原始采样保存在本次 `.local/native-baseline/<时间>/world-observations.json`。

## 这次修正

上一版 `scene-test 1` 使用 `Engine.GameMode`。其启动新玩家和允许重生的路径有比赛开始条件，实际运行只有 PlayerController，没有 Pawn。

新入口通过 `LocalMapOptions` 选择客户端自带 `Engine.GameModeBase`，该路径不受同一比赛条件阻挡，原生 SpawnActor 链实际创建了 DefaultPawn。导航配置选择基础 `PlayerInput` 和 `InputComponent`，保留其他配置；这是对照配置选择，没有证明原 EnhancedInput 配置本身有故障。

日志分析和内存观察分开判定。日志仍没有世界就绪消息；新入口在只读对象链中确认目标世界、控制器及 Pawn，且没有运行错误后返回 0。这个返回值只代表本项研发检查通过，`training_ready_proven` 始终为 false。

## 未完成与下一步

DefaultPawn 用于导航，不能作为原版步行、蹲跳、碰撞或武器逻辑的验收。原持枪角色、射击、机器人和训练循环均未完成。

已确认 `/Game/Characters/_Core/BasePawn.BasePawn_C` 的 Blueprint 父类为 `ShooterCharacter`，但后者是抽象类，不能直接作为具体角色生成。`ShooterGameMode` 构造会清空 DefaultPawnClass 并使用 AresRespawnManager；必须继续追踪原生重生管理器、角色初始化和接管顺序。

普通 INI 不能直接替换 GameMode 的 Pawn/Controller 类属性，因为这些属性没有 Config 标记。静态发现 SET/Summon 等名称也不代表有可用的运行入口，现有证据尚未证明能完整生成并接管原 BasePawn_C。
