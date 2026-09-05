#pragma once

#include <filesystem>
#include "MemoryScanner.h"

namespace DosBoxMemoryTools::TraceComparisonPersistence
{
    enum class RestoreResult { Missing, Loaded, Invalid };

    // Full snapshots, independent of the original Load A/B source files.
    bool save(const std::filesystem::path& path,
        const std::vector<RuntimeInstruction>& trace,
        const std::string& sourceFilename) noexcept;

    // Outputs are only replaced after the complete snapshot has been validated.
    RestoreResult restore(const std::filesystem::path& path,
        std::vector<RuntimeInstruction>& trace,
        std::string& sourceFilename) noexcept;
}
