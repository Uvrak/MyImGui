#pragma once

#include "WorldViewTypes.h"

namespace WorldView
{
struct Eraser
{
    int m_eraserSize = 3;
};

void eraseArea(Viewport& viewport, Hover& hover, Eraser& eraser, ChunkManager& map, bool& dirty);

void handleEraserAutoPan(Viewport& viewport, Hover& hover, Eraser& eraser,
     const ImVec2& canvasPosition,
     const ImVec2& canvasSize
 );

void handleEraser(Viewport& viewport, Hover& hover, Eraser& eraser, ChunkManager& map, bool& dirty,
    const ImVec2& canvasPosition,
    const ImVec2& canvasSize
);
}
