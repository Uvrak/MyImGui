#include "CobbleMaterial.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <thread>

namespace CobbleMaterial {
namespace {

float hash(int x, int y, int seed) {
    std::uint32_t h = std::uint32_t(x) * 374761393u + std::uint32_t(y) * 668265263u + std::uint32_t(seed) * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return float((h ^ (h >> 16)) & 0xffffff) / float(0xffffff);
}

float noise(float x, float y, int period, int seed) {
    const int x0 = int(std::floor(x)), y0 = int(std::floor(y));
    const float fx = x - x0, fy = y - y0, sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
    auto h = [&](int i, int j) { return hash(((i % period) + period) % period, ((j % period) + period) % period, seed); };
    return (h(x0, y0) + (h(x0 + 1, y0) - h(x0, y0)) * sx) * (1 - sy) + (h(x0, y0 + 1) + (h(x0 + 1, y0 + 1) - h(x0, y0 + 1)) * sx) * sy;
}

float fbm(float x, float y, int period, int seed) {
    float sum = 0, amp = 0.5f;
    for (int i = 0; i < 4; ++i) { sum += amp * noise(x, y, period, seed + i); x *= 2; y *= 2; period *= 2; amp *= 0.5f; }
    return sum;
}

bool save(const std::filesystem::path& file, const std::vector<std::uint8_t>& rgba, int size) {
    auto* image = SDL_CreateSurfaceFrom(size, size, SDL_PIXELFORMAT_RGBA32, const_cast<std::uint8_t*>(rgba.data()), size * 4);
    const bool saved = image && IMG_SavePNG(image, file.string().c_str());
    SDL_DestroySurface(image);
    return saved;
}

bool load(const std::filesystem::path& file, std::vector<std::uint8_t>& rgba, int& size) {
    auto* image = IMG_Load(file.string().c_str());
    if (!image) return false;
    auto* converted = SDL_ConvertSurface(image, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(image);
    if (!converted || converted->w != converted->h) { SDL_DestroySurface(converted); return false; }
    size = converted->w;
    rgba.resize(size_t(size) * size * 4);
    for (int y = 0; y < size; ++y)
        std::copy_n(static_cast<const std::uint8_t*>(converted->pixels) + size_t(y) * converted->pitch, size_t(size) * 4,
                    rgba.begin() + size_t(y) * size * 4);
    SDL_DestroySurface(converted);
    return true;
}

}

Images generate(const std::vector<glm::vec3>& source) {
    std::vector<glm::vec3> colours = source;
    if (colours.empty()) colours = {{.38f, .38f, .38f}, {.46f, .46f, .46f}, {.30f, .30f, .30f}, {.55f, .55f, .55f}};
    constexpr int n = Pixels;
    constexpr float metre = n / Size;
    constexpr int cells = 8;                                    // cobbles about 0.125 m
    const float cs = float(n) / cells;
    std::vector<float> height(size_t(n) * n);
    Images images;
    images.size = n;
    images.albedo.resize(size_t(n) * n * 4);
    images.normal.resize(size_t(n) * n * 4);
    auto centre = [&](int cx, int cy) {
        const int wx = ((cx % cells) + cells) % cells, wy = ((cy % cells) + cells) % cells;
        return glm::vec2((cx + 0.2f + 0.6f * hash(wx, wy, 1)) * cs, (cy + 0.2f + 0.6f * hash(wx, wy, 2)) * cs);
    };
    auto shade = [&](int from, int to) {
        for (int y = from; y < to; ++y)
            for (int x = 0; x < n; ++x) {
                const float u = x / metre, v = y / metre;
                const glm::vec2 p = glm::vec2(x + 0.5f, y + 0.5f) +
                                    glm::vec2(fbm(u * 12, v * 12, int(Size * 12), 3) - 0.5f, fbm(u * 12, v * 12, int(Size * 12), 5) - 0.5f) * (cs * 0.3f);
                float d1 = 1e9f, d2 = 1e9f;
                glm::vec2 nearest(0);
                int best = 0;
                const int gx = int(std::floor(p.x / cs)), gy = int(std::floor(p.y / cs));
                for (int j = -2; j <= 2; ++j)
                    for (int i = -2; i <= 2; ++i) {
                        const glm::vec2 c = centre(gx + i, gy + j);
                        const glm::vec2 d = (p - c) / cs;
                        const float dist = glm::length(d);
                        if (dist < d1) {
                            d2 = d1; d1 = dist; nearest = d;
                            best = ((((gy + j) % cells) + cells) % cells) * cells + ((((gx + i) % cells) + cells) % cells);
                        } else if (dist < d2) d2 = dist;
                    }
                const float edge = (d2 - d1) * 0.5f;
                const float gap = 0.07f + 0.03f * fbm(u * 30, v * 30, int(Size * 30), 7);
                glm::vec3 colour;
                float h;
                if (edge < gap) {
                    // Dark sandy gaps.
                    colour = glm::vec3(.17f, .15f, .12f) * (0.8f + 0.5f * hash(x / 2, y / 2, 9));
                    h = -0.006f;
                } else {
                    const float inside = std::min(1.f, (edge - gap) / 0.25f);
                    colour = colours[size_t(hash(best, 4, 11) * colours.size()) % colours.size()];
                    colour *= 0.88f + 0.22f * fbm(u * 70, v * 70, int(Size * 70), best);
                    // Rounded: lit upper left, darker lower right, as U7's street pixels.
                    const float rim = 1.f - inside;
                    const float toward = glm::dot(glm::normalize(nearest + glm::vec2(1e-5f)), glm::normalize(glm::vec2(-1, -1)));
                    colour *= 1.f + rim * 0.4f * toward;
                    h = 0.012f * std::sqrt(inside);
                }
                height[size_t(y) * n + x] = h;
                auto* a = images.albedo.data() + (size_t(y) * n + x) * 4;
                a[0] = std::uint8_t(std::clamp(colour.r, 0.f, 1.f) * 255);
                a[1] = std::uint8_t(std::clamp(colour.g, 0.f, 1.f) * 255);
                a[2] = std::uint8_t(std::clamp(colour.b, 0.f, 1.f) * 255);
                a[3] = 255;
            }
    };
    auto normals = [&](int from, int to) {
        for (int y = from; y < to; ++y)
            for (int x = 0; x < n; ++x) {
                auto hh = [&](int i, int j) { return height[size_t((j + n) % n) * n + size_t((i + n) % n)]; };
                const float dx = (hh(x + 1, y) - hh(x - 1, y)) * metre / 2, dy = (hh(x, y + 1) - hh(x, y - 1)) * metre / 2;
                const float length = std::sqrt(dx * dx + dy * dy + 1);
                auto* out = images.normal.data() + (size_t(y) * n + x) * 4;
                out[0] = std::uint8_t((-dx / length * 0.5f + 0.5f) * 255);
                out[1] = std::uint8_t((-dy / length * 0.5f + 0.5f) * 255);
                out[2] = std::uint8_t((1 / length * 0.5f + 0.5f) * 255);
                out[3] = 255;
            }
    };
    for (int pass = 0; pass < 2; ++pass) {
        std::vector<std::thread> threads;
        const int workers = int(std::max(1u, std::thread::hardware_concurrency()));
        for (int w = 0; w < workers; ++w)
            threads.emplace_back([&, w, pass] { const int f = n * w / workers, t = n * (w + 1) / workers; pass == 0 ? shade(f, t) : normals(f, t); });
        for (auto& t : threads) t.join();
    }
    return images;
}

Images loadOrCreate(const std::filesystem::path& assetDirectory, const std::vector<glm::vec3>& colours) {
    const auto folder = assetDirectory / "Materials";
    const auto albedo = folder / "cobble-hd.png", normal = folder / "cobble-hd-normal.png";
    Images images;
    int normalSize = 0;
    const bool haveAlbedo = load(albedo, images.albedo, images.size);
    const bool haveNormal = load(normal, images.normal, normalSize);
    if (haveAlbedo && haveNormal && normalSize == images.size) return images;
    std::error_code error;
    std::filesystem::create_directories(folder, error);
    if (!haveAlbedo) {
        const auto generated = generate(colours);
        save(albedo, generated.albedo, generated.size);
        if (!haveNormal) save(normal, generated.normal, generated.size);
        return generated;
    }
    images.normal.assign(images.albedo.size(), 0);
    for (size_t i = 0; i < images.normal.size(); i += 4) {
        images.normal[i] = 128; images.normal[i + 1] = 128; images.normal[i + 2] = 255; images.normal[i + 3] = 255;
    }
    return images;
}

}
