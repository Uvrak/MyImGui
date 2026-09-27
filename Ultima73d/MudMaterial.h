#pragma once
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <vector>

// High resolution trodden earth (mud) for Britannia3d, to lie next to the HD grass: damp brown
// soil in the colours of U7's earth pixels, small stones, cracks, dark hollows and a few stray
// grass blades, so lawn and mud blend. Seamless; colour and a normal map.
//
// Stored as <asset dir>/Materials/mud-hd.png and mud-hd-normal.png; missing files are
// generated, existing ones are used as they are (so they can be replaced).
namespace MudMaterial {

inline constexpr float Size = 1.f;          // metres covered by the texture
inline constexpr int Pixels = 1024;

struct Images {
    int size = 0;
    std::vector<std::uint8_t> albedo, normal;   // RGBA
};

Images generate(const std::vector<glm::vec3>& colours);
Images loadOrCreate(const std::filesystem::path& assetDirectory, const std::vector<glm::vec3>& colours);

}
