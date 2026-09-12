# Transmit / 制作职责与来源

[项目首页](../README.md) · [设计案例](CASE_STUDIES.md)

## 项目角色

作者：**Ely / Furinasd**。作品集记录的职责为主导玩法规则、关卡递进、架构边界与体验取舍，使用 C++、Blueprint 和 Agent 辅助制作。

| 范围 | 表达边界 | 可追溯入口 |
| --- | --- | --- |
| 玩法与空间设计 | 规则、递进、改版与体验取舍 | [案例](CASE_STUDIES.md) / [玩法演化](https://app.notion.com/p/3d36dbf617ac81aeaa37c190f5ed04b6) |
| C++ / Blueprint 与制作工具 | 包含 Agent 辅助实现与批量制作，不能由 Git 作者字段推定每行代码为纯手写 | [Source](../Source/passely/) / [制作复盘](https://app.notion.com/p/3d36dbf617ac819c814cc87822241d11) |
| 验证与交付 | 自动化、脚本与人工观察分别标记 | [证据](EVIDENCE.md) / [双平台](CROSS_PLATFORM.md) |
| 表现与声音 | 项目包含专属表现代码；音效合成过程见原始说明 | [Presentation](../Source/passely/Private/Presentation/) / [8 类合成音效](https://app.notion.com/p/2ad6dbf617ac83528cc1012f7e198171) |

## 模板、素材与参考

项目使用 Unreal Engine 及保留的 Manny / Quinn 等引擎模板资源。场景标牌、表现内容、设计参考与素材归属沿 [References & Attribution](https://app.notion.com/p/2b16dbf617ac82f5889d014cb1fbbc6b) 保存。

本页是职责与来源导航，不为全部内容统一授予再分发许可。独立分发素材或包之前，仍需按具体来源核对适用条款；没有将第三方内容笼统标记为个人原创。

## 如何阅读个人贡献

优先看作者如何定义问题、取舍方案、判断观察结果，以及是否能解释并维护对应实现。一次工具生成、一次提交或一次绿灯，都不单独证明设计 ownership 或生产效率。

项目没有可支持“效率提升 X%”的对照工时，也没有把个人项目中的 Agent 协作当作真实团队管理规模的证据。
