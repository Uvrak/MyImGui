#pragma once

#include "MemoryScanner.h"

#include <cstddef>
#include <vector>

namespace DosBoxMemoryTools
{
    struct TraceAlignment
    {
        size_t indexA = 0;
        size_t indexB = 0;

        size_t endIndexA = 0;
        size_t endIndexB = 0;

        bool synchronized = true;
    };

    class TraceAligner
    {
    public:
        static std::vector<TraceAlignment> align(
            const std::vector<RuntimeInstruction>& traceA,
            const std::vector<RuntimeInstruction>& traceB
        );
    };
}