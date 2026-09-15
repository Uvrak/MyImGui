#pragma once

#include <cstddef>
#include <vector>
#include <functional>

#include "MemoryScanner.h"
#include "TraceAlignment.h"
#include "TraceComparison.h"

namespace DosBoxMemoryTools
{
    class TraceDifferenceNavigation
    {
    public:

        using DifferenceComparer =
            std::function<TraceInstructionDifference(
                const RuntimeInstruction&,
                const RuntimeInstruction&
            )>;

        using ControlFlowComparer =
            std::function<bool(
                size_t address
                )>;

        static size_t findNextDifference(
            const std::vector<RuntimeInstruction>& traceA,
            const std::vector<RuntimeInstruction>& traceB,
            const std::vector<TraceAlignment>& alignments,
            size_t selectedIndexA,
            DifferenceComparer comparer,
            ControlFlowComparer controlFlowComparer
        );

        static size_t findPreviousDifference(
            const std::vector<RuntimeInstruction>& traceA,
            const std::vector<RuntimeInstruction>& traceB,
            const std::vector<TraceAlignment>& alignments,
            size_t selectedIndexA,
            DifferenceComparer comparer,
            ControlFlowComparer controlFlowComparer
        );
    };
}