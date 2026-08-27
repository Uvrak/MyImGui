#pragma once

#include <cstddef>
#include <vector>
#include <functional>

#include "MemoryScanner.h"
#include "RecordButton.h"
#include "FloatingWindow.h"

namespace DosBoxMemoryTools
{
    class InstructionTransitionTrackerNavigationWindow
    {
    public:
        InstructionTransitionTrackerNavigationWindow() =
            default;

        void draw(
            bool* isOpen
        );

        void setScanner(
            MemoryScanner* scanner
        );

        void setTargetText(
            char* targetText,
            size_t targetTextSize
        );

        void setTransitionHistories(
            std::vector<
            std::vector<RuntimeInstruction>
            >* transitionHistories
        );

        void setCaptureCallback(
            std::function<void()> callback
        );

        void setExecutionCaptureCallback(
            std::function<void()> callback
        );

        void setSelectedHistoryInstruction(size_t* selectedHistoryInstruction);

        bool takeHistoryNavigation();
    private:
        MemoryScanner*
            m_scanner = nullptr;

        char*
            m_targetText = nullptr;

        size_t
            m_targetTextSize = 0;

        MyImGui::RecordButton
            m_transitionRecordButton;

        MyImGui::RecordButton
            m_executionRecordButton;

        std::vector<
            std::vector<RuntimeInstruction>
        >* m_transitionHistories = nullptr;

        std::function<void()>
            m_captureCallback;

        size_t
            m_lastTransitionCount = 0;

        double
            m_lastTransitionChangeTime = 0.0;

        bool
            m_transitionSeen = false;

        std::function<void()>
            m_executionCaptureCallback;

        size_t
            m_selectedTransitionHistory = 0;

        size_t*
            m_selectedHistoryInstruction = nullptr;

        bool
            m_historyNavigation = false;

        MyImGui::FloatingWindow m_window{
            "Transition Navigation",
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

        MyImGui::RecordButton
            m_memoryWriteRecordButton;
    };
}