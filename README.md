# GTA5-Switch-Toolbox

GTA V Switch 移植版（Title ID `0100b00b51230000`）的多功能工具箱 NRO ——
DLC 管理、脚本 MOD 安装/还原、画质预设、内置模组，一个 .nro 全搞定。


中文 | [English](README.en.md)

## 界面改进

五个页签和弹窗使用统一布局。方向键或任一摇杆浏览，A 执行，B 返回，L/R 换页。
按 Minus (-) 选择底部操作栏，再用左右选择按钮、A 执行，B 返回列表。
画质及性能页使用左右调整选中设置。工具页新增手柄可选网格，继续支持触屏。
**Switch Joy-Con 和触屏界面测试已通过。**

## 自定义电台和电台导入

保留原有五个页签和操作方式。在「工具信息」页按 **X** 打开自定义电台 NX，
按 **Y** 打开电台导入；新增卡片也支持触屏。
将准备好的安装包文件夹放到 `/switch/gta5save/packages/`。
选中安装包后，**A** 安装、**X** 还原备份、**Y** 恢复中断的更新、**B** 返回。
修改游戏文件前请先关闭 GTA。

自定义电台安装包会写入所需的档案补丁和运行模块，无需下载完整的 `update.rpf`。
音乐放在游戏的 `romfs/User Music` 文件夹中。
此包适用于游戏构建 `6e3e9b3af5746e74b615bd981080c9271bc226620`。

电台导入会同时安装配套的电台 RPF 和时间元数据 RPF。
请先在电脑上的 GTA Radio Editor 中准备这两个输出；本 NRO 不负责将音乐编码为 AWC。
遇到冲突修改会停止安装。原文件保存在 `/switch/gta5save/packages/.backups/`；
SD 卡需有足够空间存放受影响档案的临时副本。只能还原本安装器建立的备份。

脚本安装前新增 payload 边界与内嵌名称检查；结构检查通过并不代表 native 兼容或实机验证。
**新增安装包的安装、还原及恢复仍待 Switch 实机验收。**

## ⚠️ 使用前必读：备份 update.rpf 和 update2.rpf

**强烈建议先备份**，任何 RPF 写入操作（装 mod / 卸载 mod / 还原官方 / 性能调参写入）都有小概率翻车：

```
SD卡:/atmosphere/contents/0100b00b51230000/romfs/update/update.rpf     （约 900MB）
SD卡:/atmosphere/contents/0100b00b51230000/romfs/update/update2.rpf    （约 700MB）
```

把这两个文件都复制一份到电脑或 SD 卡其他位置。出问题时拷回去即可完全恢复：
- **update2.rpf**：脚本 mod 装在里面（script_rel.rpf）
- **update.rpf**：gameconfig（性能调参）和 dlclist.xml（DLC 注册）在里面

工具内的「还原官方脚本」只还原 3 个脚本条目，**不能替代完整备份**。

## 功能（v6.4）

- **DLC 页**：注册/卸载 add-on 车辆 DLC（dlclist.xml + extratitleupdatedata.meta 双注册）
- **脚本 MOD 页**：安装 .nsc 脚本 mod（自动识别 RSC7 官方格式 / mod 裸格式），
  一键还原官方脚本，内置 4 套 mod（互斥关系自动检查）
- **画质页**：一键切换 settings.xml 预设（低/中/高/极限超频专用）
- **性能页**：gameconfig 调参（已结案：瓶颈是 Tegra X1，超频才是正解）
- **工具页**：内置 5 个进度存档（4% ~ 100%）、信息查看
- **中英双语**：自动跟随系统语言

## 各功能使用方法

### 安装与启动

1. 下载 Release 里的 `gta5save.nro`，复制到 SD 卡 `/switch/` 目录
2. 按 R 启动游戏进入 hbmenu（或关游戏回 hbmenu），选「GTA5 Switch Toolbox」启动
3. 工具内：**L/R 切换页签**，十字键上下选条目，A 确认，B 返回/退出

### 快捷键速查

| 按键 | 功能 |
|---|---|
| L / R | 切换页签 |
| A / B | 确认 / 返回 |
| X / Y | 各页专用（导入 / 切列表等） |
| ZL / ZR | 数值快速调整（长按连发） |
| + | 保存并退出 |

### 脚本 MOD 页（核心功能）

**装内置 mod**：
1. 选「★模组管理」→ 弹出二级菜单（3 个官方还原项 + 4 个内置 mod）
2. 选想要的 mod（如 MEGATARD v0.9.9 汉化）→ 会列出它将覆盖的条目
3. 确认后自动写入 script_rel.rpf，装完自动回读校验
4. 互斥的 mod（如两个 ragemenu 系）装第二个前会提示先卸载

**装外部 mod**：把 .nsc 文件放到 SD 卡 `/switch/gta5save/script/` 目录
（工具首次进入脚本页会自动创建，直接丢在 `/switch/gta5save/` 下也能扫到），
在列表里选中按 A 安装。官方格式（RSC7）和 mod 裸格式都能自动识别。

**还原官方**：模组管理里选「还原官方脚本」的对应项（error_listener /
achievement_controller / shop_controller），可单独还原任意一个。

**游戏内呼出 mod 菜单**：装好后进游戏，按 **L + 十字键下**（MEGATARD/ragemenu）或
**B + 十字键右**（热咖啡），十字键导航，A 确认，B 返回。

### DLC 页

**目录说明**：
- **已装目录**（游戏实际读取）：`/atmosphere/contents/0100b00b51230000/romfs/update/switch/dlcpacks/`
- **待导入目录**（把 dlcpack 文件夹丢这里，工具扫描后导入）：
  `/switch/gta5save/dlc/`（推荐）。以下位置也能被扫到：
  `/switch/gta5save/`、`/switch/GTA5DLC/`、`/dlc/`、`/gta5dlc/`、`/switch/`

**使用方法**：
1. 把 PC 移植好的 dlcpack 文件夹（内含 `dlc.rpf`）放进待导入目录
2. DLC 页按 **Y** 切到「待导入列表」，选中按 **A** 导入
   （自动复制到 dlcpacks/ + 注册 dlclist.xml + extratitleupdatedata.meta）
3. 已装列表里选中按 A 可注册/注销；⚠️ PC 原格式包会标「会闪退」，别直接导入
4. 导入后建议重启游戏验证，一次别加太多包

### 画质页

选中预设按 A 一键切换 settings.xml：
- **低画质（默频使用）**：不超频的机器用这个
- **中画质（默认画质）**：超频后的平衡选择
- **高画质（极限超频专用）**：CPU 1963/GPU 768/RAM 2666 以上再用
- **★关VSync / ★★关VSync+全压零**：帧数优先的激进方案

### 工具页

- **内置存档库**：ZL 键呼出，5 个进度（4% / 20% / 31.6% / 61.1% / 100%），
  选中覆盖当前存档（会先确认）
- **信息页**：查看当前 mod 安装状态、RPF 剩余空间等

## 内置模组（romfs/builtin_mods/）

| 模组 | 作者 | 槽位 |
|---|---|---|
| MEGATARD v0.9.9 汉化 | Geekmaxxer | ragemenu |
| 经典 ragemenu 简体 10月7日版 | maritoguionyo | ragemenu（与上互斥） |
| 热咖啡+丧尸 | Je11yb0ne/CinnamonCoffee | simple_zombies |
| 热咖啡（不含丧尸） | Je11yb0ne/CinnamonCoffee | simple_zombies（与上互斥） |

官方原版脚本（romfs/stock_scripts/）从 build 2699 原版 update2.rpf 提取，RSC7 格式。

## 构建

需要 devkitPro A64 工具链（Windows 本机无 gcc，可用 Docker 交叉编译）：

```bash
make
```

产出 `gta5save.nro`，复制到 SD 卡 `/switch/gta5save.nro`，通过 hbmenu 启动。

## 致谢

- **[Geekmaxxer](https://github.com/Geekmaxxer)** —— [MEGATARD GTA5-NX Menu SDK](https://github.com/Geekmaxxer/GTA5NX-MG-Menu)
  （MEGATARD v0.9.9 原作者）、[GTA5-NX-Tutorial](https://geekmaxxer.github.io/GTA5-NX-Tutorial/) 教程站、
  [RPF Radio Editor](https://github.com/Geekmaxxer/gta-radio-editor)
- **[Je11yb0ne](https://github.com/Je11yb0ne)** —— [HotCoffee-NX](https://github.com/Je11yb0ne/HotCoffee-NX)
  （CinnamonCoffee 的 Switch 移植，含热咖啡+丧尸两个版本）
- **maritoguionyo** —— 经典 ragemenu v13 的 Switch 移植（无公开仓库，Discord 分发）
- **[ShinyWasabi](https://github.com/ShinyWasabi)** —— [RageMenu](https://github.com/ShinyWasabi/RageMenu)
  （经典 ragemenu 的菜单底座来源）
- GTA V Switch 移植社区（Geekmaxxer 教程站、GBAtemp）
