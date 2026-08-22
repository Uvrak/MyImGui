#pragma once

#include <cstddef>
#include <vector>
#include <unordered_map>
#include <string>

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
        struct SavedAddress
        {
            size_t address = 0;
            std::string name;
        };

        void saveSession();
        void loadSession();

        void addSavedAddress();

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

        std::vector<SavedAddress>
            m_savedAddresses;

        char m_sessionName[64] = {};

        std::string m_status;

        bool m_callerSearchPerformed = false;
        bool m_wasOpen = false;

        bool m_savedAddressesLoaded = false;
    };
}