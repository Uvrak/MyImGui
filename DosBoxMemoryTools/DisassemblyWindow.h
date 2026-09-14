#pragma once

#include <cstddef>
#include <vector>
#include <unordered_map>
#include <string>

#include "MemoryReader.h"
#include "DisassemblyState.h"
#include "ScannerAddress.h"

namespace DosBoxMemoryTools
{
    class DisassemblyWindow
    {
    public:
        DisassemblyWindow(
            MemoryReader& memoryReader,
            ScannerAddress& scannerAddress
        );

        void draw(
            bool* isOpen
        );

        void goToAddress(
            size_t address
        );

    private:
        MemoryReader& m_memoryReader;
        ScannerAddress& m_scannerAddress;
        DisassemblyState state;
    };
}
