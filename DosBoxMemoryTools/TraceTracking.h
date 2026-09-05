#pragma once

#include "MemoryScanner.h"
#include "RecordButton.h"

#include <string>
#include <vector>

namespace DosBoxMemoryTools
{
    class MemoryScanner;

    class TraceTracking
    {
    public:
        TraceTracking(
            MemoryScanner& scanner,
            const std::string& gameId
        );

        ~TraceTracking();

        void draw();

        void loadTrace();

        void setGameId(
            const std::string& gameId
        );

        std::vector<RuntimeInstruction>
            m_trace;

        void drawRecorder();

        void drawTrace();

        bool loadTraceFromFile(const std::string& filename, std::vector<RuntimeInstruction>& trace);

        void saveTraceToFile(
            const std::string& filename
        ) const;

        void saveSession() const;

    private:
        void drawNavigation();

        void handleLoadTraceRequest();

        void handleSaveTraceRequest();

        MemoryScanner&
            m_scanner;

        std::string
            m_gameId;

        MyImGui::RecordButton
            m_recordButton;

        char m_targetText[32] =
            "0x31C33";

        bool m_traceWasArmedOrActive =
            false;

        size_t m_selectedTraceIndex =
            static_cast<size_t>(-1);

        bool m_scrollToSelectedTrace =
            false;

        bool m_loadTraceRequested =
            false;

        void loadSession();

        bool m_saveTraceRequested =
            false;
    };
}