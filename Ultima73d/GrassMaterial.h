#pragma once
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <vector>

// High resolution lawn for Britannia3d in the style of U7's grass tiles: dense short blades in
// U7's greens (the pixels of its grass tiles), lighter tips, darker gaps between tufts, a few
// clover leaves and small dry patches, lit as U7 draws them. Seamless; colour and a normal map.
//
// Stored as <asset dir>/Materials/grass-hd.png and grass-hd-normal.png; missing files are
// generated, existing ones are used as they are (so they can be replaced).
namespace GrassMaterial {

inline constexpr float Size = 1.f;          // metres covered by the texture
inline constexpr int Pixels = 1024;

struct Images {
    int size = 0;
    std::vector<std::uint8_t> albedo, normal;   // RGBA
};

Images generate(const std::vector<glm::vec3>& colours);
Images loadOrCreate(const std::filesystem::path& assetDirectory, const std::vector<glm::vec3>& colours);

}
