#pragma once

#include <cstddef>
#include <vector>
#include <unordered_map>
#include <string>

#include "MemoryReader.h"
#include "DisassemblyState.h"

namespace DosBoxMemoryTools
{
    class DisassemblyWindow
    {
    public:
        DisassemblyWindow(
            MemoryReader& memoryReader
        );

        void draw(
            bool* isOpen
        );

        void goToAddress(
            size_t address
        );

    private:
        MemoryReader& m_memoryReader;
        DisassemblyState state;
    };
}
