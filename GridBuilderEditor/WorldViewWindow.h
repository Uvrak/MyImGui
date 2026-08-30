#pragma once
#include <SDL3/SDL.h>
#include "imgui.h"
#include "ChunkManager.h"
#include "WallDirection.h"
#include "EditorTool.h"
#include "EdgeDirection.h"

#include <optional>
#include <vector>
#include <string>
#include <functional>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <fstream>

#include "MapPlayerMarker.h"



using EdgeTextureResolver =
std::function<
	SDL_Texture* (
		const std::string& edgeId,
		int size
		)
>;
using AssignEdgeColorCallback =
std::function<
	void(
		const std::string& colorId
		)
>;

using OpenEdgeColorMenuCallback =
std::function<
	void(
		const std::string& edgeId,
		const std::string& assignedColorId,
		const AssignEdgeColorCallback&
		assignColor
		)
>;

using MiscTextureResolver =
std::function<
	SDL_Texture* (
		const std::string& miscId,
		int size
		)
>;

using OpenMiscColorMenuCallback =
std::function<
	void(
		const std::string& miscId,
		const std::string& assignedColorId,
		const AssignEdgeColorCallback&
		assignColor
		)
>;

enum class WallOrientation
{
	Horizontal,
	Vertical
};

enum class HoveredWall
{
	None,
	North,
	East,
	South,
	West
};

struct GridView{
	float startX = 0.0f;
	float startY = 0.0f;

	int firstVisibleCellX = 0;
	int firstVisibleCellY = 0;
};

struct PaintedEdge
{
	int cellX;
	int cellY;

	EdgeDirection direction =
		EdgeDirection::North;

	bool changed = false;

	std::string previousEdgeId;
	std::string previousColorId;
};

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
    float m_cellSize = 34.0f;

	int m_hoveredCellX = 0;
	int m_hoveredCellY = 0;
	bool m_hasHoveredCell = false;

	ImU32 m_previewColor = IM_COL32(255, 255, 230, 40);

	int m_selectedCellX = 0;
	int m_selectedCellY = 0;
	bool m_hasSelectedCell = false;
	
	void drawWalls(
		ImDrawList* drawList,
		ImVec2 canvasPosition,
		ImVec2 canvasSize
	);

	void drawMisc(
		ImDrawList* drawList,
		ImVec2 canvasPosition,
		ImVec2 canvasSize
	);

	void drawPlayerMarker(
		ImDrawList* drawList,
		ImVec2 canvasPosition
	);

	MapPlayerMarker m_playerMarker;

	void drawLayerWalls(
		ImDrawList* drawList,
		ImVec2 canvasPosition,
		ImVec2 canvasSize,
		int layer,
		ImU32 color,
		bool activeLayer
	);

    ImU32 m_backgroundColor = IM_COL32(36, 78, 150, 255);
    ImU32 m_gridColor = IM_COL32(40, 68, 110, 255);
    ImU32 m_chunkGridColor = IM_COL32(38, 66, 108, 255);

    ImU32 m_hoverColor = IM_COL32(255, 255, 255, 50);
    ImU32 m_selectionColor = IM_COL32(255, 220, 0, 255);

	float m_rulerHeight = 25.0f;
	float m_rulerWidth = 40.0f;
	ImU32 m_rulerColor = IM_COL32(0, 0, 0, 255);
	ImU32 m_rulerTextColor = IM_COL32(230, 230, 230, 255);

	float m_cameraX = 0.0f;
	float m_cameraY = 0.0f;

	int m_labelStep = 4;
	int m_longTickStep = 4;

	GridView m_gridView;

	void updateGridView();

	bool isMouseInsideCanvas(
		ImVec2 canvasPosition,
		ImVec2 canvasSize,
		ImVec2& mouseCanvasPosition
	) const;

	void handleZoom(
		ImVec2 canvasPosition,
		ImVec2 canvasSize
	);
	int calculateLabelStep() const;
	void drawRulers(ImDrawList* drawList, ImVec2 canvasPosition, ImVec2 canvasSize);

	void drawGrid(ImDrawList* drawList, ImVec2 canvasPosition, ImVec2 canvasSize);
	
	void drawSelection(ImDrawList* drawList, ImVec2 canvasPosition);

	void setEdge(
		int cellX,
		int cellY,
		EdgeDirection direction,
		const std::string& edgeId,
		const std::string& colorId
	);

	void removeEdge(
		int cellX,
		int cellY,
		EdgeDirection direction
	);
	
	void updateLongTickStep();

	bool m_isPainting = false;

	bool m_isRemovingEdges = false;
	std::optional<WallOrientation> m_paintOrientation;

	void handleHorizontalToVerticalTurn(
		int cellX,
		int cellY,
		int deltaY
	);

	void handleVerticalToHorizontalTurn(
		int cellX,
		int cellY,
		int deltaX
	);

	int m_horizontalPaintDirection = 0;
	int m_verticalPaintDirection = 0;

	float m_wallSelectionWidth = 6.0f;
	bool m_isBacktracking = false;

	int m_paintRow = 0;
	int m_paintColumn = 0;

	WallDirection m_paintWallDirection = WallDirection::North;

	int m_lastPaintCellX = 0;
	int m_lastPaintCellY = 0;

	void stopPainting();
	void startPainting(
		int cellX,
		int cellY,
		HoveredWall hoveredWall
	);

	void updatePainting(
		int cellX,
		int cellY,
		float localX,
		float localY
	);

	void drawWallPreview(
		ImDrawList* drawList,
		float cellLeft,
		float cellTop,
		HoveredWall hoveredWall,
		ImU32 color
	);

	void drawMiscPreview(
		ImDrawList* drawList
	);

	HoveredWall getHoveredWall(
		float localX,
		float localY,
		float selectionWidth = 6.0f
	) const;

	void handleInput(
		const ImVec2& canvasPosition,
		const ImVec2& canvasSize,
		EditorTool activeTool
	);

	void handlePan();

	void handlePencil(
		const ImVec2& canvasPosition,
		const ImVec2& canvasSize
	);

	void handleMisc(
		const ImVec2& canvasPosition,
		const ImVec2& canvasSize
	);

	void drawNotePopup();
	void drawNoteTooltip();

	void openHoveredEdgeColorMenu();
	void openHoveredMiscColorMenu();
	void openHoveredColorMenu();

	void handleEraser(
		const ImVec2& canvasPosition,
		const ImVec2& canvasSize
	);

	HoveredWall m_hoveredWall = HoveredWall::None;

	float m_hoverLocalX = 0.0f;
	float m_hoverLocalY = 0.0f;

	float m_hoverCellLeft = 0.0f;
	float m_hoverCellTop = 0.0f;

	void updateHover(
		const ImVec2& canvasPosition,
		const ImVec2& canvasSize
	);

	void drawHover(
		ImDrawList* drawList,
		ImVec2 canvasPosition,
		ImVec2 canvasSize
	);

	EditorTool m_activeTool = EditorTool::Pencil;

	void applyEdge(
		int cellX,
		int cellY,
		EdgeDirection direction
	);

	std::vector<PaintedEdge>
		m_paintedEdges;

	PaintedEdge normalizeEdge(
		int cellX,
		int cellY,
		EdgeDirection direction
	) const;

	bool isSameEdge(
		const PaintedEdge& first,
		const PaintedEdge& second
	) const;

	std::string m_activeEdgeId =
		"wall";
	std::string m_activeColorId =
		"color-1";

	std::string m_activeMiscId;
	std::string m_activeMiscColorId;

	bool m_paintMisc = false;

	EdgeTextureResolver m_edgeTexture;
	MiscTextureResolver m_miscTexture;

	OpenEdgeColorMenuCallback
		m_openEdgeColorMenu;

	OpenMiscColorMenuCallback
		m_openMiscColorMenu;

	void drawEdgeIcon(
		ImDrawList* drawList,
		SDL_Texture* texture,
		ImVec2 center,
		bool horizontal,
		ImU32 color = IM_COL32_WHITE
	);

	bool isHoveredEdge(
		int cellX,
		int cellY,
		EdgeDirection direction
	) const;

	void drawEraserPreview(
		ImDrawList* drawList
	);

	void eraseArea();
	void drawLayerSelector();

	void handleEraserAutoPan(
		const ImVec2& canvasPosition,
		const ImVec2& canvasSize
	);

	void loadSettings();
	void saveSettings() const;

	bool m_showLowerLayer = true;
	bool m_hasUnsavedChanges = false;


	int m_eraserSize = 3;

	bool inputBlocked = false;
	int m_blockMapInputFrames = 0;

	bool m_requestNotePopup = false;

	int m_noteCellX = 0;
	int m_noteCellY = 0;
	int m_noteLayer = 0;

	char m_noteTextBuffer[1024] = {};

	bool m_followPlayer = false;

	void centerOnPlayer(
		ImVec2 canvasSize
	);

};

