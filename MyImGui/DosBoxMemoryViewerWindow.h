#pragma once

#include <string>

#include "DosBoxMemoryReader.h"
#include "DosBoxMemoryScanner.h"

namespace MyImGui
{
    class DosBoxMemoryViewerWindow
    {
    public:
        DosBoxMemoryViewerWindow(
            DosBoxMemoryReader& memoryReader
        );

        void draw(
            bool* isOpen,
            bool& liveView
        );

        void goToAddress(
            size_t address
        );

    private:
        DosBoxMemoryReader&
            m_memoryReader;

        DosBoxMemoryScanner
            m_memoryScanner;

        char m_searchText[128] = {};
        size_t m_searchResult = 0;
        bool m_hasSearchResult = false;
        bool m_scrollToSearchResult = false;
        
        char m_addressText[32] = {};
    };
}
