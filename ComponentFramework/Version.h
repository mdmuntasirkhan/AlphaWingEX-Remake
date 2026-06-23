#pragma once
#ifndef VERSION_H
#define VERSION_H

// ── Alpha Engine build version ───────────────────────────────────────────────
// Bump these numbers on every release:
//   kMajor — major overhaul / new generation
//   kMinor — new feature added
//   kPatch — bug fix only
//   kBuild — increment every time you stamp a release
namespace AppVersion {
    static constexpr int kMajor = 1;
    static constexpr int kMinor = 0;
    static constexpr int kPatch = 0;
    static constexpr int kBuild = 1;
}

#endif // VERSION_H
