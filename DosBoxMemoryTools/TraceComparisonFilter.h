#pragma once

#include <cstddef>
#include <vector>
#include <functional>

#include "MemoryScanner.h"
#include "TraceAlignment.h"

namespace DosBoxMemoryTools
{
    struct TraceComparisonDisplayEntry
    {
        size_t indexA = 0;
        size_t indexB = 0;

        bool hasA = true;
        bool hasB = true;

        size_t collapsedCount = 0;

        bool synchronized = true;
    };

    class TraceComparisonFilter
    {
    public:
        using ControlFlowFilter =
            std::function<bool(
                const TraceAlignment&
                )>;

        static std::vector<TraceComparisonDisplayEntry> build(
            const std::vector<RuntimeInstruction>& traceA,
            const std::vector<RuntimeInstruction>& traceB,
            ControlFlowFilter controlFlowFilter
        );
    };
}