# Powder Game (C++ / Win32, no external deps) — Fixed2

This update addresses the first three bugs and adds a minimal collapsible element menu.

## Fixes
1. **Brush no longer overwrites existing cells** (except Eraser).
2. **Collapsible UI menu** on the right with clickable entries for Sand/Water/Stone/Eraser.
3. **Less 'floating layers' when drawing fast**: painting updates both buffers; motion rules allow falling into spots freed earlier in the same tick.

## Build (Visual Studio)
- Same as before. Output EXE will be under `out/build/.../bin/Release` or `.../bin/Debug`.

## Controls
- `1/2/3/0` still work.
- Click the **right panel** entries to select element; click the header to collapse/expand.
- Mouse wheel changes brush size; left-drag paints, right-drag erases; `R` reset; `Esc` quit.
