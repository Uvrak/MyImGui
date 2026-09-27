#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>

// High resolution thatch for the shed roof: courses of straw bundles (Course metres), each a
// dense layer of strands running down the slope, their cut ends forming the lower edge of the
// course; golden to weathered brown, seamless in both directions. Colour and a normal map
// (tangent space: x along the texture's u = along the ridge, y along v = down the slope).
//
// Stored as <asset dir>/Materials/thatch.png and thatch-normal.png; missing files are
// generated, existing ones are used as they are (so they can be replaced).
namespace ThatchMaterial {

inline constexpr float Size = 1.f;          // metres covered by one texture, both directions
inline constexpr float Course = 0.25f;      // height of a course of bundles down the slope
inline constexpr int Pixels = 1024;         // texture size: 1024 texels per metre

struct Images {
    int size = 0;
    std::vector<std::uint8_t> albedo, normal;   // RGBA
};

Images generate();
Images loadOrCreate(const std::filesystem::path& assetDirectory);

}
