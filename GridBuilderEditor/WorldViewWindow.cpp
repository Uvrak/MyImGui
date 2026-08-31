#include "WorldViewWindow.h"

#include "Cell.h"
#include "Chunk.h"

#include "imgui.h"

#include <string>
#include <cmath>
#include <algorithm>
#include <cstdio>

#include "MapSerializer.h"

WorldViewWindow::WorldViewWindow(int chunkSize)
    : m_chunkSize(chunkSize)
{
    loadSettings();
}

void WorldViewWindow::draw(
    EditorTool activeTool,
    const std::string& activeEdgeId,
    const std::string& activeColorId,
    const std::string& activeMiscId,
    const std::string& activeMiscColorId,
    bool paintMisc,
    const EdgeTextureResolver& edgeTexture,
    const MiscTextureResolver& miscTexture,
    const OpenEdgeColorMenuCallback&
    openEdgeColorMenu,
    const OpenMiscColorMenuCallback&
    openMiscColorMenu,
    const std::function<void()>& drawToolbar,
    bool inputBlocked,
    bool showCoordinates,
    bool* isOpen
)
{
    m_activeTool =
        activeTool;

    m_activeEdgeId =
        activeEdgeId;

    m_activeColorId =
        activeColorId;

    m_activeMiscId =
        activeMiscId;

    m_activeMiscColorId =
        activeMiscColorId;

    m_paintMisc =
        paintMisc;

    m_edgeTexture =
        edgeTexture;

    m_miscTexture =
        miscTexture;

    m_openEdgeColorMenu =
        openEdgeColorMenu;

    m_openMiscColorMenu =
        openMiscColorMenu;

    const bool windowVisible =
    ImGui::Begin(
        "Map Editor",
        isOpen
    );

    if (!windowVisible)
    {
        ImGui::End();
        return;
    }

    if (inputBlocked)
    {
        stopPainting();
    }

    bool toolbarBlocksMapInput =
        ImGui::IsPopupOpen(
            "",
            ImGuiPopupFlags_AnyPopupId
        );

    if (drawToolbar)
    {
        drawToolbar();

        toolbarBlocksMapInput =
            toolbarBlocksMapInput ||
            ImGui::IsAnyItemHovered() ||
            ImGui::IsPopupOpen(
                "",
                ImGuiPopupFlags_AnyPopupId
            );

        ImGui::SameLine();
    }

    drawLayerSelector();

    if (toolbarBlocksMapInput ||
        m_blockMapInputFrames > 0)
    {
        stopPainting();
    }

    ImGui::Separator();

    ImVec2 canvasPosition = ImGui::GetCursorScreenPos();
    canvasPosition.x += m_rulerWidth;
    canvasPosition.y += m_rulerHeight;

    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.x -= m_rulerWidth;
    canvasSize.y -= m_rulerHeight;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    if (!inputBlocked &&
        !toolbarBlocksMapInput &&
        m_blockMapInputFrames == 0 &&
        activeTool == EditorTool::Scroll)
    {
        handleZoom(
            canvasPosition,
            canvasSize
        );
    }

    if (m_followPlayer &&
        m_playerMarker.visible)
    {
        centerOnPlayer(
            canvasSize
        );
    }

    updateGridView();

    updateHover(
        canvasPosition,
        canvasSize
    );

    if (!inputBlocked &&
        !toolbarBlocksMapInput &&
        m_blockMapInputFrames == 0)
    {
        handleInput(
            canvasPosition,
            canvasSize,
            activeTool
        );
    }

    updateLongTickStep();


    const ImVec2 mapCanvasEnd(
        canvasPosition.x + canvasSize.x,
        canvasPosition.y + canvasSize.y
    );

    drawList->AddRectFilled(
        canvasPosition,
        mapCanvasEnd,
        m_backgroundColor
    );

    drawList->PushClipRect(
        canvasPosition,
        mapCanvasEnd,
        true
    );

    drawGrid(
        drawList,
        canvasPosition,
        canvasSize
    );

    drawWalls(
        drawList,
        canvasPosition,
        canvasSize
    );

    drawMisc(
        drawList,
        canvasPosition,
        canvasSize
    );

    drawPlayerMarker(
        drawList,
        canvasPosition
    );

    drawNoteTooltip();

    drawList->PopClipRect();

    drawRulers(
        drawList,
        canvasPosition,
        canvasSize
    );

    if (showCoordinates)
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

        if (m_hasHoveredCell)
        {
            const auto chunkCoordinate =
                [this](int cellCoordinate)
                {
                    int result =
                        cellCoordinate /
                        m_chunkSize;

                    if (cellCoordinate < 0 &&
                        cellCoordinate %
                        m_chunkSize != 0)
                    {
                        --result;
                    }

                    return result;
                };

            const int chunkX =
                chunkCoordinate(
                    m_hoveredCellX
                );

            const int chunkY =
                chunkCoordinate(
                    m_hoveredCellY
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
                    m_hoveredCellX
                ) +
                " " +
                std::to_string(
                    m_hoveredCellY
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

    if (!inputBlocked &&
        !toolbarBlocksMapInput &&
        m_blockMapInputFrames == 0 &&
        activeTool == EditorTool::Pencil)
    {
        if (m_paintMisc)
        {
            handleMisc(
                canvasPosition,
                canvasSize
            );
        }
        else
        {
            handlePencil(
                canvasPosition,
                canvasSize
            );
        }
    }

    drawNotePopup();

    if (!inputBlocked &&
        (
            activeTool == EditorTool::Pencil ||
            activeTool == EditorTool::Eraser
            ))
    {
        drawList->PushClipRect(
            canvasPosition,
            mapCanvasEnd,
            true
        );

        if (activeTool == EditorTool::Pencil)
        {
            if (m_paintMisc)
            {
                drawMiscPreview(
                    drawList
                );
            }
            else
            {
                drawHover(
                    drawList,
                    canvasPosition,
                    canvasSize
                );
            }
        }
        else
        {
            drawEraserPreview(
                drawList
            );
        }

        drawList->PopClipRect();
    }
    ImGui::Dummy(
        ImVec2(
            canvasSize.x +
            m_rulerWidth,
            canvasSize.y +
            m_rulerHeight
        )
    );

    if (m_blockMapInputFrames > 0)
    {
        --m_blockMapInputFrames;
    }

    ImGui::End();


}

bool WorldViewWindow::saveMap(
    const std::string& filename
) 
{
    const bool saved =
        MapSerializer::save(
            m_chunkManager,
            filename
        );

    if (saved)
    {
        m_hasUnsavedChanges = false;
    }

    return saved;
}

bool WorldViewWindow::loadMap(
    const std::string& filename
)
{
    stopPainting();

    const bool loaded =
        MapSerializer::load(
            m_chunkManager,
            filename
        );

    if (loaded)
    {
        m_hasUnsavedChanges = false;
    }

    return loaded;
}

bool WorldViewWindow::hasUnsavedChanges() const
{
    return m_hasUnsavedChanges;
}

void WorldViewWindow::newMap()
{
    stopPainting();

    m_chunkManager.clear();

    m_cameraX = 0.0f;
    m_cameraY = 0.0f;

    m_hasHoveredCell = false;
    m_hasSelectedCell = false;
    m_hasUnsavedChanges = false;
}

void WorldViewWindow::removeEdgeFromAllCells(
    const std::string& edgeId
)
{
    m_chunkManager.removeEdgeFromAllCells(
        edgeId
    );
    m_hasUnsavedChanges = true;
}

void WorldViewWindow::renameEdgeInAllCells(
    const std::string& oldEdgeId,
    const std::string& newEdgeId
)
{
    m_chunkManager.renameEdgeInAllCells(
        oldEdgeId,
        newEdgeId
    );

    m_hasUnsavedChanges = true;
}

void WorldViewWindow::renameMiscInAllCells(
    const std::string& oldMiscId,
    const std::string& newMiscId
)
{
    m_chunkManager.renameMiscInAllCells(
        oldMiscId,
        newMiscId
    );

    m_hasUnsavedChanges = true;
}

int WorldViewWindow::edgeTextureSize() const
{
    return std::max(
        1,
        static_cast<int>(
            std::round(m_cellSize)
            )
    );
}

bool WorldViewWindow::isMouseOverCanvas() const
{
    return m_hasHoveredCell;
}

void WorldViewWindow::drawWalls(
    ImDrawList* drawList,
    ImVec2 canvasPosition,
    ImVec2 canvasSize
)
{
    const int currentLayer =
        m_chunkManager.activeLayer();

    const int lowerLayer =
        currentLayer - 1;

    if (m_showLowerLayer &&
        m_chunkManager.hasLayer(
            lowerLayer
        ))
    {
        drawLayerWalls(
            drawList,
            canvasPosition,
            canvasSize,
            lowerLayer,
            IM_COL32(255, 255, 255, 70),
            false
        );
    }

    drawLayerWalls(
        drawList,
        canvasPosition,
        canvasSize,
        currentLayer,
        IM_COL32_WHITE,
        true
    );
}

void WorldViewWindow::drawMisc(
    ImDrawList* drawList,
    ImVec2 canvasPosition,
    ImVec2 canvasSize
)
{
    const int visibleCellCountX =
        static_cast<int>(
            canvasSize.x / m_cellSize
            ) + 2;

    const int visibleCellCountY =
        static_cast<int>(
            canvasSize.y / m_cellSize
            ) + 2;

    const int layer =
        m_chunkManager.activeLayer();

    for (int offsetY = 0;
        offsetY < visibleCellCountY;
        ++offsetY)
    {
        const int cellY =
            m_gridView.firstVisibleCellY +
            offsetY;

        for (int offsetX = 0;
            offsetX < visibleCellCountX;
            ++offsetX)
        {
            const int cellX =
                m_gridView.firstVisibleCellX +
                offsetX;

            const Cell* cell =
                m_chunkManager.findCell(
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
                m_miscTexture
                ? m_miscTexture(
                    cell->miscId(),
                    edgeTextureSize()
                )
                : nullptr;

            const MapColor* mapColor =
                m_chunkManager
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
                offsetX * m_cellSize -
                m_gridView.startX;

            const float cellTop =
                canvasPosition.y +
                offsetY * m_cellSize -
                m_gridView.startY;

            if (cell->hasMiscText())
            {
                ImU32 noteTint =
                    tint;

                const MapColor* noteColor =
                    m_chunkManager
                    .colorPalette()
                    .find(
                        m_chunkManager.miscColorId(
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
                    m_cellSize * 0.3f;

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
                    m_cellSize * 0.1f;

                const ImVec2 topLeft(
                    cellLeft + margin,
                    cellTop + margin
                );

                const ImVec2 topRight(
                    cellLeft + m_cellSize - margin,
                    cellTop + margin
                );

                const ImVec2 bottomRight(
                    cellLeft + m_cellSize - margin,
                    cellTop + m_cellSize - margin
                );

                const ImVec2 bottomLeft(
                    cellLeft + margin,
                    cellTop + m_cellSize - margin
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

void WorldViewWindow::drawLayerWalls(
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
            canvasSize.x / m_cellSize
            ) + 2;

    const int visibleCellCountY =
        static_cast<int>(
            canvasSize.y / m_cellSize
            ) + 2;

    for (int offsetY = 0;
        offsetY < visibleCellCountY;
        ++offsetY)
    {
        const int cellY =
            m_gridView.firstVisibleCellY +
            offsetY;

        for (int offsetX = 0;
            offsetX < visibleCellCountX;
            ++offsetX)
        {
            const int cellX =
                m_gridView.firstVisibleCellX +
                offsetX;

            const Cell* cell =
                m_chunkManager.findCell(
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
                offsetX * m_cellSize -
                m_gridView.startX;

            const float cellTop =
                canvasPosition.y +
                offsetY * m_cellSize -
                m_gridView.startY;

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
                        isHoveredEdge(
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

                    if (m_edgeTexture)
                    {
                        texture =
                            m_edgeTexture(
                                edgeId,
                                edgeTextureSize()
                            );
                    }

                    const std::string& colorId =
                        cell->edgeColorId(
                            direction
                        );

                    const MapColor* mapColor =
                        m_chunkManager
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
                    drawEdgeIcon(
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
                    m_cellSize * 0.5f,
                    cellTop
                ),
                true
            );

            drawCellEdge(
                EdgeDirection::East,
                ImVec2(
                    cellLeft + m_cellSize,
                    cellTop +
                    m_cellSize * 0.5f
                ),
                false
            );

            drawCellEdge(
                EdgeDirection::South,
                ImVec2(
                    cellLeft +
                    m_cellSize * 0.5f,
                    cellTop + m_cellSize
                ),
                true
            );

            drawCellEdge(
                EdgeDirection::West,
                ImVec2(
                    cellLeft,
                    cellTop +
                    m_cellSize * 0.5f
                ),
                false
            );
        }
    }
}

void WorldViewWindow::updateGridView()
{
    m_gridView.startX =
        std::fmod(m_cameraX, m_cellSize);

    m_gridView.startY =
        std::fmod(m_cameraY, m_cellSize);

    if (m_gridView.startX < 0.0f)
        m_gridView.startX += m_cellSize;

    if (m_gridView.startY < 0.0f)
        m_gridView.startY += m_cellSize;

    m_gridView.firstVisibleCellX =
        static_cast<int>(std::floor(
            m_cameraX / m_cellSize));

    m_gridView.firstVisibleCellY =
        static_cast<int>(std::floor(
            m_cameraY / m_cellSize));
}

bool WorldViewWindow::isMouseInsideCanvas(
    ImVec2 canvasPosition,
    ImVec2 canvasSize,
    ImVec2& mouseCanvasPosition
) const
{
    ImVec2 mousePosition = ImGui::GetMousePos();

    mouseCanvasPosition = ImVec2(
        mousePosition.x - canvasPosition.x,
        mousePosition.y - canvasPosition.y
    );

    return
        mouseCanvasPosition.x >= 0.0f &&
        mouseCanvasPosition.y >= 0.0f &&
        mouseCanvasPosition.x < canvasSize.x &&
        mouseCanvasPosition.y < canvasSize.y;
}

void WorldViewWindow::handleZoom(
    ImVec2 canvasPosition,
    ImVec2 canvasSize)
{
    ImVec2 mouseCanvasPosition;

    if (!isMouseInsideCanvas(
        canvasPosition,
        canvasSize,
        mouseCanvasPosition))
    {
        return;
    }

    float mouseWheel = ImGui::GetIO().MouseWheel;

    if (m_activeTool == EditorTool::Pencil &&
        !ImGui::GetIO().KeyCtrl)
    {

        return;
    }
    if (mouseWheel != 0.0f)
    {
        m_cellSize += mouseWheel * 2.0f;

        if (m_cellSize < 5.0f)
        {
            m_cellSize = 5.0f;
        }

        if (m_cellSize > 100.0f)
        {
            m_cellSize = 100.0f;
        }
    }
}

int WorldViewWindow::calculateLabelStep() const
{
    if (m_cellSize < 15.0f)
        return 8;

    if (m_cellSize < 25.0f)
        return 4;

    if (m_cellSize < 40.0f)
        return 2;

    return 1;
}

void WorldViewWindow::handleInput(
    const ImVec2& canvasPosition,
    const ImVec2& canvasSize,
    EditorTool activeTool
)
{
    switch (activeTool)
    {

    case EditorTool::Eraser:
        handleEraser(
            canvasPosition,
            canvasSize
        );
        return;

    case EditorTool::Scroll:
    {
        ImVec2 mouseCanvasPosition;

        if (isMouseInsideCanvas(
            canvasPosition,
            canvasSize,
            mouseCanvasPosition
        ))
        {
            handlePan();
        }

        return;
    }
    }
}
void WorldViewWindow::drawRulers(ImDrawList* drawList, ImVec2 canvasPosition, ImVec2 canvasSize)
{


    drawList->AddRectFilled(
        ImVec2(
            canvasPosition.x - m_rulerWidth,
            canvasPosition.y
        ),
        ImVec2(
            canvasPosition.x,
            canvasPosition.y + canvasSize.y
        ),
        m_rulerColor
    );

    for (float y = -m_gridView.startY;
        y < canvasSize.y;
        y += m_cellSize)
    {
        float lineTop = canvasPosition.y + y;
        float textCenterY = lineTop + m_cellSize * 0.5f;

        int cellOffset = static_cast<int>(
            std::floor((y + m_gridView.startY) / m_cellSize)
            );

        int index =
            m_gridView.firstVisibleCellY + cellOffset;

        bool majorTick =
            (index % m_labelStep == 0);

        bool longTick =
            (index % m_longTickStep == 0);

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
            m_rulerTextColor
        );

        if (majorTick)
        {
            std::string text = std::to_string(index);
            ImVec2 textSize = ImGui::CalcTextSize(text.c_str());

            drawList->AddText(
                ImVec2(
                    canvasPosition.x - m_rulerWidth +
                    (m_rulerWidth - textSize.x) * 0.5f,
                    textCenterY - textSize.y * 0.5f
                ),
                m_rulerTextColor,
                text.c_str()
            );
        }
    }

    for (float x = -m_gridView.startX;
        x < canvasSize.x;
        x += m_cellSize)
    {
        float lineX = canvasPosition.x + x;

        int cellOffset = static_cast<int>(
            std::floor((x + m_gridView.startX) / m_cellSize)
            );

        int index =
            m_gridView.firstVisibleCellX + cellOffset;

        bool majorTick =
            (index % m_labelStep == 0);

        bool longTick =
            (index % m_longTickStep == 0);

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
            m_rulerTextColor
        );

        if (majorTick)
        {
            std::string text = std::to_string(index);
            ImVec2 textSize = ImGui::CalcTextSize(text.c_str());

            drawList->AddText(
                ImVec2(
                    lineX + 3.0f,
                    canvasPosition.y - m_rulerHeight +
                    (m_rulerHeight - textSize.y) * 0.5f - 3.0f
                ),
                m_rulerTextColor,
                text.c_str()
            );
        }
    }

    drawList->AddRectFilled(
        ImVec2(
            canvasPosition.x - m_rulerWidth,
            canvasPosition.y - m_rulerHeight
        ),
        ImVec2(
            canvasPosition.x,
            canvasPosition.y
        ),
        IM_COL32(45, 45, 50, 255)
    );

    drawList->AddRect(
        ImVec2(
            canvasPosition.x - m_rulerWidth,
            canvasPosition.y - m_rulerHeight
        ),
        ImVec2(
            canvasPosition.x,
            canvasPosition.y
        ),
        IM_COL32(90, 90, 95, 255)
    );

}

void WorldViewWindow::drawGrid(
    ImDrawList* drawList,
    ImVec2 canvasPosition,
    ImVec2 canvasSize)
{
    int worldY = m_gridView.firstVisibleCellY;

    for (float y = -m_gridView.startY;
        y < canvasSize.y;
        y += m_cellSize, ++worldY)
    {
        float lineY = canvasPosition.y + y;

        bool isChunkBorder =
            worldY % m_chunkSize == 0;

        ImU32 color =
            isChunkBorder
            ? m_chunkGridColor
            : m_gridColor;

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

    int worldX = m_gridView.firstVisibleCellX;

    for (float x = -m_gridView.startX;
        x < canvasSize.x;
        x += m_cellSize, ++worldX)
    {
        float lineX = canvasPosition.x + x;

        bool isChunkBorder =
            worldX % m_chunkSize == 0;

        ImU32 color =
            isChunkBorder
            ? m_chunkGridColor
            : m_gridColor;

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

void WorldViewWindow::updateLongTickStep()
{
    m_longTickStep = 4;

    while (true)
    {
        std::string left =
            std::to_string(m_gridView.firstVisibleCellX);

        std::string right =
            std::to_string(m_gridView.firstVisibleCellX +
                static_cast<int>(1000.0f / m_cellSize));

        float textWidth = std::max(
            ImGui::CalcTextSize(left.c_str()).x,
            ImGui::CalcTextSize(right.c_str()).x
        );

        float availableWidth =
            m_longTickStep * m_cellSize;

        if (textWidth + 6.0f <= availableWidth)
            break;

        m_longTickStep *= 2;
    }

    m_labelStep = m_longTickStep;
}

void WorldViewWindow::handleHorizontalToVerticalTurn(
    int cellX,
    int cellY,
    int deltaY)
{
    m_verticalPaintDirection =
        deltaY > 0 ? 1 : -1;

    WallDirection horizontalDirection =
        m_paintWallDirection;
    
   m_paintOrientation =
    WallOrientation::Vertical;



m_paintColumn = cellX;
    m_paintWallDirection =
        m_horizontalPaintDirection > 0
        ? WallDirection::East
        : WallDirection::West;
    int firstVerticalCellY;

    switch (horizontalDirection)
    {
    case WallDirection::North:

        if (deltaY > 0)
        {
            // West -> Ost -> Süd
            firstVerticalCellY = m_paintRow;
        }
        else
        {
            // West -> Ost -> Nord
            firstVerticalCellY = m_paintRow - 1;
        }

        break;

    case WallDirection::South:

        if (deltaY > 0)
        {
            // Ost -> West -> Süd
            firstVerticalCellY = m_paintRow + 1;
        }
        else
        {
            // Ost -> West -> Nord
            firstVerticalCellY = m_paintRow;
        }

        break;
    }

    if (m_paintWallDirection ==
        WallDirection::East)
    {
        applyEdge(
            m_paintColumn,
            firstVerticalCellY,
            EdgeDirection::East
        );
    }
    else
    {
        applyEdge(
            m_paintColumn,
            firstVerticalCellY,
            EdgeDirection::West
        );
    }

    m_lastPaintCellX =
        m_paintColumn;

    m_lastPaintCellY =
        firstVerticalCellY;

    return;
}

void WorldViewWindow::handleVerticalToHorizontalTurn(
    int cellX,
    int cellY,
    int deltaX)
{
    m_horizontalPaintDirection =
        deltaX > 0 ? 1 : -1;

    WallDirection verticalWall =
        m_paintWallDirection;

    m_paintOrientation =
        WallOrientation::Horizontal;


    m_paintRow = cellY;

    // Die bisherige vertikale Zugrichtung entscheidet,
    // ob eine Nord- oder Südwand entsteht.
    m_paintWallDirection =
        m_verticalPaintDirection > 0
        ? WallDirection::South
        : WallDirection::North;

    int firstHorizontalCellX = m_paintColumn;

    if (verticalWall == WallDirection::East)
    {
        if (deltaX > 0)
        {
            // An einer Ostwand nach Osten abbiegen:
            // erste Zelle liegt rechts der bisherigen Zelle.
            firstHorizontalCellX =
                m_paintColumn + 1;
        }
        else
        {
            // An einer Ostwand nach Westen abbiegen.
            firstHorizontalCellX =
                m_paintColumn;
        }
    }
    else
    {
        if (deltaX > 0)
        {
            // An einer Westwand nach Osten abbiegen.
            firstHorizontalCellX =
                m_paintColumn;
        }
        else
        {
            // An einer Westwand nach Westen abbiegen:
            // erste Zelle liegt links der bisherigen Zelle.
            firstHorizontalCellX =
                m_paintColumn - 1;
        }
    }

    if (m_paintWallDirection ==
        WallDirection::South)
    {
        applyEdge(
            firstHorizontalCellX,
            m_paintRow,
            EdgeDirection::South
        );
    }
    else
    {
        applyEdge(
            firstHorizontalCellX,
            m_paintRow,
            EdgeDirection::North
        );
    }

    m_lastPaintCellX =
        firstHorizontalCellX;

    m_lastPaintCellY =
        m_paintRow;

    return;
}

void WorldViewWindow::stopPainting()
{
    m_isPainting = false;
    m_isRemovingEdges = false;
    m_isBacktracking = false;
    m_paintOrientation.reset();
    m_paintedEdges.clear();
}

void WorldViewWindow::startPainting(
    int cellX,
    int cellY,
    HoveredWall hoveredWall)
{
    
    m_paintedEdges.clear();
    m_isBacktracking = false;
    switch (hoveredWall)
    {
    case HoveredWall::North:
        m_paintOrientation = WallOrientation::Horizontal;
        m_paintRow = cellY;
        m_paintWallDirection = WallDirection::North;
        break;

    case HoveredWall::South:
        m_paintOrientation = WallOrientation::Horizontal;
        m_paintRow = cellY;
        m_paintWallDirection = WallDirection::South;
        break;

    case HoveredWall::East:
        m_paintOrientation = WallOrientation::Vertical;
        m_paintColumn = cellX;
        m_paintWallDirection = WallDirection::East;
        break;

    case HoveredWall::West:
        m_paintOrientation = WallOrientation::Vertical;
        m_paintColumn = cellX;
        m_paintWallDirection = WallDirection::West;
        break;

    case HoveredWall::None:
        return;
    }

    m_isPainting = true;

    EdgeDirection wall;

    switch (m_paintWallDirection)
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
    if (*m_paintOrientation ==
        WallOrientation::Horizontal)
    {
        m_lastPaintCellX = cellX;
        m_lastPaintCellY = m_paintRow;
    }
    else
    {
        m_lastPaintCellX = m_paintColumn;
        m_lastPaintCellY = cellY;
    }

    Cell& cell =
        m_chunkManager.cell(
            cellX,
            cellY
        );

    m_isRemovingEdges =
        cell.hasEdge(wall) &&
        cell.edgeId(wall) ==
        m_activeEdgeId &&
        cell.edgeColorId(wall) ==
        m_activeColorId;

    applyEdge(
        cellX,
        cellY,
        wall
    );
}

void WorldViewWindow::updatePainting(
    int cellX,
    int cellY,
    float localX,
    float localY)
{
    if (m_isRemovingEdges)
    {
        switch (m_hoveredWall)
        {
        case HoveredWall::North:
            removeEdge(
                cellX,
                cellY,
                EdgeDirection::North
            );
            break;

        case HoveredWall::East:
            removeEdge(
                cellX,
                cellY,
                EdgeDirection::East
            );
            break;

        case HoveredWall::South:
            removeEdge(
                cellX,
                cellY,
                EdgeDirection::South
            );
            break;

        case HoveredWall::West:
            removeEdge(
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
        cellX - m_lastPaintCellX;

    int deltaY =
        cellY - m_lastPaintCellY;

    const ImVec2 mouseMovement =
        ImGui::GetIO().MouseDelta;

    const int previousHorizontalDirection =
        m_horizontalPaintDirection;
    
    const int previousVerticalDirection =
        m_verticalPaintDirection;

    if (m_paintOrientation ==
        WallOrientation::Horizontal &&
        deltaX != 0)
    {
        m_horizontalPaintDirection =
            deltaX > 0 ? 1 : -1;
    }

    if (m_paintOrientation ==
        WallOrientation::Vertical &&
        deltaY != 0)
    {
        m_verticalPaintDirection =
            deltaY > 0 ? 1 : -1;
    }
    const bool reversedVerticalDirection =
        m_paintOrientation ==
        WallOrientation::Vertical &&
        deltaY != 0 &&
        previousVerticalDirection != 0 &&
        m_verticalPaintDirection !=
        previousVerticalDirection;

    const bool reversedHorizontalDirection =
        m_paintOrientation ==
        WallOrientation::Horizontal &&
        deltaX != 0 &&
        previousHorizontalDirection != 0 &&
        m_horizontalPaintDirection !=
        previousHorizontalDirection;

    float turnThreshold =
        m_cellSize * 0.25f;

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
            m_cellSize - turnThreshold
            );
    bool turnHorizontalToVertical =
        m_paintOrientation ==
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
        m_cellSize - turnThreshold;

    bool turnVerticalToHorizontal =
        m_paintOrientation ==
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

        handleHorizontalToVerticalTurn(
            cellX,
            cellY,
            turnDirectionY
        );
    }

    if (turnVerticalToHorizontal)
    {
        handleVerticalToHorizontalTurn(
            cellX,
            cellY,
            deltaX
        );
    }

    if (reversedVerticalDirection &&
        m_paintOrientation ==
        WallOrientation::Vertical)
    {
        const EdgeDirection wall=
            m_paintWallDirection ==
            WallDirection::East
            ? EdgeDirection::East
            : EdgeDirection::West;

        applyEdge(
            m_paintColumn,
            m_lastPaintCellY,
            wall
        );
    }
    if (reversedHorizontalDirection &&
        m_paintOrientation ==
        WallOrientation::Horizontal)
    {
        const EdgeDirection wall=
            m_paintWallDirection ==
            WallDirection::North
            ? EdgeDirection::North
            : EdgeDirection::South;

        applyEdge(
            m_lastPaintCellX,
            m_paintRow,
            wall
        );
    }
    switch (m_paintWallDirection)
    {
    case WallDirection::North:
    case WallDirection::South:
    {
        const int step =
            cellX >= m_lastPaintCellX ? 1 : -1;

        const EdgeDirection wall=
            m_paintWallDirection == WallDirection::North
            ? EdgeDirection::North
            : EdgeDirection::South;

        for (int x = m_lastPaintCellX + step;   
            x != cellX + step;
            x += step)
        {
            applyEdge(
                x,
                m_paintRow,
                wall
            );
        }

        break;
    }

    case WallDirection::East:
    case WallDirection::West:
    {
        const int step =
            cellY >= m_lastPaintCellY ? 1 : -1;

        const EdgeDirection wall=
            m_paintWallDirection == WallDirection::East
            ? EdgeDirection::East
            : EdgeDirection::West;

        for (int y = m_lastPaintCellY + step;
            y != cellY + step;
            y += step)
        {
            applyEdge(
                m_paintColumn,
                y,
                wall
            );
        }

        break;
    }
    }
    m_lastPaintCellX = cellX;
    m_lastPaintCellY = cellY;

    if (*m_paintOrientation ==
        WallOrientation::Horizontal)
    {
        m_lastPaintCellX = cellX;
        m_lastPaintCellY = m_paintRow;
    }
    else
    {
        m_lastPaintCellX = m_paintColumn;
        m_lastPaintCellY = cellY;
    }
}

void WorldViewWindow::drawWallPreview(
    ImDrawList* drawList,
    float cellLeft,
    float cellTop,
    HoveredWall hoveredWall,
    ImU32 color
)
{
    SDL_Texture* texture =
        nullptr;

    if (m_edgeTexture)
    {
        texture =
            m_edgeTexture(
                m_activeEdgeId,
                edgeTextureSize()
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
            cellLeft + m_cellSize * 0.5f,
            cellTop
        );
        horizontal = true;
        break;

    case HoveredWall::East:
        center = ImVec2(
            cellLeft + m_cellSize,
            cellTop + m_cellSize * 0.5f
        );
        horizontal = false;
        break;

    case HoveredWall::South:
        center = ImVec2(
            cellLeft + m_cellSize * 0.5f,
            cellTop + m_cellSize
        );
        horizontal = true;
        break;

    case HoveredWall::West:
        center = ImVec2(
            cellLeft,
            cellTop + m_cellSize * 0.5f
        );
        horizontal = false;
        break;

    case HoveredWall::None:
        return;
    }

    drawEdgeIcon(
        drawList,
        texture,
        center,
        horizontal,
        color
    );
}

void WorldViewWindow::drawMiscPreview(
    ImDrawList* drawList
)
{
    if (!m_hasHoveredCell ||
        !m_paintMisc ||
        m_activeMiscId.empty())
    {
        return;
    }

    SDL_Texture* texture =
        m_miscTexture
        ? m_miscTexture(
            m_activeMiscId,
            edgeTextureSize()
        )
        : nullptr;

    if (texture == nullptr)
    {
        return;
    }

    const MapColor* mapColor =
        m_chunkManager
        .colorPalette()
        .find(
            m_activeMiscColorId
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
        m_cellSize * 0.1f;

    const ImVec2 topLeft(
        m_hoverCellLeft + margin,
        m_hoverCellTop + margin
    );

    const ImVec2 topRight(
        m_hoverCellLeft +
        m_cellSize - margin,
        m_hoverCellTop + margin
    );

    const ImVec2 bottomRight(
        m_hoverCellLeft +
        m_cellSize - margin,
        m_hoverCellTop +
        m_cellSize - margin
    );

    const ImVec2 bottomLeft(
        m_hoverCellLeft + margin,
        m_hoverCellTop +
        m_cellSize - margin
    );

}

HoveredWall WorldViewWindow::getHoveredWall(
    float localX,
    float localY,
    float selectionWidth) const
{
    float north = localY;
    float east = m_cellSize - localX;
    float south = m_cellSize - localY;
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

void WorldViewWindow::updateHover(
    const ImVec2& canvasPosition,
    const ImVec2& canvasSize
)
{
    ImVec2 mouseCanvasPosition;

    m_hasHoveredCell = isMouseInsideCanvas(
        canvasPosition,
        canvasSize,
        mouseCanvasPosition
    );

    if (!m_hasHoveredCell)
    {
        return;
    }

    m_hoveredCellX =
        m_gridView.firstVisibleCellX +
        static_cast<int>(
            (mouseCanvasPosition.x + m_gridView.startX) /
            m_cellSize
            );

    m_hoveredCellY =
        m_gridView.firstVisibleCellY +
        static_cast<int>(
            (mouseCanvasPosition.y + m_gridView.startY) /
            m_cellSize
            );

    m_hoverCellLeft =
        canvasPosition.x +
        (m_hoveredCellX -
            m_gridView.firstVisibleCellX) *
        m_cellSize -
        m_gridView.startX;

    m_hoverCellTop =
        canvasPosition.y +
        (m_hoveredCellY -
            m_gridView.firstVisibleCellY) *
        m_cellSize -
        m_gridView.startY;

    float cellCanvasLeft =
        m_hoverCellLeft - canvasPosition.x;

    float cellCanvasTop =
        m_hoverCellTop - canvasPosition.y;

    m_hoverLocalX =
        mouseCanvasPosition.x - cellCanvasLeft;

    m_hoverLocalY =
        mouseCanvasPosition.y - cellCanvasTop;

    const float selectionWidth =
        m_wallSelectionWidth *
        (
            m_isPainting
            ? 2.5f
            : 1.25f
            );
    m_hoveredWall =
        getHoveredWall(
            m_hoverLocalX,
            m_hoverLocalY,
            selectionWidth
        );
}

void WorldViewWindow::drawHover(
    ImDrawList* drawList,
    ImVec2 canvasPosition,
    ImVec2 canvasSize
)
{
    if (!m_hasHoveredCell ||
        m_activeTool != EditorTool::Pencil)
    {
        return;
    }

    const bool isDragging =
        m_isPainting &&
        ImGui::IsMouseDragging(
            ImGuiMouseButton_Left,
            2.0f
        );

    ImGui::SetMouseCursor(
        ImGuiMouseCursor_None
    );

    HoveredWall previewWall =
        m_hoveredWall;

    float previewCellLeft =
        m_hoverCellLeft;

    float previewCellTop =
        m_hoverCellTop;

    if (m_isPainting &&
        m_paintOrientation.has_value())
    {
        if (*m_paintOrientation ==
            WallOrientation::Horizontal)
        {
            previewCellTop +=
                (m_paintRow -
                    m_hoveredCellY) *
                m_cellSize;

            previewWall =
                m_paintWallDirection ==
                WallDirection::North
                ? HoveredWall::North
                : HoveredWall::South;
        }
        else
        {
            previewCellLeft +=
                (m_paintColumn -
                    m_hoveredCellX) *
                m_cellSize;

            previewWall =
                m_paintWallDirection ==
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
    drawWallPreview(
        drawList,
        previewCellLeft,
        previewCellTop,
        previewWall,
        previewColor
    );
}
PaintedEdge WorldViewWindow::normalizeEdge(
    int cellX,
    int cellY,
    EdgeDirection direction
) const
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

bool WorldViewWindow::isSameEdge(
    const PaintedEdge& first,
    const PaintedEdge& second
) const
{
    return
        first.cellX == second.cellX &&
        first.cellY == second.cellY &&
        first.direction == second.direction;
}

 void WorldViewWindow::drawEdgeIcon(
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
         m_cellSize;

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
 bool WorldViewWindow::isHoveredEdge(
     int cellX,
     int cellY,
     EdgeDirection direction
 ) const
 {
     if (!m_hasHoveredCell ||
         m_activeTool != EditorTool::Pencil ||
         m_isPainting ||
         m_paintMisc)
     {
         return false;
     }

     EdgeDirection hoveredWall;

     switch (m_hoveredWall)
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
         normalizeEdge(
             cellX,
             cellY,
             direction
         );

     const PaintedEdge second =
         normalizeEdge(
             m_hoveredCellX,
             m_hoveredCellY,
             hoveredWall
         );

     return isSameEdge(first, second);
 }
 void WorldViewWindow::drawEraserPreview(
     ImDrawList* drawList
 )
 {
     if (!m_hasHoveredCell ||
         m_activeTool != EditorTool::Eraser)
     {
         return;
     }

     const int radius =
         m_eraserSize / 2;

     const float halfCellSize =
         m_cellSize * 0.5f;

     const float offsetX =
         m_hoverLocalX >= halfCellSize
         ? halfCellSize
         : 0.0f;

     const float offsetY =
         m_hoverLocalY >= halfCellSize
         ? halfCellSize
         : 0.0f;

     const ImVec2 rectangleStart(
         m_hoverCellLeft -
         radius * m_cellSize +
         offsetX,

         m_hoverCellTop -
         radius * m_cellSize +
         offsetY
     );

     const ImVec2 rectangleEnd(
         rectangleStart.x +
         m_eraserSize * m_cellSize,

         rectangleStart.y +
         m_eraserSize * m_cellSize
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
 int WorldViewWindow::activeLayer() const
 {
     return m_chunkManager.activeLayer();
 }

 void WorldViewWindow::setActiveLayer(
     int layer
 )
 {
     if (layer ==
         m_chunkManager.activeLayer())
     {
         return;
     }

     stopPainting();

     m_chunkManager.setActiveLayer(
         layer
     );

     m_hasHoveredCell = false;
     m_hasSelectedCell = false;
 }

 MapColorPalette&
     WorldViewWindow::colorPalette()
 {
     return m_chunkManager.colorPalette();
 }

 const MapColorPalette&
     WorldViewWindow::colorPalette() const
 {
     return m_chunkManager.colorPalette();
 }

 bool WorldViewWindow::removeMapColor(
     const std::string& colorId
 )
 {
     const bool removed =
         m_chunkManager.removeMapColor(
             colorId
         );

     if (removed)
     {
         m_hasUnsavedChanges = true;
     }

     return removed;
 }

 const std::string&
     WorldViewWindow::edgeColorId(
         const std::string& edgeId
     ) const
 {
     return m_chunkManager.edgeColorId(
         edgeId
     );
 }

 const std::string&
     WorldViewWindow::miscColorId(
         const std::string& miscId
     ) const
 {
     return m_chunkManager.miscColorId(
         miscId
     );
 }

 void WorldViewWindow::setEdgeColorId(
     const std::string& edgeId,
     const std::string& colorId
 )
 {
     m_chunkManager.setEdgeColorAssignment(
         edgeId,
         colorId
     );
 }

 void WorldViewWindow::setMiscColorId(
     const std::string& miscId,
     const std::string& colorId
 )
 {
     m_chunkManager.setMiscColorAssignment(
         miscId,
         colorId
     );
 }

 bool WorldViewWindow::showLowerLayer() const
 {
     return m_showLowerLayer;
 }

 void WorldViewWindow::setShowLowerLayer(
     bool show
 )
 {
     m_showLowerLayer = show;
 }

 void WorldViewWindow::blockMapInputOnce()
 {
     m_blockMapInputFrames = 2;
 }

 void WorldViewWindow::eraseArea()
 {
     bool changed = false;

     const int radius =
         m_eraserSize / 2;

     const float halfCellSize =
         m_cellSize * 0.5f;

     const float offsetX =
         m_hoverLocalX >= halfCellSize
         ? 0.5f
         : 0.0f;

     const float offsetY =
         m_hoverLocalY >= halfCellSize
         ? 0.5f
         : 0.0f;

     const float rectangleLeft =
         static_cast<float>(
             m_hoveredCellX - radius
             ) + offsetX;

     const float rectangleTop =
         static_cast<float>(
             m_hoveredCellY - radius
             ) + offsetY;

     const float rectangleRight =
         rectangleLeft +
         static_cast<float>(m_eraserSize);

     const float rectangleBottom =
         rectangleTop +
         static_cast<float>(m_eraserSize);

     const auto isInside =
         [&](float x, float y)
         {
             return
                 x >= rectangleLeft &&
                 x < rectangleRight &&
                 y >= rectangleTop &&
                 y < rectangleBottom;
         };

     const int firstCellX =
         static_cast<int>(
             std::floor(rectangleLeft)
             ) - 1;

     const int firstCellY =
         static_cast<int>(
             std::floor(rectangleTop)
             ) - 1;

     const int lastCellX =
         static_cast<int>(
             std::ceil(rectangleRight)
             );

     const int lastCellY =
         static_cast<int>(
             std::ceil(rectangleBottom)
             );

     for (int cellY = firstCellY;
         cellY <= lastCellY;
         ++cellY)
     {
         for (int cellX = firstCellX;
             cellX <= lastCellX;
             ++cellX)
         {
             const Cell* cell =
                 m_chunkManager.findCell(
                     cellX,
                     cellY,
                     m_chunkManager.activeLayer()
                 );

             if (cell == nullptr)
             {
                 continue;
             }

             if (cell->hasMisc() &&
                 isInside(
                     cellX + 0.5f,
                     cellY + 0.5f
                 ))
             {
                 const int originalLayer =
                     m_chunkManager.activeLayer();

                 const std::string erasedMiscId =
                     cell->miscId();

                 std::string pairedMiscId =
                     erasedMiscId;

                 int pairedLayerOffset = 0;

                 const std::size_t upPosition =
                     pairedMiscId.find("_up");

                 const std::size_t downPosition =
                     pairedMiscId.find("_down");

                 if (upPosition !=
                     std::string::npos)
                 {
                     pairedMiscId.replace(
                         upPosition,
                         3,
                         "_down"
                     );

                     pairedLayerOffset = 1;
                 }
                 else if (downPosition !=
                     std::string::npos)
                 {
                     pairedMiscId.replace(
                         downPosition,
                         5,
                         "_up"
                     );

                     pairedLayerOffset = -1;
                 }

                 m_chunkManager
                     .cell(cellX, cellY)
                     .removeMisc();

                 if (pairedLayerOffset != 0)
                 {
                     const int pairedLayer =
                         originalLayer +
                         pairedLayerOffset;

                     const Cell* pairedCell =
                         m_chunkManager.findCell(
                             cellX,
                             cellY,
                             pairedLayer
                         );

                     if (pairedCell != nullptr &&
                         pairedCell->hasMisc() &&
                         pairedCell->miscId() ==
                         pairedMiscId)
                     {
                         m_chunkManager.setActiveLayer(
                             pairedLayer
                         );

                         m_chunkManager
                             .cell(cellX, cellY)
                             .removeMisc();

                         m_chunkManager.setActiveLayer(
                             originalLayer
                         );
                     }
                 }

                 changed = true;
             }

             const auto eraseEdge =
                 [&](EdgeDirection direction,
                     bool touchesEraser)
                 {
                     const Cell* currentCell =
                         m_chunkManager.findCell(
                             cellX,
                             cellY,
                             m_chunkManager.activeLayer()
                         );

                     if (touchesEraser &&
                         currentCell != nullptr &&
                         currentCell->hasEdge(direction))
                     {
                         removeEdge(
                             cellX,
                             cellY,
                             direction
                         );

                         changed = true;
                     }
                 };

             const float cellLeft =
                 static_cast<float>(cellX);

             const float cellTop =
                 static_cast<float>(cellY);

             const float cellRight =
                 cellLeft + 1.0f;

             const float cellBottom =
                 cellTop + 1.0f;

             const bool horizontalOverlap =
                 cellRight >= rectangleLeft &&
                 cellLeft <= rectangleRight;

             const bool verticalOverlap =
                 cellBottom >= rectangleTop &&
                 cellTop <= rectangleBottom;

             eraseEdge(
                 EdgeDirection::North,
                 horizontalOverlap &&
                 cellTop >= rectangleTop &&
                 cellTop <= rectangleBottom
             );

             eraseEdge(
                 EdgeDirection::East,
                 verticalOverlap &&
                 cellRight >= rectangleLeft &&
                 cellRight <= rectangleRight
             );

             eraseEdge(
                 EdgeDirection::South,
                 horizontalOverlap &&
                 cellBottom >= rectangleTop &&
                 cellBottom <= rectangleBottom
             );

             eraseEdge(
                 EdgeDirection::West,
                 verticalOverlap &&
                 cellLeft >= rectangleLeft &&
                 cellLeft <= rectangleRight
             );
         }
     }

     if (changed)
     {
         m_hasUnsavedChanges = true;
     }
 }

 void WorldViewWindow::drawLayerSelector()
 {
     const int layer =
         activeLayer();

     if (ImGui::Button(
         "-##PreviousLayer"
     ))
     {
         setActiveLayer(
             layer - 1
         );
     }

     ImGui::SameLine();

     ImGui::Text(
         "Level %d",
         layer
     );

     ImGui::SameLine();

     if (ImGui::Button(
         "+##NextLayer"
     ))
     {
         setActiveLayer(
             layer + 1
         );
     }

     ImGui::SameLine();

     if (ImGui::Checkbox(
         "Show lower level",
         &m_showLowerLayer
     ))
     {
         saveSettings();
     }

     ImGui::SameLine();

     ImGui::Checkbox(
         "Follow Player",
         &m_followPlayer
     );
 }

 void WorldViewWindow::handleEraserAutoPan(
     const ImVec2& canvasPosition,
     const ImVec2& canvasSize
 )
 {
     const int radius =
         m_eraserSize / 2;

     const float halfCellSize =
         m_cellSize * 0.5f;

     const float offsetX =
         m_hoverLocalX >= halfCellSize
         ? halfCellSize
         : 0.0f;

     const float offsetY =
         m_hoverLocalY >= halfCellSize
         ? halfCellSize
         : 0.0f;

     const ImVec2 rectangleStart(
         m_hoverCellLeft -
         radius * m_cellSize +
         offsetX,

         m_hoverCellTop -
         radius * m_cellSize +
         offsetY
     );
   

     const float rectangleLeft =
         rectangleStart.x;

     const float rectangleTop =
         rectangleStart.y;

     const float rectangleRight =
         rectangleLeft +
         m_eraserSize * m_cellSize;

     const float rectangleBottom =
         rectangleTop +
         m_eraserSize * m_cellSize;

     const float canvasRight =
         canvasPosition.x +
         canvasSize.x;

     const float canvasBottom =
         canvasPosition.y +
         canvasSize.y;

     const float panSpeed = 5.0f;

     if (rectangleLeft <= canvasPosition.x)
     {
         m_cameraX -= panSpeed;
     }
     else if (rectangleRight >= canvasRight)
     {
         m_cameraX += panSpeed;
     }

     if (rectangleTop <= canvasPosition.y)
     {
         m_cameraY -= panSpeed;
     }
     else if (rectangleBottom >= canvasBottom)
     {
         m_cameraY += panSpeed;
     }
 }

 void WorldViewWindow::loadSettings()
 {
     std::ifstream file(
         "../settings/world_view.cfg"
     );

     if (!file)
     {
         return;
     }

     int showLowerLayer = 1;

     file >> showLowerLayer;

     m_showLowerLayer =
         showLowerLayer != 0;
 }

 void WorldViewWindow::saveSettings() const
 {
     std::filesystem::create_directories(
         "settings"
     );

     std::ofstream file(
         "../settings/world_view.cfg"
     );

     if (!file)
     {
         return;
     }

     file <<
         (m_showLowerLayer ? 1 : 0);
 }

 void WorldViewWindow::centerOnPlayer(
     ImVec2 canvasSize
 )
 {
     m_cameraX =
         (
             static_cast<float>(
                 m_playerMarker.x
                 ) +
             0.5f
             ) * m_cellSize -
         canvasSize.x * 0.5f;

     m_cameraY =
         (
             static_cast<float>(
                 m_playerMarker.y
                 ) +
             0.5f
             ) * m_cellSize -
         canvasSize.y * 0.5f;
 }

 void WorldViewWindow::applyEdge(
     int cellX,
     int cellY,
     EdgeDirection direction
 )
 {
     if (m_activeTool == EditorTool::Pencil &&
         m_isPainting)
     {
         PaintedEdge currentEdge =
             normalizeEdge(
                 cellX,
                 cellY,
                 direction
             );

         /*
          * Wenn dieselbe Kante erneut erreicht wird,
          * wird die letzte Änderung rückgängig gemacht.
          */
         for (auto iterator =
             m_paintedEdges.begin();
             iterator !=
             m_paintedEdges.end();
             ++iterator)
         {
             if (!isSameEdge(
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
                     removeEdge(
                         iterator->cellX,
                         iterator->cellY,
                         iterator->direction
                     );
                 }
                 else
                 {
                     setEdge(
                         iterator->cellX,
                         iterator->cellY,
                         iterator->direction,
                         iterator->previousEdgeId,
                         iterator->previousColorId
                     );
                 }
             }

             m_paintedEdges.erase(
                 iterator
             );

             m_isBacktracking = true;
             return;
         }

         m_isBacktracking = false;

         Cell& cell =
             m_chunkManager.cell(
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

         if (m_isRemovingEdges)
         {
             if (cell.hasEdge(
                 currentEdge.direction
             ))
             {
                 removeEdge(
                     currentEdge.cellX,
                     currentEdge.cellY,
                     currentEdge.direction
                 );

                 currentEdge.changed = true;
             }
         }
         else if (
             currentEdge.previousEdgeId !=
             m_activeEdgeId ||
             currentEdge.previousColorId !=
             m_activeColorId
             )
         {
             setEdge(
                 currentEdge.cellX,
                 currentEdge.cellY,
                 currentEdge.direction,
                 m_activeEdgeId,
                 m_activeColorId
             );

             currentEdge.changed = true;
         }

         if (currentEdge.changed)
         {
             m_hasUnsavedChanges = true;
         }

         m_paintedEdges.push_back(
             currentEdge
         );

         return;
     }

 }

 void WorldViewWindow::setPlayerMarker(
     const MapPlayerMarker& marker
 )
 {
     m_playerMarker =
         marker;
 }

 void WorldViewWindow::drawPlayerMarker(
     ImDrawList* drawList,
     ImVec2 canvasPosition
 )
 {
     if (!m_playerMarker.visible)
     {
         return;
     }

     const float cellLeft =
         canvasPosition.x +
         (
             m_playerMarker.x -
             m_gridView.firstVisibleCellX
             ) * m_cellSize -
         m_gridView.startX;

     const float cellTop =
         canvasPosition.y +
         (
             m_playerMarker.y -
             m_gridView.firstVisibleCellY
             ) * m_cellSize -
         m_gridView.startY;

     const ImVec2 center(
         cellLeft + m_cellSize * 0.5f,
         cellTop + m_cellSize * 0.5f
     );

     const float radius =
         m_cellSize * 0.32f;

     ImVec2 tip;
     ImVec2 left;
     ImVec2 right;

     switch (m_playerMarker.direction)
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

void WorldViewWindow::drawSelection(
    ImDrawList* drawList,
    ImVec2 canvasPosition)
{
    if (!m_hasSelectedCell)
    {
        return;
    }

    float cellLeft =
        canvasPosition.x +
        (m_selectedCellX -
            m_gridView.firstVisibleCellX) *
        m_cellSize -
        m_gridView.startX;

    float cellTop =
        canvasPosition.y +
        (m_selectedCellY -
            m_gridView.firstVisibleCellY) *
        m_cellSize -
        m_gridView.startY;

    drawList->AddRect(
        ImVec2(cellLeft, cellTop),
        ImVec2(
            cellLeft + m_cellSize,
            cellTop + m_cellSize
        ),
        IM_COL32(255, 220, 80, 255),
        0.0f,
        0,
        3.0f
    );
}

void WorldViewWindow::setEdge(
    int cellX,
    int cellY,
    EdgeDirection direction,
    const std::string& edgeId,
    const std::string& colorId
)
{
    Cell& cell =
        m_chunkManager.cell(
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
        m_chunkManager
            .cell(cellX, cellY - 1)
            .setEdge(
                EdgeDirection::South,
                edgeId,
                colorId       
            );
        break;

    case EdgeDirection::East:
        m_chunkManager
            .cell(cellX + 1, cellY)
            .setEdge(
                EdgeDirection::West,
                edgeId,
                colorId   
            );
        break;

    case EdgeDirection::South:
        m_chunkManager
            .cell(cellX, cellY + 1)
            .setEdge(
                EdgeDirection::North,
                edgeId,
                colorId
            );
        break;

    case EdgeDirection::West:
        m_chunkManager
            .cell(cellX - 1, cellY)
            .setEdge(
                EdgeDirection::East,
                edgeId,
                colorId   
            );
        break;
    }
}

void WorldViewWindow::removeEdge(
    int cellX,
    int cellY,
    EdgeDirection direction
)
{
    Cell& cell =
        m_chunkManager.cell(
            cellX,
            cellY
        );

    cell.removeEdge(
        direction
    );

    switch (direction)
    {
    case EdgeDirection::North:
        m_chunkManager
            .cell(cellX, cellY - 1)
            .removeEdge(
                EdgeDirection::South
            );
        break;

    case EdgeDirection::East:
        m_chunkManager
            .cell(cellX + 1, cellY)
            .removeEdge(
                EdgeDirection::West
            );
        break;

    case EdgeDirection::South:
        m_chunkManager
            .cell(cellX, cellY + 1)
            .removeEdge(
                EdgeDirection::North
            );
        break;

    case EdgeDirection::West:
        m_chunkManager
            .cell(cellX - 1, cellY)
            .removeEdge(
                EdgeDirection::East
            );
        break;
    }
}

void WorldViewWindow::handlePencil(
    const ImVec2& canvasPosition,
    const ImVec2& canvasSize
)
{
    ImVec2 mouseCanvasPosition;

    const bool mouseInsideCanvas =
        isMouseInsideCanvas(
            canvasPosition,
            canvasSize,
            mouseCanvasPosition
        );

    if (!mouseInsideCanvas)
    {
        if (ImGui::IsMouseReleased(
            ImGuiMouseButton_Left
        ))
        {
            stopPainting();
        }

        return;
    }
    if (ImGui::IsMouseClicked(
        ImGuiMouseButton_Right
    ))
    {
        openHoveredColorMenu();
        return;
    }

    if (ImGui::IsMouseClicked(
        ImGuiMouseButton_Left
    ))
    {
        startPainting(
            m_hoveredCellX,
            m_hoveredCellY,
            m_hoveredWall
        );
    }

    if (m_isPainting &&
        ImGui::IsMouseDown(
            ImGuiMouseButton_Left
        ))
    {
        updatePainting(
            m_hoveredCellX,
            m_hoveredCellY,
            m_hoverLocalX,
            m_hoverLocalY
        );
    }

    if (ImGui::IsMouseReleased(
        ImGuiMouseButton_Left
    ))
    {
        stopPainting();
    }
}

void WorldViewWindow::drawNotePopup()
{
    if (m_requestNotePopup)
    {
        ImGui::OpenPopup(
            "Edit Note"
        );

        m_requestNotePopup = false;
    }

    if (!ImGui::BeginPopupModal(
        "Edit Note",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize
    ))
    {
        return;
    }

    ImGui::TextUnformatted(
        "Note text:"
    );

    ImGui::InputTextMultiline(
        "##NoteText",
        m_noteTextBuffer,
        sizeof(m_noteTextBuffer),
        ImVec2(400.0f, 160.0f)
    );

    ImGui::Spacing();

    const bool hasText =
        m_noteTextBuffer[0] != '\0';

    const Cell* existingNoteCell =
        m_chunkManager.findCell(
            m_noteCellX,
            m_noteCellY,
            m_noteLayer
        );

    const bool hasExistingNote =
        existingNoteCell != nullptr &&
        existingNoteCell->hasMiscText();
    if (!hasText)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Save"))
    {
        const int originalLayer =
            m_chunkManager.activeLayer();

        m_chunkManager.setActiveLayer(
            m_noteLayer
        );

        Cell& cell =
            m_chunkManager.cell(
                m_noteCellX,
                m_noteCellY
            );

        cell.setMiscText(
            m_noteTextBuffer
        );

        m_chunkManager.setActiveLayer(
            originalLayer
        );

        m_hasUnsavedChanges = true;
        m_blockMapInputFrames = 2;

        ImGui::CloseCurrentPopup();
    }

    if (!hasText)
    {
        ImGui::EndDisabled();
    }

    if (hasExistingNote)
    {
        ImGui::SameLine();

        if (ImGui::Button("Delete"))
        {
            const int originalLayer =
                m_chunkManager.activeLayer();

            m_chunkManager.setActiveLayer(
                m_noteLayer
            );

            m_chunkManager
                .cell(
                    m_noteCellX,
                    m_noteCellY
                )
                .removeMiscText();

            m_chunkManager.setActiveLayer(
                originalLayer
            );

            m_hasUnsavedChanges = true;
            m_blockMapInputFrames = 2;

            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        m_blockMapInputFrames = 2;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}
void WorldViewWindow::drawNoteTooltip()
{
    if (!m_hasHoveredCell ||
        ImGui::IsPopupOpen(
            "",
            ImGuiPopupFlags_AnyPopupId
        ))
    {
        return;
    }

    const Cell* cell =
        m_chunkManager.findCell(
            m_hoveredCellX,
            m_hoveredCellY,
            m_chunkManager.activeLayer()
        );

    if (cell == nullptr ||
        !cell->hasMiscText())
    {
        return;
    }

    ImGui::BeginTooltip();

    ImGui::PushTextWrapPos(
        ImGui::GetFontSize() * 25.0f
    );

    ImGui::TextUnformatted(
        cell->miscText().c_str()
    );

    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void WorldViewWindow::handleMisc(
    const ImVec2& canvasPosition,
    const ImVec2& canvasSize
)
{
    ImVec2 mouseCanvasPosition;

    if (!isMouseInsideCanvas(
        canvasPosition,
        canvasSize,
        mouseCanvasPosition
    ))
    {
        return;
    }
    if (ImGui::IsMouseClicked(
        ImGuiMouseButton_Right
    ))
    {
        openHoveredColorMenu();
        return;
    }
    if (!ImGui::IsMouseClicked(
        ImGuiMouseButton_Left
    ))
    {
        return;
    }

    if (m_activeMiscId.empty())
    {
        return;
    }

    if (m_activeMiscId == "note")
    {
        m_noteCellX =
            m_hoveredCellX;

        m_noteCellY =
            m_hoveredCellY;

        m_noteLayer =
            m_chunkManager.activeLayer();

        const Cell* noteCell =
            m_chunkManager.findCell(
                m_noteCellX,
                m_noteCellY,
                m_noteLayer
            );

        const std::string existingText =
            noteCell != nullptr
            ? noteCell->miscText()
            : std::string{};

        std::snprintf(
            m_noteTextBuffer,
            sizeof(m_noteTextBuffer),
            "%s",
            existingText.c_str()
        );

        m_requestNotePopup = true;

        return;
    }

    const int originalLayer =
        m_chunkManager.activeLayer();

    Cell& cell =
        m_chunkManager.cell(
            m_hoveredCellX,
            m_hoveredCellY
        );

    const bool sameMisc =
        cell.hasMisc() &&
        cell.miscId() ==
        m_activeMiscId &&
        cell.miscColorId() ==
        m_activeMiscColorId;

    std::string pairedMiscId =
        m_activeMiscId;

    int pairedLayerOffset = 0;

    const std::size_t upPosition =
        pairedMiscId.find("_up");

    const std::size_t downPosition =
        pairedMiscId.find("_down");

    if (upPosition !=
        std::string::npos)
    {
        pairedMiscId.replace(
            upPosition,
            3,
            "_down"
        );

        pairedLayerOffset = 1;
    }
    else if (downPosition !=
        std::string::npos)
    {
        pairedMiscId.replace(
            downPosition,
            5,
            "_up"
        );

        pairedLayerOffset = -1;
    }

    if (sameMisc)
    {
        cell.removeMisc();

        if (pairedLayerOffset != 0)
        {
            const int pairedLayer =
                originalLayer +
                pairedLayerOffset;

            const Cell* pairedCell =
                m_chunkManager.findCell(
                    m_hoveredCellX,
                    m_hoveredCellY,
                    pairedLayer
                );

            if (pairedCell != nullptr &&
                pairedCell->hasMisc() &&
                pairedCell->miscId() ==
                pairedMiscId)
            {
                m_chunkManager.setActiveLayer(
                    pairedLayer
                );

                m_chunkManager
                    .cell(
                        m_hoveredCellX,
                        m_hoveredCellY
                    )
                    .removeMisc();

                m_chunkManager.setActiveLayer(
                    originalLayer
                );
            }
        }
    }
    else
    {
        cell.setMisc(
            m_activeMiscId,
            m_activeMiscColorId
        );

        if (pairedLayerOffset != 0)
        {
            m_chunkManager.setActiveLayer(
                originalLayer +
                pairedLayerOffset
            );

            m_chunkManager
                .cell(
                    m_hoveredCellX,
                    m_hoveredCellY
                )
                .setMisc(
                    pairedMiscId,
                    m_activeMiscColorId
                );

            m_chunkManager.setActiveLayer(
                originalLayer
            );
        }
    }
    m_hasUnsavedChanges = true;
}

void WorldViewWindow::openHoveredEdgeColorMenu()
{
    if (!m_openEdgeColorMenu ||
        !m_hasHoveredCell)
    {
        return;
    }

    EdgeDirection direction;

    switch (m_hoveredWall)
    {
    case HoveredWall::North:
        direction = EdgeDirection::North;
        break;

    case HoveredWall::East:
        direction = EdgeDirection::East;
        break;

    case HoveredWall::South:
        direction = EdgeDirection::South;
        break;

    case HoveredWall::West:
        direction = EdgeDirection::West;
        break;

    case HoveredWall::None:
        return;
    }

    const Cell* cell =
        m_chunkManager.findCell(
            m_hoveredCellX,
            m_hoveredCellY,
            m_chunkManager.activeLayer()
        );

    if (cell == nullptr ||
        !cell->hasEdge(direction))
    {
        return;
    }

    const std::string edgeId =
        cell->edgeId(direction);

    const std::string assignedColorId =
        cell->edgeColorId(direction);

    const int cellX =
        m_hoveredCellX;

    const int cellY =
        m_hoveredCellY;

    m_openEdgeColorMenu(
        edgeId,
        assignedColorId,
        [this,
        cellX,
        cellY,
        direction,
        edgeId](
            const std::string& colorId
            )
        {
            setEdge(
                cellX,
                cellY,
                direction,
                edgeId,
                colorId
            );
            m_hasUnsavedChanges = true;
        }
    );
}

void WorldViewWindow::openHoveredMiscColorMenu()
{
    if (!m_openMiscColorMenu ||
        !m_hasHoveredCell)
    {
        return;
    }

    const Cell* cell =
        m_chunkManager.findCell(
            m_hoveredCellX,
            m_hoveredCellY,
            m_chunkManager.activeLayer()
        );

    if (cell == nullptr ||
        !cell->hasMisc())
    {
        return;
    }

    const std::string miscId =
        cell->miscId();

    const std::string assignedColorId =
        cell->miscColorId();

    const int cellX =
        m_hoveredCellX;

    const int cellY =
        m_hoveredCellY;

    m_openMiscColorMenu(
        miscId,
        assignedColorId,
        [this,
        cellX,
        cellY,
        miscId](
            const std::string& colorId
            )
        {
            m_chunkManager
                .cell(cellX, cellY)
                .setMisc(
                    miscId,
                    colorId
                );
            m_hasUnsavedChanges = true;
        }
    );
}

void WorldViewWindow::openHoveredColorMenu()
{
    if (!m_hasHoveredCell)
    {
        return;
    }

    const Cell* cell =
        m_chunkManager.findCell(
            m_hoveredCellX,
            m_hoveredCellY,
            m_chunkManager.activeLayer()
        );

    if (cell == nullptr)
    {
        return;
    }

    EdgeDirection direction =
        EdgeDirection::North;

    bool hasHoveredDirection =
        true;

    switch (m_hoveredWall)
    {
    case HoveredWall::North:
        direction = EdgeDirection::North;
        break;

    case HoveredWall::East:
        direction = EdgeDirection::East;
        break;

    case HoveredWall::South:
        direction = EdgeDirection::South;
        break;

    case HoveredWall::West:
        direction = EdgeDirection::West;
        break;

    case HoveredWall::None:
        hasHoveredDirection = false;
        break;
    }

    if (hasHoveredDirection &&
        cell->hasEdge(direction))
    {
        openHoveredEdgeColorMenu();
        return;
    }

    if (cell->hasMisc())
    {
        openHoveredMiscColorMenu();
    }
}

void WorldViewWindow::handleEraser(
    const ImVec2& canvasPosition,
    const ImVec2& canvasSize
)
{
    ImVec2 mouseCanvasPosition;

    const bool mouseInsideCanvas =
        isMouseInsideCanvas(
            canvasPosition,
            canvasSize,
            mouseCanvasPosition
        );

    if (!mouseInsideCanvas)
    {
        return;
    }

    const float mouseWheel =
        ImGui::GetIO().MouseWheel;

    static float lastMouseWheel = 0.0f;

    const int wheelSteps =
        std::max(
            1,
            static_cast<int>(
                std::round(
                    std::abs(mouseWheel)
                )
                )
        );

    if (mouseWheel > 0.0f)
    {
        m_eraserSize =
            std::min(
                m_eraserSize + wheelSteps,
                8
            );
    }
    else if (mouseWheel < 0.0f)
    {
        m_eraserSize =
            std::max(
                m_eraserSize - wheelSteps,
                1
            );
    }
    handleEraserAutoPan(
        canvasPosition,
        canvasSize
    );

    if (ImGui::IsMouseDown(
        ImGuiMouseButton_Left
    ))
    {
        eraseArea();
    }
}

void WorldViewWindow::handlePan()
{
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_followPlayer = false;

        ImVec2 delta = ImGui::GetIO().MouseDelta;

        m_cameraX -= delta.x;
        m_cameraY -= delta.y;
    }
}