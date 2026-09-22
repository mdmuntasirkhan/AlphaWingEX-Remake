#ifndef GAMECONSTANTS_H
#define GAMECONSTANTS_H

// Shared compile time constants. Only values used across multiple classes live here.
// Class specific tuning (HP, scale, AI) stays as static constexpr in each class header.
namespace GameConst {

    // Math
    static constexpr float kPi = 3.14159265f;

    // World space
    // All active game objects live at Z = kWorldZ.
    // kCameraZ uses a telephoto pullback: 48 deg FOV gives the same visible area as the old 70 deg at Z=0.
    static constexpr float kWorldZ   = -10.0f;
    static constexpr float kCameraZ  =   5.7f;

    // Defaults match 48° vertical FOV, camera at Z=5.7, objects at Z=-10. Overwritten by ComputeWorldBounds().
    inline float kWorldBoundX =  11.0f;
    inline float kWorldBoundY =   6.0f;
    inline float kSpawnX      =  15.0f;
    inline float kCullX       = -15.0f;

    // Call this from SceneMuntasir::OnCreate() and OnVideoChanged() whenever aspect changes.
    // tan(24°) = half of 48° vertical FOV. Distance = kCameraZ - kWorldZ = 15.7 units.
    inline void ComputeWorldBounds(float aspect) {
        constexpr float kTanHalfFOV = 0.44523f;              // tan(24°) or half of 48° vertical FOV
        const float dist  = kCameraZ - kWorldZ;              // 15.7 units - camera to object plane
        const float halfY = kTanHalfFOV * dist;              // visible half height (≈ 6.99, same as before)
        const float halfX = halfY * aspect;                  // visible half width scales with aspect
        kWorldBoundX = halfX - 0.5f;
        kWorldBoundY = halfY + 0.1f;
        kSpawnX      =  halfX + 2.55f;
        kCullX       = -(halfX + 2.55f);
    }

    // Audio
    static constexpr int   kAudioSampleRate = 44100;
    static constexpr float kHoverStreamGain = 0.35f;         // hover sound plays at this fraction of SFX volume

    // Visual feedback
    // White-flash duration (seconds) when an enemy takes a hit. Shared by Bot01 and Bot02.
    static constexpr float kHitFlashDuration = 0.18f;
}

#endif // GAMECONSTANTS_H
