#pragma once

#include <vector>

#include "DosBoxMemoryScanner.h"
#include "RecordButton.h"

namespace MyImGui
{
    class ExecutionTraceWindow
    {
    public:
        ExecutionTraceWindow(
            DosBoxMemoryScanner& scanner,
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

        DosBoxMemoryScanner&
            m_scanner;

        std::vector<RuntimeInstruction>
            m_trace;

        RecordButton
            m_recordButton;

        char m_targetText[32] =
            "0x31C33";

        std::string
            m_gameId;
    };
}