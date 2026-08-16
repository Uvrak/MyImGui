#pragma once

#include "DosBoxMemoryScannerWindow.h"
#include "DosBoxMemoryViewerWindow.h"

#include "imgui.h"

namespace MyImGui
{
    class DosBoxView;

    class DosBoxMemoryTools
    {
    public:
        DosBoxMemoryTools(
            const std::string& gameId,
            DosBoxView* dosBoxView
        );

        void setGameId(
            const std::string& gameId
        );

        void draw();

    private:
        bool m_liveView = false;
        double m_lastLiveRefresh = 0.0;

        DosBoxMemoryReader
            m_memoryReader;

        DosBoxMemoryScannerWindow
            m_scannerWindow;

        DosBoxMemoryViewerWindow
            m_viewerWindow;
    };
}              
