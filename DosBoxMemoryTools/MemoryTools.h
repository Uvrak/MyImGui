#pragma once

#include "MemoryScannerWindow.h"
#include "MemoryViewerWindow.h"
#include "MemoryReadTrackerWindow.h"
#include "DisassemblyWindow.h"
#include "InstructionTransitionTrackerWindow.h"
#include "ExecutionCaptureWindow.h"
#include "ExecutionTraceWindow.h"

#include "imgui.h"

namespace DosBoxMemoryTools
{
    class DosBoxView;

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

    private:
        bool m_liveView = false;
        double m_lastLiveRefresh = 0.0;

        MemoryReader
            m_memoryReader;

        MemoryScannerWindow
            m_scannerWindow;

        MemoryViewerWindow
            m_viewerWindow;

        MemoryReadTrackerWindow
            m_readTrackerWindow;

        InstructionTransitionTrackerWindow
            m_transitionTrackerWindow;

        ExecutionCaptureWindow
            m_executionCaptureWindow;

        ExecutionTraceWindow
            m_executionTraceWindow;

        DisassemblyWindow  
            m_disassemblyWindow;
    };
}              
