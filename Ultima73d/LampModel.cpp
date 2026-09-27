#include "LampModel.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace LampModel {
namespace {

constexpr float Pi = 3.14159265f;

// Surface of revolution around y through the profile (radius, height) points.
void lathe(std::vector<Vertex>& out, const std::vector<glm::vec2>& profile, int sides, float vScale = 1.f) {
    for (size_t k = 0; k + 1 < profile.size(); ++k) {
        const auto a = profile[k], b = profile[k + 1];
        const glm::vec2 slope = glm::normalize(glm::vec2(b.y - a.y, -(b.x - a.x)));   // outward in (r, y)
        for (int i = 0; i < sides; ++i) {
            const float t0 = 2 * Pi * i / sides, t1 = 2 * Pi * (i + 1) / sides;
            const glm::vec3 d0(std::cos(t0), 0, std::sin(t0)), d1(std::cos(t1), 0, std::sin(t1));
            const glm::vec3 p[4] = {d0 * a.x + glm::vec3(0, a.y, 0), d1 * a.x + glm::vec3(0, a.y, 0), d1 * b.x + glm::vec3(0, b.y, 0), d0 * b.x + glm::vec3(0, b.y, 0)};
            const glm::vec3 n[4] = {glm::normalize(d0 * slope.x + glm::vec3(0, slope.y, 0)), glm::normalize(d1 * slope.x + glm::vec3(0, slope.y, 0)),
                                    glm::normalize(d1 * slope.x + glm::vec3(0, slope.y, 0)), glm::normalize(d0 * slope.x + glm::vec3(0, slope.y, 0))};
            const float u0 = float(i) / sides, u1 = float(i + 1) / sides;
            const glm::vec2 uv[4] = {{u0, a.y * vScale}, {u1, a.y * vScale}, {u1, b.y * vScale}, {u0, b.y * vScale}};
            for (int j : {0, 1, 2, 0, 2, 3}) out.push_back({p[j], n[j], uv[j]});
        }
    }
}

void box(std::vector<Vertex>& out, glm::vec3 low, glm::vec3 high) {
    const glm::vec3 c[8] = {{low.x, low.y, low.z}, {high.x, low.y, low.z}, {high.x, low.y, high.z}, {low.x, low.y, high.z},
                            {low.x, high.y, low.z}, {high.x, high.y, low.z}, {high.x, high.y, high.z}, {low.x, high.y, high.z}};
    const int f[6][4] = {{4, 5, 6, 7}, {3, 2, 1, 0}, {0, 1, 5, 4}, {2, 3, 7, 6}, {1, 2, 6, 5}, {3, 0, 4, 7}};
    const glm::vec3 n[6] = {{0, 1, 0}, {0, -1, 0}, {0, 0, -1}, {0, 0, 1}, {1, 0, 0}, {-1, 0, 0}};
    const glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    for (int k = 0; k < 6; ++k)
        for (int j : {0, 1, 2, 0, 2, 3}) out.push_back({c[f[k][j]], n[k], uv[j]});
}

// Horizontal bar along x with rounded knob ends (the crossbar).
void bar(std::vector<Vertex>& out, float half, float y, float radius) {
    for (int i = 0; i < 12; ++i) {
        const float t0 = 2 * Pi * i / 12, t1 = 2 * Pi * (i + 1) / 12;
        const glm::vec3 d0(0, std::cos(t0), std::sin(t0)), d1(0, std::cos(t1), std::sin(t1));
        const glm::vec3 p[4] = {glm::vec3(-half, y, 0) + d0 * radius, glm::vec3(half, y, 0) + d0 * radius,
                                glm::vec3(half, y, 0) + d1 * radius, glm::vec3(-half, y, 0) + d1 * radius};
        const glm::vec3 n[4] = {d0, d0, d1, d1};
        const glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        for (int j : {0, 1, 2, 0, 2, 3}) out.push_back({p[j], n[j], uv[j]});
    }
    // Knobs: small spheres (a lathe around y, moved to each end).
    for (const float x : {-half, half}) {
        std::vector<Vertex> knob;
        std::vector<glm::vec2> profile;
        for (int k = 0; k <= 8; ++k) {
            const float a = Pi * k / 8;
            profile.push_back({std::sin(a) * radius * 1.8f + 0.0001f, -std::cos(a) * radius * 1.8f});
        }
        lathe(knob, profile, 12);
        for (auto& v : knob) { v.position += glm::vec3(x, y, 0); out.push_back(v); }
    }
}

float hash(int x, int y) {
    std::uint32_t h = std::uint32_t(x) * 374761393u + std::uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return float((h ^ (h >> 16)) & 0xffff) / 65535.f;
}

}

Parts build() {
    Parts parts;
    // Post: flared foot, a ring, the tapered turned shaft up to 2.6 m, a collar.
    lathe(parts.post, {{0.001f, 0.f}, {0.16f, 0.f}, {0.16f, 0.06f}, {0.11f, 0.10f}, {0.075f, 0.22f}, {0.085f, 0.28f}, {0.07f, 0.34f},
                       {0.055f, 2.5f}, {0.07f, 2.56f}, {0.07f, 2.62f}, {0.05f, 2.66f}}, 16, 1.f);
    // Crossbar under the head, as in U7.
    bar(parts.post, 0.42f, 2.40f, 0.025f);
    // Lantern: a square iron frame 0.34 m wide, 0.46 m high, with four panes.
    const float w = 0.17f, y0 = 2.66f, y1 = 3.12f, b = 0.022f;
    box(parts.iron, {-w - 0.02f, y0, -w - 0.02f}, {w + 0.02f, y0 + 0.05f, w + 0.02f});                 // base plate
    box(parts.iron, {-w - 0.03f, y1 - 0.04f, -w - 0.03f}, {w + 0.03f, y1, w + 0.03f});                  // top plate
    for (const float sx : {-1.f, 1.f})
        for (const float sz : {-1.f, 1.f})
            box(parts.iron, {sx * w - b, y0, sz * w - b}, {sx * w + b, y1, sz * w + b});                // corner posts
    const glm::vec2 uv[4] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
    for (int side = 0; side < 4; ++side) {
        const float angle = side * Pi / 2;
        const glm::vec3 n(std::sin(angle), 0, std::cos(angle)), t(std::cos(angle), 0, -std::sin(angle));
        const glm::vec3 c = n * (w - 0.005f);
        const glm::vec3 p[4] = {c - t * w + glm::vec3(0, y0 + 0.05f, 0), c + t * w + glm::vec3(0, y0 + 0.05f, 0),
                                c + t * w + glm::vec3(0, y1 - 0.04f, 0), c - t * w + glm::vec3(0, y1 - 0.04f, 0)};
        for (int j : {0, 1, 2, 0, 2, 3}) parts.glass.push_back({p[j], n, uv[j]});
        // A glazing bar across each pane.
        box(parts.iron, c - t * w + glm::vec3(0, (y0 + y1) / 2 - 0.01f, 0) - n * 0.01f, c + t * w + glm::vec3(0, (y0 + y1) / 2 + 0.01f, 0) + n * 0.01f);
    }
    // The rounded cap (U7's brown dome) with a finial.
    std::vector<glm::vec2> cap;
    for (int k = 0; k <= 10; ++k) {
        const float a = 0.5f * Pi * k / 10;
        cap.push_back({0.001f + 0.27f * std::cos(a), y1 + 0.24f * std::sin(a)});
    }
    lathe(parts.post, {{0.001f, y1}, {0.27f, y1}}, 16);
    lathe(parts.post, cap, 16);
    lathe(parts.post, {{0.001f, y1 + 0.24f}, {0.04f, y1 + 0.24f}, {0.05f, y1 + 0.30f}, {0.02f, y1 + 0.36f}, {0.001f, y1 + 0.38f}}, 10);
    parts.height = y1 + 0.38f;
    return parts;
}

std::vector<std::uint8_t> postTexture(int size) {
    // U7's lamp post brown (a pinkish grey-brown), darker streaks along the post, worn lighter spots.
    std::vector<std::uint8_t> image(size_t(size) * size * 4);
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x) {
            const float streak = 0.85f + 0.15f * std::sin(x * 0.9f + hash(x, 0) * 6.f);
            const float speck = hash(x / 2, y / 2) > 0.94f ? 1.15f : hash(x / 3, y / 3) < 0.05f ? 0.8f : 1.f;
            const float l = streak * speck * (0.95f + 0.1f * hash(x, y));
            auto* p = image.data() + (size_t(y) * size + x) * 4;
            p[0] = std::uint8_t(std::clamp(0.50f * l, 0.f, 1.f) * 255);
            p[1] = std::uint8_t(std::clamp(0.38f * l, 0.f, 1.f) * 255);
            p[2] = std::uint8_t(std::clamp(0.34f * l, 0.f, 1.f) * 255);
            p[3] = 255;
        }
    return image;
}

std::vector<std::uint8_t> glassTexture(int size) {
    // Warm lamp light: bright in the middle (the flame), soft towards the frame.
    std::vector<std::uint8_t> image(size_t(size) * size * 4);
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x) {
            const float dx = (x + 0.5f) / size - 0.5f, dy = (y + 0.5f) / size - 0.58f;
            const float glow = std::exp(-(dx * dx * 9 + dy * dy * 5));
            auto* p = image.data() + (size_t(y) * size + x) * 4;
            p[0] = std::uint8_t(std::min(255.f, 200 + 55 * glow));
            p[1] = std::uint8_t(std::min(255.f, 130 + 110 * glow));
            p[2] = std::uint8_t(std::min(255.f, 40 + 120 * glow));
            p[3] = 255;
        }
    return image;
}

}
