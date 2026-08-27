#pragma once

#include <cstddef>
#include <vector>

#include "MemoryScanner.h"
#include "RecordButton.h"
#include "FloatingWindow.h"

namespace DosBoxMemoryTools
{
    class ExecutionTraceNavigationWindow
    {
    public:
        ExecutionTraceNavigationWindow() = default;

        void draw(
            bool* isOpen
        );

        bool saveTraceRequested();
        bool loadTraceRequested();

        void setScanner(
            MemoryScanner* scanner
        );

        void setRecordButton(
            MyImGui::RecordButton* recordButton
        );

        void setTargetText(
            char* targetText,
            size_t targetTextSize
        );

        void setSelectedTraceIndex(
            size_t* selectedTraceIndex
        );

        void setTrace(
            std::vector<RuntimeInstruction>* trace
        );

        void setScrollToSelectedTrace(
            bool* scrollToSelectedTrace
        );

    private:
        MemoryScanner*
            m_scanner = nullptr;

        MyImGui::RecordButton*
            m_recordButton = nullptr;

        char*
            m_targetText = nullptr;

        size_t
            m_targetTextSize = 0;

        size_t*
            m_selectedTraceIndex = nullptr;

        int
            m_findInstructionIndex = 0;

        char m_memoryAddressText[32] =
            "0x9306";

        int
            m_previousRegisterIndex = 5;

        std::vector<RuntimeInstruction>*
            m_trace = nullptr;

        bool*
            m_scrollToSelectedTrace = nullptr;

        bool m_saveTraceRequested = false;
        bool m_loadTraceRequested = false;

        MyImGui::FloatingWindow m_window{
    "Trace Navigation",
    MyImGui::FloatingWindowOptions{
        true,   // movable
        true,   // resizable
        true,   // collapsible
        true,   // closable
        true,   // titleBar
        false,  // autoResizeHeight
        true    // dockable
        }
    };
    };
}
