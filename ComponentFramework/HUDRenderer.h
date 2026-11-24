#ifndef HUDRENDERER_H
#define HUDRENDERER_H

#include <functional>
#include <SDL3/SDL_stdinc.h>

struct SDL_AudioStream;
class Player;
class Bullet;
class Environment;
class ShardBeacon;
class LevelDirector;
class Asteroid;
class Bot01;
class Bot02;
class Sound;
class FMOD_AudioManager;

// Draws the in-game HUD, pause menu, and game-over screen for SceneMuntasir.
// Owns only HUD-presentation state (pending settings, hover debounce, etc.);
// all game state is reached through Context pointers owned by SceneMuntasir.
class HUDRenderer {
public:
    struct Context {
        // Subsystems (not owned by HUDRenderer)
        Player*        player        = nullptr;
        Bullet*        bullet        = nullptr;
        Environment*   environment   = nullptr;
        ShardBeacon*   shardBeacon   = nullptr;
        LevelDirector* levelDirector = nullptr;
        Asteroid*      asteroid      = nullptr;
        Bot01*         bot01         = nullptr;
        Bot02*         bot02         = nullptr;

        // Audio (not owned by HUDRenderer)
        SDL_AudioStream* bgmPlayer         = nullptr;
        SDL_AudioStream* sfxPlayer         = nullptr;
        SDL_AudioStream* sfxLaserHitStream = nullptr;
        SDL_AudioStream* hoverStream       = nullptr;
        Sound*           uiClickSound      = nullptr;
        FMOD_AudioManager* fmod_manager     = nullptr;


        // Game-state fields — still owned by SceneMuntasir, HUDRenderer reads/writes via pointer
        bool*  gamePaused        = nullptr;
        bool*  gameOver          = nullptr;
        int*   score             = nullptr;
        int*   shardCount        = nullptr;
        int*   currentPhase      = nullptr;
        int*   prevLives         = nullptr;
        float* autoSaveTimer     = nullptr;
        float* beaconTriggerTime = nullptr;
        bool*  Q_Held            = nullptr;
        float* Q_HoldTimer       = nullptr;
        float* musicVolume       = nullptr;
        float* sfxVolume         = nullptr;

        // SceneMuntasir::shards is a private std::vector<Shard>; this callback lets
        // DrawGameOver's "Try Again" button clear it without exposing the Shard type.
        std::function<void()> clearShards;

        // Called by "Try Again" so the scene can put the boss section back to the start.
        std::function<void()> resetBoss;
    };

    HUDRenderer();
    ~HUDRenderer();

    void Init(const Context& context);

    void DrawHUD();
    void DrawPauseMenu();
    void DrawGameOver();

    // Boss health bar at the bottom right. health and healthTrail go from 0 to 1.
    // phase2At and phase3At are where the little phase marks go on the bar.
    void DrawBossBar(const char* name, float health, float healthTrail, float phase2At, float phase3At);

    // The WARNING before the boss. seconds is the time since it started, it makes the text flash.
    void DrawBossWarning(float seconds);

    // The red lane that shows where the boss will dash. y and fromX are world units, strength goes from 0 to 1.
    void DrawDashLane(float y, float fromX, float halfHeight, float strength);

    // A smaller WARNING at the top of the screen for one attack, for example the dash. seconds makes it flash.
    void DrawAttackWarning(const char* text, float seconds);

    // A black screen on top of everything. alpha goes from 0 (clear) to 1 (black).
    void DrawFade(float alpha);

    // Big text in the middle of the screen, for example when the boss is defeated.
    void DrawBanner(const char* title, const char* subtitle, float alpha);

private:
    void PlayHoverSound();

    Context ctx;

    // HUD-only presentation state (not needed anywhere outside these draw calls)
    bool pauseShowSettings;
    int  pendingResIndex;
    bool pendingFullscreen;
    int  pendingVsync;
    int  pendingTargetFPS;
    bool musicPaused;

    unsigned int lastHoveredId;
    Uint64       lastHoverTick;
};

#endif // HUDRENDERER_H
