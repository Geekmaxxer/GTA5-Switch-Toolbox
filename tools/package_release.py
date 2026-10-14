"""Package a contributor build; source, logs and full game archives stay outside."""
import argparse,hashlib,json,shutil,zipfile
from pathlib import Path

EN="""# GTA5 Switch Toolbox — integration test

Switch testing pending. Close GTA before using the toolbox.

1. Extract the `switch` folder to your SD card and launch `gta5save.nro` from Homebrew Menu.
2. In Tools, press X for Self Radio NX or Y for Radio Imports. Touch also works.
3. Select a package: A install, X restore, Y recover an interrupted update, B back.

The included Self Radio v1.1 package requires game build
`6e3e9b3af5746e74b615bd981080c9271bc226620` and a compatible
`atmosphere/contents/0100b00b51230000/romfs/update/update.rpf`.
It patches your existing archive. Allow at least 1 GB of extra SD space for it;
keep `/switch/gta5save/packages/.backups/` until you finish testing.
Music belongs in `atmosphere/contents/0100b00b51230000/romfs/User Music/`.

Radio Imports accepts prepared station-and-timing packages from GTA Radio Editor.
It does not encode music on the console. Conflicting existing edits are refused.
An already installed package needs no changes; only toolbox-created backups can be restored here.

Toolbox: niceboat0. MG-Menu SDK, GTA Radio Editor and Self Radio NX: MEGATARD.
"""
ZH="""# GTA5 Switch 工具箱 — 集成测试版

尚待 Switch 实机测试。使用工具箱前请先关闭 GTA。

1. 将 `switch` 文件夹解压到 SD 卡，从 Homebrew Menu 启动 `gta5save.nro`。
2. 在「工具信息」页按 X 打开自定义电台 NX，按 Y 打开电台导入，也支持触屏。
3. 选中安装包：A 安装、X 还原、Y 恢复中断的更新、B 返回。

附带的自定义电台 v1.1 安装包适用于游戏构建
`6e3e9b3af5746e74b615bd981080c9271bc226620`，并需要兼容的
`atmosphere/contents/0100b00b51230000/romfs/update/update.rpf`。
它会修改现有档案。请至少预留 1 GB 额外 SD 空间；测试完成前保留
`/switch/gta5save/packages/.backups/`。
音乐放在 `atmosphere/contents/0100b00b51230000/romfs/User Music/`。

电台导入接受 GTA Radio Editor 准备好的音频及配套时间元数据安装包；不在主机上编码音乐。
遇到已有修改冲突会停止。已安装的包无需重复修改；此处只能还原工具箱创建的备份。

工具箱：niceboat0。MG-Menu SDK、GTA Radio Editor、自定义电台 NX：MEGATARD。
"""

def sha(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

RELEASE_EN="""# GTA5 Switch Toolbox — community release

Extract `switch` to your SD card. Launch `switch/gta5save/gta5save.nro` from
Homebrew Menu with GTA closed. Back up `update.rpf`, `update2.rpf` and your saves
before editing them.

- D-pad or either stick: select. A: activate. B: back/cancel. L/R: change tabs.
- Minus (-): focus the bottom buttons; Left/Right selects, A activates, B returns.
- Graphics/Perf: Up/Down selects a setting; Left/Right changes its value.
- Touch: tap items and buttons; tap scrollbar ends to page a list.
- Tools: X opens Self Radio NX; Y opens Radio Imports.

Self Radio v1.1 is included. Select its package and press A to install, X to
restore a toolbox backup, or Y to recover an interrupted update. It requires
game build `6e3e9b3af5746e74b615bd981080c9271bc226620` and compatible original
files; conflicting edits are refused. Allow at least 1 GB of extra SD space.
Keep `switch/gta5save/packages/.backups/` for restoration. Put music in
`atmosphere/contents/0100b00b51230000/romfs/User Music/`.

Radio Imports accepts prepared station/timing packages in
`switch/gta5save/packages/`; prepare matching outputs with GTA Radio Editor on
a PC. Scripts, DLC, saves and graphics tools remain available in their tabs.

Scripts > Builtin Mods includes MEGATARD v1.0.5 with ragemenu,
achievement_controller and shop_controller.

The UI passed earlier Switch Joy-Con and touch testing. This refreshed build's
MEGATARD installation and package install/restore/recovery still await Switch testing.

Toolbox: niceboat0. MG-Menu SDK, GTA Radio Editor and Self Radio NX: MEGATARD.
"""
RELEASE_ZH="""# GTA5 Switch 工具箱 — 社区发布版

将 `switch` 解压到 SD 卡，从 Homebrew Menu 启动
`switch/gta5save/gta5save.nro`，使用前先关闭 GTA。
修改前请备份 `update.rpf`、`update2.rpf` 和存档。

- 方向键或任一摇杆：选择。A：执行。B：返回/取消。L/R：换页。
- Minus (-)：选择底部按钮；左右切换，A 执行，B 返回列表。
- 画质/性能页：上下选择设置，左右调整数值。
- 触屏：点击项目和按钮；点击滚动条两端翻页。
- 工具页：X 打开自定义电台 NX；Y 打开电台导入。

附带自定义电台 v1.1。选中安装包后按 A 安装，X 还原工具箱备份，
Y 恢复中断的更新。需要游戏构建
`6e3e9b3af5746e74b615bd981080c9271bc226620` 及兼容的原始文件；
遇到修改冲突会停止。请预留至少 1 GB 额外 SD 空间，保留
`switch/gta5save/packages/.backups/` 以便还原。音乐放到
`atmosphere/contents/0100b00b51230000/romfs/User Music/`。

电台导入接受 `switch/gta5save/packages/` 下准备好的配套音频与时间元数据安装包；
请先使用电脑上的 GTA Radio Editor 生成匹配的输出。
原有脚本、DLC、存档及画质工具仍可从对应页签使用。

脚本 > 内置模组包含 MEGATARD v1.0.5，附带 ragemenu、
achievement_controller 和 shop_controller 三个脚本。

界面此前已通过 Switch Joy-Con 和触屏测试。本次更新的 MEGATARD 安装
及安装包的安装、还原和恢复仍待 Switch 实机验收。

工具箱：niceboat0。MG-Menu SDK、GTA Radio Editor、自定义电台 NX：MEGATARD。
"""

def main(a):
    nro=a.nro.read_bytes()
    if len(nro)<128 or nro[16:20]!=b'NRO0':raise ValueError('Expected a built Switch NRO')
    if a.output.exists():raise FileExistsError('Use a new test output directory')
    package=a.output/'package';destination=package/'switch/gta5save';destination.mkdir(parents=True)
    shutil.copy2(a.nro,destination/'gta5save.nro')
    if a.self_radio and not a.ui_only:
        lines=(a.self_radio/'package.txt').read_text(encoding='utf-8').splitlines()
        if 'id|self-radio-nx-1_1' not in lines or 'kind|selfradio' not in lines:raise ValueError('Expected the exported Self Radio v1.1 package')
        target=destination/'packages/self-radio-nx-1_1';target.mkdir(parents=True)
        shutil.copy2(a.self_radio/'package.txt',target/'package.txt')
        for line in lines[5:]:
            mode,_,_,source,digest,_=line.split('|')
            if mode not in ['FILE','RPF'] or not source.startswith('payload/') or '..' in Path(source).parts or Path(source).is_absolute():raise ValueError('Invalid package payload')
            original=a.self_radio/source
            if sha(original)!=digest:raise ValueError('Package payload checksum mismatch')
            output=target/source;output.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(original,output)
    ui_en="""# GTA5 Switch Toolbox — UI test 1

Switch testing pending. Replace `switch/gta5save/gta5save.nro` with this build.
Your existing package folders can be used with it.

- D-pad or either stick: navigate lists and the Tools grid.
- A: activate the selected item. B: back or cancel.
- L/R: change tabs.
- Minus (-): focus the action bar; Left/Right selects a button, A runs it, B returns to the list.
- Graphics/Perf: Up/Down selects a setting; Left/Right changes its value.
- Tools: X opens Self Radio; Y opens Radio Imports.

Please check all five tabs, scrolling, Tools, dialog cancellation, and touch.
You can test navigation without installing packages or writing game configuration.

Toolbox: niceboat0. Contributor work: MEGATARD.
"""
    ui_zh="""# GTA5 Switch 工具箱 — 界面测试版 1

尚待 Switch 实机测试。使用此版本替换 `switch/gta5save/gta5save.nro`。
可以继续使用现有的安装包文件夹。

- 方向键或任一摇杆：浏览列表及工具网格。
- A：执行选中项。B：返回或取消。
- L/R：切换页签。
- Minus (-)：选择底部操作栏；左右选按钮，A 执行，B 返回列表。
- 画质 / 性能：上下选择设置，左右调整数值。
- 工具页：X 打开自定义电台，Y 打开电台导入。

请检查五个页签、列表滚动、工具页、弹窗取消和触屏操作。
测试导航无需安装包或写入游戏配置。

工具箱：niceboat0。贡献者：MEGATARD。
"""
    if a.release and not a.ui_console_verified:
        raise ValueError('Community release guide requires confirmed Switch UI testing')
    (package/'README.en.md').write_text(RELEASE_EN if a.release else ui_en if a.ui_only else EN,encoding='utf-8')
    (package/'README.zh.md').write_text(RELEASE_ZH if a.release else ui_zh if a.ui_only else ZH,encoding='utf-8')
    archive=a.output/('GTA5-Switch-Toolbox-Community-Release.zip' if a.release else 'GTA5-Switch-Toolbox-UI-Test1.zip' if a.ui_only else 'GTA5-Switch-Toolbox-Integration-Test.zip')
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for path in sorted(package.rglob('*')):
            if path.is_file():z.write(path,path.relative_to(package).as_posix())
    files={str(path.relative_to(package)).replace('\\','/'):{'bytes':path.stat().st_size,'sha256':sha(path)} for path in package.rglob('*') if path.is_file()}
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        assert set(z.namelist())==set(files)
    record={'console_verified':False,'ui_console_verified':a.ui_console_verified,
            'package_workflows_console_verified':False,'nro_sha256':sha(a.nro),'zip_sha256':sha(archive),'files':files}
    (a.output/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
    print(archive)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--nro',type=Path,required=True);p.add_argument('--self-radio',type=Path);p.add_argument('--ui-only',action='store_true');p.add_argument('--release',action='store_true');p.add_argument('--ui-console-verified',action='store_true');p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    if not args.ui_only and not args.self_radio:p.error('Choose --ui-only or supply --self-radio')
    if args.release and (args.ui_only or not args.self_radio or not args.ui_console_verified):p.error('--release requires --self-radio and --ui-console-verified, without --ui-only')
    main(args)
