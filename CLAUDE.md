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

**Adding new source files:** Every new `.h` and `.cpp` must be registered in `ComponentFramework.vcxproj` as `<ClInclude>` and `<ClCompile>` entries respectively, or Visual Studio will not compile them. Copy an existing entry (e.g. Bot01) and change the filename.

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
- **`SceneTitle`** — profile selector / new-game flow. Three internal states (`MAIN`, `NEW_GAME_NAME`, `LOAD_SELECT`). The CREDITS button opens a `BeginPopupModal("##credits")` overlay — `OpenPopup` must be called from the root context (after all `ImGui::End()` calls) so the popup ID resolves correctly; the `showCredits` bool bridges the button handler and the popup call.
- **`SceneMuntasir`** — the real game.
- **`SceneSTG`**, **`SceneJA`** — stubs/experiments used by teammates.

`OnVideoChanged(int w, int h)` is a Scene virtual (default no-op) that `SceneManager` calls after applying a resolution change. `SceneMuntasir` overrides it to recompute the projection matrix and world bounds for the new aspect ratio.

### SceneSwitcher

`SceneSwitcher` (in `SceneSwitcher.h`) is a zero-dependency static struct that breaks the circular-include problem between scenes and `SceneManager`. No scene header needs to `#include "SceneManager.h"`.

Two entry points:
- `SceneSwitcher::Request(GameScene::X)` — deferred scene transition; `SceneManager` drains `hasPending` after `DrawGui()` each frame.
- `SceneSwitcher::RequestVideo(fullscreen, w, h, vsync)` — deferred video-mode change; `SceneManager` drains `hasVideoRequest` the same way. Both `SceneTitle` and `SceneMuntasir` call this when the user presses **APPLY** in settings.

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

> For the `LevelEvent` field semantics table and the full `EventType` reference, see [docs/HOW_TO_ADD.md](docs/HOW_TO_ADD.md).

### Phase-based enemy progression

`SceneMuntasir` holds a `currentPhase` int. Phase gates are declared as `PHASE_CHANGE` events in the level script:

| Phase | Trigger time | Active enemies |
|-------|-------------|----------------|
| 1 | t=0 (implicit) | Asteroids only |
| 2 | t=40 s | + Bot01 waves |
| 3 | t=115 s | Bot02 intro — Bot01 and asteroids pause |
| 4 | t=140 s | All enemies simultaneously |

`SceneMuntasir::Update()` gates each enemy's `Update()` and spawn calls behind `currentPhase` checks. To add a new phase or adjust timing, only `Level01Script.cpp` needs to change.

### Enemy class hierarchy

`Enemy` (`Enemy.h`) is an **abstract base class** — not a monolithic manager. It provides the shared `debris` vector, `SpawnHitDebris()` / `SpawnKillDebris()` helpers, and the virtual interface. The three concrete subclasses are each owned as a separate raw pointer in `SceneMuntasir`:

- **`Asteroid`** — manages two parallel pools: large and small asteroids. `DamageAsteroid()` / `DamageSmallAsteroid()` return `true` on kill.
- **`Bot01`** — wave-based enemy. Three wave types: `STANDARD`, `PINCER`, `SHIELDED`. `TriggerWave(type, count, interval)` is the entry point. Shielded bots activate a Fresnel shield bubble when a missile is nearby and stand off at `kStandoffX`. `PushX()` / `PushY()` use a separate `bot01XKnockbackVels` vector that decays independently so it doesn't fight the chase spring.
- **`Bot02`** — mini-boss. Hovers at `kHoverX = 6.0f`, oscillates vertically. Has its own projectile pool (`GetBulletPositions()` / `GetBulletVelocities()`) — Bot02 bullets must be handled **separately** in collision detection. `MissileTargetType` covers `ASTEROID`, `BOT01`, and `BOT02` — homing missiles prioritise Bot02 first.

**Impact knockback system:** all three enemy types accumulate per-instance knockback velocity vectors that decay via `expf(-8 or -9 × deltaTime)` each frame, independent of normal movement. Missile forces are ~2–4× larger than bullet forces; bullet impacts use a fixed +X direction.

Collision detection lives in `SceneMuntasir::Update()` — all shapes are **ellipses**, not circles.

### Game objects in SceneMuntasir

- **`Player`** — Z-roll on W/S is **5° intentional wobble — do not change without asking**. Shield collision is elliptical (X half-axis 1.05, Y half-axis 0.75). Shield recharge uses tiered penalty rates locked in at deactivation: < 80% used → 1.0× (fast), ≥ 80% → 0.5× (medium), ≥ 90% or fully expired → 0.15× (heavy).
- **`Bullet`** — two pools: lasers and homing missiles. Missiles use proportional-navigation (PN) guidance with a brief straight-flight launch phase. Three-phase speed profile: launch burst → cruise → terminal sprint. Missiles are culled by `missileMaxLifetime` timeout — they do **not** die when leaving the screen. Re-acquires nearest target if locked target is destroyed; upgrades to higher-priority tier mid-flight. Target priority: Bot02 → Bot01 → large asteroids.
- **`Environment`** — ImGui-drawn starfield. Supports `SPACE` and `WATER` types. During warp, stars streak into blue-white lines scaled by `warpSpeed`.

### World coordinate space

All game objects are placed at `Z = GameConst::kWorldZ` (= -10). Player and enemies move through world space; the camera is stationary.

`GameConst::ComputeWorldBounds(aspect)` calculates `kWorldBoundX`, `kWorldBoundY`, `kSpawnX`, and `kCullX` at runtime — call it from both `OnCreate()` and `OnVideoChanged()` whenever aspect changes.

### Warp system

Three warp modes, all using **smoothstep** (`3p² - 2p³`) for zero-derivative transitions:

| Mode | Behavior |
|------|----------|
| `WARP_ENTER` | Opens at peak 40× speed, decelerates to normal |
| `WARP_EXIT` | Starts at normal, accelerates to peak 40× |
| `WARP_FULL` | Ramp-up (30%) → hold (40%) → ramp-down (30%) |

During any warp: all enemies and bullets **fully paused** (`!warping` gate in `Update()`); player movement dampened to 35%. After warp ends: player speed eases back to 100% over 2.5 seconds via `postWarpTimer`. During `WARP_FULL` only: level geometry and enemies hidden in `Render()`; HUD hidden in `DrawGui()`.

**Q hold-to-warp:** hold Q for 3 seconds → fires `WARP_FULL` (10 s duration). Tracked via `Q_Held` + `Q_HoldTimer` in `SceneMuntasir`; charge bar shown in HUD (WARP DRIVE section).

Warp events fired from `LevelDirector` use pop-flags (`PopWarpEnterRequest()` etc.) polled in `SceneMuntasir::Update()`.

### RPG shard system

Enemies drop `Shard` structs (pos, vel, spin) when killed. Shards drift toward the player when within `kMagnetRadius` (2.4 units) using inverse-linear pull, and are collected within `kCollectRadius` (0.5 units). On death the player drops a `DroppedShard` pile at their last position; the pile pulses via `ShardBeacon` and can be recovered once per death within a 1.2-unit radius. Dying again before recovering = old shards lost permanently. `shardCount` is persisted via `SaveData`.

### Save / Load system

`SaveData` (`SaveData.h/.cpp`) is a global singleton (`SaveData::current`). Two separate file types:

- **`profile_<name>.dat`** — per-profile plain-text space-separated key-value file. Stores: profile name, shard count, high score, full mid-session state (health, lives, score, player position, wave timer, lost shard pile), and audio volume prefs. Profile discovery uses MSVC `<io.h>` (`_findfirst` / `_findnext`) — Windows-only.
- **`settings.dat`** — machine-level settings (resolution index, fullscreen, vsync mode, target FPS, volume). Written by `SaveMachineSettings()` / read by `LoadMachineSettings()`. `SceneManager` uses this exclusively for video changes so they never touch profile files.

`SceneMuntasir` auto-saves every 10 seconds (`kAutoSaveInterval`) and saves explicitly on quit and scene exit via `SaveGame()`. New save fields don't break old saves — unknown keys are skipped and defaults apply. **If you rename a key, add a migration read of the old name in `Load()`.**

Video settings follow a **pending/apply** pattern: both `SceneTitle` and `SceneMuntasir` copy `SaveData::current` values into local `pendingResIndex` / `pendingFullscreen` / `pendingVsync` / `pendingTargetFPS` fields when the settings panel opens. Changes only commit when the user presses **APPLY** → `SceneSwitcher::RequestVideo()`.

Note: a `save.dat` file may exist in the working directory from an earlier iteration of the save system — it is not read or written by the current code.

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
- `emissive > 0.5` → Fresnel rim effect. Used for the shield bubble; back faces are culled by the caller so only the outer surface renders.

Uniforms are cached by name in `Shader`'s `unordered_map<string, GLuint>` and retrieved via `GetUniformID()`.

`RenderBackground()` runs before `Render()` each frame. It clears to black and draws the nebula gradient via GL scissor. `Environment` (the starfield) draws via `ImGui::GetBackgroundDrawList()` inside `Render()` — called after all 3D objects so the stars sit on top as small foreground dots.

### Light position constraint

`lightPos.Y` must stay ≥ 50 in `SceneMuntasir::Render()`. Moving the light closer causes a visible gradient color shift on the ship mesh.

### HUD system

`SceneMuntasir::DrawGui()` draws the debug overlay and camera-debug window itself, then delegates everything else to `HUDRenderer`:

```
DrawGui()
    ├── debugOverlay->Draw()             — top-right, F9 toggle (Minimal / Detailed)
    ├── Camera debug window              — top-center, F10 toggle
    ├── [early return if fullWarp]       — hides everything below during WARP_FULL
    ├── hudRenderer->DrawHUD()           — top-left HUD, level timer, build version label
    ├── hudRenderer->DrawPauseMenu()     — center, ESC toggle
    └── hudRenderer->DrawGameOver()      — center, on game over
```

`HUDRenderer` (`HUDRenderer.h/.cpp`) owns all HUD, pause-menu, and game-over drawing — extracted out of `SceneMuntasir` so HUD code doesn't live inside the main scene class. It holds no game state itself: `SceneMuntasir::OnCreate()` builds a `HUDRenderer::Context` once, after every subsystem exists — subsystem pointers (`player`, `bullet`, `environment`, `shardBeacon`, `levelDirector`, `asteroid`, `bot01`, `bot02`), audio stream pointers, and raw pointers to the specific `SceneMuntasir` fields the HUD reads/writes (`score`, `shardCount`, `gamePaused`, `gameOver`, `currentPhase`, `prevLives`, `autoSaveTimer`, `beaconTriggerTime`, `Q_Held`, `Q_HoldTimer`, `musicVolume`, `sfxVolume`) — then calls `hudRenderer->Init(ctx)`. Presentation-only state nothing else touches (pending video settings, the pause menu's settings-panel toggle, hover-sound debounce) lives inside `HUDRenderer` itself. One field needs indirection instead of a raw pointer: `SceneMuntasir::shards` is a private nested-struct vector, so `Context` carries a `clearShards` callback (`[this]{ shards.clear(); }`) rather than exposing the vector's type across the header boundary.

`DebugOverlay` (`DebugOverlay.h/.cpp`) shows FPS, CPU%, GPU% (via Windows PDH), and RAM% in Minimal (text) or Detailed (text + 30 px scrolling waveform, 128-sample ring buffer) modes.

**Build version label** — transparent floating window anchored bottom-right `(x-10, y-10)` pivot `(1,1)`, drawn by `HUDRenderer` on `SceneMuntasir` and by `SceneTitle` itself. To bump the version, edit only `Version.h`.

**Font system (`Fonts`)** — `Fonts.h/.cpp` exposes four `ImFont*` pointers loaded in `SceneManager::Initialize()` from `fonts/Exo2-Regular.ttf`:

| Pointer | Size | Used for |
|---------|------|----------|
| `Fonts::body` | 14 px | Global default — all general HUD / menu text |
| `Fonts::medium` | 22 px | Leaderboard header, banner subtitle |
| `Fonts::large` | 32 px | Timer MM:SS digits |
| `Fonts::title` | 40 px | "ALPHA WING EX" banner |

Use `ImGui::PushFont(Fonts::large)` / `ImGui::PopFont()` — **never `SetWindowFontScale()`**, which stretches the texture and causes blur.

**HUD window size** — `HUDRenderer::DrawHUD()` uses `ImGui::SetNextWindowSize(ImVec2(318, 292), ImGuiCond_Always)`. Increase the `292` if new elements make it taller.

**ImGui rules for this project:**
- `NoDecoration` blocks `AlwaysAutoResize` — never combine them.
- `GetContentRegionAvail()` is unreliable inside `AlwaysAutoResize` windows — use fixed pixel widths.
- All widgets in the same window that share a type (e.g. `ProgressBar`, `PlotLines`) need unique IDs: `"##bar1"`, `"##bar2"`.
- Every `PushStyleColor(n)` needs `PopStyleColor(n)` with the same count.
- **Modal popups:** `OpenPopup("##id")` must be called from the **root context** (after all `ImGui::End()` calls), not from inside a window block. Use a bool flag set by the button, then call `OpenPopup` + `BeginPopupModal` together after all windows are closed.

### Audio

`SoundManager` (`SoundManager.h/.cpp`) owns all audio streams and devices: one `BGMStream` on its own dedicated device, plus a 12-pipe SFX pool (`PIPE1`–`PIPE12`) sharing a second device for mixing. Both devices and every stream use `GameConst::kAudioSampleRate` (44100 Hz) stereo S16, matching every WAV asset under `audio/`. `Sound::Play()` never re-derives format from the WAV it loads — it just writes raw bytes into whatever stream it's given — so **stream and asset sample rates must stay in sync**, or playback speed/pitch will be wrong.

`SceneMuntasir::OnCreate()` creates one `SoundManager` and borrows specific pipes from it via `GetBGMStream()` / `GetSFXPipe(index)`, storing them in its own `bgmPlayer`/`sfxPlayer`/`sfxLaserHitStream`/`hoverStream` fields. `SoundManager` is used purely as the stream/device *owner* — `SceneMuntasir`, `Player` (via `SetSFXStream()`), and `HUDRenderer` all call `Sound::Play()`, `SDL_SetAudioStreamGain()`, `SDL_ClearAudioStream()`, and `SDL_PauseAudioStreamDevice()`/`Resume()` directly on the borrowed pointers, exactly as if they owned them. Reserved pipes: `0` = general SFX (laser shoot, explosion, missile hit, shield hit, and `Player`'s shield SFX all share this one stream, so simultaneous sounds queue/serialize rather than overlap), `1` = laser-hit (cleared via `SDL_ClearAudioStream` before every play, for rapid-fire responsiveness), `2` = UI hover clicks (independent gain, `kHoverStreamGain` = 35% of SFX volume). Pipes 3–11 are unused/spare.

`SoundManager`'s own higher-level API (`playSoundAt()` auto-pick, `UpdateBGM()`, `adjustSFXVolume()`, etc.) is **not** used by `SceneMuntasir`: auto-picking a free pipe would make today's serialized SFX overlap instead, and calling `UpdateBGM()` every frame would re-trigger the whole track from the start. Adopting that API later is a deliberate behavior change, not just a wiring change.

### Version system

`Version.h` (header-only, no `.cpp`) exposes `AppVersion::kMajor`, `kMinor`, `kPatch`, `kBuild` as `static constexpr int`. This is the **only file to edit when releasing**.

### External libraries (vendored in-tree)

- **ImGui** — all `imgui*.cpp/.h` files are checked in directly (SDL3 + OpenGL3 backends).
- **tiny_obj_loader.h** — single-header OBJ loader.
- **GameDev** — external, installed at `C:\GameDev`. Provides `MATH::Vec3`, `Matrix4`, `Quaternion`, `MMath`, etc.
- **`Body.h`** — a physics body stub included for future use; currently unused.

### Assets

All runtime assets are relative paths from the working directory (the project folder):
- `meshes/` — OBJ files
- `shaders/` — GLSL source files
- `audio/music/` and `audio/sfx/` — WAV files
- `fonts/` — `Exo2-Regular.ttf` (Exo 2, Google Fonts, OFL). Not bundled in the repo — must be present on disk.
- `profile_<name>.dat` — plain-text save files, one per player profile
- `settings.dat` — machine-level video/audio settings

### Debug logging

`Debug::Info()` / `Debug::Error()` / `Debug::FatalError()` write to `GameEngineLog.txt` in the working directory. The `__FILE__` / `__LINE__` macros are passed at every call site.
