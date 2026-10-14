# UI refinement test 1

The five existing tabs and dark/blue palette remain. All tab content, empty
states, selectors, save/profile dialogs, Mod Manager, package screens and
confirmations share geometry and typography. The old rendering body is retained
in `../build/toolbox-ui-refinement-20261014/source-before/`, alongside the previous
NRO. This is a test build, not an official version bump.

`gc_ui.inc` owns presentation and focus. `gc_ui_core.c/.h` owns bounded viewport,
grid, direction repeat and UTF-8 helpers. `gc_ui_text.inc` owns FreeType metrics,
wrapping, clipping and a fixed glyph cache. Backend file operations remain in
their existing modules and main.c functions.

Concrete fixes:

- Matching display/navigation capacities: six ordinary rows, eight value rows,
  five dialog rows; all selectors scroll to the selected item.
- Tools is a controller-selectable four-column grid. Every action-bar operation
  is reachable through Minus, Left/Right and A. B returns focus to the list.
- Both sticks and D-pad use a shared 320 ms initial / 75 ms repeat timer and one
  dominant direction. The stronger stick is used when both move.
- Touch owns its input frame. Opening a screen consumes its activation key;
  cancel wins over simultaneous activation. Overlay and action-bar navigation
  suppresses background value edits. L/R remains available from the action bar.
- Graphics uses Up/Down for selection and Left/Right for values, matching Perf.
  Loading a story preset or backup focuses Save Changes as the next Tools action.
- Actual FreeType advances are used for both measuring and drawing. Display labels
  have room for full bilingual names. Long text is fitted/wrapped at UTF-8 boundaries.
- Touch scrollbar tracks page long lists/reports, and selected Graphics/Perf rows
  expose touch minus/plus controls. A held finger does not generate repeated
  button presses or automatically confirm a newly opened dialog.
  Action-bar arrows expose additional buttons to touch users.
- Fixed the old rounded-card routine's corner notches; focus outlines also follow
  rounded geometry. Touch feedback is a brief blue outline, not a long flashing frame.
- Reports and Mod Manager scroll within their panels. Package lists show position
  and a scrollbar. Dialog touch zones replace background controls.
- DLC archive-format probing is removed from painting; the existing import-time
  checks remain. Perf's baseline XML values are cached for display and invalidated
  after writes; drawing does not change those values.
- Package progress reports current-file bytes; progress resets are not presented
  as one aggregate completion percentage. Result messages remain visible in the
  package/manager dialogs. Main-page toasts stay clear of the action bar.

The glyph cache has 512 entries with 48x48 bitmap capacity, roughly 1.2 MB of fixed
BSS; no per-frame bitmap allocation is added. This is toolbox memory, not game
runtime memory. Render and input checks are not Switch performance measurements.

Host validation uses the actual C renderer and controller router, MSVC and a
freetype-py 2.5.1 host DLL. Backend fixture functions supply sample display data;
no game files are opened. The Windows font is different from the Switch shared
font. PNG previews therefore validate layout, not exact console font rasterization.
The unused legacy save-editor initializer is omitted from the host harness only.

```powershell
python tests/test_ui_core.py --output ../build/toolbox-ui-core-tests
python -m pip install freetype-py==2.5.1
python tools/preview_ui.py --output ../build/toolbox-ui-previews --devkitpro C:/devkitPro --font C:/Windows/Fonts/msyh.ttc
./tools/build.ps1 -DevkitPro C:/devkitPro
```

Console checks: all five tabs with paired Joy-Cons/handheld/Pro Controller, both
sticks and D-pad repeat, the last row of long lists, Tools tile activation,
Minus action-bar focus and L/R switching, opening/canceling every dialog, and
touch while a direction is held. Navigation testing does not require installing
anything or writing gameconfig. After navigation passes, recheck existing install,
restore, save and import flows on disposable copies. The user confirmed Switch
Joy-Con and touch UI testing on 2026-10-14. Package install/restore/recovery
console acceptance remains pending; individual controller modes and backend
workflows are not inferred from the UI confirmation.
