#pragma once
#include <cstdint>
#include <vector>

// Upscaling of the U7 pixel art for the 3D view.
//
// Scale2x (AdvMAME2x) doubles the size and rounds diagonal edges instead of blurring them; flat
// areas and dithering stay as they are. Alpha takes part in the comparison, so sprite outlines
// are rounded as well. smoothEdges then softens the remaining steps of the outline: transparent
// texels take the colour of the nearest opaque ones (no dark fringes when filtered) and the
// alpha channel is blurred, so the outline cut at alpha 0.5 runs smoothly.
namespace PixelArtScale {

std::vector<std::uint8_t> scale2x(const std::uint8_t* rgba, int width, int height);
// Scale2x applied until factor (2, 4, 8 ...) is reached.
std::vector<std::uint8_t> scale(const std::uint8_t* rgba, int width, int height, int factor);
// Softens the alpha outline within radius texels (in place); no effect on opaque images.
void smoothEdges(std::vector<std::uint8_t>& rgba, int width, int height, int radius);

}
