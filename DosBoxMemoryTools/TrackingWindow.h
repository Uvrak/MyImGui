#pragma once

#include "MemoryScanner.h"
#include "TraceComparison.h"
#include "MemoryWriteTracker.h"
#include "TraceTracking.h"
#include "TraceComparisonWindow.h"
#include "TransitionTracking.h"
#include "ExecutionTracking.h"

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

        void setGameId(
            const std::string& gameId
        );

    private:
        std::string
            m_gameId;

        char m_targetText[32] =
            "0x31C33";

        enum class TrackingTab
        {
            Trace,
            Trans,
            Exec,
            MemWr
        };

        TrackingTab m_activeTab =
            TrackingTab::Trace;

        MemoryWriteTracker
            m_memoryWriteTracker;

        TraceTracking
            m_traceTracking;

        TraceComparisonWindow
            m_traceComparisonWindow;

        TransitionTracking
            m_transitionTracking;

        ExecutionTracking
            m_executionTracking;
    };
}