#include "WorldViewWallPainter.h"
#include "WorldViewHitTest.h"
#include "WorldViewTypes.h"
#include "WorldViewViewport.h"

#include "Cell.h"
#include "Chunk.h"
#include <cstdio>

namespace WorldView
{
void handleHorizontalToVerticalTurn(WallPainting& painter, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty,
    int cellX,
    int cellY,
    int deltaY)
{
    painter.m_verticalPaintDirection =
        deltaY > 0 ? 1 : -1;

    WallDirection horizontalDirection =
        painter.m_paintWallDirection;

   painter.m_paintOrientation =
    WallOrientation::Vertical;

painter.m_paintColumn = cellX;
    painter.m_paintWallDirection =
        painter.m_horizontalPaintDirection > 0
        ? WallDirection::East
        : WallDirection::West;
    int firstVerticalCellY;

    switch (horizontalDirection)
    {
    case WallDirection::North:

        if (deltaY > 0)
        {
            // West -> Ost -> Süd
            firstVerticalCellY = painter.m_paintRow;
        }
        else
        {
            // West -> Ost -> Nord
            firstVerticalCellY = painter.m_paintRow - 1;
        }

        break;

    case WallDirection::South:

        if (deltaY > 0)
        {
            // Ost -> West -> Süd
            firstVerticalCellY = painter.m_paintRow + 1;
        }
        else
        {
            // Ost -> West -> Nord
            firstVerticalCellY = painter.m_paintRow;
        }

        break;
    }

    if (painter.m_paintWallDirection ==
        WallDirection::East)
    {
        WorldView::applyEdge(painter, toolSettings, map, dirty,
            painter.m_paintColumn,
            firstVerticalCellY,
            EdgeDirection::East
        );
    }
    else
    {
        WorldView::applyEdge(painter, toolSettings, map, dirty,
            painter.m_paintColumn,
            firstVerticalCellY,
            EdgeDirection::West
        );
    }

    painter.m_lastPaintCellX =
        painter.m_paintColumn;

    painter.m_lastPaintCellY =
        firstVerticalCellY;

    return;
}

void handleVerticalToHorizontalTurn(WallPainting& painter, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty,
    int cellX,
    int cellY,
    int deltaX)
{
    painter.m_horizontalPaintDirection =
        deltaX > 0 ? 1 : -1;

    WallDirection verticalWall =
        painter.m_paintWallDirection;

    painter.m_paintOrientation =
        WallOrientation::Horizontal;

    painter.m_paintRow = cellY;

    // Die bisherige vertikale Zugrichtung entscheidet,
    // ob eine Nord- oder Südwand entsteht.
    painter.m_paintWallDirection =
        painter.m_verticalPaintDirection > 0
        ? WallDirection::South
        : WallDirection::North;

    int firstHorizontalCellX = painter.m_paintColumn;

    if (verticalWall == WallDirection::East)
    {
        if (deltaX > 0)
        {
            // An einer Ostwand nach Osten abbiegen:
            // erste Zelle liegt rechts der bisherigen Zelle.
            firstHorizontalCellX =
                painter.m_paintColumn + 1;
        }
        else
        {
            // An einer Ostwand nach Westen abbiegen.
            firstHorizontalCellX =
                painter.m_paintColumn;
        }
    }
    else
    {
        if (deltaX > 0)
        {
            // An einer Westwand nach Osten abbiegen.
            firstHorizontalCellX =
                painter.m_paintColumn;
        }
        else
        {
            // An einer Westwand nach Westen abbiegen:
            // erste Zelle liegt links der bisherigen Zelle.
            firstHorizontalCellX =
                painter.m_paintColumn - 1;
        }
    }

    if (painter.m_paintWallDirection ==
        WallDirection::South)
    {
        WorldView::applyEdge(painter, toolSettings, map, dirty,
            firstHorizontalCellX,
            painter.m_paintRow,
            EdgeDirection::South
        );
    }
    else
    {
        WorldView::applyEdge(painter, toolSettings, map, dirty,
            firstHorizontalCellX,
            painter.m_paintRow,
            EdgeDirection::North
        );
    }

    painter.m_lastPaintCellX =
        firstHorizontalCellX;

    painter.m_lastPaintCellY =
        painter.m_paintRow;

    return;
}

void stopPainting(WallPainting& painter)
{
    painter.m_isPainting = false;
    painter.m_isRemovingEdges = false;
    painter.m_isBacktracking = false;
    painter.m_paintOrientation.reset();
    painter.m_paintedEdges.clear();
}

void startPainting(WallPainting& painter, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty,
    int cellX,
    int cellY,
    HoveredWall hoveredWall)
{

    painter.m_paintedEdges.clear();
    painter.m_isBacktracking = false;
    switch (hoveredWall)
    {
    case HoveredWall::North:
        painter.m_paintOrientation = WallOrientation::Horizontal;
        painter.m_paintRow = cellY;
        painter.m_paintWallDirection = WallDirection::North;
        break;

    case HoveredWall::South:
        painter.m_paintOrientation = WallOrientation::Horizontal;
        painter.m_paintRow = cellY;
        painter.m_paintWallDirection = WallDirection::South;
        break;

    case HoveredWall::East:
        painter.m_paintOrientation = WallOrientation::Vertical;
        painter.m_paintColumn = cellX;
        painter.m_paintWallDirection = WallDirection::East;
        break;

    case HoveredWall::West:
        painter.m_paintOrientation = WallOrientation::Vertical;
        painter.m_paintColumn = cellX;
        painter.m_paintWallDirection = WallDirection::West;
        break;

    case HoveredWall::None:
        return;
    }

    painter.m_isPainting = true;

    EdgeDirection wall;

    switch (painter.m_paintWallDirection)
    {
    case WallDirection::North:
        wall = EdgeDirection::North;
        break;

    case WallDirection::East:
        wall = EdgeDirection::East;
        break;

    case WallDirection::South:
        wall = EdgeDirection::South;
        break;

    case WallDirection::West:
        wall = EdgeDirection::West;
        break;
    }
    if (*painter.m_paintOrientation ==
        WallOrientation::Horizontal)
    {
        painter.m_lastPaintCellX = cellX;
        painter.m_lastPaintCellY = painter.m_paintRow;
    }
    else
    {
        painter.m_lastPaintCellX = painter.m_paintColumn;
        painter.m_lastPaintCellY = cellY;
    }

    Cell& cell =
        map.cell(
            cellX,
            cellY
        );

    painter.m_isRemovingEdges =
        cell.hasEdge(wall) &&
        cell.edgeId(wall) ==
        toolSettings.m_activeEdgeId &&
        cell.edgeColorId(wall) ==
        toolSettings.m_activeColorId;

    WorldView::applyEdge(painter, toolSettings, map, dirty,
        cellX,
        cellY,
        wall
    );
}

void updatePainting(Viewport& viewport, Hover& hover, WallPainting& painter, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty,
    int cellX,
    int cellY,
    float localX,
    float localY)
{
    if (painter.m_isRemovingEdges)
    {
        switch (hover.m_hoveredWall)
        {
        case HoveredWall::North:
            WorldView::removeEdge(map,
                cellX,
                cellY,
                EdgeDirection::North
            );
            break;

        case HoveredWall::East:
            WorldView::removeEdge(map,
                cellX,
                cellY,
                EdgeDirection::East
            );
            break;

        case HoveredWall::South:
            WorldView::removeEdge(map,
                cellX,
                cellY,
                EdgeDirection::South
            );
            break;

        case HoveredWall::West:
            WorldView::removeEdge(map,
                cellX,
                cellY,
                EdgeDirection::West
            );
            break;

        case HoveredWall::None:
            break;
        }

        return;
    }
    int deltaX =
        cellX - painter.m_lastPaintCellX;

    int deltaY =
        cellY - painter.m_lastPaintCellY;

    const ImVec2 mouseMovement =
        ImGui::GetIO().MouseDelta;

    const int previousHorizontalDirection =
        painter.m_horizontalPaintDirection;

    const int previousVerticalDirection =
        painter.m_verticalPaintDirection;

    if (painter.m_paintOrientation ==
        WallOrientation::Horizontal &&
        deltaX != 0)
    {
        painter.m_horizontalPaintDirection =
            deltaX > 0 ? 1 : -1;
    }

    if (painter.m_paintOrientation ==
        WallOrientation::Vertical &&
        deltaY != 0)
    {
        painter.m_verticalPaintDirection =
            deltaY > 0 ? 1 : -1;
    }
    const bool reversedVerticalDirection =
        painter.m_paintOrientation ==
        WallOrientation::Vertical &&
        deltaY != 0 &&
        previousVerticalDirection != 0 &&
        painter.m_verticalPaintDirection !=
        previousVerticalDirection;

    const bool reversedHorizontalDirection =
        painter.m_paintOrientation ==
        WallOrientation::Horizontal &&
        deltaX != 0 &&
        previousHorizontalDirection != 0 &&
        painter.m_horizontalPaintDirection !=
        previousHorizontalDirection;

    float turnThreshold =
        viewport.m_cellSize * 0.25f;

    const bool movingClearlyNorth =
        deltaY < 0 &&
        mouseMovement.y < 0.0f &&
        std::abs(mouseMovement.y) >=
        std::abs(mouseMovement.x);

    bool movedFarEnoughSouth =
        deltaY > 0 &&
        localY >= turnThreshold;

    bool movedFarEnoughNorth =
        deltaY < 0 &&
        (
            movingClearlyNorth ||
            localY <=
            viewport.m_cellSize - turnThreshold
            );
    bool turnHorizontalToVertical =
        painter.m_paintOrientation ==
        WallOrientation::Horizontal &&
        (
            movedFarEnoughSouth ||
            movedFarEnoughNorth
            );

    bool movedFarEnoughEast =
        deltaX > 0 &&
        localX >= turnThreshold;

    bool movedFarEnoughWest =
        deltaX < 0 &&
        localX <=
        viewport.m_cellSize - turnThreshold;

    bool turnVerticalToHorizontal =
        painter.m_paintOrientation ==
        WallOrientation::Vertical &&
        (
            movedFarEnoughEast ||
            movedFarEnoughWest
            );

    if (deltaX == 0 &&
        deltaY == 0)
    {
        return;
    }

    if (deltaX == 0 &&
        deltaY == 0)
    {
        return;
    }

    if (turnHorizontalToVertical)
    {
        const int turnDirectionY =
            movedFarEnoughSouth ? 1 : -1;

        WorldView::handleHorizontalToVerticalTurn(painter, toolSettings, map, dirty,
            cellX,
            cellY,
            turnDirectionY
        );
    }

    if (turnVerticalToHorizontal)
    {
        WorldView::handleVerticalToHorizontalTurn(painter, toolSettings, map, dirty,
            cellX,
            cellY,
            deltaX
        );
    }

    if (reversedVerticalDirection &&
        painter.m_paintOrientation ==
        WallOrientation::Vertical)
    {
        const EdgeDirection wall=
            painter.m_paintWallDirection ==
            WallDirection::East
            ? EdgeDirection::East
            : EdgeDirection::West;

        WorldView::applyEdge(painter, toolSettings, map, dirty,
            painter.m_paintColumn,
            painter.m_lastPaintCellY,
            wall
        );
    }
    if (reversedHorizontalDirection &&
        painter.m_paintOrientation ==
        WallOrientation::Horizontal)
    {
        const EdgeDirection wall=
            painter.m_paintWallDirection ==
            WallDirection::North
            ? EdgeDirection::North
            : EdgeDirection::South;

        WorldView::applyEdge(painter, toolSettings, map, dirty,
            painter.m_lastPaintCellX,
            painter.m_paintRow,
            wall
        );
    }
    switch (painter.m_paintWallDirection)
    {
    case WallDirection::North:
    case WallDirection::South:
    {
        const int step =
            cellX >= painter.m_lastPaintCellX ? 1 : -1;

        const EdgeDirection wall=
            painter.m_paintWallDirection == WallDirection::North
            ? EdgeDirection::North
            : EdgeDirection::South;

        for (int x = painter.m_lastPaintCellX + step;
            x != cellX + step;
            x += step)
        {
            WorldView::applyEdge(painter, toolSettings, map, dirty,
                x,
                painter.m_paintRow,
                wall
            );
        }

        break;
    }

    case WallDirection::East:
    case WallDirection::West:
    {
        const int step =
            cellY >= painter.m_lastPaintCellY ? 1 : -1;

        const EdgeDirection wall=
            painter.m_paintWallDirection == WallDirection::East
            ? EdgeDirection::East
            : EdgeDirection::West;

        for (int y = painter.m_lastPaintCellY + step;
            y != cellY + step;
            y += step)
        {
            WorldView::applyEdge(painter, toolSettings, map, dirty,
                painter.m_paintColumn,
                y,
                wall
            );
        }

        break;
    }
    }
    painter.m_lastPaintCellX = cellX;
    painter.m_lastPaintCellY = cellY;

    if (*painter.m_paintOrientation ==
        WallOrientation::Horizontal)
    {
        painter.m_lastPaintCellX = cellX;
        painter.m_lastPaintCellY = painter.m_paintRow;
    }
    else
    {
        painter.m_lastPaintCellX = painter.m_paintColumn;
        painter.m_lastPaintCellY = cellY;
    }
}

void applyEdge(WallPainting& painter, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty,
     int cellX,
     int cellY,
     EdgeDirection direction
 )
{
     if (toolSettings.m_activeTool == EditorTool::Pencil &&
         painter.m_isPainting)
     {
         PaintedEdge currentEdge =
             WorldView::normalizeEdge(
                 cellX,
                 cellY,
                 direction
             );

         /*
          * Wenn dieselbe Kante erneut erreicht wird,
          * wird die letzte Änderung rückgängig gemacht.
          */
         for (auto iterator =
             painter.m_paintedEdges.begin();
             iterator !=
             painter.m_paintedEdges.end();
             ++iterator)
         {
             if (!WorldView::isSameEdge(
                 currentEdge,
                 *iterator
             ))
             {
                 continue;
             }

             if (iterator->changed)
             {
                 if (iterator->
                     previousEdgeId.empty())
                 {
                     WorldView::removeEdge(map,
                         iterator->cellX,
                         iterator->cellY,
                         iterator->direction
                     );
                 }
                 else
                 {
                     WorldView::setEdge(map,
                         iterator->cellX,
                         iterator->cellY,
                         iterator->direction,
                         iterator->previousEdgeId,
                         iterator->previousColorId
                     );
                 }
             }

             painter.m_paintedEdges.erase(
                 iterator
             );

             painter.m_isBacktracking = true;
             return;
         }

         painter.m_isBacktracking = false;

         Cell& cell =
             map.cell(
                 currentEdge.cellX,
                 currentEdge.cellY
             );

         currentEdge.previousEdgeId =
             cell.edgeId(
                 currentEdge.direction
             );

         currentEdge.previousColorId =
             cell.edgeColorId(
                 currentEdge.direction
             );

         currentEdge.changed = false;

         if (painter.m_isRemovingEdges)
         {
             if (cell.hasEdge(
                 currentEdge.direction
             ))
             {
                 WorldView::removeEdge(map,
                     currentEdge.cellX,
                     currentEdge.cellY,
                     currentEdge.direction
                 );

                 currentEdge.changed = true;
             }
         }
         else if (
             currentEdge.previousEdgeId !=
             toolSettings.m_activeEdgeId ||
             currentEdge.previousColorId !=
             toolSettings.m_activeColorId
             )
         {
             WorldView::setEdge(map,
                 currentEdge.cellX,
                 currentEdge.cellY,
                 currentEdge.direction,
                 toolSettings.m_activeEdgeId,
                 toolSettings.m_activeColorId
             );

             currentEdge.changed = true;
         }

         if (currentEdge.changed)
         {
             dirty = true;
         }

         painter.m_paintedEdges.push_back(
             currentEdge
         );

         return;
     }

 }

void setEdge(ChunkManager& map,
    int cellX,
    int cellY,
    EdgeDirection direction,
    const std::string& edgeId,
    const std::string& colorId
)
{
    Cell& cell =
        map.cell(
            cellX,
            cellY
        );

    cell.setEdge(
        direction,
        edgeId,
        colorId
    );

    switch (direction)
    {
    case EdgeDirection::North:
        map
            .cell(cellX, cellY - 1)
            .setEdge(
                EdgeDirection::South,
                edgeId,
                colorId
            );
        break;

    case EdgeDirection::East:
        map
            .cell(cellX + 1, cellY)
            .setEdge(
                EdgeDirection::West,
                edgeId,
                colorId
            );
        break;

    case EdgeDirection::South:
        map
            .cell(cellX, cellY + 1)
            .setEdge(
                EdgeDirection::North,
                edgeId,
                colorId
            );
        break;

    case EdgeDirection::West:
        map
            .cell(cellX - 1, cellY)
            .setEdge(
                EdgeDirection::East,
                edgeId,
                colorId
            );
        break;
    }
}

void removeEdge(ChunkManager& map,
    int cellX,
    int cellY,
    EdgeDirection direction
)
{
    Cell& cell =
        map.cell(
            cellX,
            cellY
        );

    cell.removeEdge(
        direction
    );

    switch (direction)
    {
    case EdgeDirection::North:
        map
            .cell(cellX, cellY - 1)
            .removeEdge(
                EdgeDirection::South
            );
        break;

    case EdgeDirection::East:
        map
            .cell(cellX + 1, cellY)
            .removeEdge(
                EdgeDirection::West
            );
        break;

    case EdgeDirection::South:
        map
            .cell(cellX, cellY + 1)
            .removeEdge(
                EdgeDirection::North
            );
        break;

    case EdgeDirection::West:
        map
            .cell(cellX - 1, cellY)
            .removeEdge(
                EdgeDirection::East
            );
        break;
    }
}
}
