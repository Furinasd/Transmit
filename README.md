# Transmit

> 一款围绕“转移运动状态”展开的第三人称 3D 战斗解谜原型。

玩家不直接抓取或移动物体，而是从世界中的对象、敌人和机关上读取、捕获、携带、转移并转换 **Motion State（运动状态）**。同一套运动规则同时服务于战斗决策与环境谜题。

## 项目概览

| 项目 | 当前状态 |
| --- | --- |
| 当前本地工具链 | Unreal Engine 5.8（最近一次本地检查为 5.8.1；EngineAssociation 属于工作站本地差异，不是项目版本号） |
| 当前阶段 | Final v0.4 design lock 已提升（docs-only）；runtime 窄差异未实现 |
| 当前实现 | EXP-001 闭环 + v0.3 Canonical Direction Resolver / Preview=Commit / Charger Core（checkpoint `1fb96ea`）；Directional Carrier 与 Boss direction lock 为已提升未实现 |
| 核心目标 | 先完成 v0.4 窄差异（Directional Carrier / Re-capture、Boss High Motion direction lock），再进入唯一正式图 `L_Transmit`（Learn → Route → Weaponize） |
| 版本管理 | Git + Git LFS |

> [!IMPORTANT]
> **Final v0.4 design lock promoted；runtime closure pending**：普通 Linear 继续使用 CameraCanonical 六向 reroute；Directional Carrier（world-space movement / swept blocking stop / re-capture / Reset）与 Boss High Motion direction lock（保留 Dash 世界方向、bypass CameraCanonical、Preview = Commit）已提升到契约但**尚未实现**。`L_Transmit` 是唯一正式 playable（未来内容），`L_TestChamber` 只做回归验证。

## 仓库分支

| 分支 | 用途与状态 |
| --- | --- |
| `main` | 稳定基线：已合入并关闭的 EXP-001 工程验证 |
| `feat/gameplay-core-v03` | 当前 gameplay 开发线；v0.3 core checkpoint（`1fb96ea`）与 Final v0.4 文档提升 |
| `BP_LD_BeatMarker` | 关卡设计 BeatMarker 工具线；已推送并保存在远端，作为工具记录独立保留 |

日常 gameplay 工作在 `feat/gameplay-core-v03` 进行。工具或实验内容需要单独保存时，请直接推送独立命名的远端分支，不要再派生 `-local` 副本，避免本地与远端分支漂移。

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
- **方向语义**：Capture / Carry 保留 Source Motion；普通 Linear Transfer 由 CameraCanonical 确定性量化到六个 Canonical Direction；Boss High Motion 是显式例外，保留 Charger Dash 世界方向（bypass camera reroute）。Preview 与 Commit 始终共用同一方向结果。
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

## 当前已实现最小闭环（EXP-001 baseline）

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

## Final v0.4 目标闭环（已提升，未实现）

```text
Source → Capture → Directional Carrier → geometry-constrained motion
        → Re-capture → Ram Rail → Boss Dash → Capture High Motion
        → Ram Impact → Gate Break
```

- 正式 Demo 为单张 `L_Transmit`：Zone 1 Learn → Zone 2 Route → Zone 3 Weaponize；不再生产三张独立关卡。
- Directional Carrier 需把普通 Linear 六向输出变成 Actor 本体 world-space 位移，swept collision 撞停后可再次 Capture，Reset 恢复。
- Boss High Motion 需锁定 committed Dash vector，Transfer 不被 camera 改写，Preview 与 Commit 一致。
- 上述能力与 `L_Transmit` 均尚无仓库 runtime / content 证据。

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
| EXP-001 可玩闭环（PIE） | 历史通过 | Capture / Transfer / Consume 成功，E/Q/R 可用（2026-08-30，旧 baseline） |
| 20/20 Room Reset | 历史通过 | PIE 自动化 R 键 + 状态校验（`Saved/Logs/passely.log`，2026-08-30） |
| v0.3 / Gate A Core 自动化 | 历史 10/10 通过 | Resolver / Preview=Commit / Charger FSM、Actor-path Capture 与结构门禁等；当前 HEAD 需 fresh rerun |
| Directional Carrier（v0.4） | 未实现 | 无 actor / movement / collision / re-capture / Reset 证据 |
| Boss High Motion direction lock（v0.4） | 未实现 | 无 policy seam；Capture 后仍走 camera reroute |
| `L_Transmit` 单图（v0.4） | 未来内容 | 地图不存在 |
| human readability / 首次玩家理解 | 未验证 | 尚未执行首次玩家盲测 |
| Build / Packaging / 跨平台 | 部分验证 | Win64 Editor Development 与 macOS Editor module 已构建；打包和最终 Win64 release authority 尚未验证 |

## 开发路线（Final v0.4）

1. **v0.4 runtime 窄差异（尚未开始）**：修复 Actor-path Preview 方向透传；在 Motion 上增加显式 direction policy；实现 Directional Carrier；Boss Dash Capture 授予 direction-locked High Motion。
2. **`L_TestChamber` 验证矩阵**：普通六向回归、Carrier movement / collision stop / re-capture / Reset、Boss direction-lock Preview = Commit、多轮 Reset。
3. **单张 `L_Transmit`**：Zone 1 Learn（Bridge Slab / traversal change）→ Zone 2 Route（Carrier relay / re-capture / Arm Ram）→ Zone 3 Weaponize（Boss High Motion → Ram → two-hit Gate Break）。
4. **Presentation / 验收**：lighting / composition / route readability / camera / juice，随后首次玩家 human playtest。

L1 / L2 / L3 只作为单图内的 progression ID，不再对应三张独立 `.umap`。

## Next

- **Runtime closure（独立后续 commit）**：Directional Carrier / Re-capture 与 Boss High Motion direction lock 的 P0 窄差异，以及 Actor-path Preview 一致性修复。
- **Production**：runtime gate 通过后创建唯一正式图 `L_Transmit`；`L_TestChamber` 保持回归用途。
- **Gameplay Coverage**：首次玩家理解测试，以及瞄准、遮挡、不兼容和拒绝路径的人工可读性检查。

## 项目文档

建议按以下顺序阅读：

1. [`Docs/GOAL.md`](Docs/GOAL.md) — 产品目标、成功标准与非目标
2. [`Docs/DESIGN_CONTRACT.md`](Docs/DESIGN_CONTRACT.md) — 当前实现必须遵守的玩法契约
3. [`Docs/ARCHITECTURE.md`](Docs/ARCHITECTURE.md) — 当前骨架与目标系统架构
4. [`Docs/Decisions/`](Docs/Decisions/) — 状态模型与交互方式的 ADR
5. [`Docs/STATE.md`](Docs/STATE.md) — 当前里程碑、风险与下一步快照

## 当前最重要的验收门

在扩展敌人、关卡数量、Motion 类型或美术表现前，先回答：

1. Zone 1 玩家能否在没有文字讲解的情况下看懂 Motion State 当前属于谁？
2. Zone 2 的“发送 → 追赶 → Re-capture → 再布线”是否清晰、可预测并值得重复？
3. Zone 3 的 Boss Dash 截获 → direction-locked High Motion → Ram → Gate Break 是否成立同一个系统语言？
