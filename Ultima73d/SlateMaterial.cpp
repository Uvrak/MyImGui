#include "SlateMaterial.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <thread>

namespace SlateMaterial {
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
    const float a = h(x0, y0), b = h(x0 + 1, y0), c = h(x0, y0 + 1), d = h(x0 + 1, y0 + 1);
    return (a + (b - a) * sx) * (1 - sy) + (c + (d - c) * sx) * sy;
}

float fbm(float x, float y, int period, int seed) {
    float sum = 0, amp = 0.5f;
    for (int i = 0; i < 4; ++i) { sum += amp * noise(x, y, period, seed + i); x *= 2; y *= 2; period *= 2; amp *= 0.5f; }
    return sum;
}

}

Images generate() {
    constexpr int n = Pixels;
    constexpr float metre = n / Size;
    // Slates 0.25 m wide, 0.2 m of each showing, 4 per texture across and 5 rows down.
    constexpr int across = 4, rows = 5;
    const float sw = float(n) / across, sh = float(n) / rows;
    std::vector<float> height(size_t(n) * n);
    Images images;
    images.size = n;
    images.albedo.resize(size_t(n) * n * 4);
    images.normal.resize(size_t(n) * n * 4);
    // U7 slate: dark blue-greys.
    const glm::vec3 palette[] = {{.24f, .26f, .30f}, {.30f, .32f, .36f}, {.36f, .38f, .42f}, {.20f, .22f, .26f}, {.33f, .33f, .35f}};
    auto shade = [&](int from, int to) {
        for (int y = from; y < to; ++y)
            for (int x = 0; x < n; ++x) {
                const int row = y / int(sh);
                const float offset = (row & 1) ? sw * 0.5f : 0.f;
                const float fx = std::fmod(x + offset, float(n));
                const int slate = int(fx / sw);
                const float ax = std::fmod(fx, sw) / sw, ay = float(y % int(sh)) / sh;   // 0..1 inside the showing part
                const int id = row * across + slate;
                const float u = x / metre, v = y / metre;
                const float grain = fbm(u * 30, v * 6, int(Size * 30), 3);
                float value = 0.85f + 0.25f * grain + 0.10f * (hash(id, 1, 4) - 0.5f);
                glm::vec3 colour = palette[int(hash(id, 2, 7) * 5) % 5];
                // Rising towards the lower edge (the slate lies on the one below), a step at the edge.
                float h = 0.004f + 0.006f * ay;
                // Gaps between slates of a row, and the shadow line under each row's lower edge.
                const float side = std::min(ax, 1.f - ax) * sw / metre;           // metres to the side gap
                if (side < 0.004f) { h = 0.001f; value *= 0.35f; }
                if (ay < 0.08f) { value *= 0.45f + 0.55f * (ay / 0.08f); h -= 0.004f * (1 - ay / 0.08f); }
                // Chipped lower corners.
                if (ay > 0.9f && (ax < 0.06f || ax > 0.94f) && hash(id, 5, 1) > 0.5f) { value *= 0.4f; h = 0.001f; }
                // Lichen specks.
                if (hash(x / 4, y / 4, 11) > 0.985f) colour = glm::mix(colour, glm::vec3(.45f, .48f, .32f), 0.6f);
                height[size_t(y) * n + x] = h;
                auto* a = images.albedo.data() + (size_t(y) * n + x) * 4;
                a[0] = std::uint8_t(std::clamp(colour.r * value, 0.f, 1.f) * 255);
                a[1] = std::uint8_t(std::clamp(colour.g * value, 0.f, 1.f) * 255);
                a[2] = std::uint8_t(std::clamp(colour.b * value, 0.f, 1.f) * 255);
                a[3] = 255;
            }
    };
    auto normals = [&](int from, int to) {
        for (int y = from; y < to; ++y)
            for (int x = 0; x < n; ++x) {
                auto h = [&](int i, int j) { return height[size_t((j + n) % n) * n + size_t((i + n) % n)]; };
                const float dx = (h(x + 1, y) - h(x - 1, y)) * metre / 2, dy = (h(x, y + 1) - h(x, y - 1)) * metre / 2;
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
            threads.emplace_back([&, w, pass] { const int from = n * w / workers, to = n * (w + 1) / workers; pass == 0 ? shade(from, to) : normals(from, to); });
        for (auto& t : threads) t.join();
    }
    return images;
}

namespace {

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

Images loadOrCreate(const std::filesystem::path& assetDirectory) {
    const auto folder = assetDirectory / "Materials";
    const auto albedo = folder / "slate-roof.png", normal = folder / "slate-roof-normal.png";
    Images images;
    int normalSize = 0;
    const bool haveAlbedo = load(albedo, images.albedo, images.size);
    const bool haveNormal = load(normal, images.normal, normalSize);
    if (haveAlbedo && haveNormal && normalSize == images.size) return images;
    std::error_code error;
    std::filesystem::create_directories(folder, error);
    if (!haveAlbedo) {
        const auto generated = generate();
        save(albedo, generated.albedo, generated.size);
        if (!haveNormal) save(normal, generated.normal, generated.size);
        return generated;
    }
    images.normal.assign(size_t(images.size) * images.size * 4, 0);
    for (size_t i = 0; i < images.normal.size(); i += 4) {
        images.normal[i] = 128; images.normal[i + 1] = 128; images.normal[i + 2] = 255; images.normal[i + 3] = 255;
    }
    return images;
}

}
