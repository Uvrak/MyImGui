#pragma once

#include <vector>
#include <unordered_set>

#include "MemoryScanner.h"
#include "FloatingWindow.h"
#include "FlowLayout.h"
#include "View.h"

#include <string>
#include <unordered_map>


namespace DosBoxMemoryTools
{
    class MemoryScannerWindow
    {
    public:
        MemoryScannerWindow(
            MemoryReader& memoryReader,
            const std::string& gameId,
            DosBoxX::View* dosBoxView
        );

        void draw(
            bool* isOpen,
            bool& liveView
        );

        void pinAddresses(
            const std::vector<size_t>& addresses
        );

        void setGameId(
            const std::string& gameId
        );

        bool refreshMemory();

        void refreshPinnedValues();

        bool takeSelectedAddress(
            size_t& address
        );

        MemoryScanner& scanner();

    private:
        MemoryScanner
            m_scanner;

        MyImGui::FloatingWindow m_toolbarWindow{
    "Memory Scanner Toolbar",
    MyImGui::FloatingWindowOptions{
        true,   // movable
        false,  // resizable
        false,  // collapsible
        false,  // closable
        true,   // titleBar
        true,   // autoResizeHeight
        true   // dockable
        }
    };

        MyImGui::FlowLayout m_toolbarLayout;

        MemoryScanMode m_scanMode =
            MemoryScanMode::Changed;

        int m_exactValue = 0;

        bool m_limitScanRange = false;

        char m_scanStartAddress[32] = {};
        char m_scanEndAddress[32] = {};

        bool m_filterPrevious = false;
        int m_previousValue = 0;

        bool m_filterCurrent = false;
        int m_currentValue = 0;

        bool m_filterDifference = false;
        int m_differenceValue = 0;

        std::unordered_set<size_t>
            m_pinnedAddresses;

    private:
        void loadPinnedAddresses();
        void savePinnedAddresses() const;

        void loadScannerSettings();
        void saveScannerSettings() const;

        std::unordered_map<size_t, std::string>
            m_pinnedDescriptions;

        std::unordered_set<size_t>
            m_selectedAddresses;

        char m_addressSearch[32] = {};
        size_t m_foundAddress = 0;
        bool m_hasFoundAddress = false;
        bool m_addressSearchAttempted = false;
        bool m_searchPerformed = false;
        std::string m_gameId;
        std::string pinnedAddressesFilePath() const;
        std::string scannerSettingsFilePath() const;

        char m_descriptionBuffer[256] = {};
        size_t m_descriptionAddress = 0;

        bool hasDescription(
            size_t address
        ) const;

        bool m_descriptionsFirst = false;

        size_t m_writeAddress = 0;
        int m_writeValue = 0;
        bool m_showWriteValuePopup = false;

        DosBoxX::View* m_dosBoxView = nullptr;

        size_t m_lastSelectedAddress = 0;
        bool m_hasSelectedAddress = false;

        MemoryValueType m_valueType =
            MemoryValueType::Byte;

        bool m_deleteOtherDifferenceCandidates = false;
        bool m_applyDifferenceDeleteRequested = false;

        std::unordered_map<size_t, uint8_t>
            m_pinnedDisplayValues;

        void refreshPinnedDisplayValues();
    };

}