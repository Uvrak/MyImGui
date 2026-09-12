#include "WorldViewEraser.h"
#include "WorldViewHitTest.h"
#include "WorldViewViewport.h"
#include "WorldViewWallPainter.h"

#include "Cell.h"
#include "Chunk.h"
#include <cstdio>

namespace WorldView
{
void eraseArea(Viewport& viewport, Hover& hover, Eraser& eraser, ChunkManager& map, bool& dirty)
{
     bool changed = false;

     const int radius =
         eraser.m_eraserSize / 2;

     const float halfCellSize =
         viewport.m_cellSize * 0.5f;

     const float offsetX =
         hover.m_hoverLocalX >= halfCellSize
         ? 0.5f
         : 0.0f;

     const float offsetY =
         hover.m_hoverLocalY >= halfCellSize
         ? 0.5f
         : 0.0f;

     const float rectangleLeft =
         static_cast<float>(
             hover.m_hoveredCellX - radius
             ) + offsetX;

     const float rectangleTop =
         static_cast<float>(
             hover.m_hoveredCellY - radius
             ) + offsetY;

     const float rectangleRight =
         rectangleLeft +
         static_cast<float>(eraser.m_eraserSize);

     const float rectangleBottom =
         rectangleTop +
         static_cast<float>(eraser.m_eraserSize);

     const auto isInside =
         [&](float x, float y)
         {
             return
                 x >= rectangleLeft &&
                 x < rectangleRight &&
                 y >= rectangleTop &&
                 y < rectangleBottom;
         };

     const int firstCellX =
         static_cast<int>(
             std::floor(rectangleLeft)
             ) - 1;

     const int firstCellY =
         static_cast<int>(
             std::floor(rectangleTop)
             ) - 1;

     const int lastCellX =
         static_cast<int>(
             std::ceil(rectangleRight)
             );

     const int lastCellY =
         static_cast<int>(
             std::ceil(rectangleBottom)
             );

     for (int cellY = firstCellY;
         cellY <= lastCellY;
         ++cellY)
     {
         for (int cellX = firstCellX;
             cellX <= lastCellX;
             ++cellX)
         {
             const Cell* cell =
                 map.findCell(
                     cellX,
                     cellY,
                     map.activeLayer()
                 );

             if (cell == nullptr)
             {
                 continue;
             }

             if (cell->hasMisc() &&
                 isInside(
                     cellX + 0.5f,
                     cellY + 0.5f
                 ))
             {
                 const int originalLayer =
                     map.activeLayer();

                 const std::string erasedMiscId =
                     cell->miscId();

                 std::string pairedMiscId =
                     erasedMiscId;

                 int pairedLayerOffset = 0;

                 const std::size_t upPosition =
                     pairedMiscId.find("_up");

                 const std::size_t downPosition =
                     pairedMiscId.find("_down");

                 if (upPosition !=
                     std::string::npos)
                 {
                     pairedMiscId.replace(
                         upPosition,
                         3,
                         "_down"
                     );

                     pairedLayerOffset = 1;
                 }
                 else if (downPosition !=
                     std::string::npos)
                 {
                     pairedMiscId.replace(
                         downPosition,
                         5,
                         "_up"
                     );

                     pairedLayerOffset = -1;
                 }

                 map
                     .cell(cellX, cellY)
                     .removeMisc();

                 if (pairedLayerOffset != 0)
                 {
                     const int pairedLayer =
                         originalLayer +
                         pairedLayerOffset;

                     const Cell* pairedCell =
                         map.findCell(
                             cellX,
                             cellY,
                             pairedLayer
                         );

                     if (pairedCell != nullptr &&
                         pairedCell->hasMisc() &&
                         pairedCell->miscId() ==
                         pairedMiscId)
                     {
                         map.setActiveLayer(
                             pairedLayer
                         );

                         map
                             .cell(cellX, cellY)
                             .removeMisc();

                         map.setActiveLayer(
                             originalLayer
                         );
                     }
                 }

                 changed = true;
             }

             const auto eraseEdge =
                 [&](EdgeDirection direction,
                     bool touchesEraser)
                 {
                     const Cell* currentCell =
                         map.findCell(
                             cellX,
                             cellY,
                             map.activeLayer()
                         );

                     if (touchesEraser &&
                         currentCell != nullptr &&
                         currentCell->hasEdge(direction))
                     {
                         WorldView::removeEdge(map,
                             cellX,
                             cellY,
                             direction
                         );

                         changed = true;
                     }
                 };

             const float cellLeft =
                 static_cast<float>(cellX);

             const float cellTop =
                 static_cast<float>(cellY);

             const float cellRight =
                 cellLeft + 1.0f;

             const float cellBottom =
                 cellTop + 1.0f;

             const bool horizontalOverlap =
                 cellRight >= rectangleLeft &&
                 cellLeft <= rectangleRight;

             const bool verticalOverlap =
                 cellBottom >= rectangleTop &&
                 cellTop <= rectangleBottom;

             eraseEdge(
                 EdgeDirection::North,
                 horizontalOverlap &&
                 cellTop >= rectangleTop &&
                 cellTop <= rectangleBottom
             );

             eraseEdge(
                 EdgeDirection::East,
                 verticalOverlap &&
                 cellRight >= rectangleLeft &&
                 cellRight <= rectangleRight
             );

             eraseEdge(
                 EdgeDirection::South,
                 horizontalOverlap &&
                 cellBottom >= rectangleTop &&
                 cellBottom <= rectangleBottom
             );

             eraseEdge(
                 EdgeDirection::West,
                 verticalOverlap &&
                 cellLeft >= rectangleLeft &&
                 cellLeft <= rectangleRight
             );
         }
     }

     if (changed)
     {
         dirty = true;
     }
 }

void handleEraserAutoPan(Viewport& viewport, Hover& hover, Eraser& eraser,
     const ImVec2& canvasPosition,
     const ImVec2& canvasSize
 )
{
     const int radius =
         eraser.m_eraserSize / 2;

     const float halfCellSize =
         viewport.m_cellSize * 0.5f;

     const float offsetX =
         hover.m_hoverLocalX >= halfCellSize
         ? halfCellSize
         : 0.0f;

     const float offsetY =
         hover.m_hoverLocalY >= halfCellSize
         ? halfCellSize
         : 0.0f;

     const ImVec2 rectangleStart(
         hover.m_hoverCellLeft -
         radius * viewport.m_cellSize +
         offsetX,

         hover.m_hoverCellTop -
         radius * viewport.m_cellSize +
         offsetY
     );

     const float rectangleLeft =
         rectangleStart.x;

     const float rectangleTop =
         rectangleStart.y;

     const float rectangleRight =
         rectangleLeft +
         eraser.m_eraserSize * viewport.m_cellSize;

     const float rectangleBottom =
         rectangleTop +
         eraser.m_eraserSize * viewport.m_cellSize;

     const float canvasRight =
         canvasPosition.x +
         canvasSize.x;

     const float canvasBottom =
         canvasPosition.y +
         canvasSize.y;

     const float panSpeed = 5.0f;

     if (rectangleLeft <= canvasPosition.x)
     {
         viewport.m_cameraX -= panSpeed;
     }
     else if (rectangleRight >= canvasRight)
     {
         viewport.m_cameraX += panSpeed;
     }

     if (rectangleTop <= canvasPosition.y)
     {
         viewport.m_cameraY -= panSpeed;
     }
     else if (rectangleBottom >= canvasBottom)
     {
         viewport.m_cameraY += panSpeed;
     }
 }

void handleEraser(Viewport& viewport, Hover& hover, Eraser& eraser, ChunkManager& map, bool& dirty,
    const ImVec2& canvasPosition,
    const ImVec2& canvasSize
)
{
    ImVec2 mouseCanvasPosition;

    const bool mouseInsideCanvas =
        WorldView::isMouseInsideCanvas(
            canvasPosition,
            canvasSize,
            mouseCanvasPosition
        );

    if (!mouseInsideCanvas)
    {
        return;
    }

    const float mouseWheel =
        ImGui::GetIO().MouseWheel;

    static float lastMouseWheel = 0.0f;

    const int wheelSteps =
        std::max(
            1,
            static_cast<int>(
                std::round(
                    std::abs(mouseWheel)
                )
                )
        );

    if (mouseWheel > 0.0f)
    {
        eraser.m_eraserSize =
            std::min(
                eraser.m_eraserSize + wheelSteps,
                8
            );
    }
    else if (mouseWheel < 0.0f)
    {
        eraser.m_eraserSize =
            std::max(
                eraser.m_eraserSize - wheelSteps,
                1
            );
    }
    WorldView::handleEraserAutoPan(viewport, hover, eraser,
        canvasPosition,
        canvasSize
    );

    if (ImGui::IsMouseDown(
        ImGuiMouseButton_Left
    ))
    {
        WorldView::eraseArea(viewport, hover, eraser, map, dirty);
    }
}
}
