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
    // kWorldBoundX/Y, kSpawnX, kCullX are runtime values updated by ComputeWorldBounds()
    // whenever the viewport aspect ratio changes, so all resolutions (16:9 through 32:9)
    // see correct player bounds and enemy spawn/cull edges automatically.
    static constexpr float kWorldZ = -10.0f;

    // Defaults match 70° vertical FOV at Z=-10 for 16:9. Overwritten by ComputeWorldBounds().
    inline float kWorldBoundX =  11.0f;
    inline float kWorldBoundY =   6.0f;
    inline float kSpawnX      =  15.0f;
    inline float kCullX       = -15.0f;

    // Call this from SceneMuntasir::OnCreate() and OnVideoChanged() whenever aspect changes.
    // tan(35°) ≈ 0.70021 is half of the fixed 70° vertical FOV at depth abs(kWorldZ).
    inline void ComputeWorldBounds(float aspect) {
        constexpr float kTan35 = 0.70021f;
        const float halfY = kTan35 * (-kWorldZ);   // visible half-height at Z=kWorldZ
        const float halfX = halfY * aspect;         // visible half-width scales with aspect
        kWorldBoundX = halfX - 1.45f;              // player stays ~1.45 units from screen edge
        kWorldBoundY = halfY - 1.0f;               // player stays ~1 unit from top/bottom
        kSpawnX      =  halfX + 2.55f;             // enemies enter 2.55 units off right edge
        kCullX       = -(halfX + 2.55f);           // enemies removed 2.55 units off left edge
    }

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
