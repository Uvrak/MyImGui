#pragma once

#include <vector>
#include <unordered_set>

#include "DosBoxMemoryScanner.h"
#include "FloatingWindow.h"
#include "FlowLayout.h"

namespace MyImGui
{
    class DosBoxMemoryScannerWindow
    {
    public:
        void draw(
            bool* isOpen = nullptr
        );

    private:
        DosBoxMemoryScanner
            m_scanner;

        FloatingWindow m_toolbarWindow{
    "Memory Scanner Toolbar",
    FloatingWindowOptions{
        true,   // movable
        false,  // resizable
        false,  // collapsible
        false,  // closable
        true,   // titleBar
        true,   // autoResizeHeight
        true   // dockable
        }
    };

        FlowLayout m_toolbarLayout;

        DosBoxMemoryScanMode m_scanMode =
            DosBoxMemoryScanMode::Changed;

        int m_exactValue = 0;

        bool m_filterPrevious = false;
        int m_previousValue = 0;

        bool m_filterCurrent = false;
        int m_currentValue = 0;

        bool m_filterDifference = false;
        int m_differenceValue = 0;

        std::unordered_set<size_t>
            m_pinnedAddresses;

        char m_addressSearch[32] = {};
        size_t m_foundAddress = 0;
        bool m_hasFoundAddress = false;
        bool m_addressSearchAttempted = false;
    };
    
}