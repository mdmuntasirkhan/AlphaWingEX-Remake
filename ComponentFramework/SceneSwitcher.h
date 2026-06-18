#pragma once
#ifndef SCENESWITCHER_H
#define SCENESWITCHER_H

// Scenes request a transition by calling SceneSwitcher::Request().
// SceneManager drains the request after DrawGui() each frame.
// No circular includes: this header has zero game-class dependencies.

enum class GameScene { TITLE, MUN, STG, JA };

struct SceneSwitcher {
    static GameScene pending;
    static bool      hasPending;
    static void Request(GameScene s) { pending = s; hasPending = true; }
};

#endif
