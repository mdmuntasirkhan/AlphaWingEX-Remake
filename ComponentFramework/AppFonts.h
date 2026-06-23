#pragma once
#ifndef APPFONTS_H
#define APPFONTS_H

#include "imgui.h"

// ── App-wide font registry ────────────────────────────────────────────────────
// Loaded once in SceneManager::Initialize() from fonts/Exo2-Regular.ttf.
// All pointers are nullptr until that call completes.
// PushFont(nullptr) is legal in ImGui and silently uses the current default,
// so any scene that draws before init or without the font file is safe.
namespace AppFonts {
    extern ImFont* body;    // 14 px — replaces ImGui default for all general text
    extern ImFont* medium;  // 22 px — leaderboard header, subtitle
    extern ImFont* large;   // 32 px — timer MM:SS digits
    extern ImFont* title;   // 40 px — "ALPHA WING EX" banner
}

#endif // APPFONTS_H
