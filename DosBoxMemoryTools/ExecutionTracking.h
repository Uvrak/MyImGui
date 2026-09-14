#pragma once

#include "MemoryScanner.h"
#include "RecordButton.h"

#include "ScannerAddress.h"

namespace DosBoxMemoryTools
{
    class ExecutionTracking
    {
    public:
        ExecutionTracking(
            MemoryScanner& scanner,
            ScannerAddress& scannerAddress
        );

        void draw();

    private:
        MemoryScanner&
            m_scanner;

        ScannerAddress&
            m_scannerAddress;

        MyImGui::RecordButton
            m_recordButton;

        RuntimeInstruction
            m_executionCapture;

        bool m_hasExecutionCapture =
            false;

        bool m_waitingForTrigger = false;
    };
}