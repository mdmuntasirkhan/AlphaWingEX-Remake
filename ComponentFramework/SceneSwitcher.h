#ifndef SCENESWITCHER_H
#define SCENESWITCHER_H

// Scenes request a transition by calling SceneSwitcher::Request().
// SceneManager drains the request after DrawGui() each frame.
// No circular includes: this header has zero game-class dependencies.

enum class GameScene { TITLE, MUN, STG, JA };

struct SceneSwitcher {
    // Scene transition
    static GameScene pending;
    static bool      hasPending;
    static void Request(GameScene s) { pending = s; hasPending = true; }

    // Video settings — SceneManager applies after DrawGui each frame
    static bool hasVideoRequest;
    static bool videoFullscreen;
    static int  videoWidth;
    static int  videoHeight;
    static int  videoVsync;   // -1=adaptive, 1=vsync, 0=uncapped
    static void RequestVideo(bool full, int w, int h, int vsync) {
        hasVideoRequest = true;
        videoFullscreen = full;
        videoWidth      = w;
        videoHeight     = h;
        videoVsync      = vsync;
    }
};

#endif // SCENESWITCHER_H
