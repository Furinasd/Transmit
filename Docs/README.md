# Transmit / 阅读导航

[回到项目首页](../README.md) · [Notion 作品集](https://app.notion.com/p/e916dbf617ac8213bf0b01b2bde83939)

## 先选一条阅读路径

| 目的 | 建议顺序 |
| --- | --- |
| 看懂作品 | [视频](https://www.bilibili.com/video/BV1gXYT6dEeY/) → [项目首页](../README.md) → [设计与工程案例](CASE_STUDIES.md) |
| 深入技术 | [案例](CASE_STUDIES.md) → 下方源码入口 → [架构](ARCHITECTURE.md) → [证据](EVIDENCE.md) |
| 重建与运行 | [封包说明](PACKAGING.md) → [双平台边界](CROSS_PLATFORM.md) → [已知差额](submission/KNOWN_GAPS.md) |
| 理解设计演化 | [目标](GOAL.md) → [设计契约及后续修订](DESIGN_CONTRACT.md) → [决策记录](Decisions/) → [Notion](https://app.notion.com/p/e916dbf617ac8213bf0b01b2bde83939) |

## 从一个问题打开源码

| 问题 | 首读符号 | 实现入口 |
| --- | --- | --- |
| 状态何时提交，通知何时发生？ | `TryMoveBetween` / `FlushNotifications` | [MotionTransferComponent](../Source/passely/Private/Motion/MotionTransferComponent.cpp) |
| 上一帧目标还能直接提交吗？ | `RequestTransfer` / `RefreshTarget` | [MotionInteractorComponent](../Source/passely/Private/Motion/MotionInteractorComponent.cpp) |
| 普通方向怎样量化、稳定边界？ | `ResolveDirection` | [MotionCanonicalDirectionResolver](../Source/passely/Private/Motion/MotionCanonicalDirectionResolver.cpp) |
| 输入方向与设备用途在哪里分开？ | `GetCounterDirection` / `ResolveStrike` | [TransmitLevelActors](../Source/passely/Private/Transmit/TransmitLevelActors.cpp) |
| Carrier 撞墙、被再次截获会怎样？ | Carrier movement / capture callbacks | [TransmitDirectionalCarrierActor](../Source/passely/Private/Motion/TransmitDirectionalCarrierActor.cpp) |
| 全重置与局部重试有什么区别？ | `RequestLocalRetry` / room reset | [LevelActors](../Source/passely/Private/Transmit/TransmitLevelActors.cpp) / [RoomResetController](../Source/passely/Private/Motion/MotionRoomResetController.cpp) |
| 回调中再次交接如何验证？ | `FMotionNotificationReentrancyTest` | [MotionTransferTests](../Source/passely/Private/Tests/MotionTransferTests.cpp) |

## 文件职责

- [EVIDENCE](EVIDENCE.md)：版本与验证范围的展示入口。
- [CROSS_PLATFORM](CROSS_PLATFORM.md)：Windows / macOS 的兼容案例、封包差异与平台证据。
- [CREDITS](CREDITS.md)：制作职责、Agent 辅助与素材来源入口。
- [ARCHITECTURE](ARCHITECTURE.md)：职责与数据流；早期 Proposed 段落结合后续已实现修订阅读。
- [DESIGN_CONTRACT](DESIGN_CONTRACT.md)：授权规则；日期不同的修订不得混作同一快照。
- [STATE](STATE.md)：操作性交接及历史状态，不作为独立设计授权。
- [dev/](dev/)：带日期的实验、故障与运行报告。
- [design/](design/)：纸面扩展，包含 L4/L5 方案。
- [submission/](submission/)：提交时的资产清单与已知差额。
- [Handoff/](Handoff/)：A/B 制作交接历史，保留当时上下文。

## 脚本阅读

| 类别 | 入口示例 | 使用前提 |
| --- | --- | --- |
| 平台封包 | [Windows](../Scripts/package_ltransmit_windows.ps1) / [Mac](../Scripts/package_ltransmit_mac.sh) | 对应平台、匹配工具链、明确源版本；不覆盖旧候选。 |
| Editor 生命周期 | [session](../Scripts/Editor/transmit_editor_session.py) / [safety](../Scripts/Editor/transmit_editor_safety.py) | 管理自己拥有的回调；切图 / 退出前检查 PIE 与脏资源。 |
| 正式图验证 | [experience run](../Scripts/Editor/validate_transmit_experience_run.py) / [checkpoints](../Scripts/Editor/validate_transmit_experience_checkpoints.py) | 阅读对应历史报告，区分连续路径与有准备的 fixture。 |
| 一次性内容制作 | [author experience](../Scripts/Editor/author_transmit_experience.py)等 author / build / integrate 脚本 | 历史 authoring 工具，不是打开项目必须执行的安装步骤；重跑可能写入资产。 |

Unreal 的 `.uasset`、`.umap` 与 ExternalActors 是源资产。目录整理不通过文件系统移动它们；`Saved/`、`Intermediate/` 等生成内容保持本地。
