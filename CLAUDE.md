# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

AlphaWingEX-Remake is a 2.5D vertical-scrolling space shooter built in C++ with OpenGL, recreating the classic J2ME mobile game *Alpha Wing EX*. It's a personal/midstone project for Game Development at Humber College.

## Build & Run

**Platform:** Windows only. Requires Visual Studio (tested with VS 2015+ solution format).

**Prerequisites:**
1. Extract `GameDev.7z` directly to `C:\GameDev` — this is the custom math/utility library (`MATH::Vec3`, `Matrix4`, `MMath`, `Quaternion`, etc.) that the project links against. Without it, nothing compiles.
2. Open `ComponentFramework.sln` in Visual Studio.
3. Build and run as **x86** (Win32). The x64 configuration exists but the `static_assert` in `Main.cpp` is commented out — prefer x86.

**Running:**
- Press **F5** in Visual Studio to build and launch.
- At startup, `SceneTitle` loads first (profile select / new game). From there the game transitions to `SceneMuntasir`.

**Scene switching at runtime (keyboard, debug shortcuts that bypass the title):**
| Key | Scene / Action |
|-----|---------------|
| F1  | SceneSTG (placeholder/test) |
| F2  | SceneJA (Jacky's test scene) |
| F3  | SceneMuntasir (main game, no profile load) |
| F12 | Toggle wireframe rendering (in SceneMuntasir) |
| Q / ESC (title) | Quit |

**Gameplay controls:**
- WASD — move player
- Space / Left-click — fire laser
- Right-click — launch homing missile (finite supply, auto-reloads)
- E — activate shield (5 s active, 15 s cooldown)
- ESC — toggle pause (opens in-game pause menu; does **not** quit)

There is no automated test suite. Verification is done by running the game.

## Architecture

### Entry point & lifecycle

`Main.cpp` → creates `SceneManager` → calls `Initialize()` then `Run()`. `_CrtDumpMemoryLeaks()` runs at exit for leak detection.

`SceneManager` owns the game loop: polls SDL events, drives ImGui frames, then calls `currentScene->Update()` / `RenderBackground()` / `Render()` / `DrawGui()` each tick. The exact order within a frame is:

```
Update(dt) → RenderBackground() → Render() → DrawGui()
→ drain SceneSwitcher → ImGui::Render() → SDL_GL_SwapWindow
```

Scene switches are deferred: any scene calls `SceneSwitcher::Request(GameScene::X)`, and `SceneManager::Run()` drains it *after* `DrawGui()` so the current frame completes cleanly before `BuildNewScene()` destroys the old scene.

### Scene system

`Scene` (abstract base in `Scene.h`) defines the interface every scene must implement:
```
OnCreate() → Update(dt) → RenderBackground() → Render() → DrawGui() → OnDestroy()
```
`HandleEvents()` is called by `SceneManager::HandleEvents()` for every SDL event.

Concrete scenes:
- **`SceneTitle`** — profile selector / new-game flow. Three internal states (`MAIN`, `NEW_GAME_NAME`, `LOAD_SELECT`). Maintains a leaderboard cache (`vector<pair<string,int>>`) sorted by high score descending, built from `SaveData::GetLeaderboard()`. On launch it calls `SceneSwitcher::Request(GameScene::MUN)` to hand off to the main game.
- **`SceneMuntasir`** — the real game.
- **`SceneSTG`**, **`SceneJA`** — stubs/experiments used by teammates.

### SceneSwitcher

`SceneSwitcher` (in `SceneSwitcher.h`) is a zero-dependency static struct that breaks the circular-include problem between scenes and `SceneManager`. Any scene calls `SceneSwitcher::Request(GameScene::X)`; `SceneManager` checks `SceneSwitcher::hasPending` each frame after `DrawGui()`. No scene header needs to `#include "SceneManager.h"`.

### Game objects in SceneMuntasir

Each major game object follows the same `OnCreate / Update / Render / OnDestroy` pattern and is owned (raw pointer) by `SceneMuntasir`:

- **`Player`** — four mesh components: ship body, cockpit, attachment, and thrust flame. Handles WASD movement with velocity/friction physics, health/lives, Z-roll on W/S (5° intentional wobble — do not change without asking), shield activation with Fresnel rim animation, and state-restore setters used by the save/load system.
- **`Enemy`** — manages three enemy pools tracked as parallel `std::vector`s: large asteroids (6 HP), small asteroids (3 HP), and Bot01 (10 HP, wave 2). Note: the `EnemyType` enum only has `ASTEROID` and `BOT01` — `smallAsteroid` is a separate internal pool without its own enum value. Exposes `GetAsteroidPositions()` / `GetSmallAsteroidPositions()` / `GetBot01Positions()` for collision. `DamageX()` returns `true` on kill. Bot01 has a per-instance `bot01HitTimers` for a white-flash-on-hit effect. Debris particles (`struct Debris`) spawn on hit and on kill with different counts.
- **`Bullet`** — two pools: straight laser shots and homing missiles. Missiles use proportional-navigation (PN) guidance (`missileNavigationGain`) with a brief straight-flight launch phase before homing kicks in. Re-acquires nearest target if the locked one is destroyed mid-flight.
- **`Environment`** — ImGui-drawn starfield scrolling background. Supports `SPACE` and `WATER` environment types (water adds jitter and a speed multiplier).

Collision detection lives in `SceneMuntasir::Update()` — it iterates bullet/missile position vectors against enemy position vectors and calls `DamageX()` / `RemoveX()` manually by index.

### World coordinate space

All game objects are placed at Z = -10. Enemies spawn at X = 15 and are despawned when X < -15. Player Y is bounded within roughly ±3.5 to ±5 units. Bot01 begins spawning after `totalTime > 30.0f` (the wave progression timer stored in `Enemy` and persisted in `SaveData`).

### RPG shard system

Enemies drop `Shard` structs (pos, vel, spin) when killed. Shards drift toward the player when within `kMagnetRadius` (2.4 units) and are collected within `kCollectRadius` (0.5 units). On death the player drops a `DroppedShard` pile at their last position; the pile pulses and can be recovered once per death. `shardCount` is persisted via `SaveData`.

### Save / Load system

`SaveData` (in `SaveData.h/.cpp`) is a global singleton (`SaveData::current`). It stores:
- Profile name, shard count, high score
- Full mid-session state: health, lives, score, player position, enemy wave-progression timer
- Lost shard pile state (position, count)
- Audio volume preferences

Save files are plain-text key-value files named `profile_<name>.dat` in the working directory. Profile discovery uses MSVC `<io.h>` (`_findfirst` / `_findnext`) — Windows-only.

`SceneMuntasir` auto-saves every 10 seconds (`kAutoSaveInterval`) and saves explicitly on quit via `SaveGame()`.

`SceneTitle` reads `GetProfileList()` to populate the load-game list and writes `SaveData::current` before transitioning to `SceneMuntasir`.

### Camera

Fixed perspective camera set once in `SceneMuntasir::OnCreate()`:
- View: `MMath::lookAt(Vec3(0,0,0), Vec3(0,0,-1), Vec3(0,1,0))` — camera never moves.
- Projection: 45° FOV, 16:9 aspect, 0.1–100 clip range.
- Player and enemies move through the world; the camera is stationary.

### Rendering pipeline

Single shader pair: `shaders/alphaWingVert.glsl` + `shaders/alphaWingFrag.glsl`.

The fragment shader has two modes selected by the `emissive` uniform:
- `emissive == 0` → Phong lighting (ambient + diffuse + specular).
- `emissive > 0.5` → Fresnel rim effect (transparent at center, opaque at silhouette edge). Used for the shield bubble; back faces are culled by the caller so only the outer surface renders. No extra uniforms are required beyond `color`, `lightPos`, and `viewPos`.

Uniforms are cached by name in `Shader`'s `unordered_map<string, GLuint>` and retrieved via `GetUniformID()`.

`RenderBackground()` runs before `Render()` each frame. `Environment` draws via `ImGui::GetBackgroundDrawList()` in `RenderBackground()`, bypassing OpenGL entirely. This is why `RenderBackground()` is a separate virtual — it keeps ImGui background drawing decoupled from the 3D pass.

### Audio

Two audio paths exist in parallel:

1. **Per-scene SDL3 streams** — `SceneMuntasir` holds its own `SDL_AudioStream* audioPlayer` (music) and `SDL_AudioStream* sfxPlayer` (SFX), both opened at 44100 Hz S16 stereo. `Sound::Play()` queues a WAV into the given stream.
2. **`SoundManager`** — a pooled manager with one BGM stream and 12 SFX pipes (`PIPE1`–`PIPE12`), defaulting to 48000 Hz. Currently present but not wired into `SceneMuntasir`; it is the intended replacement for the per-scene streams.

Music and SFX volumes are stored in `SaveData` and applied via ImGui sliders in `DrawGui()`.

Video settings (resolution, fullscreen, vsync) follow a **pending/apply** pattern: both `SceneTitle` and `SceneMuntasir` copy `SaveData::current` values into local `pendingResIndex` / `pendingFullscreen` / `pendingVsync` fields when the settings panel opens. Changes only commit when the user presses **APPLY** — this prevents mid-game resolution thrash. The same `SaveData::kResolutionW/H` table (4 presets: 1280×720, 1600×900, 1920×1080, 2560×1440) is shared by both scenes and `SceneManager`.

### Light position constraint

`lightPos.Y` must stay ≥ 50 in `SceneMuntasir::Render()`. Moving the light closer causes a visible gradient color shift on the ship mesh.

### External libraries (vendored in-tree)

- **ImGui** — all `imgui*.cpp/.h` files are checked in directly (SDL3 + OpenGL3 backends).
- **tiny_obj_loader.h** — single-header OBJ loader.
- **GameDev** — external, installed at `C:\GameDev`. Provides `MATH::Vec3`, `Matrix4`, `Quaternion`, `MMath`, etc.
- **`Body.h`** — a physics body stub included in the project for future use; currently unused by any game object.

### Assets

All runtime assets are relative paths from the working directory (the project folder):
- `meshes/` — OBJ files (player ship, cockpit, attachment, thrust, shield, bullet, missile, asteroid, bot01, bot01 thrust)
- `shaders/` — GLSL source files
- `audio/music/` and `audio/sfx/` — WAV files
- `profile_<name>.dat` — plain-text save files, one per player profile

### Debug logging

`Debug::Info()` / `Debug::Error()` / `Debug::FatalError()` write to `GameEngineLog.txt` in the working directory. The `__FILE__` / `__LINE__` macros are passed at every call site.
