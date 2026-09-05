#pragma once

#include <vector>
#include <utility>
#include <array>

#include "MemoryScanner.h"
#include "RecordButton.h"
#include "TraceComparison.h"
#include "MemoryWriteTracker.h"
#include "TraceTracking.h"
#include "TraceComparisonWindow.h"

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


        void setGameId(
            const std::string& gameId
        );

    private:
        void drawNavigation();
        void drawRecorder();
        void drawTransitions();
        void captureTransitions();
        void drawExecutionCapture();
        void drawTraceComparison();

        void drawTraceComparisonInstruction(
            const RuntimeInstruction& instruction,
            const TraceInstructionDifference& difference,
            bool highlightChanges
        );

        // Trace load/save moved to TraceComparisonWindow

        MemoryScanner&
            m_scanner;

        std::string
            m_gameId;


        bool m_loadTraceARequested = false;
        bool m_loadTraceBRequested = false;
        bool m_saveTraceARequested = false;
        bool m_saveTraceBRequested = false;

        char m_targetText[32] =
            "0x31C33";

        // selection moved to TraceComparisonWindow

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

        TraceTracking
            m_traceTracking;

        TraceComparisonWindow
            m_traceComparisonWindow;
    };
}