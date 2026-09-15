#pragma once

#include "MemoryScanner.h"
#include "RecordButton.h"
#include "ScannerAddress.h"

#include <string>
#include <vector>

namespace DosBoxMemoryTools
{
    class TraceTracking
    {
    public:
        explicit TraceTracking(
            MemoryScanner& scanner
        );

        void draw(
            ScannerAddress& scannerAddress
        );

        const std::vector<RuntimeInstruction>&
            trace() const;

        bool targetDatasetA() const;

        bool takeCompletedTrace();

    private:
        void drawRecorder(
            ScannerAddress& scannerAddress
        );

        void updateCapture();

        void beginLoadTrace();

        void continueLoadTrace();

        MemoryScanner&
            m_scanner;

        MyImGui::RecordButton
            m_recordButton;

        std::vector<RuntimeInstruction>
            m_trace;

        bool m_traceWasArmedOrActive =
            false;

        enum class TargetDataset
        {
            A,
            B
        };

        TargetDataset m_targetDataset =
            TargetDataset::A;

        bool m_traceCompleted =
            false;

        size_t m_traceInstructionLimit =
            1000;

        bool m_traceLoadPending =
            false;

        size_t m_traceLoadCount =
            0;

        size_t m_traceLoadIndex =
            0;
    };
}