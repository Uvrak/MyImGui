#include "WorldViewWallPainter.h"
#include "WorldViewHitTest.h"
#include "WorldViewTypes.h"
#include "WorldViewViewport.h"

#include "Cell.h"
#include "Chunk.h"
#include <cstdio>

namespace WorldView
{
void stopPainting(WallPainting& painter)
{
    painter.gesture.reset();painter.line.reset();
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

    painter.line.begin({cellX,cellY,int(wall),1,true},float(cellX)+.5f,float(cellY)+.5f);
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
    painter.line.sample(cellX+localX/viewport.m_cellSize,cellY+localY/viewport.m_cellSize,[&](GridPaintTarget edge){
        WorldView::applyEdge(painter,toolSettings,map,dirty,edge.x,edge.y,static_cast<EdgeDirection>(edge.side));
    });
    painter.m_paintOrientation=painter.line.axis?WallOrientation::Vertical:WallOrientation::Horizontal;
    painter.m_paintWallDirection=painter.line.axis?WallDirection::West:WallDirection::North;
    painter.m_paintRow=painter.line.axis?painter.line.along:painter.line.fixed;
    painter.m_paintColumn=painter.line.axis?painter.line.fixed:painter.line.along;
    painter.m_lastPaintCellX=painter.m_paintColumn;painter.m_lastPaintCellY=painter.m_paintRow;
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
