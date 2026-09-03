# Transmit

> 一款围绕“转移运动状态”展开的第三人称 3D 战斗解谜原型。

玩家不直接抓取或移动物体，而是从世界中的对象、敌人和机关上读取、捕获、携带、转移并转换 **Motion State（运动状态）**。同一套运动规则同时服务于战斗决策与环境谜题。

## 项目概览

| 项目 | 当前状态 |
| --- | --- |
| 当前本地工具链 | Unreal Engine 5.8（最近一次本地检查为 5.8.1；EngineAssociation 属于工作站本地差异，不是项目版本号） |
| 当前阶段 | Part 1：v0.3 Core Logic Coverage（Gate A 验证中） |
| 当前实现 | EXP-001 闭环 + v0.3 Canonical Direction Resolver / Preview=Commit / Charger Core |
| 核心目标 | 在 `L_TestChamber` 完成 v0.3 PIE、micro cells 与 Charger 可玩验证 |
| 版本管理 | Git + Git LFS |

> [!IMPORTANT]
> **EXP-001 Engineering Validated；v0.3 Core Implemented**：原始 `Source → Player → Receiver` 闭环与 20/20 Reset 已在 PIE 验证。v0.3 direction semantics 已提升到契约；Gate A code support 已完成 macOS Editor 构建与 10/10 Motion 自动化。新语义 PIE、micro cells、关卡内 Charger、human readability、打包与完整跨平台验证仍未完成。

## 核心玩法

```text
读取 Read
   ↓
捕获 Capture：从运动中的 Source 取走 Motion State
   ↓
携带 Carry：玩家临时持有唯一一份 Motion State
   ↓
转移 Transfer：将状态按 gameplay camera 解析出的 Canonical Direction 交给兼容 Target
   ↓
转换 Convert：由明确的环境规则改变方向或运动类型
   ↓
触发 Function：驱动机关、位移、碰撞或战斗结果
```

核心设计约束：

- **唯一所有权**：一份 Motion State 同一时间只属于 Source、Player 或 Target 中的一方。
- **方向语义**：Capture / Carry 保留 Source Motion；Transfer 时由 gameplay camera 确定性量化到六个 Canonical Direction，Preview 与 Commit 共用同一解析结果。
- **战斗与谜题共用语言**：敌人、机关和环境都遵循同一套运动状态规则。
- **可读、可恢复**：合法目标、拒绝原因和状态归属必须可感知；关键资源可通过房间重置恢复。

## 当前仓库内容

```text
Transmit/
├─ Config/                 # 项目默认配置、输入与启动地图
├─ Source/passely/         # EXP-001 Motion 核心 C++：状态、事务、接口、Reset、自动化测试
├─ Content/
│  ├─ ThirdPerson/         # 当前角色、GameMode 与入口关卡
│  ├─ Transmit/            # EXP-001 输入、蓝图与 L_TestChamber
│  ├─ Input/               # Enhanced Input 资源
│  ├─ LevelPrototyping/    # 门、跳板、目标与灰盒资源
│  ├─ Characters/          # Mannequin 角色、动画与材质资源
│  └─ __ExternalActors__/  # World Partition 外部 Actor 数据
├─ Docs/                   # 目标、设计契约、架构、ADR 与状态快照
├─ passely.uproject        # 当前 Unreal 工程入口
├─ .gitattributes          # Unreal 二进制资源的 Git LFS 规则
└─ .gitignore              # Unreal、IDE、测试与本地生成文件规则
```

仓库名为 `Transmit`；为避免在尚未验证引用关系时进行高风险资产重命名，Unreal 工程文件及内部工程名暂时保留为 `passely`。

## 目标技术架构

当前工程已实现“C++ 核心规则 + Blueprint 表现与关卡装配”的边界（EXP-001）：

```text
Enhanced Input / Blueprint
            ↓
Motion Transfer Core（C++）
  ├─ FMotionState
  ├─ UMotionTransferComponent
  ├─ IMotionTransferable
  └─ 原子 Capture / Transfer 事务
            ↓
Player / Enemy / Environment
            ↓
状态事件与结果数据
            ↓
Blueprint：材质、VFX、音频、动画、UI 与关卡反馈
```

C++ 负责状态不变量、事务、兼容性判断、重置契约和可测试的确定性规则；Blueprint 负责输入绑定、目标反馈、表现层以及关卡教学节奏。详细边界见 [`Docs/ARCHITECTURE.md`](Docs/ARCHITECTURE.md)。

## 当前可玩闭环（EXP-001）

`L_TestChamber` 已装配一条最小闭环：

```text
Source_Linear_001（持有 +X / 600 的 Linear Motion）
        ↓ 按 E 捕获（Capture）
Player（携带唯一 Motion State，指示灯 + 地面方向 Preview）
        ↓ 按 Q 转移（Transfer）
Receiver_Linear_001（校验方向后消费 Consume）
        ↓ 按 R 房间重置（Reset）
Source 恢复快照，Player / Receiver 清空
```

已验证（PIE，2026-08-30）：

- Capture `Source.Linear.001 → Player` 成功，Source 停止持有。
- Transfer `Player → Receiver.Linear.001` 成功并消费，Player 清空。
- 拒绝路径：`SourceEmpty` / `CarrierOccupied` / `InvalidSource` 均按契约返回且不丢失状态。
- 房间 Reset：20/20 连续循环通过，`participants=3, success=true`，无重复/丢失 Motion State。
- 入口：`/Game/Transmit/Maps/L_TestChamber`（编辑器启动图仍为模板 `Lvl_ThirdPerson`）。

## 运行项目

### 环境要求

- Unreal Engine 5.8
- Git
- Git LFS

### 获取并打开

```bash
git clone https://github.com/Furinasd/Transmit.git
cd Transmit
git lfs pull
```

随后使用 Unreal Engine 5.8 打开 `passely.uproject`。当前编辑器与游戏入口地图为：

```text
/Game/ThirdPerson/Lvl_ThirdPerson
```

`.uproject` 当前使用 `5.8` 版本关联而不是某台工作站的引擎 GUID；其他工作站仍需安装或选择兼容的 Unreal Engine 5.8。

## 验证状态

| 检查项 | 结果 | 证据边界 |
| --- | --- | --- |
| UE 5.8.1 编辑器启动 | 已观察通过 | 本地日志记录成功初始化 |
| 当前地图检查 | 0 error / 0 warning | 本地编辑器日志 |
| Blueprint 批量编译 | 0 error / 0 warning / 0 load failure | 编译本身完成；进程因本机 DDC/Zen 无可写节点返回 1 |
| EXP-001 可玩闭环（PIE） | 已通过 | Capture / Transfer / Consume 成功，E/Q/R 可用 |
| 20/20 Room Reset | 已通过 | PIE 自动化 R 键 + 状态校验（`Saved/Logs/passely.log`，2026-08-30） |
| human readability / 首次玩家理解 | 未验证 | 尚未执行首次玩家盲测（Sep 1 门禁） |
| v0.3 / Gate A Core 自动化 | 10/10 通过 | Resolver / Preview=Commit / Charger FSM、Actor-path Capture 与结构门禁等；尚未替代 PIE 验收 |
| Build / Packaging / 跨平台 | 部分验证 | Win64 Editor Development 与 macOS Editor module 已构建；打包和最终 Win64 release authority 尚未验证 |

## 开发路线

1. **EXP-001：最小直接转移闭环（已完成工程验证）**  
   Linear Source、玩家携带、Receiver 消费与房间 Reset 已在 PIE 验证；进入可读性与首次玩家理解阶段。
2. **L1：证明玩法可读性与重复意愿**  
   验证首次玩家能在 60–90 秒内完成闭环，并理解“移动的是运动，而不是物体”。
3. **L2：验证方向推理**  
   使用 camera-driven Canonical Direction reroute 验证空间推理，不扩展第二套交互语法。
4. **L3：验证威胁反转**  
   将 Charger 的冲刺同时作为威胁和高强度运动来源，验证战斗与解谜能否共用同一系统。

## Next

- **Gate A**：在 `L_TestChamber` 完成 v0.3 PIE 回归、L1/L2/L3 micro cells 与关卡内 Charger 验证后冻结 Core。
- **Gameplay Coverage**：首次玩家理解测试，以及瞄准、遮挡、不兼容和拒绝路径的人工可读性检查。

## 项目文档

建议按以下顺序阅读：

1. [`Docs/GOAL.md`](Docs/GOAL.md) — 产品目标、成功标准与非目标
2. [`Docs/DESIGN_CONTRACT.md`](Docs/DESIGN_CONTRACT.md) — 当前实现必须遵守的玩法契约
3. [`Docs/ARCHITECTURE.md`](Docs/ARCHITECTURE.md) — 当前骨架与目标系统架构
4. [`Docs/Decisions/`](Docs/Decisions/) — 状态模型与交互方式的 ADR
5. [`Docs/STATE.md`](Docs/STATE.md) — 当前里程碑、风险与下一步快照

## 当前最重要的验收门

在扩展敌人、关卡数量、Motion 类型或美术表现前，先用一个可玩的 EXP-001 回答两个问题：

1. 玩家是否能在没有文字讲解的情况下看懂 Motion State 当前属于谁？
2. “夺取并转移运动”这一动作本身是否足够清晰、可预测并值得重复？
