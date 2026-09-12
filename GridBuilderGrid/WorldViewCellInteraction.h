#pragma once

#include "WorldViewTypes.h"

namespace WorldView
{
struct CellInteraction
{
    bool m_requestNotePopup = false;
    int m_noteCellX = 0;
    int m_noteCellY = 0;
    int m_noteLayer = 0;
    char m_noteTextBuffer[1024] = {};
};

void handlePencil(Viewport& viewport, Hover& hover, WallPainting& painter, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty,
    const ImVec2& canvasPosition,
    const ImVec2& canvasSize
);

void drawNotePopup(CellInteraction& cellInteraction, ChunkManager& map, bool& dirty, int& blockFrames);

void drawNoteTooltip(const Hover& hover, const ChunkManager& map);

void handleMisc(Hover& hover, CellInteraction& cellInteraction, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty,
    const ImVec2& canvasPosition,
    const ImVec2& canvasSize
);

void openHoveredEdgeColorMenu(Hover& hover, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty);

void openHoveredMiscColorMenu(Hover& hover, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty);

void openHoveredColorMenu(Hover& hover, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty);
}
