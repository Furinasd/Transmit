# Transmit

> 一款围绕“转移运动状态”展开的第三人称 3D 战斗解谜原型。

玩家不直接抓取或移动物体，而是从世界中的对象、敌人和机关上读取、捕获、携带、转移并转换 **Motion State（运动状态）**。同一套运动规则同时服务于战斗决策与环境谜题。

## 项目概览

| 项目 | 当前状态 |
| --- | --- |
| 当前本地工具链 | Unreal Engine 5.8（最近一次本地检查为 5.8.1；EngineAssociation 属于工作站本地差异，不是项目版本号） |
| 当前阶段 | `L_Transmit` 完整候选已整合、打包，等待 Ely 体验裁决 |
| 当前实现 | Learn → Route → Weaponize 连续主图、分步目标、局部重试、CameraCanonical / PreserveSource 与两次撞门 |
| 核心目标 | 可直接试玩、录制的完整候选；Ely 验收整体体验 |
| 版本管理 | Git + Git LFS |

当前制作线为 `Jason/L_Transmit_v01`。正式地图、共享 gameplay 与发布配置由主集成线维护；独立表现代码/资产从 `Jason/visual-presentation` 的 ready 批次集成。具体 SHA、验证边界与剩余工作见 [`Docs/STATE.md`](Docs/STATE.md)。早期 `feat/gameplay-core-v03` 是历史能力实现线。

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
├─ Source/passely/         # Motion 核心 C++：状态、事务、接口、Reset、方向策略、Carrier 与自动化测试
├─ Content/
│  ├─ Transmit/            # 正式角色、GameMode、L_Transmit、L_TestChamber 与表现资源
│  ├─ Input/               # Enhanced Input 资源
│  ├─ Characters/          # Mannequin 角色、动画与材质资源
│  └─ __ExternalActors__/  # 所属关卡的 World Partition 外部 Actor 数据（不是缓存）
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
- 入口：`/Game/Transmit/Maps/L_TestChamber`（历史测试入口；当前启动图为 `L_Transmit`）。

## 连续主关卡

```text
Learn: Source → Capture → Bridge → Cross
Route: Send → Chase → Re-capture → Re-route → Dock / Arm
Weaponize: Charger Dash → Capture High Motion → Ram → Fracture → Break → Exit
```

- 正式体验位于单张 `/Game/Transmit/Maps/L_Transmit`，`L_TestChamber` 保持回归用途。
- 普通 Linear 按 gameplay camera 解析为世界六向；High Motion 通过 `PreserveSource` 保留 Charger 已提交的 Dash 方向。Preview 与 Commit 使用同一结果。
- Directional Carrier 的 Actor 本体沿世界方向移动，用 swept collision 停止，并可被 re-capture。
- `E` 捕获，`Q` 转移；`Backspace` 重试当前区域并保留已完成进度，`R` 从开场重新开始。WASD 移动，鼠标瞄准，空格跳跃。
- 第一击让门受损，第二击解除威胁并开启出口；走入终点后显示完成状态。

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
/Game/Transmit/Maps/L_Transmit
```

`.uproject` 当前使用 `5.8` 版本关联而不是某台工作站的引擎 GUID；其他工作站仍需安装或选择兼容的 Unreal Engine 5.8。

## 验证与打包

当前源版本与历史运行证据见 [`Docs/STATE.md`](Docs/STATE.md)。2026-09-07 的 gameplay 检查点记录 27/27 自动化和连续 PIE 通关；这些记录不证明 Windows 封包或真人首玩验收。

封包命令、产物目录与平台验收见 [`Docs/PACKAGING.md`](Docs/PACKAGING.md)：

```bash
bash Scripts/package_ltransmit_mac.sh
```

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/package_ltransmit_windows.ps1 -EngineDir "C:\Program Files\Epic Games\UE_5.8"
```

Windows 脚本必须在 Windows 上执行；当前尚无本轮 Win64 EXE 构建／运行通过证据。未完成设计与普通 Motion 入 Boss 房问题见 [`提交版差额`](Docs/submission/KNOWN_GAPS.md)。

Ely 的首次完整试玩仍需确认：目标理解、Route 中继读图、Charge 捕获窗口、两次撞门差异、局部恢复、相机、节奏与视听平衡。5–7 分钟只是体验假设。

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
