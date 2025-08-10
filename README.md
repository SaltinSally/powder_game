# Powder Game (C++ / Win32, no external deps)

A minimal, dependency-free starting point for a falling-sand style simulator.
Runs fast on older Surface-class laptops. Uses Win32 + GDI for rendering pixels.

## Features in this scaffold
- Fixed-size simulation grid (default 320×180), scaled to fit the window.
- Elements: Empty, Sand, Water, Stone.
- Gravity and simple granular/liquid flow rules.
- Brush painting with the mouse.
- Hotkeys to switch elements and adjust brush size.

## Build (Visual Studio CMake)
1. Open **x64 Native Tools Command Prompt for VS 20XX**.
2. Navigate to this folder:
   ```bat
   cd %CD%\powder-game-cpp
   mkdir build && cd build
   cmake -G "Visual Studio 17 2022" -A x64 ..
   cmake --build . --config Release
   ```
3. Run the exe from `build/Release/PowderGame.exe` (or from within VS).

## Controls
- **1** = Sand, **2** = Water, **3** = Stone, **0** = Eraser (Empty)
- **Mouse Left Drag** = paint selected element
- **Mouse Right Drag** = erase (Empty)
- **Mouse Wheel** = change brush radius
- **R** = reset scene
- **Esc** = quit

## Next Steps (from our design doc)
- Add heat/temperature and pressure fields.
- Add more elements (lava, fire, steam) and reactions.
- Chunked updates + sleeping for perf.
- Optional D3D11 compute for heat/pressure later.
