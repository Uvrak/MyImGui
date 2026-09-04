#pragma once

#include <vector>
#include <utility>
#include <array>

#include "MemoryScanner.h"
#include "RecordButton.h"
#include "TraceComparison.h"
#include "MemoryWriteTracker.h"

namespace DosBoxMemoryTools
{
    class TrackingWindow
    {
    public:
        TrackingWindow(
            MemoryScanner& scanner,
            const std::string& gameId
        );

        ~TrackingWindow();

        void draw(
            bool* isOpen
        );

        void saveSession() const;
        void loadSession();


        void setGameId(
            const std::string& gameId
        );

    private:
        void drawNavigation();
        void drawRecorder();

        void drawTrace();
        void drawTransitions();
        void captureTransitions();
        void drawExecutionCapture();
        void drawTraceComparison();

        void drawTraceComparisonInstruction(
            const RuntimeInstruction& instruction,
            const TraceInstructionDifference& difference,
            bool highlightChanges
        );

        void loadTrace();

        void saveTraceToFile(
            const std::string& filename
        ) const;

        bool loadTraceFromFile(
            const std::string& filename,
            std::vector<RuntimeInstruction>& trace
        );

        MemoryScanner&
            m_scanner;

        std::string
            m_gameId;

        std::vector<RuntimeInstruction>
            m_trace;

        std::vector<RuntimeInstruction>
            m_traceA;

        std::vector<RuntimeInstruction>
            m_traceB;

        char m_traceAFilename[4096] = {};
        char m_traceBFilename[4096] = {};

        bool m_loadTraceARequested = false;
        bool m_loadTraceBRequested = false;

        bool m_compareTraces = false;

        MyImGui::RecordButton
            m_recordButton;

        char m_targetText[32] =
            "0x31C33";

        size_t m_selectedTraceIndex =
            static_cast<size_t>(-1);

        bool m_traceWasArmedOrActive = false;

        bool m_scrollToSelectedTrace = false;

        bool m_saveTraceRequested = false;
        bool m_loadTraceRequested = false;

        bool m_loadCompareTraceRequested = false;

        int m_previousRegisterIndex = 5;

        MyImGui::RecordButton
            m_transitionRecordButton;

        size_t m_lastTransitionCount = 0;

        double m_lastTransitionChangeTime = 0.0;

        bool m_transitionSeen = false;

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
            m_selectedHistoryInstruction =
            static_cast<size_t>(-1);

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

        enum class TrackingTab
        {
            Trace,
            Trans,
            Exec,
            MemWr
        };

        TrackingTab m_activeTab =
            TrackingTab::Trace;

        bool tryGetPhysicalMemoryAddress(
            const RuntimeInstruction& instruction,
            size_t& physicalAddress
        ) const;
        
        MemoryWriteTracker
            m_memoryWriteTracker;

        void selectNextControlFlowDifference();
    };
}