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
    explicit EditorToolbox(SDL_Renderer* renderer);
    ~EditorToolbox();

    void draw(EditorTool activeTool);

    EditorTool activeTool() const;

    void setActiveTool(EditorTool tool);

private:
    void drawToolButton(const ToolButton& button);

private:

    std::vector<ToolButton> m_buttons;

    EditorTool m_activeTool = EditorTool::Pencil;

    MyImGui::FloatingWindow m_window;

    SDL_Renderer* m_renderer = nullptr;

    MyImGui::FlowLayout m_flowLayout;

    MyImGui::DragDropReorder
        m_dragDropReorder;
   
};