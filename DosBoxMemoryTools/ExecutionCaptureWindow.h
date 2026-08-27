#pragma once

#include "MemoryScanner.h"
#include "RecordButton.h"

#include <string>

namespace DosBoxMemoryTools
{
    class ExecutionCaptureWindow
    {
    public:
        ExecutionCaptureWindow(
            MemoryScanner& scanner
        );

        void draw(
            bool* isOpen
        );

    private:
        void loadSession();
        void saveSession() const;

        MemoryScanner&
            m_scanner;

        char m_targetText[32] =
            "0xEC4B";

        bool m_captureHit =
            false;

        RuntimeInstruction
            m_capture{};

        MyImGui::RecordButton
            m_executionRecordButton;
    };
}