# CONTROLS.md — Key Bindings

> Complete reference for all controls — gameplay, menus, and debug.
> Update this file whenever a new key binding is added or changed.

---

## Gameplay Controls

| Key / Input | Action |
|-------------|--------|
| `W` | Move ship up |
| `A` | Move ship left |
| `S` | Move ship down |
| `D` | Move ship right |
| `Space` or `Left Click` | Fire laser |
| `Right Click` | Launch homing missile (finite supply, auto-reloads) |
| `E` | Activate shield (10s active, tiered recharge penalty) |
| `ESC` | Toggle pause menu |

---

## Menu / UI Controls

| Key / Input | Action |
|-------------|--------|
| `ESC` (title screen) | Quit game |
| `Q` (title screen) | Quit game |
| Mouse | Navigate menus, click buttons |

---

## Debug Shortcuts (SceneMuntasir only)

| Key | Action | Notes |
|-----|--------|-------|
| `F1` | Switch to SceneSTG | Placeholder / teammate test scene |
| `F2` | Switch to SceneJA | Jacky's test scene |
| `F3` | Switch to SceneMuntasir | Skips title screen — useful during development |
| `F9` | Cycle debug overlay | Hidden → Minimal → Detailed → Hidden |
| `F10` | Toggle camera FOV debug window | Live FOV slider + preset buttons |
| `F11` (hold 3s) | Trigger full hyperspace warp | Release before 3s to cancel |
| `F12` | Toggle wireframe rendering | Shows mesh geometry |

---

## Debug Overlay Modes (F9)

| Mode | What shows |
|------|-----------|
| Hidden | Nothing |
| Minimal | FPS, CPU%, GPU%, RAM% as text only |
| Detailed | Text + 30px scrolling waveform graph per stat |

Stats: FPS (green), CPU % (cyan), GPU % (purple via PDH), RAM % (orange).  
128-sample ring buffer, 0.5s poll → ~64 seconds of history in Detailed mode.

---

## Camera Debug Window (F10)

| Control | Action |
|---------|--------|
| FOV slider | Drag 25°–75° — live updates camera, no recompile needed |
| `30deg` button | Preset: near-orthographic, minimal distortion |
| `40deg` button | Preset: strong telephoto |
| `48deg` button | Preset: current default (same visible area as old 70°) |
| `50deg` button | Preset: balanced |
| `60deg` button | Preset: mild reduction |
| `70deg` button | Preset: original wide-angle |

Camera Z is auto-calculated to maintain the same visible world area at any FOV.  
Once a final FOV is chosen, bake it into `GameConst::kCameraZ` in `GameConstants.h`.

---

## Pause Menu Options

| Button | Action |
|--------|--------|
| RESUME | Close pause menu, return to game |
| SETTINGS | Open audio/video settings panel |
| TITLE | Save and return to title screen |
| QUIT GAME | Save and exit application |

Settings panel: audio volume sliders, resolution, fullscreen toggle, vsync mode, frame cap.  
Changes to video settings require pressing **APPLY** — nothing is applied until then.

---

## Game Over Screen

| Button | Action |
|--------|--------|
| TRY AGAIN | Restart level from beginning (rewinds LevelDirector timeline) |
| TITLE | Return to title screen |
| EXIT | Quit application |
