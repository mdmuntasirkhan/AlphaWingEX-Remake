# SYSTEMS.md — How Each System Works

> Deep dives into each major system.
> Read [ARCHITECTURE.md](ARCHITECTURE.md) first for the big picture.
> For step-by-step recipes, see [HOW_TO_ADD.md](HOW_TO_ADD.md).

---

## Table of Contents

1. [Level Scripting System](#1-level-scripting-system)
2. [Phase-Based Enemy Progression](#2-phase-based-enemy-progression)
3. [Enemy System](#3-enemy-system)
4. [Player System](#4-player-system)
5. [Bullet and Missile System](#5-bullet-and-missile-system)
6. [Collision Detection](#6-collision-detection)
7. [Warp System](#7-warp-system)
8. [RPG Shard System](#8-rpg-shard-system)
9. [Save and Load System](#9-save-and-load-system)
10. [Audio System](#10-audio-system)
11. [HUD and ImGui System](#11-hud-and-imgui-system)
12. [Debug Overlay](#12-debug-overlay)

---

## 1. Level Scripting System

### Overview

The level is entirely data-driven. No enemy spawning logic lives in `SceneMuntasir::Update()` directly — instead, timed events fire from a master timeline managed by `LevelDirector`.

### How the timeline works

```
LevelScript (abstract base)
    └── GetEvents() → returns list of LevelEvent structs with LOCAL timestamps

LevelDirector::AddScript(script, timeOffset)
    └── shifts all local times by offset → adds to master timeline
    └── pre-loads all mesh files referenced by SPAWN_ENV_CHUNK events

LevelDirector::Update(deltaTime)
    └── advances levelTime
    └── fires any events whose time ≤ levelTime (walks sorted vector)
    └── scrolls active environment chunks left, culls off-screen ones

SceneMuntasir::Update()
    └── calls levelDirector->Update(dt)
    └── polls pop-flags: PopWarpEnterRequest(), PopWarpExitRequest(), etc.
```

### Currently registered scripts

| Script | Offset | Active from | Content |
|--------|--------|-------------|---------|
| `Level01Script` | 0s | t=0 | Opening warp, asteroids, Bot01 waves, Bot02 intro |
| `Level02Script` | 180s | t=3min | New zone — content TBD |

### Event types

| EventType | What fires | Key fields |
|-----------|-----------|------------|
| `SPAWN_ENV_CHUNK` | Spawns a scrolling OBJ mesh | position, meshFile, color, scale, scrollSpeed |
| `PHASE_CHANGE` | Advances `currentPhase` | phaseId |
| `SPAWN_BOT01_GROUP` | Triggers a standard Bot01 wave | scale=count, scrollSpeed=interval |
| `SPAWN_BOT01_SHIELDED` | Triggers a shielded Bot01 wave | scale=count, scrollSpeed=interval |
| `SPAWN_BOT02` | Spawns the Bot02 pair | — |
| `SET_ASTEROID_RATE` | Changes asteroid spawn density | scale=large interval, scrollSpeed=small interval |
| `WARP_ENTER` | Triggers entry warp (peak → normal) | — |
| `WARP_EXIT` | Triggers exit warp (normal → peak) | — |
| `WARP_FULL` | Triggers full 3-phase cinematic warp | — |

### Callback wiring

Four lambdas are registered in `SceneMuntasir::OnCreate()`:

```cpp
levelDirector->SetPhaseCallback([this](int id)           { currentPhase = id; });
levelDirector->SetBot01Callback([this](int n, float t, bool s) { bot01->TriggerWave(...); });
levelDirector->SetBot02Callback([this]()                 { bot02->Spawn(); });
levelDirector->SetAsteroidCallback([this](float l, float s)   { asteroid->SetRates(l, s); });
```

Warp events use pop-flags instead of callbacks (polled each frame in `Update()`):
```cpp
if (levelDirector->PopWarpEnterRequest()) environment->TriggerWarpEnter(10.0f);
```

### Reset

`LevelDirector::Reset()` rewinds `levelTime` to 0 and clears active chunks without reloading meshes. Called by Try Again to replay the level from the start.

---

## 2. Phase-Based Enemy Progression

`currentPhase` is an int in `SceneMuntasir`. It gates which enemies are active in `Update()`.

| Phase | Triggered at | What is active |
|-------|-------------|----------------|
| 1 | t=0 (implicit) | Asteroids only |
| 2 | t=40s | + Bot01 waves |
| 3 | t=115s | Bot02 intro — Bot01 and asteroids paused |
| 4 | t=140s | All enemies simultaneously |

In `SceneMuntasir::Update()`:
```cpp
if (currentPhase >= 2 && !warping) bot01->Update(...);
if (currentPhase == 3 || currentPhase >= 4) bot02->Update(...);
```

To add a new phase: add a `PHASE_CHANGE` event to the level script with the new `phaseId`, then add a gate in `Update()` for the new enemy using `currentPhase >= N`.

---

## 3. Enemy System

### Class hierarchy

```
Enemy (abstract base — Enemy.h)
    ├── shared debris vector
    ├── SpawnHitDebris() / SpawnKillDebris() helpers
    └── virtual interface: Update, Render, OnDestroy, Reset

    ├── Asteroid   — manages two pools: large (6 HP) and small (3 HP)
    ├── Bot01      — wave-based enemies (10 HP), two-phase AI
    └── Bot02      — mini-boss (20 HP), hovers + fires projectiles
```

Each concrete enemy is owned as a **raw pointer** in `SceneMuntasir` and allocated/destroyed in `OnCreate()` / `OnDestroy()`.

### Asteroid

- Two pools: large and small asteroids, both using one shared mesh.
- Spawn rate controlled by `SET_ASTEROID_RATE` events (separate intervals for large and small).
- `DamageAsteroid(index, amount)` / `DamageSmallAsteroid(index, amount)` return `true` on kill.
- `PushAsteroid()` / `PushSmallAsteroid()` apply knockback with exponential velocity decay.

### Bot01

- **Wave types:** `STANDARD` (timer-spaced), `PINCER` (top+bottom pair), `SHIELDED` (one per interval).
- **Two-phase AI:** flies straight on entry until within 7 units of player X, then steers toward player Y.
- **Shielded variant:** activates a Fresnel shield bubble when a missile is nearby; stands off at `kStandoffX` instead of closing in.
- **Hit flash:** per-instance `bot01HitTimers` drive a white-flash effect on damage (`kHitFlashDuration = 0.18s`).
- **Knockback:** `PushX()` / `PushY()` use separate velocity vectors that decay independently via `expf(-8 * dt)`.

### Bot02

- Max 2 active at once (top and bottom of screen).
- Hovers at fixed world X (`kHoverX = 6.0f`), oscillates vertically.
- Fires projectiles at the player every `kFireInterval = 3.0f` seconds.
- Four mesh parts: body, cockpit, fin, thrust — plus a bullet mesh and fragment mesh.
- Bot02 bullets are a separate collision pool from player bullets — handle them separately.
- Homing missiles prioritize Bot02 first, then Bot01, then large asteroids.

### Knockback system (all enemies)

All three enemy types support physics knockback applied at collision time:
- Accumulated into per-instance velocity vectors.
- Decays via `expf(-8 or -9 × deltaTime)` each frame.
- Independent from the enemy's normal movement logic.
- Missile impacts: force ~2–4× larger than bullet impacts.
- Bullet impacts: fixed +X direction (lasers travel horizontally).
- Missile direction: derived from `GetMissileVelocities()[m]` normalized.

---

## 4. Player System

### Movement

WASD input → velocity accumulation → friction-based deceleration each frame. Hard-clamped to `±kWorldBoundX` / `±kWorldBoundY`.

**Z-roll on W/S:** 5° intentional wobble. This is a design decision — do not change without asking.

**Recoil:** firing laser or missile applies a negative-X impulse to the player.

### Health and lives

- 100 HP. Damage reduces HP; reaching 0 loses a life and restores HP.
- Lives lost → `prevLives` tracked to detect the moment of death for beacon/shard system.

### Shield

- Activated by `E` key. Active for up to 10 seconds.
- Shield collision is **elliptical**: X half-axis 1.05, Y half-axis 0.75 world units.
- Fresnel rim shader effect while active (emissive > 0.5 in fragment shader).
- Recharge uses **tiered penalty rates** locked in at deactivation:
  - < 80% charge used → 1.0× rate (fast recharge)
  - ≥ 80% used → 0.5× rate (medium penalty)
  - ≥ 90% used or fully expired → 0.15× rate (heavy penalty)

### State persistence

`Player` exposes setters used by the save/load system to restore exact mid-session state (position, health, lives).

---

## 5. Bullet and Missile System

All projectiles live in `Bullet.h` / `Bullet.cpp`. Two separate pools.

### Laser (straight shot)

- Spawned at player position, travels in +X direction at fixed speed.
- Culled when `X > kSpawnX`.
- Collision vs all enemy types in `SceneMuntasir::Update()`.

### Homing Missile

- **Target acquisition priority:** Bot02 first → Bot01 → large asteroids.
- **Re-acquisition:** if locked target is destroyed, finds nearest remaining target. Upgrades target mid-flight if a higher-priority type appears.
- **Guidance:** proportional-navigation (PN) with `missileNavigationGain`.
- **Three-phase speed profile:**
  1. Launch burst — straight flight, homing not yet active
  2. Cruise — decelerate to cruise speed, homing active
  3. Terminal sprint — accelerates when within `missileTerminalRange`
- **Lifetime:** culled by `missileMaxLifetime` timeout (does NOT die when leaving screen).
- **Supply:** finite count, auto-reloads over time. Reload progress shown in HUD.

---

## 6. Collision Detection

All collision shapes are **ellipses**, not circles. Lives in `SceneMuntasir::Update()`.

The method iterates bullet/missile/shield position vectors against enemy position vectors. Each enemy type uses different half-axis constants. On hit:
- Calls `DamageX(index, amount)` — returns `true` on kill.
- Calls `PushX(index, impulse)` for knockback.
- Spawns debris via `SpawnHitDebris()` or `SpawnKillDebris()`.
- Applies score and shard drops on kill.

Shield collision is checked separately — kills score at reduced rates (10/5 pts per enemy type).

---

## 7. Warp System

### Three modes

| Mode | Behavior | Trigger |
|------|----------|---------|
| `WARP_ENTER` | Opens at peak 40× speed, smoothly decelerates to normal | Level start, `WARP_ENTER` event |
| `WARP_EXIT` | Starts at normal, smoothly accelerates to peak 40× | Level end, `WARP_EXIT` event |
| `WARP_FULL` | Ramp-up (30%) → hold (40%) → ramp-down (30%) | F11 hold-to-fire, `WARP_FULL` event |

### Speed curve

All transitions use **smoothstep** (`3p² - 2p³`) — zero derivative at both endpoints, no pop or snap at start or end.

```
ENTER: warpSpeed = 40 - smoothstep(t) * 39     (40 → 1)
EXIT:  warpSpeed = 1  + smoothstep(t) * 39     (1 → 40)
FULL:  t < 0.3  → ramp up    (1 → 40)
       t < 0.7  → hold       (40)
       t ≥ 0.7  → ramp down  (40 → 1)
```

### What pauses during warp

- All enemies (Bot01, Bot02, Asteroid), all bullets: **fully paused** (`!warping` gate).
- Player movement: **dampened to 35%** during warp.
- After warp ends: player speed eases back to 100% over **2.5 seconds** using smoothstep (`postWarpTimer` + `prevWarping` tracking in `SceneMuntasir`).

### Visual effects during warp

- Stars streak into blue-white lines at `warpSpeed * star.speed` scroll rate.
- Streak length: `(warpSpeed - 3) * size * 6`, capped at 120px.
- During `WARP_FULL` only: enemies, bullets, level geometry hidden in `Render()`. Player ship stays visible.
- HUD hidden during `WARP_FULL`.

### F11 hold-to-warp

Hold F11 for 3 seconds → fires `WARP_FULL` (10s duration).
- `f11Held` + `f11HoldTimer` tracked in `SceneMuntasir`.
- Charge bar in HUD (WARP DRIVE section, bottom of the HUD window).
- Release before 3s cancels the charge.

---

## 8. RPG Shard System

Shards are the game's RPG currency — dropped on enemy kills, lost on death, recoverable from crash site.

### Drop and collect

- Enemies drop `Shard` structs (position, velocity, spin angle, spin speed) on kill.
- Within `kMagnetRadius` (2.4 units): shards drift toward player using inverse-linear pull.
- Within `kCollectRadius` (0.5 units): shard collected, added to `shardCount`.

### Death and beacon

- On player death: all shards are dropped at the death position as a `DroppedShard` pile.
- The pile pulses visually (beacon mesh) and can be recovered once per death within 1.2-unit radius.
- `shardCount` is persisted to the save file.

### FromSoftware-style risk/reward

- Dying again before recovering the beacon = old shards lost permanently.
- `deathLevelTime` is saved so the beacon only appears at the correct point in the next run's timeline (not immediately on respawn).

---

## 9. Save and Load System

`SaveData` is a global singleton (`SaveData::current`). Two separate file formats.

### Profile file — `profile_<name>.dat`

Plain-text key=value, one per line. Stores:
- Profile name, shard count, high score
- Full mid-session state: health, lives, score, player position, wave timer, lost shard pile
- Audio volume preferences

Written by `SaveData::SaveProfile()`, read by `LoadProfile()`.
`SceneMuntasir` auto-saves every 10 seconds (`kAutoSaveInterval`) and saves explicitly on quit and scene exit.

### Settings file — `settings.dat`

Machine-level settings only: resolution index, fullscreen, vsync mode, frame cap, volume defaults.
Written by `SaveMachineSettings()`, read by `LoadMachineSettings()`.

### Safety

- New fields added to the save file don't break old saves — unknown keys are skipped, defaults are kept.
- If you rename a key, add a migration line in `LoadProfile()` to read the old name and write to the new field.
- Profile discovery uses `_findfirst` / `_findnext` (Windows-only `<io.h>`) — Windows platform dependency.

### Pending video settings pattern

Both `SceneTitle` and `SceneMuntasir` use a **pending/apply** pattern for video settings:
- Local `pendingResIndex` / `pendingFullscreen` / `pendingVsync` fields copy from `SaveData` when the settings panel opens.
- Nothing is applied until the user presses **APPLY**.
- On APPLY: `SceneSwitcher::RequestVideo()` is called → `SceneManager` drains it after `DrawGui()` and applies the change.

---

## 10. Audio System

Two audio paths exist in parallel.

### Per-scene SDL3 streams (currently in use)

`SceneMuntasir` holds dedicated `SDL_AudioStream*` pointers:

| Stream | Purpose |
|--------|---------|
| `bgmPlayer` | Background music (looping WAV) |
| `sfxPlayer` | General SFX (queued) |
| `sfxLaserHitStream` | Rapid-fire hit sounds (cleared before each play) |
| `hoverStream` | UI hover sound (35% of sfx volume) |

All streams open at 44100 Hz stereo S16 (`kAudioSampleRate`).

**For rapid-fire SFX** (sounds that fire many times per second like laser hits): always call `SDL_ClearAudioStream(stream)` before `Sound::Play()`. Without the clear, sounds stack up in the buffer and play delayed.

### SoundManager (present but not wired)

A pooled manager (`SoundManager`) with one BGM stream and 12 SFX pipes (`PIPE1`–`PIPE12`) exists in the codebase at 48000 Hz. It is the intended replacement for per-scene streams but is not yet wired into `SceneMuntasir`. When integrating it, note the sample rate difference (48000 vs 44100).

---

## 11. HUD and ImGui System

### Window structure

`DrawGui()` in `SceneMuntasir` calls three helper methods in order:

```
DrawGui()
    ├── debugOverlay->Draw()          — top-right, F9 toggle
    ├── Camera debug window           — top-center, F10 toggle
    ├── [early return if fullWarp]    — hides everything below during WARP_FULL
    ├── DrawHUD()                     — top-left, always visible
    ├── DrawPauseMenu()               — center, ESC toggle
    └── DrawGameOver()                — center, on game over
```

### HUD layout (`DrawHUD()`)

Fixed window anchored top-left at (20, 20), size 318×292. Contains:
- Pilot name, score, high score
- Shard count
- Missile slots (colored pip bars) + vertical reload bar
- Lives display
- HP bar (color shifts green → yellow → red)
- Shield charge bar (cyan → orange → red) + threshold tick marks
- Shard beacon status (when active)
- **Warp Drive charge bar** (always visible; idle = animated blue→violet gradient; charging = filling cyan)
- ESC hint

### ImGui rules for this project

- `NoDecoration` flag includes `NoResize` — it will **block** `AlwaysAutoResize`. Never combine them.
- `GetContentRegionAvail()` is unreliable inside `AlwaysAutoResize` windows during layout passes — use fixed pixel widths.
- All `PlotLines` / `ProgressBar` widgets in the same window must have **unique IDs**: `"##fps"`, `"##cpu"`, etc.
- `PushStyleColor(n)` must always be followed by `PopStyleColor(n)` with the same count.
- Right-align text without clipping: `SetCursorPosX(GetWindowWidth() - CalcTextSize(str).x - GetStyle().WindowPadding.x)`

---

## 12. Debug Overlay

`DebugOverlay` (`DebugOverlay.h/.cpp`) — toggled with **F9**, cycles: hidden → Minimal → Detailed.

### What it shows

| Stat | Color | Source |
|------|-------|--------|
| FPS | Green | Frame delta time |
| CPU % | Cyan | `GetSystemTimes()` delta |
| GPU % | Purple | Windows PDH `\GPU Engine(*engtype_3D)\Utilization Percentage` |
| RAM % | Orange | `GetProcessMemoryInfo` + `GlobalMemoryStatusEx` |

### Modes

- **Minimal** — four text lines only, compact
- **Detailed** — text + 30px scrolling waveform graph per stat

128-sample ring buffer per stat (0.5s poll interval = ~64 seconds of history). Anchored top-right via `SetNextWindowPos(DisplaySize.x - 10, 10, Always, pivot(1,0))`.

GPU monitoring uses PDH which is cross-vendor (works on NVIDIA, AMD, Intel). Shows "N/A" gracefully if the counter is unavailable.
