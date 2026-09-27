#pragma once
#include <cstdint>
#include <vector>

// High resolution renditions of U7's pictures and fabrics, close to the original:
//
//   painting  U7's tiny picture (without its painted frame) repainted as an oil painting:
//             smoothly enlarged, then covered with brush strokes in its own colours, on canvas
//             with a faint varnish.
//   weave     U7's pattern (tapestries, curtains, rugs) enlarged with its motifs kept crisp,
//             woven: warp and weft threads, a soft wool pile, optionally a fringe at the ends.
//
// All images RGBA; the source is the U7 picture's pixels (width x height).
namespace TextileArt {

struct Image { int width = 0, height = 0; std::vector<std::uint8_t> rgba; };

Image painting(const std::vector<std::uint8_t>& source, int width, int height, int outWidth, int outHeight, unsigned seed);
// fringe: 0 none, 1 at the bottom (tapestries), 2 at the two short ends (rugs).
Image weave(const std::vector<std::uint8_t>& source, int width, int height, int texelsPerPixel, int fringe);

}
