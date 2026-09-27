#include "U7ObjectLayer.h"
#include <SDL3/SDL_opengl.h>
#include <algorithm>
#include <cctype>
#include <map>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

namespace {
std::string lower(std::string text) {
    for (auto& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}
bool matches(const std::string& name, const std::vector<std::string>& words) {
    return std::any_of(words.begin(), words.end(), [&](const std::string& w) { return name.find(w) != std::string::npos; });
}
}

U7ObjectLayer::~U7ObjectLayer() {
    if (!m_textures.empty()) glDeleteTextures(GLsizei(m_textures.size()), m_textures.data());
}

bool U7ObjectLayer::isStructure(const std::string& name) {
    static const std::vector<std::string> words = {
        "wall", "door", "window", "shutter", "fence", "gate", "portcullis", "bars", "crenellation",
        "mountain", "cavern", "pillar", "column", "chimney", "fortress"};
    static const std::vector<std::string> exclude = {"wall mount", "doorway"};
    return matches(name, words) && !matches(name, exclude);
}

GroundSpriteLayer U7ObjectLayer::build(const U7::Data& data, const std::string& name, const Filter& filter) {
    GroundSpriteLayer layer;
    layer.name = name;
    std::vector<const U7::WorldObject*> chosen;
    std::map<int, std::string> names;
    for (const auto& object : data.objects()) {
        auto [it, added] = names.try_emplace(object.shape);
        if (added) it->second = lower(data.name(object.shape));
        if (filter(object, it->second)) chosen.push_back(&object);
    }
    std::stable_sort(chosen.begin(), chosen.end(), [](const U7::WorldObject* a, const U7::WorldObject* b) {
        if (a->lift != b->lift) return a->lift < b->lift;
        return a->x + a->y < b->x + b->y;
    });
    struct Image { unsigned texture = 0; int width = 0, height = 0, hotX = 0, hotY = 0; };
    std::map<std::pair<int, int>, Image> images;
    constexpr float cell = float(U7::TilePixels);
    for (const auto* object : chosen) {
        auto [it, added] = images.try_emplace({object->shape, object->frame});
        if (added) {
            const auto frame = data.frame(object->shape, object->frame);
            if (frame.rgba.empty()) continue;
            GLuint texture = 0;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, frame.width, frame.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame.rgba.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            m_textures.push_back(texture);
            it->second = {texture, frame.width, frame.height, frame.hotX, frame.hotY};
        }
        const auto& image = it->second;
        if (!image.texture) continue;
        // Hot spot in world pixels: lower right pixel of the tile, lifted up and left.
        const float hx = float((object->x + 1) * U7::TilePixels - 1 - 4 * object->lift);
        const float hy = float((object->y + 1) * U7::TilePixels - 1 - 4 * object->lift);
        GroundSprite sprite;
        sprite.x0 = (hx - image.hotX) / cell;
        sprite.y0 = (hy - image.hotY) / cell;
        sprite.x1 = sprite.x0 + image.width / cell;
        sprite.y1 = sprite.y0 + image.height / cell;
        sprite.texture = static_cast<ImTextureID>(image.texture);
        layer.sprites.push_back(sprite);
    }
    return layer;
}
