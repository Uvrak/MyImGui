#pragma once

namespace MyImGui
{
    class SettingsWindow;
}

namespace GridBuilderHost
{
    class HostSettings;

    class HostFontSettings
    {
    public:
        HostFontSettings(
            MyImGui::SettingsWindow& settingsWindow,
            HostSettings& hostSettings
        );

        void update();

    private:
        MyImGui::SettingsWindow& m_settingsWindow;
        HostSettings& m_hostSettings;
    };
}