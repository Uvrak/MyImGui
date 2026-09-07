#pragma once

#include "TraceComparison.h"

#include <cstddef>
#include <vector>

namespace DosBoxMemoryTools
{
    struct TraceDifferenceBaselineEntry
    {
        size_t address = 0;

        TraceInstructionDifference difference;
    };

    class TraceDifferenceBaseline
    {
    public:
        void add(
            const std::vector<RuntimeInstruction>& traceA,
            const std::vector<RuntimeInstruction>& traceB
        );

        TraceInstructionDifference removeKnown(
            size_t address,
            const TraceInstructionDifference& difference
        ) const;

        void clear();

        size_t size() const;

    private:
        std::vector<TraceDifferenceBaselineEntry>
            m_entries;
    };
}