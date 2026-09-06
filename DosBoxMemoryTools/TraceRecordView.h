#pragma once

#include "MemoryScanner.h"
#include "TraceComparison.h"

namespace DosBoxMemoryTools
{
    class TraceRecordView
    {
    public:
        void draw(
            size_t index,
            const RuntimeInstruction& instruction,
            const TraceInstructionDifference& difference,
            bool selected = false
        );
    };
}