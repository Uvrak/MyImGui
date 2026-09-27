#pragma once

#include <d3d11.h>

#include <functional>
#include <string>
#include <memory>
#include "MapPlayerMarker.h"
#include "GroundLayer.h"
#include "GridPaintSelection.h"
#include "GridCellSelection.h"
#include "EditorTool.h"
#include "EditorTextureResolver.h"

class ChunkManager;

class GridBuilderGrid
{
public:
    GridBuilderGrid(
        ID3D11Device* device,
        int chunkSize,
        const std::string& iconDirectory = {}
    );

    ~GridBuilderGrid();

    GridBuilderGrid(
        const GridBuilderGrid&
    ) = delete;

    GridBuilderGrid& operator=(
        const GridBuilderGrid&
        ) = delete;

    void draw(
        bool* isOpen
    );

    // Uses the existing viewport in navigation-only mode; no editor textures required.
    void setGroundLayer(GroundLayer layer, const std::string& title);
    void focusGroundCell(int x, int y);
    GroundViewState groundView() const;
    void setGroundView(GroundViewState view);
    void setGroundCell(int x, int y, GroundMaterial material);
    // Image layers over the ground (see GroundSpriteLayer); returns the layer index.
    std::size_t addSpriteLayer(GroundSpriteLayer layer);
    std::size_t spriteLayerCount() const;
    const GroundSpriteLayer& spriteLayer(std::size_t index) const;
    void setSpriteLayerVisible(std::size_t index, bool visible);
    void setGroundBorder(int x, int y, std::uint8_t border);
    void setEditorStyle(GridEditorStyle style);
    void setGridLinesVisible(bool visible);
    GridEditorStyle editorStyle() const;
    bool graphicTilesVisible() const {return editorStyle()==GridEditorStyle::GraphicTiles;}
    void setBorderShape(GridBorderShape shape);
    GridSelectionGeometry selectionGeometry() const {return GridSelectionGeometry::Rectangle;}
    GridBorderShape borderShape() const;
    bool hasSelection() const;
    void clearSelection();
    const std::vector<GridCellCoord>& selectedCells() const;
    void registerSelectionAction(const std::string& label,GridSelectionAction callback,bool graphicOnly=false);
    void setActiveTool(EditorTool tool);
    EditorTool activeTool() const;
    void drawEditorStyleMenu();
    void setGraphicCatalogsRenderer(std::function<void()> draw);
    void drawGraphicCatalogs();
    void setPlacementCallbacks(GridPlacementQuery query,GridPlacementCallback apply);
    void setActiveGroundTile(std::uint32_t index);
    void setActiveEdgeTile(std::uint32_t index,int span=1);
    void clearActiveTile();
    GridPaintSelection activeTile() const;
    bool isActiveGroundTile(std::uint32_t index) const;
    bool isActiveEdgeTile(std::uint32_t index) const;
    void setEdgeSpanResolver(GridEdgeSpanResolver resolver);
    void setGroundDropCallback(GroundDropCallback callback);
    void setWallDropCallback(WallDropCallback callback);
    void setWallTile(int x,int y,int side,const std::string& id,ImTextureID texture);
    void removeWallTile(int x,int y,int side);
    void setGroundNavigationCallback(GroundNavigationCallback callback);
    void bindGroundView(GroundViewBinding binding);
    void setGroundCanvasRenderer(GroundCanvasRenderer renderer);
    void setEditorTextureResolver(EditorTextureResolver resolver);
    void setEditorToolsEnabled(bool enabled);

    void registerMap(const std::string& key, const std::string& filename);
    void setMapDirectory(const std::string& directory);
    bool openMap(const std::string& key, const std::string& displayName = {});
    bool saveCurrentMap();
    void setPlayerMarker(const MapPlayerMarker& marker);

    using DebugCallback =
        std::function<void(const std::string&)>;

    void setDebugCallback(
        DebugCallback callback
    );

    const ChunkManager& map() const;

private:
    class Impl;

    std::unique_ptr<Impl> m_impl;
};
