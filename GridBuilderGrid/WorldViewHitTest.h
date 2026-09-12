#pragma once

#include "WorldViewTypes.h"

namespace WorldView
{
struct Hover
{
    int m_hoveredCellX = 0;
    int m_hoveredCellY = 0;
    bool m_hasHoveredCell = false;
    int m_selectedCellX = 0;
    int m_selectedCellY = 0;
    bool m_hasSelectedCell = false;
    float m_wallSelectionWidth = 6.0f;
    HoveredWall m_hoveredWall = HoveredWall::None;
    float m_hoverLocalX = 0.0f;
    float m_hoverLocalY = 0.0f;
    float m_hoverCellLeft = 0.0f;
    float m_hoverCellTop = 0.0f;
};

HoveredWall getHoveredWall(const Viewport& viewport,
    float localX,
    float localY,
    float selectionWidth);

void updateHover(Viewport& viewport, Hover& hover, WallPainting& painter,
    const ImVec2& canvasPosition,
    const ImVec2& canvasSize
);

PaintedEdge normalizeEdge(
    int cellX,
    int cellY,
    EdgeDirection direction
);

bool isSameEdge(
    const PaintedEdge& first,
    const PaintedEdge& second
);

bool isHoveredEdge(const Hover& hover, const WallPainting& painter, const ToolSettings& toolSettings,
     int cellX,
     int cellY,
     EdgeDirection direction
 );
}
