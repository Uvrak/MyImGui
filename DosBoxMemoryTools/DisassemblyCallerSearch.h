#pragma once

#include "DisassemblyState.h"

namespace DosBoxMemoryTools
{
    class DisassemblyCallerSearch
    {
    public:
        static void drawControls(DisassemblyState& state);
        static void drawResults(DisassemblyState& state, const DisassemblyNavigate& goToAddress);
    };
}
