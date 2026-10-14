# 改进双语界面并增加带检查的脚本及电台安装功能

原界面主要依赖触屏，间距、文字大小及焦点不统一。本次保留五个页签和深色/蓝色
设计，完善手柄导航，并为英文、中文标签及弹窗使用统一布局。两个摇杆和方向键
都可导航，Minus 选择操作按钮。触屏支持翻页和数值调整，长按不会重复触发。

工具页新增自定义电台 NX v1.1 安装及配套电台/时间元数据导入。安装包检查路径
和哈希，先创建临时副本，保留备份，支持还原及中断恢复。脚本写入前检查 raw/RSC7
结构边界和内嵌名称，并修复嵌套脚本档案重定位、目录计数及写入错误处理。

## 审查和构建

源码 ZIP 包含完整可构建目录及未改动的原有 romfs 资源。
`contribution.patch` 合并了相对于原始下载快照的全部修改；
`baseline-manifest.json` 标识该快照。若上游已更新，请先审查并移植补丁。
尚未发布上游分支或 PR。

```sh
git apply --check --ignore-whitespace contribution.patch
git apply --ignore-whitespace contribution.patch
make
```

需要 devkitPro/devkitA64、libnx、switch-zlib、switch-freetype 和 switch-mbedtls。
Windows 可使用 `tools/build.ps1 -DevkitPro C:/devkitPro`。
后端约定及导出命令见 `docs/CONTRIBUTOR-HANDOFF.md`；
渲染器及本地预览检查见 `docs/UI-REFINEMENT.md`。

## 验证和限制

用户于 2026-10-14 确认 Switch Joy-Con 及触屏界面测试通过。
发布 NRO 与 UI Test 1 为同一文件。本地通过：623 项界面核心、35 项控制器、
30 项脚本、25 项安装包、15 项导出器检查，以及 48 张双语界面预览。
真实原始自定义电台档案通过安装、重复安装及逐字节还原检查。
合并补丁可应用并准确还原所有源码文件。

新增安装包的安装、还原及中断恢复仍需 Switch 验收。
原有存档/DLC/脚本写入继续使用原地修改方式。
NSC 结构检查不证明 native 兼容性；电台导入需要电脑生成的配套输出，
不在主机上编码音乐。未修改调优值、内置脚本、存档、密钥及官方应用版本元数据。

工具箱：niceboat0。贡献者：MEGATARD，集成经验来自 MG-Menu SDK、
GTA Radio Editor 和自定义电台 NX。保留原有署名。
