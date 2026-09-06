#pragma once

#include "MemoryScanner.h"
#include "RecordButton.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace DosBoxMemoryTools
{
    class TransitionTracking
    {
    public:
        TransitionTracking(
            MemoryScanner& scanner
        );

        void draw(
            char* targetText,
            size_t targetTextSize
        );

        void drawNavigation();

        void drawRecorder(
            const char* targetText
        );

    private:
        bool drawRecordButton();

        bool recording() const;

        void stopRecording();

        void startRecording(
            size_t targetAddress
        );

        void stop();

        void captureTransitions();

        bool updateAutoCapture();

        bool getTransitionCount(
            size_t& count
        ) const;

        bool getTransitions(
            std::vector<std::pair<size_t, size_t>>& transitions
        ) const;

        bool getTransitionHistory(
            size_t index,
            std::vector<RuntimeInstruction>& history
        ) const;

        bool getTransitionNextInstruction(
            size_t index,
            RuntimeInstruction& instruction
        ) const;

        bool getTransitionContexts(
            std::vector<std::pair<uint16_t, uint16_t>>& contexts
        ) const;

        bool getTransitionBytes(
            std::vector<std::array<uint8_t, 16>>& bytes
        ) const;

        MemoryScanner&
            m_scanner;

        MyImGui::RecordButton
            m_recordButton;

        std::vector<std::pair<size_t, size_t>>
            m_transitions;

        std::vector<std::pair<uint16_t, uint16_t>>
            m_transitionContexts;

        std::vector<std::array<uint8_t, 16>>
            m_transitionBytes;

        std::vector<std::vector<RuntimeInstruction>>
            m_transitionHistories;

        std::vector<RuntimeInstruction>
            m_transitionNextInstructions;

        size_t m_lastTransitionCount = 0;

        double m_lastTransitionChangeTime = 0.0;

        bool m_transitionSeen = false;

        size_t m_selectedHistoryInstruction =
            static_cast<size_t>(-1);

        bool m_scrollToSelectedHistoryInstruction =
            false;
    };
}