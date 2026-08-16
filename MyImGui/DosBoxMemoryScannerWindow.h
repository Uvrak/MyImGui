#pragma once

#include <vector>
#include <unordered_set>

#include "DosBoxMemoryScanner.h"
#include "FloatingWindow.h"
#include "FlowLayout.h"

#include <string>
#include <unordered_map>

namespace MyImGui
{
    class DosBoxView;

    class DosBoxMemoryScannerWindow
    {
    public:
        DosBoxMemoryScannerWindow(
            DosBoxMemoryReader& memoryReader,
            const std::string& gameId,
            DosBoxView* dosBoxView
        );

        void draw(
            bool* isOpen,
            bool& liveView
        );

        void setGameId(
            const std::string& gameId
        );

        bool refreshMemory();

        bool takeSelectedAddress(
            size_t& address
        );

    private:
        DosBoxMemoryScanner
            m_scanner;

        FloatingWindow m_toolbarWindow{
    "Memory Scanner Toolbar",
    FloatingWindowOptions{
        true,   // movable
        false,  // resizable
        false,  // collapsible
        false,  // closable
        true,   // titleBar
        true,   // autoResizeHeight
        true   // dockable
        }
    };

        FlowLayout m_toolbarLayout;

        DosBoxMemoryScanMode m_scanMode =
            DosBoxMemoryScanMode::Changed;

        int m_exactValue = 0;

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

        std::unordered_map<size_t, std::string>
            m_pinnedDescriptions;

        std::unordered_set<size_t>
            m_selectedAddresses;

        char m_addressSearch[32] = {};
        size_t m_foundAddress = 0;
        bool m_hasFoundAddress = false;
        bool m_addressSearchAttempted = false;
        std::string m_gameId;
        std::string pinnedAddressesFilePath() const;

        char m_descriptionBuffer[256] = {};
        size_t m_descriptionAddress = 0;

        bool hasDescription(
            size_t address
        ) const;

        bool m_descriptionsFirst = false;

        size_t m_writeAddress = 0;
        int m_writeValue = 0;
        bool m_showWriteValuePopup = false;

        DosBoxView* m_dosBoxView = nullptr;

        size_t m_lastSelectedAddress = 0;
        bool m_hasSelectedAddress = false;
    };


}