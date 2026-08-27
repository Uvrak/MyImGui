#pragma once

#include <string>

#include "MemoryReader.h"
#include "MemoryScanner.h"

namespace DosBoxMemoryTools
{
    enum class MemorySearchType
    {
        String,
        BytePattern
    };

    class MemoryViewerWindow
    {
    public:
        MemoryViewerWindow(
            MemoryReader& memoryReader
        );

        void draw(
            bool* isOpen,
            bool& liveView
        );

        void goToAddress(
            size_t address
        );
        
    private:
        MemoryReader&
            m_memoryReader;

        MemoryScanner
            m_memoryScanner;

        MemorySearchType m_searchType =
            MemorySearchType::String;

        char m_searchText[128] = {};

        bool searchBytePattern(
            int patternN,
            size_t& resultAddress
        );
        
        int m_patternN = 0;
        int m_patternFrom = 0;
        int m_patternTo = 64;

        size_t m_searchResult = 0;
        bool m_hasSearchResult = false;
        bool m_scrollToSearchResult = false;
        bool m_searchPerformed = false;
        
        char m_addressText[32] = {};

        size_t m_selectedAddress = 0;
        bool m_hasSelectedAddress = false;
        bool m_memoryViewActive = false;

        bool m_scrollToSelectedAddress = false;
        bool m_keepSelectedVisible = false;
    };
}
