# Agent Handoff

> Rebuildable and non-authoritative. Durable product rules belong in `DESIGN_CONTRACT.md`; durable technical decisions belong in `ARCHITECTURE.md`, ADRs, and Git history.

Last inspected: 2026-09-05 (Final v0.4 design lock promotion; docs only — runtime and binary assets unchanged)

## Current Truth

- **Final v0.4 design lock 已接受并提升到仓库文档**：来源为 Notion `Goal 09｜Final Design Lock`、`02｜Gameplay Lab` 与 `07｜Production Workflow`（2026-09-05 锁定）。本次 commit 只更新 `DESIGN_CONTRACT.md`、`ARCHITECTURE.md`、`ADR-003`、`STATE.md`、`README.md`，并在旧 Gate A micro-cell 文档上加 superseded banner。
- **分支 / HEAD**：`feat/gameplay-core-v03` @ `1fb96ea`（`feat: stabilize motion preview and HUD feedback`，checkpoint clean）。
- **旧 Gate A / Gate B 排期已取代**：剩余 P0 runtime closure 只收窄为 Directional Carrier / Re-capture 与 Boss High Motion direction lock；随后直接进入单张 `L_Transmit` 生产。
- 本次 commit 未修改任何 runtime code 或 binary asset；`git diff --check` 通过。

## Capability Status

### 1. Already implemented / verified（已有实现与验证）

- Ordinary Linear CameraCanonical：六向 resolver、pitch/sector hysteresis、Preview = Commit（component path）；自动化已验证。
- Charger `Telegraph → Dash → Recovery`、swept dash、capture window、blocking-hit Recovery：代码已实现；`L_TestChamber` 内放置与 PIE 证据仍待补。
- `IMotionTransferable::Call*` Actor-path dispatch（native-only C++ 可达且保留 Blueprint override）。
- EXP-001 L1 最小闭环与 20/20 Reset：历史 PIE 证据（2026-08-30）。
- macOS `passelyEditor` build + 10/10 `Transmit.MotionTransfer` automation：历史日志；当前 HEAD `1fb96ea` 的 source-aligned fresh rerun 尚未记录。
- `L_TestChamber` baseline（`d330aae` 提交）：`Source_Linear_001`、Forward `Receiver_Linear_001`、Up `Receiver_Up_L2`、`RoomReset_EXP001`。

### 2. Pre-existing committed work（checkpoint `1fb96ea`，不属于 v0.4 delta）

- `TransmitHUD` + `MotionDirectionIndicatorComponent` 表现重构；
- `TransmitMotionEndpointActor` 表现改动；
- canonical resolver camera-pitch Up/Down 更新与对应测试；
- `BP_TransmitGameMode.uasset` 更新。

以上内容保留为独立 checkpoint；v0.4 runtime 工作不得混入这些已有改动。

### 3. v0.4 promoted but not implemented（已提升、未实现）

- **Directional Carrier**：不存在。需要接收普通 Linear 六向输出 → Actor 本体 world-space 移动 → deterministic swept collision → blocking stop → moving Carrier 可 re-capture → Reset 恢复 transform/state。
- **Direction-policy seam / Boss High Motion direction lock**：不存在。当前 Capture 后 Dash Motion 在 Transfer 时仍进普通 camera resolver；需要 PreserveSource（保留 committed Dash world direction）、bypass CameraCanonical、Preview = Commit 使用同一世界方向。
- **Ram Block / Zone 3 内容角色**：无资产、无 fixture。
- **Actor-path Preview 一致性缺陷**（pre-existing，破坏 frozen Preview = Commit）：默认 `IMotionTransferable` Preview 丢弃 `DirectionResolution`，commit 组件层却用它复核；`RequiredCanonicalDirection` Receiver 可能“预览可接收、提交被拒”。现有测试绕过 Actor path，未覆盖。
- **L2 / L3 micro fixtures 与 fresh validation**：build / automation / Blueprint / Map Check / PIE 均需在 v0.4 实现后重新产生。

### 4. Future content（未来内容）

- **`L_Transmit`**：唯一正式 playable 地图，尚不存在。单图连续 Zone 1 Learn → Zone 2 Route → Zone 3 Weaponize；L1 / L2 / L3 只是 progression ID，不再做三张独立生产图。
- Bridge Slab / Ram Rail / Ram / Breakable Gate / Boss encounter 等正式 content。
- lighting / composition / route readability / camera / juice / presentation pass。
- 首次玩家 human playtest（readability、timing、Preview trust、comprehension）。

## Verified（本次 commit）

- 修改范围：6 个 markdown 文档；无 runtime code / binary asset 变化。
- `git diff --check` clean；`git diff --stat` 只覆盖这六个 markdown 文件。

## Historical Evidence（保留追溯；旧结论不代表当前实现状态）

- 5 Blueprints compile OK；Map Check 0/0；`EXP001_VALIDATE SUCCESS`（`Saved/Logs/Validate-EXP001-Assets2.log`）。
- PIE Capture/Transfer/Consume 成功；`SourceEmpty` / `CarrierOccupied` / `InvalidSource` 拒绝正确；20/20 Reset（`Saved/Logs/passely.log`，2026-08-30）。
- v0.3 Core：8/8 automation（`Saved/Logs/Automation-MotionTransfer-v03.log`）；Gate A code support：10/10 automation（`/private/tmp/Transmit-GateA-Automation.log`）。
- 09-04 回退前旧已编译模块 13/13 证据与当前源码不对齐，不作为当前证据。
- 09-02 autosave 隔离在 `Saved/MCPQuarantine/20260902-gatea-prewrite/`；不得 broad cleanup。

## Next（runtime 从后续 commit 开始，不在本次）

1. `fix`：Actor-path Preview 透传 `DirectionResolution` + Actor-path regression test。
2. `feat`：在 Motion state/context 增加显式 direction policy；Charger Dash Capture 授予 PreserveSource High Motion；Transfer bypass CameraCanonical；Preview = Commit 测试。
3. `feat`：新增 concrete Directional Carrier actor（world-space movement、swept blocking stop、re-capture、Reset）+ 测试。
4. `content/level`：在 `L_TestChamber` 加 v0.4 fixtures，跑 L1 regression、Carrier matrix、Boss lock matrix、Reset cycles。
5. `level`：fresh evidence 通过后创建单张 `L_Transmit` graybox（Learn → Route → Weaponize）。

## Not Verified / Risks

- Directional Carrier 与 Boss High Motion direction lock 均无运行时证据；不能按已实现描述。
- `L_Transmit` 不存在；不得把设计状态写成实现状态。
- Actor-path Preview 缺陷可能导致 Required Direction 预览与提交不一致。
- 当前 HEAD 的 fresh build / automation / PIE 未重新记录。
- Packaging 与最终 Win64 release authority 属于后续 RC / Release validation，不是本次 Gate。
