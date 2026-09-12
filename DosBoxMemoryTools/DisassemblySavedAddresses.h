#pragma once

#include "DisassemblyState.h"

namespace DosBoxMemoryTools
{
    class DisassemblySavedAddresses
    {
    public:
        static void draw(DisassemblyState& state, const DisassemblyNavigate& goToAddress);
        static void saveSession(DisassemblyState& state);
        static void loadSession(DisassemblyState& state);

    private:
        static void addSavedAddress(DisassemblyState& state);
    };
}
