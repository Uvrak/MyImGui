#include "DoorModel.h"
#include <cmath>

namespace DoorModel {
namespace {

// Box between two corners (metres), optionally turned about z (the brace), appended to out.
void box(std::vector<Vertex>& out, glm::vec3 low, glm::vec3 high, float tileMetres, float turnZ = 0.f, glm::vec3 pivot = {}) {
    const glm::vec3 c[8] = {{low.x, low.y, low.z}, {high.x, low.y, low.z}, {high.x, high.y, low.z}, {low.x, high.y, low.z},
                            {low.x, low.y, high.z}, {high.x, low.y, high.z}, {high.x, high.y, high.z}, {low.x, high.y, high.z}};
    const int faces[6][4] = {{4, 5, 6, 7}, {1, 0, 3, 2}, {5, 1, 2, 6}, {0, 4, 7, 3}, {7, 6, 2, 3}, {0, 1, 5, 4}};
    const glm::vec3 normals[6] = {{0, 0, 1}, {0, 0, -1}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}};
    const float cs = std::cos(turnZ), sn = std::sin(turnZ);
    auto turn = [&](glm::vec3 p) {
        p -= pivot;
        return pivot + glm::vec3(cs * p.x - sn * p.y, sn * p.x + cs * p.y, p.z);
    };
    for (int f = 0; f < 6; ++f) {
        glm::vec3 n = normals[f];
        n = glm::vec3(cs * n.x - sn * n.y, sn * n.x + cs * n.y, n.z);
        for (int i : {0, 1, 2, 0, 2, 3}) {
            glm::vec3 p = turn(c[faces[f][i]]);
            // Metres -> local units: tiles horizontally, metres up.
            out.push_back({{p.x / tileMetres, p.y, p.z / tileMetres}, n});
        }
    }
}

// Upright cylinder (hinge pin, ring pull made of segments), metres.
void cylinder(std::vector<Vertex>& out, glm::vec3 base, float radius, float height, float tileMetres, int sides = 12) {
    for (int i = 0; i < sides; ++i) {
        const float a0 = 6.2831853f * i / sides, a1 = 6.2831853f * (i + 1) / sides;
        const glm::vec3 d0(std::cos(a0), 0, std::sin(a0)), d1(std::cos(a1), 0, std::sin(a1));
        const glm::vec3 p[4] = {base + d0 * radius, base + d1 * radius, base + d1 * radius + glm::vec3(0, height, 0), base + d0 * radius + glm::vec3(0, height, 0)};
        const glm::vec3 n = glm::normalize(d0 + d1);
        for (int k : {0, 1, 2, 0, 2, 3}) out.push_back({{p[k].x / tileMetres, p[k].y, p[k].z / tileMetres}, n});
        const glm::vec3 top = base + glm::vec3(0, height, 0);
        for (const auto& q : {top, p[3], p[2]}) out.push_back({{q.x / tileMetres, q.y, q.z / tileMetres}, {0, 1, 0}});
    }
}

}

Parts build(float length, float height, float tileMetres) {
    Parts parts;
    const float L = length * tileMetres;     // metres
    const float thick = 0.055f, gap = 0.01f;
    const float inner = -thick / 2, outer = thick / 2;
    // Leaf: eight upright boards with small gaps (the grooves show between them).
    const int boards = 8;
    const float board = (L - gap) / boards;
    for (int i = 0; i < boards; ++i) {
        const float x1 = -gap - i * board, x0 = x1 - board + 0.004f;
        box(parts.planks, {x0, 0.02f, inner}, {x1, height, outer}, tileMetres);
    }
    // Inner side: two battens and the Z brace from the lower hinge side up to the latch side.
    const float bt = 0.035f, bh = 0.16f;
    const float lowY = 0.28f, highY = height - 0.30f - bh;
    box(parts.battens, {-L + 0.04f, lowY, inner - bt}, {-0.04f, lowY + bh, inner}, tileMetres);
    box(parts.battens, {-L + 0.04f, highY, inner - bt}, {-0.04f, highY + bh, inner}, tileMetres);
    {
        const glm::vec2 a(-0.12f, lowY + bh), b(-L + 0.12f, highY);
        const float span = glm::length(b - a), angle = std::atan2(b.y - a.y, b.x - a.x);
        const glm::vec3 centre((a.x + b.x) / 2, (a.y + b.y) / 2, 0);
        box(parts.battens, {centre.x - span / 2, centre.y - bh * 0.45f, inner - bt}, {centre.x + span / 2, centre.y + bh * 0.45f, inner},
            tileMetres, angle, centre);
    }
    // Outer side: two iron strap hinges (with a rounded end), the pin and a ring pull.
    for (const float y : {lowY + 0.04f, highY + 0.04f}) {
        box(parts.iron, {-L * 0.62f, y, outer}, {0.02f, y + 0.07f, outer + 0.008f}, tileMetres);
        cylinder(parts.iron, {-L * 0.62f, y - 0.005f, outer + 0.004f}, 0.045f, 0.08f, tileMetres);
        for (float x = -0.1f; x > -L * 0.6f; x -= 0.16f)   // nail heads
            cylinder(parts.iron, {x, y + 0.025f, outer + 0.008f}, 0.009f, 0.02f, tileMetres, 6);
    }
    cylinder(parts.iron, {0.015f, 0.02f, 0.f}, 0.022f, height - 0.02f, tileMetres);
    {
        const glm::vec3 ring(-L + 0.2f, 1.05f, outer + 0.03f);
        box(parts.iron, {ring.x - 0.03f, ring.y + 0.03f, outer}, {ring.x + 0.03f, ring.y + 0.09f, outer + 0.03f}, tileMetres);
        for (int i = 0; i < 16; ++i) {
            const float a = 6.2831853f * i / 16;
            const glm::vec3 p = ring + glm::vec3(std::cos(a) * 0.07f, std::sin(a) * 0.07f, 0);
            box(parts.iron, p - glm::vec3(0.012f), p + glm::vec3(0.012f), tileMetres);
        }
    }
    // Collision outline: the leaf as one box.
    std::vector<Vertex> outline;
    box(outline, {-L, 0.f, inner - bt}, {0.f, height, outer}, tileMetres);
    for (const auto& v : outline) parts.leaf.push_back(v.position);
    return parts;
}

}
