#pragma once

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

        void draw(
            char* targetText,
            size_t targetTextSize
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