#include "WorldViewWindow.h"

#include "Cell.h"
#include "Chunk.h"

#include "imgui.h"

#include <string>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <filesystem>

#include "MapSerializer.h"

WorldViewWindow::WorldViewWindow(int chunkSize)
    : m_chunkManager(chunkSize), m_chunkSize(chunkSize)
{
    loadSettings();
}

void WorldViewWindow::setGroundLayer(GroundLayer layer, const std::string& title)
{
    layer.validate();
    m_ground = std::move(layer);
    m_groundOnly = true;
    m_style.m_chunkRowOffset = 0;
    m_mapName = title;
    m_viewport.m_followPlayer = false;
}

void WorldViewWindow::focusGroundCell(int x, int y)
{
    m_viewport.m_fittedChunkSize = 0;
    m_viewport.m_cameraX = x * m_viewport.m_cellSize;
    m_viewport.m_cameraY = y * m_viewport.m_cellSize;
}

const ChunkManager& WorldViewWindow::map() const
{
    return m_chunkManager;
}

GroundViewState WorldViewWindow::groundView() const {
    return {(m_viewport.m_cameraX+m_canvasSize.x*.5f)/m_viewport.m_cellSize,
        (m_viewport.m_cameraY+m_canvasSize.y*.5f)/m_viewport.m_cellSize,
        m_canvasSize.y/m_viewport.m_cellSize};
}
void WorldViewWindow::setGroundView(GroundViewState view) {
    if(!std::isfinite(view.centerX) || !std::isfinite(view.centerY) ||
        !std::isfinite(view.visibleHeight) || view.visibleHeight<=0)return;
    m_pendingView=view;m_hasPendingView=true;
}
void WorldViewWindow::setGroundCell(int x,int y,GroundMaterial material) {
    if(!m_ground.at(x,y))throw std::out_of_range("Ground cell");
    size_t index=0;
    for(;index<m_ground.materials.size();++index)
        if(m_ground.materials[index].texture==material.texture && m_ground.materials[index].color==material.color)break;
    if(index==m_ground.materials.size()) {
        if(index>=65536)throw std::length_error("Too many ground materials");
        m_ground.materials.push_back(material);
    }
    m_ground.cells[size_t(y)*m_ground.width+x]=static_cast<std::uint16_t>(index);
}
void WorldViewWindow::setGroundBorder(int x,int y,std::uint8_t border) {
    if(!m_ground.at(x,y) || border>15)throw std::out_of_range("Ground border");
    if(m_ground.borders.empty())m_ground.borders.resize(m_ground.cells.size());
    m_ground.borders[size_t(y)*m_ground.width+x]=border;
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

    if (m_groundOnly) {
        const auto* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos({viewport->WorkPos.x + 10, viewport->WorkPos.y + 30}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({(std::min)(920.f, (std::max)(580.f, viewport->WorkSize.x - 320.f)),
            (std::min)(680.f, viewport->WorkSize.y - 50.f)}, ImGuiCond_FirstUseEver);
    }
    const std::string windowTitle = m_mapName + "###Map Editor";
    const bool windowVisible = ImGui::Begin(windowTitle.c_str(), isOpen);

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

    if (m_groundOnly) ImGui::TextUnformatted(m_editorToolsEnabled
        ? "Pan: Ziehen/Mausrad | Pencil: Zeichnen | Ctrl+Mausrad: Zoom"
        : "Ziehen: Verschieben | Mausrad: Zoom | X rechts, Y unten");
    if (!m_groundOnly || m_editorToolsEnabled) drawLayerSelector();

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

    if (canvasSize.x <= 0 || canvasSize.y <= 0) { ImGui::End(); return; }
    m_canvasSize=canvasSize;
    if(m_hasPendingView) {
        m_viewport.m_fittedChunkSize=0;
        m_viewport.m_cellSize=(std::clamp)(canvasSize.y/m_pendingView.visibleHeight,2.f,256.f);
        m_viewport.m_cameraX=m_pendingView.centerX*m_viewport.m_cellSize-canvasSize.x*.5f;
        m_viewport.m_cameraY=m_pendingView.centerY*m_viewport.m_cellSize-canvasSize.y*.5f;
        m_hasPendingView=false;
    }
    const auto navigationBefore=groundView();
    WorldView::updateChunkFit(m_viewport, canvasSize);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    bool dropPreview=false;ImVec2 dropCell{};
    {
        // Every grid canvas owns its drag, including the ordinary editor without ground tiles.
        // Otherwise ImGui treats a drag on the drawn raster as a window-move gesture.
        const auto cursor = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(canvasPosition);
        ImGui::InvisibleButton("##GroundCanvas", canvasSize);
        if(m_groundOnly && m_groundDrop && ImGui::BeginDragDropTarget()) {
            const auto mouse=ImGui::GetMousePos();
            const int x=int(std::floor((mouse.x-canvasPosition.x+m_viewport.m_cameraX)/m_viewport.m_cellSize));
            const int y=int(std::floor((mouse.y-canvasPosition.y+m_viewport.m_cameraY)/m_viewport.m_cellSize));
            if(m_ground.at(x,y)) {
                if(const auto* payload=ImGui::AcceptDragDropPayload(GroundTilePayloadType,ImGuiDragDropFlags_AcceptBeforeDelivery)) {
                    if(payload->DataSize==sizeof(std::uint32_t)) {
                        dropPreview=true;dropCell={float(x),float(y)};
                        if(payload->IsDelivery())m_groundDrop(x,y,*static_cast<const std::uint32_t*>(payload->Data));
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
        // IsWindowHovered() normally rejects any active item, including this canvas.
        // Keep our captured drag alive; other windows/items still block new input.
        const bool canvasActive = ImGui::IsItemActive();
        inputBlocked = inputBlocked || (!canvasActive &&
            !ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows));
        if (inputBlocked) WorldView::stopPainting(m_painter);
        ImGui::SetCursorScreenPos(cursor);
    }


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

    if ((m_viewport.m_followPlayer || m_centerOnNextMarker) &&
        m_playerMarker.visible)
    {
        WorldView::centerOnPlayer(m_viewport, m_playerMarker,
            canvasSize
        );
        m_centerOnNextMarker = false;
    }

    WorldView::updateGridView(m_viewport);

    WorldView::updateHover(
        m_viewport,
        m_hover,
        m_painter,
        canvasPosition,
        canvasSize
    );

    const bool fitClickedChunk = !inputBlocked && !toolbarBlocksMapInput &&
        m_blockMapInputFrames == 0 && m_hover.m_hasHoveredCell &&
        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
        (!m_groundOnly || m_ground.at(m_hover.m_hoveredCellX, m_hover.m_hoveredCellY));
    if (fitClickedChunk) {
        WorldView::stopPainting(m_painter);
        WorldView::fitChunk(m_viewport, m_hover.m_hoveredCellX, m_hover.m_hoveredCellY,
            m_chunkSize, canvasSize, m_style.m_chunkRowOffset);
        m_centerOnNextMarker = false;
        WorldView::updateHover(m_viewport, m_hover, m_painter, canvasPosition, canvasSize);
    }
    if (!inputBlocked && !fitClickedChunk &&
        !toolbarBlocksMapInput &&
        m_blockMapInputFrames == 0)
    {
        handleInput(
            canvasPosition,
            canvasSize,
            activeTool
        );
    }

    WorldView::updateGridView(m_viewport);
    WorldView::updateLongTickStep(
        m_viewport
    );
    const auto navigationAfter=groundView();
    if(m_groundNavigation && (navigationAfter.centerX!=navigationBefore.centerX ||
        navigationAfter.centerY!=navigationBefore.centerY || navigationAfter.visibleHeight!=navigationBefore.visibleHeight))
        m_groundNavigation(navigationAfter);

    if (m_hover.m_hasHoveredCell)
    {
        ImGui::SetMouseCursor(
            activeTool == EditorTool::Scroll ? ImGuiMouseCursor_Hand : ImGuiMouseCursor_Arrow
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

    if (m_groundOnly) {
        // Visit only visible cells, even for multi-million-cell layers.
        const int firstX = (std::max)(0, m_viewport.m_gridView.firstVisibleCellX);
        const int firstY = (std::max)(0, m_viewport.m_gridView.firstVisibleCellY);
        const int endX = (std::min)(m_ground.width, m_viewport.m_gridView.firstVisibleCellX + int(canvasSize.x / m_viewport.m_cellSize) + 2);
        const int endY = (std::min)(m_ground.height, m_viewport.m_gridView.firstVisibleCellY + int(canvasSize.y / m_viewport.m_cellSize) + 2);
        for (int y = firstY; y < endY; ++y) for (int x = firstX; x < endX; ++x) {
            const auto& material = *m_ground.at(x, y);
            ImVec2 p(canvasPosition.x + x * m_viewport.m_cellSize - m_viewport.m_cameraX,
                     canvasPosition.y + y * m_viewport.m_cellSize - m_viewport.m_cameraY);
            ImVec2 q(p.x + m_viewport.m_cellSize, p.y + m_viewport.m_cellSize);
            if (material.texture) drawList->AddImage(material.texture, p, q, {0,0}, {1,1}, material.color);
            else drawList->AddRectFilled(p, q, material.color);
            if (!m_ground.borders.empty() && m_ground.borderMaterial.texture) {
                const auto borders = m_ground.borders[size_t(y) * m_ground.width + x];
                const float thickness = m_viewport.m_cellSize * 0.18f;
                auto strip = [&](ImVec2 a, ImVec2 b, ImVec2 c, ImVec2 d) {
                    drawList->AddImageQuad(m_ground.borderMaterial.texture, a, b, c, d,
                        {0,0}, {0,1}, {1,1}, {1,0}, m_ground.borderMaterial.color);
                };
                if (borders & 1) strip(p, {q.x,p.y}, {q.x,p.y+thickness}, {p.x,p.y+thickness});
                if (borders & 2) strip({q.x,p.y}, q, {q.x-thickness,q.y}, {q.x-thickness,p.y});
                if (borders & 4) strip(q, {p.x,q.y}, {p.x,q.y-thickness}, {q.x,q.y-thickness});
                if (borders & 8) strip({p.x,q.y}, p, {p.x+thickness,p.y}, {p.x+thickness,q.y});
            }
        }
    }
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

    WorldView::drawChunkCoordinates(m_viewport, m_style, m_chunkSize, drawList,
        canvasPosition, canvasSize, m_groundOnly ? m_ground.width : 0, m_groundOnly ? m_ground.height : 0);
    WorldView::drawNoteTooltip(m_hover, m_chunkManager);

    drawList->PopClipRect();
    if(dropPreview) {
        const ImVec2 p{canvasPosition.x+dropCell.x*m_viewport.m_cellSize-m_viewport.m_cameraX,
            canvasPosition.y+dropCell.y*m_viewport.m_cellSize-m_viewport.m_cameraY};
        drawList->PushClipRect(canvasPosition,mapCanvasEnd,true);
        drawList->AddRect(p,{p.x+m_viewport.m_cellSize,p.y+m_viewport.m_cellSize},IM_COL32(255,220,80,255),0.f,3.f);
        drawList->PopClipRect();
    }

    WorldView::drawRulers(m_viewport, m_style,
        drawList,
        canvasPosition,
        canvasSize
    );

    if (showCoordinates && !m_groundOnly)
    {
        WorldView::drawCoordinates(m_hover, m_chunkSize, drawList, canvasPosition);
    }

    if (!inputBlocked &&
        !toolbarBlocksMapInput &&
        m_blockMapInputFrames == 0 &&
        activeTool == EditorTool::Pencil && !fitClickedChunk)
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
        m_mapName = std::filesystem::path(filename).stem().string();
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
        m_mapName = std::filesystem::path(filename).stem().string();
        m_centerOnNextMarker = true;
        m_hasUnsavedChanges = false;
    }

    return loaded;
}

bool WorldViewWindow::hasUnsavedChanges() const
{
    return m_hasUnsavedChanges;
}

void WorldViewWindow::setMapName(const std::string& name)
{
    if (!name.empty()) m_mapName = name;
}

void WorldViewWindow::newMap()
{
    WorldView::stopPainting(m_painter);

    m_chunkManager.clear();
    m_mapName = "Map Editor";
    m_centerOnNextMarker = true;

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
