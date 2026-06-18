#pragma once
#ifndef SAVEDATA_H
#define SAVEDATA_H

struct SaveData {
    int   shardCount  = 0;
    int   highScore   = 0;
    float musicVolume = 0.10f;
    float sfxVolume   = 0.05f;

    static SaveData current;

    bool FileExists(const char* path = "save.dat") const;
    bool Save      (const char* path = "save.dat") const;
    bool Load      (const char* path = "save.dat");
    void Reset();
};

#endif
