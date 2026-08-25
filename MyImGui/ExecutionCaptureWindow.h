#pragma once

#include "DosBoxMemoryScanner.h"
#include "RecordButton.h"

#include <string>

namespace MyImGui
{
    class ExecutionCaptureWindow
    {
    public:
        ExecutionCaptureWindow(
            DosBoxMemoryScanner& scanner
        );

        void draw(
            bool* isOpen
        );

    private:
        void loadSession();
        void saveSession() const;

        DosBoxMemoryScanner&
            m_scanner;

        char m_targetText[32] =
            "0xEC4B";

        bool m_captureHit =
            false;

        RuntimeInstruction
            m_capture{};

        RecordButton
            m_executionRecordButton;
    };
}