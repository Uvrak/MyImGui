#include "pch.h"
#include "SettingsWindow.h"

#include "imgui.h"

namespace MyImGui
{
    void SettingsWindow::open()
{
    m_open =
        true;

    ImGui::SetNextWindowFocus();
}

    void SettingsWindow::draw()
    {
        if (!m_open)
        {
            return;
        }

        if (ImGui::Begin(
            "Settings",
            &m_open
        ))
        {
            ImGui::InputFloat(
                "Font Size",
                &m_fontSize,
                1.0f,
                2.0f,
                "%.0f"
            );
        }

        ImGui::End();
    }
    bool SettingsWindow::isOpen() const
    {
        return m_open;
    }
    
    float SettingsWindow::fontSize() const
    {
        return m_fontSize;
    }
    
    void SettingsWindow::setFontSize(
        float fontSize
    )
    {
        m_fontSize =
            fontSize;
    }
}
