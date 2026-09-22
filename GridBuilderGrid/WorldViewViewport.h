#pragma once

#include "WorldViewTypes.h"

namespace WorldView
{
struct Viewport
{
    float m_cellSize = 34.0f;
    float m_rulerHeight = 25.0f;
    float m_rulerWidth = 40.0f;
    float m_cameraX = 0.0f;
    float m_cameraY = 0.0f;
    int m_labelStep = 4;
    int m_longTickStep = 4;
    GridView m_gridView;
    bool m_followPlayer = false;
    int m_fittedChunkSize = 0, m_fittedChunkX = 0, m_fittedChunkY = 0;
    int m_fittedRowOffset = 0;
};

int edgeTextureSize(const Viewport& viewport);

void updateGridView(Viewport& viewport);

// Fit a complete square chunk into the drawable canvas, preserving square cells.
void fitChunk(Viewport& viewport, int cellX, int cellY, int chunkSize,
    ImVec2 canvasSize, int rowOffset = 0);
void updateChunkFit(Viewport& viewport, ImVec2 canvasSize);

bool isMouseInsideCanvas(
    ImVec2 canvasPosition,
    ImVec2 canvasSize,
    ImVec2& mouseCanvasPosition
);

void handleZoom(Viewport& viewport, const ToolSettings& toolSettings,
    ImVec2 canvasPosition,
    ImVec2 canvasSize);

int calculateLabelStep(const Viewport& viewport);

void updateLongTickStep(Viewport& viewport);

void centerOnPlayer(Viewport& viewport, const MapPlayerMarker& player,
     ImVec2 canvasSize
 );

void handlePan(Viewport& viewport);
}
