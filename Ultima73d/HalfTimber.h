#pragma once
#include <cstdint>
#include <vector>

// High resolution surface for U7's light half-timbered ("sandstone") house walls: the frame's
// layout stays exactly as U7 drew it - which texels are plaster, which are timber - but the
// plaster becomes a fine lime render with grain, speckles and soft stains in U7's plaster
// colour, the timber dark oak with fibres, and U7's shading of the faces is kept.
namespace HalfTimber {

// True for a U7 wall frame (RGBA, original pixels) that is mostly light plaster with timber.
bool matches(const std::vector<std::uint8_t>& rgba);
// Re-renders an upscaled frame (RGBA, width x height) in place; frame: the original pixels,
// for the mean plaster and timber colours.
void render(std::vector<std::uint8_t>& texels, int width, int height, const std::vector<std::uint8_t>& frame);

}
