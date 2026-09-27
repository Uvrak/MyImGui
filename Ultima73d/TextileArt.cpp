#include "TextileArt.h"
#include "PixelArtScale.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

namespace TextileArt {
namespace {

float hash(int x, int y, unsigned seed) {
    std::uint32_t h = std::uint32_t(x) * 374761393u + std::uint32_t(y) * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return float((h ^ (h >> 16)) & 0xffff) / 65535.f;
}

float noise(float x, float y, unsigned seed) {
    const int ix = int(std::floor(x)), iy = int(std::floor(y));
    const float fx = x - ix, fy = y - iy, sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
    return (hash(ix, iy, seed) * (1 - sx) + hash(ix + 1, iy, seed) * sx) * (1 - sy) + (hash(ix, iy + 1, seed) * (1 - sx) + hash(ix + 1, iy + 1, seed) * sx) * sy;
}

// Bilinear sample of the source (clamped), colour 0..1.
glm::vec3 sample(const std::vector<std::uint8_t>& s, int w, int h, float x, float y) {
    x = std::clamp(x - 0.5f, 0.f, float(w - 1)); y = std::clamp(y - 0.5f, 0.f, float(h - 1));
    const int x0 = int(x), y0 = int(y), x1 = std::min(x0 + 1, w - 1), y1 = std::min(y0 + 1, h - 1);
    const float fx = x - x0, fy = y - y0;
    auto at = [&](int a, int b) { const auto* p = s.data() + (size_t(b) * w + a) * 4; return glm::vec3(p[0], p[1], p[2]) / 255.f; };
    return glm::mix(glm::mix(at(x0, y0), at(x1, y0), fx), glm::mix(at(x0, y1), at(x1, y1), fx), fy);
}

}

Image painting(const std::vector<std::uint8_t>& source, int width, int height, int outWidth, int outHeight, unsigned seed) {
    // The picture inside U7's painted frame: the bounding box of the opaque pixels, less its
    // outer two pixels (the frame and its outline) where the picture is big enough.
    int left = width, right = -1, top = height, bottom = -1;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            if (source[(size_t(y) * width + x) * 4 + 3]) { left = std::min(left, x); right = std::max(right, x); top = std::min(top, y); bottom = std::max(bottom, y); }
    Image out;
    out.width = outWidth; out.height = outHeight;
    out.rgba.assign(size_t(outWidth) * outHeight * 4, 255);
    if (right < left) return out;
    const int inset = (right - left > 8 && bottom - top > 5) ? 2 : (right - left > 4 && bottom - top > 3) ? 1 : 0;
    left += inset; right -= inset; top += inset; bottom -= inset;
    const int cw = right - left + 1, ch = bottom - top + 1;
    std::vector<std::uint8_t> crop(size_t(cw) * ch * 4);
    for (int y = 0; y < ch; ++y)
        for (int x = 0; x < cw; ++x) {
            const auto* p = source.data() + (size_t(y + top) * width + x + left) * 4;
            auto* q = crop.data() + (size_t(y) * cw + x) * 4;
            // Transparent gaps take the neighbour's colour.
            if (p[3]) std::copy_n(p, 4, q);
            else { q[0] = q[1] = q[2] = 90; q[3] = 255; }
        }
    // Under-painting: the picture smoothly enlarged.
    std::vector<glm::vec3> base(size_t(outWidth) * outHeight);
    for (int y = 0; y < outHeight; ++y)
        for (int x = 0; x < outWidth; ++x)
            base[size_t(y) * outWidth + x] = sample(crop, cw, ch, (x + 0.5f) * cw / outWidth, (y + 0.5f) * ch / outHeight);
    std::vector<glm::vec3> paint(base);
    // Brush strokes: short soft dabs in the local colour, following the picture's contours,
    // coarse ones first, then finer.
    std::uint32_t state = seed * 2654435761u + 17;
    auto random = [&] { state = state * 1664525u + 1013904223u; return float((state >> 8) & 0xffff) / 65535.f; };
    const float cell = float(outWidth) / cw;
    for (const float size : {cell * 0.9f, cell * 0.5f, cell * 0.28f}) {
        const int strokes = int(float(outWidth) * outHeight / (size * size) * 1.6f);
        for (int i = 0; i < strokes; ++i) {
            const float cx = random() * outWidth, cy = random() * outHeight;
            const auto at = [&](float x, float y) { return base[size_t(std::clamp(int(y), 0, outHeight - 1)) * outWidth + std::clamp(int(x), 0, outWidth - 1)]; };
            // Stroke direction along the contour (perpendicular to the colour gradient).
            const glm::vec3 gx = at(cx + 2, cy) - at(cx - 2, cy), gy = at(cx, cy + 2) - at(cx, cy - 2);
            glm::vec2 dir(-(gy.r + gy.g + gy.b), gx.r + gx.g + gx.b);
            dir = glm::length(dir) > 1e-3f ? glm::normalize(dir) : glm::vec2(std::cos(random() * 6.283f), std::sin(random() * 6.283f));
            const glm::vec3 colour = at(cx, cy) * (0.93f + 0.14f * random());
            const float length = size * (1.2f + random()), breadth = size * 0.35f;
            const int reach = int(length) + 2;
            for (int dy = -reach; dy <= reach; ++dy)
                for (int dx = -reach; dx <= reach; ++dx) {
                    const int x = int(cx) + dx, y = int(cy) + dy;
                    if (x < 0 || y < 0 || x >= outWidth || y >= outHeight) continue;
                    const float a = (dx * dir.x + dy * dir.y) / length, b = (-dx * dir.y + dy * dir.x) / breadth;
                    const float d = a * a + b * b;
                    if (d > 1) continue;
                    // Bristle lines along the stroke.
                    const float bristle = 0.85f + 0.3f * hash(int(b * 6 + 20), i, seed);
                    const float cover = (1 - d) * 0.8f;
                    auto& p = paint[size_t(y) * outWidth + x];
                    p = glm::mix(p, colour * bristle, cover);
                }
        }
    }
    // Canvas weave and a warm varnish, darker at the edges.
    for (int y = 0; y < outHeight; ++y)
        for (int x = 0; x < outWidth; ++x) {
            const float weave = 0.95f + 0.05f * (((x / 2) + (y / 2)) % 2 ? 1.f : -1.f) * noise(x * 0.5f, y * 0.5f, seed + 3);
            const float ex = std::min(x, outWidth - 1 - x) / float(outWidth), ey = std::min(y, outHeight - 1 - y) / float(outHeight);
            const float vignette = 0.82f + 0.18f * std::min(1.f, std::min(ex, ey) * 8.f);
            glm::vec3 c = paint[size_t(y) * outWidth + x] * weave * vignette * glm::vec3(1.03f, 1.0f, 0.9f);
            auto* q = out.rgba.data() + (size_t(y) * outWidth + x) * 4;
            q[0] = std::uint8_t(std::clamp(c.r, 0.f, 1.f) * 255);
            q[1] = std::uint8_t(std::clamp(c.g, 0.f, 1.f) * 255);
            q[2] = std::uint8_t(std::clamp(c.b, 0.f, 1.f) * 255);
            q[3] = 255;
        }
    return out;
}

Image weave(const std::vector<std::uint8_t>& source, int width, int height, int texelsPerPixel, int fringe) {
    auto big = PixelArtScale::scale(source.data(), width, height, texelsPerPixel);
    const int w = width * texelsPerPixel, h = height * texelsPerPixel;
    PixelArtScale::smoothEdges(big, w, h, texelsPerPixel / 2);
    const int fringeLength = fringe ? std::max(4, texelsPerPixel * 2) : 0;
    Image out;
    out.width = w;
    out.height = h + (fringe == 1 ? fringeLength : 0);
    out.rgba.assign(size_t(out.width) * out.height * 4, 0);
    constexpr int thread = 3;                      // texels per thread
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const auto* p = big.data() + (size_t(y) * w + x) * 4;
            auto* q = out.rgba.data() + (size_t(y) * out.width + x) * 4;
            if (!p[3]) continue;
            // Warp and weft: over-under threads, each with its own twist; a soft pile.
            const bool over = ((x / thread) + (y / thread)) % 2 == 0;
            const float along = over ? float(x % thread) / thread : float(y % thread) / thread;
            const float round = 1.f - 0.35f * (along - 0.5f) * (along - 0.5f) * 4.f;
            const float twist = 0.93f + 0.07f * std::sin(((over ? y : x) + (over ? x / thread : y / thread) * 1.7f) * 1.3f);
            const float pile = 0.9f + 0.2f * noise(x * 0.35f, y * 0.35f, 5);
            const float shade = round * twist * pile * (over ? 1.03f : 0.95f);
            for (int k = 0; k < 3; ++k) q[k] = std::uint8_t(std::clamp(p[k] * shade, 0.f, 255.f));
            q[3] = 255;
        }
    if (fringe == 1) {
        // Tassels below the bottom edge: the edge colour, in twisted strands.
        for (int x = 0; x < w; ++x) {
            const int strand = x / 3;
            if (strand % 2) continue;
            const int length = int(fringeLength * (0.7f + 0.3f * hash(strand, 1, 9)));
            const auto* edge = big.data() + (size_t(h - 1) * w + x) * 4;
            if (!edge[3]) continue;
            for (int y = 0; y < length; ++y) {
                auto* q = out.rgba.data() + (size_t(h + y) * out.width + x) * 4;
                const float shade = 0.8f - 0.3f * float(y) / length;
                for (int k = 0; k < 3; ++k) q[k] = std::uint8_t(edge[k] * shade);
                q[3] = 255;
            }
        }
    } else if (fringe == 2) {
        // Rugs: the first and last few texels of the long ends become light tassels.
        for (int y = 0; y < out.height; ++y)
            for (int x = 0; x < w; ++x) {
                const bool end = w >= out.height ? (x < fringeLength || x >= w - fringeLength) : (y < fringeLength || y >= out.height - fringeLength);
                if (!end) continue;
                auto* q = out.rgba.data() + (size_t(y) * out.width + x) * 4;
                const int strand = (w >= out.height ? y : x) / 3;
                if (strand % 2) { q[3] = 0; continue; }
                q[0] = 205; q[1] = 190; q[2] = 160; q[3] = q[3] ? 255 : 0;
            }
    }
    return out;
}

}
