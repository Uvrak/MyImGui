#pragma once
#include "ScannerAddress.h"
#include "ScannerRange.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include "MemoryScanner.h"
#include "RecordButton.h"

namespace DosBoxMemoryTools
{
    class MemoryWriteTracker
    {
    public:
        explicit MemoryWriteTracker(
            MemoryScanner& scanner
        );

        // MemoryWriteTracker.h

        void draw(
            const ScannerAddress& scannerAddress,
            const ScannerRange& scannerRange
        );

    private:
        MemoryScanner&
            m_scanner;

        bool m_captureHit =
            false;

        RuntimeInstruction
            m_capture{};

        MyImGui::RecordButton
            m_recordButton;

        uint8_t m_previousValue =
            0;

        bool m_hasPreviousValue =
            false;

        bool m_valueChanged =
            false;

        size_t m_lastDisplayedCaptureCount = 0;

        std::vector<RuntimeInstruction>
            m_captures;
    };
}