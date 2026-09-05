#include "HostFontSettings.h"

#include "HostSettings.h"
#include "SettingsWindow.h"

#include "imgui.h"

namespace GridBuilderHost
{
    HostFontSettings::HostFontSettings(
        MyImGui::SettingsWindow& settingsWindow,
        HostSettings& hostSettings
    )
        :
        m_settingsWindow(
            settingsWindow
        ),
        m_hostSettings(
            hostSettings
        )
    {}

    void HostFontSettings::update()
    {
        const float fontSize =
            m_settingsWindow.fontSize();

        ImGuiIO& io =
            ImGui::GetIO();

        io.FontGlobalScale =
            fontSize /
            18.0f;

        m_hostSettings.setFontSize(
            fontSize
        );
    }
}