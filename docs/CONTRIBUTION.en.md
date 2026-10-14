# Refine bilingual UI and add checked script/radio installation

The previous toolbox relied heavily on touch and had inconsistent spacing,
text sizing and focus. This change retains the five tabs and dark/blue design,
adds complete controller navigation, and fits English/Chinese labels and dialogs
within shared layouts. Both sticks and D-pad navigate; Minus focuses action
buttons. Touch supports paging and value adjustment without repeated activation.

Tools gains Self Radio NX v1.1 installation and paired radio/timing imports.
Packages check paths and hashes, stage changes, retain backups, and support
restore and interrupted-update recovery. Script installation validates bounded
raw/RSC7 payloads and embedded names before writing. Nested script archive
relocation, directory counts and write-error handling are corrected.

## Review and build

The source ZIP is a complete buildable tree, including unchanged original romfs
assets. `contribution.patch` combines all changes against the preserved downloaded
snapshot; `baseline-manifest.json` identifies that snapshot. If upstream has moved,
review/rebase the patch before merging. No upstream branch or PR was published.

```sh
git apply --check --ignore-whitespace contribution.patch
git apply --ignore-whitespace contribution.patch
make
```

Build with devkitPro/devkitA64, libnx, switch-zlib, switch-freetype and
switch-mbedtls. On Windows, `tools/build.ps1 -DevkitPro C:/devkitPro` sets paths.
`docs/CONTRIBUTOR-HANDOFF.md` explains the backend contracts and exporter commands;
`docs/UI-REFINEMENT.md` explains the renderer and host preview checks.

## Validation and limits

User confirmed Switch Joy-Con and touch UI testing on 2026-10-14. The shipped NRO
is the same UI Test 1 binary. Local checks passed: 623 UI core, 35 controller,
30 script, 25 package and 15 exporter checks, plus 48 bilingual screen previews.
A real original Self Radio archive passed install, repeat-install and byte-exact
restore checks. The combined patch applies and reproduces every source file.

New package install/restore/recovery still needs Switch acceptance. Existing
save/DLC/script write paths retain their in-place behavior. NSC structure checks
do not prove native compatibility. Radio import requires matching desktop-created
station and timing outputs; it does not encode music on the console. No tuning
values, bundled scripts, saves, keys or official app version metadata were changed.

Toolbox: niceboat0. Contributor: MEGATARD; integration knowledge from MG-Menu SDK,
GTA Radio Editor and Self Radio NX. Existing attribution is retained.
