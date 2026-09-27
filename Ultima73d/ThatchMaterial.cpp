#include "ThatchMaterial.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <thread>

namespace ThatchMaterial {
namespace {

std::uint32_t mix(std::uint32_t h) {
    h ^= h >> 16; h *= 0x7feb352du; h ^= h >> 15; h *= 0x846ca68bu; h ^= h >> 16;
    return h;
}
float random(std::uint32_t& state) { state = mix(state + 0x9e3779b9u); return float(state & 0xffffff) / float(0xffffff); }

}

Images generate() {
    constexpr int n = Pixels;
    constexpr float metre = n / Size;
    const int courses = int(Size / Course + 0.5f);
    const int courseTexels = n / courses;
    std::vector<float> height(size_t(n) * n, 0.f);
    std::vector<float> red(size_t(n) * n), green(size_t(n) * n), blue(size_t(n) * n);
    // Underlying straw: the dark gaps between strands.
    for (size_t i = 0; i < height.size(); ++i) { red[i] = 0.20f; green[i] = 0.14f; blue[i] = 0.07f; }

    // Strands, drawn from the top of the texture downwards; each course lies over the one above
    // it (a course's strands end at its lower edge, where the next course starts), so strands are
    // drawn course by course and a strand only raises the surface where it is on top.
    std::uint32_t state = 12345;
    const int strandsPerCourse = 5200;
    for (int course = 0; course < courses; ++course) {
        const int courseTop = course * courseTexels;
        for (int s = 0; s < strandsPerCourse; ++s) {
            const float x0 = random(state) * n;
            // Strands start somewhere above and end at or just above the course's lower edge.
            const float end = courseTop + courseTexels - random(state) * random(state) * 0.35f * courseTexels;
            const float length = (0.6f + 0.9f * random(state)) * courseTexels;
            const float start = end - length;
            const float lean = (random(state) - 0.5f) * 0.12f;   // slight slant
            const float width = 1.2f + 2.2f * random(state);
            // Straw colours: golden, pale, weathered grey-brown and dark.
            const float pick = random(state), light = 0.75f + 0.35f * random(state);
            float r, g, b;
            if (pick < 0.40f) { r = .70f; g = .53f; b = .24f; }
            else if (pick < 0.62f) { r = .80f; g = .66f; b = .36f; }
            else if (pick < 0.88f) { r = .52f; g = .41f; b = .25f; }
            else { r = .36f; g = .27f; b = .15f; }
            r *= light; g *= light; b *= light;
            for (int y = int(start); y <= int(end); ++y) {
                const float t = (y - start) / length;                          // 0 top .. 1 cut end
                const float cx = x0 + lean * (y - start);
                // Raised towards the bundle's lower part (the bulge of the course).
                const float lift = 0.004f + 0.008f * t;
                const float shade = 0.72f + 0.28f * t;                        // upper parts lie under the course above
                for (int dx = -int(width); dx <= int(width); ++dx) {
                    const float across = std::abs(dx + (cx - std::floor(cx))) / width;
                    if (across > 1.f) continue;
                    const float round = std::sqrt(1.f - across * across);   // round strand profile
                    const int px = ((int(std::floor(cx)) + dx) % n + n) % n, py = (y % n + n) % n;
                    const size_t i = size_t(py) * n + px;
                    const float h = lift + 0.0015f * round;
                    if (h < height[i]) continue;
                    height[i] = h;
                    const float l = shade * (0.8f + 0.2f * round);
                    red[i] = r * l; green[i] = g * l; blue[i] = b * l;
                }
            }
            // The cut end: a slightly darker, blunt tip.
            const int py = ((int(end)) % n + n) % n;
            const int px = ((int(x0 + lean * length)) % n + n) % n;
            const size_t i = size_t(py) * n + px;
            red[i] *= 0.7f; green[i] *= 0.7f; blue[i] *= 0.7f;
        }
    }

    Images images;
    images.size = n;
    images.albedo.resize(size_t(n) * n * 4);
    images.normal.resize(size_t(n) * n * 4);
    auto rows = [&](int from, int to) {
        for (int y = from; y < to; ++y)
            for (int x = 0; x < n; ++x) {
                const size_t i = size_t(y) * n + x;
                auto* a = images.albedo.data() + i * 4;
                a[0] = std::uint8_t(std::clamp(red[i], 0.f, 1.f) * 255);
                a[1] = std::uint8_t(std::clamp(green[i], 0.f, 1.f) * 255);
                a[2] = std::uint8_t(std::clamp(blue[i], 0.f, 1.f) * 255);
                a[3] = 255;
                auto h = [&](int px, int py) { return height[size_t((py + n) % n) * n + size_t((px + n) % n)]; };
                const float dx = (h(x + 1, y) - h(x - 1, y)) * metre / 2, dy = (h(x, y + 1) - h(x, y - 1)) * metre / 2;
                const float length = std::sqrt(dx * dx + dy * dy + 1);
                auto* out = images.normal.data() + i * 4;
                out[0] = std::uint8_t((-dx / length * 0.5f + 0.5f) * 255);
                out[1] = std::uint8_t((-dy / length * 0.5f + 0.5f) * 255);
                out[2] = std::uint8_t((1 / length * 0.5f + 0.5f) * 255);
                out[3] = 255;
            }
    };
    std::vector<std::thread> threads;
    const int workers = int(std::max(1u, std::thread::hardware_concurrency()));
    for (int w = 0; w < workers; ++w) threads.emplace_back(rows, n * w / workers, n * (w + 1) / workers);
    for (auto& t : threads) t.join();
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
    const auto albedo = folder / "thatch.png", normal = folder / "thatch-normal.png";
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
    // A colour image of its own without a matching normal map: lit flat.
    images.normal.assign(size_t(images.size) * images.size * 4, 0);
    for (size_t i = 0; i < images.normal.size(); i += 4) {
        images.normal[i] = 128; images.normal[i + 1] = 128; images.normal[i + 2] = 255; images.normal[i + 3] = 255;
    }
    return images;
}

}
