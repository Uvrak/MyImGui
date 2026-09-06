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

        void draw(
            const char* targetText
        );

    private:
        MemoryScanner&
            m_scanner;

        MyImGui::RecordButton
            m_recordButton;
    };
}