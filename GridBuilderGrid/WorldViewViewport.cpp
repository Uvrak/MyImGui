#include "WorldViewViewport.h"
#include "WorldViewTypes.h"

#include "Cell.h"
#include "Chunk.h"
#include <cstdio>

namespace WorldView
{
int edgeTextureSize(const Viewport& viewport)
{
    return (std::max)(
        1,
        static_cast<int>(
            std::round(viewport.m_cellSize)
            )
    );
}

void updateGridView(Viewport& viewport)
{
    viewport.m_gridView.startX =
        std::fmod(viewport.m_cameraX, viewport.m_cellSize);

    viewport.m_gridView.startY =
        std::fmod(viewport.m_cameraY, viewport.m_cellSize);

    if (viewport.m_gridView.startX < 0.0f)
        viewport.m_gridView.startX += viewport.m_cellSize;

    if (viewport.m_gridView.startY < 0.0f)
        viewport.m_gridView.startY += viewport.m_cellSize;

    viewport.m_gridView.firstVisibleCellX =
        static_cast<int>(std::floor(
            viewport.m_cameraX / viewport.m_cellSize));

    viewport.m_gridView.firstVisibleCellY =
        static_cast<int>(std::floor(
            viewport.m_cameraY / viewport.m_cellSize));
}

bool isMouseInsideCanvas(
    ImVec2 canvasPosition,
    ImVec2 canvasSize,
    ImVec2& mouseCanvasPosition
)
{
    ImVec2 mousePosition = ImGui::GetMousePos();

    mouseCanvasPosition = ImVec2(
        mousePosition.x - canvasPosition.x,
        mousePosition.y - canvasPosition.y
    );

    return
        mouseCanvasPosition.x >= 0.0f &&
        mouseCanvasPosition.y >= 0.0f &&
        mouseCanvasPosition.x < canvasSize.x &&
        mouseCanvasPosition.y < canvasSize.y;
}

void handleZoom(Viewport& viewport, const ToolSettings& toolSettings,
    ImVec2 canvasPosition,
    ImVec2 canvasSize)
{
    ImVec2 mouseCanvasPosition;

    if (!WorldView::isMouseInsideCanvas(
        canvasPosition,
        canvasSize,
        mouseCanvasPosition))
    {
        return;
    }

    float mouseWheel = ImGui::GetIO().MouseWheel;

    if (toolSettings.m_activeTool == EditorTool::Pencil &&
        !ImGui::GetIO().KeyCtrl)
    {

        return;
    }
    if (mouseWheel != 0.0f)
    {
        viewport.m_cellSize += mouseWheel * 2.0f;

        if (viewport.m_cellSize < 5.0f)
        {
            viewport.m_cellSize = 5.0f;
        }

        if (viewport.m_cellSize > 100.0f)
        {
            viewport.m_cellSize = 100.0f;
        }
    }
}

int calculateLabelStep(const Viewport& viewport)
{
    if (viewport.m_cellSize < 15.0f)
        return 8;

    if (viewport.m_cellSize < 25.0f)
        return 4;

    if (viewport.m_cellSize < 40.0f)
        return 2;

    return 1;
}

void updateLongTickStep(Viewport& viewport)
{
    viewport.m_longTickStep = 4;

    while (true)
    {
        std::string left =
            std::to_string(viewport.m_gridView.firstVisibleCellX);

        std::string right =
            std::to_string(viewport.m_gridView.firstVisibleCellX +
                static_cast<int>(1000.0f / viewport.m_cellSize));

        float textWidth = (std::max)(
            ImGui::CalcTextSize(left.c_str()).x,
            ImGui::CalcTextSize(right.c_str()).x
        );

        float availableWidth =
            viewport.m_longTickStep * viewport.m_cellSize;

        if (textWidth + 6.0f <= availableWidth)
            break;

        viewport.m_longTickStep *= 2;
    }

    viewport.m_labelStep = viewport.m_longTickStep;
}

void centerOnPlayer(Viewport& viewport, const MapPlayerMarker& player,
     ImVec2 canvasSize
 )
{
     viewport.m_cameraX =
         (
             static_cast<float>(
                 player.x
                 ) +
             0.5f
             ) * viewport.m_cellSize -
         canvasSize.x * 0.5f;

     viewport.m_cameraY =
         (
             static_cast<float>(
                 player.y
                 ) +
             0.5f
             ) * viewport.m_cellSize -
         canvasSize.y * 0.5f;
 }

void handlePan(Viewport& viewport)
{
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        viewport.m_followPlayer = false;

        ImVec2 delta = ImGui::GetIO().MouseDelta;

        viewport.m_cameraX -= delta.x;
        viewport.m_cameraY -= delta.y;
    }
}
}
