#include "DebugWindow.h"

#include "imgui.h"

namespace GridBuilderHost
{
    void DebugWindow::clear()
    {
        m_lines.clear();
    }

    void DebugWindow::addLine(
        const std::string& text
    )
    {
        m_lines.push_back(
            text
        );
        m_scrollToEnd = true;
    }

    void DebugWindow::setLine(
        const std::string& text
    )
    {
        m_lines.clear();

        m_lines.push_back(
            text
        );
    }

    void DebugWindow::draw()
    {
        ImGui::Begin(
            m_title.c_str()
        );

        for (const std::string& line : m_lines)
        {
            ImGui::TextUnformatted(
                line.c_str()
            );
        }

        if (m_scrollToEnd)
        {
            ImGui::SetScrollHereY(1.0f);
            m_scrollToEnd = false;
        }

        ImGui::End();
    }
}
