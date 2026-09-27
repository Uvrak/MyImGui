#pragma once
#include <imgui.h>
#include "GridPaintSelection.h"
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <functional>

struct GroundViewState {
    float centerX = 0, centerY = 0, visibleHeight = 16;
    float visibleWidth = 0; // Zero keeps the current canvas aspect ratio.
};
using GroundDropCallback = std::function<void(int, int, std::uint32_t)>;
using GroundNavigationCallback = std::function<void(GroundViewState)>;
struct GroundViewBinding {
    std::function<bool(GroundViewState&)> read;
    GroundNavigationCallback write;
};
struct GroundCanvasImage {
    ImTextureID texture = 0;
    ImVec2 uvMin{0,0},uvMax{1,1};
};
using GroundCanvasRenderer = std::function<GroundCanvasImage(GroundViewState,ImVec2)>;
// Side order: north, east, south, west.
using WallDropCallback = std::function<void(int,int,int,std::uint32_t)>;

// Row-major, finite ground cells. Texture ownership stays with the caller.
// Texture IDs must belong to the renderer used by the current ImGui context.
struct GroundMaterial {
    ImTextureID texture = 0;
    ImU32 color = IM_COL32_WHITE;
};
// Optional image layers drawn over the ground in the given order: each sprite covers a
// rectangle in cell units and may extend beyond its cell. Texture ownership stays with the
// caller. Layers can be hidden; the grid knows nothing about what the images show.
struct GroundSprite {
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    ImTextureID texture = 0;
    ImU32 color = IM_COL32_WHITE;
};
struct GroundSpriteLayer {
    std::string name;
    bool visible = true;
    std::vector<GroundSprite> sprites;
};
struct GroundLayer {
    int width = 0, height = 0;
    std::vector<std::uint16_t> cells;
    std::vector<GroundMaterial> materials;
    // Optional cell-border decoration, supplied by the importing application.
    // Bits: north=1, east=2, south=4, west=8. No game-specific classification here.
    std::vector<std::uint8_t> borders;
    GroundMaterial borderMaterial;
    void validate() const {
        if (width <= 0 || height <= 0 ||
            cells.size() != static_cast<size_t>(width) * height || materials.empty())
            throw std::invalid_argument("Invalid ground layer dimensions");
        for (auto cell : cells) if (cell >= materials.size())std::invalid_argument("Invalid ground material index");
        if (!borders.empty() && borders.size() != cells.size())
            throw std::invalid_argument("Invalid ground border dimensions");
        for (auto border : borders) if (border > 15)
            throw std::invalid_argument("Invalid ground border mask");
    }
    const GroundMaterial* at(int x, int y) const {
        if (x < 0 || y < 0 || x >= width || y >= height) return nullptr;
        return &materials[cells[static_cast<size_t>(y) * width + x]];
    }
};
