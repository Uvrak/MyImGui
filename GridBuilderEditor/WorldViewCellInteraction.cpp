#include "WorldViewCellInteraction.h"
#include "WorldViewHitTest.h"
#include "WorldViewTypes.h"
#include "WorldViewViewport.h"
#include "WorldViewWallPainter.h"

#include "Cell.h"
#include "Chunk.h"
#include <cstdio>

namespace WorldView
{
void handlePencil(Viewport& viewport, Hover& hover, WallPainting& painter, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty,
    const ImVec2& canvasPosition,
    const ImVec2& canvasSize
)
{
    ImVec2 mouseCanvasPosition;

    const bool mouseInsideCanvas =
        WorldView::isMouseInsideCanvas(
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
            WorldView::stopPainting(painter);
        }

        return;
    }
    if (ImGui::IsMouseClicked(
        ImGuiMouseButton_Right
    ))
    {
        WorldView::openHoveredColorMenu(hover, toolSettings, map, dirty);
        return;
    }

    if (ImGui::IsMouseClicked(
        ImGuiMouseButton_Left
    ))
    {
        WorldView::startPainting(painter, toolSettings, map, dirty,
            hover.m_hoveredCellX,
            hover.m_hoveredCellY,
            hover.m_hoveredWall
        );
    }

    if (painter.m_isPainting &&
        ImGui::IsMouseDown(
            ImGuiMouseButton_Left
        ))
    {
        WorldView::updatePainting(viewport, hover, painter, toolSettings, map, dirty,
            hover.m_hoveredCellX,
            hover.m_hoveredCellY,
            hover.m_hoverLocalX,
            hover.m_hoverLocalY
        );
    }

    if (ImGui::IsMouseReleased(
        ImGuiMouseButton_Left
    ))
    {
        WorldView::stopPainting(painter);
    }
}

void drawNotePopup(CellInteraction& cellInteraction, ChunkManager& map, bool& dirty, int& blockFrames)
{
    if (cellInteraction.m_requestNotePopup)
    {
        ImGui::OpenPopup(
            "Edit Note"
        );

        cellInteraction.m_requestNotePopup = false;
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
        cellInteraction.m_noteTextBuffer,
        sizeof(cellInteraction.m_noteTextBuffer),
        ImVec2(400.0f, 160.0f)
    );

    ImGui::Spacing();

    const bool hasText =
        cellInteraction.m_noteTextBuffer[0] != '\0';

    const Cell* existingNoteCell =
        map.findCell(
            cellInteraction.m_noteCellX,
            cellInteraction.m_noteCellY,
            cellInteraction.m_noteLayer
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
            map.activeLayer();

        map.setActiveLayer(
            cellInteraction.m_noteLayer
        );

        Cell& cell =
            map.cell(
                cellInteraction.m_noteCellX,
                cellInteraction.m_noteCellY
            );

        cell.setMiscText(
            cellInteraction.m_noteTextBuffer
        );

        map.setActiveLayer(
            originalLayer
        );

        dirty = true;
        blockFrames = 2;

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
                map.activeLayer();

            map.setActiveLayer(
                cellInteraction.m_noteLayer
            );

            map
                .cell(
                    cellInteraction.m_noteCellX,
                    cellInteraction.m_noteCellY
                )
                .removeMiscText();

            map.setActiveLayer(
                originalLayer
            );

            dirty = true;
            blockFrames = 2;

            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        blockFrames = 2;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void drawNoteTooltip(const Hover& hover, const ChunkManager& map)
{
    if (!hover.m_hasHoveredCell ||
        ImGui::IsPopupOpen(
            "",
            ImGuiPopupFlags_AnyPopupId
        ))
    {
        return;
    }

    const Cell* cell =
        map.findCell(
            hover.m_hoveredCellX,
            hover.m_hoveredCellY,
            map.activeLayer()
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

void handleMisc(Hover& hover, CellInteraction& cellInteraction, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty,
    const ImVec2& canvasPosition,
    const ImVec2& canvasSize
)
{
    ImVec2 mouseCanvasPosition;

    if (!WorldView::isMouseInsideCanvas(
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
        WorldView::openHoveredColorMenu(hover, toolSettings, map, dirty);
        return;
    }
    if (!ImGui::IsMouseClicked(
        ImGuiMouseButton_Left
    ))
    {
        return;
    }

    if (toolSettings.m_activeMiscId.empty())
    {
        return;
    }

    if (toolSettings.m_activeMiscId == "note")
    {
        cellInteraction.m_noteCellX =
            hover.m_hoveredCellX;

        cellInteraction.m_noteCellY =
            hover.m_hoveredCellY;

        cellInteraction.m_noteLayer =
            map.activeLayer();

        const Cell* noteCell =
            map.findCell(
                cellInteraction.m_noteCellX,
                cellInteraction.m_noteCellY,
                cellInteraction.m_noteLayer
            );

        const std::string existingText =
            noteCell != nullptr
            ? noteCell->miscText()
            : std::string{};

        std::snprintf(
            cellInteraction.m_noteTextBuffer,
            sizeof(cellInteraction.m_noteTextBuffer),
            "%s",
            existingText.c_str()
        );

        cellInteraction.m_requestNotePopup = true;

        return;
    }

    const int originalLayer =
        map.activeLayer();

    Cell& cell =
        map.cell(
            hover.m_hoveredCellX,
            hover.m_hoveredCellY
        );

    const bool sameMisc =
        cell.hasMisc() &&
        cell.miscId() ==
        toolSettings.m_activeMiscId &&
        cell.miscColorId() ==
        toolSettings.m_activeMiscColorId;

    std::string pairedMiscId =
        toolSettings.m_activeMiscId;

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
                map.findCell(
                    hover.m_hoveredCellX,
                    hover.m_hoveredCellY,
                    pairedLayer
                );

            if (pairedCell != nullptr &&
                pairedCell->hasMisc() &&
                pairedCell->miscId() ==
                pairedMiscId)
            {
                map.setActiveLayer(
                    pairedLayer
                );

                map
                    .cell(
                        hover.m_hoveredCellX,
                        hover.m_hoveredCellY
                    )
                    .removeMisc();

                map.setActiveLayer(
                    originalLayer
                );
            }
        }
    }
    else
    {
        cell.setMisc(
            toolSettings.m_activeMiscId,
            toolSettings.m_activeMiscColorId
        );

        if (pairedLayerOffset != 0)
        {
            map.setActiveLayer(
                originalLayer +
                pairedLayerOffset
            );

            map
                .cell(
                    hover.m_hoveredCellX,
                    hover.m_hoveredCellY
                )
                .setMisc(
                    pairedMiscId,
                    toolSettings.m_activeMiscColorId
                );

            map.setActiveLayer(
                originalLayer
            );
        }
    }
    dirty = true;
}

void openHoveredEdgeColorMenu(Hover& hover, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty)
{
    if (!toolSettings.m_openEdgeColorMenu ||
        !hover.m_hasHoveredCell)
    {
        return;
    }

    EdgeDirection direction;

    switch (hover.m_hoveredWall)
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
        map.findCell(
            hover.m_hoveredCellX,
            hover.m_hoveredCellY,
            map.activeLayer()
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
        hover.m_hoveredCellX;

    const int cellY =
        hover.m_hoveredCellY;

    toolSettings.m_openEdgeColorMenu(
        edgeId,
        assignedColorId,
        [&map, &dirty,
        cellX,
        cellY,
        direction,
        edgeId](
            const std::string& colorId
            )
        {
            WorldView::setEdge(map,
                cellX,
                cellY,
                direction,
                edgeId,
                colorId
            );
            dirty = true;
        }
    );
}

void openHoveredMiscColorMenu(Hover& hover, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty)
{
    if (!toolSettings.m_openMiscColorMenu ||
        !hover.m_hasHoveredCell)
    {
        return;
    }

    const Cell* cell =
        map.findCell(
            hover.m_hoveredCellX,
            hover.m_hoveredCellY,
            map.activeLayer()
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
        hover.m_hoveredCellX;

    const int cellY =
        hover.m_hoveredCellY;

    toolSettings.m_openMiscColorMenu(
        miscId,
        assignedColorId,
        [&map, &dirty,
        cellX,
        cellY,
        miscId](
            const std::string& colorId
            )
        {
            map
                .cell(cellX, cellY)
                .setMisc(
                    miscId,
                    colorId
                );
            dirty = true;
        }
    );
}

void openHoveredColorMenu(Hover& hover, const ToolSettings& toolSettings, ChunkManager& map, bool& dirty)
{
    if (!hover.m_hasHoveredCell)
    {
        return;
    }

    const Cell* cell =
        map.findCell(
            hover.m_hoveredCellX,
            hover.m_hoveredCellY,
            map.activeLayer()
        );

    if (cell == nullptr)
    {
        return;
    }

    EdgeDirection direction =
        EdgeDirection::North;

    bool hasHoveredDirection =
        true;

    switch (hover.m_hoveredWall)
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
        WorldView::openHoveredEdgeColorMenu(hover, toolSettings, map, dirty);
        return;
    }

    if (cell->hasMisc())
    {
        WorldView::openHoveredMiscColorMenu(hover, toolSettings, map, dirty);
    }
}
}
