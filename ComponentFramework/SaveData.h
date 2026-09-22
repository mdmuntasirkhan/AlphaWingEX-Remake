#ifndef SAVEDATA_H
#define SAVEDATA_H

#include <string>
#include <vector>
#include <utility>  // std::pair

struct SaveData {
    // Profile
    std::string profileName = "Player";

    // Persistent RPG stats
    int   shardCount  = 0;
    int   highScore   = 0;

    // Auto saved continuously
    float health   = 100.0f;
    int   lives    = 3;
    int   score    = 0;
    float posX     = 0.0f;
    float posY     = 0.0f;
    float waveTime = 0.0f;   // enemy wave progression timer

    // Lost shard beacon (FromSoftware-style: drops on game over, recoverable next run)
    bool  hasLostShards    = false;
    float lostShardPosX    = 0.0f;
    float lostShardPosY    = 0.0f;
    int   lostShardCount   = 0;
    float deathLevelTime   = 0.0f;  // level time at which the beacon activates (0 = immediate)

    // Audio preferences
    float musicVolume = 0.10f;
    float sfxVolume   = 0.05f;

    // Video preferences (machine-level, not profile-specific)
    bool fullscreen      = false;
    int  resolutionIndex = 0;    // index into kResolutionW / kResolutionH
    int  vsyncMode       = -1;   // -1=adaptive sync, 1=vsync, 0=uncapped
    int  targetFPS       = 240;  // frame cap when vsync is off; 0 = uncapped

    static SaveData current;

    // Resolution table — shared by scenes and SceneManager
    static constexpr int kResolutionCount = 7;
    static constexpr const char* kResolutionLabels[kResolutionCount] = {
        "1280 x 720",
        "1600 x 900",
        "1920 x 1080",
        "2560 x 1440",
        "3840 x 2160  (4K)",
        "5120 x 2160  (5K2K)",
        "7680 x 2160  (8K2K)"
    };
    static constexpr int kResolutionW[kResolutionCount] = { 1280, 1600, 1920, 2560, 3840, 5120, 7680 };
    static constexpr int kResolutionH[kResolutionCount] = {  720,  900, 1080, 1440, 2160, 2160, 2160 };

    std::string GetFilePath() const { return "profile_" + profileName + ".dat"; }

    bool FileExists()                      const;
    bool Save()                            const;
    bool Load(const std::string& name);
    void Reset();

    // Stored in settings.dat
    void SaveMachineSettings() const;
    void LoadMachineSettings();

    // Lists all profile
    static std::vector<std::string> GetProfileList();

    // Returns all profiles with their high scores, sorted descending
    static std::vector<std::pair<std::string, int>> GetLeaderboard();

    // Permanently deletes the save file for the given profile name
    static bool DeleteProfile(const std::string& name);

    static constexpr int kMaxProfiles = 10;
};

#endif // SAVEDATA_H
