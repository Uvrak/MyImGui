#pragma once

#include <string>

namespace MyImGui
{
    class MainMenu
    {
    public:
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

    private:
        std::string m_selectedGameExe;

        std::string m_gameDirectory;
        std::string m_gameFilename;

        std::string m_mountDirectory;
        std::string m_dosDirectory;

        bool m_startGameRequested = false;
    };
}