#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace DosBoxMemoryTools
{
    // Shared per-window state. Components retain the original update order.
    struct DisassemblyState
    {
        struct SavedAddress
        {
            size_t address = 0;
            std::string name;
        };

        std::vector<uint8_t>
            m_disassemblyMemory;

        char m_addressText[32] = {};

        size_t m_address = 0;

        std::unordered_map<size_t, int>
            branchTargetCounts;

        bool m_hasAddress = false;
        bool m_scrollToTop = false;

        char m_callerTargetText[32] = {};

        std::vector<size_t>
            m_callers;

        std::vector<SavedAddress>
            m_savedAddresses;

        char m_sessionName[64] = {};

        std::string m_status;

        bool m_callerSearchPerformed = false;
        bool m_wasOpen = false;

        bool m_savedAddressesLoaded = false;
    };

    using DisassemblyNavigate = std::function<void(size_t)>;
}