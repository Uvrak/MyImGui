#pragma once

#include "WorldViewTypes.h"

namespace WorldView
{
struct WallPainting
{
    bool m_isPainting = false;
    bool m_isRemovingEdges = false;
    std::optional<WallOrientation> m_paintOrientation;
    int m_horizontalPaintDirection = 0;
    int m_verticalPaintDirection = 0;
    bool m_isBacktracking = false;
    int m_paintRow = 0;
    int m_paintColumn = 0;
    WallDirection m_paintWallDirection = WallDirection::North;
    int m_lastPaintCellX = 0;
    int m_lastPaintCellY = 0;
    std::vector<PaintedEdge> m_paintedEdges;
};

void handleHorizontalToVerticalTurn(WallPainting& painter, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty,
    int cellX,
    int cellY,
    int deltaY);

void handleVerticalToHorizontalTurn(WallPainting& painter, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty,
    int cellX,
    int cellY,
    int deltaX);

void stopPainting(WallPainting& painter);

void startPainting(WallPainting& painter, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty,
    int cellX,
    int cellY,
    HoveredWall hoveredWall);

void updatePainting(Viewport& viewport, Hover& hover, WallPainting& painter, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty,
    int cellX,
    int cellY,
    float localX,
    float localY);

void applyEdge(WallPainting& painter, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty,
     int cellX,
     int cellY,
     EdgeDirection direction
 );

void setEdge(ChunkManager& map,
    int cellX,
    int cellY,
    EdgeDirection direction,
    const std::string& edgeId,
    const std::string& colorId
);

void removeEdge(ChunkManager& map,
    int cellX,
    int cellY,
    EdgeDirection direction
);
}
