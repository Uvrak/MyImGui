#include "WorldViewHitTest.h"
#include "WorldViewTypes.h"
#include "WorldViewViewport.h"
#include "WorldViewWallPainter.h"

#include "Cell.h"
#include "Chunk.h"
#include <cstdio>

namespace WorldView
{
HoveredWall getHoveredWall(const Viewport& viewport,
    float localX,
    float localY,
    float selectionWidth)
{
    float north = localY;
    float east = viewport.m_cellSize - localX;
    float south = viewport.m_cellSize - localY;
    float west = localX;

    float best = selectionWidth;
    HoveredWall result = HoveredWall::None;

    if (north < best)
    {
        best = north;
        result = HoveredWall::North;
    }

    if (east < best)
    {
        best = east;
        result = HoveredWall::East;
    }

    if (south < best)
    {
        best = south;
        result = HoveredWall::South;
    }

    if (west < best)
    {
        best = west;
        result = HoveredWall::West;
    }

    return result;
}

void updateHover(Viewport& viewport, Hover& hover, WallPainting& painter,
    const ImVec2& canvasPosition,
    const ImVec2& canvasSize
)
{
    ImVec2 mouseCanvasPosition;

    hover.m_hasHoveredCell = WorldView::isMouseInsideCanvas(
        canvasPosition,
        canvasSize,
        mouseCanvasPosition
    );

    if (!hover.m_hasHoveredCell)
    {
        return;
    }

    hover.m_hoveredCellX =
        viewport.m_gridView.firstVisibleCellX +
        static_cast<int>(
            (mouseCanvasPosition.x + viewport.m_gridView.startX) /
            viewport.m_cellSize
            );

    hover.m_hoveredCellY =
        viewport.m_gridView.firstVisibleCellY +
        static_cast<int>(
            (mouseCanvasPosition.y + viewport.m_gridView.startY) /
            viewport.m_cellSize
            );

    hover.m_hoverCellLeft =
        canvasPosition.x +
        (hover.m_hoveredCellX -
            viewport.m_gridView.firstVisibleCellX) *
        viewport.m_cellSize -
        viewport.m_gridView.startX;

    hover.m_hoverCellTop =
        canvasPosition.y +
        (hover.m_hoveredCellY -
            viewport.m_gridView.firstVisibleCellY) *
        viewport.m_cellSize -
        viewport.m_gridView.startY;

    float cellCanvasLeft =
        hover.m_hoverCellLeft - canvasPosition.x;

    float cellCanvasTop =
        hover.m_hoverCellTop - canvasPosition.y;

    hover.m_hoverLocalX =
        mouseCanvasPosition.x - cellCanvasLeft;

    hover.m_hoverLocalY =
        mouseCanvasPosition.y - cellCanvasTop;

    const float selectionWidth =
        hover.m_wallSelectionWidth *
        (
            painter.m_isPainting
            ? 2.5f
            : 1.25f
            );
    hover.m_hoveredWall =
        WorldView::getHoveredWall(viewport,
            hover.m_hoverLocalX,
            hover.m_hoverLocalY,
            selectionWidth
        );
}

PaintedEdge normalizeEdge(
    int cellX,
    int cellY,
    EdgeDirection direction
)
{
    switch (direction)
    {
    case EdgeDirection::North:
        return {
            cellX,
            cellY,
            EdgeDirection::North,
            false
        };

    case EdgeDirection::East:
        return {
            cellX,
            cellY,
            EdgeDirection::East,
            false
        };

    case EdgeDirection::South:
        return {
            cellX,
            cellY + 1,
            EdgeDirection::North,
            false
        };

    case EdgeDirection::West:
        return {
            cellX - 1,
            cellY,
            EdgeDirection::East,
            false
        };
    }

    return {
        cellX,
        cellY,
        direction,
        false
    };
}

bool isSameEdge(
    const PaintedEdge& first,
    const PaintedEdge& second
)
{
    return
        first.cellX == second.cellX &&
        first.cellY == second.cellY &&
        first.direction == second.direction;
}

bool isHoveredEdge(const Hover& hover, const WallPainting& painter, const ToolSettings& toolSettings,
     int cellX,
     int cellY,
     EdgeDirection direction
 )
{
     if (!hover.m_hasHoveredCell ||
         toolSettings.m_activeTool != EditorTool::Pencil ||
         painter.m_isPainting ||
         toolSettings.m_paintMisc)
     {
         return false;
     }

     EdgeDirection hoveredWall;

     switch (hover.m_hoveredWall)
     {
     case HoveredWall::North:
         hoveredWall = EdgeDirection::North;
         break;

     case HoveredWall::East:
         hoveredWall = EdgeDirection::East;
         break;

     case HoveredWall::South:
         hoveredWall = EdgeDirection::South;
         break;

     case HoveredWall::West:
         hoveredWall = EdgeDirection::West;
         break;

     case HoveredWall::None:
         return false;
     }

     const PaintedEdge first =
         WorldView::normalizeEdge(
             cellX,
             cellY,
             direction
         );

     const PaintedEdge second =
         WorldView::normalizeEdge(
             hover.m_hoveredCellX,
             hover.m_hoveredCellY,
             hoveredWall
         );

     return WorldView::isSameEdge(first, second);
 }
}
