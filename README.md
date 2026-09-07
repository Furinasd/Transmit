# Transmit

> 把世界已有的运动，交给新的用途。

**Transmit** 是一款基于 **Unreal Engine 5.8 / C++ / Blueprint** 的第三人称战斗解谜原型。玩家扮演巨型电子主机中的临时检修工，从物体或敌人身上截获 **Motion State（运动状态）**，再将它交给新的载体：接通断桥、穿越维修通道，最终把敌人的冲锋转化为打开出口的力量。

项目围绕一个技术核心展开：**让同一份运动资源，在玩家、机关与敌人之间可预测地流转。** 状态所有权、方向解析、目标预览和失败恢复共同支撑连续的 Learn → Route → Weaponize 体验。

[玩法与制作作品集 · Notion](https://app.notion.com/p/e916dbf617ac8213bf0b01b2bde83939) · [架构说明](Docs/ARCHITECTURE.md) · [运行与封包](Docs/PACKAGING.md) · [当前状态](Docs/STATE.md)

## 一份运动，三种用途

玩家的核心操作始终是 **E 捕获 / Q 转移**。关卡逐步改变资源的用途和空间约束，让解谜与战斗共享一套交互语言。

| 阶段 | 玩家行动 | 系统如何支撑体验 |
| --- | --- | --- |
| **Learn · 改变通路** | 取走 Source 的运动，交给桥板，走过新接通的道路 | 状态交接直接改变世界中的物体运动，建立资源归属与功能之间的联系 |
| **Route · 组织路线** | 发送载体，沿检修廊追踪，再次截获并改变方向，最终送入轨道 | 六向解析、可再次捕获的 Carrier 与 swept collision 支撑人和载体的不同路线 |
| **Weaponize · 利用威胁** | 在 Charger 冲锋窗口截获 High Motion，交给反击装置，两次有效命中后离开 | 冲锋状态机、提交时锁定的直线反击与真实碰撞判定组成战斗闭环 |

正式体验位于单张 `/Game/Transmit/Maps/L_Transmit`，包含高差检修路线、共享资源复用、载体入轨、普通试运行与 Boss 反击。`L_TestChamber` 保留为最小交接实验和能力回归入口。

## 技术亮点

### 原子状态交接与唯一所有权

`FMotionState` 以值承载类型、方向、强度和来源标识；`UMotionTransferComponent` 集中管理运行时状态写入。Player、Source、Carrier、Receiver 与 Charger 通过 `IMotionTransferable` 参与同一套交接协议。

- **单槽持有**：玩家一次只能携带一份 Motion；成功交接清空原持有者，再赋予新持有者或由接收端消费。
- **提交前校验**：资源、接收能力与兼容性检查先于状态变更；后置条件检查保护交接结果。
- **结构化失败**：`SourceEmpty`、`CarrierOccupied`、`IncompatibleDirection` 等拒绝原因可供 UI、日志和测试读取；拒绝保留原资源。
- **通知与写入分离**：状态更新完成后再派发结果与状态事件，供表现层响应。

核心入口：[`MotionTransferComponent.cpp`](Source/passely/Private/Motion/MotionTransferComponent.cpp) 中的 `TryMoveBetween`。

### 确定性方向解析，Preview 与 Commit 共用结果

普通 Linear Motion 的输出由 **CameraCanonical** 将 gameplay camera 量化为世界 ±X / ±Y / ±Z 六向；阈值和滞回稳定方向边界。目标选择独立处理瞄准、遮挡、距离和短暂粘性，提交时重新验证目标资格。

解析结果通过 `FMotionTransferContext.DirectionResolution` 传递，预览与提交消费同一结果。方向箭头和目标提示由实际交互数据驱动，减少“看见的结果”和“执行的结果”之间的偏差。

**High Motion 有明确的接收端语义**：捕获与携带仍保留 Charger 已提交的 Dash 方向；当前轨道反击装置在 Q 提交时计算指向 Boss 当前地面位置的向量，锁定后直线运动，不在飞行中追踪。反击预览、实体朝向与冲击表现共用该输出。此行为对应设计契约的 2026-09-07 refinement。

### 受控运动与真实命中分离

`ATransmitDirectionalCarrierActor` 沿解析后的世界方向移动 Actor 本体，通过 swept collision 在阻挡处停止；运动中的载体可再次捕获，取回资源并停止运动。

Boss 流程将 **资源交付成功 → 实体发射 → 实际命中** 分开处理：提交成功本身不增加破门进度，落空也不计命中。两次有效冲击分别驱动受损与开门，让操作反馈对应实际玩法结果。

### 分层恢复与事件驱动表现

`AMotionRoomResetController` 保存并恢复房间权威快照；关卡流程提供 Learn、Route、Arena 三段会话检查点。局部重试保留已完成进度，Arena 重试保留入轨与已提交的门损伤；完整重启恢复初始资源和机关状态。

HUD、方向指示器和 `ATransmitPresentationRig` 根据运行时状态与事件组织提示、材质、音效和镜头反馈。默认 HUD 保留交互与资源状态，详细解法按住 Tab 展开。

上述恢复能力覆盖已有验证路径；普通 Motion 满槽进入 Boss 房仍有未解决的恢复缺口，见下方当前边界。

## 实现结构

```text
Enhanced Input / Character
            │ E / Q
            ▼
UMotionInteractorComponent
  目标选择 · 遮挡/距离 · 方向解析 · Preview
            │ FMotionTransferContext
            ▼
IMotionTransferable / UMotionTransferComponent
  资格复核 · 原子交接 · 所有权 · 拒绝结果
            │ 状态与事务事件
            ▼
Source / Carrier / Receiver / Charger / Ram
            │ 运动、碰撞与关卡进度
            ▼
HUD / Direction Indicators / Presentation Rig / Blueprint
```

**C++** 承担状态不变量、交接事务、方向解析、受控运动、关卡流程与恢复逻辑；**Blueprint 与 Unreal 资产** 承担角色装配、输入接线、配置和表现内容。当前实现保持在一个 Runtime 模块内；新增能力优先通过 `L_TestChamber` 验证，再进入正式关卡。

| 路径 | 内容 |
| --- | --- |
| [`Source/passely/Public/Motion/`](Source/passely/Public/Motion/) | 状态类型、交互接口、组件与玩法 Actor 声明 |
| [`Source/passely/Private/Motion/`](Source/passely/Private/Motion/) | 状态交接、方向解析、Carrier、Charger、Reset 与 HUD 实现 |
| [`Source/passely/Private/Transmit/`](Source/passely/Private/Transmit/) | 主关卡流程、检查点与反击装置 |
| [`Source/passely/Private/Presentation/`](Source/passely/Private/Presentation/) | 运行时表现协调 |
| [`Source/passely/Private/Tests/`](Source/passely/Private/Tests/) | Motion 与玩法自动化回归 |
| [`Content/Transmit/`](Content/Transmit/) | 正式 Blueprint、地图与表现资源 |
| [`Scripts/`](Scripts/) | Editor 验证、内容工具与平台封包脚本 |
| [`Docs/`](Docs/) | 设计契约、架构、决策记录与验证报告 |

仓库名为 **Transmit**；Unreal 工程、Runtime 模块与工程入口仍使用 `passely`，保留现有序列化引用。Unreal 二进制资产通过 Git LFS 管理；`Content/__ExternalActors__/` 属于关卡源资产，不能作为缓存清理。

## 运行项目

需要 **Unreal Engine 5.8、对应平台的 C++ 工具链、Git 与 Git LFS**。Windows 还需与引擎匹配的 Visual Studio C++ 工具链及 Windows SDK。

```bash
git clone https://github.com/Furinasd/Transmit.git
cd Transmit
git lfs pull
```

使用 UE 5.8 打开 `passely.uproject`，完成 C++ 模块编译，在 `/Game/Transmit/Maps/L_Transmit` 中启动 PIE。项目的 `EngineAssociation` 为 `5.8`。

| 输入 | 行为 |
| --- | --- |
| WASD / 鼠标 / Space | 移动 / 瞄准 / 跳跃 |
| E / Q | 捕获 / 转移 Motion |
| 按住 Tab | 展开详细帮助与解法提示 |
| Backspace | 重试当前区域，保留已完成进度 |
| R | 从开场完整重启 |

### 封包入口

Windows：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/package_ltransmit_windows.ps1 -EngineDir "C:\Program Files\Epic Games\UE_5.8"
```

macOS：

```bash
bash Scripts/package_ltransmit_mac.sh
```

平台脚本和产物验收要求见 [`Docs/PACKAGING.md`](Docs/PACKAGING.md)。Windows 分发需包含整个打包目录，不能只发送 `passely.exe`。

## 验证证据与当前边界

以下为仓库记录的 **2026-09-07 历史检查点**，具体来源、版本和限制见 [`Docs/STATE.md`](Docs/STATE.md) 与 [体验迭代报告](Docs/dev/20260907-experience-refinement.md)。

| 验证层级 | 已记录证据 | 证据边界 |
| --- | --- | --- |
| Build / 自动化 | Mac Editor 构建通过；27 项 Transmit 自动化通过 | 不证明 Win64 构建或玩家体验 |
| 正式地图 PIE | 实际移动与 E/Q 连续通关，166.754 游戏秒，无传送或资源注入 | 熟练脚本的已知解路线，不代表真人首玩时长 |
| 回归与恢复 | 离轴反击、发射后落空、捕获范围、三段检查点及完整重置记录 | 含显式布置的专项 fixture，与干净通关证据分别记录 |
| 编辑器地图检查 | Map Check：0 errors / 0 warnings | 不替代可读性、镜头和美术验收 |

当前仍需完成：

- **满槽入场恢复**：携带普通 Motion 进入 Boss 房会因 `CarrierOccupied` 拒绝 High 捕获，尚缺已验证的交付／恢复路径。
- **装置身份与演出**：前段 SUV 与后段苹果折叠屏尚未通过独立形态和动作充分区分。
- **平台与人工验收**：Win64 封包、独立包完整通关、首玩理解、节奏、镜头、视听平衡与前台性能仍需验证；5–7 分钟体验时长仍是待测目标。

L4 / L5、Promotion 合成与分裂反弹属于纸面扩展，未计入当前可玩功能。完整差额见 [`Docs/submission/KNOWN_GAPS.md`](Docs/submission/KNOWN_GAPS.md)。

## 进一步阅读

- [Notion · Transmit 作品集](https://app.notion.com/p/e916dbf617ac8213bf0b01b2bde83939)：玩法定位、设计演化与制作复盘。
- [`Docs/GOAL.md`](Docs/GOAL.md)：产品目标与成功标准。
- [`Docs/DESIGN_CONTRACT.md`](Docs/DESIGN_CONTRACT.md)：实现授权规则及后续修订。
- [`Docs/ARCHITECTURE.md`](Docs/ARCHITECTURE.md)：职责边界、数据流与架构说明。
- [`Docs/Decisions/`](Docs/Decisions/)：状态模型与方向策略的决策依据。
- [`Docs/STATE.md`](Docs/STATE.md)：版本对应的验证记录与剩余门槛。

Notion 提供设计背景与展示叙事；已批准的玩法以仓库设计契约及其后续修订为准，实际能力以源码、资产和对应版本的运行证据为准。
