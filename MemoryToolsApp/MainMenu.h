#pragma once

#include "ScannerMenuBar.h"
#include "ScannerAddress.h"
#include "ScannerRange.h"
#include "ScannerSettings.h"

#include <string>

namespace MyImGui
{
    class MainMenu
    {
    public:
        MainMenu();

        void draw();

        const std::string&
            selectedGameExe() const;

        bool consumeStartGameRequest();

        const std::string&
            mountDirectory() const;

        const std::string&
            dosDirectory() const;

        const std::string&
            gameFilename() const;

        bool consumeGermanKeyboardLayoutRequest();
        bool consumeUSKeyboardLayoutRequest();
        bool consumeOpenSettingsRequest();

        DosBoxMemoryTools::ScannerAddress&
            scannerAddress();

        DosBoxMemoryTools::ScannerRange&
            scannerRange();

    private:
        std::string m_selectedGameExe;

        std::string m_gameDirectory;
        std::string m_gameFilename;

        std::string m_mountDirectory;
        std::string m_dosDirectory;

        bool m_startGameRequested = false;

        bool m_germanKeyboardLayoutRequested = false;
        bool m_usKeyboardLayoutRequested = false;

        bool m_germanKeyboardLayoutSelected = true;

        bool m_openSettingsRequested = false;

        DosBoxMemoryTools::ScannerAddress
            m_scannerAddress;

        DosBoxMemoryTools::ScannerRange
            m_scannerRange;

        DosBoxMemoryTools::ScannerMenuBar
            m_scannerMenuBar;

        DosBoxMemoryTools::ScannerSettings
            m_scannerSettings;
    };
}