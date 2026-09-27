#include "WoodMaterial.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <thread>

namespace WoodMaterial {
namespace {

constexpr float Pi = 3.14159265f;

float hash(int x, int y, int seed = 0) {
    std::uint32_t h = std::uint32_t(x) * 374761393u + std::uint32_t(y) * 668265263u + std::uint32_t(seed) * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return float((h ^ (h >> 16)) & 0xffffff) / float(0xffffff);
}

// Smooth value noise, periodic in x with period px and in y with period py (lattice cells).
float noise(float x, float y, int px, int py, int seed) {
    const int x0 = int(std::floor(x)), y0 = int(std::floor(y));
    const float fx = x - x0, fy = y - y0;
    const float sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
    auto h = [&](int i, int j) { return hash(((i % px) + px) % px, ((j % py) + py) % py, seed); };
    const float a = h(x0, y0), b = h(x0 + 1, y0), c = h(x0, y0 + 1), d = h(x0 + 1, y0 + 1);
    return (a + (b - a) * sx) + ((c + (d - c) * sx) - (a + (b - a) * sx)) * sy;
}

}

Images generate() {
    constexpr int n = Pixels;
    constexpr float metre = n / Size;                  // texels per metre
    constexpr int rows = int(Size / Board + 0.5f);     // boards per texture height
    std::vector<float> height(size_t(n) * n);
    Images images;
    images.size = n;
    images.albedo.resize(size_t(n) * n * 4);
    images.normal.resize(size_t(n) * n * 4);

    auto shadeRows = [&](int from, int to) {
        for (int y = from; y < to; ++y)
            for (int x = 0; x < n; ++x) {
                const float u = x / metre, v = y / metre;          // metres
                const int row = std::min(int(v / Board), rows - 1);
                const float across = v / Board - row;               // 0 top .. 1 bottom of the board
                // Long boards as in U7: 2 m (the texture repeats every 2 m), joints staggered per row.
                const float length = Size;
                const float offset = std::floor(hash(row, 3) * 8.f) * 0.25f;
                const float along = std::fmod(u + offset, length) / length;   // 0 .. 1 along the board
                const int board = row * 4 + int(std::fmod(u + offset, Size) / length);
                // Grain: long streaks along the board, warped a little.
                const float warp = noise(u * 3.f, v * 12.f, int(Size * 3), int(Size * 12), 11) * 2.f;
                const float grain = 0.5f + 0.5f * std::sin((v * 90.f + warp * 3.f + hash(board, 5) * 20.f) * Pi);
                const float fibre = noise(u * 40.f, v * 400.f, int(Size * 40), int(Size * 400), 13);
                // A knot on some boards.
                float knot = 0;
                if (hash(board, 17) < 0.3f) {
                    const float kx = hash(board, 19), ky = 0.3f + 0.4f * hash(board, 23);
                    const float dx = (along - kx) * length / 0.035f, dy = (across - ky) * Board / 0.022f;
                    knot = std::exp(-(dx * dx + dy * dy));
                }
                // Tone: U7's plank walls have clearly lighter and darker boards.
                const float tone = 0.82f + 0.3f * hash(board, 29);
                float value = tone * (0.86f + 0.14f * grain) * (0.93f + 0.07f * fibre) * (1.f - 0.45f * knot);
                // Relief: boards bulge slightly, grooves between rows and at the end joints.
                const float edge = std::min(across, 1.f - across) * Board;             // metres to the row groove
                const float joint = std::min(along, 1.f - along) * length;             // metres to the end joint
                float h = 0.005f * std::sqrt(std::max(0.f, std::sin(across * Pi)));    // 5 mm bulge
                h += 0.0006f * (grain - 0.5f) + 0.0004f * (fibre - 0.5f) - 0.0015f * knot;
                const float groove = std::min(edge, joint);
                if (groove < 0.01f) {
                    const float t = groove / 0.01f;
                    h -= 0.009f * (1.f - t * t);                                       // 9 mm deep groove
                    value *= 0.45f + 0.55f * t;
                }
                // The upper edge of each board catches light in U7's graphics: a faint bright rim.
                if (across > 0.03f && across < 0.09f) value *= 1.08f;
                height[size_t(y) * n + x] = h;
                // Neutral warm grey around 0.5; the view multiplies it with the wall's own colour.
                const float r = value * 0.52f, g = value * 0.5f, b = value * 0.47f;
                auto* out = images.albedo.data() + (size_t(y) * n + x) * 4;
                out[0] = std::uint8_t(std::clamp(r, 0.f, 1.f) * 255);
                out[1] = std::uint8_t(std::clamp(g, 0.f, 1.f) * 255);
                out[2] = std::uint8_t(std::clamp(b, 0.f, 1.f) * 255);
                out[3] = 255;
            }
    };
    auto normalRows = [&](int from, int to) {
        for (int y = from; y < to; ++y)
            for (int x = 0; x < n; ++x) {
                auto h = [&](int i, int j) { return height[size_t((j + n) % n) * n + size_t((i + n) % n)]; };
                // Slopes in metres per metre; x along u, y along v (down).
                const float dx = (h(x + 1, y) - h(x - 1, y)) * metre / 2, dy = (h(x, y + 1) - h(x, y - 1)) * metre / 2;
                const float length = std::sqrt(dx * dx + dy * dy + 1);
                auto* out = images.normal.data() + (size_t(y) * n + x) * 4;
                out[0] = std::uint8_t((-dx / length * 0.5f + 0.5f) * 255);
                out[1] = std::uint8_t((-dy / length * 0.5f + 0.5f) * 255);
                out[2] = std::uint8_t((1 / length * 0.5f + 0.5f) * 255);
                out[3] = 255;
            }
    };
    for (auto pass : {0, 1}) {
        std::vector<std::thread> threads;
        const int workers = int(std::max(1u, std::thread::hardware_concurrency()));
        for (int w = 0; w < workers; ++w) {
            const int from = n * w / workers, to = n * (w + 1) / workers;
            threads.emplace_back([&, from, to, pass] { pass == 0 ? shadeRows(from, to) : normalRows(from, to); });
        }
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
    const auto albedo = folder / "wood-planks.png", normal = folder / "wood-planks-normal.png";
    // A replaced colour image is used even without a matching normal map (then the boards are
    // lit flat); only missing files are generated, an existing file is never overwritten.
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
    // Colour image of its own without (or with a differently sized) normal map: flat relief.
    images.normal.assign(size_t(images.size) * images.size * 4, 0);
    for (size_t i = 0; i < images.normal.size(); i += 4) {
        images.normal[i] = 128; images.normal[i + 1] = 128; images.normal[i + 2] = 255; images.normal[i + 3] = 255;
    }
    return images;
}

}
