#include "GateModels.h"
#include <cmath>

namespace GateModels {
namespace {

constexpr float Pi = 3.14159265f;

void box(std::vector<Vertex>& out, glm::vec3 low, glm::vec3 high) {
    const glm::vec3 c[8] = {{low.x, low.y, low.z}, {high.x, low.y, low.z}, {high.x, low.y, high.z}, {low.x, low.y, high.z},
                            {low.x, high.y, low.z}, {high.x, high.y, low.z}, {high.x, high.y, high.z}, {low.x, high.y, high.z}};
    const int f[6][4] = {{4, 5, 6, 7}, {3, 2, 1, 0}, {0, 1, 5, 4}, {2, 3, 7, 6}, {1, 2, 6, 5}, {3, 0, 4, 7}};
    const glm::vec3 n[6] = {{0, 1, 0}, {0, -1, 0}, {0, 0, -1}, {0, 0, 1}, {1, 0, 0}, {-1, 0, 0}};
    for (int k = 0; k < 6; ++k)
        for (int j : {0, 1, 2, 0, 2, 3}) {
            const auto& p = c[f[k][j]];
            out.push_back({p, n[k], {p.x + p.z, p.y}});
        }
}

// A spike: a square pyramid pointing down from the bar's end.
void spike(std::vector<Vertex>& out, glm::vec3 top, float half, float length) {
    const glm::vec3 tip = top - glm::vec3(0, length, 0);
    const glm::vec3 c[4] = {top + glm::vec3(-half, 0, -half), top + glm::vec3(half, 0, -half), top + glm::vec3(half, 0, half), top + glm::vec3(-half, 0, half)};
    for (int i = 0; i < 4; ++i) {
        const glm::vec3 a = c[i], b = c[(i + 1) % 4];
        const glm::vec3 n = glm::normalize(glm::cross(b - a, tip - a));
        for (const auto& p : {a, b, tip}) out.push_back({p, -n, {p.x + p.z, p.y}});
    }
}

void cylinder(std::vector<Vertex>& out, glm::vec3 a, glm::vec3 b, float radius, int sides = 12) {
    const glm::vec3 d = glm::normalize(b - a);
    const glm::vec3 p = glm::normalize(glm::cross(d, std::abs(d.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0))), q = glm::cross(d, p);
    const float length = glm::length(b - a);
    for (int i = 0; i < sides; ++i) {
        const float t0 = 2 * Pi * i / sides, t1 = 2 * Pi * (i + 1) / sides;
        const glm::vec3 n0 = p * std::cos(t0) + q * std::sin(t0), n1 = p * std::cos(t1) + q * std::sin(t1);
        const glm::vec3 v[4] = {a + n0 * radius, a + n1 * radius, b + n1 * radius, b + n0 * radius};
        const glm::vec3 n[4] = {n0, n1, n1, n0};
        const glm::vec2 uv[4] = {{t0 * radius, 0}, {t1 * radius, 0}, {t1 * radius, length}, {t0 * radius, length}};
        for (int j : {0, 1, 2, 0, 2, 3}) out.push_back({v[j], n[j], uv[j]});
        for (const auto& [c, n2] : {std::pair{a, -d}, std::pair{b, d}}) {
            const glm::vec3 w[3] = {c, c + n0 * radius, c + n1 * radius};
            for (const auto& x : w) out.push_back({x, n2, {0.5f, 0.5f}});
        }
    }
}

}

Parts portcullis(float length, float height) {
    Parts parts;
    const float bar = 0.035f, depth = 0.05f;
    // Vertical bars every 0.18 m, with spikes below.
    const int count = std::max(2, int(length / 0.18f));
    for (int i = 0; i <= count; ++i) {
        const float x = 0.06f + (length - 0.12f) * i / count;
        box(parts.iron, {x - bar, 0.22f, -depth / 2}, {x + bar, height - 0.2f, depth / 2});
        spike(parts.iron, {x, 0.22f, 0}, bar, 0.22f);
    }
    // Cross bars every 0.35 m, in front, with rivet heads at the crossings.
    for (float y = 0.45f; y < height - 0.3f; y += 0.35f) {
        box(parts.iron, {0.02f, y - 0.03f, depth / 2}, {length - 0.02f, y + 0.03f, depth / 2 + 0.03f});
        for (int i = 0; i <= count; ++i) {
            const float x = 0.06f + (length - 0.12f) * i / count;
            box(parts.iron, {x - 0.018f, y - 0.018f, depth / 2 + 0.03f}, {x + 0.018f, y + 0.018f, depth / 2 + 0.045f});
        }
    }
    // Top beam (oak, iron bound).
    box(parts.darkWood, {0.f, height - 0.22f, -0.09f}, {length, height, 0.09f});
    for (float x = 0.2f; x < length; x += 0.6f) box(parts.iron, {x, height - 0.23f, -0.1f}, {x + 0.06f, height + 0.01f, 0.1f});
    return parts;
}

Parts winch(bool drum) {
    Parts parts;
    const float half = 0.7f, y = WinchAxle, r = WinchDrumRadius;
    if (drum) {
        // Drum with iron bands, chain wound around it (rings as small boxes), axle and cranks.
        cylinder(parts.wood, {-half + 0.12f, y, 0}, {half - 0.12f, y, 0}, r, 18);
        for (const float x : {-half + 0.18f, half - 0.18f}) cylinder(parts.iron, {x - 0.02f, y, 0}, {x + 0.02f, y, 0}, r + 0.012f, 18);
        for (int i = 0; i < 14; ++i)
            for (int k = 0; k < 16; ++k) {
                const float a = 2 * Pi * k / 16, x = -half + 0.26f + i * 0.066f;
                const glm::vec3 c(x, y + std::cos(a) * (r + 0.02f), std::sin(a) * (r + 0.02f));
                box(parts.iron, c - glm::vec3(0.022f, 0.012f, 0.012f), c + glm::vec3(0.022f, 0.012f, 0.012f));
            }
        cylinder(parts.iron, {-half - 0.2f, y, 0}, {half + 0.2f, y, 0}, 0.025f, 8);
        for (const float s : {-1.f, 1.f}) {
            const float x = s * (half + 0.2f);
            cylinder(parts.iron, {x, y, 0}, {x, y - 0.32f, 0.08f}, 0.022f, 8);
            cylinder(parts.darkWood, {x, y - 0.32f, 0.08f}, {x + s * 0.18f, y - 0.32f, 0.08f}, 0.03f, 8);
        }
        return parts;
    }
    // Trestles: two legs each, crossing below the axle, a foot beam, the bearings.
    for (const float x : {-half, half}) {
        for (const float z : {-0.42f, 0.42f}) cylinder(parts.darkWood, {x, 0.02f, z}, {x, y + 0.08f, 0}, 0.055f, 8);
        box(parts.darkWood, {x - 0.06f, 0.f, -0.5f}, {x + 0.06f, 0.1f, 0.5f});
        cylinder(parts.iron, {x - 0.08f, y, 0}, {x + 0.08f, y, 0}, 0.04f, 10);
    }
    return parts;
}

void stairStep(Parts& out, glm::vec3 low, glm::vec3 high) {
    const float tread = std::min(0.06f, (high.y - low.y) * 0.5f), over = 0.03f;
    // Body of planks, the tread overhanging, a dark nosing strip under the tread's front.
    box(out.wood, low, {high.x, high.y - tread, high.z});
    box(out.wood, {low.x - over, high.y - tread, low.z - over}, {high.x + over, high.y, high.z + over});
    box(out.darkWood, {low.x - over, high.y - tread - 0.02f, low.z - over}, {high.x + over, high.y - tread, high.z + over});
}

}
