#pragma once

#include "MemoryScanner.h"
#include "RecordButton.h"

namespace DosBoxMemoryTools
{
    class ExecutionTracking
    {
    public:
        ExecutionTracking(
            MemoryScanner& scanner
        );

        void draw();

    private:
        MemoryScanner&
            m_scanner;

        MyImGui::RecordButton
            m_recordButton;

        char m_targetText[32] =
            "0xBD3F";

        RuntimeInstruction
            m_executionCapture;

        bool m_hasExecutionCapture =
            false;
    };
}