#pragma once
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <vector>

// High resolution cobblestone for Britannia3d's streets in the style of U7's Trinsic streets:
// rounded, irregular cobbles (about 0.12 m) in U7's street greys, set in dark sandy gaps, each
// stone lit on its upper left and shaded lower right as U7 draws them. The colours are the
// pixels of U7's own street tiles. Seamless; colour and a normal map (tangent space: x east,
// y south).
//
// Stored as <asset dir>/Materials/cobble-hd.png and cobble-hd-normal.png; missing files are
// generated, existing ones are used as they are (so they can be replaced).
namespace CobbleMaterial {

inline constexpr float Size = 1.f;          // metres covered by the texture
inline constexpr int Pixels = 1024;

struct Images {
    int size = 0;
    std::vector<std::uint8_t> albedo, normal;   // RGBA
};

Images generate(const std::vector<glm::vec3>& colours);
Images loadOrCreate(const std::filesystem::path& assetDirectory, const std::vector<glm::vec3>& colours);

}
