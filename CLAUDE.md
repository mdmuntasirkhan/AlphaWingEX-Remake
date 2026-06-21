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

### Enemy class hierarchy

`Enemy` (`Enemy.h`) is an **abstract base class** — it is not a monolithic class managing all pools. It provides the shared `debris` vector, `SpawnHitDebris()` / `SpawnKillDebris()` helpers, and the virtual interface (`Update`, `Render`, `OnDestroy`, `Reset`). The three concrete subclasses are each owned as a separate raw pointer in `SceneMuntasir`:

- **`Asteroid`** — manages two parallel pools: large asteroids (6 HP) and small asteroids (3 HP). Both share one mesh. Exposes `GetAsteroidPositions()` / `GetSmallAsteroidPositions()` for collision. `DamageAsteroid()` / `DamageSmallAsteroid()` return `true` on kill. Debris spawns on hit and on kill.
- **`Bot01`** — wave-based enemy (10 HP). Steers toward the player's Y position. Has a `totalTime` wave-progression timer (persisted in `SaveData`). Begins spawning after `totalTime > 30.0f`. Exposes `GetBot01Positions()`. Per-instance `bot01HitTimers` drive a white-flash-on-hit effect. `PushX()` / `PushY()` apply missile knockback. `IsWaveComplete()` / `ResetWave()` control wave flow. `DamageBot01()` returns `true` on kill.
- **`Bot02`** — mini-boss enemy (20 HP). Hovers at a fixed world-X position (`kHoverX = 6.0f`) and oscillates vertically. Max 2 active at a time. Has its own projectile pool (`GetBulletPositions()` / `GetBulletVelocities()`) and fires at the player every `kFireInterval = 3.0f` seconds. Four mesh parts: body, cockpit, fin, thrust — plus a fragment mesh (asteroid shape) and a dedicated bullet mesh. Bot02 bullets must be handled separately in collision detection. **Note:** `MissileTargetType` only covers `ASTEROID` and `BOT01` — homing missiles cannot lock onto Bot02.

### Game objects in SceneMuntasir

- **`Player`** — four mesh components: ship body, cockpit, attachment, and thrust flame. Handles WASD movement with velocity/friction physics, health/lives, Z-roll on W/S (5° intentional wobble — do not change without asking), shield activation with Fresnel rim animation, and state-restore setters used by the save/load system. Shield collision is elliptical: X half-axis 1.05, Y half-axis 0.75 world units.
- **`Bullet`** — two pools: straight laser shots and homing missiles. Missiles use proportional-navigation (PN) guidance (`missileNavigationGain`) with a brief straight-flight launch phase before homing kicks in. Re-acquires nearest target if the locked one is destroyed mid-flight. Target acquisition priority: Bot01 first, then large asteroids.
- **`Environment`** — ImGui-drawn starfield scrolling background. Supports `SPACE` and `WATER` environment types (water adds jitter and a speed multiplier).

Collision detection lives in `SceneMuntasir::Update()` — all shapes are **ellipses**, not circles. Each enemy type uses different half-axis constants. The method iterates bullet / missile / shield position vectors against enemy position vectors and calls the appropriate `DamageX()` / `RemoveX()` by index.

**Score values:** large asteroid kill = 50 pts, small asteroid kill = 25 pts, Bot01 kill = 100 pts. Shield kills score at reduced rates (10/5 pts). Shards dropped on kill: large asteroid = 3, small = 2, Bot01 = 5 (7 from missile kill), Bot01 missile non-kill hit = 2.

### World coordinate space

All game objects are placed at Z = -10. Enemies spawn at X = 15 and are despawned when X < -15. Player Y is bounded within roughly ±3.5 to ±5 units. Bot01 begins spawning after `totalTime > 30.0f` (the wave progression timer stored in `Bot01` and persisted in `SaveData`). Firing the laser or missile applies a negative-X recoil impulse to the player.

### RPG shard system

Enemies drop `Shard` structs (pos, vel, spin) when killed. Shards drift toward the player when within `kMagnetRadius` (2.4 units) using inverse-linear pull, and are collected within `kCollectRadius` (0.5 units). On death the player drops a `DroppedShard` pile at their last position; the pile pulses and can be recovered once per death within a 1.2-unit radius. `shardCount` is persisted via `SaveData`.

### Save / Load system

`SaveData` (`SaveData.h/.cpp`) is a global singleton (`SaveData::current`). Two separate file types:

- **`profile_<name>.dat`** — per-profile plain-text key-value file. Stores: profile name, shard count, high score, full mid-session state (health, lives, score, player position, wave timer, lost shard pile), and audio volume prefs. Profile discovery uses MSVC `<io.h>` (`_findfirst` / `_findnext`) — Windows-only. `SaveData::DeleteProfile()` deletes the file from disk.
- **`settings.dat`** — machine-level settings (resolution index, fullscreen, vsync mode, volume). Written by `SaveMachineSettings()` / read by `LoadMachineSettings()`. `SceneManager` uses this exclusively for video changes so they never touch profile files.

`SceneMuntasir` auto-saves every 10 seconds (`kAutoSaveInterval`) and saves explicitly on quit and scene exit via `SaveGame()`. `SceneTitle` reads `GetProfileList()` to populate the load-game list and writes `SaveData::current` before transitioning to `SceneMuntasir`.

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

### Light position constraint

`lightPos.Y` must stay ≥ 50 in `SceneMuntasir::Render()`. Moving the light closer causes a visible gradient color shift on the ship mesh.

### Audio

Two audio paths exist in parallel:

1. **Per-scene SDL3 streams** — `SceneMuntasir` holds its own `SDL_AudioStream* audioPlayer` (music) and `SDL_AudioStream* sfxPlayer` (SFX), both opened at 44100 Hz S16 stereo. `Sound::Play()` queues a WAV into the given stream. A third low-gain `hoverStream` plays UI hover sounds at 35% of sfxVolume.
2. **`SoundManager`** — a pooled manager with one BGM stream and 12 SFX pipes (`PIPE1`–`PIPE12`), defaulting to 48000 Hz. Currently present but not wired into `SceneMuntasir`; it is the intended replacement for the per-scene streams.

Music and SFX volumes are stored in `SaveData` and applied via ImGui sliders in `DrawGui()`.

Video settings (resolution, fullscreen, vsync) follow a **pending/apply** pattern: both `SceneTitle` and `SceneMuntasir` copy `SaveData::current` values into local `pendingResIndex` / `pendingFullscreen` / `pendingVsync` fields when the settings panel opens. Changes only commit when the user presses **APPLY** — this prevents mid-game resolution thrash. The `SaveData::kResolutionW/H` table has **7 presets**: 1280×720, 1600×900, 1920×1080, 2560×1440, 3840×2160 (4K), 5120×2160 (5K2K), 7680×2160 (8K2K).

### External libraries (vendored in-tree)

- **ImGui** — all `imgui*.cpp/.h` files are checked in directly (SDL3 + OpenGL3 backends).
- **tiny_obj_loader.h** — single-header OBJ loader.
- **GameDev** — external, installed at `C:\GameDev`. Provides `MATH::Vec3`, `Matrix4`, `Quaternion`, `MMath`, etc.
- **`Body.h`** — a physics body stub included in the project for future use; currently unused by any game object.

### Assets

All runtime assets are relative paths from the working directory (the project folder):
- `meshes/` — OBJ files (player ship, cockpit, attachment, thrust, shield, bullet, missile, asteroid, bot01, bot02 parts)
- `shaders/` — GLSL source files
- `audio/music/` and `audio/sfx/` — WAV files
- `profile_<name>.dat` — plain-text save files, one per player profile
- `settings.dat` — machine-level video/audio settings

### Debug logging

`Debug::Info()` / `Debug::Error()` / `Debug::FatalError()` write to `GameEngineLog.txt` in the working directory. The `__FILE__` / `__LINE__` macros are passed at every call site.
