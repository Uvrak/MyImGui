#pragma once

#include <filesystem>
#include <string>

namespace DosBoxMemoryTools::TraceComparisonPersistence
{
    bool savePath(
        const std::filesystem::path& path,
        const std::string& sourceFilename
    ) noexcept;

    bool loadPath(
        const std::filesystem::path& path,
        std::string& sourceFilename
    ) noexcept;
}