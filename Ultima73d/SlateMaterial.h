#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>

// High resolution slate roof for Britannia3d's stone houses, in the blue-grey of U7's slate
// roofs: rows of overlapping rectangular slates (staggered by half a slate), each with a
// thick lower edge casting a shadow on the row below, chipped corners and a fine cleft
// texture; seamless in both directions. Colour and a normal map (tangent space: x along u =
// along the eaves, y along v = down the slope).
//
// Stored as <asset dir>/Materials/slate-roof.png and slate-roof-normal.png; missing files are
// generated, existing ones are used as they are (so they can be replaced).
namespace SlateMaterial {

inline constexpr float Size = 1.f;          // metres covered by one texture, both directions
inline constexpr int Pixels = 1024;         // 1024 texels per metre

struct Images {
    int size = 0;
    std::vector<std::uint8_t> albedo, normal;   // RGBA
};

Images generate();
Images loadOrCreate(const std::filesystem::path& assetDirectory);

}
