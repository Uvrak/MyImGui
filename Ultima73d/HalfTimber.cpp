#include "HalfTimber.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

namespace HalfTimber {
namespace {

float lum(const std::uint8_t* p) { return (p[0] * 0.3f + p[1] * 0.59f + p[2] * 0.11f) / 255.f; }
bool plaster(const std::uint8_t* p) { return lum(p) > 0.42f && p[0] >= p[2]; }
bool outline(const std::uint8_t* p) { return lum(p) < 0.12f; }

float hash(int x, int y, int seed) {
    std::uint32_t h = std::uint32_t(x) * 374761393u + std::uint32_t(y) * 668265263u + std::uint32_t(seed) * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return float((h ^ (h >> 16)) & 0xffff) / 65535.f;
}

float noise(float x, float y, int seed) {
    const int ix = int(std::floor(x)), iy = int(std::floor(y));
    const float fx = x - ix, fy = y - iy, sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
    const float a = hash(ix, iy, seed), b = hash(ix + 1, iy, seed), c = hash(ix, iy + 1, seed), d = hash(ix + 1, iy + 1, seed);
    return (a * (1 - sx) + b * sx) * (1 - sy) + (c * (1 - sx) + d * sx) * sy;
}

float fbm(float x, float y, int seed) {
    float sum = 0, amplitude = 0.5f;
    for (int o = 0; o < 4; ++o, x *= 2, y *= 2, amplitude *= 0.5f) sum += amplitude * noise(x, y, seed + o);
    return sum;
}

}

bool matches(const std::vector<std::uint8_t>& rgba) {
    int solid = 0, light = 0, timber = 0;
    for (size_t k = 0; k + 3 < rgba.size(); k += 4) {
        const auto* p = rgba.data() + k;
        if (!p[3]) continue;
        ++solid;
        if (plaster(p)) ++light;
        else if (!outline(p)) ++timber;
    }
    return solid > 0 && light * 10 >= solid * 3 && timber * 20 >= solid;
}

void render(std::vector<std::uint8_t>& texels, int width, int height, const std::vector<std::uint8_t>& frame) {
    glm::vec3 plasterSum(0), timberSum(0);
    float plasterLum = 0;
    int plasters = 0, timbers = 0;
    for (size_t k = 0; k + 3 < frame.size(); k += 4) {
        const auto* p = frame.data() + k;
        if (!p[3] || outline(p)) continue;
        const glm::vec3 c(p[0] / 255.f, p[1] / 255.f, p[2] / 255.f);
        if (plaster(p)) { plasterSum += c; plasterLum += lum(p); ++plasters; }
        else { timberSum += c; ++timbers; }
    }
    if (!plasters) return;
    const glm::vec3 plasterColour = plasterSum / float(plasters);
    const float meanLum = plasterLum / float(plasters);
    const glm::vec3 timberColour = timbers ? timberSum / float(timbers) : glm::vec3(0.3f, 0.2f, 0.12f);
    const float scale = 1.f / 24.f;          // noise features in texels
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            auto* p = texels.data() + (size_t(y) * width + x) * 4;
            if (!p[3]) continue;
            const float l = lum(p);
            // Plaster share, soft over the U7 edge between plaster and beam.
            const float share = std::clamp((l - 0.30f) / 0.15f, 0.f, 1.f) * (p[0] >= p[2] ? 1.f : 0.f);
            if (l < 0.1f) continue;                                          // U7's black outline stays
            // Lime render: large soft clouding, fine grain, sparse dark specks and a few pale spots.
            const float cloud = fbm(x * scale, y * scale, 11), grain = noise(x * 0.9f, y * 0.9f, 23);
            const float speck = hash(x, y, 37) > 0.992f ? 0.78f : hash(x, y, 41) > 0.994f ? 1.1f : 1.f;
            const float faceShade = std::clamp(l / std::max(meanLum, 0.05f), 0.6f, 1.25f);   // U7's lit and shaded faces
            const glm::vec3 render = plasterColour * (0.9f + 0.18f * cloud + 0.07f * grain) * speck * (0.4f + 0.6f * faceShade);
            // Oak: fibres (long, thin noise streaks) and darker grain lines.
            const float fibre = fbm(x * 0.08f, y * 0.6f, 51) * 0.6f + fbm(x * 0.6f, y * 0.08f, 57) * 0.4f;
            const float beamShade = std::clamp(l / 0.22f, 0.55f, 1.3f);
            const glm::vec3 oak = timberColour * (0.72f + 0.45f * fibre) * beamShade;
            const glm::vec3 c = glm::mix(oak, render, share);
            p[0] = std::uint8_t(std::clamp(c.r, 0.f, 1.f) * 255);
            p[1] = std::uint8_t(std::clamp(c.g, 0.f, 1.f) * 255);
            p[2] = std::uint8_t(std::clamp(c.b, 0.f, 1.f) * 255);
        }
}

}
