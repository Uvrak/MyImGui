#pragma once

#include "DisassemblyState.h"

namespace DosBoxMemoryTools
{
    class DisassemblyView
    {
    public:
        static void draw(DisassemblyState& state, const DisassemblyNavigate& goToAddress);
    };
}
