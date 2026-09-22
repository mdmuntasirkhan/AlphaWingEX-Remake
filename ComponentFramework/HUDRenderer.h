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
    };

    HUDRenderer();
    ~HUDRenderer();

    void Init(const Context& context);

    void DrawHUD();
    void DrawPauseMenu();
    void DrawGameOver();

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
