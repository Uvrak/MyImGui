#pragma once

#include "MemoryScanner.h"
#include "RecordButton.h"

#include <string>
#include <vector>

namespace DosBoxMemoryTools
{
    class TraceTracking
    {
    public:
        TraceTracking(
            MemoryScanner& scanner,
            const std::string& gameId
        );

        ~TraceTracking();

        void draw();

        void setGameId(
            const std::string& gameId
        );

        void saveSession() const;

        const std::vector<RuntimeInstruction>&
            trace() const;

    private:
        void drawNavigation();

        void drawRecorder();

        void drawTrace();

        void loadTrace();

        bool loadTraceFromFile(
            const std::string& filename,
            std::vector<RuntimeInstruction>& trace
        );

        void saveTraceToFile(
            const std::string& filename
        ) const;

        void handleLoadTraceRequest();

        void handleSaveTraceRequest();

        void loadSession();

        MemoryScanner&
            m_scanner;

        std::string
            m_gameId;

        MyImGui::RecordButton
            m_recordButton;

        char m_targetText[32] =
            "0x31C33";

        std::vector<RuntimeInstruction>
            m_trace;

        bool m_traceWasArmedOrActive =
            false;

        size_t m_selectedTraceIndex =
            static_cast<size_t>(-1);

        bool m_scrollToSelectedTrace =
            false;

        bool m_loadTraceRequested =
            false;

        bool m_saveTraceRequested =
            false;
    };
}