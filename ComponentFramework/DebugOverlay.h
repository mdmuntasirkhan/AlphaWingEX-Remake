#pragma once
#ifndef DEBUGOVERLAY_H
#define DEBUGOVERLAY_H

// Toggle with F9 in SceneMuntasir.
// Shows FPS, frame time, system CPU%, process RAM, system RAM, and GPU 3D engine % (Windows 10+).
class DebugOverlay {
public:
    DebugOverlay();
    ~DebugOverlay();

    void Update(float deltaTime);  // call every frame; polls OS stats at kPollInterval
    void Draw()   const;           // call inside an active ImGui frame

private:
    float cpuPercent        = 0.0f;
    float ramProcessMB      = 0.0f;
    float ramSystemUsedGB   = 0.0f;
    float ramSystemTotalGB  = 0.0f;
    float gpuPercent        = 0.0f;

    float pollTimer = 0.0f;
    static constexpr float kPollInterval = 0.5f;  // seconds between OS stat polls

    // CPU timing — raw 64-bit FILETIME values stored without pulling <windows.h> into the header
    long long prevIdleTime   = 0;
    long long prevKernelTime = 0;
    long long prevUserTime   = 0;

    // PDH GPU handles stored as void* so <pdh.h> stays out of the header
    void* hQuery   = nullptr;
    void* hCounter = nullptr;
    bool  gpuPDHValid = false;

    void InitGPUCounter();
    void PollCPU();
    void PollRAM();
    void PollGPU();
};

#endif // DEBUGOVERLAY_H
