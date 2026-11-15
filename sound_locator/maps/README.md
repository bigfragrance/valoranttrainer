# Ascent 地图约束与坐标

`ascent.png` 是本项目从当前国际服资源解码的原始 1024×1024 小地图，不是手绘地图。`ascent.json` 保存原地图变换和可达图。图像不能直接证明地面、墙壁、楼层或可行走性；本模块不会把透明像素当作墙。

原始、未裁剪、未旋转图片的坐标变换为：

```text
u = world_y_cm / 14336 + 0.8138950892857142
v = -world_x_cm / 14336 + 0.5732421875
world_x_cm = 8218 - 14336 v
world_y_cm = 14336 u - 11668
```

U 向右、V 向下。Unreal yaw 0 朝 +X，在这张图上指向上；yaw 90 朝 +Y，在图上指向右。变换来自原 AresMinimapManager 相机/捕获变换的静态字节证据；当前会话渲染标记与小地图位置仍应至少用三个分散的已知点检查。二位图不能推导高度 Z。

## 图的数据来源

最初的 `build_ascent.py` 从 `profiles/humanlike-hold-positions.json` 的两个曾通过原路径、地面、胶囊和实际到达验证的短路径（1501、1502）产生一个 44 节点局部图。其 `coverage.status` 为 `bounded_recorded_paths`，不覆盖全图，不能当成完整地图候选集。

全地图采样使用 `native/sound_locator_nav_export.h`，仅在显式测试的 Ascent stage9 会话中运行原引擎只读查询。默认在原图覆盖的世界 XY 范围按 200 cm 格网，对 Z=-100/100/300/500/700/1000/1400 cm 的多个高度调用 `K2_ProjectPointToNavigation`。只对相邻格子、合理高度差内的投影点调用原 `FindPathToLocationSynchronously`，要求路径有效、非部分路径且两个端点误差受限。路径原有的全部折点均保存，不把绕墙路线替换为两个端点间的直线。

```powershell
python -X utf8 sound_locator/maps/export_navigation.py <owned-stage9-result.active.json> --spacing 200 --out sound_locator/maps/ascent-nav-runtime.json
python -X utf8 sound_locator/maps/import_nav_export.py sound_locator/maps/ascent-nav-runtime.json --sample-cm 100
```

导出采用分批调用，每次最多约 8 ms 的查询循环，防止一次调用长时间占住游戏线程。某个单独原查询仍可能超过该时间。导出器不移动角色，不改导航或碰撞。

转换后 `coverage.status` 为 `sampled_original_navigation_queries`。每条图边仅沿一条有收据的原路径细分到至多 100 cm，来自不同查询的节点只按 0.0001 cm 坐标舍入一致性合并；不进行基于欧氏距离的“近邻捷径”。

## API

```python
from map_geometry import MapGeometry
m = MapGeometry.load()                         # 默认 maps/ascent.json
m.points_world                                # N x 3, 世界厘米
m.points_map                                  # N x 2, 完整图片 UV
m.adjacency                                   # 每个节点的邻接下标列表
m.world_to_map([[4720, -4450, 410]])
m.map_to_world([[0.5, 0.5]], z=400)
m.world_yaw_to_map_heading([0, 90])
m.nearest([[4720, -4450, 410]])                 # 下标、距离
m.reachable_neighbors(0, max_distance_cm=200)  # 原图路径距离以内节点
m.sample_neighbors([0, 1], rng)               # 只走一条已有图边，可停留
```

速度、时间、声源数量和多假设权重由跟踪器管理。最近点只是距离查询，不证明不在图上的输入位置一定可走。

## 必须保留的限制

- 这是全范围采样图，不是完整碰撞网格或全部 NavMesh 多边形。有限网格、高度探针、投影范围与局部绕行长度上限都会产生缺口；缺口不能自动被解释为墙。
- 原导航路径允许的行为与声音传播不同。图不提供声音遮挡、反射或声音绕门传播；不能把导航不连通直接当成听不见。
- 不同楼层保留各自 Z，不会在二维图中因重叠就连接。超出探针高度、狭窄出入口、特殊跳跃/下落、动态门和技能阻挡未保证完整覆盖。
- 两个曾验证的短路径用于本次原脚步实验的候选，仍须在当前会话用原地面/胶囊查询确认。历史验证不是对当前所有配置的永久保证。
- 原客户端包 SHA-256、原图哈希、数据文件哈希、运行时查询 PID 和来源路径均写在 manifest 的 `evidence` 中。

来源：`reports/training-maps.json`、`work/training-map-minimap-static/projection.json`、`profiles/humanlike-hold-positions.json`。原 Recast 数据块也确实存在，但本轮没有把尚未完整理解的自定义序列化格式当成已解析多边形；采样图来自实际原引擎查询。

## 2026-11-09 实际导出结果

在由训练脚本持有的 Ascent 测试进程 316304 中，36,288 次原投影调用得到 1,914 个格点内去重导航点；7,125 次邻接原路径查询中接受 6,816 条、拒绝 309 条。导出约 7.72 秒完成，游戏世界查询结束后立即把独占控制通道交还音频实验脚本。

按 100 cm 最大边长沿原路径细分后，图含 13,911 个节点、18,772 条无向边和 11 个组件；最大组件有 13,825 节点。节点 Z 范围 -100～910 cm。其余小组件和未覆盖处仍保持不确定，不添加未经原路径证明的连接。

原始查询收据为 `ascent-nav-runtime.json`，当前构建身份为 `ascent-nav-build.json`。`ascent-graph-preview.png` 把真实采样路径叠加于原图，已观察到走廊、房间和外边界相符；这仍不是当前游戏 HUD 图标的像素验收。4 项地图模块测试通过。
