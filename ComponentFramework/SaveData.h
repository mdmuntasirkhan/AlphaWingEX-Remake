#pragma once
#ifndef SAVEDATA_H
#define SAVEDATA_H

#include <string>
#include <vector>

struct SaveData {
    // Profile
    std::string profileName = "Player";

    // Persistent RPG stats
    int   shardCount  = 0;
    int   highScore   = 0;

    // Full mid-session state (auto-saved continuously)
    float health   = 100.0f;
    int   lives    = 3;
    int   score    = 0;
    float posX     = 0.0f;
    float posY     = 0.0f;
    float waveTime = 0.0f;   // enemy wave progression timer

    // Lost shard pile state (persisted so reload restores the pile)
    bool  hasLostShards    = false;
    float lostShardPosX    = 0.0f;
    float lostShardPosY    = 0.0f;
    int   lostShardCount   = 0;

    // Audio preferences
    float musicVolume = 0.10f;
    float sfxVolume   = 0.05f;

    static SaveData current;

    std::string GetFilePath() const { return "profile_" + profileName + ".dat"; }

    bool FileExists()                      const;
    bool Save()                            const;
    bool Load(const std::string& name);
    void Reset();

    // Lists all profile names found on disk
    static std::vector<std::string> GetProfileList();
};

#endif
