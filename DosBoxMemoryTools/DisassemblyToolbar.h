#pragma once

#include "DisassemblyState.h"

namespace DosBoxMemoryTools
{
    class DisassemblyToolbar
    {
    public:
        static void draw(DisassemblyState& state, const std::vector<uint8_t>& liveMemory, const DisassemblyNavigate& goToAddress);
    };
}
