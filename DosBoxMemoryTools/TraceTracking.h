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

        bool targetDatasetA() const;

        bool takeCompletedTrace();

    private:
        void drawNavigation();

        void drawRecorder();

        void updateCapture();

        void loadTrace();

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

        enum class TargetDataset
        {
            A,
            B
        };

        TargetDataset m_targetDataset =
            TargetDataset::A;

        bool m_traceCompleted =
            false;
    };
}