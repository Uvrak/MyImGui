#include "TraceComparisonToolbar.h"
#include "imgui.h"

namespace DosBoxMemoryTools
{
    void TraceComparisonToolbar::draw()
    {
        if (ImGui::SmallButton("Save") &&
            m_save)
        {
            m_save();
        }


        ImGui::SameLine();

        if (ImGui::SmallButton("Prev Diff") &&
            m_prevDiff)
        {
            m_prevDiff();
        }

        ImGui::SameLine();

        if (ImGui::SmallButton("Next Diff") &&
            m_nextDiff)
        {
            m_nextDiff();
        }

        if (ImGui::IsItemHovered() &&
            ImGui::IsMouseClicked(1) &&
            m_focusFilter)
        {
            m_focusFilter();
        }
    }
}
