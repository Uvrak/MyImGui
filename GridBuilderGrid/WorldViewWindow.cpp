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

const ChunkManager& WorldViewWindow::map() const
{
    return m_chunkManager;
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
    m_toolSettings.m_activeTool =
        activeTool;

    m_toolSettings.m_activeEdgeId =
        activeEdgeId;

    m_toolSettings.m_activeColorId =
        activeColorId;

    m_toolSettings.m_activeMiscId =
        activeMiscId;

    m_toolSettings.m_activeMiscColorId =
        activeMiscColorId;

    m_toolSettings.m_paintMisc =
        paintMisc;

    m_toolSettings.m_edgeTexture =
        edgeTexture;

    m_toolSettings.m_miscTexture =
        miscTexture;

    m_toolSettings.m_openEdgeColorMenu =
        openEdgeColorMenu;

    m_toolSettings.m_openMiscColorMenu =
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
        WorldView::stopPainting(m_painter);
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
        WorldView::stopPainting(m_painter);
    }

    ImGui::Separator();

    ImVec2 canvasPosition = ImGui::GetCursorScreenPos();
    canvasPosition.x += m_viewport.m_rulerWidth;
    canvasPosition.y += m_viewport.m_rulerHeight;

    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.x -= m_viewport.m_rulerWidth;
    canvasSize.y -= m_viewport.m_rulerHeight;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    if (!inputBlocked &&
        !toolbarBlocksMapInput &&
        m_blockMapInputFrames == 0 &&
        (
            activeTool == EditorTool::Scroll ||
            ImGui::GetIO().KeyCtrl
            ))
    {
        WorldView::handleZoom(
            m_viewport,
            m_toolSettings,
            canvasPosition,
            canvasSize
        );
    }

    if (m_viewport.m_followPlayer &&
        m_playerMarker.visible)
    {
        WorldView::centerOnPlayer(m_viewport, m_playerMarker,
            canvasSize
        );
    }

    WorldView::updateGridView(m_viewport);

    WorldView::updateHover(
        m_viewport,
        m_hover,
        m_painter,
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

    WorldView::updateLongTickStep(
        m_viewport
    );

    if (m_hover.m_hasHoveredCell)
    {
        ImGui::SetMouseCursor(
            ImGuiMouseCursor_Arrow
        );
    }

    WorldView::updateLongTickStep(m_viewport);

    const ImVec2 mapCanvasEnd(
        canvasPosition.x + canvasSize.x,
        canvasPosition.y + canvasSize.y
    );

    drawList->AddRectFilled(
        canvasPosition,
        mapCanvasEnd,
        m_style.m_backgroundColor
    );

    drawList->PushClipRect(
        canvasPosition,
        mapCanvasEnd,
        true
    );

    WorldView::drawGrid(m_viewport, m_style, m_chunkSize,
        drawList,
        canvasPosition,
        canvasSize
    );

    WorldView::drawWalls(m_viewport, m_hover, m_painter, m_toolSettings, m_chunkManager, m_showLowerLayer,
        drawList,
        canvasPosition,
        canvasSize
    );

    WorldView::drawMisc(m_viewport, m_toolSettings, m_chunkManager,
        drawList,
        canvasPosition,
        canvasSize
    );

    WorldView::drawPlayerMarker(m_viewport, m_playerMarker,
        drawList,
        canvasPosition
    );

    WorldView::drawNoteTooltip(m_hover, m_chunkManager);

    drawList->PopClipRect();

    WorldView::drawRulers(m_viewport, m_style,
        drawList,
        canvasPosition,
        canvasSize
    );

    if (showCoordinates)
    {
        WorldView::drawCoordinates(m_hover, m_chunkSize, drawList, canvasPosition);
    }

    if (!inputBlocked &&
        !toolbarBlocksMapInput &&
        m_blockMapInputFrames == 0 &&
        activeTool == EditorTool::Pencil)
    {
        if (m_toolSettings.m_paintMisc)
        {
            WorldView::handleMisc(m_hover, m_cellInteraction, m_toolSettings, m_chunkManager, m_hasUnsavedChanges,
                canvasPosition,
                canvasSize
            );
        }
        else
        {
            WorldView::handlePencil(m_viewport, m_hover, m_painter, m_toolSettings, m_chunkManager, m_hasUnsavedChanges,
                canvasPosition,
                canvasSize
            );
        }
    }

    WorldView::drawNotePopup(m_cellInteraction, m_chunkManager, m_hasUnsavedChanges, m_blockMapInputFrames);

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
            if (m_toolSettings.m_paintMisc)
            {
                WorldView::drawMiscPreview(m_viewport, m_hover, m_toolSettings, m_chunkManager,
                    drawList
                );
            }
            else
            {
                WorldView::drawHover(m_viewport, m_hover, m_painter, m_toolSettings,
                    drawList,
                    canvasPosition,
                    canvasSize
                );
            }
        }
        else
        {
            WorldView::drawEraserPreview(m_viewport, m_hover, m_eraser, m_toolSettings,
                drawList
            );
        }

        drawList->PopClipRect();
    }
    ImGui::Dummy(
        ImVec2(
            canvasSize.x +
            m_viewport.m_rulerWidth,
            canvasSize.y +
            m_viewport.m_rulerHeight
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
    WorldView::stopPainting(m_painter);

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
    WorldView::stopPainting(m_painter);

    m_chunkManager.clear();

    m_viewport.m_cameraX = 0.0f;
    m_viewport.m_cameraY = 0.0f;

    m_hover.m_hasHoveredCell = false;
    m_hover.m_hasSelectedCell = false;
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
    return WorldView::edgeTextureSize(m_viewport);
}

bool WorldViewWindow::isMouseOverCanvas() const
{
    return m_hover.m_hasHoveredCell;
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
        WorldView::handleEraser(m_viewport, m_hover, m_eraser, m_chunkManager, m_hasUnsavedChanges,
            canvasPosition,
            canvasSize
        );
        return;

    case EditorTool::Scroll:
    {
        ImVec2 mouseCanvasPosition;

        if (WorldView::isMouseInsideCanvas(
            canvasPosition,
            canvasSize,
            mouseCanvasPosition
        ))
        {
            WorldView::handlePan(m_viewport);
        }

        return;
    }
    }
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

     WorldView::stopPainting(m_painter);

     m_chunkManager.setActiveLayer(
         layer
     );

     m_hover.m_hasHoveredCell = false;
     m_hover.m_hasSelectedCell = false;
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
         &m_viewport.m_followPlayer
     );
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

void WorldViewWindow::setPlayerMarker(
     const MapPlayerMarker& marker
 )
{
     m_playerMarker =
         marker;
 }
