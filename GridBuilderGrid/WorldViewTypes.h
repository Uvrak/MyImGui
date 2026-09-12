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

namespace WorldView
{
struct ToolSettings
{
    EditorTool m_activeTool = EditorTool::Pencil;
    std::string m_activeEdgeId = "wall";
    std::string m_activeColorId = "color-1";
    std::string m_activeMiscId;
    std::string m_activeMiscColorId;
    bool m_paintMisc = false;
    EdgeTextureResolver m_edgeTexture;
    MiscTextureResolver m_miscTexture;
    OpenEdgeColorMenuCallback m_openEdgeColorMenu;
    OpenMiscColorMenuCallback m_openMiscColorMenu;
};
struct Viewport;
struct Hover;
struct WallPainting;
struct Eraser;
struct CellInteraction;
struct RenderStyle;
}
