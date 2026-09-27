#include "Britannia3dView.h"
#include <set>
#include "PixelArtScale.h"
#include "U73dScale.h"
#include "SlateMaterial.h"
#include "AshlarMaterial.h"
#include "CobbleMaterial.h"
#include "GrassMaterial.h"
#include "StoneMaterial.h"
#include "ThatchMaterial.h"
#include "GateModels.h"
#include "LampModel.h"
#include "WellModel.h"
#include "TreeModel.h"
#include "WoodMaterial.h"
#include "U7GroundGrid.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <SDL3_image/SDL_image.h>
#include <atomic>

namespace {

constexpr int P = U7::TilePixels;
constexpr float Radius = ow3d::PlanetRadius;
constexpr float GlobeDepth = 0.3f;        // metres the coarse globe lies below the detail ground
constexpr float MaxWrapAngle = 2.9f;      // radians from the centre the flat map may be wrapped

// Flat positions (x east, height up, y south; metres) are wrapped onto the sphere here as in
// Britannia3dView::planet; "direct" meshes (the ocean sphere) are already in planet units.
const char* VertexSource = R"(#version 460 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;
layout(location = 3) in float aShade;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 flatModel;          // moves a mesh in the flat world (doors); identity otherwise
uniform vec2 centre;
uniform float metre;
uniform float radius;
uniform float tileMetres;
uniform bool direct;
uniform int wind;                // 1: sways with the height above the ground, 2: leaves also flutter
uniform float time;
out vec3 vNormal;
out vec2 vUv;
out float vShade;
out vec3 vFlat;
out vec3 vMapNormal;
vec3 planet(vec3 f) {
    vec2 t = (f.xz - centre) * tileMetres * metre;
    float a = length(t);
    vec3 d = vec3(0.0, 0.0, 1.0);
    if (a > 1e-9) d = cos(a / radius) * d + sin(a / radius) * vec3(t.x, -t.y, 0.0) / a;
    return d * (radius + f.y * metre);
}
void main() {
    // Materials stick to the mesh (vFlat, vMapNormal before flatModel); light follows it.
    vNormal = mat3(flatModel) * aNormal; vMapNormal = aNormal; vUv = aUv; vShade = aShade; vFlat = aPosition;
    vec3 f = (flatModel * vec4(aPosition, 1.0)).xyz;
    if (wind != 0) {
        // Metres above the trunk base (y), the tree's place gives the phase.
        float phase = (floor(f.x) + floor(f.z)) * 0.37;
        float sway = pow(max(f.y - 1.2, 0.0) / 8.0, 1.6) * 0.45;
        float gust = sin(time * 1.1 + phase) + 0.45 * sin(time * 2.3 + phase * 1.7);
        vec2 push = vec2(0.8, 0.45) * sway * gust;
        if (wind == 2) push += vec2(sin(time * 5.1 + f.y * 3.0 + f.x), cos(time * 4.3 + f.z * 2.7)) * 0.035;
        f.xz += push / tileMetres;
        f.y -= dot(push, push) * 0.3;
    }
    gl_Position = projection * view * vec4(direct ? aPosition : planet(f), 1.0);
})";

// Lit in the flat frame: every place of Britannia gets the same U7 light from the south east.
const char* FragmentSource = R"(#version 460 core
in vec3 vNormal;
in vec2 vUv;
in float vShade;
in vec3 vFlat;
in vec3 vMapNormal;
uniform mat4 flatModel;
uniform sampler2D image;
uniform sampler2DArray tiles;
uniform bool tileArray;
uniform int planks;             // 0: U7 graphic, 1/2: boards, 3: thatch, 4: stone wall, 5: slate
uniform sampler2D stoneAlbedo;
uniform sampler2D stoneNormal;
uniform float stoneSize;        // metres along a wall per atlas face
uniform sampler2D grassAlbedo;
uniform sampler2D grassNormal;
uniform float grassSize;
uniform sampler2D cobbleAlbedo;
uniform sampler2D cobbleNormal;
uniform float cobbleSize;
uniform sampler2D ashlarAlbedo;
uniform sampler2D ashlarNormal;
uniform float ashlarSize;
uniform float stoneHeight;      // metres up per atlas face (a storey)
uniform sampler2D slateAlbedo;
uniform sampler2D slateNormal;
uniform float slateSize;
uniform sampler2D thatchAlbedo;
uniform sampler2D thatchNormal;
uniform float thatchSize;
uniform vec3 tint;
uniform sampler2D woodAlbedo;
uniform sampler2D woodNormal;
uniform float woodSize;
uniform float woodTile;          // metres per tile: the material is mapped in metres
out vec4 color;
const vec3 sun = normalize(vec3(-0.35, 1.0, 0.45));
void main() {
    if (planks == 3 || planks == 5) {
        // Roofs: uv in metres (u along the ridge, v down the slope), as the roof mesh sets it.
        vec3 n = normalize(vMapNormal), t = abs(n.x) > abs(n.z) ? vec3(0, 0, 1) : vec3(1, 0, 0);
        vec3 down = normalize(cross(t, n));
        if (down.y > 0.0) down = -down;
        t = normalize(cross(n, down));
        vec2 uv = vUv / (planks == 3 ? thatchSize : slateSize);
        vec3 relief = (planks == 3 ? texture(thatchNormal, uv) : texture(slateNormal, uv)).xyz * 2.0 - 1.0;
        vec3 bumped = normalize(t * relief.x + down * relief.y + n * relief.z);
        float light = 0.42 + 0.68 * max(dot(bumped, sun), 0.0);
        vec3 albedo = planks == 3 ? texture(thatchAlbedo, uv).rgb : texture(slateAlbedo, uv).rgb;
        color = vec4(albedo * light * vShade, 1.0);
        return;
    }
    if (planks != 0) {
        // Plank material, mapped in metres: boards run horizontally on every wall face.
        vec3 n = normalize(vMapNormal), t, down;
        vec2 uv;
        if (abs(n.y) > 0.5) { uv = vFlat.xz * woodTile; t = vec3(1, 0, 0); down = vec3(0, 0, 1); }
        else if (abs(n.x) > 0.5) { uv = vec2(vFlat.z * woodTile, -vFlat.y); t = vec3(0, 0, 1); down = vec3(0, -1, 0); }
        else { uv = vec2(vFlat.x * woodTile, -vFlat.y); t = vec3(1, 0, 0); down = vec3(0, -1, 0); }
        if (planks == 2 && abs(n.y) <= 0.5) { uv = uv.yx; vec3 s = t; t = down; down = s; }
        const bool stone = planks == 4, ashlar = planks == 6;
        uv /= stone ? stoneSize : ashlar ? ashlarSize : woodSize;
        vec3 relief = (stone ? texture(stoneNormal, uv) : ashlar ? texture(ashlarNormal, uv) : texture(woodNormal, uv)).xyz * 2.0 - 1.0;
        vec3 bumped = normalize(mat3(flatModel) * (t * relief.x + down * relief.y + n * relief.z));
        float light = 0.42 + 0.65 * max(dot(bumped, sun), 0.0);
        vec3 albedo = stone ? texture(stoneAlbedo, uv).rgb * tint * 2.0 : ashlar ? texture(ashlarAlbedo, uv).rgb * 1.3 : tint * texture(woodAlbedo, uv).rgb * 2.0;
        color = vec4(albedo * light * vShade, 1.0);
        return;
    }
    if (tileArray && vShade >= 4000.0) {
        // Lawn edge with earth: U7's green pixels become the high resolution grass, its earth stays.
        vec4 earth = texture(tiles, vec3(vUv, vShade - 4000.0));
        vec2 uv = vFlat.xz * woodTile / grassSize;
        vec3 relief = texture(grassNormal, uv).xyz * 2.0 - 1.0;
        float light = 0.5 + 0.55 * max(dot(normalize(vec3(relief.x, relief.z, relief.y)), sun), 0.0);
        vec3 lawn = texture(grassAlbedo, uv).rgb * light;
        float greenness = earth.g - max(earth.r, earth.b);
        color = vec4(mix(earth.rgb, lawn, smoothstep(0.0, 0.05, greenness)), 1.0);
        return;
    }
    if (tileArray && vShade >= 3000.0) {
        // Street with dirt: U7's grey stones become the cobblestone, its dirt stays.
        vec4 dirt = texture(tiles, vec3(vUv, vShade - 3000.0));
        vec2 uv = vFlat.xz * woodTile / cobbleSize;
        vec3 relief = texture(cobbleNormal, uv).xyz * 2.0 - 1.0;
        float light = 0.45 + 0.62 * max(dot(normalize(vec3(relief.x, relief.z, relief.y)), sun), 0.0);
        vec3 stone = texture(cobbleAlbedo, uv).rgb * light;
        float saturation = max(max(dirt.r, dirt.g), dirt.b) - min(min(dirt.r, dirt.g), dirt.b);
        float isStone = 1.0 - smoothstep(0.05, 0.12, saturation);
        color = vec4(mix(dirt.rgb, stone, isStone), 1.0);
        return;
    }
    if (tileArray && vShade >= 2000.0) {
        // Lawn: the high resolution grass, mapped in metres.
        vec2 uv = vFlat.xz * woodTile / grassSize;
        vec3 relief = texture(grassNormal, uv).xyz * 2.0 - 1.0;
        vec3 bumped = normalize(vec3(relief.x, relief.z, relief.y));
        float light = 0.5 + 0.55 * max(dot(bumped, sun), 0.0);
        color = vec4(texture(grassAlbedo, uv).rgb * light, 1.0);
        return;
    }
    if (tileArray && vShade >= 1000.0) {
        // Street: cobblestone mapped in metres (x east, y south).
        vec2 uv = vFlat.xz * woodTile / cobbleSize;
        vec3 relief = texture(cobbleNormal, uv).xyz * 2.0 - 1.0;
        vec3 bumped = normalize(vec3(relief.x, relief.z, relief.y));
        float light = 0.45 + 0.62 * max(dot(bumped, sun), 0.0);
        color = vec4(texture(cobbleAlbedo, uv).rgb * light, 1.0);
        return;
    }
    vec4 texel = tileArray ? texture(tiles, vec3(vUv, vShade)) : texture(image, vUv);
    float shade = tileArray ? 1.0 : vShade;
    // Outline at alpha 0.5, antialiased over one screen pixel (alpha to coverage).
    float alpha = clamp((texel.a - 0.5) / max(fwidth(texel.a), 1e-4) + 0.5, 0.0, 1.0);
    if (alpha <= 0.0) discard;
    vec3 n = normalize(gl_FrontFacing ? vNormal : -vNormal);
    float light = 0.62 + 0.38 * max(dot(n, normalize(vec3(-0.35, 1.0, 0.45))), 0.0);
    color = vec4(texel.rgb * light * shade, alpha);
})";

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

}

bool Britannia3dView::loadOpenGl() {
    return gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress)) != 0;
}

Britannia3dView::~Britannia3dView() {
    destroy();
    character.destroy();
}

void Britannia3dView::destroy() {
    m_batches.clear();
    m_globe.clear();
    m_batchOfFrame.clear();
    m_models.clear();
    m_walls.clear();
    m_roofTiles.clear();
    m_fortressTiles.clear();
    m_gates.clear();
    m_gateIron.destroy(); m_gateWood.destroy();
    m_stepTops.clear();
    m_solidTiles.clear();
    m_stableProps.reset();
    m_stableTools.reset();
    m_doors.clear();
    m_doorLeaf.clear();
    m_doorPlanks.destroy(); m_doorBattens.destroy(); m_doorIron.destroy();
    m_windowFrame.destroy(); m_windowGlass.destroy();
    m_stoneTiles.clear();
    m_wallTop.clear();
    m_ironTexture = 0;
    if (!m_textures.empty()) glDeleteTextures(GLsizei(m_textures.size()), m_textures.data());
    m_textures.clear();
    if (m_color) glDeleteTextures(1, &m_color);
    if (m_framebuffer) glDeleteFramebuffers(1, &m_framebuffer);
    if (m_msColor) glDeleteRenderbuffers(1, &m_msColor);
    if (m_msDepth) glDeleteRenderbuffers(1, &m_msDepth);
    if (m_msFramebuffer) glDeleteFramebuffers(1, &m_msFramebuffer);
    m_color = m_framebuffer = m_msColor = m_msDepth = m_msFramebuffer = 0;
    m_targetWidth = m_targetHeight = 0;
}

glm::vec3 Britannia3dView::planet(glm::vec3 f) const {
    const glm::vec2 t = (glm::vec2(f.x, f.z) - m_centre) * (U73dScale::TileMetres * Metre);
    const float a = glm::length(t);
    glm::vec3 d(0, 0, 1);
    if (a > 1e-9f) d = std::cos(a / Radius) * d + std::sin(a / Radius) * glm::vec3(t.x, -t.y, 0) / a;
    return d * (Radius + f.y * Metre);
}

glm::vec3 Britannia3dView::flat(glm::vec3 p) const {
    const float length = glm::length(p);
    const glm::vec3 d = p / length;
    const float angle = std::acos(std::clamp(d.z, -1.f, 1.f));
    glm::vec2 t(0);
    const glm::vec2 side(d.x, -d.y);
    if (glm::length(side) > 1e-9f) t = glm::normalize(side) * angle * Radius;
    const glm::vec2 xz = m_centre + t / (U73dScale::TileMetres * Metre);
    return {xz.x, (length - Radius) / Metre, xz.y};
}

unsigned Britannia3dView::makeTexture(int width, int height, const std::uint8_t* rgba, bool mipmaps) {
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    if (mipmaps) glGenerateMipmap(GL_TEXTURE_2D);
    // Mipmapped textures are the upscaled U7 graphics: smooth, sharp at flat angles too.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mipmaps ? GL_LINEAR : GL_NEAREST);
    if (mipmaps) glTexParameterf(GL_TEXTURE_2D, 0x84FE /* GL_TEXTURE_MAX_ANISOTROPY, core 4.6 */, 8.f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_textures.push_back(id);
    return id;
}

void Britannia3dView::addQuad(Batch& batch, const glm::vec3 (&p)[4], const glm::vec2 (&uv)[4], const glm::vec3& normal) {
    for (int i : {0, 1, 2, 0, 2, 3})
        batch.vertices.push_back({p[i].x, p[i].y, p[i].z, normal.x, normal.y, normal.z, uv[i].x, uv[i].y, 1.f});
}

void Britannia3dView::addObject(const U7::Data& data, const U7::WorldObject& object, int layer, bool roof, float liftMetres, int planks,
                                bool thinWall) {
    const auto model = m_models.find({object.shape, object.frame});
    if (model == m_models.end()) return;
    const auto& source = model->second;
    if (source.vertices.empty()) return;
    auto [slot, added] = m_batchOfFrame.try_emplace({object.shape, object.frame, layer}, m_batches.size());
    if (added) {
        auto& created = m_batches.emplace_back();
        created.texture = source.texture;
        created.layer = layer;
        created.roof = roof;
        created.planks = planks;
        created.tint = source.tint;
    }
    Batch& batch = m_batches[slot->second];
    // Model origin = south east bottom corner of the object, in flat world metres.
    const glm::vec3 anchor(float(object.x + 1), object.lift * liftMetres, float(object.y + 1));
    // Boxes take the real lift height; trees and bushes keep their frame size (8 pixels = 1 m).
    // (Model x / z are in tiles; an upright's height is in tiles too, 8 frame pixels each.)
    const float height = source.kind == U7ObjectModel::Kind::Upright ? U73dScale::TileMetres : liftMetres / U7ObjectModel::LiftMetres;
    const glm::vec3 stretch(1, height, 1);
    // Walls, doors, fences ... of layer 1 stop Sir Canegm (not the walkways on the town walls).
    const bool wall = layer == 1 && source.kind != U7ObjectModel::Kind::Flat && source.kind != U7ObjectModel::Kind::Upright;
    auto& walls = m_walls[{object.x / 8, object.y / 8}];
    if (layer == 1 && source.kind == U7ObjectModel::Kind::Box && lower(data.name(object.shape)) == "wall") {
        const auto size = data.shapeSize(object.shape);
        const float top = (object.lift + size.z) * liftMetres;
        for (int y = object.y - size.y + 1; y <= object.y; ++y)
            for (int x = object.x - size.x + 1; x <= object.x; ++x) {
                m_solidTiles.insert({x, y});
                if (planks == 4) m_stoneTiles.insert({x, y});
                auto& t = m_wallTop[{x, y}];
                t = std::max(t, top);
            }
    }
    for (const auto& v : source.vertices) {
        // Faces turned away from the light get a little darker, as in U7.
        const float shade = v.normal.x < -0.5f || v.normal.z < -0.5f ? 0.9f : 1.f;
        auto local = v.position * stretch;
        // Thinner walls: squeeze the one tile thick side about its middle (x / z run -size .. 0).
        if (thinWall && source.kind == U7ObjectModel::Kind::Box) {
            const auto size = data.shapeSize(object.shape);
            if (size.y == 1 && size.x > 1) local.z = -0.5f + (local.z + 0.5f) * U73dScale::WallThickness;
            if (size.x == 1 && size.y > 1) local.x = -0.5f + (local.x + 0.5f) * U73dScale::WallThickness;
            // Corner and end pieces (1 x 1): a side is pulled in to the wall thickness only where
            // no wall, door or window continues, so the pieces close flush with their neighbours.
            if (size.x == 1 && size.y == 1) {
                const float t = U73dScale::WallThickness * 0.5f;
                auto has = [&](int dx, int dy) { return m_wallFootprint.count({object.x + dx, object.y + dy}) > 0; };
                if (local.x < -0.5f && !has(-1, 0)) local.x = -0.5f - t;
                if (local.x > -0.5f && !has(1, 0)) local.x = -0.5f + t;
                if (local.z < -0.5f && !has(0, -1)) local.z = -0.5f - t;
                if (local.z > -0.5f && !has(0, 1)) local.z = -0.5f + t;
            }
        }
        const auto p = anchor + local;
        batch.vertices.push_back({p.x, p.y, p.z, v.normal.x, v.normal.y, v.normal.z, v.uv.x, v.uv.y, shade});
        if (wall) walls.push_back(p);
    }
    if (roof) {
        const auto size = data.shapeSize(object.shape);
        for (int y = object.y - size.y + 1; y <= object.y; ++y)
            for (int x = object.x - size.x + 1; x <= object.x; ++x) m_roofTiles.insert({x, y});
    }
    ++(source.kind == U7ObjectModel::Kind::Upright ? m_quadObjects : m_boxes);
}

void Britannia3dView::loadModels(const U7::Data& data, const std::vector<std::pair<int, int>>& graphics) {
    // Model files are read (or generated) and their textures upscaled on all cores, in groups
    // so the upscaled images do not all wait in memory at once; the upload stays on this thread.
    struct Prepared { U7ObjectModel model; bool created = false; std::vector<std::uint8_t> texels; int width = 0, height = 0; };
    constexpr int factor = TexelsPerTile / P;
    const unsigned workers = std::max(1u, std::thread::hardware_concurrency());
    for (size_t first = 0; first < graphics.size(); first += 96) {
        const size_t count = std::min<size_t>(96, graphics.size() - first);
        std::vector<Prepared> prepared(count);
        std::atomic<size_t> next{0};
        std::vector<std::thread> threads;
        for (unsigned w = 0; w < workers; ++w)
            threads.emplace_back([&] {
                for (size_t i; (i = next++) < count;) {
                    auto& p = prepared[i];
                    const auto [shape, frame] = graphics[first + i];
                    p.model = U7ObjectModel::loadOrCreate(assetDirectory, data, shape, frame, &p.created);
                    if (p.model.empty()) continue;
                    // Blood (shape 912) uses U7 palette entries that the game colour-cycles to red;
                    // with the static palette they show yellow, so it is recoloured to dark red.
                    if (shape == 912)
                        for (size_t k = 0; k + 3 < p.model.texture.size(); k += 4) {
                            auto* c = p.model.texture.data() + k;
                            if (!c[3]) continue;
                            const float light = (c[0] * 0.3f + c[1] * 0.59f + c[2] * 0.11f) / 255.f;
                            c[0] = std::uint8_t(std::clamp(40.f + 150.f * light, 0.f, 255.f));
                            c[1] = std::uint8_t(4.f + 10.f * light);
                            c[2] = std::uint8_t(4.f + 12.f * light);
                        }
                    // The model file keeps the original frame; the view shows it upscaled.
                    p.texels = PixelArtScale::scale(p.model.texture.data(), p.model.textureWidth, p.model.textureHeight, factor);
                    p.width = p.model.textureWidth * factor;
                    p.height = p.model.textureHeight * factor;
                    PixelArtScale::smoothEdges(p.texels, p.width, p.height, factor / 2);
                }
            });
        for (auto& thread : threads) thread.join();
        for (size_t i = 0; i < count; ++i) {
            auto& p = prepared[i];
            m_createdFiles += p.created;
            if (p.model.empty()) continue;
            auto& model = m_models[graphics[first + i]];
            model.texture = makeTexture(p.width, p.height, p.texels.data(), true);
            model.kind = p.model.kind;
            model.vertices = std::move(p.model.vertices);
            // Mean colour of the original graphic without its black outline.
            glm::vec3 sum(0);
            int count = 0;
            for (size_t k = 0; k + 3 < p.model.texture.size(); k += 4) {
                const glm::vec3 c(p.model.texture[k], p.model.texture[k + 1], p.model.texture[k + 2]);
                if (!p.model.texture[k + 3] || c.r + c.g + c.b < 90) continue;
                sum += c; ++count;
            }
            if (count) {
                model.tint = sum / float(count) / 255.f;
                const auto& c = model.tint;
                model.planks = c.r > c.g + 0.04f && c.g > c.b + 0.03f && c.r - c.b > 0.13f && c.r < 0.55f;
                // Stone: grey, hardly coloured (U7's rubble walls).
                model.stone = !model.planks && std::max({c.r, c.g, c.b}) - std::min({c.r, c.g, c.b}) < 0.07f && c.r < 0.6f;
            }
        }
    }
}

void Britannia3dView::buildGateModels(const U7::Data& data, const std::vector<std::pair<U7::WorldObject, float>>& parts) {
    if (parts.empty()) return;
    constexpr float tm = U73dScale::TileMetres;
    glm::vec3 woodTone(0.45f, 0.32f, 0.24f);
    for (const auto& batch : m_batches) if (batch.planks == 1 && batch.tint != glm::vec3(1)) { woodTone = batch.tint; break; }
    const std::uint8_t ironColour[4] = {70, 70, 74, 255};
    const unsigned ironTexture = makeTexture(1, 1, ironColour, false);
    GateModels::Parts all;
    std::vector<glm::vec3> winches;
    // (x, z) in metres on the flat world; models are placed by a turn and an offset.
    auto place = [&](const GateModels::Parts& model, glm::vec3 origin, bool turn) {
        auto put = [&](const std::vector<GateModels::Vertex>& from, std::vector<GateModels::Vertex>& to) {
            for (auto v : from) {
                if (turn) { v.position = glm::vec3(-v.position.z, v.position.y, v.position.x); v.normal = glm::vec3(-v.normal.z, v.normal.y, v.normal.x); }
                v.position += origin;
                to.push_back(v);
            }
        };
        put(model.iron, all.iron);
        put(model.wood, all.wood);
        put(model.darkWood, all.darkWood);
    };
    for (const auto& [o, liftMetres] : parts) {
        const auto size = data.shapeSize(o.shape);
        const auto name = lower(data.name(o.shape));
        const float x0 = float(o.x - size.x + 1) * tm, z0 = float(o.y - size.y + 1) * tm;
        const float x1 = float(o.x + 1) * tm, z1 = float(o.y + 1) * tm;
        if (name == "portcullis") {
            Gate gate;
            gate.alongX = size.x > size.y;
            gate.length = float(gate.alongX ? size.x : size.y);
            gate.height = size.z * U73dScale::StructureLift;
            // Along x it runs from x0 through the tile row's middle; along y turned by 90 degrees.
            gate.origin = gate.alongX ? glm::vec3(float(o.x - size.x + 1), o.lift * U73dScale::StructureLift, o.y + 0.5f)
                                      : glm::vec3(o.x + 0.5f, o.lift * U73dScale::StructureLift, float(o.y - size.y + 1));
            m_gates.push_back(gate);
        } else if (name == "winch") {
            winches.push_back(glm::vec3((x0 + x1) / 2 / tm, o.lift * liftMetres, (z0 + z1) / 2 / tm));
            const bool alongX = size.x > size.y;
            place(GateModels::winch(), {(x0 + x1) / 2, o.lift * liftMetres, (z0 + z1) / 2}, !alongX);
        } else {
            // A stair step: its block from the ground (or its lift) to its top.
            const float base = o.lift * liftMetres, top = (o.lift + size.z) * liftMetres;
            GateModels::Parts step;
            GateModels::stairStep(step, {x0, base, z0}, {x1, top, z1});
            place(step, glm::vec3(0), false);
            const glm::vec3 a(x0 / tm, top, z0 / tm), b(x1 / tm, top, z0 / tm), c(x1 / tm, top, z1 / tm), d(x0 / tm, top, z1 / tm);
            m_stepTops.insert(m_stepTops.end(), {a, b, c, a, c, d});
        }
    }
    // Each gate is worked by its nearest winch; the portcullis mesh is shared (along x, in tiles).
    for (auto& gate : m_gates) {
        float best = 1e9f;
        const glm::vec3 centre = gate.origin + (gate.alongX ? glm::vec3(gate.length / 2, 0, 0) : glm::vec3(0, 0, gate.length / 2));
        for (const auto& w : winches)
            if (const float d = glm::distance(glm::vec2(w.x, w.z), glm::vec2(centre.x, centre.z)); d < best) { best = d; gate.winch = w; }
    }
    if (!m_gates.empty()) {
        const auto model = GateModels::portcullis(m_gates.front().length * tm, m_gates.front().height);
        auto upload = [&](ow3d::Mesh& mesh, const std::vector<GateModels::Vertex>& vertices) {
            std::vector<float> data;
            for (const auto& v : vertices)
                data.insert(data.end(), {v.position.x / tm, v.position.y, v.position.z / tm, v.normal.x, v.normal.y, v.normal.z, v.uv.x, v.uv.y, 1.f});
            mesh.create(data.data(), unsigned(data.size() / 9), 9);
        };
        upload(m_gateIron, model.iron);
        upload(m_gateWood, model.darkWood);
        if (!m_ironTexture) { const std::uint8_t iron[4] = {58, 56, 54, 255}; m_ironTexture = makeTexture(1, 1, iron, false); }
    }
    struct Part { const std::vector<GateModels::Vertex>* vertices; int planks; glm::vec3 tint; unsigned texture; int layer; };
    const Part list[3] = {{&all.iron, 0, glm::vec3(1), ironTexture, 1}, {&all.wood, 1, woodTone, 0, 1}, {&all.darkWood, 1, woodTone * 0.7f, 0, 1}};
    for (const auto& part : list) {
        auto& batch = m_batches.emplace_back();
        batch.planks = part.planks;
        batch.tint = part.tint;
        batch.texture = part.texture;
        batch.layer = part.layer;
        for (const auto& v : *part.vertices)
            batch.vertices.push_back({v.position.x / tm, v.position.y, v.position.z / tm, v.normal.x, v.normal.y, v.normal.z, v.uv.x, v.uv.y, 1.f});
    }
}

void Britannia3dView::buildRoads() {
    const auto cobble = CobbleMaterial::loadOrCreate(assetDirectory, m_roadColours);
    m_cobbleAlbedo = makeTexture(cobble.size, cobble.size, cobble.albedo.data(), true);
    m_cobbleNormal = makeTexture(cobble.size, cobble.size, cobble.normal.data(), true);
    for (unsigned id : {m_cobbleAlbedo, m_cobbleNormal}) {
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    const int x0 = m_groundRect.x, y0 = m_groundRect.y, x1 = m_groundRect.z, y1 = m_groundRect.w, w = x1 - x0;
    auto& road = m_roadTiles;
    road.clear();
    std::set<std::pair<int, int>> doorZone, doorCobbles;
    for (const auto& door : m_doors) {
        if (door.kind != 0) continue;
        const glm::vec3 dir = glm::vec3(glm::rotate(glm::mat4(1), door.baseYaw, glm::vec3(0, 1, 0)) * glm::vec4(-1, 0, 0, 0));
        for (float t = 0.25f; t < door.length; t += 0.5f) {
            const glm::vec3 p = door.pivot + dir * t;
            for (int dy = -2; dy <= 2; ++dy)
                for (int dx = -2; dx <= 2; ++dx) doorZone.insert({int(std::floor(p.x)) + dx, int(std::floor(p.z)) + dy});
        }
    }
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x) {
            const int layer = m_tileLayer[size_t(y - y0) * w + (x - x0)];
            if (!m_roadLayer[size_t(layer)] || m_roofTiles.count({x, y}) || m_solidTiles.count({x, y})) continue;
            if (doorZone.count({x, y})) {
                // In front of a door: the same cobbles, level with the threshold.
                auto* v = m_batches[m_groundBatch].vertices.data() + (size_t(y - y0) * w + (x - x0)) * 6;
                for (int i = 0; i < 6; ++i) v[i].shade += 1000.f;
                doorCobbles.insert({x, y});
                continue;
            }
            road.insert({x, y});
            auto* v = m_batches[m_groundBatch].vertices.data() + (size_t(y - y0) * w + (x - x0)) * 6;
            for (int i = 0; i < 6; ++i) { v[i].shade += 1000.f; v[i].y = RoadHeight; }
        }
    // Kerbs: dressed stone 0.25 m wide and 0.12 m high along the edges where a street tile meets
    // other open ground (grass, earth); not against walls or houses.
    auto& kerb = m_batches.emplace_back();
    kerb.planks = 6;
    kerb.layer = 0;
    constexpr float tm = U73dScale::TileMetres, width = 0.25f / tm, height = 0.12f;
    auto box = [&](glm::vec2 a, glm::vec2 b) {                // a / b: opposite corners (tiles)
        const glm::vec3 c[8] = {{a.x, 0, a.y}, {b.x, 0, a.y}, {b.x, 0, b.y}, {a.x, 0, b.y},
                                {a.x, height, a.y}, {b.x, height, a.y}, {b.x, height, b.y}, {a.x, height, b.y}};
        const int f[5][4] = {{4, 5, 6, 7}, {0, 1, 5, 4}, {2, 3, 7, 6}, {1, 2, 6, 5}, {3, 0, 4, 7}};
        const glm::vec3 n[5] = {{0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {1, 0, 0}, {-1, 0, 0}};
        const glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        for (int k = 0; k < 5; ++k) addQuad(kerb, {c[f[k][0]], c[f[k][1]], c[f[k][2]], c[f[k][3]]}, uv, n[k]);
    };
    // Street tiles with dirt next to a street: the cobbles with the dirt, level with the ground, no kerb.
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x) {
            const int layer = m_tileLayer[size_t(y - y0) * w + (x - x0)];
            // (Under a roof only in a doorway: the threshold of the shed.)
            if (!m_mixedLayer[size_t(layer)] || (m_roofTiles.count({x, y}) && !doorZone.count({x, y})) || m_solidTiles.count({x, y})) continue;
            bool near = false;
            for (int dy = -3; dy <= 3 && !near; ++dy)
                for (int dx = -3; dx <= 3 && !near; ++dx) near = road.count({x + dx, y + dy}) || doorCobbles.count({x + dx, y + dy});
            if (!near) continue;
            auto* v = m_batches[m_groundBatch].vertices.data() + (size_t(y - y0) * w + (x - x0)) * 6;
            for (int i = 0; i < 6; ++i) v[i].shade = float(layer) + 3000.f;
            doorCobbles.insert({x, y});
        }
    auto free = [&](int x, int y) {
        return x >= x0 && y >= y0 && x < x1 && y < y1 && !road.count({x, y}) && !m_solidTiles.count({x, y}) && !m_roofTiles.count({x, y}) &&
               !doorZone.count({x, y}) && !doorCobbles.count({x, y});
    };
    // Diagonal corners: an open tile with street on two adjacent sides (and not on the other
    // two) gets the street's half as a triangle with a diagonal kerb, grass on the other half.
    // Corners: 0 north west, 1 north east, 2 south east, 3 south west.
    std::map<std::pair<int, int>, int> diagonal;                 // tile -> corner towards the street
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x) {
            if (!free(x, y)) continue;
            const bool n = road.count({x, y - 1}) > 0, e = road.count({x + 1, y}) > 0, so = road.count({x, y + 1}) > 0, w = road.count({x - 1, y}) > 0;
            if (n + e + so + w != 2) continue;
            if (n && e) diagonal[{x, y}] = 1;
            else if (e && so) diagonal[{x, y}] = 2;
            else if (so && w) diagonal[{x, y}] = 3;
            else if (w && n) diagonal[{x, y}] = 0;
        }
    auto open = [&](int x, int y) { return free(x, y) && !diagonal.count({x, y}); };
    {
        auto& ground = m_batches[m_groundBatch];
        const glm::vec2 corner[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        for (const auto& [tile, c] : diagonal) {
            const auto [x, y] = tile;
            // The grass half: the ground layer of the open neighbour away from the street.
            const glm::ivec2 away[4] = {{1, 1}, {-1, 1}, {-1, -1}, {1, -1}};         // opposite of corner c
            int grass = -1;
            for (const glm::ivec2 d : {glm::ivec2(away[c].x, 0), glm::ivec2(0, away[c].y)})
                if (open(x + d.x, y + d.y)) { grass = m_tileLayer[size_t(y + d.y - y0) * w + (x + d.x - x0)]; break; }
            auto* v = ground.vertices.data() + (size_t(y - y0) * w + (x - x0)) * 6;
            if (grass >= 0) for (int i = 0; i < 6; ++i) v[i].shade = float(grass);
            // Street triangle at street height.
            const glm::vec2 p0 = glm::vec2(x, y) + corner[(c + 3) % 4], p1 = glm::vec2(x, y) + corner[c], p2 = glm::vec2(x, y) + corner[(c + 1) % 4];
            for (const auto& p : {p0, p1, p2})
                ground.vertices.push_back({p.x, RoadHeight, p.y, 0, 1, 0, 0, 0, 1000.f});
        }
    }
    // Where a street meets a house floor or a doorway (no kerb): a stone edge down to the floor.
    auto skirt = [&](glm::vec2 a, glm::vec2 b, glm::vec3 n) {
        const glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        addQuad(kerb, {glm::vec3(a.x, RoadHeight, a.y), glm::vec3(b.x, RoadHeight, b.y), glm::vec3(b.x, 0, b.y), glm::vec3(a.x, 0, a.y)}, uv, n);
    };
    auto lower = [&](int x, int y) { return x >= x0 && y >= y0 && x < x1 && y < y1 && !road.count({x, y}); };
    for (const auto& [x, y] : road) {
        if (lower(x, y - 1)) skirt({float(x), float(y)}, {float(x + 1), float(y)}, {0, 0, -1});
        if (lower(x, y + 1)) skirt({float(x), float(y + 1)}, {float(x + 1), float(y + 1)}, {0, 0, 1});
        if (lower(x - 1, y)) skirt({float(x), float(y)}, {float(x), float(y + 1)}, {-1, 0, 0});
        if (lower(x + 1, y)) skirt({float(x + 1), float(y)}, {float(x + 1), float(y + 1)}, {1, 0, 0});
    }
    auto inWall = [&](int x, int y) {
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) if (m_fortressTiles.count({x + dx, y + dy})) return true;
        return false;
    };
    // Diagonal kerbs along the corner triangles' long side, on the street side.
    for (const auto& [tile, c] : diagonal) {
        if (inWall(tile.first, tile.second)) continue;
        const glm::vec2 corner[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        const glm::vec2 base(float(tile.first), float(tile.second));
        glm::vec2 a = base + corner[(c + 3) % 4], b = base + corner[(c + 1) % 4];
        const glm::vec2 toward = base + corner[c];
        const glm::vec2 off = glm::normalize(toward - (a + b) * 0.5f) * width;
        // Mitred into the straight kerbs at both ends.
        const glm::vec2 dir = glm::normalize(b - a);
        a -= dir * width; b += dir * width;
        const glm::vec2 q[4] = {a, b, b + off, a + off};
        const glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        auto at = [&](glm::vec2 p, float h) { return glm::vec3(p.x, h, p.y); };
        const glm::vec2 along = glm::normalize(b - a), side = glm::normalize(off);
        addQuad(kerb, {at(q[0], height), at(q[1], height), at(q[2], height), at(q[3], height)}, uv, {0, 1, 0});
        addQuad(kerb, {at(q[1], height), at(q[0], height), at(q[0], 0), at(q[1], 0)}, uv, {-side.x, 0, -side.y});
        addQuad(kerb, {at(q[3], height), at(q[2], height), at(q[2], 0), at(q[3], 0)}, uv, {side.x, 0, side.y});
        addQuad(kerb, {at(q[0], height), at(q[3], height), at(q[3], 0), at(q[0], 0)}, uv, {-along.x, 0, -along.y});
        addQuad(kerb, {at(q[2], height), at(q[1], height), at(q[1], 0), at(q[2], 0)}, uv, {along.x, 0, along.y});
    }
    {
        const auto grass = GrassMaterial::loadOrCreate(assetDirectory, m_grassColours);
        m_grassAlbedo = makeTexture(grass.size, grass.size, grass.albedo.data(), true);
        m_grassNormal = makeTexture(grass.size, grass.size, grass.normal.data(), true);
        for (unsigned id : {m_grassAlbedo, m_grassNormal}) {
            glBindTexture(GL_TEXTURE_2D, id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        auto* v = m_batches[m_groundBatch].vertices.data();
        for (size_t i = 0; i < size_t(w) * (y1 - y0) * 6; ++i)
            if (v[i].shade < 1000.f) {
                if (m_grassLayer[size_t(v[i].shade)]) v[i].shade += 2000.f;
                else if (m_grassEdgeLayer[size_t(v[i].shade)]) v[i].shade += 4000.f;
            }
    }
    for (const auto& [x, y] : road) {
        if (inWall(x, y)) continue;
        if (open(x, y - 1)) box({float(x), float(y)}, {float(x + 1), y + width});
        if (open(x, y + 1)) box({float(x), y + 1 - width}, {float(x + 1), float(y + 1)});
        if (open(x - 1, y)) box({float(x), float(y)}, {x + width, float(y + 1)});
        if (open(x + 1, y)) box({x + 1 - width, float(y)}, {float(x + 1), float(y + 1)});
    }
}

void Britannia3dView::buildGround(const U7::Data& data) {
    // Detail ground: one quad per tile, its flat tile from a texture array of all flat tiles
    // used, each upscaled to TexelsPerTile (as a 3 x 3 repeat, so it stays seamless).
    constexpr int c = U7::ChunkTiles, factor = TexelsPerTile / P, tiled = 3 * P;
    const int x0 = m_chunkX * c, y0 = m_chunkY * c, x1 = (m_chunkX1 + 1) * c, y1 = (m_chunkY1 + 1) * c;
    std::map<int, int> layers;
    std::vector<int> tileLayer(size_t(x1 - x0) * (y1 - y0));
    std::vector<std::pair<int, int>> flats;
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x) {
            const auto t = U7GroundGrid::groundTile(data, x, y);
            auto [it, added] = layers.try_emplace(t.shape << 5 | t.frame, int(flats.size()));
            if (added) flats.push_back({t.shape, t.frame});
            tileLayer[size_t(y - y0) * (x1 - x0) + (x - x0)] = it->second;
        }
    std::vector<std::vector<std::uint8_t>> images(flats.size());
    std::atomic<size_t> next{0};
    std::vector<std::thread> threads;
    for (unsigned w = 0; w < std::max(1u, std::thread::hardware_concurrency()); ++w)
        threads.emplace_back([&] {
            for (size_t i; (i = next++) < flats.size();) {
                const auto pixels = U7GroundGrid::rgba(data, flats[i].first, flats[i].second);
                std::vector<std::uint8_t> repeated(size_t(tiled) * tiled * 4);
                for (int y = 0; y < tiled; ++y)
                    for (int x = 0; x < tiled; ++x)
                        std::copy_n(pixels.begin() + (size_t(y % P) * P + x % P) * 4, 4, repeated.begin() + (size_t(y) * tiled + x) * 4);
                const auto scaled = PixelArtScale::scale(repeated.data(), tiled, tiled, factor);
                auto& image = images[i];
                image.resize(size_t(TexelsPerTile) * TexelsPerTile * 4);
                for (int y = 0; y < TexelsPerTile; ++y)
                    std::copy_n(scaled.begin() + (size_t(y + TexelsPerTile) * tiled * factor + TexelsPerTile) * 4, size_t(TexelsPerTile) * 4,
                                image.begin() + size_t(y) * TexelsPerTile * 4);
            }
        });
    for (auto& thread : threads) thread.join();

    // Street tiles: mostly grey, hardly coloured pixels (U7's cobbles and kerb stones).
    m_roadLayer.assign(flats.size(), false);
    m_roadColours.clear();
    for (size_t i = 0; i < flats.size(); ++i) {
        const auto pixels = U7GroundGrid::rgba(data, flats[i].first, flats[i].second);
        int grey = 0;
        for (int k = 0; k < P * P; ++k) {
            const auto* p = pixels.data() + k * 4;
            const int high = std::max({p[0], p[1], p[2]}), low = std::min({p[0], p[1], p[2]});
            if (high - low < 16 && high > 25 && high < 215) ++grey;
        }
        m_roadLayer[i] = grey >= P * P * 7 / 10;
        if (m_roadLayer[i] && m_roadColours.size() < 4000)
            for (int k = 0; k < P * P; ++k) {
                const auto* p = pixels.data() + k * 4;
                const glm::vec3 c(p[0] / 255.f, p[1] / 255.f, p[2] / 255.f);
                if (c.r > 0.2f) m_roadColours.push_back(c);
            }
    }
    // Street tiles with dirt: a quarter or more grey stone pixels, not a full street tile.
    m_mixedLayer.assign(flats.size(), false);
    for (size_t i = 0; i < flats.size(); ++i) {
        if (m_roadLayer[i]) continue;
        const auto pixels = U7GroundGrid::rgba(data, flats[i].first, flats[i].second);
        int grey = 0;
        for (int k = 0; k < P * P; ++k) {
            const auto* p = pixels.data() + k * 4;
            const int high = std::max({p[0], p[1], p[2]}), low = std::min({p[0], p[1], p[2]});
            if (high - low < 16 && high > 25 && high < 215) ++grey;
        }
        m_mixedLayer[i] = grey >= P * P / 4;
    }
    // Lawn tiles: mostly green pixels; lawn edges: a quarter or more green, with earth.
    m_grassLayer.assign(flats.size(), false);
    m_grassEdgeLayer.assign(flats.size(), false);
    m_grassColours.clear();
    for (size_t i = 0; i < flats.size(); ++i) {
        const auto pixels = U7GroundGrid::rgba(data, flats[i].first, flats[i].second);
        int green = 0;
        for (int k = 0; k < P * P; ++k) {
            const auto* p = pixels.data() + k * 4;
            if (p[1] > p[0] + 6 && p[1] > p[2] + 6) ++green;
        }
        m_grassLayer[i] = green >= P * P * 9 / 10;
        m_grassEdgeLayer[i] = !m_grassLayer[i] && green >= P * P / 4 && !m_roadLayer[i];
        if (m_grassLayer[i] && m_grassColours.size() < 4000)
            for (int k = 0; k < P * P; ++k) {
                const auto* p = pixels.data() + k * 4;
                if (p[1] > p[0] + 6 && p[1] > p[2] + 6) m_grassColours.push_back(glm::vec3(p[0], p[1], p[2]) / 255.f);
            }
    }
    m_groundRect = glm::ivec4(x0, y0, x1, y1);
    m_tileLayer = tileLayer;

    GLuint array = 0;
    glGenTextures(1, &array);
    glBindTexture(GL_TEXTURE_2D_ARRAY, array);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, TexelsPerTile, TexelsPerTile, GLsizei(flats.size()), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    for (size_t i = 0; i < images.size(); ++i)
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, GLint(i), TexelsPerTile, TexelsPerTile, 1, GL_RGBA, GL_UNSIGNED_BYTE, images[i].data());
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D_ARRAY, 0x84FE /* GL_TEXTURE_MAX_ANISOTROPY, core 4.6 */, 8.f);
    m_textures.push_back(array);

    m_groundBatch = m_batches.size();
    auto& batch = m_batches.emplace_back();
    batch.texture = array;
    batch.array = true;
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x) {
            const float layer = float(tileLayer[size_t(y - y0) * (x1 - x0) + (x - x0)]);
            const glm::vec3 p[4] = {{x, 0, y}, {x + 1, 0, y}, {x + 1, 0, y + 1}, {x, 0, y + 1}};
            const glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
            for (int i : {0, 1, 2, 0, 2, 3})
                batch.vertices.push_back({p[i].x, p[i].y, p[i].z, 0, 1, 0, uv[i].x, uv[i].y, layer});
        }
}

glm::mat4 Britannia3dView::doorModel(const Door& door) const {
    return glm::rotate(glm::translate(glm::mat4(1), door.pivot), door.baseYaw + door.angle, glm::vec3(0, 1, 0));
}

void Britannia3dView::toggleDoor(size_t index, bool immediately) {
    if (index >= m_doors.size()) return;
    auto& door = m_doors[index];
    door.open = !door.open;
    if (immediately) door.angle = door.open ? door.openAngle : 0.f;
}

int Britannia3dView::pickDoor(glm::vec2 ndc) const {
    // The leaf's outer face as a screen quad; the nearest hit wins.
    const auto viewProjection = m_camera.projectionMatrix() * m_camera.viewMatrix();
    int best = -1;
    float bestDepth = 1e30f;
    for (size_t i = 0; i < m_doors.size(); ++i) {
        const auto model = doorModel(m_doors[i]);
        glm::vec2 corner[4];
        float depth = 0;
        bool visible = true;
        const float l = m_doors[i].length, h = m_doors[i].height;
        const glm::vec3 local[4] = {{0, 0, 0}, {-l, 0, 0}, {-l, h, 0}, {0, h, 0}};
        for (int k = 0; k < 4; ++k) {
            const auto clip = viewProjection * glm::vec4(planet(glm::vec3(model * glm::vec4(local[k], 1))), 1);
            if (clip.w <= 0) { visible = false; break; }
            corner[k] = glm::vec2(clip) / clip.w;
            depth += clip.w / 4;
        }
        if (!visible) continue;
        bool inside = true;
        float sign = 0;
        for (int k = 0; k < 4 && inside; ++k) {
            const auto a = corner[k], b = corner[(k + 1) % 4];
            const float side = (b.x - a.x) * (ndc.y - a.y) - (b.y - a.y) * (ndc.x - a.x);
            if (sign == 0) sign = side;
            else if (side * sign < 0) inside = false;
        }
        if (inside && depth < bestDepth) { bestDepth = depth; best = int(i); }
    }
    return best;
}

void Britannia3dView::drawDoors(unsigned program) {
    if (m_doors.empty()) return;
    const GLint model = glGetUniformLocation(program, "flatModel"), planks = glGetUniformLocation(program, "planks"),
                tint = glGetUniformLocation(program, "tint");
    glm::vec3 wood(0.45f, 0.32f, 0.24f);
    for (const auto& batch : m_batches) if (batch.planks) { wood = batch.tint; break; }
    for (const auto& door : m_doors) {
        const auto matrix = doorModel(door);
        glUniformMatrix4fv(model, 1, GL_FALSE, &matrix[0][0]);
        if (door.kind == 1) {
            glUniform1i(planks, 0);
            glBindTexture(GL_TEXTURE_2D, m_windowWoodTexture);
            m_windowFrame.bind();
            glDrawArrays(GL_TRIANGLES, 0, GLsizei(m_windowFrame.vertexCount()));
            glBindTexture(GL_TEXTURE_2D, m_windowGlassTexture);
            m_windowGlass.bind();
            glDrawArrays(GL_TRIANGLES, 0, GLsizei(m_windowGlass.vertexCount()));
            continue;
        }
        glUniform3f(tint, wood.r * 0.9f, wood.g * 0.9f, wood.b * 0.9f);
        glUniform1i(planks, 2);
        m_doorPlanks.bind();
        glDrawArrays(GL_TRIANGLES, 0, GLsizei(m_doorPlanks.vertexCount()));
        glUniform1i(planks, 1);
        m_doorBattens.bind();
        glDrawArrays(GL_TRIANGLES, 0, GLsizei(m_doorBattens.vertexCount()));
        glUniform1i(planks, 0);
        glBindTexture(GL_TEXTURE_2D, m_ironTexture);
        m_doorIron.bind();
        glDrawArrays(GL_TRIANGLES, 0, GLsizei(m_doorIron.vertexCount()));
    }
    const glm::mat4 identity(1);
    glUniformMatrix4fv(model, 1, GL_FALSE, &identity[0][0]);
}

namespace {

// A box in the flat world: a (tiles) along the run, depth across it (metres), y up (metres).
struct Run {
    bool alongX;
    float line;                       // z (alongX) or x of the wall's middle, tiles
    glm::vec3 at(float a, float depth, float y) const {
        const float d = depth / U73dScale::TileMetres;
        return alongX ? glm::vec3(a, y, line + d) : glm::vec3(line + d, y, a);
    }
    glm::vec3 axis(glm::vec3 n) const { return alongX ? n : glm::vec3(n.z, n.y, n.x); }
};

}

void Britannia3dView::buildOpenings(const U7::Data& data, const std::vector<U7::WorldObject>& windows, const std::vector<U7::WorldObject>& doors) {
    constexpr float tm = U73dScale::TileMetres, SL = U73dScale::StructureLift;
    // Frame wood and the lit U7 glass.
    const std::uint8_t wood[4] = {104, 86, 70, 255};
    constexpr int gs = 128;
    std::vector<std::uint8_t> glass(size_t(gs) * gs * 4);
    for (int y = 0; y < gs; ++y)
        for (int x = 0; x < gs; ++x) {
            // Warm yellow, a little brighter in the middle, with a faint diamond leading.
            const float fx = float(x) / gs, fy = float(y) / gs;
            const float glow = 1.f - 0.25f * (std::abs(fx - .5f) + std::abs(fy - .5f));
            const float d1 = std::fmod(fx * 4 + fy * 6, 1.f), d2 = std::fmod(fx * 4 - fy * 6 + 8, 1.f);
            const bool lead = d1 < 0.04f || d2 < 0.04f;
            auto* p = glass.data() + (size_t(y) * gs + x) * 4;
            const float l = lead ? 0.35f : glow;
            p[0] = std::uint8_t(std::min(255.f, 250 * l)); p[1] = std::uint8_t(std::min(255.f, 205 * l)); p[2] = std::uint8_t(std::min(255.f, 110 * l)); p[3] = 255;
        }
    m_windowWoodTexture = makeTexture(1, 1, wood, false);
    auto& frame = m_batches.emplace_back();
    frame.texture = m_windowWoodTexture;
    frame.layer = 1;
    const size_t frameIndex = m_batches.size() - 1;
    m_windowGlassTexture = makeTexture(gs, gs, glass.data(), true);
    auto& stone = m_batches.emplace_back();
    stone.planks = 4;                                  // dressed stone: sills and door jambs, lighter
    stone.tint = glm::vec3(0.82f);
    stone.layer = 1;
    const size_t stoneIndex = m_batches.size() - 1;
    auto box = [&](size_t batch, const Run& run, float a0, float a1, float d0, float d1, float y0, float y1) {
        const glm::vec3 c[8] = {run.at(a0, d0, y0), run.at(a1, d0, y0), run.at(a1, d1, y0), run.at(a0, d1, y0),
                                run.at(a0, d0, y1), run.at(a1, d0, y1), run.at(a1, d1, y1), run.at(a0, d1, y1)};
        const int f[6][4] = {{4, 5, 6, 7}, {3, 2, 1, 0}, {0, 1, 5, 4}, {2, 3, 7, 6}, {1, 2, 6, 5}, {3, 0, 4, 7}};
        const glm::vec3 n[6] = {{0, 1, 0}, {0, -1, 0}, {0, 0, -1}, {0, 0, 1}, {1, 0, 0}, {-1, 0, 0}};
        const glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        for (int k = 0; k < 6; ++k) addQuad(m_batches[batch], {c[f[k][0]], c[f[k][1]], c[f[k][2]], c[f[k][3]]}, uv, run.axis(n[k]));
    };
    const float wall = U73dScale::WallThickness * tm;           // metres
    glm::vec2 leafSize(0);
    for (const auto& w : windows) {
        const auto size = data.shapeSize(w.shape);
        const bool alongX = size.x > size.y;
        const Run run{alongX, alongX ? w.y + 0.5f : w.x + 0.5f};
        const float a1 = alongX ? w.x + 1.f : w.y + 1.f, a0 = a1 - float(alongX ? size.x : size.y);
        const float y0 = w.lift * SL, y1 = (w.lift + size.z) * SL, f = 0.09f / tm, am = (a0 + a1) / 2;
        // Frame (jambs, head, sill piece, mullion) through the wall's middle, 12 cm deep.
        box(frameIndex, run, a0, a0 + f, -0.06f, 0.06f, y0, y1);
        box(frameIndex, run, a1 - f, a1, -0.06f, 0.06f, y0, y1);
        box(frameIndex, run, a0, a1, -0.06f, 0.06f, y1 - 0.09f, y1);
        box(frameIndex, run, a0, a1, -0.06f, 0.06f, y0, y0 + 0.09f);
        (void)am;
        // Two casements meeting in the middle, hinged at the jambs, opening inwards on a click.
        {
            const float inner0 = a0 + f, inner1 = a1 - f, leaf = (inner1 - inner0) / 2, hy = y0 + 0.09f, hh = (y1 - 0.09f) - hy;
            leafSize = glm::vec2(leaf, hh);
            for (int side = 0; side < 2; ++side) {
                Door casement;
                casement.kind = 1;
                casement.length = leaf;
                casement.height = hh;
                const float hinge = side == 0 ? inner1 : inner0;
                casement.pivot = run.at(hinge, 0.f, hy);
                // Local -x runs from the hinge along the leaf: towards -a (side 0) or +a (side 1).
                const float along = alongX ? 0.f : glm::radians(-90.f);
                casement.baseYaw = along + (side == 0 ? 0.f : glm::radians(180.f));
                const glm::vec3 localMinusZ = glm::vec3(glm::rotate(glm::mat4(1), casement.baseYaw, glm::vec3(0, 1, 0)) * glm::vec4(0, 0, -1, 0));
                const glm::vec3 mid = run.at((a0 + a1) / 2, 0.f, 0.f);
                casement.openAngle = glm::radians(indoors(mid, localMinusZ) ? -80.f : 80.f);
                m_doors.push_back(casement);
            }
        }
        box(stoneIndex, run, a0 - 0.1f / tm, a1 + 0.1f / tm, -wall / 2 - 0.06f, wall / 2 + 0.06f, y0 - 0.1f, y0);
        // Reveals: the wall below and above the window in the opening (dressed stone).
        box(stoneIndex, run, a0, a1, -wall / 2, wall / 2, y1, y1 + 0.12f);
    }
    // Casement meshes: a frame of four bars and a mullion-less leaded pane (all windows share
    // one leaf size).
    if (leafSize.x > 0) {
        const float l = leafSize.x, h = leafSize.y, b = 0.06f / tm;
        std::vector<float> frameData, glassData;
        auto boxInto = [&](std::vector<float>& out, float x0, float x1, float y0, float y1, float d) {
            const glm::vec3 c[8] = {{x0, y0, -d}, {x1, y0, -d}, {x1, y0, d}, {x0, y0, d}, {x0, y1, -d}, {x1, y1, -d}, {x1, y1, d}, {x0, y1, d}};
            const int f[6][4] = {{4, 5, 6, 7}, {3, 2, 1, 0}, {0, 1, 5, 4}, {2, 3, 7, 6}, {1, 2, 6, 5}, {3, 0, 4, 7}};
            const glm::vec3 n[6] = {{0, 1, 0}, {0, -1, 0}, {0, 0, -1}, {0, 0, 1}, {1, 0, 0}, {-1, 0, 0}};
            for (int k = 0; k < 6; ++k)
                for (int i : {0, 1, 2, 0, 2, 3}) {
                    const auto& p = c[f[k][i]];
                    out.insert(out.end(), {p.x, p.y, p.z, n[k].x, n[k].y, n[k].z, 0.f, 0.f, 1.f});
                }
        };
        const float d = 0.03f / tm;
        boxInto(frameData, -l, 0, 0, b * tm, d);                      // bottom rail
        boxInto(frameData, -l, 0, h - b * tm, h, d);                  // top rail
        boxInto(frameData, -b, 0, 0, h, d);                           // hinge stile
        boxInto(frameData, -l, -l + b, 0, h, d);                      // meeting stile
        boxInto(frameData, -l, 0, h / 2 - 0.02f, h / 2 + 0.02f, d * 0.8f);   // glazing bar
        for (const float z : {-0.004f / tm, 0.004f / tm}) {
            const glm::vec3 q[4] = {{-l + b, h - b * tm, z}, {-b, h - b * tm, z}, {-b, b * tm, z}, {-l + b, b * tm, z}};
            const glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
            for (int i : {0, 1, 2, 0, 2, 3})
                glassData.insert(glassData.end(), {q[i].x, q[i].y, q[i].z, 0.f, 0.f, z > 0 ? 1.f : -1.f, uv[i].x, uv[i].y, 1.f});
        }
        m_windowFrame.create(frameData.data(), unsigned(frameData.size() / 9), 9);
        m_windowGlass.create(glassData.data(), unsigned(glassData.size() / 9), 9);
    }
    // Doors: a DoorModel door (2.25 m) in the opening, dressed stone jambs and a stone lintel up
    // to the wall's top above it (planks in wooden houses). The opening runs along the wall.
    auto& lintelStone = m_batches.emplace_back();
    lintelStone.planks = 4;
    lintelStone.tint = glm::vec3(0.5f);
    lintelStone.layer = 1;
    const size_t lintelStoneIndex = m_batches.size() - 1;
    auto& lintelWood = m_batches.emplace_back();
    lintelWood.planks = 1;
    lintelWood.layer = 1;
    for (const auto& batch : m_batches) if (batch.planks == 1 && batch.tint != glm::vec3(1)) { lintelWood.tint = batch.tint; break; }
    const size_t lintelWoodIndex = m_batches.size() - 1;
    for (const auto& d : doors) {
        const auto size = data.shapeSize(d.shape);
        const int length = std::max(size.x, size.y);
        bool converted = false;
        struct Candidate { bool alongX; float line, a0, a1; };
        const Candidate candidates[2] = {
            {true, d.y + 0.5f, float(d.x + 1 - length), float(d.x + 1)},
            {false, d.x + 0.5f, float(d.y + 1 - length), float(d.y + 1)}};
        for (const auto& c : candidates) {
            const int line = int(std::floor(c.line));
            // The wall goes on beyond at least one end (within two tiles) of the opening.
            auto solid = [&](int a) { return m_solidTiles.count(c.alongX ? std::pair<int, int>{a, line} : std::pair<int, int>{line, a}) > 0; };
            const bool before = solid(int(c.a0) - 1) || solid(int(c.a0) - 2), after = solid(int(c.a1)) || solid(int(c.a1) + 1);
            if (!before && !after) continue;
            const Run run{c.alongX, c.line};
            const float j = 0.16f / tm, base = d.lift * SL, top = (d.lift + size.z) * SL;
            auto isStone = [&](int a) { return m_stoneTiles.count(c.alongX ? std::pair<int, int>{a, line} : std::pair<int, int>{line, a}) > 0; };
            const bool stoneHouse = isStone(int(c.a0) - 1) || isStone(int(c.a1)) || isStone(int(c.a0) - 2) || isStone(int(c.a1) + 1);
            if (stoneHouse) {
                box(stoneIndex, run, c.a0, c.a0 + j, -wall / 2 - 0.05f, wall / 2 + 0.05f, base, base + U73dScale::DoorHeight + 0.2f);
                box(stoneIndex, run, c.a1 - j, c.a1, -wall / 2 - 0.05f, wall / 2 + 0.05f, base, base + U73dScale::DoorHeight + 0.2f);
                box(stoneIndex, run, c.a0, c.a1, -wall / 2 - 0.05f, wall / 2 + 0.05f, base, base + 0.04f);
                // A dressed lintel beam over the door, then the wall above it.
                box(stoneIndex, run, c.a0 - 0.1f / tm, c.a1 + 0.1f / tm, -wall / 2 - 0.05f, wall / 2 + 0.05f, base + U73dScale::DoorHeight,
                    base + U73dScale::DoorHeight + 0.2f);
            }
            box(stoneHouse ? lintelStoneIndex : lintelWoodIndex, run, c.a0, c.a1, -wall / 2, wall / 2, base + U73dScale::DoorHeight + (stoneHouse ? 0.2f : 0.f), top);
            // The door itself, hinged at the a1 end, leaf towards a0; opens inwards.
            Door door;
            door.pivot = run.at(c.a1, 0.f, base);
            door.baseYaw = c.alongX ? 0.f : glm::radians(-90.f);
            const glm::vec3 localMinusZ = glm::vec3(glm::rotate(glm::mat4(1), door.baseYaw, glm::vec3(0, 1, 0)) * glm::vec4(0, 0, -1, 0));
            door.openAngle = glm::radians(indoors(run.at((c.a0 + c.a1) / 2, 0.f, 0.f), localMinusZ) ? -90.f : 90.f);
            m_doors.push_back(door);
            converted = true;
            break;
        }
        // Not a door in a wall (an open door, a gate): the U7 door as before.
        if (!converted) addObject(data, d, 1, false, SL, 0, true);
    }
}

void Britannia3dView::buildPictures(const U7::Data& data, const std::vector<U7::WorldObject>& pictures) {
    constexpr float tm = U73dScale::TileMetres, SL = U73dScale::StructureLift;
    for (const auto& picture : pictures) {
        const auto frame = data.frame(picture.shape, picture.frame);
        if (frame.rgba.empty()) continue;
        const int w = frame.width, h = frame.height;
        // Unshear: on a north wall (plane along x) a frame pixel (u, v) is along = u - v, up = -v;
        // on a west wall (along y) along = v - u, up = -u.
        const bool alongX = w > h;
        const int cols = w + h - 1, rows = alongX ? h : w;
        std::vector<std::uint8_t> flat(size_t(cols) * rows * 4, 0);
        int left = cols, right = -1, top = rows, bottom = -1;
        for (int v = 0; v < h; ++v)
            for (int u = 0; u < w; ++u) {
                const auto* p = frame.rgba.data() + (size_t(v) * w + u) * 4;
                if (!p[3]) continue;
                const int x = alongX ? u - v + h - 1 : v - u + w - 1, y = alongX ? v : u;
                std::copy_n(p, 4, flat.data() + (size_t(y) * cols + x) * 4);
                left = std::min(left, x); right = std::max(right, x); top = std::min(top, y); bottom = std::max(bottom, y);
            }
        if (right < left) continue;
        const int cw = right - left + 3, ch = bottom - top + 3;
        std::vector<std::uint8_t> crop(size_t(cw) * ch * 4, 0);
        for (int y = 0; y < ch - 2; ++y)
            std::copy_n(flat.begin() + (size_t(y + top) * cols + left) * 4, size_t(cw - 2) * 4, crop.begin() + (size_t(y + 1) * cw + 1) * 4);
        // Fill single transparent pixels the unshearing left inside the picture.
        for (int y = 1; y + 1 < ch; ++y)
            for (int x = 1; x + 1 < cw; ++x) {
                auto* p = crop.data() + (size_t(y) * cw + x) * 4;
                if (p[3]) continue;
                const auto* l = p - 4; const auto* r = p + 4;
                if (l[3] && r[3]) for (int k = 0; k < 4; ++k) p[k] = std::uint8_t((l[k] + r[k]) / 2);
            }
        constexpr int factor = 8;
        auto big = PixelArtScale::scale(crop.data(), cw, ch, factor);
        PixelArtScale::smoothEdges(big, cw * factor, ch * factor, 3);
        // Size: 16 frame pixels per metre (U7's 8 per tile); on the nearest wall's inner face.
        const float width = cw / 16.f, height = ch / 16.f;
        const auto size = data.shapeSize(picture.shape);
        float centre, face = 0;
        bool found = false;
        if (alongX) {
            centre = picture.x + 1 - size.x * 0.5f;
            for (int row = picture.y; row >= picture.y - 2 && !found; --row)
                if (m_solidTiles.count({int(centre), row})) { face = row + 0.5f + U73dScale::WallThickness * 0.5f + 0.02f; found = true; }
        } else {
            centre = picture.y + 1 - size.y * 0.5f;
            for (int col = picture.x; col >= picture.x - 2 && !found; --col)
                if (m_solidTiles.count({col, int(centre)})) { face = col + 0.5f + U73dScale::WallThickness * 0.5f + 0.02f; found = true; }
        }
        if (!found) face = (alongX ? picture.y : picture.x) + 0.1f;
        const float bottomY = std::max(picture.lift * SL, 0.9f), topY = bottomY + height;
        const float a0 = centre - width / 2 / tm, a1 = centre + width / 2 / tm;
        auto at = [&](float a, float y, float off) { return alongX ? glm::vec3(a, y, face + off) : glm::vec3(face + off, y, a); };
        auto& batch = m_batches.emplace_back();
        batch.texture = makeTexture(cw * factor, ch * factor, big.data(), true);
        batch.layer = 2;
        const glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        const glm::vec3 n = alongX ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
        addQuad(batch, {at(a0, topY, 0), at(a1, topY, 0), at(a1, bottomY, 0), at(a0, bottomY, 0)}, uv, n);
        ++m_boxes;
    }
}

void Britannia3dView::buildWells(const std::vector<glm::vec2>& places) {
    if (places.empty()) return;
    constexpr float tm = U73dScale::TileMetres;
    const auto parts = WellModel::build();
    glm::vec3 woodTone(0.45f, 0.32f, 0.24f);
    for (const auto& batch : m_batches) if (batch.planks == 1 && batch.tint != glm::vec3(1)) { woodTone = batch.tint; break; }
    const std::uint8_t iron[4] = {40, 38, 36, 255}, rope[4] = {150, 128, 92, 255}, water[4] = {18, 34, 70, 255};
    struct Part { const std::vector<WellModel::Vertex>* vertices; int planks; glm::vec3 tint; unsigned texture; };
    const Part list[6] = {{&parts.stone, 4, glm::vec3(0.5f), 0}, {&parts.rim, 4, glm::vec3(0.7f), 0}, {&parts.wood, 1, woodTone, 0},
                          {&parts.iron, 0, glm::vec3(1), makeTexture(1, 1, iron, false)}, {&parts.rope, 0, glm::vec3(1), makeTexture(1, 1, rope, false)},
                          {&parts.water, 0, glm::vec3(1), makeTexture(1, 1, water, false)}};
    for (const auto& part : list) {
        auto& batch = m_batches.emplace_back();
        batch.planks = part.planks;
        batch.tint = part.tint;
        batch.texture = part.texture;
        batch.layer = 2;
        for (const auto& place : places)
            for (const auto& v : *part.vertices)
                batch.vertices.push_back({place.x + v.position.x / tm, v.position.y, place.y + v.position.z / tm,
                                          v.normal.x, v.normal.y, v.normal.z, v.uv.x, v.uv.y, 1.f});
    }
    // The wall stops Sir Canegm: a ring of wall triangles.
    for (const auto& place : places) {
        auto& cell = m_walls[{int(place.x) / 8, int(place.y) / 8}];
        for (int i = 0; i < 16; ++i) {
            const float a0 = 6.2831853f * i / 16, a1 = 6.2831853f * (i + 1) / 16, r = 0.74f / tm;
            const glm::vec3 p0(place.x + std::cos(a0) * r, 0, place.y + std::sin(a0) * r), p1(place.x + std::cos(a1) * r, 0, place.y + std::sin(a1) * r);
            const glm::vec3 up(0, 0.78f, 0);
            for (const auto& q : {p0, p1, p1 + up, p0, p1 + up, p0 + up}) cell.push_back(q);
        }
    }
}

void Britannia3dView::buildLamps(const std::vector<glm::vec2>& places) {
    if (places.empty()) return;
    constexpr float tm = U73dScale::TileMetres;
    const auto parts = LampModel::build();
    const auto post = LampModel::postTexture(128), glass = LampModel::glassTexture(128);
    const std::uint8_t iron[4] = {38, 36, 34, 255};
    struct Part { const std::vector<LampModel::Vertex>* vertices; unsigned texture; float shade; };
    const Part list[3] = {{&parts.post, makeTexture(128, 128, post.data(), true), 1.f},
                          {&parts.iron, makeTexture(1, 1, iron, false), 1.f},
                          {&parts.glass, makeTexture(128, 128, glass.data(), true), 1.55f}};   // lit: brighter than the light
    for (const auto& part : list) {
        auto& batch = m_batches.emplace_back();
        batch.texture = part.texture;
        batch.layer = 2;
        for (const auto& place : places) {
            // Turned like U7's: the crossbar runs from north west to south east.
            const float c = 0.7071f, sn = 0.7071f;
            for (const auto& v : *part.vertices) {
                const glm::vec3 p(c * v.position.x - sn * v.position.z, v.position.y, sn * v.position.x + c * v.position.z);
                const glm::vec3 n(c * v.normal.x - sn * v.normal.z, v.normal.y, sn * v.normal.x + c * v.normal.z);
                batch.vertices.push_back({place.x + p.x / tm, p.y, place.y + p.z / tm, n.x, n.y, n.z, v.uv.x, v.uv.y, part.shade});
            }
        }
    }
}

void Britannia3dView::buildSignposts(const U7::Data& data, const std::vector<U7::WorldObject>& posts,
                                     const std::vector<U7::WorldObject>& signs) {
    constexpr float tm = U73dScale::TileMetres;
    std::vector<bool> postUsed(posts.size(), false), signUsed(signs.size(), false);
    std::vector<int> postOf(signs.size(), -1);
    for (size_t s = 0; s < signs.size(); ++s)
        for (size_t p = 0; p < posts.size(); ++p)
            if (std::abs(signs[s].x - posts[p].x) <= 1 && std::abs(signs[s].y - posts[p].y) <= 1) {
                postUsed[p] = signUsed[s] = true;
                postOf[s] = int(p);
                break;
            }
    // Posts and boards without a partner stay U7 graphics (placed first: addObject can grow
    // m_batches, which would leave a reference into it dangling).
    for (size_t p = 0; p < posts.size(); ++p) if (!postUsed[p]) addObject(data, posts[p], 2, false, U73dScale::FurnitureLift, 0, false);
    for (size_t k = 0; k < signs.size(); ++k) if (!signUsed[k]) addObject(data, signs[k], 2, false, U73dScale::FurnitureLift, 0, false);
    const auto wood = LampModel::postTexture(128);
    auto& batch = m_batches.emplace_back();
    batch.texture = makeTexture(128, 128, wood.data(), true);
    glBindTexture(GL_TEXTURE_2D, batch.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    batch.layer = 2;
    // A prism from a 2D outline (x along, y up, metres; counter-clockwise) extruded by
    // thickness across `side`, placed at origin (metres), into the batch.
    auto prism = [&](const std::vector<glm::vec2>& outline, float thickness, glm::vec3 origin, glm::vec3 along, glm::vec3 side, float shade) {
        auto at = [&](glm::vec2 q, float d) {
            const glm::vec3 m = origin + along * q.x + glm::vec3(0, q.y, 0) + side * d;
            return glm::vec3(m.x / tm, m.y, m.z / tm);
        };
        const float h = thickness / 2;
        for (const float d : {-h, h})
            for (size_t i = 1; i + 1 < outline.size(); ++i) {
                const glm::vec2 q[3] = {outline[0], outline[i], outline[i + 1]};
                const glm::vec3 n = side * (d > 0 ? 1.f : -1.f);
                for (const auto& v : q) {
                    const auto p = at(v, d);
                    batch.vertices.push_back({p.x, p.y, p.z, n.x, n.y, n.z, v.x * 1.5f, v.y * 3.f, shade});
                }
            }
        for (size_t i = 0; i < outline.size(); ++i) {
            const auto a = outline[i], b = outline[(i + 1) % outline.size()];
            const glm::vec2 e = glm::normalize(b - a);
            const glm::vec3 n = glm::normalize(along * e.y - glm::vec3(0, e.x, 0));
            const glm::vec3 pts[4] = {at(a, -h), at(b, -h), at(b, h), at(a, h)};
            const glm::vec2 uv[4] = {{a.x * 1.5f, a.y * 3.f}, {b.x * 1.5f, b.y * 3.f}, {b.x * 1.5f, b.y * 3.f + 0.05f}, {a.x * 1.5f, a.y * 3.f + 0.05f}};
            for (int j : {0, 1, 2, 0, 2, 3}) batch.vertices.push_back({pts[j].x, pts[j].y, pts[j].z, n.x, n.y, n.z, uv[j].x, uv[j].y, shade * 0.9f});
        }
    };
    for (size_t p = 0; p < posts.size(); ++p) {
        if (!postUsed[p]) continue;
        const glm::vec3 base((posts[p].x + 0.5f) * tm, 0, (posts[p].y + 0.5f) * tm);
        // Square post 12 cm thick, 2.4 m high, with a pointed top.
        const float w = 0.06f;
        const std::vector<glm::vec2> outline = {{-w, 0}, {w, 0}, {w, 2.4f}, {0, 2.52f}, {-w, 2.4f}};
        prism(outline, 2 * w, base, {1, 0, 0}, {0, 0, 1}, 1.f);
        prism(outline, 2 * w, base, {0, 0, 1}, {1, 0, 0}, 0.95f);
    }
    for (size_t s = 0; s < signs.size(); ++s) {
        const auto& sign = signs[s];
        if (!signUsed[s]) continue;
        // Frames 7 - 10 point along x, 0 - 3 along y; even frames east / south, odd west / north.
        const auto& post = posts[size_t(postOf[s])];
        const glm::vec3 base((post.x + 0.5f) * tm, 0, (post.y + 0.5f) * tm);
        const bool alongX = sign.frame >= 7;
        const float direction = sign.frame % 2 == 0 ? 1.f : -1.f;
        const glm::vec3 along = alongX ? glm::vec3(direction, 0, 0) : glm::vec3(0, 0, direction);
        const glm::vec3 side = alongX ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
        const float y = std::clamp(1.35f + 0.3f * (sign.lift - 3), 1.1f, 2.2f);
        // Arrow board: 1.1 m long, 0.2 m high, pointed, fixed to the side of the post.
        prism({{0.0f, y}, {0.98f, y}, {1.14f, y + 0.1f}, {0.98f, y + 0.2f}, {0.0f, y + 0.2f}}, 0.035f, base + side * 0.08f, along, side, 1.05f);
    }
}

void Britannia3dView::buildTrees(const std::vector<glm::vec2>& places) {
    if (places.empty()) return;
    constexpr float tm = U73dScale::TileMetres;
    const auto bark = TreeModel::barkTexture(256), leaves = TreeModel::leafTexture(512);
    auto& barkBatch = m_batches.emplace_back();
    barkBatch.texture = makeTexture(256, 256, bark.data(), true);
    glBindTexture(GL_TEXTURE_2D, barkBatch.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    barkBatch.layer = 3;
    barkBatch.wind = 1;
    const size_t barkIndex = m_batches.size() - 1;
    auto& leafBatch = m_batches.emplace_back();
    leafBatch.texture = makeTexture(512, 512, leaves.data(), true);
    leafBatch.layer = 3;
    leafBatch.wind = 2;
    for (const auto& place : places) {
        // The place decides the tree: 7 to 11 m, its own branching.
        const std::uint32_t seed = std::uint32_t(int(place.x)) * 73856093u ^ std::uint32_t(int(place.y)) * 19349663u;
        auto tree = TreeModel::build(seed, 7.f, 11.f);
        const float turn = float(seed % 628) / 100.f, c = std::cos(turn), sn = std::sin(turn);
        // Keep the crown (and its sway) 0.9 m clear of the nearest wall or roof: the tree is
        // narrowed horizontally to fit, its height stays.
        float clearance = 1e9f;
        glm::vec2 away(0);                                   // from the buildings, metres
        for (int dy = -24; dy <= 24; ++dy)
            for (int dx = -24; dx <= 24; ++dx) {
                const std::pair<int, int> tile{int(std::floor(place.x)) + dx, int(std::floor(place.y)) + dy};
                if (!m_solidTiles.count(tile) && !m_roofTiles.count(tile)) continue;
                const float nx = std::clamp(place.x, float(tile.first), float(tile.first + 1));
                const float nz = std::clamp(place.y, float(tile.second), float(tile.second + 1));
                const glm::vec2 off = glm::vec2(place.x - nx, place.y - nz) * tm;
                const float dist = glm::length(off);
                clearance = std::min(clearance, dist);
                if (dist > 0.01f && dist < 8.f) away += off / (dist * dist);
            }
        float reach = 0.f;
        for (const auto& v : tree.leaves) reach = std::max(reach, glm::length(glm::vec2(v.position.x, v.position.z)));
        for (const auto& v : tree.bark) if (v.position.y > 2.f) reach = std::max(reach, glm::length(glm::vec2(v.position.x, v.position.z)));
        // Near a building the crown is flattened on the building's side (keeping 0.9 m clear)
        // and grows fuller on the opposite side instead, so it keeps its volume.
        if (glm::length(away) > 1e-4f && reach > 0) {
            const glm::vec2 dir = glm::normalize(away);
            const glm::vec2 awayModel(c * dir.x + sn * dir.y, -sn * dir.x + c * dir.y);   // undoes the turn below
            const float squeeze = std::clamp((clearance - 0.9f) / reach, 0.1f, 1.f);
            const float grow = 1.f + 0.6f * (1.f - squeeze);
            for (auto* part : {&tree.leaves, &tree.bark})
                for (auto& v : *part) {
                    if (v.position.y <= 0.6f) continue;
                    const glm::vec2 xz(v.position.x, v.position.z);
                    const float along = glm::dot(xz, awayModel);
                    const glm::vec2 across = xz - awayModel * along;
                    // Low on the trunk nothing changes; the crown fully.
                    const float w = std::clamp((v.position.y - 1.5f) / 2.5f, 0.f, 1.f);
                    const float scaled = along < 0 ? along * squeeze : along * grow;
                    const glm::vec2 moved = across + awayModel * (along + (scaled - along) * w);
                    v.position.x = moved.x; v.position.z = moved.y;
                }
        }
        auto add = [&](Batch& batch, const std::vector<TreeModel::Vertex>& vertices, float shade) {
            for (const auto& v : vertices) {
                const glm::vec3 p(c * v.position.x - sn * v.position.z, v.position.y, sn * v.position.x + c * v.position.z);
                const glm::vec3 n(c * v.normal.x - sn * v.normal.z, v.normal.y, sn * v.normal.x + c * v.normal.z);
                batch.vertices.push_back({place.x + p.x / tm, p.y, place.y + p.z / tm, n.x, n.y, n.z, v.uv.x, v.uv.y * 0.6f, shade});
            }
        };
        add(m_batches[barkIndex], tree.bark, 1.f);
        add(leafBatch, tree.leaves, 1.f);
        ++m_quadObjects;
    }
}

void Britannia3dView::buildSignAndFork(const U7::Data& data) {
    constexpr float tm = U73dScale::TileMetres;
    // --- Horse sign: U7 draws it on a plane along y (x fixed); a point (y, h) of that plane is
    // frame pixel (c - 8h, 8y - 8h), so pixel (u, v) unshears to along = v - u, up = -u.
    const auto frame = data.frame(361, 7);
    if (!frame.rgba.empty()) {
        const int w = frame.width, h = frame.height, uw = w + h - 1;
        std::vector<std::uint8_t> flat(size_t(uw) * w * 4, 0);
        int left = uw, right = -1, top = w, bottom = -1;
        for (int v = 0; v < h; ++v)
            for (int u = 0; u < w; ++u) {
                const auto* p = frame.rgba.data() + (size_t(v) * w + u) * 4;
                if (!p[3]) continue;
                const int x = v - u + w - 1, y = u;
                // Wrought iron: the silhouette in one dark iron colour.
                auto* q = flat.data() + (size_t(y) * uw + x) * 4;
                q[0] = 34; q[1] = 32; q[2] = 30; q[3] = 255;
                left = std::min(left, x); right = std::max(right, x); top = std::min(top, y); bottom = std::max(bottom, y);
            }
        if (right >= left) {
            const int cw = right - left + 3, ch = bottom - top + 3;     // one pixel margin
            std::vector<std::uint8_t> crop(size_t(cw) * ch * 4, 0);
            for (int y = 0; y < ch - 2; ++y)
                std::copy_n(flat.begin() + (size_t(y + top) * uw + left) * 4, size_t(cw - 2) * 4, crop.begin() + (size_t(y + 1) * cw + 1) * 4);
            constexpr int factor = 16;
            auto big = PixelArtScale::scale(crop.data(), cw, ch, factor);
            PixelArtScale::smoothEdges(big, cw * factor, ch * factor, 6);
            auto& sign = m_batches.emplace_back();
            sign.texture = makeTexture(cw * factor, ch * factor, big.data(), true);
            sign.layer = 2;
            // 8 pixels = 0.3125 m (half its U7 size, 25 % larger). The bracket (image left) sits on the wall face
            // west of the door, the sign reaches south out of the wall, its bar at 2.7 m.
            const float scale = 0.25f * 1.25f / 8.f, width = cw * scale, height = ch * scale;   // half U7 size, then 25 % larger
            const float x = 1066.0f, wall = 2207.875f, topY = 2.75f;
            const glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
            addQuad(sign, {glm::vec3(x, topY, wall), glm::vec3(x, topY, wall + width / tm), glm::vec3(x, topY - height, wall + width / tm),
                           glm::vec3(x, topY - height, wall)}, uv, {1, 0, 0});
        }
    }
    // --- Pitchfork in the gargoyle's chest: U7's frame 589:0 (handle lower left, tines upper
    // right) on one quad along the fork, from the floor in front up into the chest.
    const auto fork = m_models.find({589, 0});
    const auto forkFrame = data.frame(589, 0);
    if (fork != m_models.end() && !forkFrame.rgba.empty()) {
        const auto chest = StableScene::u7Tile(661.90f, 1217.80f);
        // The tines go into the chest (1.3 m up, inside the body), the handle rests on the floor in front.
        const glm::vec3 tip(chest.x + 0.08f / tm, 1.25f, chest.y - 0.75f / tm), handle(chest.x - 0.25f / tm, 0.03f, chest.y + 0.75f / tm);
        auto metres = [&](glm::vec3 v) { return glm::vec3(v.x * tm, v.y, v.z * tm); };
        auto tiles = [&](glm::vec3 v) { return glm::vec3(v.x / tm, v.y, v.z / tm); };
        const glm::vec3 axis = glm::normalize(metres(tip - handle));
        const glm::vec3 across = glm::normalize(glm::cross(axis, glm::vec3(0, 1, 0)));
        const float fw = float(forkFrame.width), fh = float(forkFrame.height);
        const glm::vec2 h0(1.f, fh - 1.f), t0(fw - 1.f, 1.f);        // handle end and tines in the frame
        const glm::vec2 ds = glm::normalize(t0 - h0), ps(-ds.y, ds.x);
        const float k = glm::length(metres(tip - handle)) / glm::length(t0 - h0);   // metres per pixel
        auto place = [&](glm::vec2 s) {
            const float a = glm::dot(s - h0, ds), b = glm::dot(s - h0, ps);
            return handle + tiles(axis * (a * k) + across * (b * k));
        };
        // Blood: the tines and the upper shaft (what went into the chest) are soaked dark red,
        // with runs down the handle; the lower handle stays clean.
        std::vector<std::uint8_t> bloody = forkFrame.rgba;
        const float span = glm::length(t0 - h0);
        for (int y = 0; y < forkFrame.height; ++y)
            for (int x = 0; x < forkFrame.width; ++x) {
                auto* p = bloody.data() + (size_t(y) * forkFrame.width + x) * 4;
                if (!p[3]) continue;
                const float along = glm::dot(glm::vec2(x + 0.5f, y + 0.5f) - h0, ds) / span;   // 0 handle .. 1 tines
                const std::uint32_t hash = std::uint32_t(x * 73856093) ^ std::uint32_t(y * 19349663);
                const float speck = float((hash >> 8) & 255) / 255.f;
                const bool soaked = along > 0.72f || (along > 0.45f && speck < (along - 0.45f) * 2.6f);
                if (!soaked) continue;
                const float dark = 0.45f + 0.55f * speck;
                p[0] = std::uint8_t(std::min(255.f, 70.f + 110.f * dark)); p[1] = std::uint8_t(4 + 8 * dark); p[2] = std::uint8_t(4 + 8 * dark);
            }
        constexpr int factor = 8;
        auto big = PixelArtScale::scale(bloody.data(), forkFrame.width, forkFrame.height, factor);
        PixelArtScale::smoothEdges(big, forkFrame.width * factor, forkFrame.height * factor, factor / 2);
        auto& batch = m_batches.emplace_back();
        batch.texture = makeTexture(forkFrame.width * factor, forkFrame.height * factor, big.data(), true);
        batch.layer = 2;
        const glm::vec2 corners[4] = {{0, 0}, {fw, 0}, {fw, fh}, {0, fh}};
        const glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        addQuad(batch, {place(corners[0]), place(corners[1]), place(corners[2]), place(corners[3])}, uv, glm::cross(across, axis));
    }
}

static void loadRepeating(unsigned id) {
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void Britannia3dView::addGableRoof(Batch& roof, Batch& gable, float x0, float z0, float x1, float z1, float eave, float pitchDegrees,
                                   float overhang, float thick, bool ridgeCap) {
    // Built across a (x) and along b (z, the ridge) in tiles, then turned so the ridge runs
    // along the longer side. Heights are metres, overhang and thickness metres.
    constexpr float tm = U73dScale::TileMetres;
    const bool alongZ = (z1 - z0) >= (x1 - x0);
    const float a0 = alongZ ? x0 : z0, a1 = alongZ ? x1 : z1, b0 = alongZ ? z0 : x0, b1 = alongZ ? z1 : x1;
    auto place = [&](float a, float y, float b) { return alongZ ? glm::vec3(a, y, b) : glm::vec3(b, y, a); };
    auto turn = [&](glm::vec3 n) { return alongZ ? n : glm::vec3(n.z, n.y, n.x); };
    const float pitch = glm::radians(pitchDegrees);
    const float half = (a1 - a0) * 0.5f * tm, mid = (a0 + a1) * 0.5f, rise = half * std::tan(pitch);
    const float ridge = eave + rise, outer = half + overhang, low = eave - overhang * std::tan(pitch);
    const float bo0 = b0 - overhang / tm, bo1 = b1 + overhang / tm;
    const float slope = outer / std::cos(pitch), length = (bo1 - bo0) * tm, t = thick / std::cos(pitch);
    auto quad = [&](Batch& batch, const glm::vec3 (&p)[4], glm::vec3 n, const glm::vec2 (&uv)[4], float shade) {
        for (int i : {0, 1, 2, 0, 2, 3}) batch.vertices.push_back({p[i].x, p[i].y, p[i].z, n.x, n.y, n.z, uv[i].x, uv[i].y, shade});
    };
    for (const float side : {-1.f, 1.f}) {
        const glm::vec3 n = turn(glm::normalize(glm::vec3(side * std::sin(pitch), std::cos(pitch), 0)));
        const float eaveA = mid + side * outer / tm;
        for (const float offset : {0.f, -t})
            quad(roof, {place(mid, ridge + offset, bo0), place(mid, ridge + offset, bo1), place(eaveA, low + offset, bo1), place(eaveA, low + offset, bo0)},
                 offset == 0.f ? n : -n, {glm::vec2(0, 0), glm::vec2(length, 0), glm::vec2(length, slope), glm::vec2(0, slope)}, offset == 0.f ? 1.f : 0.55f);
        quad(roof, {place(eaveA, low, bo0), place(eaveA, low, bo1), place(eaveA, low - t, bo1), place(eaveA, low - t, bo0)},
             turn(glm::normalize(glm::vec3(side * std::cos(pitch), -std::sin(pitch), 0))),
             {glm::vec2(0, slope), glm::vec2(length, slope), glm::vec2(length, slope + 0.05f), glm::vec2(0, slope + 0.05f)}, 0.7f);
        for (const float b : {bo0, bo1})
            quad(roof, {place(mid, ridge, b), place(eaveA, low, b), place(eaveA, low - t, b), place(mid, ridge - t, b)},
                 turn(glm::vec3(0, 0, b == bo0 ? -1.f : 1.f)), {glm::vec2(0, 0), glm::vec2(0, slope), glm::vec2(0.3f, slope), glm::vec2(0.3f, 0)}, 0.8f);
    }
    if (ridgeCap) {
        constexpr int segments = 10;
        const float r = 0.28f;
        for (int i = 0; i < segments; ++i) {
            const float c0 = glm::radians(-70.f + 140.f * i / segments), c1 = glm::radians(-70.f + 140.f * (i + 1) / segments);
            const glm::vec3 d0(std::sin(c0), std::cos(c0), 0), d1(std::sin(c1), std::cos(c1), 0);
            auto at = [&](glm::vec3 d, float b) { return place(mid + d.x * r / tm, ridge - 0.08f + d.y * r, b); };
            quad(roof, {at(d0, bo0), at(d0, bo1), at(d1, bo1), at(d1, bo0)}, turn(glm::normalize(d0 + d1)),
                 {glm::vec2(0, i * 0.08f), glm::vec2(length, i * 0.08f), glm::vec2(length, (i + 1) * 0.08f), glm::vec2(0, (i + 1) * 0.08f)}, 1.f);
        }
    } else {
        // A ridge tile of the same material.
        const float r = 0.10f;
        quad(roof, {place(mid - r / tm, ridge + 0.02f, bo0), place(mid - r / tm, ridge + 0.02f, bo1), place(mid + r / tm, ridge + 0.02f, bo1), place(mid + r / tm, ridge + 0.02f, bo0)},
             glm::vec3(0, 1, 0), {glm::vec2(0, 0), glm::vec2(length, 0), glm::vec2(length, 0.2f), glm::vec2(0, 0.2f)}, 0.8f);
    }
    // Gables close the triangle between the eaves and the ridge, at both ends of the ridge.
    const float inset = 0.19f / tm, half2 = U73dScale::WallThickness * 0.25f;
    for (const float b : {b0 + inset, b1 - inset})
        for (const float face : {-1.f, 1.f}) {
            const float bf = b + face * half2;
            const glm::vec3 p[3] = {place(a0, eave, bf), place(a1, eave, bf), place(mid, ridge - t, bf)};
            const glm::vec3 n = turn(glm::vec3(0, 0, face));
            for (const auto& q : p) gable.vertices.push_back({q.x, q.y, q.z, n.x, n.y, n.z, 0, 0, 1});
        }
    for (int y = int(z0); y <= int(z1); ++y)
        for (int x = int(x0); x <= int(x1); ++x) m_roofTiles.insert({x, y});
}

void Britannia3dView::buildThatchRoof() {
    const auto thatch = ThatchMaterial::loadOrCreate(assetDirectory);
    m_thatchAlbedo = makeTexture(thatch.size, thatch.size, thatch.albedo.data(), true);
    m_thatchNormal = makeTexture(thatch.size, thatch.size, thatch.normal.data(), true);
    loadRepeating(m_thatchAlbedo);
    loadRepeating(m_thatchNormal);
    auto& roof = m_batches.emplace_back();
    roof.planks = 3;
    roof.roof = true;
    roof.layer = 2;
    const size_t roofIndex = m_batches.size() - 1;
    auto& gable = m_batches.emplace_back();
    gable.planks = 1;
    gable.layer = 1;
    gable.roof = true;                   // part of the roof: hidden with it
    for (const auto& batch : m_batches) if (batch.planks == 1 && &batch != &gable) { gable.tint = batch.tint; break; }
    // The shed's outer wall faces: 45 degrees, 0.6 m overhang, 0.35 m thick straw, ridge roll.
    addGableRoof(m_batches[roofIndex], gable, 1056.125f, 2180.125f, 1083.875f, 2207.875f, U73dScale::StoreyHeight, 45.f, 0.6f, 0.35f, true);
}

void Britannia3dView::buildSlateRoofs(const std::vector<glm::ivec4>& tiles) {
    if (tiles.empty()) return;
    const auto slate = SlateMaterial::loadOrCreate(assetDirectory);
    m_slateAlbedo = makeTexture(slate.size, slate.size, slate.albedo.data(), true);
    m_slateNormal = makeTexture(slate.size, slate.size, slate.normal.data(), true);
    loadRepeating(m_slateAlbedo);
    loadRepeating(m_slateNormal);
    auto& roof = m_batches.emplace_back();
    roof.planks = 5;
    roof.roof = true;
    roof.layer = 2;
    const size_t roofIndex = m_batches.size() - 1;
    auto& gableBatch = m_batches.emplace_back();
    gableBatch.planks = 1;               // wooden gables between the roof halves
    gableBatch.layer = 1;
    gableBatch.roof = true;
    for (const auto& batch : m_batches) if (batch.planks == 1 && batch.tint != glm::vec3(1)) { gableBatch.tint = batch.tint; break; }
    const size_t gableIndex = m_batches.size() - 1;
    // Roof pieces (tiles x0 y0 x1 y1, exclusive) -> rectangles of connected houses.
    std::set<std::pair<int, int>> free;
    for (const auto& t : tiles)
        for (int y = t.y; y < t.w; ++y)
            for (int x = t.x; x < t.z; ++x) free.insert({x, y});
    while (!free.empty()) {
        // Greedy: the largest rectangle growing right and down from the first free tile.
        const auto start = *free.begin();
        int w = 0;
        while (free.count({start.first + w, start.second})) ++w;
        int h = 1;
        for (;; ++h) {
            bool full = true;
            for (int x = 0; x < w && full; ++x) full = free.count({start.first + x, start.second + h}) > 0;
            if (!full) break;
        }
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) free.erase({start.first + x, start.second + y});
        if (w < 2 || h < 2) continue;
        // The eaves lie on the highest wall under the roof, so no wall reaches through it.
        float eave = 5 * U73dScale::StructureLift;
        for (int y = start.second - 1; y <= start.second + h; ++y)
            for (int x = start.first - 1; x <= start.first + w; ++x)
                if (const auto top = m_wallTop.find({x, y}); top != m_wallTop.end()) eave = std::max(eave, top->second);
        const float x0 = float(start.first) + 0.125f, z0 = float(start.second) + 0.125f;
        const float x1 = float(start.first + w) - 0.125f, z1 = float(start.second + h) - 0.125f;
        addGableRoof(m_batches[roofIndex], m_batches[gableIndex], x0, z0, x1, z1, eave, 38.f, 0.35f, 0.12f, false);
    }
}

void Britannia3dView::buildStraw() {
    std::ifstream in(dataDirectory / "Maps/stable-straw.txt");
    std::string tag, line;
    size_t count = 0;
    if (!(in >> tag >> count) || tag != "stable-straw-v1" || count > 4096) return;
    std::getline(in, line);
    std::map<std::string, size_t> batchOf;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream row(line);
        int x, y;
        std::string id;
        if (!(row >> x >> y >> id)) continue;
        auto [it, added] = batchOf.try_emplace(id, m_batches.size());
        if (added) {
            // Hard U7 pixels, upscaled like the objects; transparent between the straws.
            SDL_Surface* image = IMG_Load((stableSceneDirectory / "Materials/Straw" / (id + ".png")).string().c_str());
            if (!image) { batchOf.erase(it); continue; }
            SDL_Surface* rgba = SDL_ConvertSurface(image, SDL_PIXELFORMAT_RGBA32);
            SDL_DestroySurface(image);
            if (!rgba) { batchOf.erase(it); continue; }
            std::vector<std::uint8_t> pixels(size_t(rgba->w) * rgba->h * 4);
            for (int r = 0; r < rgba->h; ++r)
                std::copy_n(static_cast<const std::uint8_t*>(rgba->pixels) + size_t(r) * rgba->pitch, size_t(rgba->w) * 4, pixels.begin() + size_t(r) * rgba->w * 4);
            auto scaled = PixelArtScale::scale(pixels.data(), rgba->w, rgba->h, 4);
            PixelArtScale::smoothEdges(scaled, rgba->w * 4, rgba->h * 4, 1);
            auto& batch = m_batches.emplace_back();
            batch.texture = makeTexture(rgba->w * 4, rgba->h * 4, scaled.data(), true);
            batch.layer = 3;
            SDL_DestroySurface(rgba);
        }
        auto& batch = m_batches[it->second];
        const auto a = StableScene::u7Tile(float(x), float(y)), b = StableScene::u7Tile(float(x + 1), float(y + 1));
        const float h = 0.012f;
        const glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        addQuad(batch, {glm::vec3(a.x, h, a.y), glm::vec3(b.x, h, a.y), glm::vec3(b.x, h, b.y), glm::vec3(a.x, h, b.y)}, uv, {0, 1, 0});
    }
}

unsigned Britannia3dView::railTexture() {
    if (m_railTexture) return m_railTexture;
    // The rail of U7's stall dividers: a black outline over four greys of weathered wood.
    constexpr int w = 64, h = 64;
    const glm::vec3 band[4] = {{.427f, .298f, .235f}, {.333f, .235f, .188f}, {.235f, .173f, .141f}, {.157f, .110f, .078f}};
    std::vector<std::uint8_t> rail(size_t(w) * h * 4);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const float t = (y + 0.5f) / h;
            glm::vec3 c = band[std::clamp(int(t * 4), 0, 3)];
            c *= 0.92f + 0.08f * float((x * 7919 + (x / 3) * 104729) % 2);
            if (t < 0.08f || t > 0.92f) c = {.063f, .047f, .031f};
            auto* p = rail.data() + (size_t(y) * w + x) * 4;
            p[0] = std::uint8_t(c.r * 255); p[1] = std::uint8_t(c.g * 255); p[2] = std::uint8_t(c.b * 255); p[3] = 255;
        }
    m_railTexture = makeTexture(w, h, rail.data(), true);
    return m_railTexture;
}

void Britannia3dView::buildTownFences(const U7::Data& data, const std::vector<U7::WorldObject>& fences,
                                      const std::vector<std::pair<glm::vec2, glm::vec2>>& runs) {
    if (fences.empty() && runs.empty()) return;
    constexpr float tm = U73dScale::TileMetres;
    auto& batch = m_batches.emplace_back();
    batch.texture = railTexture();
    batch.layer = 1;
    std::set<std::pair<int, int>> occupied;
    for (const auto& f : fences) {
        const auto size = data.shapeSize(f.shape);
        for (int y = f.y - size.y + 1; y <= f.y; ++y)
            for (int x = f.x - size.x + 1; x <= f.x; ++x) occupied.insert({x, y});
    }
    std::set<std::pair<int, int>> posts;                      // quarter tiles
    auto box = [&](glm::vec2 a, glm::vec2 b, float bottom, float height, float thick) {
        // A box from a to b (tiles) thick metres across, from bottom to bottom + height metres.
        const glm::vec2 d = glm::normalize(b - a), n(-d.y, d.x);
        const glm::vec2 off = n * (thick * 0.5f / tm);
        const glm::vec2 c[4] = {a - off, b - off, b + off, a + off};
        glm::vec3 p[8];
        for (int i = 0; i < 4; ++i) { p[i] = glm::vec3(c[i].x, bottom, c[i].y); p[i + 4] = glm::vec3(c[i].x, bottom + height, c[i].y); }
        const int faces[6][4] = {{4, 5, 6, 7}, {3, 2, 1, 0}, {0, 1, 5, 4}, {2, 3, 7, 6}, {1, 2, 6, 5}, {3, 0, 4, 7}};
        const glm::vec3 normals[6] = {{0, 1, 0}, {0, -1, 0}, {-n.x, 0, -n.y}, {n.x, 0, n.y}, {d.x, 0, d.y}, {-d.x, 0, -d.y}};
        const glm::vec2 uv[4] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
        for (int k = 0; k < 6; ++k) {
            const glm::vec3 q[4] = {p[faces[k][0]], p[faces[k][1]], p[faces[k][2]], p[faces[k][3]]};
            addQuad(batch, q, uv, normals[k]);
            const glm::vec3 centre = (q[0] + q[2]) * 0.5f;
            auto& cell = m_walls[{int(std::floor(centre.x)) / 8, int(std::floor(centre.z)) / 8}];
            for (int j : {0, 1, 2, 0, 2, 3}) cell.push_back(q[j]);
        }
    };
    auto post = [&](glm::vec2 at, glm::vec2 dir) {
        if (!posts.insert({int(std::lround(at.x * 4)), int(std::lround(at.y * 4))}).second) return;
        const glm::vec2 h = dir * (0.08f / tm);
        box(at - h, at + h, 0, 1.14f, 0.16f);
    };
    for (const auto& f : fences) {
        const auto size = data.shapeSize(f.shape);
        bool alongX = size.x > size.y;
        if (size.x == size.y) alongX = occupied.count({f.x - 1, f.y}) || occupied.count({f.x + 1, f.y});
        const glm::vec2 a = alongX ? glm::vec2(f.x - size.x + 1, f.y + 0.5f) : glm::vec2(f.x + 0.5f, f.y - size.y + 1);
        const glm::vec2 b = alongX ? glm::vec2(f.x + 1, f.y + 0.5f) : glm::vec2(f.x + 0.5f, f.y + 1);
        box(a, b, 0.42f, 0.16f, 0.14f);
        box(a, b, 0.90f, 0.16f, 0.14f);
        const glm::vec2 dir = alongX ? glm::vec2(1, 0) : glm::vec2(0, 1);
        post(a, dir);
        post(b, dir);
    }
    for (const auto& [a, b] : runs) {
        box(a, b, 0.42f, 0.16f, 0.14f);
        box(a, b, 0.90f, 0.16f, 0.14f);
        const glm::vec2 dir = glm::normalize(b - a);
        post(a, dir);
        post(b, dir);
    }
}

void Britannia3dView::buildFences() {
    std::ifstream in(dataDirectory / "Maps/stable-fences.txt");
    std::string tag;
    size_t count = 0;
    if (!(in >> tag >> count) || tag != "stable-fences-v1" || count > 512) return;
    std::set<std::tuple<int, int, int>> edges;
    std::map<std::tuple<int, int, int>, glm::vec2> shifts;      // edge -> shift in U7 tiles
    std::string line;
    std::getline(in, line);
    m_fenceRuns.clear();
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream row(line);
        if (line.rfind("run ", 0) == 0) {
            std::string word;
            glm::vec2 a, b;
            if (row >> word >> a.x >> a.y >> b.x >> b.y) m_fenceRuns.push_back({a, b});
            continue;
        }
        int x, y, side;
        if (!(row >> x >> y >> side)) continue;
        glm::vec2 shift(0);
        if (row >> shift.x >> shift.y && edges.count({x, y, side})) {
            // A second, shifted copy of an edge (extends a run): stored under a free key.
            int key = 1;
            while (edges.count({x, y, side + 4 * key})) ++key;
            edges.insert({x, y, side + 4 * key});
            shifts[{x, y, side + 4 * key}] = shift;
            continue;
        }
        edges.insert({x, y, side});
        shifts[{x, y, side}] = shift;
    }
    auto& batch = m_batches.emplace_back();
    batch.texture = railTexture();
    batch.layer = 1;
    auto& walls = m_walls;
    for (const auto& [x, y, keyedSide] : edges) {
        const int side = keyedSide % 4;
        const glm::vec2 shift = shifts[{x, y, keyedSide}];
        // As KnownGeometry::buildEditedEdge (kind 5): the edge runs along u (0 .. 1 cell), depth
        // across it; heights are metres. Ultima7Remake map units become U7 tiles.
        const bool vertical = side == 1 || side == 3;
        const float bx = float(x) + (side == 1 ? 1.f : 0.f), by = float(y) + (side == 2 ? 1.f : 0.f);
        auto point = [&](float u, float depth, float height) {
            const auto tile = StableScene::u7Tile(bx + (vertical ? depth : u), by + (vertical ? u : depth));
            return glm::vec3(tile.x + shift.x, height, tile.y + shift.y);
        };
        auto box = [&](float u, float width, float bottom, float height, float thickness) {
            glm::vec3 p[8];
            for (int i = 0; i < 8; ++i)
                p[i] = point(u + (i & 1 ? width : 0), i & 2 ? thickness * .5f : -thickness * .5f, bottom + (i & 4 ? height : 0));
            const int faces[][4] = {{0, 1, 5, 4}, {3, 2, 6, 7}, {2, 0, 4, 6}, {1, 3, 7, 5}, {4, 5, 7, 6}, {2, 3, 1, 0}};
            const glm::vec2 uv[4] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
            for (const auto& f : faces) {
                const glm::vec3 q[4] = {p[f[0]], p[f[1]], p[f[2]], p[f[3]]};
                // Normal in metres (x / z are tiles), pointing away from the box centre.
                auto metres = [](glm::vec3 v) { return glm::vec3(v.x * U73dScale::TileMetres, v.y, v.z * U73dScale::TileMetres); };
                glm::vec3 n = glm::normalize(glm::cross(metres(q[1] - q[0]), metres(q[2] - q[0])));
                glm::vec3 centre(0), faceCentre = (q[0] + q[1] + q[2] + q[3]) * 0.25f;
                for (const auto& c : p) centre += c / 8.f;
                if (glm::dot(n, metres(faceCentre - centre)) < 0) n = -n;
                addQuad(batch, q, uv, n);
                auto& cell = walls[{int(std::floor(faceCentre.x)) / 8, int(std::floor(faceCentre.z)) / 8}];
                for (int k : {0, 1, 2, 0, 2, 3}) cell.push_back(q[k]);
            }
        };
        // Two rails at knee and hip height; posts every 2 m and at both free ends of a run.
        box(0, 1, .42f, .16f, .14f);
        box(0, 1, .90f, .16f, .14f);
        const int along = vertical ? y : x;
        auto fence = [&](int d) { return edges.count({x + (vertical ? 0 : d), y + (vertical ? d : 0), keyedSide}) > 0; };
        if ((along & 1) == 0 || !fence(-1)) box(-.08f, .16f, 0, 1.14f, .16f);
        if (!fence(1)) box(.92f, .16f, 0, 1.14f, .16f);
    }
}

void Britannia3dView::ensureDoorMeshes() {
    if (m_doorPlanks.vertexCount()) return;
    constexpr float length = 4.f;                     // tiles, as U7's door objects
    const auto parts = DoorModel::build(length - 0.02f, U73dScale::DoorHeight, U73dScale::TileMetres);
    auto upload = [](ow3d::Mesh& mesh, const std::vector<DoorModel::Vertex>& vertices) {
        std::vector<float> data;
        for (const auto& v : vertices)
            data.insert(data.end(), {v.position.x - 0.01f, v.position.y, v.position.z, v.normal.x, v.normal.y, v.normal.z, 0.f, 0.f, 1.f});
        mesh.create(data.data(), unsigned(data.size() / 9), 9);
    };
    upload(m_doorPlanks, parts.planks);
    upload(m_doorBattens, parts.battens);
    upload(m_doorIron, parts.iron);
    for (const auto& p : parts.leaf) m_doorLeaf.push_back(p - glm::vec3(0.01f, 0, 0));
    const std::uint8_t iron[4] = {58, 56, 54, 255};
    m_ironTexture = makeTexture(1, 1, iron, false);
}

bool Britannia3dView::indoors(glm::vec3 point, glm::vec3 direction) const {
    const glm::vec3 p = point + direction * 1.5f;
    return m_roofTiles.count({int(std::floor(p.x)), int(std::floor(p.z))}) > 0;
}

glm::mat4 Britannia3dView::gateModel(const Gate& gate) const {
    glm::mat4 m = glm::translate(glm::mat4(1), gate.origin + glm::vec3(0, gate.raised, 0));
    if (!gate.alongX) m = glm::rotate(m, glm::radians(-90.f), glm::vec3(0, 1, 0));
    return m;
}

bool Britannia3dView::operateGate(size_t index, bool immediately) {
    if (index >= m_gates.size() || m_gates[index].locked) return false;
    auto& gate = m_gates[index];
    gate.open = !gate.open;
    if (immediately) gate.raised = gate.open ? gate.height * 0.8f : 0.f;
    return true;
}

int Britannia3dView::pickWinch(glm::vec2 ndc) const {
    // The winch's box (1.6 x 1 x 1 m) on screen; the nearest hit wins.
    const auto viewProjection = m_camera.projectionMatrix() * m_camera.viewMatrix();
    constexpr float tm = U73dScale::TileMetres;
    int best = -1;
    float bestDepth = 1e30f;
    for (size_t i = 0; i < m_gates.size(); ++i) {
        glm::vec2 low(1e9f), high(-1e9f);
        float depth = 0;
        bool visible = true;
        for (int k = 0; k < 8; ++k) {
            const glm::vec3 corner = m_gates[i].winch + glm::vec3((k & 1 ? 0.8f : -0.8f) / tm, k & 2 ? 1.0f : 0.f, (k & 4 ? 0.8f : -0.8f) / tm);
            const auto clip = viewProjection * glm::vec4(planet(corner), 1);
            if (clip.w <= 0) { visible = false; break; }
            const glm::vec2 p = glm::vec2(clip) / clip.w;
            low = glm::min(low, p); high = glm::max(high, p);
            depth += clip.w / 8;
        }
        if (visible && ndc.x >= low.x && ndc.x <= high.x && ndc.y >= low.y && ndc.y <= high.y && depth < bestDepth) {
            bestDepth = depth;
            best = int(i);
        }
    }
    return best;
}

void Britannia3dView::drawGates(unsigned program) {
    if (m_gates.empty()) return;
    const GLint model = glGetUniformLocation(program, "flatModel"), planks = glGetUniformLocation(program, "planks"),
                tint = glGetUniformLocation(program, "tint");
    glm::vec3 wood(0.45f, 0.32f, 0.24f);
    for (const auto& batch : m_batches) if (batch.planks == 1 && batch.tint != glm::vec3(1)) { wood = batch.tint * 0.7f; break; }
    for (const auto& gate : m_gates) {
        const auto matrix = gateModel(gate);
        glUniformMatrix4fv(model, 1, GL_FALSE, &matrix[0][0]);
        glUniform1i(planks, 0);
        glBindTexture(GL_TEXTURE_2D, m_ironTexture);
        m_gateIron.bind();
        glDrawArrays(GL_TRIANGLES, 0, GLsizei(m_gateIron.vertexCount()));
        glUniform1i(planks, 1);
        glUniform3f(tint, wood.r, wood.g, wood.b);
        m_gateWood.bind();
        glDrawArrays(GL_TRIANGLES, 0, GLsizei(m_gateWood.vertexCount()));
    }
    const glm::mat4 identity(1);
    glUniformMatrix4fv(model, 1, GL_FALSE, &identity[0][0]);
}

bool Britannia3dView::characterUnderRoof() const {
    if (!character.loaded()) return false;
    const auto here = flat(character.position());
    return m_roofTiles.count({int(std::floor(here.x)), int(std::floor(here.z))}) > 0;
}

void Britannia3dView::buildGlobe(const U7::Data& data) {
    // The whole map, one texel per tile: the mean colour of its ground tile.
    constexpr int world = U7::WorldTiles, chunks = U7::WorldChunks, c = U7::ChunkTiles;
    std::map<int, std::array<std::uint8_t, 4>> means;
    std::vector<std::uint8_t> map(size_t(world) * world * 4);
    for (int y = 0; y < world; ++y)
        for (int x = 0; x < world; ++x) {
            const auto t = U7GroundGrid::groundTile(data, x, y);
            auto [mean, added] = means.try_emplace(t.shape << 5 | t.frame);
            if (added) {
                const auto pixels = U7GroundGrid::rgba(data, t.shape, t.frame);
                int sum[3] = {0, 0, 0};
                for (int i = 0; i < P * P; ++i)
                    for (int k = 0; k < 3; ++k) sum[k] += pixels[size_t(i) * 4 + k];
                mean->second = {std::uint8_t(sum[0] / (P * P)), std::uint8_t(sum[1] / (P * P)), std::uint8_t(sum[2] / (P * P)), 255};
            }
            std::copy_n(mean->second.begin(), 4, map.begin() + (size_t(y) * world + x) * 4);
        }
    auto& land = m_globe.emplace_back();
    land.texture = makeTexture(world, world, map.data(), true);
    // One quad per chunk, left out under the detail area and where the map would wrap too far.
    auto wrapped = [&](float x, float y) {
        return glm::length(glm::vec2(x, y) - m_centre) * U73dScale::TileMetres * Metre / Radius < MaxWrapAngle;
    };
    for (int cy = 0; cy < chunks; ++cy)
        for (int cx = 0; cx < chunks; ++cx) {
            if (cx >= m_chunkX && cx <= m_chunkX1 && cy >= m_chunkY && cy <= m_chunkY1) continue;
            const float x0 = float(cx * c), y0 = float(cy * c), x1 = x0 + c, y1 = y0 + c;
            if (!wrapped(x0, y0) || !wrapped(x1, y0) || !wrapped(x1, y1) || !wrapped(x0, y1)) continue;
            addQuad(land, {glm::vec3(x0, -GlobeDepth, y0), glm::vec3(x1, -GlobeDepth, y0), glm::vec3(x1, -GlobeDepth, y1), glm::vec3(x0, -GlobeDepth, y1)},
                    {glm::vec2(x0, y0) / float(world), glm::vec2(x1, y0) / float(world), glm::vec2(x1, y1) / float(world), glm::vec2(x0, y1) / float(world)},
                    {0, 1, 0});
        }
    // The ocean: the rest of the sphere, a little lower still (planet units, drawn "direct").
    const std::uint8_t sea[4] = {map[0], map[1], map[2], 255};
    auto& ocean = m_globe.emplace_back();
    ocean.texture = makeTexture(1, 1, sea, false);
    ocean.layer = -1;
    constexpr int rings = 90, segments = 180;
    const float r = Radius - 2.f * Metre;
    auto point = [&](int ring, int segment) {
        const float theta = 3.14159265f * ring / rings, phi = 6.2831853f * segment / segments;
        return glm::vec3(std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi), std::cos(theta));
    };
    for (int ring = 0; ring < rings; ++ring)
        for (int segment = 0; segment < segments; ++segment) {
            const glm::vec3 n[4] = {point(ring, segment), point(ring, segment + 1), point(ring + 1, segment + 1), point(ring + 1, segment)};
            for (int i : {0, 1, 2, 0, 2, 3})
                ocean.vertices.push_back({n[i].x * r, n[i].y * r, n[i].z * r, 0, 1, 0, 0.5f, 0.5f, 1.f});
        }
}

void Britannia3dView::build(const U7::Data& data, int chunkX0, int chunkY0, int chunkX1, int chunkY1, const LayerOf& layerOf,
                            const char* title) {
    destroy();
    m_chunkX = chunkX0; m_chunkY = chunkY0; m_chunkX1 = chunkX1; m_chunkY1 = chunkY1;
    m_title = title;
    m_boxes = m_quadObjects = m_createdFiles = 0;
    constexpr int c = U7::ChunkTiles;
    m_centre = glm::vec2((chunkX0 + chunkX1 + 1) * c / 2.f, (chunkY0 + chunkY1 + 1) * c / 2.f);
    if (!m_shader.create(VertexSource, FragmentSource)) throw std::runtime_error("Britannia3d shader failed");

    buildGround(data);

    // Every object whose footprint touches the detail chunks, except markers the game never shows.
    const int x0 = chunkX0 * c, y0 = chunkY0 * c, x1 = (chunkX1 + 1) * c, y1 = (chunkY1 + 1) * c;
    // Ultima7Remake's stable (x 655-669, y 1217-1231, and the paddock north of it up to y 1212)
    // replaces U7's furnishings there: everything but layer 1, roofs and upper floors.
    const bool stableScene = !stableSceneDirectory.empty() && std::filesystem::exists(dataDirectory / "Maps/static-scene-props.txt");
    if (stableScene) {
        // The stable itself (U7 tiles 1054 - 1084, 2176 - 2208), not the house north of it.
        m_stableArea = glm::ivec4(1054, 2176, 1086, 2209);
    }
    std::vector<glm::vec2> treePlaces, lampPlaces, wellPlaces;
    std::vector<U7::WorldObject> signPosts, signBoards, townFences;
    std::vector<glm::ivec4> slateTiles;
    std::vector<U7::WorldObject> windows, houseDoors, pictures;
    std::map<int, std::string> names;
    struct Placed { const U7::WorldObject* object; int layer; bool roof; float liftMetres; bool wall, thinWall; };
    std::vector<Placed> placed;
    std::vector<std::pair<int, int>> graphics;
    std::set<std::pair<int, int>> seen;
    for (const auto& object : data.objects()) {
        const auto size = data.shapeSize(object.shape);
        if (object.x < x0 || object.x - size.x + 1 >= x1 || object.y < y0 || object.y - size.y + 1 >= y1) continue;
        auto [it, added] = names.try_emplace(object.shape);
        if (added) it->second = lower(data.name(object.shape));
        const auto& name = it->second;
        if (name == "egg" || name == "path" || name == "light source") continue;
        const int layer = layerOf ? layerOf(object, name) : 2;
        // Signposts: posts and arrow boards, matched in buildSignposts.
        if (object.shape == 713 && object.lift == 0) {
            signPosts.push_back(object);
            if (seen.insert({object.shape, object.frame}).second) graphics.push_back({object.shape, object.frame});
            continue;
        }
        if (object.shape == 379) {
            signBoards.push_back(object);
            if (seen.insert({object.shape, object.frame}).second) graphics.push_back({object.shape, object.frame});
            continue;
        }
        // U7's wells (470) become 3D wells; their windlass (740) is part of the model.
        if (object.shape == 470) {
            const auto size = data.shapeSize(object.shape);
            glm::vec2 at(object.x + 1 - size.x * 0.5f, object.y + 1 - size.y * 0.5f);
            // The well north of the road junction stands in the corner of its lawn, by the kerbs.
            if (object.x == 1005 && object.y == 2131) at.x += 3.4f;
            wellPlaces.push_back(at);
            continue;
        }
        if (object.shape == 740) continue;
        // U7's lamp posts become 3D street lamps.
        if (object.shape == 889) {
            lampPlaces.push_back({object.x + 0.5f, object.y + 0.5f});
            continue;
        }
        // U7's trees become 3D trees.
        if (object.shape == 453) {
            treePlaces.push_back({object.x + 0.5f, object.y + 0.5f});
            continue;
        }
        // The horse sign in front of the shed becomes the iron sign on the wall.
        if (stableScene && object.shape == 361 && object.frame == 7 && object.x == 1067 && object.y == 2211) continue;
        // U7's fences everywhere else become rail fences too.
        // (Not in the stable and its paddock: Ultima7Remake's fences stand there, see below.)
        const bool paddock = stableScene && ((object.x >= m_stableArea.x && object.x < m_stableArea.z && object.y >= m_stableArea.y && object.y < m_stableArea.w) ||
                                             (object.x >= 1070 && object.x < 1090 && object.y >= 2158 && object.y < 2182));
        if (name == "fence" && !paddock) { townFences.push_back(object); continue; }
        // Windows get a 3D frame with panes; paintings and tapestries hang as pictures.
        if (object.shape == 732 || object.shape == 438) { windows.push_back(object); continue; }
        if (name == "painting" || name == "tapestry") { pictures.push_back(object); continue; }
        if (name == "door" && !(stableScene && object.x >= m_stableArea.x && object.x < m_stableArea.z &&
                                object.y >= m_stableArea.y && object.y < m_stableArea.w)) {
            // Becomes a DoorModel door in buildOpenings (or the U7 door again, if it is no door
            // in a wall); its graphic is loaded for that case.
            houseDoors.push_back(object);
            if (seen.insert({object.shape, object.frame}).second) graphics.push_back({object.shape, object.frame});
            continue;
        }
        // U7's flat slate roofs (on the walls, lift 5) become slate gable roofs.
        if (name == "slate roof" && object.lift == 5) {
            const auto size = data.shapeSize(object.shape);
            slateTiles.push_back({object.x - size.x + 1, object.y - size.y + 1, object.x + 1, object.y + 1});
            continue;
        }
        // The thatched roof replaces U7's flat wood roof over the shed.
        if (stableScene && name == "wood roof" && object.x >= 1056 && object.x < 1087 && object.y >= 2180 && object.y < 2211)
            continue;
        // Ultima7Remake's fences and trough replace U7's in the stable and the paddock north east of it.
        if (stableScene && (name == "fence" || name == "trough" || name == "water trough") &&
            ((object.x >= m_stableArea.x && object.x < m_stableArea.z && object.y >= m_stableArea.y && object.y < m_stableArea.w) ||
             (object.x >= 1070 && object.x < 1090 && object.y >= 2158 && object.y < 2182)))
            continue;
        // The shed's doors become DoorModel doors (270: closed along x, 376: opened inwards).
        if (stableScene && name == "door" && (object.shape == 270 || object.shape == 376) &&
            object.x >= m_stableArea.x && object.x < m_stableArea.z && object.y >= m_stableArea.y && object.y < m_stableArea.w) {
            Door door;
            // Hinge at the east end of the opening: U7's door object ends at its tile's east edge.
            door.pivot = glm::vec3(object.x + 1.f, 0.f, object.y + 0.5f);
            door.openAngle = glm::radians(-90.f);
            door.open = object.shape == 376;
            door.angle = door.open ? door.openAngle : 0.f;
            m_doors.push_back(door);
            continue;
        }
        // U7's own pitchfork, shovel and rake stay (their models are preferred to the imported ones),
        // as do the straw on the floor ("garbage" straw specks) and the blood.
        const bool keptTool = name == "pitchfork" || name == "shovel" || name == "rake" || name == "garbage" || name == "blood";
        if (stableScene && !keptTool && layer != 1 && name.find("roof") == std::string::npos && object.lift < U73dScale::UpperFloorLift &&
            object.x >= m_stableArea.x && object.x < m_stableArea.z && object.y >= m_stableArea.y && object.y < m_stableArea.w)
            continue;
        const bool roof = name.find("roof") != std::string::npos;
        // Building scale: walls, doors, windows (layer 1 without the fences), roofs, stairs and
        // everything on upper floors; furniture scale for the rest.
        const bool structure = (layer == 1 && name.find("fence") == std::string::npos) || roof ||
                               name.find("stairs") != std::string::npos || object.lift >= U73dScale::UpperFloorLift;
        // The wooden stairs beside the shed (east of it) are half as steep.
        const bool shedStairs = name == "stairs" && object.x >= 1084 && object.x < 1089 && object.y >= 2176 && object.y < 2185;
        placed.push_back({&object, layer == 1 || layer == 3 ? layer : 2, roof,
                          (structure ? U73dScale::StructureLift : U73dScale::FurnitureLift) * (shedStairs ? 0.5f : 1.f), layer == 1 && name == "wall",
                          layer == 1 && name.find("fence") == std::string::npos});
        if (seen.insert({object.shape, object.frame}).second) graphics.push_back({object.shape, object.frame});
    }
    // A bloodier stable: pools around the corpse and the gargoyle, and a trail of drops from the
    // corpse along U7's trail to the north east, out onto the landing of the wooden stairs.
    m_extraObjects.clear();
    if (stableScene) {
        auto random = [](int i, int salt) { std::uint32_t h = std::uint32_t(i) * 2654435761u ^ std::uint32_t(salt) * 40503u; h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; return float(h & 0xffff) / 65535.f; };
        auto add = [&](float x, float y, int frame, int lift) {
            U7::WorldObject blood;
            blood.x = int(std::floor(x)); blood.y = int(std::floor(y)); blood.lift = lift;
            blood.shape = 912; blood.frame = std::uint8_t(frame); blood.source = U7::Source::Movable;
            m_extraObjects.push_back(blood);
        };
        const glm::vec2 corpse = StableScene::u7Tile(662.85f, 1226.65f), gargoyle = StableScene::u7Tile(661.90f, 1218.10f);
        for (int i = 0; i < 22; ++i) {
            const float a = random(i, 1) * 6.2831853f, r = 1.f + 3.5f * random(i, 2);
            add(corpse.x + std::cos(a) * r, corpse.y + std::sin(a) * r, i % 3 == 0 ? 3 : i % 3 == 1 ? 2 : 1, 0);
        }
        for (int i = 0; i < 10; ++i) {
            const float a = random(i, 3) * 3.1415926f, r = 0.8f + 2.2f * random(i, 4);   // south of it, in the chamber
            add(gargoyle.x + std::cos(a) * r, gargoyle.y + 0.6f + std::sin(a) * r, i % 2 ? 2 : 0, 0);
        }
        // The trail: corpse -> U7's drops (1071,2194 ... 1084,2177) -> the stairs at x 1084 - 1086.
        const glm::vec2 path[] = {corpse, {1071.5f, 2194.5f}, {1076.5f, 2184.5f}, {1084.5f, 2177.5f}, {1085.5f, 2179.5f}, {1085.5f, 2180.5f}};
        int drop = 0;
        for (size_t k = 0; k + 1 < std::size(path); ++k) {
            const float length = glm::length(path[k + 1] - path[k]);
            for (float t = 0; t < length; t += 0.9f, ++drop) {
                const auto p = path[k] + (path[k + 1] - path[k]) * (t / length) + glm::vec2(random(drop, 5) - 0.5f, random(drop, 6) - 0.5f) * 0.6f;
                // On the stairs a drop lies on the step it falls on.
                int lift = 0;
                for (const auto& other : data.objects()) {
                    if (other.shape != 426 && other.shape != 427) continue;
                    const auto size = data.shapeSize(other.shape);
                    if (int(p.x) > other.x || int(p.x) <= other.x - size.x || int(p.y) > other.y || int(p.y) <= other.y - size.y) continue;
                    lift = std::max(lift, other.lift + size.z);
                }
                add(p.x, p.y, 24 + drop % 4, lift);
            }
        }
        for (const auto& blood : m_extraObjects) {
            // Drops on the shed stairs take their (flattened) height.
            const bool onStairs = blood.lift > 0;
            placed.push_back({&blood, 2, false, onStairs ? U73dScale::StructureLift * 0.5f : U73dScale::FurnitureLift, false, false});
            if (seen.insert({blood.shape, blood.frame}).second) graphics.push_back({blood.shape, blood.frame});
        }
    }
    loadModels(data, graphics);
    // All plank walls share one wood tone (the mean of their graphics), so the pieces do not
    // form stripes; the narrow 1 x 1 pieces are posts, as in U7, with upright boards.
    glm::vec3 woodTone(0);
    int woodGraphics = 0;
    for (const auto& [key, model] : m_models)
        if (model.planks) { woodTone += model.tint; ++woodGraphics; }
    if (woodGraphics) woodTone /= float(woodGraphics);
    const auto wood = WoodMaterial::loadOrCreate(assetDirectory);
    // A coloured material image (a replacement) keeps its own colours: neutral tint (x 2 = 1).
    double saturation = 0;
    for (size_t i = 0; i + 3 < wood.albedo.size(); i += 4 * 97) {
        const int high = std::max({wood.albedo[i], wood.albedo[i + 1], wood.albedo[i + 2]});
        const int low = std::min({wood.albedo[i], wood.albedo[i + 1], wood.albedo[i + 2]});
        saturation += high ? double(high - low) / high : 0;
    }
    if (saturation / std::max<size_t>(1, wood.albedo.size() / (4 * 97)) > 0.15) woodTone = glm::vec3(0.5f);
    for (auto& [key, model] : m_models)
        if (model.planks) {
            const auto size = data.shapeSize(key.first);
            model.tint = woodTone;
            model.post = size.x == 1 && size.y == 1;
        }
    {
        const auto stone = StoneMaterial::loadOrCreate(assetDirectory, data);   // tiles every StoneMaterial::Size metres
        m_stoneAlbedo = makeTexture(stone.width, stone.height, stone.albedo.data(), true);
        m_stoneNormal = makeTexture(stone.width, stone.height, stone.normal.data(), true);
        for (unsigned id : {m_stoneAlbedo, m_stoneNormal}) {
            glBindTexture(GL_TEXTURE_2D, id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        for (auto& [key, model] : m_models) if (model.stone) model.tint = glm::vec3(0.5f);
        // Town walls: fortress blocks and crenellations get the dressed stone.
        for (auto& [key, model] : m_models) {
            const auto name = lower(data.name(key.first));
            model.fortress = (name == "fortress" || name == "crenellations") && model.kind == U7ObjectModel::Kind::Box;
        }
        const auto ashlar = AshlarMaterial::loadOrCreate(assetDirectory, data);
        m_ashlarAlbedo = makeTexture(ashlar.width, ashlar.height, ashlar.albedo.data(), true);
        m_ashlarNormal = makeTexture(ashlar.width, ashlar.height, ashlar.normal.data(), true);
        for (unsigned id : {m_ashlarAlbedo, m_ashlarNormal}) {
            glBindTexture(GL_TEXTURE_2D, id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
    }
    // Wooden plank walls get the high resolution plank material (stone and plaster keep U7's).
    m_woodAlbedo = makeTexture(wood.size, wood.size, wood.albedo.data(), true);
    m_woodNormal = makeTexture(wood.size, wood.size, wood.normal.data(), true);
    for (unsigned id : {m_woodAlbedo, m_woodNormal}) {
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    // Above each stable door the wall goes on in plank material up to its full height.
    if (!m_doors.empty()) {
        constexpr float length = 4.f;
        auto& lintel = m_batches.emplace_back();
        lintel.planks = 1;
        lintel.layer = 1;
        lintel.tint = woodTone;
        const float half = U73dScale::WallThickness / 2, top = U73dScale::StoreyHeight, low = U73dScale::DoorHeight;
        for (const auto& door : m_doors) {
            const float x0 = door.pivot.x - length, x1 = door.pivot.x, z0 = door.pivot.z - half, z1 = door.pivot.z + half;
            const glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
            addQuad(lintel, {glm::vec3(x0, top, z1), glm::vec3(x1, top, z1), glm::vec3(x1, low, z1), glm::vec3(x0, low, z1)}, uv, {0, 0, 1});
            addQuad(lintel, {glm::vec3(x1, top, z0), glm::vec3(x0, top, z0), glm::vec3(x0, low, z0), glm::vec3(x1, low, z0)}, uv, {0, 0, -1});
            addQuad(lintel, {glm::vec3(x0, low, z1), glm::vec3(x1, low, z1), glm::vec3(x1, low, z0), glm::vec3(x0, low, z0)}, uv, {0, -1, 0});
            addQuad(lintel, {glm::vec3(x0, top, z0), glm::vec3(x1, top, z0), glm::vec3(x1, top, z1), glm::vec3(x0, top, z1)}, uv, {0, 1, 0});
            // Collision: the lintel is overhead, the closed door is added per frame.
        }
    }
    m_wallFootprint.clear();
    auto footprint = [&](const U7::WorldObject& o) {
        const auto size = data.shapeSize(o.shape);
        for (int y = o.y - size.y + 1; y <= o.y; ++y)
            for (int x = o.x - size.x + 1; x <= o.x; ++x) m_wallFootprint.insert({x, y});
    };
    for (const auto& p : placed) if (p.wall || (p.layer == 1 && p.thinWall)) footprint(*p.object);
    for (const auto& o : houseDoors) footprint(o);
    for (const auto& o : windows) footprint(o);
    std::vector<std::pair<U7::WorldObject, float>> gateParts;          // object, metres per lift
    for (const auto& p : placed) {
        const auto name = lower(data.name(p.object->shape));
        if (name == "fortress" || name == "crenellations" || name == "fortress gateway") {
            const auto size = data.shapeSize(p.object->shape);
            for (int y = p.object->y - size.y + 1; y <= p.object->y; ++y)
                for (int x = p.object->x - size.x + 1; x <= p.object->x; ++x) m_fortressTiles.insert({x, y});
        }
        if (name == "portcullis" || name == "winch" || name == "stairs") {
            gateParts.push_back({*p.object, p.liftMetres});
            continue;
        }
    }
    for (const auto& p : placed) {
        const auto name = lower(data.name(p.object->shape));
        if (name == "portcullis" || name == "winch" || name == "stairs") continue;
        const auto model = m_models.find({p.object->shape, p.object->frame});
        const bool plank = p.wall && model != m_models.end() && model->second.planks && model->second.kind == U7ObjectModel::Kind::Box;
        const bool stoneWall = p.wall && model != m_models.end() && model->second.stone && model->second.kind == U7ObjectModel::Kind::Box;
        const bool fortress = model != m_models.end() && model->second.fortress;
        const int planks = plank ? (model->second.post ? 2 : 1) : stoneWall ? 4 : fortress ? 6 : 0;
        // Walkways: the tops of the town wall, its gateways and raised floors carry Sir Canegm.
        if (fortress || name == "fortress gateway" || (name == "floor" && p.object->lift > 0)) {
            const auto size = data.shapeSize(p.object->shape);
            const float top = (p.object->lift + std::max(size.z, name == "floor" ? 0 : 1)) * p.liftMetres;
            const float fx0 = float(p.object->x - size.x + 1), fz0 = float(p.object->y - size.y + 1), fx1 = float(p.object->x + 1), fz1 = float(p.object->y + 1);
            const glm::vec3 a(fx0, top, fz0), b(fx1, top, fz0), c(fx1, top, fz1), d(fx0, top, fz1);
            m_stepTops.insert(m_stepTops.end(), {a, b, c, a, c, d});
        }
        addObject(data, *p.object, p.layer, p.roof, p.liftMetres, planks, p.thinWall);
    }
    buildGateModels(data, gateParts);
    buildSlateRoofs(slateTiles);
    buildGlobe(data);
    if (stableScene) {
        const StableScene::Ground ground = [this](float x, float y) {
            const auto tile = StableScene::u7Tile(x, y);
            return planet({tile.x, 0, tile.y});
        };
        try {
            m_stableProps = std::make_unique<StableSceneProps>();
            m_stableProps->skipFiles = {"Pitchfork"};          // U7's own pitchfork graphic instead
            m_stableProps->load(stableSceneDirectory, dataDirectory, ground);
            m_stableTools = std::make_unique<StableTools>();
            m_stableTools->skip[0] = m_stableTools->skip[1] = m_stableTools->skip[2] = true;   // U7's own tools instead
            m_stableTools->load(stableSceneDirectory, dataDirectory, ground);
            buildFences();
            buildStraw();
            buildThatchRoof();
            buildSignAndFork(data);
        } catch (const std::exception& e) {
            SDL_Log("Britannia3d: stable scene not loaded: %s", e.what());
            m_stableProps.reset();
            m_stableTools.reset();
        }
    }
    buildLamps(lampPlaces);
    buildWells(wellPlaces);
    buildSignposts(data, signPosts, signBoards);
    buildTownFences(data, townFences, m_fenceRuns);
    buildOpenings(data, windows, houseDoors);
    if (!m_doors.empty()) ensureDoorMeshes();
    buildPictures(data, pictures);
    buildRoads();                      // after all walls and roofs: streets outside the houses
    buildTrees(treePlaces);            // after all walls and roofs: the crowns keep clear of them
    for (auto* batches : {&m_batches, &m_globe})
        for (auto& batch : *batches)
            batch.mesh.create(reinterpret_cast<const float*>(batch.vertices.data()), unsigned(batch.vertices.size()), 9);
    m_target = glm::vec3(m_centre.x, 0, m_centre.y);
    m_distance = std::max(26.f, std::max(chunkX1 - chunkX0 + 1, chunkY1 - chunkY0 + 1) * c * 1.3f * U73dScale::TileMetres);
}

void Britannia3dView::prepareCollision(ow3d::BuildingCollision& collision, glm::vec3 position, float height) const {
    const auto here = flat(position);
    const int cellX = int(std::floor(here.x / 8)), cellY = int(std::floor(here.z / 8));
    std::vector<glm::vec3> triangles;
    // Hidden walls do not stop Sir Canegm either.
    if (layerVisible[1])
    for (int y = cellY - 2; y <= cellY + 2; ++y)
        for (int x = cellX - 2; x <= cellX + 2; ++x)
            if (auto it = m_walls.find({x, y}); it != m_walls.end())
                for (size_t k = 0; k + 2 < it->second.size(); k += 3) {
                    // A face whose top is at most a step (0.7 m) above Sir Canegm's feet is
                    // stepped onto (stairs, the wall walk), not a wall.
                    const auto& t = it->second;
                    if ((std::max)({t[k].y, t[k + 1].y, t[k + 2].y}) <= here.y + 0.7f) continue;
                    for (int j = 0; j < 3; ++j) triangles.push_back(planet(t[k + j]));
                }
    if (layerVisible[1])
        for (const auto& door : m_doors) {
            if (door.kind != 0) continue;
            if (glm::distance(glm::vec2(door.pivot.x, door.pivot.z), glm::vec2(here.x, here.z)) > 12.f) continue;
            const auto model = doorModel(door);
            for (const auto& p : m_doorLeaf) triangles.push_back(planet(glm::vec3(model * glm::vec4(p, 1))));
        }
    for (const auto& gate : m_gates) {
        if (gate.raised > 1.6f) continue;                     // drawn up far enough to pass under
        const glm::vec3 a = gate.origin, b = gate.origin + (gate.alongX ? glm::vec3(gate.length, 0, 0) : glm::vec3(0, 0, gate.length));
        if (glm::distance(glm::vec2(a.x, a.z), glm::vec2(here.x, here.z)) > 12.f) continue;
        const glm::vec3 low(0, gate.raised, 0), up(0, gate.raised + gate.height, 0);
        for (const auto& q : {a + low, b + low, b + up, a + low, b + up, a + up}) triangles.push_back(planet(q));
    }
    collision.setSurfaceGeometry(position, height, triangles);
    // The raised streets carry Sir Canegm.
    std::vector<glm::vec3> support;
    for (int y = int(std::floor(here.z)) - 3; y <= int(std::floor(here.z)) + 3; ++y)
        for (int x = int(std::floor(here.x)) - 3; x <= int(std::floor(here.x)) + 3; ++x)
            if (m_roadTiles.count({x, y})) {
                const glm::vec3 a = planet({float(x), RoadHeight, float(y)}), b = planet({float(x + 1), RoadHeight, float(y)});
                const glm::vec3 c = planet({float(x + 1), RoadHeight, float(y + 1)}), d = planet({float(x), RoadHeight, float(y + 1)});
                support.insert(support.end(), {a, b, c, a, c, d});
            }
    for (size_t i = 0; i + 2 < m_stepTops.size(); i += 3)
        if (glm::distance(glm::vec2(m_stepTops[i].x, m_stepTops[i].z), glm::vec2(here.x, here.z)) < 10.f)
            for (int k = 0; k < 3; ++k) support.push_back(planet(m_stepTops[i + k]));
    if (!support.empty()) collision.appendSupportGeometry(support);
}

bool Britannia3dView::loadCharacter(const std::filesystem::path& folder, float tileX, float tileY) {
    if (!std::filesystem::exists(folder) || !character.load(folder)) return false;
    character.outfit.load(folder);
    // Settings of Ultima7Remake (scene scale 0.6 there, so its distances are taken x 0.6).
    character.setHeight(U73dScale::CharacterHeight * Metre);
    character.minimumCameraDistance = .12f * .6f;
    character.preferOutwardCamera = true;
    m_camera.minimumFieldOfView = 12.f;
    character.setPosition(planet({tileX, 0, tileY}));
    character.prepareCollision = [this](ow3d::BuildingCollision& collision, glm::vec3 position, float height) {
        prepareCollision(collision, position, height);
    };
    character.topDown = true;
    character.follow = true;
    character.setTopDownAngle(55.95f);
    const auto north = glm::normalize(planet({tileX, 0, tileY - 1}) - character.position());
    character.restoreCameraOrbit(north, {glm::radians(55.95f), 0.f});
    character.setCameraDistance(.40f);
    step(0, false);
    return true;
}

void Britannia3dView::step(float dt, bool forward, float turn, float tilt) {
    if (!character.loaded()) return;
    if (character.follow) {
        character.update(dt, forward, turn, tilt);
        character.updateCamera(m_camera, dt);
    } else {
        character.update(dt, false, 0, 0);
    }
}

void Britannia3dView::setFreeCamera(float tileX, float tileY, float distance, float yaw, float pitch) {
    character.follow = false;
    m_target = glm::vec3(tileX, 0, tileY);
    m_distance = distance; m_yaw = yaw; m_pitch = pitch;
}

void Britannia3dView::updateFreeCamera(float aspect) {
    const float yaw = glm::radians(m_yaw), pitch = glm::radians(m_pitch);
    const glm::vec3 direction(std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch));
    const auto target = planet(m_target);
    // Distance in metres; flat x / y are tiles.
    const glm::vec3 offset(direction.x / U73dScale::TileMetres, direction.y, direction.z / U73dScale::TileMetres);
    m_camera.setPosition(planet(m_target + offset * m_distance));
    m_camera.lookAt(target, glm::normalize(target));
    (void)aspect;
}

void Britannia3dView::resizeTarget(int width, int height) {
    if (width == m_targetWidth && height == m_targetHeight && m_framebuffer) return;
    if (!m_framebuffer) {
        glGenFramebuffers(1, &m_framebuffer);
        glGenTextures(1, &m_color);
        glGenFramebuffers(1, &m_msFramebuffer);
        glGenRenderbuffers(1, &m_msColor);
        glGenRenderbuffers(1, &m_msDepth);
    }
    m_targetWidth = width; m_targetHeight = height;
    // Drawn multisampled, resolved into the texture that ImGui shows.
    glBindRenderbuffer(GL_RENDERBUFFER, m_msColor);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, Samples, GL_RGBA8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, m_msDepth);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, Samples, GL_DEPTH24_STENCIL8, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, m_msFramebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_msColor);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_msDepth);
    glBindTexture(GL_TEXTURE_2D, m_color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_color, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Britannia3dView::render(int width, int height) {
    resizeTarget(width, height);
    const float aspect = float(width) / float(height);
    if (!followCharacter()) updateFreeCamera(aspect);
    // Near and far planes follow the height above the ground: from Sir Canegm's face to the globe.
    const float altitude = std::max(glm::length(m_camera.position()) - Radius, 0.f);
    const float nearPlane = std::clamp(altitude * 0.05f, 0.0004f, 2.f);
    m_camera.setPerspective(m_camera.fieldOfView(), aspect, nearPlane, std::max(80.f, altitude * 4.f + Radius * 2.f));

    GLint previous = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous);
    glBindFramebuffer(GL_FRAMEBUFFER, m_msFramebuffer);
    glViewport(0, 0, width, height);
    // Sky blue near the ground, dark space high above the globe.
    const float space = glm::smoothstep(0.5f, 12.f, altitude);
    glClearColor(0.42f * (1 - space) + 0.02f * space, 0.6f * (1 - space) + 0.02f * space, 0.82f * (1 - space) + 0.06f * space, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glEnable(GL_SAMPLE_ALPHA_TO_ONE);
    m_shader.bind();
    m_shader.setMat4("view", m_camera.viewMatrix());
    m_shader.setMat4("projection", m_camera.projectionMatrix());
    m_shader.setInt("image", 0);
    m_shader.setFloat("metre", Metre);
    m_shader.setFloat("tileMetres", U73dScale::TileMetres);
    m_shader.setFloat("woodTile", U73dScale::TileMetres);
    m_shader.setFloat("radius", Radius);
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glUniform2f(glGetUniformLocation(GLuint(program), "centre"), m_centre.x, m_centre.y);
    const GLint direct = glGetUniformLocation(GLuint(program), "direct");
    const GLint wind = glGetUniformLocation(GLuint(program), "wind");
    glUniform1f(glGetUniformLocation(GLuint(program), "time"), float(SDL_GetTicks()) / 1000.f);
    const GLint tileArray = glGetUniformLocation(GLuint(program), "tileArray");
    {
        const glm::mat4 identity(1);
        glUniformMatrix4fv(glGetUniformLocation(GLuint(program), "flatModel"), 1, GL_FALSE, &identity[0][0]);
    }
    const GLint planks = glGetUniformLocation(GLuint(program), "planks"), tint = glGetUniformLocation(GLuint(program), "tint");
    glUniform1i(glGetUniformLocation(GLuint(program), "tiles"), 1);
    glUniform1i(glGetUniformLocation(GLuint(program), "woodAlbedo"), 2);
    glUniform1i(glGetUniformLocation(GLuint(program), "woodNormal"), 3);
    glUniform1f(glGetUniformLocation(GLuint(program), "woodSize"), WoodMaterial::Size);
    glUniform1i(glGetUniformLocation(GLuint(program), "stoneAlbedo"), 6);
    glUniform1i(glGetUniformLocation(GLuint(program), "stoneNormal"), 7);
    glUniform1f(glGetUniformLocation(GLuint(program), "stoneSize"), StoneMaterial::Size);
    glUniform1f(glGetUniformLocation(GLuint(program), "stoneHeight"), U73dScale::StoreyHeight);
    glUniform1i(glGetUniformLocation(GLuint(program), "slateAlbedo"), 8);
    glUniform1i(glGetUniformLocation(GLuint(program), "slateNormal"), 9);
    glUniform1f(glGetUniformLocation(GLuint(program), "slateSize"), SlateMaterial::Size);
    glUniform1i(glGetUniformLocation(GLuint(program), "grassAlbedo"), 14);
    glUniform1i(glGetUniformLocation(GLuint(program), "grassNormal"), 15);
    glUniform1f(glGetUniformLocation(GLuint(program), "grassSize"), GrassMaterial::Size);
    glActiveTexture(GL_TEXTURE14);
    glBindTexture(GL_TEXTURE_2D, m_grassAlbedo);
    glActiveTexture(GL_TEXTURE15);
    glBindTexture(GL_TEXTURE_2D, m_grassNormal);
    glUniform1i(glGetUniformLocation(GLuint(program), "cobbleAlbedo"), 12);
    glUniform1i(glGetUniformLocation(GLuint(program), "cobbleNormal"), 13);
    glUniform1f(glGetUniformLocation(GLuint(program), "cobbleSize"), CobbleMaterial::Size);
    glActiveTexture(GL_TEXTURE12);
    glBindTexture(GL_TEXTURE_2D, m_cobbleAlbedo);
    glActiveTexture(GL_TEXTURE13);
    glBindTexture(GL_TEXTURE_2D, m_cobbleNormal);
    glUniform1i(glGetUniformLocation(GLuint(program), "ashlarAlbedo"), 10);
    glUniform1i(glGetUniformLocation(GLuint(program), "ashlarNormal"), 11);
    glUniform1f(glGetUniformLocation(GLuint(program), "ashlarSize"), AshlarMaterial::Size);
    const unsigned units[6] = {m_stoneAlbedo, m_stoneNormal, m_slateAlbedo, m_slateNormal, m_ashlarAlbedo, m_ashlarNormal};
    for (int k = 0; k < 6; ++k) { glActiveTexture(GL_TEXTURE6 + k); glBindTexture(GL_TEXTURE_2D, units[k]); }
    glUniform1i(glGetUniformLocation(GLuint(program), "thatchAlbedo"), 4);
    glUniform1i(glGetUniformLocation(GLuint(program), "thatchNormal"), 5);
    glUniform1f(glGetUniformLocation(GLuint(program), "thatchSize"), ThatchMaterial::Size);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, m_thatchAlbedo);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, m_thatchNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_woodAlbedo);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, m_woodNormal);
    glActiveTexture(GL_TEXTURE0);
    const bool roofs = roofsVisible && !(followCharacter() && characterUnderRoof());
    for (const auto* batches : {&m_globe, &m_batches})
        for (const auto& batch : *batches) {
            if (batches == &m_batches && ((batch.roof && !roofs) || !layerVisible[size_t(batch.layer)])) continue;
            glUniform1i(direct, batch.layer < 0);
            glUniform1i(tileArray, batch.array);
            glUniform1i(wind, batch.wind);
            glUniform1i(planks, batch.planks);
            glUniform3f(tint, batch.tint.r, batch.tint.g, batch.tint.b);
            if (batch.array) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D_ARRAY, batch.texture);
                glActiveTexture(GL_TEXTURE0);
            } else glBindTexture(GL_TEXTURE_2D, batch.texture);
            batch.mesh.bind();
            glDrawArrays(GL_TRIANGLES, 0, GLsizei(batch.mesh.vertexCount()));
        }
    if (layerVisible[1]) {
        glUniform1i(direct, 0);
        glUniform1i(tileArray, 0);
        glUniform1i(wind, 0);
        drawDoors(GLuint(program));
        drawGates(GLuint(program));
    }
    glBindVertexArray(0);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glDisable(GL_SAMPLE_ALPHA_TO_ONE);
    if (stableSceneVisible && (m_stableProps || m_stableTools)) {
        const auto viewProjection = m_camera.projectionMatrix() * m_camera.viewMatrix();
        const float time = float(SDL_GetTicks()) / 1000.f;
        if (m_stableProps) m_stableProps->render(viewProjection, m_camera.position(), time);
        if (m_stableTools) m_stableTools->render(viewProjection, m_camera.position());
        glEnable(GL_DEPTH_TEST);
    }
    if (character.loaded()) character.draw(m_camera, height);
    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msFramebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, GLuint(previous));
}

std::vector<std::uint8_t> Britannia3dView::renderImage(int width, int height) {
    render(width, height);
    std::vector<std::uint8_t> pixels(size_t(width) * height * 4), flipped(pixels.size());
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    for (int y = 0; y < height; ++y)
        std::copy_n(pixels.begin() + size_t(height - 1 - y) * width * 4, size_t(width) * 4, flipped.begin() + size_t(y) * width * 4);
    return flipped;
}

void Britannia3dView::handleInput(bool hovered) {
    const auto& io = ImGui::GetIO();
    const float dt = std::clamp(io.DeltaTime, 0.f, .1f);
    SDL_Window* window = SDL_GL_GetCurrentWindow();
    const auto buttons = SDL_GetMouseState(nullptr, nullptr);
    float dx = 0, dy = 0;
    // Right mouse held on the view captures the mouse, as in Ultima7Remake.
    if (m_mouseCaptured) {
        SDL_GetRelativeMouseState(&dx, &dy);
        if (!(buttons & SDL_BUTTON_RMASK) || !window) {
            SDL_SetWindowRelativeMouseMode(window, false);
            m_mouseCaptured = false;
        }
    } else if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && window) {
        if (SDL_SetWindowRelativeMouseMode(window, true)) {
            m_mouseCaptured = true;
            SDL_GetRelativeMouseState(nullptr, nullptr);
        }
    }
    const bool active = hovered || m_mouseCaptured;
    if (active && io.MouseWheel != 0) {
        if (followCharacter()) character.zoom(io.MouseWheel);
        else m_distance = std::clamp(m_distance * std::exp(-io.MouseWheel * 0.15f), 2.f, 8000.f);
    }
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle) && character.loaded()) {
        character.follow = !character.follow;
        if (!character.follow) {
            // The free camera starts where Sir Canegm stands.
            const auto here = flat(character.position());
            m_target = glm::vec3(here.x, 0, here.z);
            m_distance = std::max(8.f, glm::length(m_camera.position() - character.position()) / Metre);
        }
    }
    if (followCharacter()) {
        const bool* keys = SDL_GetKeyboardState(nullptr);
        const bool keyboard = (m_mouseCaptured || ImGui::IsWindowFocused()) && !io.WantTextInput;
        constexpr float turnSpeed = 90.f;
        if (keyboard && keys[SDL_SCANCODE_LEFT]) dx -= turnSpeed * dt / .15f;
        if (keyboard && keys[SDL_SCANCODE_RIGHT]) dx += turnSpeed * dt / .15f;
        const bool mouseWalking = m_mouseCaptured && (buttons & SDL_BUTTON_LMASK) && (buttons & SDL_BUTTON_RMASK);
        step(dt, mouseWalking || (keyboard && keys[SDL_SCANCODE_W]), dx, dy);
        return;
    }
    step(dt, false);
    if (m_mouseCaptured) {
        m_yaw -= dx * 0.3f;
        m_pitch = std::clamp(m_pitch + dy * 0.3f, 5.f, 89.f);
    }
    if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.f)) {
        const float yaw = glm::radians(m_yaw), step = m_distance * 0.0015f / U73dScale::TileMetres;
        const glm::vec3 right(std::cos(yaw), 0, -std::sin(yaw)), back(std::sin(yaw), 0, std::cos(yaw));
        m_target -= (right * io.MouseDelta.x + back * io.MouseDelta.y) * step;
    }
}

void Britannia3dView::draw(bool* open) {
    ImGui::SetNextWindowSize(ImVec2(900, 650), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Britannia3d", open)) { ImGui::End(); return; }
    ImGui::Checkbox("Waende, Tueren, Fenster (Ebene 1)", &layerVisible[1]);
    ImGui::SameLine();
    ImGui::Checkbox("Felsen, Pflanzen, Baeume (Ebene 3)", &layerVisible[3]);
    ImGui::SameLine();
    ImGui::Checkbox("Daecher", &roofsVisible);
    ImGui::SameLine();
    if (m_stableProps || m_stableTools) {
        ImGui::Checkbox("Stallszene", &stableSceneVisible);
        ImGui::SameLine();
    }
    if (character.loaded()) {
        bool follow = character.follow;
        if (ImGui::Checkbox("Sir Canegm folgen", &follow)) {
            character.follow = follow;
            if (!follow) { const auto here = flat(character.position()); m_target = glm::vec3(here.x, 0, here.z); m_distance = 40.f; }
        }
        ImGui::SameLine();
    }
    ImGui::TextDisabled(followCharacter() ? "Rechte Maus: drehen | Links+Rechts oder W: gehen | Rad: Zoom | Mittelklick: freie Kamera"
                                          : "Rechte Maus: drehen | Links ziehen: verschieben | Rad: Zoom | Mittelklick: Sir Canegm");
    // Positions and distances in metres (1 tile = 1 m; tile x / y are east / south).
    {
        const auto eye = flat(m_camera.position());
        if (character.loaded()) {
            const auto at = flat(character.position());
            ImGui::Text("Sir Canegm: Tile %.1f, %.1f (%.2f m gross)  |  Kamera: %.1f m entfernt, %.1f m ueber dem Boden  |  1 Tile = %.2f m",
                        at.x, at.z, U73dScale::CharacterHeight, glm::length(m_camera.position() - character.position()) / Metre, eye.y,
                        U73dScale::TileMetres);
        } else
            ImGui::Text("Kamera: Tile %.1f, %.1f, %.1f m ueber dem Boden  |  1 Tile = %.2f m", eye.x, eye.z, eye.y, U73dScale::TileMetres);
    }
    const ImVec2 size = ImGui::GetContentRegionAvail();
    const int width = std::max(1, int(size.x)), height = std::max(1, int(size.y));
    const ImVec2 corner = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("Britannia3d view", ImVec2(float(width), float(height)),
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    const bool hovered = ImGui::IsItemHovered();
    handleInput(hovered);
    // A click (no drag, right button free) on a door opens or closes it.
    const auto& io = ImGui::GetIO();
    if (hovered && !m_mouseCaptured && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && io.MouseDragMaxDistanceSqr[0] < 16.f &&
        !ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        const glm::vec2 ndc(2.f * (io.MousePos.x - corner.x) / width - 1.f, 1.f - 2.f * (io.MousePos.y - corner.y) / height);
        if (const int gate = pickWinch(ndc); gate >= 0) {
            auto& g = m_gates[size_t(gate)];
            if (io.KeyShift) {
                g.locked = !g.locked;
                m_gateMessage = g.locked ? "Winde verriegelt" : "Winde entriegelt";
            } else if (!operateGate(size_t(gate))) m_gateMessage = "Die Winde ist verriegelt (Shift + Klick entriegelt)";
            else m_gateMessage = g.open ? "Das Fallgatter wird hochgezogen" : "Das Fallgatter wird herabgelassen";
            m_gateMessageTime = 3.f;
        } else if (const int door = pickDoor(ndc); door >= 0) toggleDoor(size_t(door));
    }
    for (auto& gate : m_gates) {
        // Drawn up slowly (0.6 m/s) into the gateway above, let down faster.
        const float target = gate.open ? gate.height * 0.8f : 0.f, dt = std::clamp(io.DeltaTime, 0.f, 0.1f);
        const float step = (gate.open ? 0.6f : 1.5f) * dt;
        gate.raised += std::clamp(target - gate.raised, -step, step);
    }
    if (hovered && !m_mouseCaptured) {
        const glm::vec2 ndc(2.f * (io.MousePos.x - corner.x) / width - 1.f, 1.f - 2.f * (io.MousePos.y - corner.y) / height);
        if (const int gate = pickWinch(ndc); gate >= 0)
            ImGui::SetTooltip("Torwinde (%s)\nKlick: Fallgatter %s | Shift + Klick: %s", m_gates[size_t(gate)].locked ? "verriegelt" : "frei",
                              m_gates[size_t(gate)].open ? "herablassen" : "hochziehen", m_gates[size_t(gate)].locked ? "entriegeln" : "verriegeln");
    }
    if (m_gateMessageTime > 0.f) {
        m_gateMessageTime -= io.DeltaTime;
        ImGui::GetWindowDrawList()->AddText(ImVec2(corner.x + 12, corner.y + 12), IM_COL32(255, 230, 160, 255), m_gateMessage.c_str());
    }
    for (auto& door : m_doors) {
        const float target = door.open ? door.openAngle : 0.f, step = glm::radians(120.f) * std::clamp(io.DeltaTime, 0.f, 0.1f);
        door.angle += std::clamp(target - door.angle, -step, step);
    }
    render(width, height);
    ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(m_color), corner,
                                         ImVec2(corner.x + width, corner.y + height), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::End();
}
