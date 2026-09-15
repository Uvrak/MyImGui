#pragma once

#include "MemoryScanner.h"
#include "TraceComparison.h"
#include "MemoryWriteTracker.h"
#include "TraceTracking.h"
#include "TraceComparisonWindow.h"
#include "TransitionTracking.h"
#include "ExecutionTracking.h"
#include "ScannerAddress.h"
#include "ScannerRange.h"
#include "MemoryReadTracker.h"

namespace DosBoxMemoryTools
{
    class TrackingWindow
    {
    public:
        TrackingWindow(
            MemoryScanner& scanner,
            const std::string& gameId,
            ScannerAddress& scannerAddress,
            ScannerRange& scannerRange
        );

        void draw(
            bool* isOpen
        );

        void saveSession() const;

        void setGameId(
            const std::string& gameId
        );

    private:
        ScannerAddress&
            m_scannerAddress;

        ScannerRange&
            m_scannerRange;

        std::string
            m_gameId;

        char m_targetText[32] =
            "0x31C33";

        enum class TrackingTab
        {
            Trace,
            Trans,
            Exec,
            MemRd,
            MemWr
        };

        TrackingTab m_activeTab =
            TrackingTab::Trace;

        MemoryReadTracker
            m_memoryReadTracker;

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