#pragma once

#include "TraceAlignment.h"

#include <cstddef>
#include <vector>

namespace DosBoxMemoryTools
{
    enum class TraceRegister
    {
        AX,
        BX,
        CX,
        DX,
        SI,
        DI,
        BP,
        SP,
        DS,
        ES,
        SS
    };

    class TraceRegisterDivergenceFinder
    {
    public:
        static bool findNext(
            const std::vector<RuntimeInstruction>& traceA,
            const std::vector<RuntimeInstruction>& traceB,
            const std::vector<TraceAlignment>& alignment,
            TraceRegister traceRegister,
            size_t startIndexA,
            size_t startIndexB,
            size_t& resultIndexA,
            size_t& resultIndexB
        );

        static bool findPrevious(
            const std::vector<RuntimeInstruction>& traceA,
            const std::vector<RuntimeInstruction>& traceB,
            const std::vector<TraceAlignment>& alignment,
            TraceRegister traceRegister,
            size_t startIndexA,
            size_t startIndexB,
            size_t& resultIndexA,
            size_t& resultIndexB
        );

        static bool findPreviousChange(
            const std::vector<RuntimeInstruction>& trace,
            TraceRegister traceRegister,
            size_t startIndex,
            size_t& resultIndex
        );
    };
}