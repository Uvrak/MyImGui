#pragma once

#include "WorldViewViewport.h"
#include "GroundLayer.h"
#include "GridPaintSelection.h"
#include "GridPaintStroke.h"
#include "GridCellSelection.h"
#include <map>
#include "WorldViewHitTest.h"
#include "WorldViewWallPainter.h"
#include "WorldViewEraser.h"
#include "WorldViewCellInteraction.h"
#include "WorldViewRenderer.h"

class WorldViewWindow
{
public:

	explicit WorldViewWindow(int chunkSize);

	void draw(
		EditorTool activeTool,
		const std::string& activeEdgeId,
		const std::string& activeColorId,
		const std::string& activeMiscId,
		const std::string& activeMiscColorId,
		bool paintMisc,
		const EdgeTextureResolver& edgeTexture,
		const MiscTextureResolver& miscTexture,
		const OpenEdgeColorMenuCallback& openEdgeColorMenu,
		const OpenMiscColorMenuCallback& openMiscColorMenu,
		const std::function<void()>& drawToolbar,
		bool inputBlocked,
		bool showCoordinates,
		bool* isOpen
	);

    GridCellSelection selection;
    bool gridLinesVisible=true;
    bool selecting=false;
    bool selectionPending=false;
    int selectionButton=1;
    GridBorderShape borderShape=GridBorderShape::Rectangular;
    GridCellCoord selectionStart;
    struct SelectionAction {std::string label;GridSelectionAction callback;bool graphicOnly=false;};
    std::vector<SelectionAction> selectionActions;
    void setGroundLayer(GroundLayer layer, const std::string& title);
    void focusGroundCell(int x, int y);
    GroundViewState groundView() const;
    void setGroundView(GroundViewState view);
    void setGroundCell(int x, int y, GroundMaterial material);
    void setGroundBorder(int x, int y, std::uint8_t border);
    void setPlacementCallbacks(GridPlacementQuery query,GridPlacementCallback apply){m_placementQuery=std::move(query);m_placementApply=std::move(apply);}
    void setActiveTile(GridPaintSelection selection) { m_tileStroke.reset();m_activeTile=selection; }
    void setEditorStyle(GridEditorStyle style) { if(m_editorStyle==style)return;m_editorStyle=style;selecting=false;selectionPending=false;m_tileStroke.reset();WorldView::stopPainting(m_painter); }
    GridEditorStyle editorStyle() const {return m_editorStyle;}
    void cancelPainting(){selecting=false;selectionPending=false;m_tileStroke.reset();WorldView::stopPainting(m_painter);}
    GridPaintSelection activeTile() const { return m_activeTile; }
    void setEdgeSpanResolver(GridEdgeSpanResolver resolver) { m_edgeSpan=std::move(resolver); }
    void setGroundDropCallback(GroundDropCallback callback) { m_groundDrop = std::move(callback); }
    void setWallDropCallback(WallDropCallback callback) { m_wallDrop=std::move(callback); }
    void setWallTile(int x,int y,int side,const std::string& id,ImTextureID texture);
    void removeWallTile(int x,int y,int side);
    void setGroundNavigationCallback(GroundNavigationCallback callback) { m_groundNavigation = std::move(callback); }
    void setGroundCanvasRenderer(GroundCanvasRenderer renderer) { m_groundRenderer=std::move(renderer); }
    void setEditorToolsEnabled(bool enabled) { m_editorToolsEnabled = enabled; }
	bool saveMap(const std::string& filename);
	bool loadMap(const std::string& filename);
	bool hasUnsavedChanges() const;
	void setMapName(const std::string& name);

	void newMap();

	const ChunkManager& map() const;

	void removeEdgeFromAllCells(
		const std::string& edgeId
	);

	void renameEdgeInAllCells(
		const std::string& oldEdgeId,
		const std::string& newEdgeId
	);

	void renameMiscInAllCells(
		const std::string& oldMiscId,
		const std::string& newMiscId
	);

	int edgeTextureSize() const;

	bool isMouseOverCanvas() const;

	int activeLayer() const;
	void setActiveLayer(int layer);

	MapColorPalette& colorPalette();

	const MapColorPalette&
		colorPalette() const;

	bool removeMapColor(
		const std::string& colorId
	);

	const std::string& edgeColorId(
		const std::string& edgeId
	) const;

	const std::string& miscColorId(
		const std::string& miscId
	) const;

	void setEdgeColorId(
		const std::string& edgeId,
		const std::string& colorId
	);

	void setMiscColorId(
		const std::string& miscId,
		const std::string& colorId
	);

	bool showLowerLayer() const;
	void setShowLowerLayer(bool show);

	void blockMapInputOnce();

	void setPlayerMarker(
		const MapPlayerMarker& marker
	);

private:
    GroundLayer m_ground;
    GridPlacementQuery m_placementQuery;
    GridPlacementCallback m_placementApply;
    GridPaintSelection m_activeTile;
    GridEditorStyle m_editorStyle=GridEditorStyle::GraphicTiles;
    GridPaintStroke m_tileStroke;
    GridEdgeSpanResolver m_edgeSpan;
    GroundDropCallback m_groundDrop;
    WallDropCallback m_wallDrop;
    std::map<std::string,ImTextureID> m_wallTextures;
    GroundNavigationCallback m_groundNavigation;
    GroundCanvasRenderer m_groundRenderer;
    ImVec2 m_canvasSize{};
    GroundViewState m_pendingView;
    bool m_hasPendingView = false;
    float m_groundAspect = 0;
    bool m_groundOnly = false;
    bool m_editorToolsEnabled = true;
    std::string m_mapName = "Map Editor";
    ChunkManager m_chunkManager;

    int m_chunkSize;

    MapPlayerMarker m_playerMarker;
    bool m_centerOnNextMarker = false;

    void handleInput(
		const ImVec2& canvasPosition,
		const ImVec2& canvasSize,
		EditorTool activeTool
	);

    void drawLayerSelector();

    void loadSettings();

    void saveSettings() const;

    bool m_showLowerLayer = true;

    bool m_hasUnsavedChanges = false;

    bool inputBlocked = false;

    int m_blockMapInputFrames = 0;

    WorldView::Viewport m_viewport;
    WorldView::Hover m_hover;
    WorldView::WallPainting m_painter;
    WorldView::Eraser m_eraser;
    WorldView::CellInteraction m_cellInteraction;
    WorldView::ToolSettings m_toolSettings;
    WorldView::RenderStyle m_style;
};
