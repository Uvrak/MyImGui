#pragma once

#include "MemoryScannerWindow.h"
#include "MemoryViewerWindow.h"
#include "MemoryReadTrackerWindow.h"
#include "DisassemblyWindow.h"
#include "TrackingWindow.h"

#include <string>

namespace DosBoxX
{
    class View;
}

namespace DosBoxMemoryTools
{
    class MemoryTools
    {
    public:
        MemoryTools(
            const std::string& gameId,
            DosBoxX::View* dosBoxView
        );

        void setGameId(
            const std::string& gameId
        );

        void draw();

        MemoryReader& memoryReader();

        void saveSession();

        bool refreshMemory();

        void refreshPinnedValues();

    private:    
        bool m_liveView = false;
        double m_lastLiveRefresh = 0.0;
        bool m_initialRefreshDone = false;

        MemoryReader
            m_memoryReader;

        MemoryScannerWindow
            m_scannerWindow;

        MemoryViewerWindow
            m_viewerWindow;

        MemoryReadTrackerWindow
            m_readTrackerWindow;

        TrackingWindow
            m_trackingWindow;

        DisassemblyWindow
            m_disassemblyWindow;
    };
}