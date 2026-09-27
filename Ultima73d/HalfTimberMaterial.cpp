#include "HalfTimberMaterial.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>

namespace HalfTimberMaterial {
namespace {

float hash(int x, int y, int seed) {
    std::uint32_t h = std::uint32_t(x) * 374761393u + std::uint32_t(y) * 668265263u + std::uint32_t(seed) * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return float((h ^ (h >> 16)) & 0xffffff) / float(0xffffff);
}

// Value noise, periodic in x (period cells) so the texture tiles along the wall.
float noise(float x, float y, int period, int seed) {
    const int x0 = int(std::floor(x)), y0 = int(std::floor(y));
    const float fx = x - x0, fy = y - y0, sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
    auto h = [&](int i, int j) { return hash(((i % period) + period) % period, j, seed); };
    return (h(x0, y0) + (h(x0 + 1, y0) - h(x0, y0)) * sx) * (1 - sy) + (h(x0, y0 + 1) + (h(x0 + 1, y0 + 1) - h(x0, y0 + 1)) * sx) * sy;
}

float fbm(float x, float y, int period, int seed) {
    float sum = 0, amp = 0.5f;
    for (int i = 0; i < 4; ++i) { sum += amp * noise(x, y, period, seed + i); x *= 2; y *= 2; period *= 2; amp *= 0.5f; }
    return sum;
}

bool save(const std::filesystem::path& file, const std::vector<std::uint8_t>& rgba, int w, int h) {
    auto* image = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, const_cast<std::uint8_t*>(rgba.data()), w * 4);
    const bool saved = image && IMG_SavePNG(image, file.string().c_str());
    SDL_DestroySurface(image);
    return saved;
}

bool load(const std::filesystem::path& file, std::vector<std::uint8_t>& rgba, int& w, int& h) {
    auto* image = IMG_Load(file.string().c_str());
    if (!image) return false;
    auto* converted = SDL_ConvertSurface(image, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(image);
    if (!converted) return false;
    w = converted->w; h = converted->h;
    rgba.resize(size_t(w) * h * 4);
    for (int y = 0; y < h; ++y)
        std::copy_n(static_cast<const std::uint8_t*>(converted->pixels) + size_t(y) * converted->pitch, size_t(w) * 4,
                    rgba.begin() + size_t(y) * w * 4);
    SDL_DestroySurface(converted);
    return true;
}

// Distance (metres) from p to the segment a-b, and the position along it (0..1).
float segment(glm::vec2 p, glm::vec2 a, glm::vec2 b, float& along) {
    const glm::vec2 d = b - a;
    along = std::clamp(glm::dot(p - a, d) / glm::dot(d, d), 0.f, 1.f);
    return glm::length(p - (a + d * along));
}

}

Images generate(float storey, glm::vec3 plaster) {
    const int w = int(Width * PixelsPerMetre), h = int(storey * PixelsPerMetre);
    const float m = 1.f / PixelsPerMetre;
    const int period = int(Width * 8);                 // noise cells along the wall (8 a metre)
    std::vector<float> height(size_t(w) * h);
    std::vector<glm::vec3> colour(size_t(w) * h);
    const glm::vec3 oak(0.27f, 0.17f, 0.10f);
    // The timbers: (x metres along, y metres up) segments with their half width.
    struct Beam { glm::vec2 a, b; float half; };
    const float sill = 0.11f, plate = storey - 0.1f, rail = 1.12f, post = 0.085f;
    const Beam beams[] = {
        {{-1, sill}, {Width + 1, sill}, 0.11f},                       // sill
        {{-1, plate}, {Width + 1, plate}, 0.1f},                       // plate under the eaves
        {{-1, rail}, {Width + 1, rail}, 0.075f},                       // rail
        {{0, 0}, {0, storey}, post}, {{Width, 0}, {Width, storey}, post},   // posts at the seams
        {{0.12f, 0.22f}, {Width - 0.12f, rail - 0.07f}, 0.065f},        // brace, rising
        {{0.12f, plate - 0.1f}, {Width - 0.12f, rail + 0.07f}, 0.065f}, // brace, falling
    };
    for (int py = 0; py < h; ++py)
        for (int px = 0; px < w; ++px) {
            const glm::vec2 p((px + 0.5f) * m, storey - (py + 0.5f) * m);   // row 0 at the top
            // Nearest timber: how far inside it, and its direction for the grain.
            float inside = -1e9f, alongBeam = 0;
            glm::vec2 dir(1, 0);
            for (const auto& beam : beams) {
                float along;
                const float d = beam.half - segment(p, beam.a, beam.b, along);
                if (d > inside) { inside = d; dir = glm::normalize(beam.b - beam.a); alongBeam = along; }
            }
            const size_t i = size_t(py) * w + px;
            // Lime plaster: soft clouding, fine grain, specks, a little grime towards the sill.
            const float u = p.x * 8.f, v = p.y * 8.f;
            const float cloud = fbm(u * 0.5f, v * 0.5f, period / 2, 3), grain = noise(u * 12.f, v * 12.f, period * 12, 9);
            float tone = 0.9f + 0.16f * cloud + 0.06f * grain;
            if (hash(px, py, 17) > 0.993f) tone *= 0.8f;
            tone *= 1.f - 0.12f * std::clamp(1.f - (p.y - 0.22f) / 0.6f, 0.f, 1.f);
            glm::vec3 c = plaster * tone;
            float z = 0.0015f * cloud + 0.0006f * grain;
            if (inside > -0.004f) {
                // Oak: grain along the timber, darker edges, pegs where timbers meet.
                const glm::vec2 across(-dir.y, dir.x);
                const float s = glm::dot(p, dir) * 3.f, t = glm::dot(p, across) * 60.f;
                const float fibre = fbm(s, t, 1 << 20, 31) * 0.7f + 0.3f * noise(s * 8.f, t * 0.5f, 1 << 20, 37);
                const float edge = std::clamp(inside / 0.018f, 0.f, 1.f);
                glm::vec3 wood = oak * (0.72f + 0.5f * fibre) * (0.75f + 0.25f * edge);
                const float peg = std::min(glm::length(p - glm::vec2(0.04f, rail)), glm::length(p - glm::vec2(Width - 0.04f, rail)));
                if (peg < 0.012f) wood *= 0.6f;
                const float blend = std::clamp((inside + 0.004f) / 0.004f, 0.f, 1.f);
                c = glm::mix(c * 0.7f, wood, blend);                   // a dark line of shadow at the joint
                z = 0.015f * edge * blend + 0.0008f * fibre;
                (void)alongBeam;
            }
            colour[i] = c;
            height[i] = z;
        }
    Images images;
    images.width = w;
    images.height = h;
    images.albedo.resize(size_t(w) * h * 4);
    images.normal.resize(size_t(w) * h * 4);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const size_t i = size_t(y) * w + x;
            auto* a = images.albedo.data() + i * 4;
            for (int k = 0; k < 3; ++k) a[k] = std::uint8_t(std::clamp(colour[i][k], 0.f, 1.f) * 255);
            a[3] = 255;
            auto at = [&](int xx, int yy) { return height[size_t(std::clamp(yy, 0, h - 1)) * w + size_t((xx + w) % w)]; };
            const float dx = (at(x + 1, y) - at(x - 1, y)) * PixelsPerMetre / 2, dy = (at(x, y + 1) - at(x, y - 1)) * PixelsPerMetre / 2;
            const float length = std::sqrt(dx * dx + dy * dy + 1);
            auto* n = images.normal.data() + i * 4;
            n[0] = std::uint8_t((-dx / length * 0.5f + 0.5f) * 255);
            n[1] = std::uint8_t((-dy / length * 0.5f + 0.5f) * 255);
            n[2] = std::uint8_t((1 / length * 0.5f + 0.5f) * 255);
            n[3] = 255;
        }
    return images;
}

Images loadOrCreate(const std::filesystem::path& assetDirectory, float storey, glm::vec3 plaster) {
    const auto folder = assetDirectory / "Materials";
    const auto albedo = folder / "half-timber-hd.png", normal = folder / "half-timber-hd-normal.png";
    Images images;
    int nw = 0, nh = 0;
    if (load(albedo, images.albedo, images.width, images.height) && load(normal, images.normal, nw, nh) && nw == images.width && nh == images.height)
        return images;
    std::error_code error;
    std::filesystem::create_directories(folder, error);
    images = generate(storey, plaster);
    save(albedo, images.albedo, images.width, images.height);
    save(normal, images.normal, images.width, images.height);
    return images;
}

}
