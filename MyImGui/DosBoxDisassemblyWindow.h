#pragma once

#include <cstddef>
#include <vector>
#include <unordered_map>

#include "DosBoxMemoryReader.h"

namespace MyImGui
{
    class DosBoxDisassemblyWindow
    {
    public:
        DosBoxDisassemblyWindow(
            DosBoxMemoryReader& memoryReader
        );

        void draw(
            bool* isOpen
        );

        void goToAddress(
            size_t address
        );

    private:
        DosBoxMemoryReader&
            m_memoryReader;

        char m_addressText[32] = {};

        size_t m_address = 0;

        std::unordered_map<size_t, int>
            branchTargetCounts;

        bool m_hasAddress = false;
        bool m_scrollToTop = false;

        char m_callerTargetText[32] = {};

        std::vector<size_t>
            m_callers;

        bool m_callerSearchPerformed = false;
    };
}