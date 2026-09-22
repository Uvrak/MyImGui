#pragma once

#include "WorldViewViewport.h"
#include "GroundLayer.h"
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

    void setGroundLayer(GroundLayer layer, const std::string& title);
    void focusGroundCell(int x, int y);
    GroundViewState groundView() const;
    void setGroundView(GroundViewState view);
    void setGroundCell(int x, int y, GroundMaterial material);
    void setGroundBorder(int x, int y, std::uint8_t border);
    void setGroundDropCallback(GroundDropCallback callback) { m_groundDrop = std::move(callback); }
    void setGroundNavigationCallback(GroundNavigationCallback callback) { m_groundNavigation = std::move(callback); }
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
    GroundDropCallback m_groundDrop;
    GroundNavigationCallback m_groundNavigation;
    ImVec2 m_canvasSize{};
    GroundViewState m_pendingView;
    bool m_hasPendingView = false;
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
