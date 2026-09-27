#include "WellModel.h"
#include <cmath>

namespace WellModel {
namespace {

constexpr float Pi = 3.14159265f;

// Surface of revolution around y through the profile (radius, height) points; uv in metres.
void lathe(std::vector<Vertex>& out, const std::vector<glm::vec2>& profile, int sides, glm::vec3 at = glm::vec3(0)) {
    for (size_t k = 0; k + 1 < profile.size(); ++k) {
        const auto a = profile[k], b = profile[k + 1];
        glm::vec2 slope(b.y - a.y, -(b.x - a.x));
        slope = glm::length(slope) > 1e-6f ? glm::normalize(slope) : glm::vec2(0, 1);
        for (int i = 0; i < sides; ++i) {
            const float t0 = 2 * Pi * i / sides, t1 = 2 * Pi * (i + 1) / sides;
            const glm::vec3 d0(std::cos(t0), 0, std::sin(t0)), d1(std::cos(t1), 0, std::sin(t1));
            const glm::vec3 p[4] = {at + d0 * a.x + glm::vec3(0, a.y, 0), at + d1 * a.x + glm::vec3(0, a.y, 0),
                                    at + d1 * b.x + glm::vec3(0, b.y, 0), at + d0 * b.x + glm::vec3(0, b.y, 0)};
            const glm::vec3 n0 = glm::normalize(d0 * slope.x + glm::vec3(0, slope.y, 0)), n1 = glm::normalize(d1 * slope.x + glm::vec3(0, slope.y, 0));
            const glm::vec3 n[4] = {n0, n1, n1, n0};
            const float u0 = t0 * std::max(a.x, b.x), u1 = t1 * std::max(a.x, b.x);
            const glm::vec2 uv[4] = {{u0, a.y}, {u1, a.y}, {u1, b.y}, {u0, b.y}};
            for (int j : {0, 1, 2, 0, 2, 3}) out.push_back({p[j], n[j], uv[j]});
        }
    }
}

// A round bar from a to b.
void cylinder(std::vector<Vertex>& out, glm::vec3 a, glm::vec3 b, float radius, int sides = 10) {
    const glm::vec3 d = glm::normalize(b - a);
    const glm::vec3 p = glm::normalize(glm::cross(d, std::abs(d.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0))), q = glm::cross(d, p);
    const float length = glm::length(b - a);
    for (int i = 0; i < sides; ++i) {
        const float t0 = 2 * Pi * i / sides, t1 = 2 * Pi * (i + 1) / sides;
        const glm::vec3 n0 = p * std::cos(t0) + q * std::sin(t0), n1 = p * std::cos(t1) + q * std::sin(t1);
        const glm::vec3 v[4] = {a + n0 * radius, a + n1 * radius, b + n1 * radius, b + n0 * radius};
        const glm::vec3 n[4] = {n0, n1, n1, n0};
        const glm::vec2 uv[4] = {{float(i) / sides, 0}, {float(i + 1) / sides, 0}, {float(i + 1) / sides, length}, {float(i) / sides, length}};
        for (int j : {0, 1, 2, 0, 2, 3}) out.push_back({v[j], n[j], uv[j]});
    }
    // End caps.
    for (const auto& [c, n] : {std::pair{a, -d}, std::pair{b, d}})
        for (int i = 0; i < sides; ++i) {
            const float t0 = 2 * Pi * i / sides, t1 = 2 * Pi * (i + 1) / sides;
            const glm::vec3 v[3] = {c, c + (p * std::cos(t0) + q * std::sin(t0)) * radius, c + (p * std::cos(t1) + q * std::sin(t1)) * radius};
            for (const auto& x : v) out.push_back({x, n, {0.5f, 0.5f}});
        }
}

void box(std::vector<Vertex>& out, glm::vec3 low, glm::vec3 high) {
    const glm::vec3 c[8] = {{low.x, low.y, low.z}, {high.x, low.y, low.z}, {high.x, low.y, high.z}, {low.x, low.y, high.z},
                            {low.x, high.y, low.z}, {high.x, high.y, low.z}, {high.x, high.y, high.z}, {low.x, high.y, high.z}};
    const int f[6][4] = {{4, 5, 6, 7}, {3, 2, 1, 0}, {0, 1, 5, 4}, {2, 3, 7, 6}, {1, 2, 6, 5}, {3, 0, 4, 7}};
    const glm::vec3 n[6] = {{0, 1, 0}, {0, -1, 0}, {0, 0, -1}, {0, 0, 1}, {1, 0, 0}, {-1, 0, 0}};
    for (int k = 0; k < 6; ++k) {
        const glm::vec2 uv[4] = {{c[f[k][0]].x + c[f[k][0]].z, c[f[k][0]].y}, {c[f[k][1]].x + c[f[k][1]].z, c[f[k][1]].y},
                                 {c[f[k][2]].x + c[f[k][2]].z, c[f[k][2]].y}, {c[f[k][3]].x + c[f[k][3]].z, c[f[k][3]].y}};
        for (int j : {0, 1, 2, 0, 2, 3}) out.push_back({c[f[k][j]], n[k], uv[j]});
    }
}

}

Parts build() {
    Parts parts;
    const float outer = 0.72f, inner = 0.52f, top = 0.78f;
    // Round wall: outside, inside (going down into the shaft).
    lathe(parts.stone, {{outer + 0.03f, 0.f}, {outer, 0.12f}, {outer, top - 0.1f}}, 32);
    lathe(parts.stone, {{inner, top - 0.1f}, {inner, -1.2f}}, 32);
    // Dressed rim: a rounded ring of cut stone on top.
    lathe(parts.rim, {{outer, top - 0.1f}, {outer + 0.05f, top - 0.05f}, {outer + 0.04f, top}, {inner - 0.03f, top}, {inner - 0.04f, top - 0.05f}, {inner, top - 0.1f}}, 32);
    // Water far down.
    lathe(parts.water, {{0.001f, -0.45f}, {inner, -0.45f}}, 32);
    // Windlass: two posts east and west outside the rim, a beam across the top, the roller with
    // a crank on the east side, the rope and a bucket hanging over the water.
    const float px = outer + 0.12f, post = 0.07f, beam = 1.85f;
    box(parts.wood, {-px - post, 0.f, -post}, {-px + post, beam, post});
    box(parts.wood, {px - post, 0.f, -post}, {px + post, beam, post});
    box(parts.wood, {-px - 0.18f, beam, -0.09f}, {px + 0.18f, beam + 0.12f, 0.09f});                // top beam
    // Slanted braces from the ground to the posts (U7 draws them).
    for (const float s : {-1.f, 1.f})
        for (const float z : {-1.f, 1.f})
            cylinder(parts.wood, {s * px, 0.02f, z * 0.55f}, {s * px, 1.1f, z * 0.05f}, 0.04f, 8);
    const float ry = 1.2f;
    cylinder(parts.wood, {-px + post, ry, 0}, {px - post, ry, 0}, 0.09f, 14);                       // roller
    cylinder(parts.iron, {px - post, ry, 0}, {px + 0.22f, ry, 0}, 0.025f, 8);                       // axle
    cylinder(parts.iron, {px + 0.22f, ry, 0}, {px + 0.22f, ry - 0.3f, 0.05f}, 0.022f, 8);          // crank arm
    cylinder(parts.wood, {px + 0.22f, ry - 0.3f, 0.05f}, {px + 0.40f, ry - 0.3f, 0.05f}, 0.03f, 8); // handle
    // Rope wound on the roller and hanging down to the bucket.
    for (int i = 0; i < 6; ++i) {
        const float x = -0.2f + i * 0.06f;
        cylinder(parts.rope, {x, ry, 0}, {x + 0.05f, ry, 0}, 0.105f, 12);
    }
    // The bucket hangs just above the rim.
    const glm::vec3 bucket(0.1f, 0.62f, 0);
    cylinder(parts.rope, {0.1f, ry - 0.1f, 0}, bucket + glm::vec3(0, 0.62f, 0), 0.012f, 6);
    lathe(parts.wood, {{0.001f, 0.28f}, {0.12f, 0.28f}, {0.15f, 0.52f}, {0.14f, 0.52f}, {0.11f, 0.30f}, {0.001f, 0.30f}}, 16, bucket);
    for (const float y : {0.33f, 0.47f})
        lathe(parts.iron, {{0.125f + (y - 0.28f) * 0.12f, y}, {0.128f + (y - 0.28f) * 0.12f, y + 0.02f}}, 16, bucket);
    cylinder(parts.iron, bucket + glm::vec3(-0.14f, 0.52f, 0), bucket + glm::vec3(0, 0.62f, 0), 0.008f, 6);   // bucket handle
    cylinder(parts.iron, bucket + glm::vec3(0.14f, 0.52f, 0), bucket + glm::vec3(0, 0.62f, 0), 0.008f, 6);
    return parts;
}

}
