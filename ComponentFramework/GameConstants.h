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
    static constexpr float kWorldZ   = -10.0f;
    static constexpr float kCameraZ  =   5.7f;  // telephoto pull-back: 48° FOV, same visible area as old 70°@Z=0

    // Defaults match 48° vertical FOV, camera at Z=5.7, objects at Z=-10. Overwritten by ComputeWorldBounds().
    inline float kWorldBoundX =  11.0f;
    inline float kWorldBoundY =   6.0f;
    inline float kSpawnX      =  15.0f;
    inline float kCullX       = -15.0f;

    // Call this from SceneMuntasir::OnCreate() and OnVideoChanged() whenever aspect changes.
    // tan(24°) = half of 48° vertical FOV. Distance = kCameraZ - kWorldZ = 15.7 units.
    inline void ComputeWorldBounds(float aspect) {
        constexpr float kTanHalfFOV = 0.44523f;           // tan(24°) — half of 48° vertical FOV
        const float dist  = kCameraZ - kWorldZ;            // 15.7 units — camera to object plane
        const float halfY = kTanHalfFOV * dist;            // visible half-height (≈ 6.99, same as before)
        const float halfX = halfY * aspect;                 // visible half-width scales with aspect
        kWorldBoundX = halfX - 0.5f;
        kWorldBoundY = halfY + 0.1f;
        kSpawnX      =  halfX + 2.55f;
        kCullX       = -(halfX + 2.55f);
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
