#ifndef DEBUGOVERLAY_H
#define DEBUGOVERLAY_H

// Real-time debug overlay with scrolling waveform graphs.
// F9 cycles: hidden > minimal > detailed > hidden.
// The [MIN]/[FULL] button inside the window switches mode without hiding.
class DebugOverlay {
public:
    enum class Mode { MINIMAL, DETAILED };

    DebugOverlay();
    ~DebugOverlay();

    void Update(float deltaTime);
    void Draw();                          // non-const: in-window button can change mode

    Mode GetMode()        const { return currentMode; }
    void SetMode(Mode m)        { currentMode = m; }

private:
    Mode  currentMode = Mode::MINIMAL;

    float cpuPercent       = 0.0f;
    float ramProcessMB     = 0.0f;
    float ramSystemUsedGB  = 0.0f;
    float ramSystemTotalGB = 0.0f;
    float gpuPercent       = 0.0f;

    float pollTimer = 0.0f;
    static constexpr float kPollInterval = 0.5f;

    static constexpr int kHistorySize = 128;
    float fpsHistory   [kHistorySize];
    float cpuHistory   [kHistorySize];
    float gpuHistory   [kHistorySize];
    float ramPctHistory[kHistorySize];
    int   historyOffset = 0;

    long long prevIdleTime   = 0;
    long long prevKernelTime = 0;
    long long prevUserTime   = 0;

    void* hQuery      = nullptr;
    void* hCounter    = nullptr;
    bool  gpuPDHValid = false;

    void InitGPUCounter();
    void PollCPU();
    void PollRAM();
    void PollGPU();
    void PushHistory(float fps);
};

#endif // DEBUGOVERLAY_H
