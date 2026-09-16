#pragma once

#include "WorldViewViewport.h"
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

	bool saveMap(const std::string& filename);
	bool loadMap(const std::string& filename);
	bool hasUnsavedChanges() const;

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
    ChunkManager m_chunkManager;

    int m_chunkSize;

    MapPlayerMarker m_playerMarker;

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
