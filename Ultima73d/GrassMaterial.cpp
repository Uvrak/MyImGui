#include "GrassMaterial.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <thread>

namespace GrassMaterial {
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
    if (colours.empty()) colours = {{.24f, .40f, .14f}, {.30f, .48f, .18f}, {.20f, .33f, .12f}};
    glm::vec3 mean(0);
    for (const auto& c : colours) mean += c;
    mean /= float(colours.size());
    constexpr int n = Pixels;
    constexpr float metre = n / Size;
    std::vector<float> height(size_t(n) * n, 0.f);
    std::vector<glm::vec3> colour(size_t(n) * n);
    // Ground between the blades: dark U7 green, mottled.
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x) {
            const float u = x / metre, v = y / metre;
            colour[size_t(y) * n + x] = mean * (0.55f + 0.25f * fbm(u * 6, v * 6, int(Size * 6), 3));
        }
    // Blades: short strokes, leaning, lighter towards the tip; drawn back to front by height.
    std::uint32_t state = 777;
    auto random = [&] { state = state * 1664525u + 1013904223u; return float((state >> 8) & 0xffff) / 65535.f; };
    const int blades = n * n / 55;
    for (int i = 0; i < blades; ++i) {
        const float bx = random() * n, by = random() * n;
        const float length = (0.02f + 0.03f * random()) * metre, lean = (random() - 0.5f) * 1.2f;
        const glm::vec3 base = colours[size_t(random() * colours.size()) % colours.size()] * (0.8f + 0.35f * random());
        const float tuft = fbm(bx / metre * 5, by / metre * 5, int(Size * 5), 9);      // denser, brighter tufts
        for (int k = 0; k < int(length); ++k) {
            const float t = k / length;
            const int px = ((int(bx + lean * k) % n) + n) % n, py = ((int(by - k) % n) + n) % n;
            const float h = 0.004f + 0.01f * t * (0.6f + tuft);
            for (int w = 0; w <= (t < 0.6f ? 1 : 0); ++w) {
                const size_t idx = size_t(py) * n + size_t((px + w) % n);
                if (h < height[idx]) continue;
                height[idx] = h;
                colour[idx] = base * (0.75f + 0.55f * t) * (0.85f + 0.3f * tuft);
            }
        }
    }
    // A few clover leaves and dry spots.
    for (int i = 0; i < 90; ++i) {
        const float cx = random() * n, cy = random() * n, r = (0.008f + 0.01f * random()) * metre;
        const bool dry = random() < 0.3f;
        for (int dy = -int(r); dy <= int(r); ++dy)
            for (int dx = -int(r); dx <= int(r); ++dx) {
                if (dx * dx + dy * dy > r * r) continue;
                const size_t idx = size_t(((int(cy) + dy) % n + n) % n) * n + size_t(((int(cx) + dx) % n + n) % n);
                colour[idx] = dry ? glm::vec3(.45f, .40f, .22f) : mean * 1.25f;
                height[idx] = std::max(height[idx], 0.012f);
            }
    }
    Images images;
    images.size = n;
    images.albedo.resize(size_t(n) * n * 4);
    images.normal.resize(size_t(n) * n * 4);
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x) {
            const size_t i = size_t(y) * n + x;
            auto* a = images.albedo.data() + i * 4;
            a[0] = std::uint8_t(std::clamp(colour[i].r, 0.f, 1.f) * 255);
            a[1] = std::uint8_t(std::clamp(colour[i].g, 0.f, 1.f) * 255);
            a[2] = std::uint8_t(std::clamp(colour[i].b, 0.f, 1.f) * 255);
            a[3] = 255;
            auto hh = [&](int i2, int j) { return height[size_t((j + n) % n) * n + size_t((i2 + n) % n)]; };
            const float dx = (hh(x + 1, y) - hh(x - 1, y)) * metre / 2, dy = (hh(x, y + 1) - hh(x, y - 1)) * metre / 2;
            const float length = std::sqrt(dx * dx + dy * dy + 1);
            auto* out = images.normal.data() + i * 4;
            out[0] = std::uint8_t((-dx / length * 0.5f + 0.5f) * 255);
            out[1] = std::uint8_t((-dy / length * 0.5f + 0.5f) * 255);
            out[2] = std::uint8_t((1 / length * 0.5f + 0.5f) * 255);
            out[3] = 255;
        }
    return images;
}

Images loadOrCreate(const std::filesystem::path& assetDirectory, const std::vector<glm::vec3>& colours) {
    const auto folder = assetDirectory / "Materials";
    const auto albedo = folder / "grass-hd.png", normal = folder / "grass-hd-normal.png";
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
