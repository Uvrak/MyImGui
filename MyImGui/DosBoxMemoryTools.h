#pragma once

#include "DosBoxMemoryScannerWindow.h"
#include "DosBoxMemoryViewerWindow.h"

namespace MyImGui
{
    class DosBoxMemoryTools
    {
    public:
        DosBoxMemoryTools(
            const std::string& gameId
        );

        void setGameId(
            const std::string& gameId
        );
        void draw();

    private:
        DosBoxMemoryReader
            m_memoryReader;

        DosBoxMemoryScannerWindow
            m_scannerWindow;

        DosBoxMemoryViewerWindow
            m_viewerWindow;
    };
}              
