#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <vector>

#include "IconsFontAwesome7.h"
#include "EditorTool.h"

#include "MyImGui.h"

class EditorToolbox
{
public:

    const char* activeToolIcon() const;

private:
    struct ToolButton
    {
        const char* icon;
        const char* tooltip;
        EditorTool tool;
    };

public:
    EditorToolbox();
    ~EditorToolbox();

    void draw(EditorTool activeTool);
    void setTextLabels(bool enabled) { m_textLabels = enabled; }

    EditorTool activeTool() const;

    void setActiveTool(EditorTool tool);

private:
    void drawToolButton(const ToolButton& button);

private:

    std::vector<ToolButton> m_buttons;

    bool m_textLabels = false;
    EditorTool m_activeTool = EditorTool::Pencil;

    MyImGui::FloatingWindow m_window;

    MyImGui::FlowLayout m_flowLayout;

    MyImGui::DragDropReorder
        m_dragDropReorder;
};