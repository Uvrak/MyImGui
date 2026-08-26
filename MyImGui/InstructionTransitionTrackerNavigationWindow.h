#pragma once

#include <cstddef>
#include <vector>
#include <functional>

#include "DosBoxMemoryScanner.h"
#include "RecordButton.h"
#include "FloatingWindow.h"

namespace MyImGui
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
            DosBoxMemoryScanner* scanner
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

    private:
        DosBoxMemoryScanner*
            m_scanner = nullptr;

        char*
            m_targetText = nullptr;

        size_t
            m_targetTextSize = 0;

        RecordButton
            m_transitionRecordButton;

        RecordButton
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

        FloatingWindow m_window{
            "Transition Navigation",
            FloatingWindowOptions{
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