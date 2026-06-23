# HOW_TO_ADD.md — Developer Recipes

> Step-by-step guides for adding new content to the engine.
> For system explanations, see [SYSTEMS.md](SYSTEMS.md).
> For engine structure, see [ARCHITECTURE.md](ARCHITECTURE.md).

---

## Table of Contents

1. [Add an Enemy Wave to a Level](#1-add-an-enemy-wave-to-a-level)
2. [Add a New Event Type](#2-add-a-new-event-type)
3. [Add a New Enemy Class](#3-add-a-new-enemy-class)
4. [Add a New Weapon or Projectile](#4-add-a-new-weapon-or-projectile)
5. [Add a New HUD Element](#5-add-a-new-hud-element)
6. [Add a New Save Field](#6-add-a-new-save-field)
7. [Add a New Audio File](#7-add-a-new-audio-file)
8. [Add a New Environment Mesh](#8-add-a-new-environment-mesh)
9. [Add a New Level Zone](#9-add-a-new-level-zone)
10. [Add a New Phase](#10-add-a-new-phase)
11. [Add a Warp to a Level](#11-add-a-warp-to-a-level)
12. [Add a New Debug Key or Window](#12-add-a-new-debug-key-or-window)
13. [Add a New Scene](#13-add-a-new-scene)

---

## 1. Add an Enemy Wave to a Level

**Files to touch:** `Level01Script.cpp` (or whichever level zone)

Open the script file and add a line inside `GetEvents()`:

```cpp
// Standard Bot01 wave — 4 bots, one every 8 seconds
{ 60.0f, EventType::SPAWN_BOT01_GROUP, {}, nullptr, {}, 4.0f, 8.0f, 0 },

// Shielded Bot01 wave — 2 shielded bots, 5 seconds apart
{ 90.0f, EventType::SPAWN_BOT01_SHIELDED, {}, nullptr, {}, 2.0f, 5.0f, 0 },

// Bot02 pair (always spawns top + bottom)
{ 120.0f, EventType::SPAWN_BOT02, {}, nullptr, {}, 0.0f, 0.0f, 0 },

// Change asteroid density (large every 2s, small every 1.5s)
{ 30.0f, EventType::SET_ASTEROID_RATE, {}, nullptr, {}, 2.0f, 1.5f, 0 },
```

**Event field reference:**

```
{ time,  EventType,  position,  meshFile,  color,  scale,  scrollSpeed,  phaseId }
```

| Field | Meaning |
|-------|---------|
| `time` | Seconds from level start |
| `scale` | BOT01: bot count. ASTEROID_RATE: large spawn interval |
| `scrollSpeed` | BOT01: seconds between spawns. ASTEROID_RATE: small spawn interval |
| `phaseId` | PHASE_CHANGE only: new phase number |
| `position` | ENV_CHUNK only: world spawn position |
| `meshFile` | ENV_CHUNK only: path to OBJ |
| `color` | ENV_CHUNK only: RGB tint |

Times are **local** to the script (start from 0). `LevelDirector::AddScript(script, offset)` shifts them to absolute master timeline time automatically. No need to keep events in time order — `LevelDirector` sorts them.

---

## 2. Add a New Event Type

Use when existing types don't cover a new gameplay trigger (e.g. boss spawn, cutscene, powerup).

**Files to touch:**
1. `EventType.h`
2. `LevelDirector.h`
3. `LevelDirector.cpp`
4. `SceneMuntasir.cpp` — `OnCreate()`
5. Your `LevelXXScript.cpp`

**Step 1 — Declare in `EventType.h`:**
```cpp
enum class EventType {
    // ... existing types ...
    SPAWN_BOSS,    // ← new
};
```

**Step 2 — Add pop-flag to `LevelDirector.h`:**
```cpp
// private:
bool bossSpawnRequested;
// public:
bool PopBossRequest();
```

**Step 3 — Wire in `LevelDirector.cpp`:**

Constructor: `bossSpawnRequested{ false }`

`FireEvent()`:
```cpp
else if (e.type == EventType::SPAWN_BOSS) { bossSpawnRequested = true; }
```

Pop method:
```cpp
bool LevelDirector::PopBossRequest() {
    if (!bossSpawnRequested) return false;
    bossSpawnRequested = false;
    return true;
}
```

**Step 4 — Poll in `SceneMuntasir::Update()`:**
```cpp
if (levelDirector->PopBossRequest()) boss->Spawn();
```

**Step 5 — Add to level script:**
```cpp
{ 200.0f, EventType::SPAWN_BOSS, {}, nullptr, {}, 0.0f, 0.0f, 0 },
```

---

## 3. Add a New Enemy Class

**Files to touch:**
1. `NewEnemy.h` + `NewEnemy.cpp` (create)
2. `ComponentFramework.vcxproj` (register)
3. `SceneMuntasir.h` + `SceneMuntasir.cpp`
4. `EventType.h` + `LevelDirector` (spawn event — see recipe 2)

**Step 1 — Create `NewEnemy.h`:**
```cpp
#pragma once
#include "Enemy.h"

class NewEnemy : public Enemy {
public:
    NewEnemy();
    ~NewEnemy();
    bool OnCreate() override;
    void OnDestroy() override;
    void Update(float dt, Vec3 playerPos) override;
    void Render(Shader* shader, const Matrix4& proj, const Matrix4& view) const override;
    void Reset() override;

    bool DamageNewEnemy(int index, int amount);  // returns true on kill
    std::vector<Vec3> GetPositions() const;

private:
    Mesh* mesh;
    std::vector<Vec3>   positions;
    std::vector<int>    health;
    std::vector<float>  hitTimers;
    static constexpr int   kMaxHP = 15;
    static constexpr float kScale = 1.0f;
};
```

**Step 2 — Implement using Bot01/Bot02 as reference:**
- Knockback decay: `expf(-8 * deltaTime)`
- Hit flash: use `GameConst::kHitFlashDuration`
- Debris: call `SpawnKillDebris()` on kill (inherited from `Enemy`)

**Step 3 — Register in `ComponentFramework.vcxproj`:**
Copy the `<ClInclude>` and `<ClCompile>` entries from Bot01 and change the filename.

**Step 4 — Wire into `SceneMuntasir`:**

`SceneMuntasir.h`:
```cpp
NewEnemy* newEnemy;
```

`OnCreate()`:
```cpp
newEnemy = new NewEnemy();
newEnemy->OnCreate();
```

`OnDestroy()`:
```cpp
newEnemy->OnDestroy();
delete newEnemy;
```

`Update()` — gate behind phase and warp:
```cpp
if (currentPhase >= 3 && !warping)
    newEnemy->Update(deltaTime, player->GetPosition());
```

`Render()`:
```cpp
if (!fullWarp)
    newEnemy->Render(shader, projectionMatrix, viewMatrix);
```

**Step 5 — Add collision detection in `Update()`** following the elliptical collision pattern from Bot01.

**Step 6 — Add spawn event** (recipe 2) and level script entry (recipe 1).

---

## 4. Add a New Weapon or Projectile

All projectiles live in `Bullet.h` / `Bullet.cpp`.

**Files to touch:** `Bullet.h`, `Bullet.cpp`, `SceneMuntasir.cpp`

**Step 1 — Add a new pool in `Bullet.h`:**
```cpp
std::vector<Vec3> spreadPositions;
std::vector<Vec3> spreadVelocities;

void SpawnSpread(Vec3 origin);
std::vector<Vec3> GetSpreadPositions() const;
void RemoveSpread(int index);
```

**Step 2 — Implement in `Bullet.cpp`:**

Spawn:
```cpp
void Bullet::SpawnSpread(Vec3 origin) {
    for (int i = -1; i <= 1; i++) {
        float angle = i * 10.0f * GameConst::kPi / 180.0f;
        spreadPositions.push_back(origin);
        spreadVelocities.push_back(Vec3(cosf(angle) * 20.0f, sinf(angle) * 5.0f, 0.0f));
    }
}
```

Update (inside `Bullet::Update()`):
```cpp
for (int i = (int)spreadPositions.size() - 1; i >= 0; i--) {
    spreadPositions[i] += spreadVelocities[i] * deltaTime;
    if (spreadPositions[i].x > GameConst::kSpawnX) {
        spreadPositions.erase(spreadPositions.begin() + i);
        spreadVelocities.erase(spreadVelocities.begin() + i);
    }
}
```

**Step 3 — Fire from `HandleEvents()` in `SceneMuntasir.cpp`:**
```cpp
case SDL_SCANCODE_Q:
    bullet->SpawnSpread(player->GetPosition());
    break;
```

**Step 4 — Add collision in `SceneMuntasir::Update()`** using the same ellipse loop pattern as lasers.

---

## 5. Add a New HUD Element

**File to touch:** `SceneMuntasir.cpp` → `DrawHUD()`

Find `void SceneMuntasir::DrawHUD()` and add your element inside the `ImGui::Begin("##hud")` / `ImGui::End()` block.

If your content makes the window taller, increase the height:
```cpp
ImGui::SetNextWindowSize(ImVec2(318, 292), ImGuiCond_Always);
//                                   ↑ increase this number
```

**Common patterns:**

```cpp
// Colored text
ImGui::TextColored(ImVec4(R, G, B, A), "LABEL: %d", value);

// Full-width progress bar
ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(R, G, B, 1.0f));
ImGui::ProgressBar(fraction_0_to_1, ImVec2(-1.0f, 16.0f), "");
ImGui::PopStyleColor();

// Right-aligned text (avoids border clipping)
const char* str = "HOLD F11";
ImGui::SameLine(0, 0);
ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize(str).x - ImGui::GetStyle().WindowPadding.x);
ImGui::TextDisabled("%s", str);

// Separator line
ImGui::Separator();
```

**ImGui rules:**
- Every `PushStyleColor(n)` needs `PopStyleColor(n)` with the same count.
- Multiple progress bars in the same window need unique IDs: `"##bar1"`, `"##bar2"`.
- `NoDecoration` blocks `AlwaysAutoResize` — never combine them.

---

## 6. Add a New Save Field

**Files to touch:** `SaveData.h`, `SaveData.cpp`

**Step 1 — Add the field to the struct in `SaveData.h`:**
```cpp
struct SaveData {
    // ... existing fields ...
    int newField = 0;    // ← give it a sensible default
};
```

**Step 2 — Write in `SaveProfile()`:**
```cpp
file << "newField=" << current.newField << "\n";
```

**Step 3 — Read in `LoadProfile()`:**
```cpp
else if (key == "newField") current.newField = std::stoi(value);
```

Old save files that don't have this key will simply skip it and use the default. Nothing breaks.

**If you rename an existing key**, add a migration line to handle old files:
```cpp
else if (key == "oldName") current.newName = std::stoi(value);
```

**For machine-level settings** (resolution, vsync, frame cap), use `SaveMachineSettings()` / `LoadMachineSettings()` targeting `settings.dat` instead — same pattern.

---

## 7. Add a New Audio File

**Files to touch:** `SceneMuntasir.h`, `SceneMuntasir.cpp`

**Step 1 — Place the WAV file:**
```
audio/sfx/your_sound.wav      ← sound effects
audio/music/your_track.wav    ← background music
```

**Step 2 — Declare in `SceneMuntasir.h`:**
```cpp
Sound* sfxNewSound;
```

**Step 3 — Load in `OnCreate()`:**
```cpp
sfxNewSound = new Sound();
sfxNewSound->Load("audio/sfx/your_sound.wav");
```

**Step 4 — Play when the event occurs:**
```cpp
// Standard SFX — queues onto the shared stream
sfxNewSound->Play(sfxPlayer);

// Rapid-fire SFX — clear before playing to avoid stacking
SDL_ClearAudioStream(sfxLaserHitStream);   // or create a dedicated stream
sfxNewSound->Play(sfxLaserHitStream);
```

**Step 5 — Delete in `OnDestroy()`:**
```cpp
delete sfxNewSound;
```

Volume is driven by `SaveData::current.sfxVolume` (0.0–1.0), set via the pause menu slider.

---

## 8. Add a New Environment Mesh

**Files to touch:** `meshes/` folder, `Level01Script.cpp` (or your level)

**Step 1 — Export from Blender as OBJ** → save to `ComponentFramework/meshes/yourchunk.obj`

**Step 2 — Add one event line in the level script:**
```cpp
{
    5.0f,                              // time (seconds from level start)
    EventType::SPAWN_ENV_CHUNK,
    Vec3(15.0f, 0.0f, -10.0f),        // spawn position (X=15=right edge, Z must be -10)
    "meshes/yourchunk.obj",            // path to OBJ
    Vec3(0.6f, 0.6f, 0.7f),           // RGB color tint
    1.0f,                              // scale (1.0 = natural OBJ size)
    1.5f,                              // scroll speed (units/s leftward)
    0
},
```

That's it. `LevelDirector` pre-loads the mesh at startup, spawns it at the right time, scrolls it left, and culls it automatically.

**Tips:**
- `position.y`: +5 = top, 0 = center, -5 = bottom of screen
- `scrollSpeed` 1.5 matches the asteroid field feel
- Stack multiple chunks at different times and Y offsets to build dense zones

---

## 9. Add a New Level Zone

**Files to touch:**
1. `Level03Script.h` + `Level03Script.cpp` (create)
2. `ComponentFramework.vcxproj` (register)
3. `SceneMuntasir.cpp` — `OnCreate()`

**Step 1 — Create `Level03Script.h`:**
```cpp
#pragma once
#include "LevelScript.h"

class Level03Script : public LevelScript {
public:
    std::vector<LevelEvent> GetEvents() const override;
};
```

**Step 2 — Create `Level03Script.cpp`:**
```cpp
#include "Level03Script.h"
#include "EventType.h"

std::vector<LevelEvent> Level03Script::GetEvents() const {
    return {
        { 0.0f,  EventType::WARP_ENTER,        {}, nullptr, {}, 0.0f, 0.0f, 0 },
        { 0.0f,  EventType::SET_ASTEROID_RATE, {}, nullptr, {}, 2.0f, 1.0f, 0 },
        { 20.0f, EventType::SPAWN_BOT01_GROUP, {}, nullptr, {}, 4.0f, 6.0f, 0 },
        // ... all times LOCAL starting from 0 ...
    };
}
```

**Step 3 — Register in `SceneMuntasir::OnCreate()`:**
```cpp
#include "Level03Script.h"

// Offset = when Level03 should start on the master timeline
// Level02 starts at 180s. If Level02 lasts ~3 minutes, Level03 offset = 360s
levelDirector->AddScript(new Level03Script(), 360.0f);
```

The offset controls absolute start time. Level01 is at 0, Level02 at 180. Calculate your offset by adding the previous zone's offset + its last event time.

---

## 10. Add a New Phase

**Files to touch:** `Level01Script.cpp` (add event), `SceneMuntasir.cpp` (gate new enemy)

**Step 1 — Add `PHASE_CHANGE` to the level script:**
```cpp
{ 200.0f, EventType::PHASE_CHANGE, {}, nullptr, {}, 0.0f, 0.0f, 5 },
//                                                              ↑ new phase ID
```

**Step 2 — Gate the new enemy in `SceneMuntasir::Update()`:**
```cpp
if (currentPhase >= 5 && !warping)
    newEnemy->Update(deltaTime, player->GetPosition());
```

The `phaseCallback` lambda registered in `OnCreate()` handles setting `currentPhase` automatically — you do not need to touch it.

---

## 11. Add a Warp to a Level

**File to touch:** Your `LevelXXScript.cpp`

Add the appropriate event. All times are local to the script:

```cpp
// Scene opens at peak speed, decelerates to normal (use at start of a zone)
{ 0.0f, EventType::WARP_ENTER, {}, nullptr, {}, 0.0f, 0.0f, 0 },

// Normal speed, accelerates to peak (use at end of a zone before transition)
{ 170.0f, EventType::WARP_EXIT, {}, nullptr, {}, 0.0f, 0.0f, 0 },

// Full cinematic: ramp up → hold → ramp down
{ 50.0f, EventType::WARP_FULL, {}, nullptr, {}, 0.0f, 0.0f, 0 },
```

Warp duration is **always 10 seconds**, controlled by the `TriggerWarpXxx(10.0f)` calls in `SceneMuntasir::Update()`. To change it, edit those calls.

During warp: all enemies and bullets pause, player moves at 35% speed. After warp: player control eases back over 2.5 seconds.

---

## 12. Add a New Debug Key or Window

**File to touch:** `SceneMuntasir.h`, `SceneMuntasir.cpp`

**Step 1 — Declare a toggle in `SceneMuntasir.h`:**
```cpp
bool showMyDebugWindow;
```

**Step 2 — Initialize in the constructor:**
```cpp
showMyDebugWindow{ false },
```

**Step 3 — Add the key in `HandleEvents()` (inside the `SDL_EVENT_KEY_DOWN` switch):**
```cpp
case SDL_SCANCODE_F8:
    showMyDebugWindow = !showMyDebugWindow;
    break;
```

**Step 4 — Draw the window in `DrawGui()` (before the `IsFullWarp` early return if it should show during warp, after it if not):**
```cpp
if (showMyDebugWindow) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(10, io.DisplaySize.y - 10), ImGuiCond_Always, ImVec2(0, 1));
    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGui::Begin("##mydbg", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoSavedSettings);

    ImGui::TextColored(ImVec4(1, 1, 0, 1), "MY DEBUG");
    ImGui::Text("Value: %.2f", myValue);

    ImGui::End();
}
```

**Window pivot reference** (second arg to `SetNextWindowPos`):
- `ImVec2(0, 0)` = top-left pinned
- `ImVec2(1, 0)` = top-right pinned
- `ImVec2(0.5f, 0.5f)` = centered
- `ImVec2(0, 1)` = bottom-left pinned

---

## 13. Add a New Scene

**Files to touch:**
1. `NewScene.h` + `NewScene.cpp` (create)
2. `ComponentFramework.vcxproj` (register)
3. `SceneManager.h` — add to `GameScene` enum
4. `SceneManager.cpp` — add to `BuildNewScene()`
5. Anywhere you want to switch to it

**Step 1 — Create `NewScene.h`:**
```cpp
#pragma once
#include "Scene.h"

class NewScene : public Scene {
public:
    explicit NewScene();
    virtual ~NewScene();
    virtual bool OnCreate()    override;
    virtual void OnDestroy()   override;
    virtual void Update(float dt)                      override;
    virtual void RenderBackground()                    override;
    virtual void Render()                        const override;
    virtual void HandleEvents(const SDL_Event& e)      override;
    virtual void DrawGui()                             override;
};
```

**Step 2 — Add to `GameScene` enum in `SceneManager.h`:**
```cpp
enum class GameScene { TITLE, MUN, JA, STG, NEWSCENE };
```

**Step 3 — Add to `BuildNewScene()` in `SceneManager.cpp`:**
```cpp
case GameScene::NEWSCENE:
    currentScene = new NewScene();
    break;
```

**Step 4 — Switch to it from any scene:**
```cpp
SceneSwitcher::Request(GameScene::NEWSCENE);
```

Scene switches are deferred to after the current frame's `DrawGui()` completes — never mid-frame. The old scene's `OnDestroy()` is called first, then the new scene's `OnCreate()`.
