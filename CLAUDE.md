# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

AlphaWingEX-Remake is a 2.5D vertical-scrolling space shooter built in C++ with OpenGL, recreating the classic J2ME mobile game *Alpha Wing EX*. It's a personal/midstone project for Game Development at Humber College.

## Build & Run

**Platform:** Windows only. Requires Visual Studio (tested with VS 2015+ solution format).

**Prerequisites:**
1. Extract `GameDev.7z` directly to `C:\GameDev` — this is the custom math/utility library (`MATH::Vec3`, `Matrix4`, `MMath`, `Quaternion`, etc.) that the project links against. Without it, nothing compiles.
2. Open `ComponentFramework.sln` in Visual Studio.
3. Build and run as **x86** (Win32). The x64 configuration exists but prefer x86.

**Running:** Press **F5** in Visual Studio to build and launch. At startup, `SceneTitle` loads first (profile select / new game). From there the game transitions to `SceneMuntasir`.

**Controls:**

| Key / Input | Action |
|-------------|--------|
| WASD | Move player |
| Space / Left-click | Fire laser |
| Right-click | Launch homing missile (finite supply, auto-reloads) |
| E | Activate shield (10 s active, tiered recharge penalty) |
| Q (hold 3 s) | Trigger hyperspace warp; release before 3 s to cancel |
| ESC | Toggle pause menu (does **not** quit) |
| Q / ESC (title screen) | Quit |

**Debug shortcuts (SceneMuntasir only):**

| Key | Action |
|-----|--------|
| F1  | SceneSTG (placeholder/test) |
| F2  | SceneJA (teammate test scene) |
| F3  | SceneMuntasir (main game, no profile load) |
| F9  | Cycle debug overlay: hidden → Minimal → Detailed |
| F10 | Toggle camera FOV debug window (live FOV slider 25°–75°) |
| F12 | Toggle wireframe rendering |

There is no automated test suite. Verification is done by running the game.

## Supplemental docs

| Document | What it covers |
|----------|---------------|
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Engine structure — frame loop, scene lifecycle, data flow |
| [docs/SYSTEMS.md](docs/SYSTEMS.md) | How each system works — level scripting, enemies, warp, save/load, audio, camera |
| [docs/HOW_TO_ADD.md](docs/HOW_TO_ADD.md) | Step-by-step recipes — add an enemy, a level, a HUD element, a save field |
| [docs/CONTROLS.md](docs/CONTROLS.md) | Every key binding — gameplay and debug shortcuts |

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
`HandleEvents()` is called by `SceneManager::HandleEvents()` for every SDL event. `Render()` is `const` — all state changes happen in `Update()` or `DrawGui()`.

Concrete scenes:
- **`SceneTitle`** — profile selector / new-game flow. Three internal states (`MAIN`, `NEW_GAME_NAME`, `LOAD_SELECT`). Maintains a leaderboard cache (`vector<pair<string,int>>`) sorted by high score descending. On launch it calls `SceneSwitcher::Request(GameScene::MUN)` to hand off to the main game.
- **`SceneMuntasir`** — the real game.
- **`SceneSTG`**, **`SceneJA`** — stubs/experiments used by teammates.

`OnVideoChanged(int w, int h)` is a Scene virtual (default no-op) that `SceneManager` calls after applying a resolution change. `SceneMuntasir` overrides it to recompute the projection matrix and world bounds for the new aspect ratio.

### SceneSwitcher

`SceneSwitcher` (in `SceneSwitcher.h`) is a zero-dependency static struct that breaks the circular-include problem between scenes and `SceneManager`. Any scene calls `SceneSwitcher::Request(GameScene::X)`; `SceneManager` checks `SceneSwitcher::hasPending` each frame after `DrawGui()`. No scene header needs to `#include "SceneManager.h"`.

### Level scripting system

`LevelDirector` owns the master level timeline. It merges any number of `LevelScript` chunks into one sorted `vector<LevelEvent>`, pre-loads every mesh they reference at startup, fires events as `levelTime` advances, scrolls active environment chunks left, and culls them when off-screen. It does **not** touch enemies, bullets, the player, or save data.

`LevelScript` is an abstract base — one `GetEvents()` override per concrete subclass returning a list of `LevelEvent`s with **local** timestamps (starting at 0). `LevelDirector::AddScript(script, timeOffset)` shifts all local times to absolute time on the master timeline, enabling seamless level-zone transitions.

**`LevelDirector` callback wiring** — `SceneMuntasir::OnCreate()` registers four lambdas after construction, one per event category:
- `SetPhaseCallback(fn)` — called on `PHASE_CHANGE`; sets `currentPhase`
- `SetBot01Callback(fn)` — called on `SPAWN_BOT01_GROUP` / `SPAWN_BOT01_SHIELDED`; signature `(count, spawnInterval, isShielded)` → `Bot01::TriggerWave`
- `SetBot02Callback(fn)` — called on `SPAWN_BOT02`; triggers the Bot02 pair spawn
- `SetAsteroidCallback(fn)` — called on `SET_ASTEROID_RATE`; signature `(largeInterval, smallInterval)`

Warp events use **pop-flags** instead of callbacks — polled each frame in `Update()`:
```cpp
if (levelDirector->PopWarpEnterRequest()) environment->TriggerWarpEnter(10.0f);
```

**`LevelDirector::Reset()`** — rewinds the timeline to t=0 without reloading meshes. Used by Try Again to replay the level from the start.

Currently registered scripts in `SceneMuntasir::OnCreate()`:
- **`Level01Script`** — offset 0 (starts immediately)
- **`Level02Script`** — offset 180 s (3 min in; Level02's first chunk enters exactly as Level01's last exits)

**To add a new environment chunk:** export an OBJ from Blender into `meshes/`, then add one `LevelEvent` of type `SPAWN_ENV_CHUNK` inside the relevant `LevelXXScript.cpp`. No other file changes are required.

**`EventType` values** defined in `EventType.h`:
- `SPAWN_ENV_CHUNK` — spawns a mesh that scrolls left at `scrollSpeed` units/second.
- `WARP_ENTER` — entry warp: opens at peak 40× speed, smoothly decelerates to normal (use at zone start).
- `WARP_EXIT` — exit warp: starts at normal speed, smoothly accelerates to peak 40× (use at zone end).
- `WARP_FULL` — full 3-phase cinematic warp: ramp-up → hold → ramp-down. Also fired by Q hold-to-warp.
- `PHASE_CHANGE` — fires `phaseCallback(phaseId)` in `SceneMuntasir`, advancing enemy progression.
- `SPAWN_BOT01_GROUP` — triggers a standard Bot01 wave. `scale` = bot count, `scrollSpeed` = seconds between individual spawns.
- `SPAWN_BOT01_SHIELDED` — triggers a shielded Bot01 wave. `scale` = bot count; bots spawn sequentially with `scrollSpeed` as the delay between each.
- `SPAWN_BOT02` — spawns the Bot02 pair (always 2, top and bottom).
- `SET_ASTEROID_RATE` — changes spawn density mid-level. `scale` = large interval (s), `scrollSpeed` = small interval (s).

### Phase-based enemy progression

`SceneMuntasir` holds a `currentPhase` int. `LevelDirector::SetPhaseCallback()` is called once in `OnCreate()` to wire up a lambda that sets `currentPhase`. Phase gates are declared as `PHASE_CHANGE` events in the level script:

| Phase | Trigger time | Active enemies |
|-------|-------------|----------------|
| 1 | t=0 (implicit) | Asteroids only |
| 2 | t=40 s | + Bot01 waves |
| 3 | t=115 s | Bot02 intro — Bot01 and asteroids pause |
| 4 | t=140 s | All enemies simultaneously |

`SceneMuntasir::Update()` gates each enemy's `Update()` and spawn calls behind `currentPhase` checks. To add a new phase or adjust timing, only `Level01Script.cpp` needs to change.

### Enemy class hierarchy

`Enemy` (`Enemy.h`) is an **abstract base class** — it is not a monolithic class managing all pools. It provides the shared `debris` vector, `SpawnHitDebris()` / `SpawnKillDebris()` helpers, and the virtual interface (`Update`, `Render`, `OnDestroy`, `Reset`). The three concrete subclasses are each owned as a separate raw pointer in `SceneMuntasir`:

- **`Asteroid`** — manages two parallel pools: large asteroids (6 HP) and small asteroids (3 HP). Both share one mesh. Exposes `GetAsteroidPositions()` / `GetSmallAsteroidPositions()` for collision. `DamageAsteroid()` / `DamageSmallAsteroid()` return `true` on kill. `PushAsteroid()` / `PushSmallAsteroid()` apply knockback velocity (exponential decay). Debris spawns on hit and on kill.
- **`Bot01`** — wave-based enemy (10 HP). Two-phase AI: flies straight on entry until within 7 world-units of the player's X, then steers toward the player's Y. Three wave types driven by `Bot01WaveType` enum — `STANDARD` (timer-spaced spawns), `PINCER` (simultaneous top+bottom pair), `SHIELDED` (sequential shielded bots, one per `bot01SpawnInterval`). `TriggerWave(type, count, interval)` is the entry point called by `LevelDirector`'s bot01 callback. Shielded bots activate a Fresnel shield bubble when a missile is nearby and stand off at `kStandoffX` instead of closing in. Per-instance `bot01HitTimers` drive a white-flash-on-hit effect. `PushX()` / `PushY()` apply impulse knockback (X uses a separate `bot01XKnockbackVels` vector that decays independently so it doesn't fight the chase spring). `DamageBot01()` returns `true` on kill.
- **`Bot02`** — mini-boss enemy (20 HP). Hovers at a fixed world-X position (`kHoverX = 6.0f`) and oscillates vertically. Max 2 active at a time. Has its own projectile pool (`GetBulletPositions()` / `GetBulletVelocities()`) and fires at the player every `kFireInterval = 3.0f` seconds. Four mesh parts: body, cockpit, fin, thrust — plus a fragment mesh (asteroid shape) and a dedicated bullet mesh. Bot02 bullets must be handled separately in collision detection. `MissileTargetType` covers `ASTEROID`, `BOT01`, and `BOT02` — homing missiles prioritise Bot02 first. `PushBot02()` applies velocity-based knockback (decays via `expf(-9*dt)`).

### Game objects in SceneMuntasir

- **`Player`** — four mesh components: ship body, cockpit, attachment, and thrust flame. Handles WASD movement with velocity/friction physics, health/lives, Z-roll on W/S (5° intentional wobble — do not change without asking), shield activation with Fresnel rim animation, and state-restore setters used by the save/load system. Shield collision is elliptical: X half-axis 1.05, Y half-axis 0.75 world units. Shield recharge uses tiered penalty rates locked in at deactivation: < 80% charge used → 1.0× (fast), ≥ 80% → 0.5× (medium penalty), ≥ 90% or fully expired → 0.15× (heavy penalty).
- **`Bullet`** — two pools: straight laser shots and homing missiles. Missiles use proportional-navigation (PN) guidance (`missileNavigationGain`) with a brief straight-flight launch phase before homing kicks in. Three-phase speed profile: launch burst → decelerate to cruise speed → terminal sprint once within `missileTerminalRange`. Missiles are culled by `missileMaxLifetime` timeout — they do **not** die when leaving the screen. Re-acquires nearest target if the locked one is destroyed mid-flight. Also upgrades target mid-flight if a higher-priority tier appears. Target acquisition priority: Bot02 first, then Bot01, then large asteroids.
- **`Environment`** — ImGui-drawn starfield scrolling background. Supports `SPACE` and `WATER` environment types. During warp, stars streak into blue-white lines scaled by `warpSpeed`.

**Impact knockback system:** all three enemy types support physics-based knockback applied at collision time. Bot01 uses `PushX(index, impulse)` / `PushY(index, impulse)`; Bot02 uses `PushBot02(index, dx, dy)`; Asteroid uses `PushAsteroid(index, dx, dy)` / `PushSmallAsteroid(index, dx, dy)`. All accumulate into per-instance knockback velocity vectors that decay via `expf(-8 or -9 × deltaTime)` each frame, independent of the enemy's normal movement logic. Missile impacts derive direction from `GetMissileVelocities()[m]` normalized; bullet impacts use a fixed +X direction since lasers travel horizontally. Missile forces are roughly 2–4× larger than bullet forces.

Collision detection lives in `SceneMuntasir::Update()` — all shapes are **ellipses**, not circles. Each enemy type uses different half-axis constants. The method iterates bullet / missile / shield position vectors against enemy position vectors and calls the appropriate `DamageX()` / `RemoveX()` by index.

**Score values:** large asteroid kill = 50 pts, small asteroid kill = 25 pts, Bot01 kill = 100 pts, Bot02 kill = 300 pts (bullet, missile, or ricochet). Shield kills score at reduced rates (10/5 pts). Shards dropped on kill: large asteroid = 3, small = 2, Bot01 = 5 (7 from missile kill), Bot01 missile non-kill hit = 2, Bot02 bullet kill = 8, Bot02 missile kill = 10, Bot02 ricochet kill = 8.

### World coordinate space

All game objects are placed at `Z = GameConst::kWorldZ` (= -10). Player and enemies move through world space; the camera is stationary.

`GameConst::ComputeWorldBounds(aspect)` calculates `kWorldBoundX`, `kWorldBoundY`, `kSpawnX`, and `kCullX` at runtime based on the active FOV and aspect ratio — call it from `OnCreate()` and `OnVideoChanged()` whenever aspect changes. Approximate values for the default 48° FOV at 16:9: player bounded at X ±11, Y ±6; enemies spawn at X ≈ +15 and are culled at X ≈ -15.

The `totalTime` counter in `Bot01` is only used for wave-progression persistence via `SaveData`. Firing the laser or missile applies a negative-X recoil impulse to the player.

### Warp system

Three warp modes, all using **smoothstep** (`3p² - 2p³`) for zero-derivative transitions:

| Mode | Behavior |
|------|----------|
| `WARP_ENTER` | Opens at peak 40× speed, decelerates to normal |
| `WARP_EXIT` | Starts at normal, accelerates to peak 40× |
| `WARP_FULL` | Ramp-up (30%) → hold (40%) → ramp-down (30%) |

During any warp: all enemies and bullets **fully paused** (`!warping` gate in `Update()`); player movement dampened to 35%. After warp ends: player speed eases back to 100% over 2.5 seconds via `postWarpTimer`. During `WARP_FULL` only: level geometry and enemies hidden in `Render()`; HUD hidden in `DrawGui()`.

**Q hold-to-warp:** hold Q for 3 seconds → fires `WARP_FULL` (10 s duration). Tracked via `f11Held` + `f11HoldTimer` in `SceneMuntasir`; charge bar shown in HUD (WARP DRIVE section).

Warp events fired from `LevelDirector` use pop-flags (`PopWarpEnterRequest()` etc.) polled in `SceneMuntasir::Update()`.

### RPG shard system

Enemies drop `Shard` structs (pos, vel, spin) when killed. Shards drift toward the player when within `kMagnetRadius` (2.4 units) using inverse-linear pull, and are collected within `kCollectRadius` (0.5 units). On death the player drops a `DroppedShard` pile at their last position; the pile pulses via `ShardBeacon` and can be recovered once per death within a 1.2-unit radius. Dying again before recovering = old shards lost permanently. `shardCount` is persisted via `SaveData`.

### Save / Load system

`SaveData` (`SaveData.h/.cpp`) is a global singleton (`SaveData::current`). Two separate file types:

- **`profile_<name>.dat`** — per-profile plain-text key-value file. Stores: profile name, shard count, high score, full mid-session state (health, lives, score, player position, wave timer, lost shard pile), and audio volume prefs. Profile discovery uses MSVC `<io.h>` (`_findfirst` / `_findnext`) — Windows-only. `SaveData::DeleteProfile()` deletes the file from disk.
- **`settings.dat`** — machine-level settings (resolution index, fullscreen, vsync mode, volume). Written by `SaveMachineSettings()` / read by `LoadMachineSettings()`. `SceneManager` uses this exclusively for video changes so they never touch profile files.

`SceneMuntasir` auto-saves every 10 seconds (`kAutoSaveInterval`) and saves explicitly on quit and scene exit via `SaveGame()`. New save fields don't break old saves — unknown keys are skipped and defaults apply. If you rename a key, add a migration read of the old name in `LoadProfile()`.

### Camera

Fixed perspective camera set once in `SceneMuntasir::OnCreate()` and rebuilt only on resolution change:
```cpp
viewMatrix       = MMath::lookAt(Vec3(0, 0, kCameraZ), Vec3(0, 0, -1), Vec3(0, 1, 0));
projectionMatrix = MMath::perspective(48.0f, aspect, 0.1f, 100.0f);
```

| Property | Value |
|----------|-------|
| FOV | 48° vertical (telephoto — same visible area as old 70° but less corner distortion) |
| Camera Z (`kCameraZ`) | 5.7 |
| Near / Far clip | 0.1 / 100.0 |

The `viewPos` uniform in `Render()` must always match `kCameraZ` for correct specular highlights. To audition a different FOV, use the F10 debug window — live slider, auto-calculates camera Z. Bake any final choice into `GameConst::kCameraZ` in `GameConstants.h`.

### Rendering pipeline

Single shader pair: `shaders/alphaWingVert.glsl` + `shaders/alphaWingFrag.glsl`.

The fragment shader has two modes selected by the `emissive` uniform:
- `emissive == 0` → Phong lighting (ambient + diffuse + specular).
- `emissive > 0.5` → Fresnel rim effect (transparent at center, opaque at silhouette edge). Used for the shield bubble; back faces are culled by the caller so only the outer surface renders. No extra uniforms are required beyond `color`, `lightPos`, and `viewPos`.

Uniforms are cached by name in `Shader`'s `unordered_map<string, GLuint>` and retrieved via `GetUniformID()`.

`RenderBackground()` runs before `Render()` each frame. It clears to black and draws the nebula gradient via GL scissor. `Environment` (the starfield) draws via `ImGui::GetBackgroundDrawList()` inside `Render()` — called after all 3D objects so the stars sit on top as small foreground dots.

### Light position constraint

`lightPos.Y` must stay ≥ 50 in `SceneMuntasir::Render()`. Moving the light closer causes a visible gradient color shift on the ship mesh.

### HUD and ImGui

`DrawGui()` calls helpers in order:

```
DrawGui()
    ├── debugOverlay->Draw()         — top-right, F9 toggle (Minimal / Detailed)
    ├── Camera debug window          — top-center, F10 toggle
    ├── [early return if fullWarp]   — hides everything below during WARP_FULL
    ├── DrawHUD()                    — top-left, always visible
    ├── DrawPauseMenu()              — center, ESC toggle
    └── DrawGameOver()               — center, on game over
```

`DebugOverlay` (`DebugOverlay.h/.cpp`) shows FPS, CPU%, GPU% (via Windows PDH), and RAM% in Minimal (text) or Detailed (text + 30 px scrolling waveform, 128-sample ring buffer) modes. GPU monitoring gracefully shows "N/A" if the PDH counter is unavailable.

**ImGui rules for this project:**
- `NoDecoration` blocks `AlwaysAutoResize` — never combine them.
- `GetContentRegionAvail()` is unreliable inside `AlwaysAutoResize` windows — use fixed pixel widths.
- All widgets in the same window that share a type (e.g. `ProgressBar`, `PlotLines`) need unique IDs: `"##bar1"`, `"##bar2"`.
- Every `PushStyleColor(n)` needs `PopStyleColor(n)` with the same count.

### Audio

Two audio paths exist in parallel:

1. **Per-scene SDL3 streams** — `SceneMuntasir` holds dedicated `SDL_AudioStream*` pointers: `bgmPlayer` (music), `sfxPlayer` (general SFX), `sfxLaserHitStream` (rapid-fire hit sounds), `hoverStream` (UI hover at 35% of sfx volume `kHoverStreamGain`). All open at `GameConst::kAudioSampleRate` (44100 Hz) stereo S16. `Sound::Play()` queues a WAV into the given stream. For rapid-fire SFX: call `SDL_ClearAudioStream(stream)` **before** every `Play()` — this flushes queued audio so each hit plays immediately instead of stacking up.
2. **`SoundManager`** — a pooled manager with one BGM stream and 12 SFX pipes (`PIPE1`–`PIPE12`), at 48000 Hz. Present in the codebase but not yet wired into `SceneMuntasir`; it is the intended replacement for per-scene streams. Note the sample rate difference (48000 vs 44100) when integrating.

Music and SFX volumes are stored in `SaveData` and applied via ImGui sliders in `DrawGui()`.

Video settings (resolution, fullscreen, vsync) follow a **pending/apply** pattern: both `SceneTitle` and `SceneMuntasir` copy `SaveData::current` values into local `pendingResIndex` / `pendingFullscreen` / `pendingVsync` fields when the settings panel opens. Changes only commit when the user presses **APPLY**. The `SaveData::kResolutionW/H` table has **7 presets**: 1280×720, 1600×900, 1920×1080, 2560×1440, 3840×2160 (4K), 5120×2160 (5K2K), 7680×2160 (8K2K).

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
