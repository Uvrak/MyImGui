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
#include "TraceDetailView.h"
#include "TraceListView.h"
#include "TraceRecordView.h"

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

        void draw();

        void drawToolbar();

        bool openAndLoadTrace(
            bool forA
        );

        bool openAndSaveTrace(
            bool forA
        );

        static bool loadTraceFromFile(
            const std::string& filename,
            std::vector<RuntimeInstruction>& trace
        );

        static bool saveTraceToFile(
            const std::string& filename,
            const std::vector<RuntimeInstruction>& trace
        );

        void setSideBySide(
            bool value
        )
        {
            m_sideBySide = value;

            m_toolbar.setSideBySideState(
                value
            );
        }

        bool sideBySide() const
        {
            return m_sideBySide;
        }

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
        }

        void setTraceB(
            std::vector<RuntimeInstruction> trace
        )
        {
            m_traceB =
                std::move(
                    trace
                );
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

            m_listView.setSelectedIndex(
                index
            );

            m_listView.requestScrollToSelected();
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

    private:
        void drawTraceSide(
            const char* childId,
            const std::vector<RuntimeInstruction>& trace,
            const std::vector<TraceComparisonDisplayEntry>& displayEntries,
            bool scrollToSelected
        );

        void handleKeyboardNavigation();

        void selectPreviousDifference();
        void selectNextDifference();

        void selectFirstDifference();

        size_t findDifferenceStart(
            size_t index
        ) const;

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

        bool m_scrollToSelectedTrace =
            false;

        bool m_sideBySide =
            true;

        TraceComparisonToolbar
            m_toolbar;

        TraceListView
            m_listView;

        TraceDetailView
            m_detailView;

        TraceRecordView
            m_recordView;
    };
}