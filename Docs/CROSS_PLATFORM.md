# Windows / macOS 开发与兼容证据

2026-09-12 审阅；源码基线 `9455e05e8e7026b90c71dbc2556be74dfdeada6d`。本次为源码、历史报告与本地日志核对，没有重新执行 UE 或 Mac 命令。

Transmit 使用同一个 `passely` Runtime 模块、Motion 交互契约与 `/Game/Transmit/Maps/L_Transmit`。双平台工作的重点是处理编译器、Editor 工具进程和交付产物的差异，并保留各自的验证证据。

## 平台证据矩阵

| 层级 | Windows | macOS |
| --- | --- | --- |
| 封包入口 | [PowerShell](../Scripts/package_ltransmit_windows.ps1)：Build / Cook / Stage / Archive | [Bash](../Scripts/package_ltransmit_mac.sh)：Build / Cook / Stage，完整复制 staged app |
| 历史构建 / 封包 | 本轮读取 `dbd1ab8` 候选日志：BuildCookRun exit 0 | [历史体验报告](dev/20260907-experience-refinement.md)与 [STATE](STATE.md)记录 Mac Build/Cook/Stage；本轮未在 Mac 重跑 |
| 产物检查 | 源版本前后检查、完整 EXE / Pak / IoStore、SHA-256 清单；历史记录 50 个文件哈希匹配，本轮未重新计算 | 检查 staged executable / Paks，`ditto` 保留完整 app，`codesign --verify --deep --strict`；签名完整性不等于公证 |
| 运行 | 本地日志确认 D3D12 离屏独立启动进入 L_Transmit，退出码 0；作者确认最终提交演示来自 Windows 打包后录制 | 历史报告记录 Metal SM6 原生启动和部分输入观察；不等于当前候选完整通关 |
| 尚存边界 | GameFeatureData handled ensure；完整前台人工验收清单、首玩及性能待收口 | 原始退出崩溃根因、连续稳定退出、对应候选的完整人工通关与性能待收口 |

这些记录来自不同检查点，不构成同一 SHA 在双端完整验收的结论。[最终提交演示](https://www.bilibili.com/video/BV1gXYT6dEeY/)的 Windows 独立包录制环境由作者确认；视频对应的精确 SHA / candidate ID 尚未关联。

## 三项兼容处置

### 1. MSVC 窄化转换：显式转换，保留共享实现

[`4d9184d`](https://github.com/Furinasd/Transmit/commit/4d9184d7cff9e91fd2c62cb59a049ccc32fb1d6e) 将 `LegacyArrows.Add({Arrow, Arrow->bHiddenInGame})` 改为 `LegacyArrows.Add({Arrow, Arrow->bHiddenInGame != 0})`，明确表达位字段到 `bool` 的转换。

这是编译器兼容问题的窄范围修正，没有引入两套表现逻辑。本轮核对了提交 diff；不把这一处改动扩大为完整跨平台编译保证。

### 2. Editor 依赖与 Cooker 进程隔离

[`passely.uproject`](../passely.uproject) 将 `ModelContextProtocol` / `AllToolsets` 限定到 Editor target；[Runtime Build.cs](../Source/passely/passely.Build.cs) 没有 UnrealEd / MCP 模块依赖。

Windows 封包通过 Cooker 启动参数关闭 MCP 自动监听，避免与交互 Editor 的监听端口冲突；不修改用户全局 Editor Preferences。GameFeatureData 的启动 ensure 仍然存在，因此依赖隔离不能被表述为所有启动配置问题均已解决。

### 3. Mac 自动化退出：以进程生命周期为验证边界

[事故记录](dev/20260907-mac-editor-crashes.md)将 Editor teardown 的 invalid free 与 CrashReportClient 的二次崩溃分开。项目侧 [editor safety](../Scripts/Editor/transmit_editor_safety.py) 管理自有 callback / keep-alive，换图前拒绝 PIE 和脏资源，避免重复加载同一地图；预览地图使用 `new_level_from_template`。

立即 `quit_editor()` 的对照仍 exit 1；移除立即退出，让 `-ExecutePythonScript` executor 在下一 tick 销毁通知并延迟退出后，一次受控 probe exit 0。这里能主张的是项目侧防御性缓解与最小对照，不能主张彻底修复引擎根因或已证明长期稳定。

这项案例的重点是：测试成功标记、Editor 回调完成与进程成功退出，必须分别观察。

## 本次读取的 Windows 原始证据

本地候选目录：`Saved/LTransmitCandidate/20260907-134820-Win64/`。目录受 `.gitignore` 排除，不是公开下载入口。

- `source-head.txt`：`dbd1ab84260e823d784e346f20a69c5f275dc4fe`；`source-status.txt`：`clean`。
- `package.log` 第 861 / 863 行：`BUILD SUCCESSFUL` / `AutomationTool exiting with ExitCode=0 (Success)`。
- `final-smoke.log` 第 842–847 行：GameFeatureData 加载警告及 ensure；第 937 行：L_Transmit up for play；第 1055 行：Exiting。
- `smoke-exit-code.txt`：`0`。
- `FINAL-VALIDATION.txt`：历史 27 项自动化通过、50 项包文件哈希匹配；仍要求人工 Learn → Route → Boss → Exit、两次命中、Backspace / R 与普通 Motion 满槽入场验证。

当前基线相对包源 `dbd1ab8` 只修改 README，但包仍必须标记真实来源 SHA。本次摘录可供审阅；未上传完整原日志、没有重复运行上述验证，也没有创建 Release。

## 下一步验收与交付

1. 关联最终提交视频的准确 source SHA / candidate ID；保留 Windows 独立包录制说明。
2. 在对应 Windows 候选中完成前台人工清单，定位 GameFeatureData ensure，并单独验证满槽入场的已批准恢复方案。
3. 对齐两套封包脚本的证据格式。Mac 当前在封包后记录源状态，没有 Windows 同等强度的构建前后 clean / SHA 校验；补齐后必须在 Mac 实机执行，不能只凭静态检查宣布一致。
4. 在有 Mac 实机时按对应源版本补完整通关与生命周期复测；使用固定硬件、画质、RHI、路线记录两端性能。原始退出崩溃仍沿事故记录中的待验门槛推进。

项目原则：**同一套玩法，分平台处理编译、Editor 工具生命周期和分发；每个平台的成功与未验项都能追溯到具体证据。** 完整版本索引见 [证据账本](EVIDENCE.md)，返回 [项目首页](../README.md)。
