#include "DebugOverlay.h"
#include "imgui.h"
#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#include <vector>
#include <cstring>
#include <cstdio>
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")

DebugOverlay::DebugOverlay() {
    memset(fpsHistory,    0, sizeof(fpsHistory));
    memset(cpuHistory,    0, sizeof(cpuHistory));
    memset(gpuHistory,    0, sizeof(gpuHistory));
    memset(ramPctHistory, 0, sizeof(ramPctHistory));
    InitGPUCounter();
    PollCPU();
}

DebugOverlay::~DebugOverlay() {
    if (hQuery) {
        PdhCloseQuery((PDH_HQUERY)hQuery);
        hQuery = hCounter = nullptr;
    }
}

void DebugOverlay::InitGPUCounter() {
    PDH_HQUERY q = nullptr; PDH_HCOUNTER c = nullptr;
    if (PdhOpenQuery(nullptr, 0, &q) != ERROR_SUCCESS) return;
    if (PdhAddCounter(q, "\\GPU Engine(*engtype_3D)\\Utilization Percentage", 0, &c) != ERROR_SUCCESS) {
        PdhCloseQuery(q); return;
    }
    PdhCollectQueryData(q);
    hQuery = q; hCounter = c; gpuPDHValid = true;
}

void DebugOverlay::Update(float deltaTime) {
    pollTimer += deltaTime;
    if (pollTimer >= kPollInterval) {
        pollTimer = 0.0f;
        PollCPU(); PollRAM(); PollGPU();
        PushHistory(ImGui::GetIO().Framerate);
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
    prevIdleTime = i; prevKernelTime = k; prevUserTime = u;
    if (dTotal > 0) cpuPercent = (float)(dTotal - dIdle) * 100.0f / (float)dTotal;
}

void DebugOverlay::PollRAM() {
    PROCESS_MEMORY_COUNTERS pmc = {};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        ramProcessMB = (float)pmc.WorkingSetSize / (1024.0f * 1024.0f);
    MEMORYSTATUSEX ms = {}; ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        ramSystemTotalGB = (float)ms.ullTotalPhys / (1024.0f * 1024.0f * 1024.0f);
        ramSystemUsedGB  = (float)(ms.ullTotalPhys - ms.ullAvailPhys) / (1024.0f * 1024.0f * 1024.0f);
    }
}

void DebugOverlay::PollGPU() {
    if (!gpuPDHValid) return;
    PdhCollectQueryData((PDH_HQUERY)hQuery);
    DWORD bufSize = 0, itemCount = 0;
    PdhGetFormattedCounterArray((PDH_HCOUNTER)hCounter, PDH_FMT_DOUBLE, &bufSize, &itemCount, nullptr);
    if (bufSize == 0 || itemCount == 0) return;
    std::vector<BYTE> buf(bufSize);
    if (PdhGetFormattedCounterArray((PDH_HCOUNTER)hCounter, PDH_FMT_DOUBLE, &bufSize, &itemCount,
            reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(buf.data())) == ERROR_SUCCESS) {
        auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(buf.data());
        double peak = 0.0;
        for (DWORD i = 0; i < itemCount; i++)
            if (items[i].FmtValue.doubleValue > peak) peak = items[i].FmtValue.doubleValue;
        gpuPercent = (float)peak;
    }
}

void DebugOverlay::PushHistory(float fps) {
    int slot = historyOffset % kHistorySize;
    fpsHistory[slot]    = fps;
    cpuHistory[slot]    = cpuPercent;
    gpuHistory[slot]    = gpuPercent;
    ramPctHistory[slot] = (ramSystemTotalGB > 0.0f)
                          ? (ramSystemUsedGB / ramSystemTotalGB) * 100.0f : 0.0f;
    historyOffset++;
}

void DebugOverlay::Draw() {
    float fps      = ImGui::GetIO().Framerate;
    float frameMs  = (fps > 0.0f) ? 1000.0f / fps : 0.0f;
    int   off      = historyOffset % kHistorySize;
    bool  detailed = (currentMode == Mode::DETAILED);
    char  buf[80];

    // Fixed graph width so AlwaysAutoResize has a stable content size to measure
    static constexpr float kW = 220.0f;
    static constexpr float kH = 30.0f;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10.0f, 10.0f),
                            ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.75f);

    // AlwaysAutoResize lets the window grow/shrink when switching modes.
    // We omit NoResize (part of NoDecoration) so auto-resize can work.
    ImGui::Begin("##dbg", nullptr,
        ImGuiWindowFlags_NoTitleBar          |
        ImGuiWindowFlags_NoScrollbar         |
        ImGuiWindowFlags_NoScrollWithMouse   |
        ImGuiWindowFlags_NoCollapse          |
        ImGuiWindowFlags_AlwaysAutoResize    |
        ImGuiWindowFlags_NoSavedSettings     |
        ImGuiWindowFlags_NoFocusOnAppearing  |
        ImGuiWindowFlags_NoNav               |
        ImGuiWindowFlags_NoMove);

    // ── FPS ──────────────────────────────────────────────────
    {
        ImVec4 col(0.0f, 1.0f, 0.45f, 1.0f);
        snprintf(buf, sizeof(buf), "FPS   %.1f  (%.2f ms)", fps, frameMs);
        ImGui::TextColored(col, "%s", buf);
        if (detailed) {
            ImGui::PushStyleColor(ImGuiCol_PlotLines, col);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.12f, 0.05f, 0.6f));
            ImGui::PlotLines("##fps", fpsHistory, kHistorySize, off, nullptr, 0.0f, 300.0f, ImVec2(kW, kH));
            ImGui::PopStyleColor(2);
        }
    }
    ImGui::Separator();

    // ── CPU ──────────────────────────────────────────────────
    {
        ImVec4 col(0.2f, 0.8f, 1.0f, 1.0f);
        snprintf(buf, sizeof(buf), "CPU   %.1f%%", cpuPercent);
        ImGui::TextColored(col, "%s", buf);
        if (detailed) {
            ImGui::PushStyleColor(ImGuiCol_PlotLines, col);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.02f, 0.1f, 0.12f, 0.6f));
            ImGui::PlotLines("##cpu", cpuHistory, kHistorySize, off, nullptr, 0.0f, 100.0f, ImVec2(kW, kH));
            ImGui::PopStyleColor(2);
        }
    }
    ImGui::Separator();

    // ── GPU ──────────────────────────────────────────────────
    {
        ImVec4 col(0.75f, 0.3f, 1.0f, 1.0f);
        if (gpuPDHValid) {
            snprintf(buf, sizeof(buf), "GPU   %.1f%%", gpuPercent);
            ImGui::TextColored(col, "%s", buf);
            if (detailed) {
                ImGui::PushStyleColor(ImGuiCol_PlotLines, col);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.09f, 0.04f, 0.12f, 0.6f));
                ImGui::PlotLines("##gpu", gpuHistory, kHistorySize, off, nullptr, 0.0f, 100.0f, ImVec2(kW, kH));
                ImGui::PopStyleColor(2);
            }
        } else {
            ImGui::TextDisabled("GPU   N/A");
        }
    }
    ImGui::Separator();

    // ── RAM ──────────────────────────────────────────────────
    {
        ImVec4 col(1.0f, 0.55f, 0.0f, 1.0f);
        snprintf(buf, sizeof(buf), "RAM   %.0f MB  (%.1f / %.1f GB)",
                 ramProcessMB, ramSystemUsedGB, ramSystemTotalGB);
        ImGui::TextColored(col, "%s", buf);
        if (detailed) {
            ImGui::PushStyleColor(ImGuiCol_PlotLines, col);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.07f, 0.0f, 0.6f));
            ImGui::PlotLines("##ram", ramPctHistory, kHistorySize, off, nullptr, 0.0f, 100.0f, ImVec2(kW, kH));
            ImGui::PopStyleColor(2);
        }
    }

    // ── Mode toggle ───────────────────────────────────────────
    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.18f, 0.18f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.38f, 0.38f, 0.38f, 0.9f));
    if (ImGui::Button(detailed ? "[ Minimal ]" : "[ Detailed ]", ImVec2(kW, 0)))
        currentMode = detailed ? Mode::MINIMAL : Mode::DETAILED;
    ImGui::PopStyleColor(2);

    ImGui::End();
}
