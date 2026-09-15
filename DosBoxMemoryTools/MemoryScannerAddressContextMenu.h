#pragma once

#include <cstddef>
#include <cstdint>

namespace DosBoxMemoryTools
{
    struct MemoryCandidate;
    class MemoryScannerWindow;

    class MemoryScannerAddressContextMenu
    {
    public:
        static void drawPinnedOnly(MemoryScannerWindow& window, size_t address, uint32_t currentValue);
        static void drawCandidate(MemoryScannerWindow& window, const MemoryCandidate& candidate, bool pinned);
    };
}
