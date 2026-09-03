#pragma once

namespace GridBuilderHost
{
    class MainMenu
    {
    public:
        void draw();

        bool consumeOpenSettingsRequest();

    private:
        bool m_openSettingsRequested =
            false;

    };
}