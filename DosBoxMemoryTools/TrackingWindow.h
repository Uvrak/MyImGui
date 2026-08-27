#pragma once

#include <vector>
#include <utility>
#include <array>

#include "MemoryScanner.h"
#include "RecordButton.h"
#include "TrackingNavigationWindow.h"
#include "InstructionTransitionTrackerNavigationWindow.h"


namespace DosBoxMemoryTools
{
    class TrackingWindow
    {
    public:
        TrackingWindow(
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
        void drawTrace();
        void drawTransitions();
        void captureTransitions();
        void drawExecutionCapture();
        void drawMemoryWriteWatch();

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

        TrackingNavigationWindow
            m_navigationWindow;

        InstructionTransitionTrackerNavigationWindow
            m_transitionNavigationWindow;

        std::vector<std::pair<size_t, size_t>>
            m_transitions;

        std::vector<std::pair<uint16_t, uint16_t>>
            m_transitionContexts;

        std::vector<std::array<uint8_t, 16>>
            m_transitionBytes;

        char m_transitionTargetText[32] =
            "0xEC4B";

        std::vector<
            std::vector<RuntimeInstruction>
        >
            m_transitionHistories;

        std::vector<RuntimeInstruction>
            m_transitionNextInstructions;

        size_t
            m_selectedHistoryInstruction = 0;

        bool
            m_scrollToSelectedHistoryInstruction = false;

        char m_executionTargetText[32] =
            "0xEC4B";

        bool m_executionCaptureHit =
            false;

        RuntimeInstruction
            m_executionCapture{};

        MyImGui::RecordButton
            m_executionRecordButton;

        char m_memoryWriteTargetText[32] =
            "0xEC4B";

        bool m_memoryWriteCaptureHit =
            false;

        RuntimeInstruction
            m_memoryWriteCapture{};

        MyImGui::RecordButton
            m_memoryWriteRecordButton;

   
    };
}