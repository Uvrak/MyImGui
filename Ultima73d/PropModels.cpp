#include "PropModels.h"
#include <algorithm>
#include <cmath>

namespace PropModels {
namespace {

constexpr float Pi = 3.14159265f;

struct Random {
    unsigned state;
    float next() { state = state * 1664525u + 1013904223u; unsigned h = state ^ (state >> 16); h *= 0x7feb352du; h ^= h >> 15; return float(h & 0xffffff) / float(0xffffff); }
    float range(float a, float b) { return a + (b - a) * next(); }
};

void box(std::vector<Vertex>& out, glm::vec3 low, glm::vec3 high) {
    const glm::vec3 c[8] = {{low.x, low.y, low.z}, {high.x, low.y, low.z}, {high.x, low.y, high.z}, {low.x, low.y, high.z},
                            {low.x, high.y, low.z}, {high.x, high.y, low.z}, {high.x, high.y, high.z}, {low.x, high.y, high.z}};
    const int f[6][4] = {{4, 5, 6, 7}, {3, 2, 1, 0}, {0, 1, 5, 4}, {2, 3, 7, 6}, {1, 2, 6, 5}, {3, 0, 4, 7}};
    const glm::vec3 n[6] = {{0, 1, 0}, {0, -1, 0}, {0, 0, -1}, {0, 0, 1}, {1, 0, 0}, {-1, 0, 0}};
    for (int k = 0; k < 6; ++k)
        for (int j : {0, 1, 2, 0, 2, 3}) {
            const auto& p = c[f[k][j]];
            const glm::vec2 uv = std::abs(n[k].y) > 0.5f ? glm::vec2(p.x, p.z) : std::abs(n[k].x) > 0.5f ? glm::vec2(p.z, p.y) : glm::vec2(p.x, p.y);
            out.push_back({p, n[k], uv});
        }
}

// Surface of revolution around y through (radius, height) points, at `at`.
void lathe(std::vector<Vertex>& out, const std::vector<glm::vec2>& profile, int sides, glm::vec3 at) {
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
            const glm::vec2 uv[4] = {{float(i) / sides, a.y}, {float(i + 1) / sides, a.y}, {float(i + 1) / sides, b.y}, {float(i) / sides, b.y}};
            for (int j : {0, 1, 2, 0, 2, 3}) out.push_back({p[j], n[j], uv[j]});
        }
    }
}

// A card with the leaf texture, facing `normal` roughly, centred at c.
void card(std::vector<Vertex>& out, glm::vec3 c, float size, float yaw, float tilt) {
    const glm::vec3 right(std::cos(yaw), 0, std::sin(yaw));
    const glm::vec3 up = glm::normalize(glm::vec3(-std::sin(yaw) * tilt, 1, std::cos(yaw) * tilt));
    const glm::vec3 shading = glm::length(glm::vec3(c.x, 0, c.z)) > 0.01f ? glm::normalize(glm::normalize(glm::vec3(c.x, 0, c.z)) + glm::vec3(0, 1, 0)) : glm::vec3(0, 1, 0);
    const float s = size * 0.5f;
    const glm::vec3 v[4] = {c - right * s - up * s, c + right * s - up * s, c + right * s + up * s, c - right * s + up * s};
    const glm::vec2 uv[4] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
    for (int j : {0, 1, 2, 0, 2, 3}) out.push_back({v[j], shading, uv[j]});
}

}

Model table(float w, float d, float h) {
    Model m;
    const float top = 0.05f, leg = 0.07f, inset = 0.08f;
    box(m.parts[Wood], {-w + 0.02f, h - top, -d + 0.02f}, {-0.02f, h, -0.02f});
    for (const float x : {-w + inset, -inset - leg})
        for (const float z : {-d + inset, -inset - leg}) box(m.parts[DarkWood], {x, 0, z}, {x + leg, h - top, z + leg});
    // Apron under the top.
    box(m.parts[DarkWood], {-w + inset, h - top - 0.1f, -d + inset}, {-inset, h - top, -d + inset + 0.03f});
    box(m.parts[DarkWood], {-w + inset, h - top - 0.1f, -inset - 0.03f}, {-inset, h - top, -inset});
    return m;
}

Model bench(float w, float d, float h, bool alongX) {
    Model m;
    const float seat = std::min(h, 0.45f);
    // The seat plank runs along the bench; legs at both ends.
    const float sw = alongX ? w - 0.04f : 0.32f, sd = alongX ? 0.32f : d - 0.04f;
    const glm::vec3 c(-w / 2, 0, -d / 2);
    box(m.parts[Wood], {c.x - sw / 2, seat - 0.05f, c.z - sd / 2}, {c.x + sw / 2, seat, c.z + sd / 2});
    for (const float s : {-1.f, 1.f}) {
        const glm::vec3 at = c + (alongX ? glm::vec3(s * (sw / 2 - 0.08f), 0, 0) : glm::vec3(0, 0, s * (sd / 2 - 0.08f)));
        const glm::vec3 half = alongX ? glm::vec3(0.035f, 0, sd / 2 - 0.02f) : glm::vec3(sw / 2 - 0.02f, 0, 0.035f);
        box(m.parts[DarkWood], {at.x - half.x, 0, at.z - half.z}, {at.x + half.x, seat - 0.05f, at.z + half.z});
    }
    return m;
}

Model chair(float size, float h, int back) {
    Model m;
    const float seat = 0.45f, leg = 0.045f, s = std::min(size, 0.45f);
    const glm::vec3 c(-size / 2, 0, -size / 2);
    box(m.parts[Wood], {c.x - s / 2, seat - 0.04f, c.z - s / 2}, {c.x + s / 2, seat, c.z + s / 2});
    for (const float x : {-1.f, 1.f})
        for (const float z : {-1.f, 1.f}) {
            const glm::vec3 p = c + glm::vec3(x * (s / 2 - leg), 0, z * (s / 2 - leg));
            box(m.parts[DarkWood], {p.x - leg / 2, 0, p.z - leg / 2}, {p.x + leg / 2, seat - 0.04f, p.z + leg / 2});
        }
    // Back: two posts and slats on the side `back`, up to the chair's height.
    const glm::vec2 dir[4] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
    const glm::vec2 b = dir[back & 3], t(-b.y, b.x);
    const float top = std::max(h, 0.95f);
    for (const float k : {-1.f, 1.f}) {
        const glm::vec3 p = c + glm::vec3(b.x, 0, b.y) * (s / 2 - leg) + glm::vec3(t.x, 0, t.y) * (k * (s / 2 - leg));
        box(m.parts[DarkWood], {p.x - leg / 2, seat, p.z - leg / 2}, {p.x + leg / 2, top, p.z + leg / 2});
    }
    for (const float y : {seat + 0.2f, top - 0.12f}) {
        const glm::vec3 p = c + glm::vec3(b.x, 0, b.y) * (s / 2 - leg);
        const glm::vec3 half = glm::vec3(std::abs(t.x), 0, std::abs(t.y)) * (s / 2 - leg) + glm::vec3(std::abs(b.x), 0, std::abs(b.y)) * 0.015f;
        box(m.parts[Wood], {p.x - half.x, y, p.z - half.z}, {p.x + half.x, y + 0.08f, p.z + half.z});
    }
    return m;
}

Model bed(float w, float d, float h, bool alongX) {
    Model m;
    const float frame = std::max(0.3f, h * 0.8f), mattress = 0.14f;
    // Frame and legs, mattress with the cover, a pillow and a headboard at the head end.
    box(m.parts[DarkWood], {-w + 0.03f, 0.12f, -d + 0.03f}, {-0.03f, frame, -0.03f});
    box(m.parts[Linen], {-w + 0.06f, frame, -d + 0.06f}, {-0.06f, frame + mattress, -0.06f});
    // Cover over the foot two thirds.
    if (alongX) box(m.parts[Cloth], {-w * 0.68f, frame + 0.02f, -d + 0.04f}, {-0.04f, frame + mattress + 0.03f, -0.04f});
    else box(m.parts[Cloth], {-w + 0.04f, frame + 0.02f, -d * 0.68f}, {-0.04f, frame + mattress + 0.03f, -0.04f});
    if (alongX) {
        box(m.parts[Linen], {-w + 0.12f, frame + mattress, -d + 0.15f}, {-w + 0.42f, frame + mattress + 0.1f, -0.15f});
        box(m.parts[DarkWood], {-w, 0, -d}, {-w + 0.06f, 0.95f, 0.f});
    } else {
        box(m.parts[Linen], {-w + 0.15f, frame + mattress, -d + 0.12f}, {-0.15f, frame + mattress + 0.1f, -d + 0.42f});
        box(m.parts[DarkWood], {-w, 0, -d}, {0.f, 0.95f, -d + 0.06f});
    }
    for (const float x : {-w + 0.03f, -0.1f})
        for (const float z : {-d + 0.03f, -0.1f}) box(m.parts[DarkWood], {x, 0, z}, {x + 0.07f, 0.14f, z + 0.07f});
    return m;
}

Model crate(float w, float d, float h) {
    Model m;
    const float s = 0.03f;
    box(m.parts[Wood], {-w + s, 0, -d + s}, {-s, h, -s});
    // Corner battens and a band across each face.
    for (const float x : {-w + s, -s - 0.05f})
        for (const float z : {-d + s, -s - 0.05f}) box(m.parts[DarkWood], {x - 0.01f, 0, z - 0.01f}, {x + 0.06f, h + 0.01f, z + 0.06f});
    box(m.parts[DarkWood], {-w + s - 0.01f, h * 0.45f, -d + s - 0.01f}, {-s + 0.01f, h * 0.55f, -s + 0.01f});
    return m;
}

Model chest(float w, float d, float h) {
    Model m;
    const float body = h * 0.7f;
    box(m.parts[Wood], {-w + 0.05f, 0, -d + 0.08f}, {-0.05f, body, -0.08f});
    // The lid, rounded as a half cylinder along x.
    const float r = (d - 0.16f) / 2, cz = -d / 2;
    for (int i = 0; i < 10; ++i) {
        const float a0 = Pi * i / 10, a1 = Pi * (i + 1) / 10;
        const glm::vec3 n0(0, std::sin(a0), -std::cos(a0)), n1(0, std::sin(a1), -std::cos(a1));
        const glm::vec3 p[4] = {glm::vec3(-w + 0.04f, body, cz) + n0 * r, glm::vec3(-0.04f, body, cz) + n0 * r,
                                glm::vec3(-0.04f, body, cz) + n1 * r, glm::vec3(-w + 0.04f, body, cz) + n1 * r};
        const glm::vec3 n[4] = {n0, n0, n1, n1};
        const glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        for (int j : {0, 1, 2, 0, 2, 3}) m.parts[Wood].push_back({p[j], n[j], uv[j]});
    }
    // Iron bands and a lock.
    for (const float x : {-w + 0.15f, -0.19f}) box(m.parts[Iron], {x, 0, -d + 0.07f}, {x + 0.04f, body + r + 0.01f, -0.07f});
    box(m.parts[Iron], {-w / 2 - 0.05f, body - 0.12f, -0.085f}, {-w / 2 + 0.05f, body, -0.06f});
    return m;
}

Model desk(float w, float d, float h) {
    Model m = table(w, d, std::max(h, 0.75f));
    const float top = std::max(h, 0.75f) - 0.05f;
    // A drawer box under the top on the east side, with two drawer fronts and knobs.
    const float bw = std::min(w * 0.4f, 0.45f);
    box(m.parts[Wood], {-bw - 0.05f, 0.08f, -d + 0.06f}, {-0.05f, top, -0.06f});
    for (int k = 0; k < 2; ++k) {
        const float y0 = 0.12f + (top - 0.12f) * k / 2, y1 = 0.12f + (top - 0.12f) * (k + 1) / 2 - 0.03f;
        box(m.parts[DarkWood], {-bw - 0.03f, y0, -0.06f}, {-0.07f, y1, -0.04f});
        box(m.parts[Iron], {-bw / 2 - 0.08f, (y0 + y1) / 2 - 0.015f, -0.04f}, {-bw / 2 - 0.04f, (y0 + y1) / 2 + 0.015f, -0.02f});
    }
    return m;
}

Model drawers(float w, float d, float h) {
    Model m;
    const float top = std::max(h, 0.9f);
    box(m.parts[Wood], {-w + 0.04f, 0.06f, -d + 0.06f}, {-0.04f, top - 0.03f, -0.06f});
    box(m.parts[DarkWood], {-w + 0.02f, top - 0.03f, -d + 0.04f}, {-0.02f, top, -0.04f});
    for (const float x : {-w + 0.05f, -0.1f})
        for (const float z : {-d + 0.07f, -0.12f}) box(m.parts[DarkWood], {x, 0, z}, {x + 0.05f, 0.06f, z + 0.05f});
    // Drawer fronts on the south side, with two knobs each.
    const int rows = 3;
    for (int k = 0; k < rows; ++k) {
        const float y0 = 0.1f + (top - 0.16f) * k / rows, y1 = 0.1f + (top - 0.16f) * (k + 1) / rows - 0.03f;
        box(m.parts[DarkWood], {-w + 0.07f, y0, -0.06f}, {-0.07f, y1, -0.04f});
        for (const float x : {-w * 0.72f, -w * 0.28f})
            box(m.parts[Iron], {x - 0.02f, (y0 + y1) / 2 - 0.02f, -0.04f}, {x + 0.02f, (y0 + y1) / 2 + 0.02f, -0.015f});
    }
    return m;
}

Model rock(float size, unsigned seed) {
    Model m;
    Random random{seed * 2654435761u + 7u};
    random.next();
    // A lumpy, flattened sphere: a lathe with noise on the radius per vertex.
    constexpr int rings = 6, sides = 9;   // few, large facets: a broken field stone
    const float r = size * 0.5f, flat = random.range(0.45f, 0.7f);
    auto point = [&](int ring, int side) {
        const float a = Pi * ring / rings, b = 2 * Pi * side / sides;
        const unsigned h = unsigned(ring * 131 + (side % sides) * 17) + seed * 31u;
        const float bump = 0.7f + 0.55f * float((h * 2654435761u >> 8) & 0xff) / 255.f;
        return glm::vec3(std::sin(a) * std::cos(b) * r * bump, (1 - std::cos(a)) * r * flat * bump, std::sin(a) * std::sin(b) * r * bump);
    };
    for (int i = 0; i < rings; ++i)
        for (int k = 0; k < sides; ++k) {
            const glm::vec3 p[4] = {point(i, k), point(i, k + 1), point(i + 1, k + 1), point(i + 1, k)};
            const glm::vec3 n = glm::normalize(glm::cross(p[2] - p[0], p[1] - p[0]));
            const glm::vec2 uv[4] = {{float(k) / sides, float(i) / rings}, {float(k + 1) / sides, float(i) / rings},
                                     {float(k + 1) / sides, float(i + 1) / rings}, {float(k) / sides, float(i + 1) / rings}};
            for (int j : {0, 1, 2, 0, 2, 3}) m.parts[Stone].push_back({p[j] - glm::vec3(0, r * flat * 0.25f, 0), n, uv[j] * 2.f});
        }
    return m;
}

Model weeds(float size, unsigned seed) {
    Model m;
    Random random{seed * 2654435761u + 3u};
    random.next();
    // A tuft of blades: thin tapering quads fanning out from the centre.
    const int blades = 26;
    for (int i = 0; i < blades; ++i) {
        const float a = random.range(0, 2 * Pi), lean = random.range(0.1f, 0.6f), len = size * random.range(0.5f, 1.f);
        const glm::vec3 base(std::cos(a) * size * 0.1f, 0, std::sin(a) * size * 0.1f);
        const glm::vec3 dir = glm::normalize(glm::vec3(std::cos(a) * lean, 1, std::sin(a) * lean));
        const glm::vec3 side = glm::normalize(glm::cross(dir, glm::vec3(std::cos(a), 0, std::sin(a)) + glm::vec3(0.01f, 0, 0))) * 0.012f;
        const glm::vec3 tip = base + dir * len;
        const glm::vec3 n = glm::normalize(glm::cross(side, dir));
        const glm::vec3 p[3] = {base - side, base + side, tip};
        const glm::vec2 uv[3] = {{0, 0}, {1, 0}, {0.5f, 1}};
        for (int j = 0; j < 3; ++j) m.parts[Blades].push_back({p[j], n, uv[j]});
    }
    return m;
}

Model bush(float size, unsigned seed) {
    Model m;
    Random random{seed * 2654435761u + 5u};
    random.next();
    const float r = size * 0.5f;
    // A few stems, and leaf cards filling a squashed sphere.
    for (int i = 0; i < 5; ++i) {
        const float a = random.range(0, 2 * Pi);
        lathe(m.parts[DarkWood], {{0.02f, 0}, {0.012f, r * 0.9f}}, 5, glm::vec3(std::cos(a) * 0.05f, 0, std::sin(a) * 0.05f));
    }
    for (int i = 0; i < 40; ++i) {
        glm::vec3 d;
        do { d = glm::vec3(random.range(-1, 1), random.range(-1, 1), random.range(-1, 1)); } while (glm::dot(d, d) > 1.f);
        d *= 0.6f + 0.4f * std::cbrt(random.next());
        card(m.parts[Leaves], glm::vec3(d.x * r, r * 0.9f + d.y * r * 0.7f, d.z * r), r * random.range(0.7f, 1.1f), random.range(0, Pi), random.range(-0.6f, 0.6f));
    }
    return m;
}

Model pottedPlant(float size, unsigned seed) {
    Model m = bush(size * 0.8f, seed);
    const float pot = size * 0.32f;
    for (auto& part : m.parts)
        for (auto& v : part) v.position.y += pot * 0.9f;
    lathe(m.parts[Clay], {{0, 0}, {pot * 0.55f, 0}, {pot * 0.7f, pot * 0.8f}, {pot * 0.8f, pot * 0.85f}, {pot * 0.8f, pot}, {pot * 0.68f, pot}, {0, pot * 0.9f}}, 14, glm::vec3(0));
    return m;
}

Model evergreen(float height, unsigned seed) {
    Model m;
    Random random{seed * 2654435761u + 11u};
    random.next();
    const float trunk = 0.1f + height * 0.015f;
    lathe(m.parts[DarkWood], {{trunk * 1.3f, 0}, {trunk, 0.4f}, {trunk * 0.4f, height * 0.9f}}, 8, glm::vec3(0));
    // Tiers of drooping needle cards, wider at the bottom.
    const int tiers = 7;
    for (int t = 0; t < tiers; ++t) {
        const float y = height * (0.2f + 0.72f * t / tiers), spread = height * 0.36f * (1.f - float(t) / tiers) + 0.25f;
        const int cards = 9 - t / 2;
        for (int k = 0; k < cards; ++k) {
            const float a = 2 * Pi * (k + random.next() * 0.5f) / cards;
            const glm::vec3 c(std::cos(a) * spread * 0.55f, y - spread * 0.15f, std::sin(a) * spread * 0.55f);
            card(m.parts[Needles], c, spread * 1.1f, a + Pi / 2, 0.5f);
        }
    }
    card(m.parts[Needles], glm::vec3(0, height * 0.95f, 0), height * 0.18f, random.range(0, Pi), 0.f);
    return m;
}

}
