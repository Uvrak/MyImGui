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

// Small things, centred at the origin on the surface they stand on.

Model candle(bool lit, float stand) {
    // A candle in a pewter holder; on the floor (stand > 0) on an iron candle stand that high.
    Model m;
    const float h = 0.16f, s = stand;
    if (s > 0) {
        lathe(m.parts[Iron], {{0, 0}, {0.16f, 0}, {0.16f, 0.03f}, {0.03f, 0.06f}, {0.018f, 0.1f}, {0.018f, s - 0.02f}, {0.06f, s - 0.01f}, {0.06f, s}, {0, s}},
              12, glm::vec3(0));
    }
    lathe(m.parts[Pewter], {{0, s}, {0.05f, s}, {0.05f, s + 0.012f}, {0.02f, s + 0.02f}, {0.02f, s + 0.04f}, {0, s + 0.04f}}, 12, glm::vec3(0));
    lathe(m.parts[Linen], {{0.014f, s + 0.04f}, {0.014f, s + 0.04f + h}, {0, s + 0.04f + h}}, 10, glm::vec3(0));
    if (lit) lathe(m.parts[Flame], {{0, 0}, {0.009f, 0.012f}, {0.006f, 0.03f}, {0, 0.045f}}, 8, glm::vec3(0, s + 0.045f + h, 0));
    return m;
}

Model sconce(bool lit) {
    // A wall plate (on the wall at z = 0, facing +z), an arm and a candle on a drip pan.
    Model m;
    box(m.parts[Iron], {-0.05f, 1.45f, 0}, {0.05f, 1.7f, 0.015f});
    box(m.parts[Iron], {-0.012f, 1.5f, 0.015f}, {0.012f, 1.53f, 0.16f});
    lathe(m.parts[Iron], {{0, 0}, {0.05f, 0}, {0.055f, 0.02f}, {0.045f, 0.02f}, {0, 0.012f}}, 12, glm::vec3(0, 1.52f, 0.16f));
    const float h = lit ? 0.14f : 0.05f;
    lathe(m.parts[Linen], {{0.016f, 0}, {0.016f, h}, {0, h}}, 10, glm::vec3(0, 1.535f, 0.16f));
    if (lit) lathe(m.parts[Flame], {{0, 0}, {0.01f, 0.014f}, {0.007f, 0.034f}, {0, 0.05f}}, 8, glm::vec3(0, 1.535f + h, 0.16f));
    return m;
}

Model cup() {
    Model m;
    lathe(m.parts[Pewter], {{0, 0}, {0.03f, 0}, {0.032f, 0.004f}, {0.012f, 0.02f}, {0.01f, 0.05f}, {0.04f, 0.07f}, {0.045f, 0.12f},
                            {0.04f, 0.12f}, {0.035f, 0.075f}, {0, 0.07f}}, 14, glm::vec3(0));
    return m;
}

Model plate() {
    Model m;
    lathe(m.parts[Pewter], {{0, 0.004f}, {0.08f, 0}, {0.13f, 0.012f}, {0.135f, 0.02f}, {0.12f, 0.016f}, {0.08f, 0.008f}, {0, 0.008f}}, 20, glm::vec3(0));
    return m;
}

Model pitcher() {
    Model m;
    lathe(m.parts[Clay], {{0, 0}, {0.06f, 0}, {0.085f, 0.08f}, {0.07f, 0.17f}, {0.05f, 0.21f}, {0.06f, 0.24f}, {0.05f, 0.24f}, {0, 0.2f}}, 16, glm::vec3(0));
    box(m.parts[Clay], {0.07f, 0.07f, -0.012f}, {0.11f, 0.2f, 0.012f});
    return m;
}

Model bottle(unsigned seed) {
    Model m;
    const float s = 0.9f + 0.1f * float(seed % 3);
    lathe(m.parts[Glass], {{0, 0}, {0.04f * s, 0}, {0.042f * s, 0.14f * s}, {0.016f, 0.2f * s}, {0.014f, 0.27f * s}, {0.018f, 0.28f * s}, {0, 0.28f * s}}, 12, glm::vec3(0));
    lathe(m.parts[DarkWood], {{0.012f, 0.28f * s}, {0.012f, 0.3f * s}, {0, 0.3f * s}}, 8, glm::vec3(0));
    return m;
}

Model book(unsigned seed) {
    // A closed book lying flat: cover boards around a paper block, the spine rounded.
    Model m;
    const float w = 0.22f, d = 0.16f, t = 0.045f + 0.02f * float(seed % 3);
    box(m.parts[Cloth], {-w / 2, 0, -d / 2}, {w / 2, 0.006f, d / 2});
    box(m.parts[Cloth], {-w / 2, t - 0.006f, -d / 2}, {w / 2, t, d / 2});
    box(m.parts[Cloth], {-w / 2 - 0.004f, 0, -d / 2}, {-w / 2 + 0.008f, t, d / 2});
    box(m.parts[Linen], {-w / 2 + 0.008f, 0.006f, -d / 2 + 0.005f}, {w / 2 - 0.006f, t - 0.006f, d / 2 - 0.005f});
    return m;
}

Model scroll() {
    Model m;
    constexpr int sides = 10;
    const float r = 0.028f, len = 0.26f;
    // A rolled parchment along x, with wooden rod ends.
    for (int i = 0; i < sides; ++i) {
        const float a0 = 6.2831853f * i / sides, a1 = 6.2831853f * (i + 1) / sides;
        const glm::vec3 n0(0, std::sin(a0), std::cos(a0)), n1(0, std::sin(a1), std::cos(a1));
        const glm::vec3 p[4] = {glm::vec3(-len / 2, r, 0) + n0 * r, glm::vec3(len / 2, r, 0) + n0 * r, glm::vec3(len / 2, r, 0) + n1 * r, glm::vec3(-len / 2, r, 0) + n1 * r};
        const glm::vec3 n[4] = {n0, n0, n1, n1};
        const glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        for (int j : {0, 1, 2, 0, 2, 3}) m.parts[Linen].push_back({p[j], n[j], uv[j]});
    }
    for (const float x : {-len / 2 - 0.02f, len / 2}) box(m.parts[DarkWood], {x, r - 0.012f, -0.012f}, {x + 0.02f, r + 0.012f, 0.012f});
    return m;
}

Model bag(float size, unsigned seed) {
    // A sack tied at the neck, slumped a little.
    Model m;
    const float s = size * (0.9f + 0.1f * float(seed % 3));
    lathe(m.parts[Burlap], {{0, 0}, {s * 0.4f, 0}, {s * 0.5f, s * 0.2f}, {s * 0.45f, s * 0.55f}, {s * 0.12f, s * 0.8f}, {s * 0.1f, s * 0.85f},
                            {s * 0.2f, s * 1.0f}, {0, s * 0.95f}}, 14, glm::vec3(0));
    return m;
}

Model bucket() {
    Model m;
    lathe(m.parts[Wood], {{0, 0.01f}, {0.12f, 0}, {0.15f, 0.28f}, {0.14f, 0.28f}, {0.11f, 0.02f}, {0, 0.02f}}, 16, glm::vec3(0));
    for (const float y : {0.05f, 0.23f}) lathe(m.parts[Iron], {{0.125f + y * 0.1f, y}, {0.127f + y * 0.1f, y + 0.02f}}, 16, glm::vec3(0));
    // The bail handle.
    for (int i = 0; i < 8; ++i) {
        const float a0 = 3.14159f * i / 8, a1 = 3.14159f * (i + 1) / 8;
        box(m.parts[Iron], {-0.15f * std::cos(a0) - 0.005f, 0.28f + 0.12f * std::sin(a0), -0.005f}, {-0.15f * std::cos(a1) + 0.005f, 0.28f + 0.12f * std::sin(a1) + 0.008f, 0.005f});
    }
    return m;
}

Model pot() {
    // An iron cooking pot on three stubby legs, with a lid.
    Model m;
    lathe(m.parts[Iron], {{0, 0.06f}, {0.14f, 0.07f}, {0.19f, 0.18f}, {0.17f, 0.3f}, {0.16f, 0.3f}, {0, 0.32f}}, 18, glm::vec3(0));
    for (int i = 0; i < 3; ++i) {
        const float a = 6.2831853f * i / 3;
        box(m.parts[Iron], {std::cos(a) * 0.1f - 0.015f, 0, std::sin(a) * 0.1f - 0.015f}, {std::cos(a) * 0.1f + 0.015f, 0.08f, std::sin(a) * 0.1f + 0.015f});
    }
    return m;
}

Model boots() {
    // A pair of leather boots standing side by side.
    Model m;
    for (const float x : {-0.07f, 0.07f}) {
        box(m.parts[Leather], {x - 0.045f, 0, -0.13f}, {x + 0.045f, 0.06f, 0.13f});
        box(m.parts[Leather], {x - 0.05f, 0.06f, -0.13f}, {x + 0.05f, 0.34f, -0.02f});
        box(m.parts[DarkWood], {x - 0.046f, 0, -0.131f}, {x + 0.046f, 0.02f, 0.131f});
    }
    return m;
}

Model horseshoe() {
    Model m;
    constexpr int segments = 12;
    for (int i = 0; i < segments; ++i) {
        const float a0 = -0.6f + (3.14159f + 1.2f) * i / segments, a1 = -0.6f + (3.14159f + 1.2f) * (i + 1) / segments;
        const glm::vec2 p0(std::cos(a0) * 0.06f, std::sin(a0) * 0.06f), p1(std::cos(a1) * 0.06f, std::sin(a1) * 0.06f);
        box(m.parts[Iron], {std::min(p0.x, p1.x) - 0.009f, 0, std::min(p0.y, p1.y) - 0.009f}, {std::max(p0.x, p1.x) + 0.009f, 0.012f, std::max(p0.y, p1.y) + 0.009f});
    }
    return m;
}

Model pillar(float width, float depth, float height) {
    // A stone column: square plinth, round shaft with entasis, capital, square abacus.
    Model m;
    const float r = std::min(width, depth) * 0.36f;
    box(m.parts[Stone], {-width / 2, 0, -depth / 2}, {width / 2, 0.18f, depth / 2});
    lathe(m.parts[Stone], {{r * 1.15f, 0.18f}, {r * 1.1f, 0.26f}, {r, 0.3f}, {r * 1.03f, height * 0.4f}, {r * 0.92f, height - 0.34f},
                           {r * 1.2f, height - 0.22f}, {r * 1.25f, height - 0.18f}}, 20, glm::vec3(0));
    box(m.parts[Stone], {-width / 2, height - 0.18f, -depth / 2}, {width / 2, height, depth / 2});
    return m;
}

Model haystack(float width, float depth, float height, unsigned seed) {
    // A rounded heap of hay: a squashed dome with straw tufts.
    Model m = bush(std::max(width, depth) * 0.9f, seed);
    for (auto& v : m.parts[Leaves]) v.position.y *= height / std::max(width, depth);
    m.parts[Straw] = std::move(m.parts[Leaves]);
    m.parts[Leaves].clear();
    m.parts[DarkWood].clear();
    lathe(m.parts[Straw], {{0, 0}, {width * 0.48f, 0}, {width * 0.45f, height * 0.35f}, {width * 0.3f, height * 0.75f}, {0, height * 0.85f}}, 16, glm::vec3(0));
    return m;
}


// Weapons and armour lie flat, along x, centred.

Model blade(float length, bool hilt) {
    Model m;
    const float grip = hilt ? std::min(0.22f, length * 0.25f) : 0.f, x0 = -length / 2, bladeStart = x0 + grip + (hilt ? 0.03f : 0.f);
    const float w = std::clamp(length * 0.05f, 0.022f, 0.05f);
    if (hilt) {
        box(m.parts[Leather], {x0 + 0.03f, 0.005f, -0.014f}, {x0 + grip, 0.03f, 0.014f});
        box(m.parts[Iron], {x0, 0.004f, -0.02f}, {x0 + 0.03f, 0.032f, 0.02f});                         // pommel
        box(m.parts[Iron], {x0 + grip, 0.006f, -w * 2.2f}, {x0 + grip + 0.03f, 0.028f, w * 2.2f});   // guard
    }
    // The blade: flat, with a point.
    const float tip = length / 2, taper = tip - w * 2.5f, y0 = 0.012f, y1 = 0.02f;
    box(m.parts[Steel], {bladeStart, y0, -w / 2}, {taper, y1, w / 2});
    const glm::vec3 p[3] = {{taper, y1, -w / 2}, {taper, y1, w / 2}, {tip, y1, 0}};
    for (int j = 0; j < 3; ++j) m.parts[Steel].push_back({p[j], glm::vec3(0, 1, 0), glm::vec2(0)});
    return m;
}

Model hafted(float length, int head) {
    // A haft with a head: 0 mace (iron ball with flanges), 1 morning star (spiked ball),
    // 2 club (thick wooden end), 3 hammer (iron block), 4 two handed axe (bearded blade).
    Model m;
    const float x0 = -length / 2, x1 = length / 2, r = head == 2 ? 0.03f : 0.018f;
    box(m.parts[head == 2 ? Wood : DarkWood], {x0, 0.02f - r, -r}, {x1 - (head == 2 ? 0.f : 0.1f), 0.02f + r, r});
    if (head == 0 || head == 1) {
        box(m.parts[Iron], {x1 - 0.14f, 0.0f, -0.05f}, {x1, 0.1f, 0.05f});
        if (head == 1)
            for (int k = 0; k < 6; ++k) {
                const float a = 6.2831853f * k / 6;
                box(m.parts[Steel], {x1 - 0.08f + std::cos(a) * 0.06f - 0.008f, 0.05f + std::sin(a) * 0.06f - 0.008f, -0.07f},
                    {x1 - 0.08f + std::cos(a) * 0.06f + 0.008f, 0.05f + std::sin(a) * 0.06f + 0.008f, 0.07f});
            }
    } else if (head == 2) {
        box(m.parts[Wood], {x1 - 0.2f, 0.0f, -0.05f}, {x1, 0.09f, 0.05f});
    } else if (head == 3) {
        box(m.parts[Iron], {x1 - 0.1f, 0.0f, -0.08f}, {x1 - 0.02f, 0.06f, 0.08f});
    } else {
        box(m.parts[Steel], {x1 - 0.2f, 0.012f, -0.03f}, {x1 - 0.04f, 0.024f, 0.22f});
        box(m.parts[Iron], {x1 - 0.14f, 0.0f, -0.06f}, {x1 - 0.08f, 0.04f, 0.03f});
    }
    return m;
}

Model tongs() {
    Model m;
    for (const float z : {-0.02f, 0.02f}) box(m.parts[Iron], {-0.25f, 0, z - 0.008f}, {0.25f, 0.016f, z + 0.008f});
    box(m.parts[Iron], {-0.02f, 0, -0.03f}, {0.02f, 0.02f, 0.03f});
    return m;
}

Model shield(float radius, bool wooden) {
    // A round shield lying on its back: boards (or iron), a rim and a boss.
    Model m;
    lathe(m.parts[wooden ? Wood : Iron], {{0, 0}, {radius, 0}, {radius, 0.03f}, {0, 0.04f}}, 20, glm::vec3(0));
    lathe(m.parts[Iron], {{radius, 0.028f}, {radius * 1.02f, 0.034f}, {radius * 0.92f, 0.04f}}, 20, glm::vec3(0));
    lathe(m.parts[Iron], {{radius * 0.22f, 0.04f}, {radius * 0.18f, 0.08f}, {0, 0.1f}}, 14, glm::vec3(0));
    return m;
}

Model helm(bool leather) {
    Model m;
    lathe(m.parts[leather ? Leather : Iron], {{0.12f, 0}, {0.125f, 0.02f}, {0.12f, 0.1f}, {0.09f, 0.17f}, {0.04f, 0.2f}, {0, 0.21f}}, 16, glm::vec3(0));
    return m;
}

Model armour() {
    // A leather jerkin lying flat: body, shoulders and straps.
    Model m;
    box(m.parts[Leather], {-0.22f, 0, -0.26f}, {0.22f, 0.06f, 0.26f});
    for (const float x : {-0.34f, 0.22f}) box(m.parts[Leather], {x, 0, -0.24f}, {x + 0.12f, 0.05f, -0.08f});
    for (const float z : {-0.1f, 0.05f, 0.18f}) box(m.parts[DarkWood], {-0.225f, 0.02f, z}, {0.225f, 0.065f, z + 0.025f});
    return m;
}

Model gloves() {
    Model m;
    for (const float x : {-0.07f, 0.07f}) {
        box(m.parts[Leather], {x - 0.05f, 0, -0.1f}, {x + 0.05f, 0.03f, 0.06f});
        box(m.parts[Leather], {x - 0.06f, 0, 0.06f}, {x + 0.06f, 0.04f, 0.14f});
    }
    return m;
}

Model clothHeap(float size, unsigned seed) {
    // A folded cloak, hood or bolt of cloth: a low soft heap.
    Model m = bag(size, seed);
    for (auto& v : m.parts[Burlap]) { v.position.y *= 0.3f; v.position.x *= 1.3f; }
    m.parts[Cloth] = std::move(m.parts[Burlap]);
    m.parts[Burlap].clear();
    return m;
}

Model bread(unsigned seed) {
    Model m;
    const float l = 0.12f + 0.03f * float(seed % 3);
    lathe(m.parts[Food], {{0, 0}, {0.06f, 0}, {0.075f, 0.03f}, {0.06f, 0.06f}, {0, 0.07f}}, 14, glm::vec3(0));
    for (auto& v : m.parts[Food]) v.position.x *= l / 0.075f;
    return m;
}

Model potion(unsigned seed) {
    Model m;
    lathe(m.parts[Cloth], {{0, 0}, {0.03f, 0}, {0.035f, 0.05f}, {0.012f, 0.08f}, {0.01f, 0.11f}, {0, 0.11f}}, 12, glm::vec3(0));
    lathe(m.parts[DarkWood], {{0.009f, 0.11f}, {0.009f, 0.125f}, {0, 0.125f}}, 8, glm::vec3(0));
    (void)seed;
    return m;
}

Model shards() {
    Model m;
    const float s[3][4] = {{-0.06f, -0.03f, -0.01f, 0.03f}, {0.0f, 0.01f, 0.06f, 0.05f}, {0.02f, -0.06f, 0.07f, -0.02f}};
    for (const auto& q : s) box(m.parts[Pewter], {q[0], 0, q[1]}, {q[2], 0.008f, q[3]});
    return m;
}

Model inkwell() {
    Model m;
    lathe(m.parts[Glass], {{0, 0}, {0.035f, 0}, {0.035f, 0.04f}, {0.015f, 0.05f}, {0, 0.05f}}, 12, glm::vec3(0));
    box(m.parts[Linen], {-0.005f, 0.04f, -0.004f}, {0.005f, 0.22f, 0.004f});
    return m;
}

Model coins() {
    Model m;
    for (int i = 0; i < 5; ++i) lathe(m.parts[Gold], {{0, i * 0.004f}, {0.02f, i * 0.004f}, {0.02f, i * 0.004f + 0.004f}, {0, i * 0.004f + 0.004f}}, 12,
                                      glm::vec3(i % 2 ? 0.004f : -0.003f, 0, i % 3 ? 0.003f : -0.002f));
    return m;
}

Model utensils() {
    Model m;
    box(m.parts[Pewter], {-0.1f, 0, -0.04f}, {0.1f, 0.006f, -0.028f});
    box(m.parts[Pewter], {-0.1f, 0, 0.02f}, {0.06f, 0.006f, 0.03f});
    lathe(m.parts[Pewter], {{0, 0}, {0.022f, 0.004f}, {0.024f, 0.01f}, {0, 0.006f}}, 10, glm::vec3(0.08f, 0, 0.025f));
    return m;
}

Model top() {
    Model m;
    lathe(m.parts[Wood], {{0, 0}, {0.03f, 0.03f}, {0.035f, 0.045f}, {0.006f, 0.05f}, {0.006f, 0.07f}, {0, 0.07f}}, 12, glm::vec3(0));
    return m;
}

// Larger things, built along x, footprint centred.

Model anvil() {
    Model m;
    lathe(m.parts[Wood], {{0.2f, 0}, {0.19f, 0.45f}, {0, 0.46f}}, 12, glm::vec3(0));
    box(m.parts[Iron], {-0.13f, 0.46f, -0.08f}, {0.13f, 0.56f, 0.08f});
    box(m.parts[Iron], {-0.08f, 0.56f, -0.05f}, {0.08f, 0.64f, 0.05f});
    box(m.parts[Iron], {-0.2f, 0.64f, -0.07f}, {0.18f, 0.74f, 0.07f});
    box(m.parts[Iron], {0.18f, 0.67f, -0.04f}, {0.32f, 0.73f, 0.04f});
    box(m.parts[Iron], {0.32f, 0.685f, -0.025f}, {0.4f, 0.72f, 0.025f});
    return m;
}

Model stove(float width, float depth, float height) {
    // An iron stove on a stone hearth, with its flue pipe.
    Model m;
    box(m.parts[Stone], {-width / 2, 0, -depth / 2}, {width / 2, 0.12f, depth / 2});
    box(m.parts[Iron], {-width / 2 + 0.08f, 0.12f, -depth / 2 + 0.08f}, {width / 2 - 0.08f, height, depth / 2 - 0.08f});
    box(m.parts[Flame], {-width / 4, 0.25f, depth / 2 - 0.085f}, {width / 4, 0.45f, depth / 2 - 0.075f});
    lathe(m.parts[Iron], {{0.07f, height}, {0.07f, height + 1.4f}, {0, height + 1.4f}}, 10, glm::vec3(width / 4, 0, 0));
    return m;
}

Model firepit(float size) {
    Model m;
    const float r = size * 0.4f;
    for (int i = 0; i < 12; ++i) {
        const float a = 6.2831853f * i / 12;
        box(m.parts[Stone], {std::cos(a) * r - 0.1f, 0, std::sin(a) * r - 0.1f}, {std::cos(a) * r + 0.1f, 0.14f, std::sin(a) * r + 0.1f});
    }
    for (int i = 0; i < 4; ++i) {
        const float o = (i % 2 ? 0.08f : -0.08f), y = 0.02f + (i / 2) * 0.07f, l = r * 0.75f;
        if (i / 2 == 0) box(m.parts[DarkWood], {-l, y, o - 0.04f}, {l, y + 0.07f, o + 0.04f});
        else box(m.parts[DarkWood], {o - 0.04f, y, -l}, {o + 0.04f, y + 0.07f, l});
    }
    lathe(m.parts[Flame], {{0, 0}, {r * 0.4f, 0.08f}, {r * 0.2f, 0.35f}, {0, 0.55f}}, 8, glm::vec3(0, 0.2f, 0));
    return m;
}

Model easel(bool withPalette) {
    Model m;
    // Two front legs and one back leg, a ledge and a canvas.
    for (const float x : {-0.28f, 0.28f}) box(m.parts[Wood], {x - 0.02f, 0, -0.02f}, {x + 0.02f, 1.7f, 0.02f});
    box(m.parts[Wood], {-0.02f, 0, -0.45f}, {0.02f, 1.6f, -0.41f});
    box(m.parts[Wood], {-0.35f, 0.75f, 0.0f}, {0.35f, 0.79f, 0.06f});
    box(m.parts[Linen], {-0.3f, 0.79f, 0.03f}, {0.3f, 1.45f, 0.05f});
    if (withPalette) lathe(m.parts[Wood], {{0, 0}, {0.12f, 0}, {0.12f, 0.01f}, {0, 0.01f}}, 14, glm::vec3(0.5f, 0, 0.2f));
    return m;
}

Model mirror() {
    // A standing mirror: a wooden frame on feet, the glass bright.
    Model m;
    for (const float x : {-0.3f, 0.3f}) box(m.parts[DarkWood], {x - 0.03f, 0, -0.2f}, {x + 0.03f, 0.05f, 0.2f});
    for (const float x : {-0.3f, 0.3f}) box(m.parts[DarkWood], {x - 0.025f, 0, -0.025f}, {x + 0.025f, 1.75f, 0.025f});
    box(m.parts[DarkWood], {-0.3f, 0.35f, -0.03f}, {0.3f, 0.4f, 0.03f});
    box(m.parts[DarkWood], {-0.3f, 1.7f, -0.03f}, {0.3f, 1.75f, 0.03f});
    box(m.parts[Pewter], {-0.275f, 0.4f, -0.01f}, {0.275f, 1.7f, 0.01f});
    return m;
}

Model sundial() {
    Model m;
    lathe(m.parts[Stone], {{0.22f, 0}, {0.22f, 0.1f}, {0.12f, 0.15f}, {0.1f, 0.85f}, {0.25f, 0.9f}, {0.25f, 0.96f}, {0, 0.96f}}, 16, glm::vec3(0));
    const glm::vec3 p[3] = {{-0.15f, 0.96f, 0}, {0.15f, 0.96f, 0}, {-0.15f, 1.12f, 0}};
    for (const float z : {-0.006f, 0.006f})
        for (int j = 0; j < 3; ++j) m.parts[Gold].push_back({p[j] + glm::vec3(0, 0, z), glm::vec3(0, 0, z > 0 ? 1.f : -1.f), glm::vec2(0)});
    return m;
}

Model podium() {
    // A lectern: a post on a foot, the sloped reading desk on top.
    Model m;
    box(m.parts[DarkWood], {-0.25f, 0, -0.2f}, {0.25f, 0.06f, 0.2f});
    box(m.parts[Wood], {-0.06f, 0.06f, -0.06f}, {0.06f, 1.0f, 0.06f});
    const float y0 = 1.0f, y1 = 1.2f;
    const glm::vec3 c[4] = {{-0.28f, y0, 0.2f}, {0.28f, y0, 0.2f}, {0.28f, y1, -0.2f}, {-0.28f, y1, -0.2f}};
    const glm::vec3 n = glm::normalize(glm::vec3(0, 0.4f, 0.2f));
    for (int j : {0, 1, 2, 0, 2, 3}) m.parts[Wood].push_back({c[j], n, glm::vec2(c[j].x, c[j].z)});
    box(m.parts[Wood], {-0.28f, y0 - 0.04f, -0.2f}, {0.28f, y0, 0.2f});
    return m;
}

Model pedestal() {
    Model m;
    box(m.parts[Stone], {-0.3f, 0, -0.3f}, {0.3f, 0.12f, 0.3f});
    box(m.parts[Stone], {-0.22f, 0.12f, -0.22f}, {0.22f, 0.9f, 0.22f});
    box(m.parts[Stone], {-0.3f, 0.9f, -0.3f}, {0.3f, 1.0f, 0.3f});
    return m;
}

Model trough(float length, float width) {
    Model m;
    const float h = 0.5f, t = 0.05f;
    box(m.parts[Wood], {-length / 2, 0.1f, -width / 2}, {length / 2, 0.16f, width / 2});
    box(m.parts[Wood], {-length / 2, 0.1f, -width / 2}, {length / 2, h, -width / 2 + t});
    box(m.parts[Wood], {-length / 2, 0.1f, width / 2 - t}, {length / 2, h, width / 2});
    box(m.parts[Wood], {-length / 2, 0.1f, -width / 2}, {-length / 2 + t, h, width / 2});
    box(m.parts[Wood], {length / 2 - t, 0.1f, -width / 2}, {length / 2, h, width / 2});
    for (const float x : {-length / 2 + 0.1f, length / 2 - 0.2f}) box(m.parts[DarkWood], {x, 0, -width / 2}, {x + 0.1f, 0.1f, width / 2});
    box(m.parts[Water], {-length / 2 + t, h - 0.08f, -width / 2 + t}, {length / 2 - t, h - 0.07f, width / 2 - t});
    return m;
}

Model lever() {
    Model m;
    box(m.parts[DarkWood], {-0.15f, 0, -0.12f}, {0.15f, 0.2f, 0.12f});
    box(m.parts[Iron], {-0.02f, 0.2f, -0.02f}, {0.02f, 0.75f, 0.02f});
    lathe(m.parts[Iron], {{0, 0}, {0.04f, 0.02f}, {0.04f, 0.06f}, {0, 0.08f}}, 10, glm::vec3(0, 0.74f, 0));
    return m;
}

Model ironBars(float length, float height) {
    Model m;
    const int bars = std::max(2, int(length / 0.14f));
    for (int i = 0; i <= bars; ++i) {
        const float x = -length / 2 + length * i / bars;
        box(m.parts[Iron], {x - 0.015f, 0, -0.015f}, {x + 0.015f, height, 0.015f});
    }
    for (const float y : {0.15f, height * 0.5f, height - 0.12f}) box(m.parts[Iron], {-length / 2, y, -0.025f}, {length / 2, y + 0.04f, 0.025f});
    return m;
}

Model flag(unsigned seed) {
    // A pole with a red banner rippling in the wind (along x).
    Model m;
    lathe(m.parts[DarkWood], {{0.035f, 0}, {0.03f, 3.2f}, {0, 3.25f}}, 8, glm::vec3(0));
    constexpr int strips = 10;
    for (int i = 0; i < strips; ++i) {
        const float s0 = float(i) / strips, s1 = float(i + 1) / strips;
        auto z = [&](float s) { return 0.12f * s * std::sin(s * 7.f + float(seed)); };
        const glm::vec3 p[4] = {{0.03f + 1.3f * s0, 3.1f, z(s0)}, {0.03f + 1.3f * s1, 3.1f, z(s1)}, {0.03f + 1.3f * s1, 2.3f, z(s1)}, {0.03f + 1.3f * s0, 2.3f, z(s0)}};
        const glm::vec3 n = glm::normalize(glm::vec3(-(z(s1) - z(s0)) / (1.3f / strips), 0, 1));
        const glm::vec2 uv[4] = {{s0, 0}, {s1, 0}, {s1, 1}, {s0, 1}};
        for (int j : {0, 1, 2, 0, 2, 3}) m.parts[Cloth].push_back({p[j], n, uv[j]});
    }
    return m;
}

Model basket() {
    Model m;
    lathe(m.parts[Straw], {{0, 0}, {0.13f, 0}, {0.18f, 0.2f}, {0.17f, 0.2f}, {0.12f, 0.02f}, {0, 0.02f}}, 16, glm::vec3(0));
    return m;
}

Model bellows() {
    Model m;
    const glm::vec3 a[3] = {{-0.3f, 0.1f, -0.15f}, {0.25f, 0.1f, 0}, {-0.3f, 0.1f, 0.15f}};
    for (const float y : {0.1f, 0.3f})
        for (int j = 0; j < 3; ++j) m.parts[Wood].push_back({a[j] + glm::vec3(0, y - 0.1f, 0), glm::vec3(0, 1, 0), glm::vec2(0)});
    box(m.parts[Leather], {-0.28f, 0.1f, -0.12f}, {0.12f, 0.3f, 0.12f});
    box(m.parts[Iron], {0.25f, 0.17f, -0.02f}, {0.45f, 0.21f, 0.02f});
    box(m.parts[DarkWood], {-0.5f, 0.28f, -0.02f}, {-0.3f, 0.32f, 0.02f});
    for (const float x : {-0.25f, 0.1f}) box(m.parts[DarkWood], {x, 0, -0.12f}, {x + 0.05f, 0.1f, 0.12f});
    return m;
}

Model wagon(float length, float width) {
    // A farm wagon along x, the shafts towards +x: bed with side boards on four spoked wheels.
    Model m;
    const float shafts = std::min(1.4f, length * 0.3f), body = length - shafts, x0 = -length / 2, x1 = x0 + body, w = width * 0.8f;
    const float bed = 0.75f, r = 0.45f;
    box(m.parts[Wood], {x0 + 0.1f, bed, -w / 2}, {x1 - 0.1f, bed + 0.06f, w / 2});
    for (const float z : {-w / 2, w / 2 - 0.04f}) box(m.parts[Wood], {x0 + 0.1f, bed, z}, {x1 - 0.1f, bed + 0.45f, z + 0.04f});
    for (const float x : {x0 + 0.1f, x1 - 0.14f}) box(m.parts[Wood], {x, bed, -w / 2}, {x + 0.04f, bed + 0.4f, w / 2});
    for (const float x : {x0 + 0.1f, x1 - 0.14f}) box(m.parts[DarkWood], {x - 0.03f, r - 0.03f, -w / 2 - 0.12f}, {x + 0.07f, r + 0.03f, w / 2 + 0.12f});   // axles
    for (const float x : {x0 + 0.12f + r * 0.2f, x1 - 0.12f - r * 0.2f})
        for (const float z : {-w / 2 - 0.12f, w / 2 + 0.06f}) {
            // A wheel: rim of 16 segments, 8 spokes and a hub, upright in the x/y plane.
            for (int i = 0; i < 16; ++i) {
                const float a0 = 6.2831853f * i / 16, a1 = 6.2831853f * (i + 1) / 16;
                const glm::vec3 p0(x + std::cos(a0) * r, r + std::sin(a0) * r, z), p1(x + std::cos(a1) * r, r + std::sin(a1) * r, z);
                box(m.parts[DarkWood], {std::min(p0.x, p1.x) - 0.03f, std::min(p0.y, p1.y) - 0.03f, z}, {std::max(p0.x, p1.x) + 0.03f, std::max(p0.y, p1.y) + 0.03f, z + 0.06f});
            }
            for (int i = 0; i < 8; ++i) {
                const float a = 6.2831853f * i / 8;
                const glm::vec3 d(std::cos(a), std::sin(a), 0), s(-d.y * 0.02f, d.x * 0.02f, 0), c(x, r, z + 0.03f);
                const glm::vec3 p[4] = {c - s, c + d * r - s, c + d * r + s, c + s};
                for (int j : {0, 1, 2, 0, 2, 3}) m.parts[Wood].push_back({p[j], glm::vec3(0, 0, 1), glm::vec2(0)});
            }
            box(m.parts[Iron], {x - 0.07f, r - 0.07f, z - 0.01f}, {x + 0.07f, r + 0.07f, z + 0.07f});
        }
    // Shafts.
    for (const float z : {-w / 2 + 0.1f, w / 2 - 0.14f}) box(m.parts[DarkWood], {x1 - 0.2f, bed - 0.1f, z}, {length / 2, bed - 0.04f, z + 0.05f});
    return m;
}


}
