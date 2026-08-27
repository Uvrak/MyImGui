#pragma once

#include <vector>

#include "MemoryScanner.h"
#include "RecordButton.h"
#include "ExecutionTraceNavigationWindow.h"

namespace DosBoxMemoryTools
{
    class ExecutionTraceWindow
    {
    public:
        ExecutionTraceWindow(
            MemoryScanner& scanner,
            const std::string& gameId
        );

        void draw(
            bool* isOpen
        );

        void saveSession() const;
        void loadSession();

        void setGameId(
            const std::string& gameId
        );

    private:
        void loadTrace();

        void saveTraceToFile(
            const std::string& filename
        ) const;

        bool loadTraceFromFile(
            const std::string& filename
        );

        MemoryScanner&
            m_scanner;

        std::vector<RuntimeInstruction>
            m_trace;

        MyImGui::RecordButton
            m_recordButton;

        char m_targetText[32] =
            "0x31C33";

        std::string
            m_gameId;

        size_t m_selectedTraceIndex =
            static_cast<size_t>(-1);

        bool m_traceWasArmedOrActive = false;

        bool m_scrollToSelectedTrace = false;

        int m_previousRegisterIndex = 5;

        ExecutionTraceNavigationWindow
            m_navigationWindow;
    };
}