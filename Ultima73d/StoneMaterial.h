#pragma once
#include "U7Data.h"
#include <cstdint>
#include <filesystem>
#include <vector>

// High resolution rubble stone for Britannia3d's stone houses in the style of U7's stone walls:
// irregular stones (about 0.18 x 0.12 m, a little smaller than U7's) with dark joints, a light
// rim on their upper left edge and a shadow on the lower right as U7 draws them, speckled
// surfaces; every stone takes its colour from the pixels of U7's own stone wall (shape 348), so
// the wall keeps U7's greys and browns. Seamless in both directions; colour and a normal map
// (tangent space: x along u, y along v = downwards on a wall).
//
// Stored as <asset dir>/Materials/stone-wall-hd.png and stone-wall-hd-normal.png; missing files
// are generated, existing ones are used as they are (so they can be replaced).
namespace StoneMaterial {

inline constexpr float Size = 1.f;          // metres covered by the texture, both directions
inline constexpr int Pixels = 1024;         // 1024 texels per metre

struct Images {
    int width = 0, height = 0;
    std::vector<std::uint8_t> albedo, normal;   // RGBA
};

Images generate(const U7::Data& data);
Images loadOrCreate(const std::filesystem::path& assetDirectory, const U7::Data& data);

}
