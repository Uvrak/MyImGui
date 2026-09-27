#include "StoneMaterial.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <thread>

namespace StoneMaterial {
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
    // U7's stone colours: the lit pixels of its stone wall, dark joints and outlines left out.
    std::vector<glm::vec3> stones, joints;
    const auto frame = data.frame(348, 0);
    for (size_t i = 0; i + 3 < frame.rgba.size(); i += 4) {
        if (!frame.rgba[i + 3]) continue;
        const glm::vec3 c(frame.rgba[i] / 255.f, frame.rgba[i + 1] / 255.f, frame.rgba[i + 2] / 255.f);
        const float l = c.r * 0.3f + c.g * 0.59f + c.b * 0.11f;
        (l > 0.22f ? stones : joints).push_back(c);
    }
    if (stones.empty()) stones = {{.45f, .42f, .40f}, {.55f, .50f, .46f}, {.36f, .38f, .40f}};
    glm::vec3 joint(.10f, .09f, .09f);
    if (!joints.empty()) { joint = glm::vec3(0); for (const auto& c : joints) joint += c; joint /= float(joints.size()); }

    constexpr int n = Pixels;
    constexpr float metre = n / Size;
    // Stones about 0.18 x 0.12 m: a jittered grid, rows offset like laid courses.
    constexpr int cols = 6, rows = 9;
    const float cw = float(n) / cols, ch = float(n) / rows;
    std::vector<float> height(size_t(n) * n);
    Images images;
    images.width = images.height = n;
    images.albedo.resize(size_t(n) * n * 4);
    images.normal.resize(size_t(n) * n * 4);
    auto centre = [&](int cx, int cy) {
        const int wx = ((cx % cols) + cols) % cols, wy = ((cy % rows) + rows) % rows;
        const float shift = (wy & 1) ? 0.5f : 0.f;
        return glm::vec2((cx + shift + 0.15f + 0.7f * hash(wx, wy, 1)) * cw, (cy + 0.25f + 0.5f * hash(wx, wy, 2)) * ch);
    };
    auto shade = [&](int from, int to) {
        for (int y = from; y < to; ++y)
            for (int x = 0; x < n; ++x) {
                const glm::vec2 p(x + 0.5f, y + 0.5f);
                // Irregular outlines: the sample point is warped a little.
                const float u = x / metre, v = y / metre;
                const glm::vec2 q = p + glm::vec2(fbm(u * 9, v * 9, int(Size * 9), 21) - 0.5f, fbm(u * 9, v * 9, int(Size * 9), 37) - 0.5f) * (cw * 0.35f);
                float d1 = 1e9f, d2 = 1e9f;
                glm::vec2 nearest(0);
                int best = 0;
                const int gx = int(std::floor(q.x / cw)), gy = int(std::floor(q.y / ch));
                for (int j = -2; j <= 2; ++j)
                    for (int i = -2; i <= 2; ++i) {
                        const glm::vec2 c = centre(gx + i, gy + j);
                        const glm::vec2 d((q.x - c.x) / cw, (q.y - c.y) / ch);
                        const float dist = glm::length(d);
                        if (dist < d1) {
                            d2 = d1; d1 = dist; nearest = d;
                            best = ((((gy + j) % rows) + rows) % rows) * cols + ((((gx + i) % cols) + cols) % cols);
                        } else if (dist < d2) d2 = dist;
                    }
                const float edge = (d2 - d1) * 0.5f;                               // 0 on the joint
                const float jointWidth = 0.045f + 0.02f * fbm(u * 20, v * 20, int(Size * 20), 5);
                glm::vec3 colour;
                float h;
                if (edge < jointWidth) {
                    colour = joint * (0.8f + 0.4f * hash(x / 2, y / 2, 3));
                    h = -0.008f;
                } else {
                    const float inside = std::min(1.f, (edge - jointWidth) / 0.18f);
                    colour = stones[size_t(hash(best, 4, 9) * stones.size()) % stones.size()];
                    // A second U7 colour mottles the stone, as its pixels do.
                    const glm::vec3 second = stones[size_t(hash(best, 5, 11) * stones.size()) % stones.size()];
                    colour = glm::mix(colour, second, 0.35f * fbm(u * 30, v * 30, int(Size * 30), best));
                    colour *= 0.9f + 0.2f * fbm(u * 60, v * 60, int(Size * 60), 13);
                    // U7's light: a bright rim on the upper left of each stone, shadow lower right.
                    const float rim = 1.f - inside;
                    const float toward = glm::dot(glm::normalize(nearest + glm::vec2(1e-5f)), glm::normalize(glm::vec2(-1, -1)));
                    colour *= 1.f + rim * rim * 0.45f * toward;
                    if (hash(x / 2, y / 2, 17) > 0.97f) colour *= 0.7f;           // pits
                    h = 0.006f * std::sqrt(inside);
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

Images loadOrCreate(const std::filesystem::path& assetDirectory, const U7::Data& data) {
    const auto folder = assetDirectory / "Materials";
    const auto albedo = folder / "stone-wall-hd.png", normal = folder / "stone-wall-hd-normal.png";
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
