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
- At startup, `SceneMuntasir` (the main game scene) loads automatically.

**Scene switching at runtime (keyboard):**
| Key | Scene |
|-----|-------|
| F1  | SceneSTG (placeholder/test) |
| F2  | SceneJA (Jacky's test scene) |
| F3  | SceneMuntasir (main game) |
| F12 | Toggle wireframe mode |
| ESC / Q | Quit |

**Gameplay controls:**
- WASD — move player
- Space / Left-click — fire laser
- Right-click — launch homing missile

There is no automated test suite. Verification is done by running the game.

## Architecture

### Entry point & lifecycle

`Main.cpp` → creates `SceneManager` → calls `Initialize()` then `Run()`.

`SceneManager` owns the game loop: polls SDL events, drives ImGui frames, then calls `currentScene->Update()` / `Render()` / `DrawGui()` each tick. Scene switches destroy the current scene and construct the new one via `BuildNewScene()`.

### Scene system

`Scene` (abstract base in `Scene.h`) defines the interface every scene must implement:
```
OnCreate() → Update(dt) → Render() → DrawGui() → OnDestroy()
```
All three concrete scenes (`SceneMuntasir`, `SceneSTG`, `SceneJA`) inherit from it. `SceneMuntasir` is the real game; the others are stubs/experiments.

### Game objects in SceneMuntasir

Each major game object follows the same `OnCreate / Update / Render / OnDestroy` pattern and is owned (raw pointer) by `SceneMuntasir`:

- **`Player`** — ship mesh + shield mesh. Handles WASD movement with velocity/friction physics, health/lives, and a shield with sweep-glow animation.
- **`Enemy`** — manages two enemy pools: `ASTEROID` (wave 1) and `BOT01` (wave 2). Exposes `GetAsteroidPositions()` / `GetBot01Positions()` vectors for collision checking in the scene.
- **`Bullet`** — two pools: straight laser shots and homing missiles. Missiles use proportional-navigation (PN) guidance with a brief straight-flight launch phase before homing kicks in. Re-acquires nearest target if the locked one is destroyed mid-flight.
- **`Environment`** — ImGui-drawn starfield scrolling background. Supports `SPACE` and `WATER` environment types (water adds jitter and a speed multiplier).

Collision detection lives in `SceneMuntasir::Update()` — it iterates bullet/missile position vectors against enemy position vectors and removes hits manually via `RemoveAt()` / `RemoveAsteroid()` etc.

### Rendering pipeline

Single shader pair: `shaders/alphaWingVert.glsl` + `shaders/alphaWingFrag.glsl`.

The fragment shader has two modes selected by the `emissive` uniform:
- `emissive == 0` → Phong lighting (ambient + diffuse + specular).
- `emissive > 0.5` → flat color with three animated crescent glints (shield effect). Glow points are passed as `shieldGlowPointA/B/C` uniforms computed in `Player::Update()` using `ComputeShieldGlowPoint()`.

Uniforms are cached by name in `Shader`'s `unordered_map<string, GLuint>` and retrieved via `GetUniformID()`.

The `Environment` starfield bypasses OpenGL entirely — it renders using ImGui's background draw list (`ImGui::GetBackgroundDrawList()`).

### Audio

SDL3 audio streams. Two separate `SDL_AudioStream` handles: one for music (looping), one for SFX (fire-and-forget). `Sound` loads a WAV file; `Sound::Play()` queues it into the given stream. Music and SFX volumes are tunable via ImGui sliders in `DrawGui()`.

### External libraries (vendored in-tree)

- **ImGui** — all `imgui*.cpp/.h` files are checked in directly (SDL3 + OpenGL3 backends).
- **tiny_obj_loader.h** — single-header OBJ loader.
- **GameDev** — external, installed at `C:\GameDev`. Provides `MATH::Vec3`, `Matrix4`, `Quaternion`, `MMath`, etc.

### Assets

All runtime assets are relative paths from the working directory (the project folder):
- `meshes/` — OBJ files (player ship, shield, bullet, missile, asteroid, bot01)
- `shaders/` — GLSL source files
- `audio/music/` and `audio/sfx/` — WAV files

### Debug logging

`Debug::Info()` / `Debug::Error()` / `Debug::FatalError()` write to `GameEngineLog.txt` in the working directory. The `__FILE__` / `__LINE__` macros are passed at every call site.
