#pragma once

#include <vector>
#include <cstddef>
#include <string>
#include <utility>

#include "MemoryScanner.h"
#include "RecordButton.h"

namespace DosBoxMemoryTools
{
    class MemoryScannerWindow;

    class MemoryReadTrackerWindow
    {
    public:
        MemoryReadTrackerWindow(
            MemoryScanner& scanner,
            MemoryScannerWindow& scannerWindow,
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


    private:
        MemoryScanner&
            m_scanner;

        MemoryScannerWindow&
            m_scannerWindow;

        MyImGui::RecordButton
            m_idleRecordButton;

        MyImGui::RecordButton
            m_attackRecordButton;

        std::string
            m_gameId;

        std::vector<size_t>
            m_idleReadAddresses;

        std::vector<size_t>
            m_attackReadAddresses;

        std::vector<size_t>
            m_attackOnlyReadAddresses;

        std::vector<size_t>
            m_previousAttackOnlyReadAddresses;

        std::vector<std::pair<size_t, size_t>>
            m_attackReadInstructions;

        bool m_limitAddressRange = false;

        char m_rangeStartText[32] = "0x2BF20";
        char m_rangeEndText[32] = "0x2BF40";

        bool m_limitInstructionRange = false;

        char m_instructionRangeStartText[32] = "0xECCF";
        char m_instructionRangeEndText[32] = "0xECCF";
	};
}
