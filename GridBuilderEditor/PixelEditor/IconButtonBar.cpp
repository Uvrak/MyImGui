#include "IconButtonBar.h"

#include "imgui.h"

#include <utility>

IconButtonBar::IconButtonBar(
    std::vector<IconButtonDefinition> definitions
)
{
    m_buttons.reserve(definitions.size());

    for (std::size_t index = 0;
        index < definitions.size();
        ++index)
    {
        Button button;

        button.definition =
            std::move(definitions[index]);

        button.id =
            "##IconButton" +
            std::to_string(index);

        m_buttons.push_back(
            std::move(button)
        );
    }
}

bool IconButtonBar::draw(
    int& activeValue
)
{
    bool changed = false;

    for (std::size_t index = 0;
        index < m_buttons.size();
        ++index)
    {
        Button& button = m_buttons[index];

        const bool isActive =
            button.definition.value ==
            activeValue;

        if (isActive)
        {
            ImGui::PushStyleColor(
                ImGuiCol_Button,
                IM_COL32(45, 110, 190, 255)
            );
        }

        const std::string label =
            button.definition.icon +
            button.id;

        if (ImGui::Button(
            label.c_str(),
            ImVec2(
                button.definition.width,
                button.definition.height
            )
        ))
        {
            if (activeValue !=
                button.definition.value)
            {
                activeValue =
                    button.definition.value;

                changed = true;
            }
        }

        if (isActive)
        {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemHovered() &&
            !button.definition.tooltip.empty())
        {
            ImGui::SetTooltip(
                "%s",
                button.definition.tooltip.c_str()
            );
        }

        if (index + 1 < m_buttons.size())
        {
            ImGui::SameLine();
        }
    }

    return changed;
}