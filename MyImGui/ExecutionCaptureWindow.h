#pragma once

#include "DosBoxMemoryScanner.h"

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
        DosBoxMemoryScanner&
            m_scanner;

        char m_targetText[32] =
            "0xEC4B";

        bool m_captureHit =
            false;

        RuntimeInstruction
            m_capture{};
    };
}
