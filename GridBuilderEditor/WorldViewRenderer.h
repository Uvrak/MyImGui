#pragma once

#include "WorldViewTypes.h"

namespace WorldView
{
struct RenderStyle
{
    ImU32 m_previewColor = IM_COL32(255, 255, 230, 40);
    ImU32 m_backgroundColor = IM_COL32(36, 78, 150, 255);
    ImU32 m_gridColor = IM_COL32(40, 68, 110, 255);
    ImU32 m_chunkGridColor = IM_COL32(38, 66, 108, 255);
    ImU32 m_hoverColor = IM_COL32(255, 255, 255, 50);
    ImU32 m_selectionColor = IM_COL32(255, 220, 0, 255);
    ImU32 m_rulerColor = IM_COL32(0, 0, 0, 255);
    ImU32 m_rulerTextColor = IM_COL32(230, 230, 230, 255);
};

void drawWalls(const Viewport& viewport, const Hover& hover, const WallPainting& painter, const ToolSettings& toolSettings, const ChunkManager& map, bool showLowerLayer,
    ImDrawList* drawList,
    ImVec2 canvasPosition,
    ImVec2 canvasSize
);

void drawMisc(const Viewport& viewport, const ToolSettings& toolSettings, const ChunkManager& map,
    ImDrawList* drawList,
    ImVec2 canvasPosition,
    ImVec2 canvasSize
);

void drawLayerWalls(const Viewport& viewport, const Hover& hover, const WallPainting& painter, const ToolSettings& toolSettings, const ChunkManager& map,
    ImDrawList* drawList,
    ImVec2 canvasPosition,
    ImVec2 canvasSize,
    int layer,
    ImU32 color,
    bool activeLayer
);

void drawRulers(const Viewport& viewport, const RenderStyle& style, ImDrawList* drawList, ImVec2 canvasPosition, ImVec2 canvasSize);

void drawGrid(const Viewport& viewport, const RenderStyle& style, int chunkSize,
    ImDrawList* drawList,
    ImVec2 canvasPosition,
    ImVec2 canvasSize);

void drawWallPreview(const Viewport& viewport, const ToolSettings& toolSettings,
    ImDrawList* drawList,
    float cellLeft,
    float cellTop,
    HoveredWall hoveredWall,
    ImU32 color
);

void drawMiscPreview(const Viewport& viewport, const Hover& hover, const ToolSettings& toolSettings, const ChunkManager& map,
    ImDrawList* drawList
);

void drawHover(const Viewport& viewport, const Hover& hover, const WallPainting& painter, const ToolSettings& toolSettings,
    ImDrawList* drawList,
    ImVec2 canvasPosition,
    ImVec2 canvasSize
);

void drawEdgeIcon(const Viewport& viewport,
     ImDrawList* drawList,
     SDL_Texture* texture,
     ImVec2 center,
     bool horizontal,
     ImU32 color
 );

void drawEraserPreview(const Viewport& viewport, const Hover& hover, const Eraser& eraser, const ToolSettings& toolSettings,
     ImDrawList* drawList
 );

void drawPlayerMarker(const Viewport& viewport, const MapPlayerMarker& player,
     ImDrawList* drawList,
     ImVec2 canvasPosition
 );

void drawSelection(const Viewport& viewport, const Hover& hover,
    ImDrawList* drawList,
    ImVec2 canvasPosition);

void drawCoordinates(const Hover& hover, int chunkSize, ImDrawList* drawList, ImVec2 canvasPosition);
}
