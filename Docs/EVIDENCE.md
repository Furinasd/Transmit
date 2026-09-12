# Transmit / 版本与验证证据

[项目首页](../README.md) · [案例](CASE_STUDIES.md) · [双平台](CROSS_PLATFORM.md) · [封包](PACKAGING.md)

更新：2026-09-13。展示整理核对源码与已有报告 / 日志，没有重新执行 UE。每一条记录只覆盖自己的提交、产物和观察路径。

## 候选身份

| 对象 | 版本 / 来源 | 解释 |
| --- | --- | --- |
| 展示审阅源码 | `9455e05e8e7026b90c71dbc2556be74dfdeada6d` | 本次文档整理前的 main；新的文档提交不会改变旧包身份。 |
| 本地 Win64 Development | `dbd1ab84260e823d784e346f20a69c5f275dc4fe` | 候选 `20260907-134820-Win64`。 |
| Windows 实机视频 | [BV1gXYT6dEeY](https://www.bilibili.com/video/BV1gXYT6dEeY/) | 作者确认是最终提交版、Windows 打包后录制；精确 SHA / candidate ID 尚未关联，不自行认定为 dbd1ab8。 |
| 历史体验修订 | [60cbf00](https://github.com/Furinasd/Transmit/tree/60cbf000d41a578aef6896e9c03e5f34cb87dd17)及对应报告 | 27 项自动化、166.754 游戏秒连续 PIE 等，按原报告引用。 |
| 历史 Mac 候选 | `20260907-quiet-counter-Mac` | [体验报告](dev/20260907-experience-refinement.md) / [历史 STATE](STATE.md)记录封包与启动，不能替代同版本双端验收。 |

截至 2026-09-13 的远端检查，GitHub Releases 列表为空。源码入口和本地候选路径不是公开包下载链接。

## 证据覆盖

| 层级 | 已有记录 | 可以支持的结论 | 未覆盖的结论 |
| --- | --- | --- | --- |
| 原生构建 / 自动化 | 历史 Mac Editor 构建、27 项测试通过 | 被测试的规则、方向与回归条件 | 首玩体验、全路径无缺陷 |
| 连续正式地图 PIE | 166.754 game seconds，真实移动 / E/Q，无传送或资源注入 | 一条熟练已知解路线可到出口 | 首玩时长、理解、趣味 |
| 专项 fixture | 落空、离轴反击、三段检查点、全重置等 | 各自显式准备条件下的行为 | 没有准备的盲测；所有合法入场组合 |
| Map Check | 历史 0 errors / 0 warnings | 地图检查器的检查范围 | 镜头、可读性与美术验收 |
| Windows BuildCookRun | dbd1ab8，exit 0 | 当时 Build / Cook / Stage / Archive 成功 | 前台完整通关与性能 |
| Windows 离屏启动 | D3D12，正式地图进入 play，exit 0 | 对应启动与退出路径 | 音画、鼠标键盘、首玩 |
| Mac 分发 / 启动 | 历史 Stage、签名完整性、Metal SM6 启动记录 | 对应报告中的包与有限启动路径 | 公证、当前候选完整通关 |
| Mac 延迟退出 probe | 一次受控 exit 0 | 一次最小对照的缓解结果 | 原始根因关闭与长期可靠性 |

自动化数字不与 Python editor safety 的 6 项测试相加：两者检查不同对象。低层检查通过不能升级为更高层验收。

## Windows 日志摘录

本地目录为 `Saved/LTransmitCandidate/20260907-134820-Win64/`，受 Git 忽略。以下是 2026-09-12 / 13 读取的历史记录，不是本轮新运行：

| 文件 | 读取内容 |
| --- | --- |
| `source-head.txt` / `source-status.txt` | dbd1ab84260e823d784e346f20a69c5f275dc4fe / clean |
| `package.log`，861 / 863 行 | BUILD SUCCESSFUL / AutomationTool exiting with ExitCode=0 (Success) |
| `final-smoke.log`，842–847 行 | GameFeatureData 加载失败与 handled ensure |
| `final-smoke.log`，937 / 1055 行 | L_Transmit up for play / Exiting |
| `smoke-exit-code.txt` | 0 |
| `FINAL-VALIDATION.txt` | 历史记录 27 项通过、50 个包文件 SHA-256 匹配；本轮未重新计算哈希 |

本页提供必要摘录，完整原日志尚未作为公开附件分发。进程成功退出与日志存在 ensure 可以同时成立，因此不能称“零错误启动”。

## 尚待收口

1. **普通 Motion 满槽入场。**当前只有玩家反馈与拒绝分支支持，缺完整恢复验证。具体方案与验证链见 [KNOWN_GAPS](submission/KNOWN_GAPS.md)。
2. **装置身份。**SUV 与折叠屏的形态 / 演出差额仍在；代码复用不能证明玩家理解了身份关系。
3. **Windows 前台人工清单。**完整流程、两次命中、Backspace、R、中文、音频、输入与性能。
4. **macOS 完整候选验收。**对应版本的独立包通关、退出生命周期与连续稳定性。
5. **五人首玩。**理解 4/5、主动复用 3/5、路线 / 改向预测 4/5、威胁重构 3/5、中位通关 5–7 分钟是合同目标，尚无本轮可审阅的结果。
6. **视频身份。**关联视频与源 SHA；作者演示和首玩观察分别记录。

## 后续记录格式

一条新证据最少包含：**日期、源 SHA、工作树状态、平台 / 硬件 / 引擎、产物 ID、运行模式、操作路径、结果、已知限制、原始文件位置。**

新增修复影响玩法或资产时，在新版本重新验证受影响路径。文档更新只更新展示，不给历史测试或包重新贴上新源码标签。

历史报告仍保留在 [dev/](dev/)，当前入口以本页与 [CROSS_PLATFORM](CROSS_PLATFORM.md) 为准。设计规则以 [DESIGN_CONTRACT](DESIGN_CONTRACT.md)及其后续授权修订为准。
