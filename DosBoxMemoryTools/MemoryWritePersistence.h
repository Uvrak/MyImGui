#pragma once

#include <filesystem>
#include <vector>

#include "MemoryScannerTypes.h"

namespace DosBoxMemoryTools::MemoryWritePersistence
{
    struct Snapshot
    {
        size_t rangeStart = 0;
        size_t rangeEnd = 0;

        std::vector<RuntimeInstruction>
            captures;
    };

    bool save(
        const std::filesystem::path& path,
        const Snapshot& snapshot
    );

    bool load(
        const std::filesystem::path& path,
        Snapshot& snapshot
    );
}