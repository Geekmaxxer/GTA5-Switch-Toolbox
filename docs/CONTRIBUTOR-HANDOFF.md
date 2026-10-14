# Toolbox integration handoff

Local source: `GTA5-Switch-Toolbox-main` inside the `switchgta5` workspace.
Upstream: <https://github.com/niceboat0/GTA5-Switch-Toolbox>.
This folder is a downloaded snapshot without `.git`; no upstream branch or PR
was created. The original 38-file snapshot and SHA-256 manifest are preserved in
`../build/toolbox-source-audit-20261014/source-before/` and `baseline-manifest.json`.
The packaged community build is separate from an official niceboat0 release.
See CONTRIBUTION.en.md / CONTRIBUTION.zh.md and RELEASE-VALIDATION.json for the
current handoff and exact validation scope.
For the downloaded Windows snapshot, the contribution patch was checked with
`git apply --check --ignore-whitespace contribution.patch` to handle CRLF context.

## Source map

| Module | Responsibility |
| --- | --- |
| `main.c` | Framebuffer/FreeType UI, touch and pad dispatch, five tabs, dialogs, saves, presets, script manager, gameconfig and DLC actions |
| `save.c/.h` | Save layout detection, AES body processing, title/block checksums, money/stat read/write bounds |
| `gc_rpf.c/.h` | RPF7 header/TOC/name pool, raw/zlib reads, bounded in-place entry writes, gameconfig XML helpers |
| `gc_script.c/.h` | `update2.rpf` and nested `script_rel.rpf`, script install/delete/list/query and stock restoration |
| `gc_dlc.c/.h` | DLC registration, archive import, nested RPF/name conversion |
| `gc_presets.h` | Embedded graphics/configuration/save preset data |
| `gc_key_container.h` | Existing save key container; untouched |
| `lang.h` | Existing runtime Chinese/English selection through `TR` |
| `romfs/` | Bundled stock scripts, mods and saves; unchanged |
| `gc_nsc.c/.h` | Added bounded raw/RSC7 payload validation |
| `gc_package.c/.h` | Added manifest parsing, full-path archive patching, staging, SHA-256, backup/restore and recovery |
| `gc_package_ui.inc` | Package controller dispatch, progress and install/restore actions |
| `gc_ui.inc` | Shared five-tab layouts, dialogs, action bars and touch controls |
| `gc_ui_core.c/.h` | Bounded list/grid navigation, direction repeat and UTF-8 decoding |
| `gc_ui_text.inc` | Measured/clipped FreeType text with a bounded glyph cache |

The original dark/blue design, five tabs, compiled presets, author/version
metadata and gameconfig settings are retained. Tools uses a four-column grid
with Self Radio and Radio Import tiles. The shared renderer fixes spacing and
text clipping; touch and both sticks/D-pad have consistent navigation.
An existing English format-string argument mismatch in Script Dir Info is fixed;
the existing action-button labels now refresh when language selection changes.
No SDK scripts, native calls, tuning values, keys or bundled mods were changed.

## NSC knowledge carried over

The Switch `.nsc` layout uses tagged pointers, 0x4000-byte code/string blocks,
an embedded name and a Jenkins name hash. File/entry renaming must agree with
the compiled name. A deliberately named installer alias may still map to the
original embedded entry, such as `simple_zombies_combo.nsc` → `simple_zombies.nsc`.

RSC7 decoding tries a valid bare payload, then complete raw deflate; resource
flags are not treated as a compression selector. Validation is bounded to
16 MiB and runs before opening the destination archive for writing. Existing
RSC7 stock resources retain the author's resource installation path; mod payloads
retain the ordinary compressed-file path. Do not bulk reclassify stock entries.

Fixed actual archive errors: root child count was written to entry 1 rather than
directory entry 0; relocating an inner payload mixed relative and absolute
offsets; deletion confused the root directory index with its first child.
Seek/write failures and final SD flush/commit failures now return errors.
The original script operations still edit in place; the new transaction backend
does not silently replace every existing write path. Keep full update2 backups.

Structural checks do not prove native compatibility or adapt headers. Continue
using MG-SwitchMenu SDK's NscAdapter/NscDump for build-specific adaptation and
decoded native-table comparison. Native values rotate by `(codeLength + index)`;
CALL_NATIVE indexes are big-endian. Do not infer PC/Switch compatibility from
prefixes. The local stock comparison covered 1,020 of 1,026 extracted scripts
and 6,352 observed native hashes; six files were undecodable. The bundled classic
RageMenu has one hash absent from that observed union, so no new blanket rejection
rule was enabled. All ten bundled scripts pass the new structure/name checks.

## Self Radio package

`tools/export_package.py selfradio` consumes the minimal v1.1 HARD package and
its builder `release-verification.json`. It verifies the confirmed runtime and
all sixteen archive payload hashes. The resulting eighteen-file package contains
three general archive patches, thirteen language archives, subsdk4 and main.npdm.
The patched module is the user's confirmed manual-skip fix, not an older probe.

Runtime SHA-256: `3a58059dcfe0b2a9f1cd07328c237e6d3ee42528710e43e8913dd9fa2f0ffd8a`.
Supported game build: `6e3e9b3af5746e74b615bd981080c9271bc226620`.
Public Self Radio version remains **v1.1**. The toolbox is a community build;
the upstream app version metadata is unchanged.
The existing 256-track and MP3/WAV/AAC/M4A/FLAC implementation remains in that
runtime; the toolbox does not add another game-time scanner or decoder.

Existing target bytes must match the manifest's expected baseline or replacement.
Occupied foreign subsdk4/NPDM files are refused. An already installed package is
an idempotent no-op. It does not create a fictitious backup of an installation
made by another tool. Music, playback caches, saves and gameconfig are untouched.
The checks compare affected assets, not the running executable's build ID;
the supported-build requirement still applies.

## Radio import and timing

Desktop reference: `Documents/GitHub/gta-radio-editor`, especially
`RadioTimingService`, `RadioCueService` and `RpfRadioService`. The workspace mirror
is older. Real schedules use `Dat54StreamingSound.Duration` in sounds.dat54.rel
and AWC sample markers; `Dat151RadioTrackSettings` has no invented timing fields.
Both completed station and timing outputs must be installed together.

`tools/export_package.py radio` verifies OPEN RPF directory/payload bounds and
presence of AWC and dat54 entries, then exports a paired overlay package.
It does not independently recompute timing or prove the two outputs were built
together. Supply matching Radio Editor outputs and exact pre-install overlay
files when overlays already exist. The import only publishes those files after
both staged files have been verified. Native audio editing/encoding in the NRO
remains future work; CodeWalker .NET assemblies cannot simply be linked into C.

## Transaction behavior and cost

Manifest version 1 uses bounded pipe-delimited UTF-8 records; see an exported
`package.txt`. Relative paths, file counts, types, hash lengths and target paths
are checked. RPF resolution follows full directory ownership, avoiding basename
substring collisions. Modified binary entries are appended as raw, 512-aligned
payloads so embedded RPFs remain directly seekable; other records/data remain.

Prepare all target copies, apply patches, re-read and hash them, write a flushed
recovery journal, then rename originals into retained backups and publish stages.
A failed commit attempts rollback. Y Recover can retry an interrupted commit.
Restore refuses to overwrite later edits. Recovery journals are written through
a temporary file before publication. These checks do not make FAT/exFAT immune
to physical corruption or prove libnx behavior on hardware.

Copy/hash buffers are 64 KiB; package lists are capped at 16 and each package at
24 files. Progress redraws are throttled to roughly 100 ms, including restore
hashing. Archive entry reads still use the existing RPF reader's memory limits.
Full archive staging costs additional SD capacity and I/O, not GTA frame time:
the toolbox is closed before running GTA. New overlays are removed on restore;
pre-existing files are restored byte for byte.

## Building and validation

Use current devkitPro, devkitA64, libnx, switch-zlib, switch-freetype,
switch-mbedtls and freetype's bzip2/png/harfbuzz dependencies. No libraries are
copied into source. Run `make` from a devkitPro shell, or on Windows:

```powershell
./tools/build.ps1 -DevkitPro 'C:/devkitPro'
```

Host tests compile the actual C backends through MSVC, using an official zlib
source tree. Test-only libnx stubs cover mount/commit; Windows BCrypt supplies
SHA-256 in host fixtures. Production links mbedTLS and libnx.

```powershell
python tests/test_scripts.py --output ../build/toolbox-tests/scripts --zlib-source C:/deps/zlib-1.3.1
python tests/test_packages.py --output ../build/toolbox-tests/packages --zlib-source C:/deps/zlib-1.3.1
```

Optional `test_self_radio.py` takes an original archive, prepared package and an
independent RPF reader. It uses disposable copies, verifies all eighteen files,
compares every original byte except the selected TOC records, checks repeat
installation and verifies byte-exact restoration. No installed game is edited.

The user confirmed Switch Joy-Con and touch UI testing on 2026-10-14. New package
workflow acceptance remains pending: install from compatible original files,
reopen the toolbox, boot GTA,
check loading/autosave/pause/Audio, play Self Radio, restore and boot again.
Test paired radio imports, later-edit refusal and interrupted-update recovery
on a spare SD copy. Preserve the existing DLC/save/script workflows during soak.

Optional future work, outside this contribution: share staging with older write paths,
add an executable build-ID probe where the port exposes it, and port narrowly
verified AWC/dat54 editing operations rather than a wholesale desktop encoder.

Credits: niceboat0 for the toolbox; MEGATARD for MG-Menu SDK, GTA Radio Editor
and Self Radio NX source/workspace knowledge. Existing attribution is retained.

## Reproduce the packages

These commands create new output folders and refuse to overwrite an existing one.
The HARD input retains its parent release-verification.json. Radio imports need
the desktop editor's matching station and timing outputs; pass --station-before
and --timing-before when replacing existing SD overlays.

```sh
python tools/export_package.py selfradio --input ../Self-Radio-NX-v1.1/HARD/package --output ../prepared-self-radio
python tools/export_package.py radio --station ../RADIO_01_CLASS_ROCK.rpf --timing ../audio_rel.rpf --id my-radio --title "My Radio" --title-zh "我的电台" --output ../prepared-radio
python tools/package_release.py --release --ui-console-verified --nro gta5save.nro --self-radio ../prepared-self-radio --output ../community-release
python tools/package_contribution.py --baseline ../audit/baseline-manifest.json --work ../audit/contribution-check --output ../source-contribution
```

Use --ui-console-verified only after the corresponding binary's UI has passed
Switch testing. This flag does not certify package workflows. Source packaging
requires the preserved source-before folder next to baseline-manifest.json for
changed original files; unchanged assets are validated against the manifest.
The resulting patch is applied to a second baseline copy and every source file
is compared byte for byte. Logs, SDKs, build artifacts and full game archives are
excluded; original repo romfs assets remain so the source tree can build.
