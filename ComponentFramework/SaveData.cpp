#include "SaveData.h"
#include <fstream>
#include <algorithm>   // std::sort
#include <cstdio>      // std::remove
#include <io.h>        // _findfirst / _findnext (MSVC)

SaveData SaveData::current;

bool SaveData::FileExists() const {
    std::ifstream f(GetFilePath());
    return f.good();
}

bool SaveData::Save() const {
    std::ofstream f(GetFilePath());
    if (!f) return false;
    f << "profile "        << profileName    << "\n"
      << "shards "         << shardCount     << "\n"
      << "highscore "      << highScore      << "\n"
      << "health "         << health         << "\n"
      << "lives "          << lives          << "\n"
      << "score "          << score          << "\n"
      << "posx "           << posX           << "\n"
      << "posy "           << posY           << "\n"
      << "wavetime "       << waveTime       << "\n"
      << "haslostshards "  << (hasLostShards ? 1 : 0) << "\n"
      << "lostshardposx "  << lostShardPosX  << "\n"
      << "lostshardposy "  << lostShardPosY  << "\n"
      << "lostshardcount " << lostShardCount << "\n"
      << "musicvolume "    << musicVolume    << "\n"
      << "sfxvolume "      << sfxVolume      << "\n";
    return true;
}

bool SaveData::Load(const std::string& name) {
    profileName = name;
    std::ifstream f(GetFilePath());
    if (!f) return false;
    std::string key;
    while (f >> key) {
        if      (key == "profile")        f >> profileName;
        else if (key == "shards")         f >> shardCount;
        else if (key == "highscore")      f >> highScore;
        else if (key == "health")         f >> health;
        else if (key == "lives")          f >> lives;
        else if (key == "score")          f >> score;
        else if (key == "posx")           f >> posX;
        else if (key == "posy")           f >> posY;
        else if (key == "wavetime")       f >> waveTime;
        else if (key == "haslostshards")  { int v; f >> v; hasLostShards = (v != 0); }
        else if (key == "lostshardposx")  f >> lostShardPosX;
        else if (key == "lostshardposy")  f >> lostShardPosY;
        else if (key == "lostshardcount") f >> lostShardCount;
        else if (key == "musicvolume")    f >> musicVolume;
        else if (key == "sfxvolume")      f >> sfxVolume;
    }
    return true;
}

void SaveData::Reset() {
    shardCount      = 0;
    health          = 100.0f;
    lives           = 3;
    score           = 0;
    posX            = 0.0f;
    posY            = 0.0f;
    waveTime        = 0.0f;
    hasLostShards   = false;
    lostShardPosX   = 0.0f;
    lostShardPosY   = 0.0f;
    lostShardCount  = 0;
    musicVolume     = 0.10f;
    sfxVolume       = 0.05f;
}

std::vector<std::pair<std::string, int>> SaveData::GetLeaderboard() {
    std::vector<std::pair<std::string, int>> board;
    auto names = GetProfileList();
    for (const auto& name : names) {
        std::ifstream f("profile_" + name + ".dat");
        int hs = 0;
        if (f) {
            std::string key;
            while (f >> key) {
                if (key == "highscore") { f >> hs; break; }
                std::string skip; f >> skip; // consume this key's value
            }
        }
        board.push_back({ name, hs });
    }
    std::sort(board.begin(), board.end(),
        [](const std::pair<std::string, int>& a,
           const std::pair<std::string, int>& b) {
            return a.second > b.second;
        });
    return board;
}

bool SaveData::DeleteProfile(const std::string& name) {
    std::string path = "profile_" + name + ".dat";
    return std::remove(path.c_str()) == 0;
}

std::vector<std::string> SaveData::GetProfileList() {
    std::vector<std::string> list;
    struct _finddata_t fd;
    intptr_t h = _findfirst("profile_*.dat", &fd);
    if (h == -1) return list;
    do {
        std::string fname = fd.name;
        // Strip "profile_" (8 chars) prefix and ".dat" (4 chars) suffix
        if (fname.size() > 12)
            list.push_back(fname.substr(8, fname.size() - 12));
    } while (_findnext(h, &fd) == 0);
    _findclose(h);
    return list;
}
