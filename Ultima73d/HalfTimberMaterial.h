#pragma once
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <vector>

// High resolution half-timbering for Britannia3d's light ("sandstone") houses, after U7's walls:
// lime plaster in U7's plaster colour between dark oak timbers - a sill at the floor, a plate
// under the eaves, a rail at 1.1 m, posts every Width metres and a brace in each panel (rising
// in the lower one, falling in the upper one). One storey high, seamless along the wall; the
// shader maps it in metres, its bottom row at the floor. Colour and a normal map (tangent space:
// x along the wall, y downwards), timbers standing 1.5 cm proud of the plaster.
//
// Stored as <asset dir>/Materials/half-timber-hd.png and half-timber-hd-normal.png; missing files
// are generated, existing ones are used as they are (so they can be replaced).
namespace HalfTimberMaterial {

inline constexpr float Width = 1.5f;           // metres along the wall per repeat (post to post)
inline constexpr int PixelsPerMetre = 256;

struct Images {
    int width = 0, height = 0;
    std::vector<std::uint8_t> albedo, normal;   // RGBA
};

// storey: metres high; plaster: U7's plaster colour (0..1).
Images generate(float storey, glm::vec3 plaster);
Images loadOrCreate(const std::filesystem::path& assetDirectory, float storey, glm::vec3 plaster);

}
