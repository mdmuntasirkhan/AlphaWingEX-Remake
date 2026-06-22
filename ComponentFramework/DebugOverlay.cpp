#include "DebugOverlay.h"
#include "imgui.h"
#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#include <vector>
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")

DebugOverlay::DebugOverlay() {
    InitGPUCounter();
    PollCPU();   // establish baseline so the first real poll gets a valid delta
}

DebugOverlay::~DebugOverlay() {
    if (hQuery) {
        PdhCloseQuery((PDH_HQUERY)hQuery);
        hQuery   = nullptr;
        hCounter = nullptr;
    }
}

void DebugOverlay::InitGPUCounter() {
    PDH_HQUERY   q = nullptr;
    PDH_HCOUNTER c = nullptr;
    if (PdhOpenQuery(nullptr, 0, &q) != ERROR_SUCCESS) return;
    // PdhAddCounter (not the Enhanced variant) is available across all supported SDK versions.
    // \GPU Engine(*engtype_3D)\Utilization Percentage — Windows 10 1803+, cross-vendor.
    if (PdhAddCounter(q, "\\GPU Engine(*engtype_3D)\\Utilization Percentage", 0, &c) != ERROR_SUCCESS) {
        PdhCloseQuery(q);
        return;
    }
    PdhCollectQueryData(q);   // first collection establishes the rate baseline
    hQuery      = q;
    hCounter    = c;
    gpuPDHValid = true;
}

void DebugOverlay::Update(float deltaTime) {
    pollTimer += deltaTime;
    if (pollTimer >= kPollInterval) {
        pollTimer = 0.0f;
        PollCPU();
        PollRAM();
        PollGPU();
    }
}

void DebugOverlay::PollCPU() {
    FILETIME idle, kernel, user;
    if (!GetSystemTimes(&idle, &kernel, &user)) return;

    auto toLL = [](const FILETIME& ft) -> long long {
        return (static_cast<long long>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    };
    long long i = toLL(idle), k = toLL(kernel), u = toLL(user);

    long long dIdle  = i - prevIdleTime;
    long long dTotal = (k - prevKernelTime) + (u - prevUserTime);

    prevIdleTime   = i;
    prevKernelTime = k;
    prevUserTime   = u;

    if (dTotal > 0)
        cpuPercent = (float)(dTotal - dIdle) * 100.0f / (float)dTotal;
}

void DebugOverlay::PollRAM() {
    PROCESS_MEMORY_COUNTERS pmc = {};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        ramProcessMB = (float)pmc.WorkingSetSize / (1024.0f * 1024.0f);

    MEMORYSTATUSEX ms = {};
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        ramSystemTotalGB = (float)ms.ullTotalPhys / (1024.0f * 1024.0f * 1024.0f);
        ramSystemUsedGB  = (float)(ms.ullTotalPhys - ms.ullAvailPhys) / (1024.0f * 1024.0f * 1024.0f);
    }
}

void DebugOverlay::PollGPU() {
    if (!gpuPDHValid) return;

    PdhCollectQueryData((PDH_HQUERY)hQuery);

    // First call with null buffer returns required sizes without PDH_MORE_DATA dependency.
    DWORD bufSize = 0, itemCount = 0;
    PdhGetFormattedCounterArray(
        (PDH_HCOUNTER)hCounter, PDH_FMT_DOUBLE, &bufSize, &itemCount, nullptr);

    if (bufSize == 0 || itemCount == 0) return;

    std::vector<BYTE> buf(bufSize);
    if (PdhGetFormattedCounterArray(
            (PDH_HCOUNTER)hCounter, PDH_FMT_DOUBLE, &bufSize, &itemCount,
            reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(buf.data())) == ERROR_SUCCESS) {
        auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(buf.data());
        double peak = 0.0;
        for (DWORD i = 0; i < itemCount; i++)
            if (items[i].FmtValue.doubleValue > peak)
                peak = items[i].FmtValue.doubleValue;
        gpuPercent = (float)peak;
    }
}

void DebugOverlay::Draw() const {
    float fps     = ImGui::GetIO().Framerate;
    float frameMs = (fps > 0.0f) ? 1000.0f / fps : 0.0f;

    // Anchor to top-right: pivot (1,0) means the window's top-right corner sits at the pos.
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x - 10.0f, 10.0f),
        ImGuiCond_Always,
        ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("##dbg", nullptr,
        ImGuiWindowFlags_NoDecoration    | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav           | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "FPS  %.1f  (%.2f ms)", fps, frameMs);
    ImGui::Separator();
    ImGui::Text("CPU  %.1f%%", cpuPercent);
    ImGui::Text("RAM  %.0f MB   (sys %.1f / %.1f GB)",
        ramProcessMB, ramSystemUsedGB, ramSystemTotalGB);
    if (gpuPDHValid)
        ImGui::Text("GPU  %.1f%%", gpuPercent);
    else
        ImGui::TextDisabled("GPU  N/A");

    ImGui::End();
}
