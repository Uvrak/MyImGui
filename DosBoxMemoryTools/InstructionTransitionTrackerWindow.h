#pragma once

#include <string>
#include <vector>
#include <utility>
#include <array>
#include <functional>

#include "MemoryScanner.h"
#include "RecordButton.h"

#include "InstructionTransitionTrackerNavigationWindow.h"

namespace DosBoxMemoryTools
{
    class InstructionTransitionTrackerWindow
    {
    public:
        InstructionTransitionTrackerWindow(
            MemoryScanner& scanner,
            const std::string& gameId
        );

        void draw(
            bool* isOpen
        );

        void saveSession() const;
        void loadSession();

        void setGameId(
            const std::string& gameId
        );

        void captureTransitions();

        void setSelectedHistoryInstruction(
            size_t* selectedHistoryInstruction
        );

    private:
        MemoryScanner&
            m_scanner;

        char m_transitionTargetText[32] =
            "0xEC4B";

        char m_executionCaptureTargetText[32] =
            "0x30684";

        bool m_executionCaptureHit =
            false;

        RuntimeInstruction
            m_executionCapture;

        std::vector<std::pair<size_t, size_t>>
            m_transitions;

        std::vector<std::pair<uint16_t, uint16_t>>
            m_transitionContexts;

        std::vector<std::array<uint8_t, 16>>
            m_transitionBytes;

        std::vector<
            std::vector<RuntimeInstruction>
        >
            m_transitionHistories;

        std::vector<RuntimeInstruction>
            m_transitionNextInstructions;

        std::string m_gameId;

        InstructionTransitionTrackerNavigationWindow
            m_navigationWindow;

        size_t
            m_selectedHistoryInstruction = 0;

        bool
            m_scrollToSelectedHistoryInstruction = false;

    };
}
