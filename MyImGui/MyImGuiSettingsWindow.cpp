#include "pch.h"
#include "MyImGuiSettingsWindow.h"

#include "imgui.h"

namespace MyImGui
{
    MyImGuiSettingsWindow::
        MyImGuiSettingsWindow(
            MyImGuiSettings& settings
        )
        : m_settings(
            settings
        )
    {}

    void MyImGuiSettingsWindow::draw(
        bool* isOpen
    )
    {
        if (isOpen &&
            !*isOpen)
        {
            return;
        }

        if (!ImGui::Begin(
            "Settings",
            isOpen
        ))
        {
            ImGui::End();
            return;
        }

        ImGui::TextUnformatted(
            "Font Size"
        );

        float fontScalePercent =
            m_settings.fontScale() *
            100.0f;

        if (ImGui::SliderFloat(
            "##FontScale",
            &fontScalePercent,
            75.0f,
            200.0f,
            "%.0f %%"
        ))
        {
            m_settings.setFontScale(
                fontScalePercent /
                100.0f
            );

            m_settings.apply();
        }

        ImGui::End();
    }
}
