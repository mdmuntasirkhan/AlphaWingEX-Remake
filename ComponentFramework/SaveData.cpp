#include "SaveData.h"
#include <fstream>
#include <string>

SaveData SaveData::current;

bool SaveData::FileExists(const char* path) const {
    std::ifstream f(path);
    return f.good();
}

bool SaveData::Save(const char* path) const {
    std::ofstream f(path);
    if (!f) return false;
    f << "shards "      << shardCount  << "\n"
      << "highscore "   << highScore   << "\n"
      << "musicvolume " << musicVolume << "\n"
      << "sfxvolume "   << sfxVolume   << "\n";
    return true;
}

bool SaveData::Load(const char* path) {
    std::ifstream f(path);
    if (!f) return false;
    std::string key;
    while (f >> key) {
        if      (key == "shards")      f >> shardCount;
        else if (key == "highscore")   f >> highScore;
        else if (key == "musicvolume") f >> musicVolume;
        else if (key == "sfxvolume")   f >> sfxVolume;
    }
    return true;
}

void SaveData::Reset() {
    shardCount  = 0;
    highScore   = 0;
    musicVolume = 0.10f;
    sfxVolume   = 0.05f;
}
