# Transmit 封包与验收

正式入口：`/Game/Transmit/Maps/L_Transmit`。工程名保持 `passely`；Windows 启动文件为 `passely.exe`。完整分发整个打包目录，不能单独发送 EXE。

## Windows / Win64

需要 Windows 主机、UE 5.8、与该引擎匹配的 Visual Studio C++ 工具链及 Windows SDK、Git LFS。已有指定提交的 Win64 封包记录，见 [证据账本](EVIDENCE.md)；每个候选仍需按下面的步骤独立验收。

在干净的目标提交上运行 Windows PowerShell：

```powershell
git lfs pull
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/package_ltransmit_windows.ps1 -EngineDir "C:\Program Files\Epic Games\UE_5.8"
```

默认 Development，便于保留诊断日志；最终需要 Shipping 时附加 `-Configuration Shipping`，并对该产物重新验收。也可用 `TRANSMIT_ENGINE_DIR` 指定引擎目录，或用 `-OutputDir` 指定一个不存在的新候选目录。

脚本运行 Build → Cook → Stage → Archive，启用 Pak / IoStore，收集 prerequisites，检查启动 EXE、游戏 EXE 和 `.pak/.utoc/.ucas`，记录 SHA、UAT 参数、日志和文件 SHA-256。失败立即停止，保留现场，不覆盖旧候选。提交 SHA 与工作区状态在前后核对。

输出：`Saved/LTransmitCandidate/<timestamp>-Win64/Package/Windows/passely.exe`。交付 `Windows` 整个文件夹，连同候选根目录的 `PLAYTEST.txt`。`Stage` 是中间产物，不是另一个交付版本。`package.log` 成功和文件存在只证明封包检查通过。

Windows 人工验收：独立启动后进入正式地图；检查中文字体、材质、音效、鼠标键盘；实际完成 Learn → Route → Boss → 出口，检查两次命中、Backspace 局部重试及 R 全重置。另行复测普通 Motion 入场的已知问题。记录 Windows／GPU／引擎版本、运行日志、失败点与源 SHA。未完成这些检查时，不能标记 Windows 交付已验收。

## macOS

```bash
bash Scripts/package_ltransmit_mac.sh
```

默认 UE 5.8；可用 `TRANSMIT_ENGINE_DIR` 覆盖。本地 Development 候选在 `Saved/LTransmitCandidate/<timestamp>/Transmit.app`。脚本复制完整 staged app 并检查签名；未公证，完整独立包通关仍需实际观察。

## 仓库交付边界

- `Source/`、`Config/`、`Content/` 和工程文件是重建输入；Unreal 资产通过 Git LFS 传输。
- `Scripts/` 保存封包与编辑器工具；`Docs/dev/` 保存历史证据，`Docs/design/` 保存纸面方案，`Docs/submission/` 保存提交差额。保留设计推演，不以整理为由删除。
- `Saved/`、`Binaries/`、`Intermediate/`、包和本地报告保持忽略，不加入 Git。候选目录保存日志和源版本供追溯。
- MCP 和 AllToolsets 仅对 Editor 目标启用，游戏不依赖编辑器自动化插件。
- 已知未完成项见 [提交版差额](submission/KNOWN_GAPS.md)。PR #9 已合并；合并状态不代替候选验收。后续发布包应附真实源版本、完整运行清单及未验项。
