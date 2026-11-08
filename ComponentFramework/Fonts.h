#ifndef FONTS_H
#define FONTS_H

#include "imgui.h"

// Font Registry. Loaded once in SceneManager::Initialize() from fonts/Exo2-Regular.ttf

namespace Fonts {
    extern ImFont* body;    // 14 px — replaces ImGui default for all general text
    extern ImFont* medium;  // 22 px — leaderboard header, subtitle
    extern ImFont* large;   // 32 px — timer MM:SS digits
    extern ImFont* title;   // 40 px — "ALPHA WING EX" banner
}

#endif // FONTS_H
