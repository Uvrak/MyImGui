#include "TreeModel.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace TreeModel {
namespace {

constexpr float Pi = 3.14159265f;

struct Random {
    std::uint32_t state;
    float next() { state = state * 1664525u + 1013904223u; std::uint32_t h = state ^ (state >> 16); h *= 0x7feb352du; h ^= h >> 15; return float(h & 0xffffff) / float(0xffffff); }
    float range(float a, float b) { return a + (b - a) * next(); }
};

// Any vector perpendicular to d.
glm::vec3 perpendicular(glm::vec3 d) {
    const glm::vec3 a = std::abs(d.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    return glm::normalize(glm::cross(d, a));
}

void tube(std::vector<Vertex>& out, glm::vec3 a, glm::vec3 b, float ra, float rb, float vStart, int sides) {
    const glm::vec3 d = glm::normalize(b - a), p = perpendicular(d), q = glm::cross(d, p);
    const float length = glm::length(b - a);
    for (int i = 0; i < sides; ++i) {
        const float a0 = 2 * Pi * i / sides, a1 = 2 * Pi * (i + 1) / sides;
        const glm::vec3 n0 = p * std::cos(a0) + q * std::sin(a0), n1 = p * std::cos(a1) + q * std::sin(a1);
        const glm::vec3 v[4] = {a + n0 * ra, a + n1 * ra, b + n1 * rb, b + n0 * rb};
        const glm::vec3 n[4] = {n0, n1, n1, n0};
        const float u0 = float(i) / sides, u1 = float(i + 1) / sides;
        const glm::vec2 uv[4] = {{u0, vStart}, {u1, vStart}, {u1, vStart + length}, {u0, vStart + length}};
        for (int k : {0, 1, 2, 0, 2, 3}) out.push_back({v[k], n[k], uv[k]});
    }
}

void leafCluster(std::vector<Vertex>& out, glm::vec3 centre, float size, Random& random) {
    // Three crossed cards in random orientations, normals pointing outwards from the crown.
    for (int c = 0; c < 3; ++c) {
        const float yaw = random.range(0, Pi), tilt = random.range(-0.6f, 0.6f);
        const glm::vec3 right(std::cos(yaw), 0, std::sin(yaw));
        const glm::vec3 up = glm::normalize(glm::vec3(-std::sin(yaw) * tilt, 1, std::cos(yaw) * tilt));
        const glm::vec3 n = glm::normalize(glm::cross(right, up));
        const float s = size * random.range(0.8f, 1.2f) * 0.5f;
        const glm::vec3 v[4] = {centre - right * s - up * s, centre + right * s - up * s, centre + right * s + up * s, centre - right * s + up * s};
        const glm::vec2 uv[4] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
        glm::vec3 outward = centre;
        outward.y *= 0.3f;
        const glm::vec3 shading = glm::length(outward) > 0.01f ? glm::normalize(glm::normalize(outward) + glm::vec3(0, 0.8f, 0)) : glm::vec3(0, 1, 0);
        (void)n;
        for (int k : {0, 1, 2, 0, 2, 3}) out.push_back({v[k], shading, uv[k]});
    }
}

void branch(Tree& tree, glm::vec3 base, glm::vec3 direction, float length, float radius, int depth, Random& random, float crownTop) {
    const glm::vec3 end = base + direction * length;
    tube(tree.bark, base, end, radius, radius * 0.62f, 0.f, depth == 0 ? 10 : 6);
    if (depth >= 3 || radius < 0.025f) {
        // Tip: a leaf cluster, larger low in the crown.
        leafCluster(tree.leaves, end, random.range(1.4f, 2.1f), random);
        return;
    }
    const int children = depth == 0 ? int(random.range(4, 7)) : int(random.range(2, 4));
    for (int i = 0; i < children; ++i) {
        const float around = 2 * Pi * (float(i) + random.next() * 0.6f) / children;
        const float spread = depth == 0 ? random.range(0.55f, 0.95f) : random.range(0.35f, 0.75f);
        const glm::vec3 p = perpendicular(direction), q = glm::cross(direction, p);
        glm::vec3 d = glm::normalize(direction * std::cos(spread) + (p * std::cos(around) + q * std::sin(around)) * std::sin(spread));
        d.y = std::max(d.y, 0.15f);                                       // branches keep rising
        d = glm::normalize(d);
        const float along = depth == 0 ? random.range(0.55f, 1.f) : 1.f; // main branches start up the trunk
        const glm::vec3 start = base + direction * length * along;
        const float childLength = std::min(length * random.range(0.55f, 0.75f), std::max(0.4f, crownTop - start.y));
        branch(tree, start, d, childLength, radius * 0.62f, depth + 1, random, crownTop);
    }
    // Extra foliage along the thicker branches, so the crown is dense.
    if (depth >= 1)
        for (int i = 0; i < 2; ++i) leafCluster(tree.leaves, base + direction * length * random.range(0.4f, 0.9f), random.range(1.2f, 1.8f), random);
}

float fbm(float x, float y) {
    float sum = 0, amp = 0.5f;
    for (int i = 0; i < 4; ++i) {
        const int xi = int(std::floor(x)), yi = int(std::floor(y));
        auto h = [](int a, int b) { std::uint32_t k = std::uint32_t(a) * 374761393u + std::uint32_t(b) * 668265263u; k = (k ^ (k >> 13)) * 1274126177u; return float(k & 0xffff) / 65535.f; };
        const float fx = x - xi, fy = y - yi, sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
        const float v = (h(xi, yi) + (h(xi + 1, yi) - h(xi, yi)) * sx) * (1 - sy) + (h(xi, yi + 1) + (h(xi + 1, yi + 1) - h(xi, yi + 1)) * sx) * sy;
        sum += amp * v; x *= 2.03f; y *= 2.03f; amp *= 0.5f;
    }
    return sum;
}

}

Tree build(std::uint32_t seed, float minimumHeight, float maximumHeight) {
    Random random{seed * 2654435761u + 12345u};
    random.next();
    Tree tree;
    tree.height = random.range(minimumHeight, maximumHeight);
    const float trunk = tree.height * random.range(0.32f, 0.42f);
    const float radius = 0.12f + tree.height * 0.022f;
    const glm::vec3 lean = glm::normalize(glm::vec3(random.range(-0.08f, 0.08f), 1, random.range(-0.08f, 0.08f)));
    // Trunk flare at the ground, then the trunk and the crown's leader.
    tube(tree.bark, glm::vec3(0, -0.1f, 0), glm::vec3(0, 0.35f, 0), radius * 1.45f, radius, 0.f, 10);
    branch(tree, glm::vec3(0, 0.35f, 0), lean, trunk, radius, 0, random, tree.height - 1.f);
    // Fill the crown's volume: clusters throughout the ellipsoid around the branch tips.
    glm::vec3 low(1e9f), high(-1e9f);
    for (const auto& v : tree.leaves) { low = glm::min(low, v.position); high = glm::max(high, v.position); }
    if (!tree.leaves.empty()) {
        const glm::vec3 centre = (low + high) * 0.5f, radius = (high - low) * 0.5f;
        for (int i = 0; i < 70; ++i) {
            glm::vec3 d;
            do { d = glm::vec3(random.range(-1, 1), random.range(-1, 1), random.range(-1, 1)); } while (glm::dot(d, d) > 1.f);
            // Mostly towards the outside, where the crown is seen.
            d *= 0.55f + 0.45f * std::cbrt(random.next());
            leafCluster(tree.leaves, centre + d * radius * 1.05f, random.range(1.5f, 2.3f), random);
        }
    }
    // Crown top: a cluster at the very top.
    leafCluster(tree.leaves, glm::vec3(lean.x * tree.height * 0.5f, tree.height - 1.1f, lean.z * tree.height * 0.5f), 2.2f, random);
    return tree;
}

std::vector<std::uint8_t> barkTexture(int size) {
    std::vector<std::uint8_t> image(size_t(size) * size * 4);
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x) {
            const float u = float(x) / size, v = float(y) / size;
            // Vertical furrows, seamless around the trunk (u) and along it (v).
            const float furrow = 0.5f + 0.5f * std::sin(u * 2 * Pi * 9 + fbm(u * 9, v * 3) * 4);
            const float grain = fbm(u * 32, v * 6);
            const float l = 0.55f + 0.30f * furrow * furrow + 0.25f * grain;
            auto* p = image.data() + (size_t(y) * size + x) * 4;
            p[0] = std::uint8_t(std::clamp(0.30f * l, 0.f, 1.f) * 255);
            p[1] = std::uint8_t(std::clamp(0.22f * l, 0.f, 1.f) * 255);
            p[2] = std::uint8_t(std::clamp(0.15f * l, 0.f, 1.f) * 255);
            p[3] = 255;
        }
    return image;
}

std::vector<std::uint8_t> leafTexture(int size) {
    // Many small leaves in U7's autumn colours on a transparent card, denser in the middle.
    std::vector<std::uint8_t> image(size_t(size) * size * 4, 0);
    Random random{777u};
    const glm::vec3 colours[] = {{.80f, .36f, .06f}, {.90f, .55f, .10f}, {.66f, .24f, .05f}, {.95f, .70f, .18f}, {.52f, .20f, .06f}, {.72f, .45f, .10f}};
    const int leaves = size * size / 90;
    for (int i = 0; i < leaves; ++i) {
        float cx, cy;
        do { cx = random.next(); cy = random.next(); } while (glm::length(glm::vec2(cx - .5f, cy - .5f)) > 0.47f * std::sqrt(random.next()) + 0.05f);
        const float angle = random.range(0, Pi), length = size * random.range(0.030f, 0.050f), width = length * random.range(0.45f, 0.65f);
        const glm::vec3 colour = colours[int(random.next() * 6) % 6] * random.range(0.8f, 1.1f);
        const glm::vec2 axis(std::cos(angle), std::sin(angle)), side(-axis.y, axis.x);
        for (int y = int(cy * size - length); y <= int(cy * size + length); ++y)
            for (int x = int(cx * size - length); x <= int(cx * size + length); ++x) {
                if (x < 0 || y < 0 || x >= size || y >= size) continue;
                const glm::vec2 d(x - cx * size, y - cy * size);
                const float a = glm::dot(d, axis) / length, b = glm::dot(d, side) / width;
                const float r = a * a + b * b;
                if (r > 1.f) continue;
                // Midrib darker, leaf lit towards one side.
                const float rib = std::abs(b) < 0.12f ? 0.8f : 1.f, lit = 0.85f + 0.2f * a;
                auto* p = image.data() + (size_t(y) * size + x) * 4;
                p[0] = std::uint8_t(std::clamp(colour.r * rib * lit, 0.f, 1.f) * 255);
                p[1] = std::uint8_t(std::clamp(colour.g * rib * lit, 0.f, 1.f) * 255);
                p[2] = std::uint8_t(std::clamp(colour.b * rib * lit, 0.f, 1.f) * 255);
                p[3] = 255;
            }
    }
    return image;
}

}
