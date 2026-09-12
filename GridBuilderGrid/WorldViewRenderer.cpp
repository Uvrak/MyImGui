#include "WorldViewRenderer.h"
#include "WorldViewEraser.h"
#include "WorldViewHitTest.h"
#include "WorldViewTypes.h"
#include "WorldViewViewport.h"
#include "WorldViewWallPainter.h"

#include "Cell.h"
#include "Chunk.h"
#include <cstdio>

namespace WorldView
{
void drawWalls(const Viewport& viewport, const Hover& hover, const WallPainting& painter, const ToolSettings& toolSettings, const ChunkManager& map, bool showLowerLayer,
    ImDrawList* drawList,
    ImVec2 canvasPosition,
    ImVec2 canvasSize
)
{
    const int currentLayer =
        map.activeLayer();

    const int lowerLayer =
        currentLayer - 1;

    if (showLowerLayer &&
        map.hasLayer(
            lowerLayer
        ))
    {
        WorldView::drawLayerWalls(viewport, hover, painter, toolSettings, map,
            drawList,
            canvasPosition,
            canvasSize,
            lowerLayer,
            IM_COL32(255, 255, 255, 70),
            false
        );
    }

    WorldView::drawLayerWalls(viewport, hover, painter, toolSettings, map,
        drawList,
        canvasPosition,
        canvasSize,
        currentLayer,
        IM_COL32_WHITE,
        true
    );
}

void drawMisc(const Viewport& viewport, const ToolSettings& toolSettings, const ChunkManager& map,
    ImDrawList* drawList,
    ImVec2 canvasPosition,
    ImVec2 canvasSize
)
{
    const int visibleCellCountX =
        static_cast<int>(
            canvasSize.x / viewport.m_cellSize
            ) + 2;

    const int visibleCellCountY =
        static_cast<int>(
            canvasSize.y / viewport.m_cellSize
            ) + 2;

    const int layer =
        map.activeLayer();

    for (int offsetY = 0;
        offsetY < visibleCellCountY;
        ++offsetY)
    {
        const int cellY =
            viewport.m_gridView.firstVisibleCellY +
            offsetY;

        for (int offsetX = 0;
            offsetX < visibleCellCountX;
            ++offsetX)
        {
            const int cellX =
                viewport.m_gridView.firstVisibleCellX +
                offsetX;

            const Cell* cell =
                map.findCell(
                    cellX,
                    cellY,
                    layer
                );

            if (cell == nullptr ||
                (
                    !cell->hasMisc() &&
                    !cell->hasMiscText()
                    ))
            {
                continue;
            }

            SDL_Texture* texture =
                cell->hasMisc() &&
                toolSettings.m_miscTexture
                ? toolSettings.m_miscTexture(
                    cell->miscId(),
                    WorldView::edgeTextureSize(viewport)
                )
                : nullptr;

            const MapColor* mapColor =
                map
                .colorPalette()
                .find(
                    cell->miscColorId()
                );

            ImU32 tint =
                IM_COL32_WHITE;

            if (mapColor != nullptr)
            {
                tint =
                    IM_COL32(
                        mapColor->color.red,
                        mapColor->color.green,
                        mapColor->color.blue,
                        mapColor->color.alpha
                    );
            }

            const float cellLeft =
                canvasPosition.x +
                offsetX * viewport.m_cellSize -
                viewport.m_gridView.startX;

            const float cellTop =
                canvasPosition.y +
                offsetY * viewport.m_cellSize -
                viewport.m_gridView.startY;

            if (cell->hasMiscText())
            {
                ImU32 noteTint =
                    tint;

                const MapColor* noteColor =
                    map
                    .colorPalette()
                    .find(
                        map.miscColorId(
                            "note"
                        )
                    );

                if (noteColor != nullptr)
                {
                    noteTint =
                        IM_COL32(
                            noteColor->color.red,
                            noteColor->color.green,
                            noteColor->color.blue,
                            noteColor->color.alpha
                        );
                }

                const float noteMarkerSize =
                    viewport.m_cellSize * 0.3f;

                drawList->AddTriangleFilled(
                    ImVec2(
                        cellLeft,
                        cellTop
                    ),
                    ImVec2(
                        cellLeft + noteMarkerSize,
                        cellTop
                    ),
                    ImVec2(
                        cellLeft,
                        cellTop + noteMarkerSize
                    ),
                    noteTint
                );

            }

            if (texture != nullptr)
            {

                const float margin =
                    viewport.m_cellSize * 0.1f;

                const ImVec2 topLeft(
                    cellLeft + margin,
                    cellTop + margin
                );

                const ImVec2 topRight(
                    cellLeft + viewport.m_cellSize - margin,
                    cellTop + margin
                );

                const ImVec2 bottomRight(
                    cellLeft + viewport.m_cellSize - margin,
                    cellTop + viewport.m_cellSize - margin
                );

                const ImVec2 bottomLeft(
                    cellLeft + margin,
                    cellTop + viewport.m_cellSize - margin
                );

                drawList->AddImageQuad(
                    (ImTextureID)(intptr_t)texture,
                    topLeft,
                    topRight,
                    bottomRight,
                    bottomLeft,
                    ImVec2(0.0f, 0.0f),
                    ImVec2(1.0f, 0.0f),
                    ImVec2(1.0f, 1.0f),
                    ImVec2(0.0f, 1.0f),
                    tint
                );
            }
        }
    }
}

void drawLayerWalls(const Viewport& viewport, const Hover& hover, const WallPainting& painter, const ToolSettings& toolSettings, const ChunkManager& map,
    ImDrawList* drawList,
    ImVec2 canvasPosition,
    ImVec2 canvasSize,
    int layer,
    ImU32 color,
    bool activeLayer
)
{
    const int visibleCellCountX =
        static_cast<int>(
            canvasSize.x / viewport.m_cellSize
            ) + 2;

    const int visibleCellCountY =
        static_cast<int>(
            canvasSize.y / viewport.m_cellSize
            ) + 2;

    for (int offsetY = 0;
        offsetY < visibleCellCountY;
        ++offsetY)
    {
        const int cellY =
            viewport.m_gridView.firstVisibleCellY +
            offsetY;

        for (int offsetX = 0;
            offsetX < visibleCellCountX;
            ++offsetX)
        {
            const int cellX =
                viewport.m_gridView.firstVisibleCellX +
                offsetX;

            const Cell* cell =
                map.findCell(
                    cellX,
                    cellY,
                    layer
                );

            if (cell == nullptr)
            {
                continue;
            }

            const float cellLeft =
                canvasPosition.x +
                offsetX * viewport.m_cellSize -
                viewport.m_gridView.startX;

            const float cellTop =
                canvasPosition.y +
                offsetY * viewport.m_cellSize -
                viewport.m_gridView.startY;

            const auto drawCellEdge =
                [&](
                    EdgeDirection direction,
                    ImVec2 center,
                    bool horizontal
                    )
                {
                    if (!cell->hasEdge(
                        direction
                    ))
                    {
                        return;
                    }

                    if (activeLayer &&
                        WorldView::isHoveredEdge(hover, painter, toolSettings,
                            cellX,
                            cellY,
                            direction
                        ))
                    {
                        return;
                    }

                    const std::string& edgeId =
                        cell->edgeId(
                            direction
                        );

                    SDL_Texture* texture =
                        nullptr;

                    if (toolSettings.m_edgeTexture)
                    {
                        texture =
                            toolSettings.m_edgeTexture(
                                edgeId,
                                WorldView::edgeTextureSize(viewport)
                            );
                    }

                    const std::string& colorId =
                        cell->edgeColorId(
                            direction
                        );

                    const MapColor* mapColor =
                        map
                        .colorPalette()
                        .find(colorId);

                    const EdgeColor edgeColor =
                        mapColor != nullptr
                        ? mapColor->color
                        : EdgeColor
                    {
                        255,
                        255,
                        255,
                        255
                    };

                    const ImVec4 layerColor =
                        ImGui::ColorConvertU32ToFloat4(
                            color
                        );

                    const ImU32 edgeTint =
                        IM_COL32(
                            static_cast<int>(
                                edgeColor.red *
                                layerColor.x
                                ),
                            static_cast<int>(
                                edgeColor.green *
                                layerColor.y
                                ),
                            static_cast<int>(
                                edgeColor.blue *
                                layerColor.z
                                ),
                            static_cast<int>(
                                edgeColor.alpha *
                                layerColor.w
                                )
                        );
                    WorldView::drawEdgeIcon(viewport,
                        drawList,
                        texture,
                        center,
                        horizontal,
                        edgeTint
                    );
                };

            drawCellEdge(
                EdgeDirection::North,
                ImVec2(
                    cellLeft +
                    viewport.m_cellSize * 0.5f,
                    cellTop
                ),
                true
            );

            drawCellEdge(
                EdgeDirection::East,
                ImVec2(
                    cellLeft + viewport.m_cellSize,
                    cellTop +
                    viewport.m_cellSize * 0.5f
                ),
                false
            );

            drawCellEdge(
                EdgeDirection::South,
                ImVec2(
                    cellLeft +
                    viewport.m_cellSize * 0.5f,
                    cellTop + viewport.m_cellSize
                ),
                true
            );

            drawCellEdge(
                EdgeDirection::West,
                ImVec2(
                    cellLeft,
                    cellTop +
                    viewport.m_cellSize * 0.5f
                ),
                false
            );
        }
    }
}

void drawRulers(const Viewport& viewport, const RenderStyle& style, ImDrawList* drawList, ImVec2 canvasPosition, ImVec2 canvasSize)
{

    drawList->AddRectFilled(
        ImVec2(
            canvasPosition.x - viewport.m_rulerWidth,
            canvasPosition.y
        ),
        ImVec2(
            canvasPosition.x,
            canvasPosition.y + canvasSize.y
        ),
        style.m_rulerColor
    );

    for (float y = -viewport.m_gridView.startY;
        y < canvasSize.y;
        y += viewport.m_cellSize)
    {
        float lineTop = canvasPosition.y + y;
        float textCenterY = lineTop + viewport.m_cellSize * 0.5f;

        int cellOffset = static_cast<int>(
            std::floor((y + viewport.m_gridView.startY) / viewport.m_cellSize)
            );

        int index =
            viewport.m_gridView.firstVisibleCellY + cellOffset;

        bool majorTick =
            (index % viewport.m_labelStep == 0);

        bool longTick =
            (index % viewport.m_longTickStep == 0);

        float tickLength =
            longTick ? 15.0f : 5.0f;

        drawList->AddLine(
            ImVec2(
                canvasPosition.x - tickLength,
                lineTop
            ),
            ImVec2(
                canvasPosition.x,
                lineTop
            ),
            style.m_rulerTextColor
        );

        if (majorTick)
        {
            std::string text = std::to_string(index);
            ImVec2 textSize = ImGui::CalcTextSize(text.c_str());

            drawList->AddText(
                ImVec2(
                    canvasPosition.x - viewport.m_rulerWidth +
                    (viewport.m_rulerWidth - textSize.x) * 0.5f,
                    textCenterY - textSize.y * 0.5f
                ),
                style.m_rulerTextColor,
                text.c_str()
            );
        }
    }

    for (float x = -viewport.m_gridView.startX;
        x < canvasSize.x;
        x += viewport.m_cellSize)
    {
        float lineX = canvasPosition.x + x;

        int cellOffset = static_cast<int>(
            std::floor((x + viewport.m_gridView.startX) / viewport.m_cellSize)
            );

        int index =
            viewport.m_gridView.firstVisibleCellX + cellOffset;

        bool majorTick =
            (index % viewport.m_labelStep == 0);

        bool longTick =
            (index % viewport.m_longTickStep == 0);

        float tickLength =
            longTick ? 20.0f : 5.0f;

        drawList->AddLine(
            ImVec2(
                lineX,
                canvasPosition.y - tickLength
            ),
            ImVec2(
                lineX,
                canvasPosition.y
            ),
            style.m_rulerTextColor
        );

        if (majorTick)
        {
            std::string text = std::to_string(index);
            ImVec2 textSize = ImGui::CalcTextSize(text.c_str());

            drawList->AddText(
                ImVec2(
                    lineX + 3.0f,
                    canvasPosition.y - viewport.m_rulerHeight +
                    (viewport.m_rulerHeight - textSize.y) * 0.5f - 3.0f
                ),
                style.m_rulerTextColor,
                text.c_str()
            );
        }
    }

    drawList->AddRectFilled(
        ImVec2(
            canvasPosition.x - viewport.m_rulerWidth,
            canvasPosition.y - viewport.m_rulerHeight
        ),
        ImVec2(
            canvasPosition.x,
            canvasPosition.y
        ),
        IM_COL32(45, 45, 50, 255)
    );

    drawList->AddRect(
        ImVec2(
            canvasPosition.x - viewport.m_rulerWidth,
            canvasPosition.y - viewport.m_rulerHeight
        ),
        ImVec2(
            canvasPosition.x,
            canvasPosition.y
        ),
        IM_COL32(90, 90, 95, 255)
    );

}

void drawGrid(const Viewport& viewport, const RenderStyle& style, int chunkSize,
    ImDrawList* drawList,
    ImVec2 canvasPosition,
    ImVec2 canvasSize)
{
    int worldY = viewport.m_gridView.firstVisibleCellY;

    for (float y = -viewport.m_gridView.startY;
        y < canvasSize.y;
        y += viewport.m_cellSize, ++worldY)
    {
        float lineY = canvasPosition.y + y;

        bool isChunkBorder =
            worldY % chunkSize == 0;

        ImU32 color =
            isChunkBorder
            ? style.m_chunkGridColor
            : style.m_gridColor;

        float thickness =
            isChunkBorder
            ? 2.0f
            : 1.0f;

        drawList->AddRectFilled(
            ImVec2(
                canvasPosition.x,
                lineY
            ),
            ImVec2(
                canvasPosition.x + canvasSize.x,
                lineY + thickness
            ),
            color
        );
    }

    int worldX = viewport.m_gridView.firstVisibleCellX;

    for (float x = -viewport.m_gridView.startX;
        x < canvasSize.x;
        x += viewport.m_cellSize, ++worldX)
    {
        float lineX = canvasPosition.x + x;

        bool isChunkBorder =
            worldX % chunkSize == 0;

        ImU32 color =
            isChunkBorder
            ? style.m_chunkGridColor
            : style.m_gridColor;

        float thickness =
            isChunkBorder
            ? 2.0f
            : 1.0f;

        drawList->AddRectFilled(
            ImVec2(
                lineX,
                canvasPosition.y
            ),
            ImVec2(
                lineX + thickness,
                canvasPosition.y + canvasSize.y
            ),
            color
        );
    }
}

void drawWallPreview(const Viewport& viewport, const ToolSettings& toolSettings,
    ImDrawList* drawList,
    float cellLeft,
    float cellTop,
    HoveredWall hoveredWall,
    ImU32 color
)
{
    SDL_Texture* texture =
        nullptr;

    if (toolSettings.m_edgeTexture)
    {
        texture =
            toolSettings.m_edgeTexture(
                toolSettings.m_activeEdgeId,
                WorldView::edgeTextureSize(viewport)
            );
    }

    if (texture == nullptr)
    {
        return;
    }

    ImVec2 center;
    bool horizontal;

    switch (hoveredWall)
    {
    case HoveredWall::North:
        center = ImVec2(
            cellLeft + viewport.m_cellSize * 0.5f,
            cellTop
        );
        horizontal = true;
        break;

    case HoveredWall::East:
        center = ImVec2(
            cellLeft + viewport.m_cellSize,
            cellTop + viewport.m_cellSize * 0.5f
        );
        horizontal = false;
        break;

    case HoveredWall::South:
        center = ImVec2(
            cellLeft + viewport.m_cellSize * 0.5f,
            cellTop + viewport.m_cellSize
        );
        horizontal = true;
        break;

    case HoveredWall::West:
        center = ImVec2(
            cellLeft,
            cellTop + viewport.m_cellSize * 0.5f
        );
        horizontal = false;
        break;

    case HoveredWall::None:
        return;
    }

    WorldView::drawEdgeIcon(viewport,
        drawList,
        texture,
        center,
        horizontal,
        color
    );
}

void drawMiscPreview(const Viewport& viewport, const Hover& hover, const ToolSettings& toolSettings, const ChunkManager& map,
    ImDrawList* drawList
)
{
    if (!hover.m_hasHoveredCell ||
        !toolSettings.m_paintMisc ||
        toolSettings.m_activeMiscId.empty())
    {
        return;
    }

    SDL_Texture* texture =
        toolSettings.m_miscTexture
        ? toolSettings.m_miscTexture(
            toolSettings.m_activeMiscId,
            WorldView::edgeTextureSize(viewport)
        )
        : nullptr;

    if (texture == nullptr)
    {
        return;
    }

    const MapColor* mapColor =
        map
        .colorPalette()
        .find(
            toolSettings.m_activeMiscColorId
        );

    EdgeColor color
    {
        255,
        255,
        255,
        255
    };

    if (mapColor != nullptr)
    {
        color =
            mapColor->color;
    }

    const float pulse =
        static_cast<float>(
            std::sin(
                ImGui::GetTime() * 5.0
            ) * 0.5 + 0.5
            );

    const int alpha =
        static_cast<int>(
            40.0f +
            pulse * 215.0f
            );

    const ImU32 tint =
        IM_COL32(
            color.red,
            color.green,
            color.blue,
            alpha
        );

    const float margin =
        viewport.m_cellSize * 0.1f;

    const ImVec2 topLeft(
        hover.m_hoverCellLeft + margin,
        hover.m_hoverCellTop + margin
    );

    const ImVec2 topRight(
        hover.m_hoverCellLeft +
        viewport.m_cellSize - margin,
        hover.m_hoverCellTop + margin
    );

    const ImVec2 bottomRight(
        hover.m_hoverCellLeft +
        viewport.m_cellSize - margin,
        hover.m_hoverCellTop +
        viewport.m_cellSize - margin
    );

    const ImVec2 bottomLeft(
        hover.m_hoverCellLeft + margin,
        hover.m_hoverCellTop +
        viewport.m_cellSize - margin
    );

}

void drawHover(const Viewport& viewport, const Hover& hover, const WallPainting& painter, const ToolSettings& toolSettings,
    ImDrawList* drawList,
    ImVec2 canvasPosition,
    ImVec2 canvasSize
)
{
    if (!hover.m_hasHoveredCell ||
        toolSettings.m_activeTool != EditorTool::Pencil)
    {
        return;
    }

    const bool isDragging =
        painter.m_isPainting &&
        ImGui::IsMouseDragging(
            ImGuiMouseButton_Left,
            2.0f
        );

    ImGui::SetMouseCursor(
        ImGuiMouseCursor_None
    );

    HoveredWall previewWall =
        hover.m_hoveredWall;

    float previewCellLeft =
        hover.m_hoverCellLeft;

    float previewCellTop =
        hover.m_hoverCellTop;

    if (painter.m_isPainting &&
        painter.m_paintOrientation.has_value())
    {
        if (*painter.m_paintOrientation ==
            WallOrientation::Horizontal)
        {
            previewCellTop +=
                (painter.m_paintRow -
                    hover.m_hoveredCellY) *
                viewport.m_cellSize;

            previewWall =
                painter.m_paintWallDirection ==
                WallDirection::North
                ? HoveredWall::North
                : HoveredWall::South;
        }
        else
        {
            previewCellLeft +=
                (painter.m_paintColumn -
                    hover.m_hoveredCellX) *
                viewport.m_cellSize;

            previewWall =
                painter.m_paintWallDirection ==
                WallDirection::East
                ? HoveredWall::East
                : HoveredWall::West;
        }
    }
    const float pulse =
        0.5f +
        0.5f * std::sin(
            static_cast<float>(
                ImGui::GetTime()
                ) * 6.0f
        );

    const int alpha =
        static_cast<int>(
            40.0f + pulse * 215.0f
            );

    const ImU32 previewColor =
        IM_COL32(
            255,
            255,
            255,
            alpha
        );
    WorldView::drawWallPreview(viewport, toolSettings,
        drawList,
        previewCellLeft,
        previewCellTop,
        previewWall,
        previewColor
    );
}

void drawEdgeIcon(const Viewport& viewport,
     ImDrawList* drawList,
     SDL_Texture* texture,
     ImVec2 center,
     bool horizontal,
     ImU32 color
 )
{
     if (texture == nullptr)
     {
         return;
     }

     const float iconSize =
         viewport.m_cellSize;

     const float halfSize =
         iconSize * 0.5f;

     const ImVec2 topLeft(
         center.x - halfSize,
         center.y - halfSize
     );

     const ImVec2 topRight(
         center.x + halfSize,
         center.y - halfSize
     );

     const ImVec2 bottomRight(
         center.x + halfSize,
         center.y + halfSize
     );

     const ImVec2 bottomLeft(
         center.x - halfSize,
         center.y + halfSize
     );

     const ImTextureID textureId =
         (ImTextureID)(intptr_t)texture;

     if (horizontal)
     {
         drawList->AddImageQuad(
             textureId,
             topLeft,
             topRight,
             bottomRight,
             bottomLeft,
             ImVec2(0.0f, 0.0f),
             ImVec2(1.0f, 0.0f),
             ImVec2(1.0f, 1.0f),
             ImVec2(0.0f, 1.0f),
             color
         );
     }
     else
     {
         drawList->AddImageQuad(
             textureId,
             topLeft,
             topRight,
             bottomRight,
             bottomLeft,
             ImVec2(0.0f, 1.0f),
             ImVec2(0.0f, 0.0f),
             ImVec2(1.0f, 0.0f),
             ImVec2(1.0f, 1.0f),
             color
         );
     }
 }

void drawEraserPreview(const Viewport& viewport, const Hover& hover, const Eraser& eraser, const ToolSettings& toolSettings,
     ImDrawList* drawList
 )
{
     if (!hover.m_hasHoveredCell ||
         toolSettings.m_activeTool != EditorTool::Eraser)
     {
         return;
     }

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

     const ImVec2 rectangleEnd(
         rectangleStart.x +
         eraser.m_eraserSize * viewport.m_cellSize,

         rectangleStart.y +
         eraser.m_eraserSize * viewport.m_cellSize
     );

     drawList->AddRect(
         rectangleStart,
         rectangleEnd,
         IM_COL32(255, 100, 100, 255),
         0.0f,
         0,
         2.0f
     );
 }

void drawPlayerMarker(const Viewport& viewport, const MapPlayerMarker& player,
     ImDrawList* drawList,
     ImVec2 canvasPosition
 )
{
     if (!player.visible)
     {
         return;
     }

     const float cellLeft =
         canvasPosition.x +
         (
             player.x -
             viewport.m_gridView.firstVisibleCellX
             ) * viewport.m_cellSize -
         viewport.m_gridView.startX;

     const float cellTop =
         canvasPosition.y +
         (
             player.y -
             viewport.m_gridView.firstVisibleCellY
             ) * viewport.m_cellSize -
         viewport.m_gridView.startY;

     const ImVec2 center(
         cellLeft + viewport.m_cellSize * 0.5f,
         cellTop + viewport.m_cellSize * 0.5f
     );

     const float radius =
         viewport.m_cellSize * 0.32f;

     ImVec2 tip;
     ImVec2 left;
     ImVec2 right;

     switch (player.direction)
     {
     case MapFacingDirection::North:
         tip = ImVec2(center.x, center.y - radius);
         left = ImVec2(
             center.x - radius * 0.7f,
             center.y + radius * 0.65f
         );
         right = ImVec2(
             center.x + radius * 0.7f,
             center.y + radius * 0.65f
         );
         break;

     case MapFacingDirection::East:
         tip = ImVec2(center.x + radius, center.y);
         left = ImVec2(
             center.x - radius * 0.65f,
             center.y - radius * 0.7f
         );
         right = ImVec2(
             center.x - radius * 0.65f,
             center.y + radius * 0.7f
         );
         break;

     case MapFacingDirection::South:
         tip = ImVec2(center.x, center.y + radius);
         left = ImVec2(
             center.x + radius * 0.7f,
             center.y - radius * 0.65f
         );
         right = ImVec2(
             center.x - radius * 0.7f,
             center.y - radius * 0.65f
         );
         break;

     case MapFacingDirection::West:
         tip = ImVec2(center.x - radius, center.y);
         left = ImVec2(
             center.x + radius * 0.65f,
             center.y + radius * 0.7f
         );
         right = ImVec2(
             center.x + radius * 0.65f,
             center.y - radius * 0.7f
         );
         break;
     }

     drawList->AddTriangleFilled(
         tip,
         left,
         right,
         IM_COL32(255, 220, 0, 255)
     );

     drawList->AddTriangle(
         tip,
         left,
         right,
         IM_COL32(0, 0, 0, 255),
         2.0f
     );
 }

void drawSelection(const Viewport& viewport, const Hover& hover,
    ImDrawList* drawList,
    ImVec2 canvasPosition)
{
    if (!hover.m_hasSelectedCell)
    {
        return;
    }

    float cellLeft =
        canvasPosition.x +
        (hover.m_selectedCellX -
            viewport.m_gridView.firstVisibleCellX) *
        viewport.m_cellSize -
        viewport.m_gridView.startX;

    float cellTop =
        canvasPosition.y +
        (hover.m_selectedCellY -
            viewport.m_gridView.firstVisibleCellY) *
        viewport.m_cellSize -
        viewport.m_gridView.startY;

    drawList->AddRect(
        ImVec2(cellLeft, cellTop),
        ImVec2(
            cellLeft + viewport.m_cellSize,
            cellTop + viewport.m_cellSize
        ),
        IM_COL32(255, 220, 80, 255),
        0.0f,
        0,
        3.0f
    );
}

void drawCoordinates(const Hover& hover, int chunkSize, ImDrawList* drawList, ImVec2 canvasPosition)
{
        float textY =
            canvasPosition.y + 8.0f;

        const float textX =
            canvasPosition.x + 8.0f;

        const auto drawCoordinateText =
            [&](
                const std::string& text
                )
            {
                const ImVec2 position(
                    textX,
                    textY
                );

                drawList->AddText(
                    ImVec2(
                        position.x + 1.0f,
                        position.y + 1.0f
                    ),
                    IM_COL32(0, 0, 0, 255),
                    text.c_str()
                );

                drawList->AddText(
                    position,
                    IM_COL32(
                        255,
                        255,
                        255,
                        255
                    ),
                    text.c_str()
                );

                textY +=
                    ImGui::GetTextLineHeightWithSpacing();
            };

        if (hover.m_hasHoveredCell)
        {
            const auto chunkCoordinate =
                [chunkSize](int cellCoordinate)
                {
                    int result =
                        cellCoordinate /
                        chunkSize;

                    if (cellCoordinate < 0 &&
                        cellCoordinate %
                        chunkSize != 0)
                    {
                        --result;
                    }

                    return result;
                };

            const int chunkX =
                chunkCoordinate(
                    hover.m_hoveredCellX
                );

            const int chunkY =
                chunkCoordinate(
                    hover.m_hoveredCellY
                );

            drawCoordinateText(
                "Chunk: " +
                std::to_string(chunkX) +
                " " +
                std::to_string(chunkY)
            );

            drawCoordinateText(
                "Cell: " +
                std::to_string(
                    hover.m_hoveredCellX
                ) +
                " " +
                std::to_string(
                    hover.m_hoveredCellY
                )
            );
        }
        else
        {
            drawCoordinateText(
                "Chunk: -- --"
            );

            drawCoordinateText(
                "Cell: -- --"
            );
        }
    }
}
