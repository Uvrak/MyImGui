#pragma once

#include "MemoryScanner.h"
#include "ScannerAddress.h"
#include "ScannerRange.h"
#include "RecordButton.h"

namespace DosBoxMemoryTools
{
    class MemoryReadTracker
    {
    public:
        explicit MemoryReadTracker(
            MemoryScanner& scanner
        );

        void draw(
            ScannerAddress& scannerAddress,
            const ScannerRange& scannerRange
        );

    private:
        MemoryScanner&
            m_scanner;

        MyImGui::RecordButton
            m_recordButton;

        bool m_captureHit =
            false;

        RuntimeInstruction
            m_capture{};
    };
}