# 当前接续点 · 2026-11-07 无目标巡逻与路线预瞄

最新模块为 `b45dc40bbf1cb914e04ac672c5eb0579f489e6d879c10003048d8a9d4d1aa646`，221份源码与构建快照一致。已拆开角度停留和搜索停留，按当前原生路线保持游走，不再把全图锚点当作可见角度；经过当前有视线证明的路线拐角预瞄。丢目标后的短暂停留结束后恢复移动搜索和路线视线。沿声音估计接近时使用已证明路线，不持续直盯声音后方的墙。原配置/2392参数契约、原输入与伤害不改。

1821项生产Brain/路线几何检查及374项射击权限回归通过。独立12人Ascent明确使用11个赋能快照：384采样、337原生观战active、68权威击杀，多次重生后继续跟随、返回自己成功；双runner0/Critical0、cleanup[]。同生命最多采样移动163.753米，1429个无目标有路径样本里40个无导航请求。视角对当前路线水平误差中位数约0.536度，转向/声音/受击有长尾，不能称全部行为已真人校准。

日常设置保持用户原rev135/SHA `ea7fee3f…`，名单预设仍是default标签，未用QA赋能设置覆盖。用户原对局触发的3235输入队列保护独立保留日志，本次未宣称修复；首次QA启动适配器超时也保留为失败。详细机制、数据及限制见 [巡逻修复记录](../work/humanlike-bots/no-target-navigation-20261107/README.md)。当前复制训练器已停止，新版在下一次开始训练时加载。英雄技能仍暂停，不打包，产物均在F盘。

## 此前固定名单与原生观战扩展

**英雄技能开发暂停，优先落实用户的人机说明。** 新增的固定可增删名单已经接入，每条独立保存英雄、预设和战斗参数；开局及重生不再抽英雄。内置25个细分段位（黑铁I至赋能）及命名自定义预设。名单结构/英雄变化由下一次手动开局采用，同名单战斗参数保持实时发布与消费者确认。

当前本地服务器模块 `7fc56a6b06d65d29da992583dacf65cf3bc9190df83d5fc20292096c1ee7c974`，220份源码与捕获快照一致。原生观战已完成指定人机、默认鼠标左/右切换、死亡后跟随同条目重生及返回自己；实际客户端目标决定WebUI显示。观察者不参与人机目标选择及双向伤害，退出后恢复。

实际12人固定名单回归：239组采样、219组原生观战active、17组等待重生，权威计分31次击杀，观察目标11经历3次死亡后保持条目；返回自己成功。该回归模块c75与最终模块的唯一native差异是显式测试inl。最终模块的标准输入fixture8又通过原Key按/松、request/ACK 0→1→2、镜头3→2→3、两腿服务器调用/选择/玩家输入各+1；双runner0、Critical0、cleanup[]。单独关闭无敌的45秒观察者检查168采样通过。最终54项Python回归、47项界面DOM、63项Zoom及19项目标次序模型通过；模型不当作渲染或真人段位验收。

日常8792仍为原rev134/12人Ascent偏好，原始文件SHA未变，测试使用独立副本。用法见 [人机一版说明](HUMANLIKE_BOTS_V1.md)，完整证据见 [本轮验证](../work/humanlike-bots/root-integration/FIXED_ROSTER_VERIFICATION.md)。入口为 `启动本地服务器控制台.cmd` / [本机人机设置](http://127.0.0.1:8792/#human-bots-settings)。当前训练器已停止；Computer Use仍暂停，没有重新验收渲染像素。不打包、不复制游戏资源，产物仅放F盘。

以下为此前Bot-only基线证据，按其原模块与范围理解。

Bot-only基线模块 `9b66347e…`，2392项配置及9组可编辑段位风格。已合入独立感知/记忆、分阶段瞄准、武器×距离×情境节奏、身法/八种拉出、搜点和FFA选择，以及实际视角采样时钟、严格Info两腿、合法估计预开、原惯性停车与重排原路径。12项来源不足的控制仍只读，不伪造命中或隐藏敌人视觉。

真实专项证据：Info007完整一条“首探→实际发现→退回停稳→同目标再拉→交战→锁退”；1303/1304各有原路径纠偏后连续41条/2.5秒真实零速停留及原释放。生产数据新增Ascent两处架点1501/1502，1502默认风险门仍拒绝；未称全部地图架点已覆盖。stationary-prefire1304-003也通过136条实际物理及停留/释放，射击因果另审。

最终普通12人Ascent008自然达到9分钟规则终点、102击杀；其中长时间测试暂停，计时仍走，不能称9分钟连续交战。1282组双端样本通过、双runner0/Critical0、cleanup[]。原游戏进程1为请求关闭。57条严格原物理快照证明实际推进慢走。927原发射含75人类及852NPC，Brain849，NPC5多3发尚未解释，不补造计数。该局23979fc与当前9b663普通产品代码相同；后者仅多test14临时clone别名修复。

配置8项实采热更新通过；28项人机Python、16项runner和18项controller测试通过；211份当前native与构建快照逐一相符。日常8792仍为用户原rev134/12人Ascent偏好，QA8793不覆盖它。此前报告覆盖问题已按原冻结哈希恢复字节一致，新报告强制独立路径。

详细模块、证据和未覆盖范围见 [Root状态](../work/humanlike-bots/ROOT_STATUS.md)。尚无全部八种拉出、全部武器/地图/情境的逐项原生正例；原逐intent执行ACK未知，段位未真人校准，性能长尾仍须保留。被平台停止的World detour未安装，不重试。继续仅在F盘开发。

以下旧检查点按其当时的模块和范围理解，不能覆盖上面的最新状态。

**11月6日最新增量（优先于下面旧记录）：** 当前发布模块`a9044810…`。真实`pair-jett-q-002`已证明一次原生Q起飞、回落，双方原位置都有升空/落地；host本步物理与同一原client-wire claim收敛，但异步bridge截图不能当同时姿态误差。Q资源样本仍1→1，未称完整资源闭环。`pair-jett-e-001`原预备/二次触发、按下/松开、六次状态迁移通过，双Critical0；实际冲刺运动正在分析。C/X尚未验收。

`pair-phoenix-c-primary-007`实际92.296秒正常、双Critical0。原C选择及Primary500ms输入完成，原Fire单次转发/返回、来源稳定。同一次Fire前后CanThrow表达式全部已知且允许，真实Owner Ares/selected Projectile类型匹配，计时器不阻挡；真实World+40非空触发原世界函数的mode3提前返回，未进入原OnThrow广播。正在准备仅本项目owned-host世界的原函数兼容入口，客户端与其他世界保留原路径，未修改原安装或任何完整性检测。

`pair-empty-bootstrap-003`实际双端17槽空/current null，host原Apply/physics19/clock19==consumed19、client源1023/own预测/roundtrip；Root独立paired checker通过。新004首funded观察回归32.641秒正常、双Critical0，但观察器误用了Mode的0x990基偏移，真实human clock_base_offset是0x9F0，四点算术拒绝；候选正按原构造/ClockState证据修正，未修改clock/ACK/H/q或释放正式装备发布。最终高差路径恢复已合入，12人003实际回归进行中；Phantom购买3235、跨生命结束时钟/成绩UI、八种身法完整结果及全部技能仍未完成。

**最新实际结果优先：** 已发布 DLL`9959f06d…`，`pair-peek-acceptance-12-001`实际跑420秒，双端退出0/Critical0、runner passed=true。这是有界动作观察，并非9分钟/结束合同验收。八种身法分别给NPC1–8单独开启；已见大拉、跳拉、蹲拉、肩拉启动，5次蹲拉回撤记录终点距锁定撤回点12.695–17.063cm。其余中断动作不当完成；Poppin在同时有掩体/地面/路径的严格窗口没有已获得目标，InfoRepeek该窗口没有可用掩体，尚不能从累计计数推断门错误。源代码另找到三维导航到达与水平移动停止的边界不一致；准备保留垂直保护的修复及同帧下一节点读回。

Phoenix显式C输入测试`pair-phoenix-c-primary-001`实际91.812秒正常退出、无Critical；CU选C、原Primary500ms两次调用及host/client held/released读回成功，双方状态到Bending/Unequip，但未观察到火墙，原事件观察器3个hook实际entries均0。新合入原Context Reset工厂返回数组观察、三个原生状态私表计数、25ms元数据轮询和blind严格拒绝元数据读回；当前176依赖严格编译通过，**这些新诊断尚未发布/实测，也不是技能已完成**。下一轮查Actor ProcessEvent真实世界门；不伪造墙、不重复Tick。

旧DLL`da75952c…`的30人9分钟规则局正常652.438秒，403实际击杀/玩家6次重生；原native-end complete/inactive但当前生命clock/end=5.921875，跨生命全局时钟与完整成绩UI仍未解决。原脚步885生产回执、1018线索采样，不等于1018次不同声音。当前12人QPC严格活跃285.657秒/67510 Host调用，均1.871916ms，Equipment/Combat/Feature归一合计约0.159ms，剩约1.115ms未归因；含抢占/嵌套限制，不外推旧30人性能。

新输入checkpoint语法已合并，73 helper/67源码与真实数据检查、161依赖严格编译通过，保留原15门/queue100/prune/Apply/clock/ACK。该局保留历史仅见Aim ordinal4的旧形状，**新中间状态分支仍未实证**，不能从6次重生就宣称3235完全覆盖。新增6阶段QPC：active窗口HostTick10.147ms、NetAdapter11.494ms、NPC2.032ms、Sensor0.631ms、单Brain0.00126ms、Registry+Replication3.439ms、ControlPublish0.263ms，含抢占且嵌套不可相加，尚无卡顿完整归因。

日常控制台8792使用独立持久文件`humanlike-bots.json`，当前rev134/12人Ascent/Jett/原无限弹true，无individual测试覆盖；原保存配置SHA不变。新脚步感知首次默认明确开1，落地仍0。场景5和全桥接peek计划诊断均已发布并有真实读回；仅显式测试文件启用输入fixture，日常模式不自动操作玩家。最新启动适配器已用正常入口重建，ResumeThread失败不再清掉持有记录；C001真实启动各阶段正常，不能据此推定旧启动超时原因。

最新增量（优先于下文早期检查点）：`pair-20261105T190152.944068Z` 完成普通9分钟乱斗，双端runner0/Critical0；原组件Tick发布至少92次已验证脚步请求，实际原几何掩体281条ready。换弹有原弹量/储备变化，但正换弹状态与请求的完整关联仍待新摘要验证。跳拉未请求的直接配置事实是JumpEnabled=0；另外独立复现并修正了零权重短路线占据提议、TTL重复的饥饿问题，不能倒推它是旧实测未跳的原因。护甲Poll平均约0.065ms，无理由删减原审计。普通历史采用无损gzip，240条完整证据和原gate投影已用实际reader核对。

30人重试`pair-20261105T193858.560350Z`实际进入，英雄不重复且采用rev54；记录118跳跃请求采样、140原生上升/跳跃采样，实际向上速度381cm/s、离地，46741正换弹状态采样、140击杀、351真实脚步生产事件。完整起跳/落地/回撤仍逐生命核对；不是118或140次完整动作。约218s玩家重生后原输入队列115超100，Fail3235终止；双端Critical0但整局passed=false，正修原事件/纠正checkpoint合同，不放宽上限。所有resolved profile脚步/落地默认禁用，所以footstep cue=0不代表生产器失败。

当前DLL`1aa36824…`只在b30上增加诊断非有限数转JSON null，不改输入/计数/ACK，139项wrapper和449项严格JSON检查通过。两轮Phoenix practice因原clock/consumed index15分歧Fail3252，在技能按键前结束；另发现client只读技能观察器复用host-only profile flag，候选正纠正其独立原字节校验。新`启动本地服务器控制台.cmd`/`dev.ps1 play-local`已合入，独立持久文件只首次迁移原偏好为12人Ascent/拟人启用/原无限弹true，--check通过，尚未真正seed/切换当前服务。8792仍为rev56测试fixture(Phoenix/Duel1/practice2)，不得当交付配置。全部技能、跨玩家重生时钟/输入稳定、逐动作完整验证仍未完成。

## 当前：拟人人机已合入并完成一轮真实乱斗，继续补几何、换弹和听觉

用户允许睡觉后持续自主开发及本项目实测；保持所有新文件在 F 盘，不打包、不改官方安装与账号。不得在本项目 client 仍加载 DLL 时发布模块。

当前最近一次实际运行模块是 `b7080da0adecdfdcc441e455a671f7e80485515a75219044355e3673221aeea4`，普通 WebUI 拥有的 `.local/local-server/pair-20261105T174659.755981Z`。真实12人 Ascent 运行到9分钟规则结束，384次人机原发射回执、37次击杀、3316条真实蹲姿采样；原生结束记录接受并完成、原生 active=false、胜者2。双端 runner=0、Critical=0、最终 `passed=true`，正常请求退出。原结果 UI 像素、玩家手动射击和逐 Intent 消费相关性仍不能据此宣称已验收。

新系统沿唯一 `local_server_match::TickBots → local_server_npc_input::SubmitBot` 路径，原128Hz物理/武器/命中不改。2353数值配置通过独立版本 sidefile、命令17原子提交；所有消费者采用后才 ACK，禁用在全false实际提交或旧生命实际退休后确认。实测 rev44–50 已证明：修改#1只改变#1的 profile fingerprint、实际新值读回、显式 seed 两向切换、所有活体 NPC 暂停为中性决策、禁用/重新启用。证据 `work/humanlike-bots/root-integration/live-smoke-2.json`。

WebUI 已接当前真实API，22项真实页面只读/草稿检查通过，实际11个NPC身份、版本、参数与底部动作显示匹配。存储版本领先原生时显示等待，配置采用不当作行为验证。Computer Use 在本项目窗口观察到50甲/100血、Jett手、原枪械、12英雄榜；B购买幻影即时清掉狂徒已购买状态，Esc可关闭。短注入点击不能证明玩家实际开火，详见该目录 `computer-use-observations.md`。

已发现并修正启动快照仍用v2 reader、Python命令表漏17、未知负换弹状态阻止原生已许可开火等集成缺口。Python62项既有回归及20项独立 guard/真实Channel检查通过；当前 native 源151文件严格语法检查通过，后续 source 已继续改动，下一 DLL 尚需重新构建/实测。最新已合待测：真实原Reload请求与正状态/弹量/许可恢复闭环；普通交互的无损压缩历史（同240条，不减采样/物理tick；明确fixture不变），以及手机配置状态说明。动态原几何掩体、原组件Tick脚步接入候选仍在并行完善。

限制保持明确：尚无可感知 whole-origin Miss 证明，相关四权重锁定；近战原攻击读回、其他战斗声与消音属性三项仍待来源。脚步旧E0观察在首局0次，不能称为完整脚步能力。静态 authored 掩体0行，正补自身几何提议与验证。前一护甲 fixture 的跨人类重生结束时钟证明仍未收口；本次人类未死亡，不能覆盖那个分支。全部英雄技能的网络迁移也仍未完成，当前正审计共性输入/状态证据入口。

## 以下为此前记录，优先以上述当前接续点为准

## 本轮进行中的护甲回归与拟人人机候选

用户再次明确睡觉后自主继续，不再等待确认。当前已合入原重甲 `native/local_server_armor.h`（v2 `4d4697d3…`）及 runtime 最小接线，仅当前独立服务器 FFA。宿主原 HeavyArmorItem、真实槽11、AttachedDamageSection 与客户端 ActorChannel 都保留；原生初生50、真实消耗0/50和一次宿主重生重新50已观察。第一批两个短run分别被显式输入fixture4920和名单ACK3712中止，均不能报整局通过。正面证据及限制分别在 `armor-default/first-grant-evidence.json`、`first-respawn-evidence.json`。

当前候选 DLL `4922fbb4c3e508c80f73f8312aef5813e3d3c7b2b7563ce3b8bdf6810e67b751` 正运行 `.local/local-server/pair-nov5-armor-2`，是显式原输入 FFA fixture，非留给用户的普通局。已合严格只读护甲观察、仅当前原500ms hold预算等待真实按下消费的fixture7行修订，以及名单原子文件32/33（或Move5且DELETE-access探测具体32/33）有界重试；未确认发布前ready=false，其他错误仍fatal。48项真实Win32文件检查及完整127输入严格语法检查通过；旧3712没有保存具体Win32错误，不能倒推已确认原因为共享竞争。实测继续，不能在项目client仍加载模块时build/publish。

截至该局约160秒：宿主和client模块无错，已多次发甲/退休，client严格tuple/Weak稳定并读到max50；Computer Use实际HUD见护甲23、HP87。client原Heal RPC观察已证明一次NPC同生命盾27.332→50与HP88.668→100；还需检查耗尽0→50、玩家自身回血与更多死亡重生。该局尚未结束，不写整局通过。

拟人人机阶段已并行完成现源码/资料调查，当前还未应用新产品实现。工作在 `work/humanlike-bots/`：`investigation/sensors.md/json` 是真实调用链/可靠性审计；`references/REFERENCE_REVIEW.md/json` 区分全部8链接的实际可读层级；brain/types、配置schema/Python、原生sensor、原武器反馈及control传输候选由独立代理写work，根负责审查合并、WebUI和真实运行。唯一NPC调度是 `local_server_match::TickBots → local_server_npc_input::SubmitBot → paired native physics`；不能并行启用旧 `training_bot_controller::Tick`。brain只接个体已获信息，不接hidden实时roster位置；全局epoch仅退休旧life。听觉若无原事件须明确unsupported，不能用全图坐标冒充。当前旧bot代码尚没有FOV/个体记忆/听觉/细分技巧，不能把候选合同/旧8参数当新要求完成。

## 前一完整回归（护甲接入前）

先完成本轮射击、切枪、计分、击杀补弹、死亡回血包和出生护甲，再按用户新提供的 [拟人人机完整要求](HUMANLIKE_BOTS_REQUEST_2026-11-05.md) 开发。后续要求已原文保存在 F 盘，尚未开始新的人机实现；不能把旧六个难度参数或历史报告当成该要求已完成。

本轮最新正面回归是 `.local/local-server/pair-nov5-five-fixes-4`，DLL `284e09044deab39fd368bf332733bcf1c30b352e23544793a7aec5846489a25a`：205.641 秒正常退出，双方零 Critical。原输入测试中的玩家 186 发射线、59 次伤害、40 杀、5 次死亡重生，活体射击没有 `source_inactive`。121 个真实击杀提交带来120次补弹，最后一个延迟意图随比赛结束退休；客户端有21次同生命且未换弹时的实际弹药回升。120个死亡掉落、21次真实碰撞拾取、3次原生回血；客户端确实执行3次原 Heal RPC，其中两次为本地角色60→100、26→100。完整摘要见 `work/local-server-playable/nov5-three-fixes/five-fixes-case4-summary.json`。

顶部原玩家条已使用全局累计击杀，原连杀计时组件独立维护横幅与右上角反馈；这局120条实际击杀回调全部映射，40次本地横幅映射，原排序恢复失败0。缺早期击杀事件时的原 Pending 行视觉补映射已编译并有独立生产 helper 检查，但这局没有真实 Pending 缺口，不能宣称该运行分支已实测。普通切枪的原生动作和额外纠正取消已在先前180秒普通局实际观察，未做逐帧视频等价认证。

**当前优先缺口是用户刚指出的护甲：** 候选确实未发放原重甲，所有原生 shield/maxShield 为0；已完成的回血只恢复现有生命，不会凭空造甲。正在独立接入原 `HeavyArmorItem`、真实非 Equippable 装备槽、AttachedDamageSection 和复制生命周期，下一轮须验证每次出生的50甲及耗尽后原回血包恢复。研究/候选在 `work/local-server-playable/nov5-three-fixes/`；所有子代理只编辑独立 work，不直接改根 native 或运行目标。

独立公共 GameInfo 时间来源候选在真实客户端产生至少130.094秒结束成熟延迟，已拒绝并最小撤回；原 start/end 路径已恢复。不能把那局 fixture 最终 passed 当成及时结束验收。保留 `game-info-time-domain/root-rejection-reversion.json` 和 `human-clock-lifecycle/` 反证，不在本轮擅自重写 human 时钟/ACK。剩余 body_miss 也不能一律称正常：现有缓存无法完成逐发双端碰撞形状和原生命映射归因，限制见 `residual-sync/shot-correlation-review/REPORT.md`。

继续只操作本项目复制客户端 `F:\valoranttrainer\client\ShooterGame\Binaries\Win64\VALORANT-Win64-Shipping.exe`；原 `F:\Riot Games\VALORANT\live` 只读，独立国服、账号和登录不碰。新构建/缓存/临时文件均在 F 盘，不打包；不得在项目客户端仍加载 DLL 时重建发布。Computer Use 已授权，仅限本项目窗口。WebUI候选仍为 `http://127.0.0.1:8792/`，普通启动与显式自动输入 fixture 要区分。

## 2026-11-04 历史接续

完整可玩验收仍为 **false**。当前以 [Root 接续索引](../work/local-server-playable/ROOT_STATUS.md)、[结构化进度](../reports/local-server-playable-progress.json) 和 [case50–58 运行记录](../reports/local-server-runtime-cases50-58.json) 为准。真实双端 Join、原输入/物理/ACK、射击/伤害/机器人重生、21武器接收、购买页交互及原技能部分行为已证明；原地图初次出生点已在cases57/58实测站稳。正在收尾两条原 Ares 专用警告入口兼容、原输入解码3231及长时稳定性，保留原同步与错误检查。全部30英雄/191个原起始装备引用的来源已核对；英雄选择与通用权威装备框架已编译，具体技能的生成、子枪、伤害和状态尚在接入验证，不能称全部可用。用户已明确恢复 Computer Use，仅操作本项目训练器和本机控制台；枪皮2D贴图预览已目视通过。下文为历史记录，不代表当前待办或运行进程。

当前重做以 [Root 接续索引](../work/local-server-playable/ROOT_STATUS.md) 和 [结构化进度](../reports/local-server-playable-progress.json) 为准。已验证真实双端原网络、HUD/持枪、原输入与机器人原物理。当前继续修复原射击结果来源、弹药 push 发布、购买 UI 协调器和长时输入积压；最新实际用例为 case34，完整游戏验收仍未完成。下文保留历史阶段记录。

## 当前重做进度 · 2026-11-03 06:00 CST

完整可玩版仍在重做中，未完成验收。以 `reports/local-server-playable-progress.json` 为当前证据索引；下文第一步报告是历史 DefaultPawn 样机。独立候选 `native/local_server_bridge.cpp` 已使用原网络 Join 和所有权完成真实捷风的双端 BeginPlay，客户端 A90、inventory/input、LocalPlayer/PC/PS/Pawn 绑定已成立。最新 case11 客户端发来79个原生输入包、92条不同命令；宿主因输入序号窗口3236停止，物理/ACK仍为0，双端0 Critical。武器、技能、伤害、机器人、模式及 WebUI 的新模块正在接入候选 runtime，不能仅凭源码或编译宣布可玩。可玩旧 DLL 和基本网络证明保留，原安装只读，所有新输出在 F 盘，不打包。

本地原生网络样机已实现并通过重复验证。优先阅读 [运行入口与边界](LOCAL_SERVER_PROOF.md)、[第一步验证报告](../reports/local-native-server-first-step.json) 和 `.local/development-current.json`。核心命令是 `python tools/local_network_probe.py` 后 `python tools/run_local_native_network.py --transport ip --observe-seconds 45`。

宿主/客户端使用当前同版原程序和原 IP 网络实现；真实 Join、自身 PS/Pawn(Role2)、远程原生 DefaultPawn(Role1)、原 AI 移动和连续属性复制已通过。样机补了 server 托管装配、已关联 Actor 的复制调度，以及每批原 ReplicationFrame4D8 推进；客户端位置、Role 和新 Pawn 数据不写。原 AresNetConnection 初始化仍存在本地依赖缺口。原 ShooterCharacter 输入/ACK/校正、捷风/枪械/技能/HUD 没有迁入，不可把此框架样机当作训练器替代版。

最终 45 秒重复用例目录 `.local/local-network/pair-20261102T161412.465612Z`：249 组有时序约束轨迹对照通过，3515 次原编码调用、139241 bits、4 个通道；双端0模块错误/0 Critical、原清理与进程退出通过、可玩 DLL/build.json 不变。191 项 Python 测试通过。所有构建和缓存仍在 F 盘；没有打包或复制完整资源。以下是前阶段研究与修复记录。

最新用户要求优先研究“原客户端连接自建本地服务器”，暂缓继续围绕客户端数据补丁修复。入口是 [本地原生服务器研究](LOCAL_SERVER_RESEARCH.md)、[结构化研究索引](../reports/local-server-research.json) 和 `.local/development-current.json`。

静态研究已完成：原驱动工厂、InitConnect/InitListen 实质代码和 Role1 远程角色 NetGUID/组件位流解码保留；常规 LoadMap 托管动作及若干服务器权威入口缺失。优先候选是同版本程序作为独立引擎宿主、集中补服务器权威，让客户端走真实 UE/Ares 连接。候选尚未运行证明，完整 host/join、own Role2 ACK/校正/回放和服务器状态流生产仍未知。后续先做斗牛一、一名真人、一个有效 NetGUID 远程角色的真实连接验证，不能用 HTTP、单纯改 Role 或现有本地 NPC 工厂替代。

本轮没有构建、启动、连接、操作或读取游戏进程，也没有改 88 个已构建 native 源文件、当前产品服务或原安装。研究证据只新增在 F 盘，原程序字节和文档完整性检查通过；实际连接成功仍为 false。用户仍有游戏 bug，前阶段自动回归不代表所有实际体验已经验收。

## 前阶段声音与界面修复记录

当前阶段以 [STATUS.md](STATUS.md)、[声音与界面回归报告](../reports/training-audio-ui-v3-verification.json) 和 `.local/development-current.json` 为准。最新用户已恢复运行本项目训练器的许可；不要操作独立国服客户端，不涉及账号登录，国际服安装目录继续只读。

乱斗鼠标焦点根因已由 NPC 初始化前禁用其本地 UI 服务工厂修复，用户已确认。声音修复已完成运行回归：实际 Ak 默认监听器收敛为本地 1、NPC 0，原音频蓝图按真实本地关系分流，保持模拟与原播放参数。30 人乱斗的菜单、购买状态、音频、101 次重生、本地死亡后 UI、原连杀计时和排名条重置均已通过；斗牛一的真实射击、菜单、购买、音频和本地重生也通过。已按保存的 revision 97 配置启动正常 stage8、12 人 Jam 乱斗，PID 129664；不发送自动测试输入，保留给用户试听。实际听感待用户确认，本轮不打包、不复制大体积资源。

以下保留早期开发记录；它们不能覆盖上述当前状态。

# 早期接续点（原系统接入阶段）

2026-11-01 最新用户验收反馈：跳跃低、持枪移速不对、不能下蹲；敌人移动时丢材质/动作与尺寸不对；HUD不像原版；开枪动画错误且没有后坐力。用户要求**尽量直接复用原玩家、原射击、原HUD代码**，不要把自编公式/单动画替代版当最终。此前允许补写缺失本地生成/入库/伤害，但不是允许用替代动画宣布原版等价。技能 C/Q/E/X 要实际可用。原始无账号登录约束持续。

## 当前最晚接续（20:10 CST，优先读；尚未完成，不要交付旧stage3）

- 用户最新再次验收：画面没有枪/捷风手/HUD，且两次Critical Error。不能拿原逻辑/指针验证冒充画面完成。Computer Use已再次经异步明确授权，sky可用；只操作F:/Riot Games目标窗口，不能碰独立国服。
- **原射击本体已实测跑通**：`original_weapon`补原EquippedEvent本地注册(原ServerOnly注册为空)，从真实Inactive.AutoEquip2响应复制原filter/原EquipState，用原AddEquipResponse追加Equip0，随后原图自然Equip→Idle→Firing。`.local/original-player-verification/20261101T110408.946201Z` 12发、原GetAmmo25→13、原Reload25、peakRecoil9.42、原移动/下蹲/115cm跳跃/相机全通过。Ammo+12c是初始配置，真实原GetAmmo读Resource+10c，已纠正，没写扣弹/换弹/散布公式。角色级ForceInput和原buffer泵不再要求Rifle当前，支持切技能后primary。
- **原弹道起点正确配对已实现，待重跑**：原FXC decoder711b6a0每项48byte只写方向24bytes，initial FireData的Start未初始化。112624短probe捕获原Pawn slot850的确切78840ca→71227dd→711f0ae FireShot链，12/12 source/Pawn/原shotIndex匹配，nativeStart[-2050,0,376.298]。`original_shot_observer.h`现对flags4强制唯一未消费原origin配对，flags新增8，不自造camera射线，方向仍原；队列+optionalSink并发消费者。Writer original-shot-origin-probe.json 已root接入；Full新assert检查完整worldstart，不能把旧count12当有效worldray。
- **物品栏刷错已实测归零**：`original_inventory_slots.h::EnsureForPawn`创建17真实原槽+原OnRep_ItemSlots+原查询；105449.275133Z runtime21618byte，0条ItemSlot错误。Multi6/7/8/13容量16明确为本地训练schema，非恢复server原分型。已有物品不覆盖；RetirePawn只清缓存，支持无限重生，带原WeakSerial/Index防GC地址重用。root local+3NPC均已接；120614四人均无Slot/Manager错误。
- **真正显示主阻塞已定位并由实测证实**：原ShooterCamera786a100直接取World弱PC列表218[0]，若其Player510为空就return，原world/1P projection不更新。旧EnginePC仍占首，虽GI.LocalPlayers已转realPC且GameplayStatics.GetPC0优先GI正确，投影专用路径仍读错。120614.557185Z original-render.json: WorldControllerCount5，firstPC!=active，firstPlayer=null，activePlayer=realShooterLP，LP38正确；camera+490、Scene3be0/3c60三个128B矩阵全identity/0变化。simulation agent正在找真实UWorld.RemoveController入口清旧注册，不能手改数组/矩阵或盲目销毁tick宿主。
- **NPC全被当第一人称的独立显示问题**：114717.548775Z四人3P core/cosmetic/gun全hidden，所有NPC1P visible。`original_enemy_view.h`新原API Set3PMeshVisible(true)→原IsFirstPerson=false，按实际PC.Player及GI.LocalPlayers仅对NPC，保Role，root已接Spawn后、enemyGun前（还未编译/实测该新头）。其余原敌人/材质/骨动作不替换。
- 原视觉类已由启动配置真实创建，`--original-visual-classes`：AresGameViewportClient+ShooterLocalPlayer，保Engine.GameEngine/原AresGI engine-only Init不登录。114717与120614短probe成功，原类型无CalcSceneView额外override，类型迁移本身没解决projection。AresGameEngine本身也无Tick/Draw override，别为此启动平台Init。
- `original_effects.h`已接真实Pawn/PC/全部技能/枪原组件生命周期，13manager中仅4技能缺manager而创建；原BeginPlay/原timer Start各1，所有defer false，120614 runtime0条no effect manager（本轮未fire），没手发FXC。Skin原Load回调一次，StandardChroma原路径0可合法无callback，Champions effect override待actual画面/排名/kill验证。
- 原4技能已真实grant/refill跑通：114717+120614 original-jett.json，C2/Q1/E1(读原CDO)、X原飞刀、ult8/8 ready，均真state/resource/owner。技能实际按键执行待stage9测试；Q/E缺失genericForceApply时可选原DeterministicForceModule桥尚未自动启用。
- HUD目前仍未完成。original_hud.h依次补原空Social/Party/Premier/Platform/两lock等服务，不在线Init；最后probe13已有Platform补丁待跑probe14。下一短原HUDprobe应带真实visual classes+技能，原core factory已在Options菜单之前执行，但整体coordinator没返，不能把它说显示成功。
- **Critical已保存两次原栈**：104132.441494Z是模块每字段VirtualQuery触发hang，OriginalPlayers.Read已改SEH POD copy。110911.149328Z第二次hang原ParticleSystemComponent finalize→attachment transform→Skinned.GetSocketTransform/GetBoneTransform（424a33f..483c360），当前cause仍未证实，不能以投影修复就宣称稳定。已新增NativeLogMarker增量扫描，原runner不再每20ms读完整巨量log，发现GameThreadHang及时清理owned child。仅短14s探针，后续必须含射击/皮肤长测才能关闭Critical项。
- 未接完：original_damage.h/original_damage_tuning.h/respawn headers已交未root运行。要用原NativeShot(direction+matched originalStart)、原Core19body nativeTrace、原CDO Damage curves→原PointDamageResponse/Life/Death原事件→scoreboard只累加且不重复broadcast→原新Pawn重生保PC/PS/PlayerArray/stats。不能回旧Python枪或单动画模型。
- 当前root只编译/运行；队友现重点：original_simulation=原PC列表清理+lifeprobe，original_camera=render/projection/原hang，original_hud=原HUD，original_enemies=NPC3P view，original_damage=worldshot observer/damage，original_content=effects。全部旧Context对象保留供诊断，不要触国服/不要写安装EXE。Python109tests已过，Native每次manifest/source-snapshot验证。

## 较早接续（18:15 CST，以下部分已被以上取代）

- 用户说明同时运行的是独立国服，已用只读 QueryFullProcessImageName 验证国服在 G:/Program Files (x86)/Tencent Games/VALORANT；训练器目标仍 F:/Riot Games/VALORANT。`run_native_probe` 预检改为按目标安装树区分，路径未知仍阻止，外国服进程不进入修改或清理。11项预检测试通过，国服保持运行。
- **原物理/装备阻塞已解**：`original_simulation::SelectOriginalLocalMovementBranch()` 经原 setter7002070 关闭真实 feature81 `BotMovementDowntickingEnabled`，保留原通知并原query回读。probe20 `.local/native-baseline/20261101T100822.017024Z` 原physics735次/clock735，AK Current/ready=true；不手调Tick/不改Role。
- **原跳跃实测成功**：新显式stage7模块测试 `.local/original-player-verification/20261101T101102.999977Z/verification.json`，跳高114.9606cm，原CDO115cm；原run540cm/s。尚失败：walk/crouch被原7881110从Controller.PlayerInput重置，皆仍540/capsule98；fire flags9没有消费弹药/rays0，故原射击仍未完成。simulation agent改原ForceMoveInputState持久override，并查AresInputComponent原command泵，不能改enum/自写fire替代。
- `original_probe_channel.h` / `original_probe_channel.py` 仅stage7接收显式带0.3s租期的测试命令，不做OS输入；stage5也发布原位移/速度/胶囊/后坐力/真实shot ring，生产不接受测试命令。`tools/verify_original_player.py`生成阶段采样与原系统断言，不起Python武器模拟器。
- **ContentIndex真实初始化成功**：`original_content.h`复用原GI纯叶子构造70470d0、原同步初始化71c4300；probe8 `.local/native-baseline/20261101T101222.176078Z` 原32个hero全部GUID查回一致，36equippables/1433skins/2723levels/2972chromas。ready由原代码写，无空表替代。
- 原HUD probe8越过ContentIndex后新AV18ce6e6<-28b5f3f<-7b5a510：ClientGI+420 SocialExperienceModel空导致delegate228。HUD agent补原空provider纯构造，随后测stage6。root已加入 `original_enemies::BindRosterPlayer(0)`，原PS的DesiredGUID/Spawned/Possessed/Team需真实关联。
- `original_enemies.h`新原英雄敌人封装已落盘，尚未实测。真实PC/PS/Character/SCS/AnimBP/材料保持，AI只提交原MoveForward/Right；原YellowGreen renderer outline。root尚未Spawn，不得宣称材质/动作验证。
- 纠错：真正原PreloadManager在ClientGI+3f0，+448是InventoryManager。`original_champions.h`已修正分别按真实类型检查；两个本地服务仍待接入，不能把+448构成PreloadManager。
- 当前agent: original_simulation(移动/射击原输入)、original_content(真实Inventory/Preload服务)、original_hud(全原HUD)、original_enemies(原敌人/PS关联)。root独占编译/运行，所有原测试均清理自有子进程，保留国服。

## 较早接续（16:35 CST，部分已被以上取代）

- 原完整角色路径已实机成功：真实AresGI(baseEngine生命周期)→真实BPAresClientGI→原Config/Events/MapLoad/Settings/MenuStack/Navigation服务→原Settings异步Init→**711条SettingData + 原ready=true**→原GS128→原ShooterPC+ShootingRangePS+原Wushu→LocalPlayer转移成功。原原生Owner/Inventory/Attributes/Movement均有效。
- Settings首次异步AV 18bfec1<-75d9724<-75f321d 是ClientGI+750 RSOManager缺失。已仅构造原本地RSO对象，保留空identity数组，不调用Init/Auth/Login；其Ctor两导入经目标进程严格核对为临界区初始化。修复后Settings711ready实测通过。原设置INI始终写项目`.local/profiles/GameUserSettings.ini`。
- 原AK已实机创建/绑定/原TryEquip返回true。缺失主槽由原ItemSlot对象本地补齐（原factory缺失，非重写射击）。状态：Desired1、Current0，原Ammo25、rate9.75、error.25、装备moveRatio.8。**还没有原枪真正射击**。
- 主阻塞为模拟不推进：原movement GetMaxSpeed742.5(无当前枪)、JumpVelocity681.481、capsule98；MovementIsDisabled=false，但total_moves0。反射实测 ActorTickEnabled1、MovementTickEnabled0、LocalRole3 Authority/RemoteRole1。在ActivateLocal后补原ClientRestart（真实UFunction ParamSize8）仍无效。config agent查GameEngine/central scheduler；jett_abilities查movement组件/role/客户端预测门槛及原ExecuteMove桥。不写自造物理/后坐力。
- 原ShooterCameraManager 当前Default/Locked103而POV90，真实eye376（floor200+176）正确原比例；原最终投影还须查，不能照搬GenericPCM结论。
- **原hitbox现在实机通过**：`.local/training-verification/20261101T081251.005063Z/verification.json`，26个原形状查询，4真实头部命中/4击杀，遮挡/重生/换弹通过。原follower有LeaderPose时原ShouldCreatePhysicsState故意false，因此新增同gBot内无Leader的隐藏Core SkeletalMesh physics proxy，19真实Bodies/Actors，用原FBodyInstance.SetBodyTransform同步可见骨，再原K2_LineTraceComponent。不是自造球。保持同gBot被环境trace忽略。
- 保活原组件引用修复曾成功100秒47次英雄切换无AV（此前约90秒失效）；视觉材质/动作仍要最终原角色验收，不把这次稳定性当全部完成。
- 原完整HUD在stage6独立测试，尚未成功。现用原配置`ui.ingame.pcshell.enabled=false`（原ConfigManager写+原getter defaulttrue回读false）选择Shipping自带InGame-only分支，原HUD所有核心控件保留，设置走原InGameMenu，避免PCShell party依赖。
- 原HUD最初PartyManager+770 null已由原分支避开；后AresRGIFriendsClient+7a8 delegate null已仅纯构造原空provider解决。**最新HUD失败** `.local/native-baseline/20261101T083432.767980Z/original-hud.json`：1982336 RCX120 <-722a3d9 <-7229bb6 <-7d7d25c <-7d7d5cf；原Options/InGameMenu binding阶段另一个provider缺失。UI agent继续查。现在有96帧 SEH context/unwind诊断，原HUD WriteDiagnostics在SaveState自动原子写。
- stage5=原player+weapon探针，stage6=原player+完整HUD探针（不先spawn枪），都需`--original-game-instance`。暂不运行生产stage3冒称原系统交付。
- 所有native源在编译前后内容必须一致；编译成功后保留`.local/native-module/source-snapshot`，manifest验证这份编译源码快照和DLL，避免并行开发修改当前源码使测试进程误拒绝已构建DLL。源码快照不是跳过DLL身份检查。新源码改了依旧要重新build才能进入新运行。
- `run_native_baseline`结束后归档本次Native original-player/weapon/hud JSON到run_dir（只收mtime晚于本次start，防旧数据混入），错误退出也检查进程清理/安装EXE哈希。
- 最新103项Python测试通过。最后原player probe10 run `.local/native-baseline/20261101T082956.317238Z` 20秒完成，仍0moves。最新HUD probe5 run `.local/native-baseline/20261101T083432.767980Z` 已退出。
- 现工作分工：config_native(+static)负责原scheduler/GameEngine；jett_abilities重新激活负责原movement role/预测；duel1_respawn负责完整HUD缺provider；original_weapon_runtime已交原slot/shot观察/Champions/scoreboard模块；bot_weapon已交原physics proxy和原enemy接入边界。
- `original_scoreboard.h`已交但父尚未集成；实际PS BasicCombatStats/OwnerExclusiveInfo原GetKills回读，原冠军蓝图自行排名；`original_champions.h`等待原PreloadManager，不能只material覆盖冒充完整原skin。

## 更晚的实机接续（15:32 CST）

- `--original-game-instance` 已加到 `tools/run_native_baseline.py`。`tools/native_offline_gi.py` 在启动日志刚建立后对本次拥有进程的 AresGI vtable Init/Shutdown 两个数据指针使用 PAGE_WRITECOPY 私有修改到原 EngineGI 生命周期；不改指令/安装文件。原rdata启动后是 MEM_MAPPED，只允许该原地址/原指针/PE静态数据段。启动前修改不生效，runtime PAGE_READWRITE被Windows拒绝，最终COPY成功；不可改回错误方案。
- 成功run `.local/native-baseline/20261101T071003.789114Z`，`reports/duel1-original-gi-startup.json`：实机AresGameInstance+LocalPlayer1+原地图+DefaultPawn稳定；hash未变。
- `native/original_hud.h::InitializeServices(world,controller)` 创建真实BPAresClientGI并双向原属性持有，未调用online Init。`InitializeUIServiceObjects()`已实机创建原ConfigManager/ClientEvents/MapLoadModel/SettingsManager/MenuStack/Navigation原工厂。
- **原角色成功进展**：run `.local/native-baseline/20261101T073204.360464Z`，原PC+ShootingRangePlayerState（包含BombTeam SCS）成功，原Wushu Character `ready=true/character_initialized=true/registered=true`，Inventory/Mesh1P/Abilities/Attributes/Movement非空，nativePlayerCount2。最新失败是ActivateLocal门槛763 `original_existing_settings_manager_not_initialized`。UI agent正在确认原 BeginSettingsInitialization 的 AssetManager前置，父需异步Poll原ready/设置数据，再ActivateLocal。
- `original_movement.h` / `original_weapon.h` / `original_shot_observer.h` / `original_champions.h` 已包括进stage5，原角色激活后才依次初始化（当前尚未执行到它们）。原射线有原最终Start/Direction，首发绕PE路径也有被动观察，不用旧spread/recoil公式。
- 原PhysicsAsset `native/enemy_hitboxes.h` 已接旧stage3/4渲染分支；opcode2查询真实骨体，region@424/id@428。opcode1 worldtrace显式ignore gBot。Host生产必须native target callback；旧三球只显式fixture使用。Frame新增target_id@188/head double3@192用于自动验收瞄准。103项Python测试通过；最近实际hitbox验收因agent修改header导致manifest变化在注入前中止，**尚无真实命中通过证据**。
- 完整原HUD/原champions/原移动/枪实际扣弹后坐力、原敌人AnimBP以及>90秒保活回归仍未完成，继续做，不作为最终交付。

## 最新已证实

- 实机原 BasePawn/BasePlayerCharacter/Wushu_PC CDO：capsule half98/radius42，CharacterScale1，站立 view root+76，原 BasePlayer/Wushu mesh相对Z=-100/scale1。因此脚底到眼约174，而此前通用Engine.Character约152，不能靠缩小英雄模型处理差异。原数据在 `.local/native-module/original-player-defaults.json`。
- 实际 FOV 修正：PC.FOV只写LockedFOV+48c，普通Pawn实际POV每帧来自 DefaultFOV+484。现在写已验证原字段，已实机读回 actual_fov103。见 `player_avatar.h::ApplyFOV`。
- 原枪动作根因：目前SingleNode Fire替换原姿势，原游戏用AnimBlueprint/Montage/状态机；并且旧 presentation.Update 对齐枪骨覆盖此前 recoil变换。原枪新模块不会沿用这条射击算法。
- 原射击可用路径已建 `native/original_weapon.h`：原 AK Blueprint + Firing/Ammo/Stability/EquippableAnimGraph，原ForceInputState/TryEquip；仅补已证明为空实现的生成/空主槽数据。尚未实机跑到装备阶段。
- 原GameState已实机创建成功：FixedMoveTimeStep target/actual128，flags104。原玩家stage5首次停在 `original_player_state_team_component_missing`（736），**没有冒充原玩家已完成**。报告 `.local/native-module/original-player-probe.json`，run `.local/native-baseline/20261101T064316.508906Z`。
- 技能已在原客户端通过11项自动测试：X/5刀/耗刀/击杀补充/右键全投/枪弹保留，C烟雾生成消失，Q真实升高，E真实水平位移。`.local/ability-verification/20261101T063827.990096Z/verification.json`。这是本地技能规则验收，不是原枪/原HUD验收。
- 原 UMG 容器+原地图/技能纹理已创建且加入viewport，`original_ui_flags=3,error0`（同上技能run）。原图不等于完整原HUD，完整ShooterHUD/ClientGI服务仍在接。
- 斗牛1原小地图世界标定 `u=.5+Y/12800,v=.5-X/12800`；原相机/原矩阵与两只箱体8条边像素交叉验证。见 `reports/duel1-map-calibration.json`。
- 两次约90秒后AV已定位 SetLeaderPoseComponent 重建映射访问已失效的mesh asset指针（424c80c/4245f99），不是直接bone索引越界。enemy候选mesh/动画现用固定隐藏原组件持有UPROPERTY引用；需要超过90秒切换测试确认。
- enemy原生outline改为 AresOutlineComponent.PushMeshOutline Enemy3 和原renderthread颜色接口，不再用冠军Fresnel重叠人物壳。原LB仅19下半身轨已证实；当前备用Poseable合成保留原上身，真实原角色AnimBP路径由原玩家集成接替。
- 当前92项Python测试通过；新增 `training_settings.py` + HUD Esc菜单有本地设置持久化与native敏感度/FOV/outline通道，**仍不是完整原版设置页**。

## 目前代码及运行

- `trainer_bridge.cpp` stage3: 旧可玩本地桥接；stage4: 显式自动测试输入；**stage5: 原GameState→原Jett玩家→原AK的诊断探针**，不会启动旧Python射击host，以免双流程。
- stage5：`python tools/build_native_module.py` 后 `python tools/run_native_baseline.py --run --seconds 35 --bootstrap .local/map-probe/20261101T020214.576649Z/map-1.bin --map 1 --mode navigation --observe-world --training-module --module-stage 5 --no-training-hud --result-file .local/duel1-training/original-player-probe-run.json`。
- runner现在检测native marker非零error即停止所属进程；保留异常地址/operation，避免继续运行破损对象。
- build manifest现自动绑定native目录全体.h/.cpp/.cmd和DLL；源码有改必须重建。不要在正在运行的DLL上重建。
- `native/original_players.h`负责诊断、原GS/PS/PC/Pawn、LocalPlayer转移与回退；默认关闭创建，stage5显式开启。异常后缓存诊断仍可导出。
- 当前全原射击、真实下蹲/原移动、原敌人AnimBP与完整HUD均未完成，不要结束为交付。

## 独立工作分工

- duel1_config_native：原玩家，正在修PS Team组件生命周期736；其子代理 static负责字段/ABI。
- original_weapon_runtime：原AK状态机/输入/冠军skin原应用与shot事件输出。
- jett_abilities：已完技能验收脚本，现在查原移动、JumpTuning、持枪移速、自定义crouch消费。
- duel1_bot_weapon：敌人动画/原outline/资产保活，进一步查原hitbox与原AnimationState覆盖入口。
- duel1_respawn：原UI资源和map标定已交，当前继续完整原HUD/ClientGI本地服务bootstrap（不登录）。

---
以下为早期接续记录（部分已被上述取代）：

# 当前接续点（开发中，未作为全部完成验收）

用户要求在斗牛 1 完成持枪、射击、随机敌人，并明确接受：保留原客户端场景/原资源/可用客户端逻辑，补写缺失的本地生成、伤害与目标控制。后续追加：捷风玩家本体及第一人称手臂、修正视角/FOV/尺度、冠军狂徒领先击杀发光和原特效、敌人改为随机英雄而非机器人，正常动作，黄绿色敌方高亮。禁止账号登录继续有效。

## 已实际跑通

- 原客户端进程中以标准 LoadLibrary 加载自建 DLL；DefaultPawn 实例复制 vtable 的 Tick 槽回调在游戏线程稳定运行。
- `native/trainer_bridge.cpp` + `native/engine_api.h` 使用原引擎 SpawnActor/反射函数创建原冠军狂徒模型、原目标骨骼模型，移除本次 World 两个出生屏障的显示/碰撞。
- 共享内存协议 `Local\\ValorantTrainer-{pid}`：frame、scene trace、render；具体偏移见代码。Python host 用原生 trace 验证原地图路线，并逐发检查遮挡。
- `tools/local_training_rules.py` / `training_channel.py` / `training_host.py` / `training_hud.py` 已接入真实鼠标输入、弹药/换弹、随机拉出/血量/击杀/下一轮、透明 HUD、Tab 暂停、Esc 请求结束所属进程。
- 自动运行验收文件：`.local/training-verification/20261101T045154.965591Z/verification.json`。原客户端内 12 发、4 次爆头击杀、换弹、再生均观察到；无安装 EXE 修改、无遗留进程。此记录先于完整捷风/敌方英雄升级，必须在最终代码上重跑。
- 捷风 1P 原手臂通过 `native/presentation.h`：隐藏 FP_Core_Retarget 播放原 AK Idle/Fire/Reload，Wushu 手臂 LeaderPose；原冠军材质与轮廓曲线、枪口粒子已接入代码。实际已看见捷风手臂与枪对齐，截图 `.local/duel1-training/jett-hands-idle.jpg`；原动画、领先发光和粒子仍需最终实测。
- 80 项测试目前通过。实际渲染/输入不以离线测试代替。

## 正在集成

- `player_avatar.h` 由静态分析 agent 编写：Engine.Character 本体、Jett 3P Mesh、Possess、胶囊/步行/跳跃、真实 CameraManager 摄像机与 FOV103。现用 DefaultPawn 仍需替换。Engine.Character 的 Crouch/UnCrouch 在当前客户端无有效实现，不能宣称已支持蹲下。
- `enemy_presentation.h` 由资源 agent 编写：至少三种原英雄、共享原 TP 动画、正常横移动作、死亡表现、黄绿色轮廓。当前父模块的目标初始 Mesh 已改为 Jett 3P，不再回退 TrainingBot；最终仍需接随机英雄与动画模块。
- 分辨率已改 1280×720，HUD 透明底从洋红改为接近黑色以减少彩边。
- `tools/run_duel1_training.py` 与 `dev.ps1 play` 已创建；最终需接入上述模块、重跑验证和视觉检查后交付。

## 关键代码与证据

- `reports/duel1-native-bridge-abi.json`：Tick/ProcessEvent/LoadObject/Spawn 等固定镜像 ABI。
- `reports/duel1-render-call-layouts.json`、`duel1-trace-call-layouts.json`、`duel1-presentation-call-layouts.json`、`duel1-character-call-layouts.json`：真实反射参数偏移。
- `reports/duel1-jett-champions-assets.json`：Jett、原动画、冠军曲线/资源。
- `native/presentation.h` 是独立模块；Update 在父枪变换之后执行，默认把枪对齐到 Core WeaponGameOverride。全局计数器报告动画/粒子/变换调用，须结合实际画面。
- 运行前 `python tools/build_native_module.py`，manifest 绑定源与 DLL；任何 header 增加必须加入 build/native_module 两个 Python 文件的 manifest allowlist。
- 目前开发启动：`python tools/run_native_baseline.py --run --seconds 120 --bootstrap .local/map-probe/20261101T020214.576649Z/map-1.bin --map 1 --mode navigation --observe-world --training-module --module-stage 3`。正式启动器会重新准备配置，无需写死历史路径。
- `tools/verify_native_training.py` 使用 stage4 专用测试输入，不是人工输入证据；stage3 不接受它。对最终版重跑，必要时保存击杀后 glow/枪口/换弹截图。

失败实验：启动前修改生成属性 Config 位虽可读回，但没有进入引擎日志；同样 suspended+job 无修改对照正常。保留在 `native_class_config.py` 供研究，正式训练入口不使用它，不将失败归因于未经证明的保护机制。
