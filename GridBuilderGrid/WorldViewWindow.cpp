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

void WorldViewWindow::setWallTile(int x,int y,int side,const std::string& id,ImTextureID texture) {
    if(!m_ground.at(x,y) || side<0 || side>3 || !texture)throw std::invalid_argument("Invalid wall tile");
    m_wallTextures[id]=texture;
    WorldView::setEdge(m_chunkManager,x,y,static_cast<EdgeDirection>(side),id,"");
    m_hasUnsavedChanges=true;
}
void WorldViewWindow::removeWallTile(int x,int y,int side) {
    if(!m_ground.at(x,y) || side<0 || side>3)return;
    WorldView::removeEdge(m_chunkManager,x,y,static_cast<EdgeDirection>(side));m_hasUnsavedChanges=true;
}

void WorldViewWindow::setGroundLayer(GroundLayer layer, const std::string& title)
{
    selection.clear();cancelPainting();
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
        m_canvasSize.y/m_viewport.m_cellSize,m_canvasSize.x/m_viewport.m_cellSize};
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

    m_toolSettings.m_edgeTexture = [this,edgeTexture](const std::string& id,int size){
        const auto found=m_wallTextures.find(id);
        return found!=m_wallTextures.end()?(m_editorStyle==GridEditorStyle::GraphicTiles?found->second:ImTextureID(0)):(edgeTexture?edgeTexture(id,size):ImTextureID(0));
    };

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
        cancelPainting();
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

    if (m_groundOnly) ImGui::TextUnformatted(m_editorStyle==GridEditorStyle::GraphicTiles
        ? "Grafik-Tiles | LMB: Malen | RMB ziehen: Auswahl | RMB klicken: Aktionen | Pan: Verschieben | Ctrl+Mausrad: Zoom"
        : "Schematisch | Pencil: Kanten zeichnen | Pan: Verschieben | Ctrl+Mausrad: Zoom");
    if (!m_groundOnly || m_editorToolsEnabled) drawLayerSelector();

    if (toolbarBlocksMapInput ||
        m_blockMapInputFrames > 0)
    {
        WorldView::stopPainting(m_painter);
    }

    ImGui::TextUnformatted("Border Shape:");ImGui::SameLine();
    if(ImGui::RadioButton("Rectangle",borderShape==GridBorderShape::Rectangular))borderShape=GridBorderShape::Rectangular;
    toolbarBlocksMapInput=toolbarBlocksMapInput || ImGui::IsItemHovered();
    ImGui::SameLine();if(ImGui::RadioButton("Diagonal",borderShape==GridBorderShape::Diagonal))borderShape=GridBorderShape::Diagonal;
    toolbarBlocksMapInput=toolbarBlocksMapInput || ImGui::IsItemHovered();
    ImGui::Separator();

    ImVec2 canvasPosition = ImGui::GetCursorScreenPos();
    canvasPosition.x += m_viewport.m_rulerWidth;
    canvasPosition.y += m_viewport.m_rulerHeight;

    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.x -= m_viewport.m_rulerWidth;
    canvasSize.y -= m_viewport.m_rulerHeight;

    if (canvasSize.x <= 0 || canvasSize.y <= 0) { ImGui::End(); return; }
    if(m_hasPendingView && m_pendingView.visibleWidth>0)
        m_groundAspect=m_pendingView.visibleWidth/m_pendingView.visibleHeight;
    if(m_groundOnly && m_groundAspect>0) {
        const ImVec2 fitted{(std::min)(canvasSize.x,canvasSize.y*m_groundAspect),
            (std::min)(canvasSize.y,canvasSize.x/m_groundAspect)};
        canvasPosition.x+=(canvasSize.x-fitted.x)*.5f;
        canvasPosition.y+=(canvasSize.y-fitted.y)*.5f;
        canvasSize=fitted;
    }
    m_canvasSize=canvasSize;
    if(m_hasPendingView) {
        const auto current=groundView();
        // Viewport synchronization echoes the just-fitted chunk back to the grid.
        // Keep full-size mode (and its coordinate label) for that same view.
        const bool sameView=std::abs(current.centerX-m_pendingView.centerX)<.05f &&
            std::abs(current.centerY-m_pendingView.centerY)<.05f &&
            std::abs(current.visibleHeight-m_pendingView.visibleHeight)<.05f &&
            (m_pendingView.visibleWidth<=0 || std::abs(current.visibleWidth-m_pendingView.visibleWidth)<.05f);
        if(!sameView)m_viewport.m_fittedChunkSize=0;
        m_viewport.m_cellSize=(std::clamp)(canvasSize.y/m_pendingView.visibleHeight,2.f,256.f);
        m_viewport.m_cameraX=m_pendingView.centerX*m_viewport.m_cellSize-canvasSize.x*.5f;
        m_viewport.m_cameraY=m_pendingView.centerY*m_viewport.m_cellSize-canvasSize.y*.5f;
        m_hasPendingView=false;
    }
    const auto navigationBefore=groundView();
    WorldView::updateChunkFit(m_viewport, canvasSize);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    bool dropPreview=false;ImVec2 dropCell{};int dropSide=-1,dropSpan=1;
    {
        // Every grid canvas owns its drag, including the ordinary editor without ground tiles.
        // Otherwise ImGui treats a drag on the drawn raster as a window-move gesture.
        const auto cursor = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(canvasPosition);
        ImGui::InvisibleButton("##GroundCanvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
        const auto mouse=ImGui::GetMousePos();
        const float tileX=(mouse.x-canvasPosition.x+m_viewport.m_cameraX)/m_viewport.m_cellSize;
        const float tileY=(mouse.y-canvasPosition.y+m_viewport.m_cameraY)/m_viewport.m_cellSize;
        const bool selectionAllowed=!inputBlocked && !toolbarBlocksMapInput && m_blockMapInputFrames==0 && !ImGui::GetDragDropPayload();
        if(!selectionAllowed){selecting=false;selectionPending=false;}
        const GridCellCoord pointed{int(std::floor(tileX)),int(std::floor(tileY))};
        const bool validCell=!m_groundOnly || (pointed.x>=0 && pointed.y>=0 && pointed.x<m_ground.width && pointed.y<m_ground.height);
        if(selectionAllowed && validCell && ImGui::IsItemHovered() && !ImGui::IsMouseDown(0) && ImGui::IsMouseClicked(1)){
            selectionPending=true;selecting=false;selectionButton=1;selectionStart=pointed;
            m_tileStroke.reset();WorldView::stopPainting(m_painter);
        }
        // Once a working selection exists, a short left click edits it cell by cell.
        // Dragging still replaces it with an inclusive rectangle.
        // An active graphic tile owns the left button. Selection cells remain
        // editable with the Select tool, or after the tile brush is cleared.
        const bool tileOwnsLeftButton=m_activeTile.kind==GridPaintKind::Ground || m_activeTile.kind==GridPaintKind::Edge;
        if(selectionAllowed && (activeTool==EditorTool::Select || (!tileOwnsLeftButton && !selection.cells().empty())) && ImGui::IsItemClicked(0) && !ImGui::IsMouseDown(1) && validCell){
            selectionPending=true;selecting=false;selectionButton=0;selectionStart=pointed;
            selectionResizeEdges=0;selectionResizeOriginal.clear();
            if(selection.bounds(selectionOldLeft,selectionOldTop,selectionOldRight,selectionOldBottom) && selection.contains(pointed.x,pointed.y)){
                // Every exposed side is a resize handle.  This includes dents,
                // holes and other user-modified edges inside the bounding box.
                const float localX=tileX-std::floor(tileX),localY=tileY-std::floor(tileY);
                const float distances[]={localX,1-localX,localY,1-localY};
                const bool exposed[]={!selection.contains(pointed.x-1,pointed.y),!selection.contains(pointed.x+1,pointed.y),
                    !selection.contains(pointed.x,pointed.y-1),!selection.contains(pointed.x,pointed.y+1)};
                float nearest=2.f;for(int side=0;side<4;++side)if(exposed[side])nearest=(std::min)(nearest,distances[side]);
                for(int side=0;side<4;++side)if(exposed[side] && distances[side]<=nearest+.12f)selectionResizeEdges|=1<<side;
                // Opposite sides only both qualify when the click is about equally far from
                // each (e.g. mid-cell beside a hole). Dragging both would move instead of
                // resize, so keep the side on the outer selection bounds, else the nearer.
                const bool outer[]={pointed.x==selectionOldLeft,pointed.x==selectionOldRight,
                    pointed.y==selectionOldTop,pointed.y==selectionOldBottom};
                for(int first=0;first<4;first+=2){
                    const int pair=3<<first;if((selectionResizeEdges&pair)!=pair)continue;
                    int keep=first;
                    if(outer[first]!=outer[first+1])keep=outer[first]?first:first+1;
                    else if(distances[first+1]<distances[first])keep=first+1;
                    selectionResizeEdges&=~pair;selectionResizeEdges|=1<<keep;
                }
                if(selectionResizeEdges)selectionResizeOriginal=selection.cells();
            }
            m_tileStroke.reset();WorldView::stopPainting(m_painter);
        }
        if(selectionPending){
            // Max distance also survives a fast drag ending between two frames.
            const float threshold=ImGui::GetIO().MouseDragThreshold;
            if(ImGui::GetIO().MouseDragMaxDistanceSqr[selectionButton]>=threshold*threshold){
                // A new rectangle is exclusively a right-button gesture.  The
                // left button may drag only an existing outer selection edge.
                if(selectionButton==1 || selectionResizeEdges)selecting=true;
                else {selectionPending=false;selectionResizeOriginal.clear();}
            }
            if(selecting && selectionButton==0 && selectionResizeEdges){
                int left=selectionOldLeft,top=selectionOldTop,right=selectionOldRight,bottom=selectionOldBottom;
                const int shiftX=pointed.x-selectionStart.x,shiftY=pointed.y-selectionStart.y;
                if(selectionResizeEdges&1)left=(std::min)(selectionOldLeft+shiftX,right);
                if(selectionResizeEdges&2)right=(std::max)(selectionOldRight+shiftX,left);
                if(selectionResizeEdges&4)top=(std::min)(selectionOldTop+shiftY,bottom);
                if(selectionResizeEdges&8)bottom=(std::max)(selectionOldBottom+shiftY,top);
                if(m_groundOnly){left=(std::clamp)(left,0,m_ground.width-1);right=(std::clamp)(right,0,m_ground.width-1);top=(std::clamp)(top,0,m_ground.height-1);bottom=(std::clamp)(bottom,0,m_ground.height-1);}
                selection.resizePreservingShape(selectionResizeOriginal,selectionOldLeft,selectionOldTop,selectionOldRight,selectionOldBottom,left,top,right,bottom);
            }else if(selecting)selection.select(selectionStart,pointed,m_groundOnly?m_ground.width:0,m_groundOnly?m_ground.height:0);
            if(!ImGui::IsMouseDown(selectionButton)){
                if(!selecting && ImGui::IsMouseReleased(selectionButton) && ImGui::IsItemHovered()){
                    if(selectionButton==0)
                        selection.toggle(selectionStart,m_groundOnly?m_ground.width:0,m_groundOnly?m_ground.height:0);
                    else if(selection.contains(pointed.x,pointed.y))
                        ImGui::OpenPopup("Grid Selection Actions");
                }
                selectionPending=false;selecting=false;selectionResizeEdges=0;selectionResizeOriginal.clear();
            }
        }
        if(ImGui::BeginPopup("Grid Selection Actions")){
            for(const auto& action:selectionActions)if(!action.graphicOnly || m_editorStyle==GridEditorStyle::GraphicTiles){
                if(ImGui::MenuItem(action.label.c_str(),nullptr,false,!selection.cells().empty())){
                    const SelectionActionContext context{selection.cells(),borderShape};action.callback(context);
                }
            }
            ImGui::EndPopup();
        }
        inputBlocked=inputBlocked || selectionPending || ImGui::IsPopupOpen("Grid Selection Actions");
        auto place=[&](GridPaintSelection selection,bool deliver,float x,float y,bool stroke=false,int axis=-1){
            auto target=gridPaintTarget(x,y,m_viewport.m_cellSize,m_ground.width,m_ground.height,selection,m_edgeSpan);
            PlacementAction action=PlacementAction::Place;
            GridPlacementState state;
            if(stroke && m_placementQuery){
                auto probe=selection;probe.edgeSpan=1;
                const auto hit=gridPaintTarget(x,y,m_viewport.m_cellSize,m_ground.width,m_ground.height,probe);
                if(!hit.valid)return;
                state=m_placementQuery(selection,hit);action=evaluatePlacement(state);
                if(action==PlacementAction::Remove && state.instance.valid)target=state.instance;
            }
            if(!target.valid || (axis>=0 && target.side%2!=axis))return;
            dropPreview=true;dropCell={float(target.x),float(target.y)};dropSide=target.side;dropSpan=target.span;
            if(deliver && (!stroke || m_tileStroke.visit(target))){
                if(stroke && m_placementApply)m_placementApply(selection,target,action);
                else if(selection.kind==GridPaintKind::Ground && m_groundDrop)m_groundDrop(target.x,target.y,selection.index);
                else if(selection.kind==GridPaintKind::Edge && m_wallDrop)m_wallDrop(target.x,target.y,target.side,selection.index);
            }
        };
        const bool externalPaint=m_activeTile.kind==GridPaintKind::Ground || m_activeTile.kind==GridPaintKind::Edge;
        const bool graphic=m_editorStyle==GridEditorStyle::GraphicTiles;
        const bool allowed=m_groundOnly && graphic && externalPaint && activeTool==EditorTool::Pencil && !inputBlocked && !toolbarBlocksMapInput &&
            m_blockMapInputFrames==0 && !ImGui::GetIO().KeyCtrl && !ImGui::GetDragDropPayload() && !ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
        // Consume the last pointer position before ending a fast stroke on release.
        const bool finishing=m_tileStroke.gesture.active && ImGui::IsMouseReleased(ImGuiMouseButton_Left);
        m_tileStroke.update(ImGui::IsItemClicked(ImGuiMouseButton_Left),ImGui::IsMouseDown(ImGuiMouseButton_Left) || finishing,allowed);
        if(allowed && ImGui::IsItemHovered()){
            place(m_activeTile,false,tileX,tileY);
            m_tileStroke.sample(tileX,tileY,[&](float x,float y){
                if(m_activeTile.kind!=GridPaintKind::Edge){place(m_activeTile,true,x,y,true);return;}
                auto& line=m_tileStroke.edgeLine;
                if(!line.active){
                    auto probe=m_activeTile;probe.edgeSpan=1;
                    const auto first=gridPaintTarget(x,y,m_viewport.m_cellSize,m_ground.width,m_ground.height,probe);
                    if(!first.valid)return;
                    line.begin(first,x,y);place(m_activeTile,true,x,y,true);return;
                }
                line.sample(x,y,[&](GridPaintTarget edge){
                    // Sample just inside the locked physical edge; the common hit test
                    // remains authoritative for bounds, spans and host metadata.
                    const float px=edge.side==3?edge.x+.001f:edge.x+.5f;
                    const float py=edge.side==0?edge.y+.001f:edge.y+.5f;
                    place(m_activeTile,true,px>=m_ground.width?px-.002f:px,py>=m_ground.height?py-.002f:py,true);
                });
            });
        }else m_tileStroke.breakPath();
        if(finishing)m_tileStroke.reset();
        if(m_groundOnly && graphic && (m_groundDrop || m_wallDrop) && ImGui::BeginDragDropTarget()) {
            if(const auto* payload=ImGui::AcceptDragDropPayload(GroundTilePayloadType,ImGuiDragDropFlags_AcceptBeforeDelivery))
                if(payload->DataSize==sizeof(std::uint32_t))place({GridPaintKind::Ground,*static_cast<const std::uint32_t*>(payload->Data)},payload->IsDelivery(),tileX,tileY);
            if(const auto* payload=ImGui::AcceptDragDropPayload(WallTilePayloadType,ImGuiDragDropFlags_AcceptBeforeDelivery))
                if(payload->DataSize==sizeof(WallTilePayload)){
                    const auto& tile=*static_cast<const WallTilePayload*>(payload->Data);
                    place({GridPaintKind::Edge,tile.index,tile.span},payload->IsDelivery(),tileX,tileY);
                }
            ImGui::EndDragDropTarget();
        }
        // IsWindowHovered() normally rejects any active item, including this canvas.
        // Keep our captured drag alive; other windows/items still block new input.
        const bool canvasActive = ImGui::IsItemActive();
        inputBlocked = inputBlocked || ImGui::GetDragDropPayload()!=nullptr || (!canvasActive &&
            !ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows));
        if (inputBlocked) WorldView::stopPainting(m_painter);
        ImGui::SetCursorScreenPos(cursor);
    }


    if (activeTool!=EditorTool::Select && !inputBlocked &&
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

    const bool fitClickedChunk = activeTool!=EditorTool::Select && !inputBlocked && !toolbarBlocksMapInput &&
        m_blockMapInputFrames == 0 && m_hover.m_hasHoveredCell &&
        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
        (!m_groundOnly || m_ground.at(m_hover.m_hoveredCellX, m_hover.m_hoveredCellY));
    if (fitClickedChunk) {
        m_tileStroke.reset();
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
    const bool userNavigation=fitClickedChunk || (!inputBlocked && !toolbarBlocksMapInput && m_blockMapInputFrames==0 && m_hover.m_hasHoveredCell &&
        ((activeTool==EditorTool::Scroll && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) ||
         ((activeTool==EditorTool::Scroll || ImGui::GetIO().KeyCtrl) && ImGui::GetIO().MouseWheel!=0)));
    if(userNavigation && m_groundNavigation && (navigationAfter.centerX!=navigationBefore.centerX ||
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

    GroundCanvasImage groundImage;
    if(m_groundOnly && m_editorStyle==GridEditorStyle::GraphicTiles && m_groundRenderer)groundImage=m_groundRenderer(groundView(),canvasSize);
    if(groundImage.texture)drawList->AddImage(groundImage.texture,canvasPosition,mapCanvasEnd,groundImage.uvMin,groundImage.uvMax);
    if (m_groundOnly && m_editorStyle==GridEditorStyle::GraphicTiles && !groundImage.texture) {
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
    if(gridLinesVisible)WorldView::drawGrid(m_viewport, m_style, m_chunkSize,
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

    for(const auto cell:selection.cells()){
        const ImVec2 p{canvasPosition.x+cell.x*m_viewport.m_cellSize-m_viewport.m_cameraX,canvasPosition.y+cell.y*m_viewport.m_cellSize-m_viewport.m_cameraY};
        const ImVec2 q{p.x+m_viewport.m_cellSize,p.y+m_viewport.m_cellSize};
        if(q.x<canvasPosition.x || q.y<canvasPosition.y || p.x>mapCanvasEnd.x || p.y>mapCanvasEnd.y)continue;
        drawList->AddRectFilled(p,q,IM_COL32(80,190,255,45));
        const auto border=IM_COL32(100,210,255,240);
        if(!selection.contains(cell.x,cell.y-1))drawList->AddLine(p,{q.x,p.y},border,2.f);
        if(!selection.contains(cell.x+1,cell.y))drawList->AddLine({q.x,p.y},q,border,2.f);
        if(!selection.contains(cell.x,cell.y+1))drawList->AddLine(q,{p.x,q.y},border,2.f);
        if(!selection.contains(cell.x-1,cell.y))drawList->AddLine({p.x,q.y},p,border,2.f);
    }
    drawList->PopClipRect();
    if(dropPreview) {
        const ImVec2 p{canvasPosition.x+dropCell.x*m_viewport.m_cellSize-m_viewport.m_cameraX,
            canvasPosition.y+dropCell.y*m_viewport.m_cellSize-m_viewport.m_cameraY};
        drawList->PushClipRect(canvasPosition,mapCanvasEnd,true);
        if(dropSide<0)drawList->AddRect(p,{p.x+m_viewport.m_cellSize,p.y+m_viewport.m_cellSize},IM_COL32(255,220,80,255),0.f,3.f);
        else {
            const float s=m_viewport.m_cellSize;
            ImVec2 a=p,b=p;
            if(dropSide==0 || dropSide==2){a.y+=dropSide==2?s:0;b={a.x+s*dropSpan,a.y};}
            else {a.x+=dropSide==1?s:0;b={a.x,a.y+s*dropSpan};}
            drawList->AddLine(a,b,IM_COL32(255,220,80,255),5.f);
        }
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

    if (activeTool!=EditorTool::Select && !inputBlocked &&
        !toolbarBlocksMapInput &&
        m_blockMapInputFrames == 0 &&
        activeTool == EditorTool::Pencil && !fitClickedChunk && m_editorStyle==GridEditorStyle::Schematic)
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

    if (activeTool!=EditorTool::Select && !inputBlocked &&
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

        if (activeTool == EditorTool::Pencil && m_editorStyle==GridEditorStyle::Schematic)
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
        else if(activeTool==EditorTool::Eraser)
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
        selection.clear();cancelPainting();
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
    selection.clear();cancelPainting();
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
