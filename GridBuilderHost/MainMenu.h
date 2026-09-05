#pragma once

namespace GridBuilderHost
{
    class HostSettings;

    class MainMenu
    {
    public:
        explicit MainMenu(
            HostSettings& hostSettings
        );

        void draw();

        bool consumeOpenSettingsRequest();

        bool showDosBoxCoordinates() const;

    private:
        bool m_openSettingsRequested =
            false;

        bool m_showDosBoxCoordinates =
            false;

        HostSettings& m_hostSettings;
    };
}