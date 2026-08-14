#include "pch.h"
#include "MyImGuiSettings.h"

#include <filesystem>
#include <fstream>

#include "imgui.h"
#include "MyImGuiSettingsWindow.h"

namespace MyImGui
{
    MyImGuiSettings::
        MyImGuiSettings()
    {
        load();
    }

    MyImGuiSettings::
        ~MyImGuiSettings()
    {
        save();
    }
    void MyImGuiSettings::apply() const
    {
        ImGuiIO& io =
            ImGui::GetIO();

        io.FontGlobalScale =
            m_fontScale;
    }

    void MyImGuiSettings::setFontScale(
        float scale
    )
    {
        m_fontScale =
            scale;
    }

    float MyImGuiSettings::fontScale() const
    {
        return m_fontScale;
    }
    void MyImGuiSettings::load()
    {
        std::ifstream file(
            "settings/myimgui_settings.cfg"
        );

        if (!file)
        {
            return;
        }

        file >>
            m_fontScale;
    }

    void MyImGuiSettings::save() const
    {
        std::filesystem::create_directories(
            "settings"
        );

        std::ofstream file(
            "settings/myimgui_settings.cfg"
        );

        if (!file)
        {
            return;
        }

        file <<
            m_fontScale;
    }
}
