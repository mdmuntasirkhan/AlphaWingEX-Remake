# ARCHITECTURE.md — Engine Structure

> How the engine is built and how data flows through it.
> Read this first before touching any code.
> For system-level detail, see [SYSTEMS.md](SYSTEMS.md).
> For step-by-step recipes, see [HOW_TO_ADD.md](HOW_TO_ADD.md).

---

## Table of Contents

1. [Tech Stack](#1-tech-stack)
2. [Entry Point and Startup](#2-entry-point-and-startup)
3. [The Game Loop](#3-the-game-loop)
4. [Scene Lifecycle](#4-scene-lifecycle)
5. [Scene Switching](#5-scene-switching)
6. [The Rendering Pipeline](#6-the-rendering-pipeline)
7. [World Coordinate System](#7-world-coordinate-system)
8. [Camera](#8-camera)

---

## 1. Tech Stack

| Layer | Technology |
|-------|-----------|
| Language | C++ (MSVC, x86) |
| Graphics API | OpenGL (via GLEW) |
| Window / Input / Audio | SDL3 |
| UI | Dear ImGui (SDL3 + OpenGL3 backends, vendored in-tree) |
| Math library | Custom GameDev (`C:\GameDev`) — `MATH::Vec3`, `Matrix4`, `MMath`, `Quaternion` |
| 3D model loading | `tiny_obj_loader.h` (single-header, vendored) |
| Build system | Visual Studio solution (`ComponentFramework.sln`) |

---

## 2. Entry Point and Startup

```
Main.cpp
  └── creates SceneManager
        ├── SceneManager::Initialize()   — creates window, OpenGL context, ImGui
        └── SceneManager::Run()          — starts the game loop
              └── (on exit) _CrtDumpMemoryLeaks() — checks for memory leaks
```

`SceneManager::Initialize()` builds the first scene (`SceneTitle`) and calls `currentScene->OnCreate()`.

---

## 3. The Game Loop

`SceneManager::Run()` drives everything. One iteration = one frame.

```
┌─────────────────────────────────────────────────────┐
│                  SceneManager::Run()                │
│                                                     │
│  1. Poll SDL events                                 │
│       └── currentScene->HandleEvents(event)        │
│                                                     │
│  2. currentScene->Update(deltaTime)                 │
│       └── game logic, AI, physics, collisions      │
│                                                     │
│  3. currentScene->RenderBackground()                │
│       └── GL clear, nebula gradient, scissor rect  │
│                                                     │
│  4. currentScene->Render()                          │
│       └── 3D draw calls (player, enemies, bullets) │
│                                                     │
│  5. ImGui new frame                                 │
│       └── currentScene->DrawGui()                  │
│             └── HUD, pause menu, debug windows     │
│                                                     │
│  6. Drain SceneSwitcher                             │
│       └── if pending: BuildNewScene() → OnDestroy  │
│                        old, OnCreate() new          │
│                                                     │
│  7. ImGui::Render() + SDL_GL_SwapWindow             │
│                                                     │
│  8. Frame cap delay (if vsync off)                  │
└─────────────────────────────────────────────────────┘
```

**Key rule:** Scene switches are always deferred to step 6. Never destroy a scene mid-frame.

---

## 4. Scene Lifecycle

Every scene inherits from the `Scene` abstract base class and implements six methods:

```
OnCreate()          — allocate resources, load meshes, set up audio
    │
    ▼
HandleEvents()      — called once per SDL event, every frame
    │
    ▼
Update(dt)          — game logic, called once per frame
    │
    ▼
RenderBackground()  — GL clear + background pass (nebula, color)
    │
    ▼
Render()            — 3D draw calls (const — no state changes)
    │
    ▼
DrawGui()           — ImGui windows (HUD, menus, debug)
    │
    ▼
OnDestroy()         — free all resources allocated in OnCreate()
```

**`Render()` is `const`** — it cannot modify scene state. All state changes happen in `Update()` or `DrawGui()`.

### Active scenes

| Scene class | When active | Purpose |
|-------------|-------------|---------|
| `SceneTitle` | On launch | Profile select, new game, leaderboard |
| `SceneMuntasir` | Main gameplay | The actual game |
| `SceneJA` | F2 shortcut | Teammate test scene |
| `SceneSTG` | F1 shortcut | Placeholder / experiment |

---

## 5. Scene Switching

Scene switches use a zero-dependency static relay called `SceneSwitcher` to avoid circular includes between scene headers and `SceneManager`.

```
Any scene calls:
    SceneSwitcher::Request(GameScene::MUN)
          │
          ▼
    SceneSwitcher::hasPending = true
    SceneSwitcher::pending    = GameScene::MUN
          │
          ▼ (after DrawGui() this frame)
    SceneManager checks SceneSwitcher::hasPending
          │
          ▼
    SceneManager::BuildNewScene()
          ├── currentScene->OnDestroy()
          ├── delete currentScene
          ├── currentScene = new SceneMuntasir()
          └── currentScene->OnCreate()
```

**Why this pattern?** Without it, a scene header would need to `#include "SceneManager.h"` and `SceneManager.h` would need to `#include` every scene — circular dependency. `SceneSwitcher` holds no scene or manager references; it is just a shared flag.

---

## 6. The Rendering Pipeline

Two separate passes per frame, always in this order:

### Pass 1 — `RenderBackground()`
- Calls `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`
- Draws the nebula gradient using GL scissor rect
- Runs **before** any 3D draw calls

### Pass 2 — `Render()`
- Binds the single shader pair (`alphaWingVert.glsl` + `alphaWingFrag.glsl`)
- Uploads `lightPos`, `viewPos`, `projectionMatrix`, `viewMatrix` uniforms once
- Draws each object by setting its `modelMatrix` + `color` uniforms and calling `mesh->Render()`
- Environment (starfield) draws **last** via `ImGui::GetBackgroundDrawList()` inside `Render()` — this puts star dots on top of 3D geometry as foreground

### Shader modes (controlled by `emissive` uniform)

| `emissive` value | Effect |
|-----------------|--------|
| `0.0` | Standard Phong shading (ambient + diffuse + specular) |
| `> 0.5` | Fresnel rim effect — used for the player shield bubble |

### Light position rule
`lightPos.Y` **must stay ≥ 50** in `SceneMuntasir::Render()`. Moving the light closer causes a visible gradient color shift on all ship meshes. Do not change without testing.

---

## 7. World Coordinate System

```
        +Y (up)
         │
         │        Camera at Z = +5.7
         │        looking toward -Z
         │
─────────┼─────────────────────── +X (right)
         │
         │        All game objects at Z = -10
         │        (kWorldZ = -10.0f)
        -Y
```

- **Camera:** fixed at `Z = +5.7`, looking toward `-Z`. Never moves.
- **Player and enemies:** always at `Z = -10` (`GameConst::kWorldZ`).
- **Enemies spawn** at `X = kSpawnX` (right edge, ~+15 units) and travel left.
- **Enemies are culled** when `X < kCullX` (left edge, ~-15 units).
- **Player is clamped** to `±kWorldBoundX` horizontally, `±kWorldBoundY` vertically.

All four boundary values (`kWorldBoundX/Y`, `kSpawnX`, `kCullX`) are computed at runtime by `GameConst::ComputeWorldBounds(aspect)` based on the current FOV and aspect ratio. This keeps them correct across all supported resolutions.

### Score and shard values (reference)

| Kill | Score | Shards dropped |
|------|-------|----------------|
| Large asteroid | 50 | 3 |
| Small asteroid | 25 | 2 |
| Bot01 | 100 | 5 (7 via missile) |
| Bot02 | 300 | 8–10 |

---

## 8. Camera

Fixed perspective camera, set once in `SceneMuntasir::OnCreate()` and rebuilt only on resolution change.

```cpp
viewMatrix       = MMath::lookAt(Vec3(0, 0, kCameraZ), Vec3(0, 0, -1), Vec3(0, 1, 0));
projectionMatrix = MMath::perspective(48.0f, aspect, 0.1f, 100.0f);
```

| Property | Value | Why |
|----------|-------|-----|
| FOV | 48° vertical | Telephoto — same visible area as old 70° but far less corner distortion |
| Camera Z | 5.7 | Pulled back to compensate for narrower FOV, maintains same world bounds |
| Near clip | 0.1 | |
| Far clip | 100.0 | |

**Debug:** Press **F10** in-game to open the camera FOV debug window — live slider from 25°–75° with auto-calculated camera Z. Use this to audition different focal lengths. When a final value is chosen, it gets baked into `GameConst::kCameraZ` in `GameConstants.h`.

**`viewPos` uniform** in `Render()` must always match the camera's actual Z position (`kCameraZ`) for correct specular highlights.
