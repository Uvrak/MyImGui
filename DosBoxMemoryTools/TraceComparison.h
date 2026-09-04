#pragma once

#include "MemoryScanner.h"

namespace DosBoxMemoryTools
{
    struct TraceInstructionDifference
    {
        bool address = false;

        bool ax = false;
        bool bx = false;
        bool cx = false;
        bool dx = false;

        bool si = false;
        bool di = false;
        bool bp = false;
        bool sp = false;

        bool ds = false;
        bool es = false;
        bool ss = false;

        bool any() const
        {
            return
                address ||
                ax || bx || cx || dx ||
                si || di || bp || sp ||
                ds || es || ss;
        }
    };

    TraceInstructionDifference
        compareTraceInstructions(
            const RuntimeInstruction& current,
            const RuntimeInstruction& reference
        );
}