#include "U7GroundGrid.h"
#include <SDL3/SDL_opengl.h>
#include <map>
#include <stdexcept>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

U7GroundGrid::~U7GroundGrid() {
    if (!m_textures.empty()) glDeleteTextures(GLsizei(m_textures.size()), m_textures.data());
}

std::vector<std::uint8_t> U7GroundGrid::rgba(const U7::Data& data, int shape, int frame) {
    std::vector<std::uint8_t> pixels(U7::TilePixels * U7::TilePixels * 4, 0);
    const auto* source = data.flatFrame(shape, frame);
    if (!source) return pixels;
    for (int i = 0; i < U7::TilePixels * U7::TilePixels; ++i) {
        const auto c = data.palette()[source[i]];
        pixels[i * 4] = c.r; pixels[i * 4 + 1] = c.g; pixels[i * 4 + 2] = c.b; pixels[i * 4 + 3] = 255;
    }
    return pixels;
}

U7::TileRef U7GroundGrid::groundTile(const U7::Data& data, int x, int y) {
    auto t = data.tile(x, y);
    if (data.flatFrame(t.shape, t.frame)) return t;
    for (int d = 1; d < 32; ++d)
        for (int nx : {x - d, x + d}) {
            if (nx < 0 || nx >= U7::WorldTiles) continue;
            const auto n = data.tile(nx, y);
            if (data.flatFrame(n.shape, n.frame)) return n;
        }
    return t;
}

GroundLayer U7GroundGrid::build(const U7::Data& data) {
    GroundLayer layer;
    layer.width = layer.height = U7::WorldTiles;
    layer.cells.assign(size_t(U7::WorldTiles) * U7::WorldTiles, 0);
    layer.materials.push_back({0, IM_COL32(255, 0, 255, 255)});      // not a flat ground tile
    std::map<std::pair<int, int>, std::uint16_t> materials;
    m_nonFlat = 0;
    for (int y = 0; y < U7::WorldTiles; ++y)
        for (int x = 0; x < U7::WorldTiles; ++x) {
            if (!data.flatFrame(data.tile(x, y).shape, data.tile(x, y).frame)) ++m_nonFlat;
            const auto t = groundTile(data, x, y);
            if (!data.flatFrame(t.shape, t.frame)) continue;
            auto [it, added] = materials.try_emplace({t.shape, t.frame}, std::uint16_t(layer.materials.size()));
            if (added) {
                if (layer.materials.size() >= 65535) throw std::runtime_error("Too many U7 ground tiles");
                const auto pixels = rgba(data, t.shape, t.frame);
                GLuint texture = 0;
                glGenTextures(1, &texture);
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, U7::TilePixels, U7::TilePixels, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                m_textures.push_back(texture);
                layer.materials.push_back({static_cast<ImTextureID>(texture), IM_COL32_WHITE});
            }
            layer.cells[size_t(y) * U7::WorldTiles + x] = it->second;
        }
    layer.validate();
    return layer;
}
