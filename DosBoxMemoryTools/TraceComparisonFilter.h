#pragma once

#include <cstddef>
#include <vector>

#include "MemoryScanner.h"

namespace DosBoxMemoryTools
{
    struct TraceComparisonDisplayEntry
    {
        size_t traceIndex = 0;
        size_t collapsedCount = 0;
    };

    class TraceComparisonFilter
    {
    public:
        static std::vector<TraceComparisonDisplayEntry> build(
            const std::vector<RuntimeInstruction>& traceA,
            const std::vector<RuntimeInstruction>& traceB
        );
    };
}