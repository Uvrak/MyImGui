#include "EditorToolbox.h"

#include "imgui.h"
#include <cstdint>

EditorToolbox::EditorToolbox(
    SDL_Renderer* renderer
)
    : m_window(
        "Editor Toolbox",
        {
            .movable = true,
            .resizable = true,
            .collapsible = true,
            .closable = false,
            .titleBar = true,
            .autoResizeHeight = false
        }
    ),
    m_renderer(renderer)
{
    m_buttons =
    {
        { ICON_FA_PENCIL, "Pencil", EditorTool::Pencil },
        { ICON_FA_ERASER, "Eraser", EditorTool::Eraser },
        
        { ICON_FA_HAND,   "Pan", EditorTool::Scroll }
    };
}

EditorToolbox::~EditorToolbox()
{
    
}


void EditorToolbox::draw(
    EditorTool activeTool
)
{
    ImGui::SetNextWindowSize(
        ImVec2(170.0f, 70.0f),
        ImGuiCond_FirstUseEver
    );

    const bool windowVisible =
        m_window.begin();

    if (windowVisible)
    {
        m_dragDropReorder.begin();
        m_flowLayout.begin();

        for (std::size_t index = 0;
            index < m_buttons.size();
            ++index)
        {
            m_flowLayout.beginItem(
                ImVec2(40.0f, 40.0f),
                ImGui::GetStyle().ItemSpacing.x
            );

            drawToolButton(
                m_buttons[index]
            );

            m_dragDropReorder.handleItem(
                index
            );

            m_flowLayout.endItem();
        }

        m_dragDropReorder.apply(
            m_buttons
        );

        m_flowLayout.end();
    }

    m_window.end();
}
EditorTool EditorToolbox::activeTool() const
{
    return m_activeTool;
}

void EditorToolbox::drawToolButton(
    const ToolButton& button
)
{
    const bool isActive =
        m_activeTool ==
        button.tool;

    if (isActive)
    {
        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImGui::GetStyleColorVec4(
                ImGuiCol_ButtonActive
            )
        );
    }

    ImGui::PushID(
        static_cast<int>(
            button.tool
            )
    );

    ImVec4 iconColor;

    switch (button.tool)
    {
    case EditorTool::Pencil:
        iconColor = ImVec4(
            1.0f,
            0.8f,
            0.2f,
            1.0f
        );
        break;

    case EditorTool::Eraser:
        iconColor = ImVec4(
            1.0f,
            0.45f,
            0.45f,
            1.0f
        );
        break;

    case EditorTool::Scroll:
        iconColor = ImVec4(
            0.4f,
            0.75f,
            1.0f,
            1.0f
        );
        break;
    }

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        iconColor
    );

    if (ImGui::Button(
        button.icon,
        ImVec2(40.0f, 40.0f)
    ))
    {
        m_activeTool =
            button.tool;
    }

    ImGui::PopStyleColor();

    ImGui::PopID();

    if (isActive)
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "%s",
            button.tooltip
        );
    }
}

void EditorToolbox::setActiveTool(
    EditorTool tool
)
{
    m_activeTool =
        tool;
}

const char*
EditorToolbox::activeToolIcon() const
{
    for (const ToolButton& button :
        m_buttons)
    {
        if (button.tool ==
            m_activeTool)
        {
            return button.icon;
        }
    }

    return "";
}