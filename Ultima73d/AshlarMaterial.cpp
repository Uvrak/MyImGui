#include "AshlarMaterial.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <thread>

namespace AshlarMaterial {
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

}

Images generate(const U7::Data& data) {
    // U7's fortress colours: the lit pixels of its town wall block.
    std::vector<glm::vec3> colours;
    const auto frame = data.frame(192, 1);
    for (size_t i = 0; i + 3 < frame.rgba.size(); i += 4) {
        if (!frame.rgba[i + 3]) continue;
        const glm::vec3 c(frame.rgba[i] / 255.f, frame.rgba[i + 1] / 255.f, frame.rgba[i + 2] / 255.f);
        if (c.r * 0.3f + c.g * 0.59f + c.b * 0.11f > 0.35f) colours.push_back(c);
    }
    if (colours.empty()) colours = {{.62f, .55f, .52f}, {.70f, .58f, .54f}, {.55f, .55f, .56f}};

    constexpr int n = Pixels;
    constexpr float metre = n / Size;
    constexpr int rows = 4;                                   // courses of 0.5 m
    const float rowH = float(n) / rows;
    // Block joints per course: lengths 0.5 - 1.1 m, scaled to fill the texture exactly; each
    // course starts at another offset, so the joints do not line up.
    std::vector<std::vector<float>> joints(rows);
    for (int r = 0; r < rows; ++r) {
        std::vector<float> lengths;
        float sum = 0;
        for (int k = 0; sum < Size; ++k) { const float l = 0.5f + 0.6f * hash(r, k, 3); lengths.push_back(l); sum += l; }
        float at = hash(r, 99, 5) * n;
        for (float l : lengths) { joints[r].push_back(at); at += l / sum * n; }
    }
    std::vector<float> height(size_t(n) * n);
    Images images;
    images.width = images.height = n;
    images.albedo.resize(size_t(n) * n * 4);
    images.normal.resize(size_t(n) * n * 4);
    auto shade = [&](int from, int to) {
        for (int y = from; y < to; ++y)
            for (int x = 0; x < n; ++x) {
                const int r = std::min(rows - 1, int(y / rowH));
                const float inRow = y - r * rowH;
                const auto& j = joints[r];
                int block = 0;
                float left = 0, right = 0;
                for (size_t k = 0; k < j.size(); ++k) {
                    const float a = j[k], b = k + 1 < j.size() ? j[k + 1] : j[0] + n;
                    for (const float shift : {0.f, float(n)}) {
                        const float px = x + shift;
                        if (px >= a && px < b) { block = int(k); left = px - a; right = b - px; }
                    }
                }
                const float u = x / metre, v = y / metre;
                const float top = inRow, bottom = rowH - inRow;
                const float joint = 0.012f * metre, bevel = 0.035f * metre;
                const float edge = std::min({left, right, top, bottom});
                glm::vec3 colour;
                float h;
                if (edge < joint) {
                    colour = glm::vec3(.16f, .14f, .13f) * (0.85f + 0.3f * hash(x / 2, y / 2, 7));
                    h = -0.006f;
                } else {
                    const int id = r * 64 + block;
                    colour = colours[size_t(hash(id, 1, 9) * colours.size()) % colours.size()];
                    const glm::vec3 second = colours[size_t(hash(id, 2, 13) * colours.size()) % colours.size()];
                    colour = glm::mix(colour, second, 0.4f * fbm(u * 8, v * 8, int(Size * 8), id));
                    colour *= 0.88f + 0.22f * fbm(u * 80, v * 80, int(Size * 80), 17);      // grain
                    if (hash(x / 2, y / 2, 19) > 0.975f) colour *= 0.72f;                     // pits
                    // Bevel: light on the top and left edges, shadow on the bottom and right.
                    const float e = std::clamp((edge - joint) / bevel, 0.f, 1.f);
                    if (e < 1.f) {
                        const bool lit = edge == top || edge == left;
                        colour *= lit ? 1.f + 0.35f * (1 - e) : 1.f - 0.35f * (1 - e);
                    }
                    h = 0.008f * std::sqrt(e) + 0.001f * fbm(u * 60, v * 60, int(Size * 60), 23);
                }
                height[size_t(y) * n + x] = h;
                auto* p = images.albedo.data() + (size_t(y) * n + x) * 4;
                p[0] = std::uint8_t(std::clamp(colour.r, 0.f, 1.f) * 255);
                p[1] = std::uint8_t(std::clamp(colour.g, 0.f, 1.f) * 255);
                p[2] = std::uint8_t(std::clamp(colour.b, 0.f, 1.f) * 255);
                p[3] = 255;
            }
    };
    auto normals = [&](int from, int to) {
        for (int y = from; y < to; ++y)
            for (int x = 0; x < n; ++x) {
                auto hh = [&](int i, int k) { return height[size_t((k + n) % n) * n + size_t((i + n) % n)]; };
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

Images loadOrCreate(const std::filesystem::path& assetDirectory, const U7::Data& data) {
    const auto folder = assetDirectory / "Materials";
    const auto albedo = folder / "town-wall-hd.png", normal = folder / "town-wall-hd-normal.png";
    Images images;
    int nw = 0, nh = 0;
    const bool haveAlbedo = load(albedo, images.albedo, images.width, images.height);
    const bool haveNormal = load(normal, images.normal, nw, nh);
    if (haveAlbedo && haveNormal && nw == images.width && nh == images.height) return images;
    std::error_code error;
    std::filesystem::create_directories(folder, error);
    if (!haveAlbedo) {
        const auto generated = generate(data);
        save(albedo, generated.albedo, generated.width, generated.height);
        if (!haveNormal) save(normal, generated.normal, generated.width, generated.height);
        return generated;
    }
    images.normal.assign(images.albedo.size(), 0);
    for (size_t i = 0; i < images.normal.size(); i += 4) {
        images.normal[i] = 128; images.normal[i + 1] = 128; images.normal[i + 2] = 255; images.normal[i + 3] = 255;
    }
    return images;
}

}
