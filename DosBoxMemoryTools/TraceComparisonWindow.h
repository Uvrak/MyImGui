#pragma once

#include <cstddef>
#include <cstring>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "TraceComparison.h"
#include "TraceComparisonFilter.h"
#include "TraceComparisonToolbar.h"
#include "TraceRecordView.h"
#include "TraceDifferenceBaseline.h"
#include "ScannerAddress.h"

namespace DosBoxMemoryTools
{
    class TraceComparisonWindow
    {
    public:
        using PhysicalAddrResolver =
            std::function<
            bool(
                const RuntimeInstruction&,
                size_t&
                )
            >;

        TraceComparisonWindow();

        void draw(
            const ScannerAddress& scannerAddress
        );

        void drawToolbar();

        void beginLoadTrace();

        void continueLoadTrace();

        bool openAndLoadTrace(
            bool forA
        );

        bool openAndSaveTrace(
            bool forA,
            const ScannerAddress& scannerAddress
        );

        static bool loadTraceFromFile(
            const std::string& filename,
            std::vector<RuntimeInstruction>& trace
        );

        static bool saveTraceToFile(
            const std::string& filename,
            const std::vector<RuntimeInstruction>& trace
        );

        const std::vector<RuntimeInstruction>& traceA() const
        {
            return m_traceA;
        }

        const std::vector<RuntimeInstruction>& traceB() const
        {
            return m_traceB;
        }

        void setTraceA(
            std::vector<RuntimeInstruction> trace
        )
        {
            m_traceA =
                std::move(
                    trace
                );

            m_traceAFilename[0] = '\0';

            if (!m_traceA.empty() &&
                !m_traceB.empty())
            {
                selectFirstDifference();
            }
        }

        void setTraceB(
            std::vector<RuntimeInstruction> trace
        )
        {
            m_traceB =
                std::move(
                    trace
                );

            m_traceBFilename[0] = '\0';

            if (!m_traceA.empty() &&
                !m_traceB.empty())
            {
                selectFirstDifference();
            }
        }

        bool hasLoadedTraceA() const
        {
            return m_hasLoadedTraceA;
        }

        const char* traceAFilename() const
        {
            return m_traceAFilename[0]
                ? m_traceAFilename
                : "";
        }

        const char* traceBFilename() const
        {
            return m_traceBFilename[0]
                ? m_traceBFilename
                : "";
        }

        void setTraceAFilename(
            const char* filename
        )
        {
            if (filename)
            {
                strncpy_s(
                    m_traceAFilename,
                    sizeof(m_traceAFilename),
                    filename,
                    _TRUNCATE
                );
            }
        }

        void setTraceBFilename(
            const char* filename
        )
        {
            if (filename)
            {
                strncpy_s(
                    m_traceBFilename,
                    sizeof(m_traceBFilename),
                    filename,
                    _TRUNCATE
                );
            }
        }

        size_t selectedTraceIndex() const
        {
            return m_selectedTraceIndex;
        }

        void setSelectedTraceIndex(
            size_t index
        )
        {
            m_selectedTraceIndex =
                index;

            m_scrollToSelectedTrace =
                true;
        }

        void setScrollToSelectedTrace(
            bool value
        )
        {
            m_scrollToSelectedTrace =
                value;
        }

        bool takeScrollToSelectedTrace()
        {
            const bool value =
                m_scrollToSelectedTrace;

            m_scrollToSelectedTrace =
                false;

            return value;
        }

        void setSelectedDatasetA(
            bool selectedA
        );

    private:
        void drawTraceRows(
            const std::vector<TraceComparisonDisplayEntry>& displayEntries,
            bool scrollToSelected
        );
        
        void drawDirectTraceRows(
            size_t count,
            bool scrollToSelected
        );

        void handleKeyboardNavigation(
            const ScannerAddress& scannerAddress
        );

        void selectPreviousDifference();
        void selectNextDifference();
        void selectPreviousRegisterDifference();
        void selectNextRegisterDifference();
        void selectFirstDifference();

        bool m_hasLoadedTraceA =
            false;

        std::string m_persistenceErrors[2];

        std::vector<RuntimeInstruction>
            m_traceA;

        std::vector<RuntimeInstruction>
            m_traceB;

        char m_traceAFilename[4096] = {};
        char m_traceBFilename[4096] = {};

        size_t m_selectedTraceIndex =
            static_cast<size_t>(
                -1
                );

        size_t m_selectedTraceIndexB =
            static_cast<size_t>(
                -1
                );

        bool m_scrollToSelectedTrace =
            false;

        TraceComparisonToolbar
            m_toolbar;

        TraceRecordView
            m_recordView;

        bool m_selectedDatasetA =
            true;

        bool m_collapseIdentical =
            true;

        TraceDifferenceBaseline m_differenceBaseline;

        bool m_ignoreDifferenceBaseline =
            false;

        TraceInstructionDifference compareInstructions(
            const RuntimeInstruction& instructionA,
            const RuntimeInstruction& instructionB
        ) const;

        float m_traceScrollY = 0.0f;

        std::vector<TraceComparisonDisplayEntry>
            m_collapsedDisplayEntries;

        bool
            m_collapsedDisplayEntriesDirty = true;

        TraceRegister m_selectedRegister =
            TraceRegister::AX;
    };

}