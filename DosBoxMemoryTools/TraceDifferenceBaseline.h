#pragma once

#include "TraceComparison.h"

#include <cstddef>
#include <vector>
#include <filesystem>

namespace DosBoxMemoryTools
{
    struct TraceDifferenceBaselineEntry
    {
        size_t address = 0;

        TraceInstructionDifference difference;

        bool controlFlow = false;

        size_t controlFlowEndAddressA = 0;
        size_t controlFlowStartAddressB = 0;
        size_t controlFlowEndAddressB = 0;
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

        bool containsControlFlow(
            size_t startAddressA,
            size_t endAddressA,
            size_t startAddressB,
            size_t endAddressB
        ) const;

        void clear();

        size_t size() const;

        bool save(
            const std::filesystem::path& path
        ) const;

        bool load(
            const std::filesystem::path& path
        );

    private:
        std::vector<TraceDifferenceBaselineEntry>
            m_entries;
    };
}