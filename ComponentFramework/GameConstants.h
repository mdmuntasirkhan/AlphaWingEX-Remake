#pragma once
#ifndef GAMECONSTANTS_H
#define GAMECONSTANTS_H

// ============================================================
//  Shared compile-time constants for AlphaWingEX.
//
//  Put constants here only when the SAME value is used in more than
//  one class. Class-specific tuning values (HP, scale, AI parameters)
//  belong as `static constexpr` in their own class header instead.
//
//  Usage: #include "GameConstants.h"  →  GameConst::kPi
// ============================================================
namespace GameConst {

    // ── Math ────────────────────────────────────────────────
    static constexpr float kPi = 3.14159265f;

    // ── World space ─────────────────────────────────────────
    // All active game objects live at Z = kWorldZ.
    // Player is hard-clamped to ±kWorldBoundX / ±kWorldBoundY (70° FOV at Z=-10).
    // Enemies enter from kSpawnX (right edge) and cull at kCullX (left edge).
    static constexpr float kWorldZ      = -10.0f;
    static constexpr float kWorldBoundX =  11.0f;
    static constexpr float kWorldBoundY =   6.0f;
    static constexpr float kSpawnX      =  15.0f;
    static constexpr float kCullX       = -15.0f;

    // ── Audio ───────────────────────────────────────────────
    // SDL3 audio streams in every scene open at this sample rate.
    static constexpr int   kAudioSampleRate = 44100;
    // Hover sound plays at this fraction of the main SFX volume.
    static constexpr float kHoverStreamGain = 0.35f;

    // ── Visual feedback ─────────────────────────────────────
    // White-flash duration (seconds) when an enemy takes a hit. Shared by Bot01 and Bot02.
    static constexpr float kHitFlashDuration = 0.18f;
}

#endif // GAMECONSTANTS_H
