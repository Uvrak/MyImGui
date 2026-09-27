#pragma once
#include "U7Data.h"
#include <cstdint>
#include <filesystem>
#include <vector>

// High resolution dressed stone for Britannia3d's town walls, close to U7's fortress blocks
// (shapes 191, 192, 263 ...): courses of large rectangular blocks of varying length, pinkish,
// grey or beige as U7's, with narrow dark joints, bevelled edges (lit upper left, shadow lower
// right) and a granular surface. Colours come from U7's own fortress block pixels. Seamless in
// both directions; colour and a normal map.
//
// Stored as <asset dir>/Materials/town-wall-hd.png and town-wall-hd-normal.png; missing files
// are generated, existing ones are used as they are (so they can be replaced).
namespace AshlarMaterial {

inline constexpr float Size = 2.f;          // metres covered by the texture, both directions
inline constexpr int Pixels = 1024;         // 512 texels per metre

struct Images {
    int width = 0, height = 0;
    std::vector<std::uint8_t> albedo, normal;   // RGBA
};

Images generate(const U7::Data& data);
Images loadOrCreate(const std::filesystem::path& assetDirectory, const U7::Data& data);

}
