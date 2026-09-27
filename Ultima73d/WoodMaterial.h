#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>

// High resolution plank material replacing U7's wooden plank walls in the 3D view, in their
// style: long horizontal boards (Board metres high, Size metres long) with grooves, grain,
// knots and staggered end joints, seamless in both directions. The colour image is a detail pattern around mid grey
// that the view tints with the mean colour of each original wall graphic, so every wall keeps
// its U7 tone; the normal map (tangent space: x along the texture's u, y along its v, i.e.
// downwards on a wall) lets the boards catch the light.
//
// Stored as <asset dir>/Materials/wood-planks.png and wood-planks-normal.png; missing files
// are generated, existing ones are used as they are (so they can be replaced). A coloured
// replacement image is shown in its own colours (not tinted); without a normal map it is lit
// flat.
namespace WoodMaterial {

inline constexpr float Size = 2.f;          // metres covered by one texture, both directions
inline constexpr float Board = 0.25f;       // board height in metres
inline constexpr int Pixels = 1024;         // texture size: 512 texels per metre

struct Images {
    int size = 0;
    std::vector<std::uint8_t> albedo, normal;   // RGBA
};

Images generate();
Images loadOrCreate(const std::filesystem::path& assetDirectory);

}
