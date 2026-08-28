# Transmit

> 一款围绕“转移运动状态”展开的第三人称 3D 战斗解谜原型。

玩家不直接抓取或移动物体，而是从世界中的对象、敌人和机关上读取、捕获、携带、转移并转换 **Motion State（运动状态）**。同一套运动规则同时服务于战斗决策与环境谜题。

## 项目概览

| 项目 | 当前状态 |
| --- | --- |
| 引擎 | Unreal Engine 5.8（最近一次本地检查为 5.8.1） |
| 当前阶段 | M0：玩法契约、架构边界与 EXP-001 准备 |
| 当前实现 | UE 第三人称 Blueprint 模板、输入/角色/关卡原型资源、设计与架构文档 |
| 核心目标 | 验证最小 `Source → Player → Receiver` 运动转移闭环 |
| 版本管理 | Git + Git LFS |

> [!IMPORTANT]
> 当前仓库尚未实现 Motion State、Capture、Carry 或 Transfer 的可玩运行时链路。本文中的核心系统架构与关卡规划属于已定义的目标边界，不代表玩法已经完成。

## 核心玩法

```text
读取 Read
   ↓
捕获 Capture：从运动中的 Source 取走 Motion State
   ↓
携带 Carry：玩家临时持有唯一一份 Motion State
   ↓
转移 Transfer：将状态原样交给兼容的 Target
   ↓
转换 Convert：由明确的环境规则改变方向或运动类型
   ↓
触发 Function：驱动机关、位移、碰撞或战斗结果
```

核心设计约束：

- **唯一所有权**：一份 Motion State 同一时间只属于 Source、Player 或 Target 中的一方。
- **方向保留**：玩家瞄准只负责选中目标，不能凭空改写运动方向。
- **战斗与谜题共用语言**：敌人、机关和环境都遵循同一套运动状态规则。
- **可读、可恢复**：合法目标、拒绝原因和状态归属必须可感知；关键资源可通过房间重置恢复。

## 当前仓库内容

```text
Transmit/
├─ Config/                 # 项目默认配置、输入与启动地图
├─ Content/
│  ├─ ThirdPerson/         # 当前角色、GameMode 与入口关卡
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

当前工程仍是 Blueprint 模板骨架。M0 计划采用“C++ 核心规则 + Blueprint 表现与关卡装配”的边界：

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

`.uproject` 当前使用 GUID 关联本机引擎；在其他工作站首次打开时，可能需要重新选择 Unreal Engine 5.8。

## 验证状态

| 检查项 | 结果 | 证据边界 |
| --- | --- | --- |
| UE 5.8.1 编辑器启动 | 已观察通过 | 本地日志记录成功初始化 |
| 当前地图检查 | 0 error / 0 warning | 本地编辑器日志 |
| Blueprint 批量编译 | 0 error / 0 warning / 0 load failure | 编译本身完成；进程因本机 DDC/Zen 无可写节点返回 1 |
| Motion Transfer 可玩闭环 | 未实现 | 当前没有自定义 C++ 或 Transfer 运行时链路 |
| PIE 玩法验收 | 未验证 | 尚无 EXP-001 可玩实现 |
| Build / Packaging / 跨平台 | 未验证 | 尚未执行干净构建与打包门禁 |

## 开发路线

1. **EXP-001：最小直接转移闭环**  
   实现一个 Linear Source、玩家携带状态、一个 Receiver、明确的 Capture/Transfer 反馈以及房间 Reset。
2. **L1：证明玩法可读性与重复意愿**  
   验证首次玩家能在 60–90 秒内完成闭环，并理解“移动的是运动，而不是物体”。
3. **L2：验证方向推理**  
   仅在 L1 通过后加入确定性的 Redirect/Relay，不扩展第二套交互语法。
4. **L3：验证威胁反转**  
   将 Charger 的冲刺同时作为威胁和高强度运动来源，验证战斗与解谜能否共用同一系统。

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
