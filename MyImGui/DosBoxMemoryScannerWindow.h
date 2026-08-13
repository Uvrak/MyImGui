#pragma once

#include "DosBoxMemoryScanner.h"

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

        DosBoxMemoryScanMode m_scanMode =
            DosBoxMemoryScanMode::Changed;
    };
}