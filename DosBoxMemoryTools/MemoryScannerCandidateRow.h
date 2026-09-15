#pragma once

#include <cstddef>
#include <vector>

namespace DosBoxMemoryTools
{
    class MemoryScannerWindow;

    class MemoryScannerCandidateRow
    {
    public:
        static void draw(MemoryScannerWindow& window, bool liveView, int index,
        const std::vector<size_t>& pinnedOnlyAddresses,
        const std::vector<size_t>& describedOnlyAddresses,
        const std::vector<int>& filteredIndices);
    };
}
