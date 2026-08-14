#pragma once

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
    };
    
}