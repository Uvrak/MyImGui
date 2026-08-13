#pragma once
#pragma once

#include <cstdint>

namespace MyImGui
{
    constexpr uint32_t
        DosBoxMemorySnapshotVersion = 1;

    constexpr uint32_t
        DosBoxMemorySnapshotCapacity =
        640U * 1024U;

    struct DosBoxMemorySnapshotHeader
    {
        uint32_t version = 0;
        uint32_t memorySize = 0;
        uint64_t snapshotId = 0;
    };
}