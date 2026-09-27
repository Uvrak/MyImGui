#include "Britannia3dView.h"
#include <set>
#include "PixelArtScale.h"
#include "U73dScale.h"
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
uniform int planks;             // 0: U7 graphic, 1: horizontal boards, 2: vertical boards (posts)
uniform vec3 tint;
uniform sampler2D woodAlbedo;
uniform sampler2D woodNormal;
uniform float woodSize;
uniform float woodTile;          // metres per tile: the material is mapped in metres
out vec4 color;
const vec3 sun = normalize(vec3(-0.35, 1.0, 0.45));
void main() {
    if (planks != 0) {
        // Plank material, mapped in metres: boards run horizontally on every wall face.
        vec3 n = normalize(vMapNormal), t, down;
        vec2 uv;
        if (abs(n.y) > 0.5) { uv = vFlat.xz * woodTile; t = vec3(1, 0, 0); down = vec3(0, 0, 1); }
        else if (abs(n.x) > 0.5) { uv = vec2(vFlat.z * woodTile, -vFlat.y); t = vec3(0, 0, 1); down = vec3(0, -1, 0); }
        else { uv = vec2(vFlat.x * woodTile, -vFlat.y); t = vec3(1, 0, 0); down = vec3(0, -1, 0); }
        if (planks == 2 && abs(n.y) <= 0.5) { uv = uv.yx; vec3 s = t; t = down; down = s; }
        uv /= woodSize;
        vec3 relief = texture(woodNormal, uv).xyz * 2.0 - 1.0;
        vec3 bumped = normalize(mat3(flatModel) * (t * relief.x + down * relief.y + n * relief.z));
        float light = 0.42 + 0.65 * max(dot(bumped, sun), 0.0);
        color = vec4(tint * texture(woodAlbedo, uv).rgb * 2.0 * light, 1.0);
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
    m_stableProps.reset();
    m_stableTools.reset();
    m_doors.clear();
    m_doorLeaf.clear();
    m_doorPlanks.destroy(); m_doorBattens.destroy(); m_doorIron.destroy();
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
    for (const auto& v : source.vertices) {
        // Faces turned away from the light get a little darker, as in U7.
        const float shade = v.normal.x < -0.5f || v.normal.z < -0.5f ? 0.9f : 1.f;
        auto local = v.position * stretch;
        // Thinner walls: squeeze the one tile thick side about its middle (x / z run -size .. 0).
        if (thinWall && source.kind == U7ObjectModel::Kind::Box) {
            const auto size = data.shapeSize(object.shape);
            if (size.y == 1 && size.x > 1) local.z = -0.5f + (local.z + 0.5f) * U73dScale::WallThickness;
            if (size.x == 1 && size.y > 1) local.x = -0.5f + (local.x + 0.5f) * U73dScale::WallThickness;
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
            }
        }
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
        const glm::vec3 local[4] = {{0, 0, 0}, {-4, 0, 0}, {-4, U73dScale::DoorHeight, 0}, {0, U73dScale::DoorHeight, 0}};
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

void Britannia3dView::buildStraw() {
    std::ifstream in(stableSceneDirectory / "Maps/stable-straw.txt");
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

void Britannia3dView::buildFences() {
    std::ifstream in(stableSceneDirectory / "Maps/stable-fences.txt");
    std::string tag;
    size_t count = 0;
    if (!(in >> tag >> count) || tag != "stable-fences-v1" || count > 512) return;
    std::set<std::tuple<int, int, int>> edges;
    std::string line;
    std::getline(in, line);
    while (edges.size() < count && std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream row(line);
        int x, y, side;
        if (row >> x >> y >> side) edges.insert({x, y, side});
    }
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
    auto& batch = m_batches.emplace_back();
    batch.texture = makeTexture(w, h, rail.data(), true);
    batch.layer = 1;
    auto& walls = m_walls;
    for (const auto& [x, y, side] : edges) {
        // As KnownGeometry::buildEditedEdge (kind 5): the edge runs along u (0 .. 1 cell), depth
        // across it; heights are metres. Ultima7Remake map units become U7 tiles.
        const bool vertical = side == 1 || side == 3;
        const float bx = float(x) + (side == 1 ? 1.f : 0.f), by = float(y) + (side == 2 ? 1.f : 0.f);
        auto point = [&](float u, float depth, float height) {
            const auto tile = StableScene::u7Tile(bx + (vertical ? depth : u), by + (vertical ? u : depth));
            return glm::vec3(tile.x, height, tile.y);
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
        auto fence = [&](int d) { return edges.count({x + (vertical ? 0 : d), y + (vertical ? d : 0), side}) > 0; };
        if ((along & 1) == 0 || !fence(-1)) box(-.08f, .16f, 0, 1.14f, .16f);
        if (!fence(1)) box(.92f, .16f, 0, 1.14f, .16f);
    }
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
    const bool stableScene = !stableSceneDirectory.empty() && std::filesystem::exists(stableSceneDirectory / "Maps/static-scene-props.txt");
    if (stableScene) {
        // The stable itself (U7 tiles 1054 - 1084, 2176 - 2208), not the house north of it.
        m_stableArea = glm::ivec4(1054, 2176, 1086, 2209);
    }
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
        // Ultima7Remake's fences and trough replace U7's in the stable and the paddock north east of it.
        if (stableScene && (name == "fence" || name == "trough") &&
            ((object.x >= m_stableArea.x && object.x < m_stableArea.z && object.y >= m_stableArea.y && object.y < m_stableArea.w) ||
             (object.x >= 1070 && object.x < 1090 && object.y >= 2158 && object.y < 2182)))
            continue;
        // The shed's doors become DoorModel doors (270: closed along x, 376: opened inwards).
        if (stableScene && name == "door" && (object.shape == 270 || object.shape == 376) &&
            object.x >= m_stableArea.x && object.x < m_stableArea.z && object.y >= m_stableArea.y && object.y < m_stableArea.w) {
            Door door;
            door.pivot = glm::vec3(object.x + 0.5f, 0.f, object.y + 0.5f);
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
    // Wooden plank walls get the high resolution plank material (stone and plaster keep U7's).
    m_woodAlbedo = makeTexture(wood.size, wood.size, wood.albedo.data(), true);
    m_woodNormal = makeTexture(wood.size, wood.size, wood.normal.data(), true);
    for (unsigned id : {m_woodAlbedo, m_woodNormal}) {
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    // Door meshes, and above each door opening the wall in plank material up to its full height.
    if (!m_doors.empty()) {
        constexpr float length = 4.f;                     // tiles, as U7's door objects
        const auto parts = DoorModel::build(length - 0.08f, U73dScale::DoorHeight, U73dScale::TileMetres);
        auto upload = [](ow3d::Mesh& mesh, const std::vector<DoorModel::Vertex>& vertices) {
            std::vector<float> data;
            for (const auto& v : vertices)
                data.insert(data.end(), {v.position.x - 0.04f, v.position.y, v.position.z, v.normal.x, v.normal.y, v.normal.z, 0.f, 0.f, 1.f});
            mesh.create(data.data(), unsigned(data.size() / 9), 9);
        };
        upload(m_doorPlanks, parts.planks);
        upload(m_doorBattens, parts.battens);
        upload(m_doorIron, parts.iron);
        for (const auto& p : parts.leaf) m_doorLeaf.push_back(p - glm::vec3(0.04f, 0, 0));
        const std::uint8_t iron[4] = {58, 56, 54, 255};
        m_ironTexture = makeTexture(1, 1, iron, false);
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
    for (const auto& p : placed) {
        const auto model = m_models.find({p.object->shape, p.object->frame});
        const bool plank = p.wall && model != m_models.end() && model->second.planks && model->second.kind == U7ObjectModel::Kind::Box;
        const int planks = !plank ? 0 : model->second.post ? 2 : 1;
        addObject(data, *p.object, p.layer, p.roof, p.liftMetres, planks, p.thinWall);
    }
    buildGlobe(data);
    if (stableScene) {
        const StableScene::Ground ground = [this](float x, float y) {
            const auto tile = StableScene::u7Tile(x, y);
            return planet({tile.x, 0, tile.y});
        };
        try {
            m_stableProps = std::make_unique<StableSceneProps>();
            m_stableProps->skipFiles = {"Pitchfork"};
            m_stableProps->load(stableSceneDirectory, ground);
            m_stableTools = std::make_unique<StableTools>();
            m_stableTools->skip[0] = m_stableTools->skip[1] = m_stableTools->skip[2] = true;   // U7's own tools instead
            m_stableTools->load(stableSceneDirectory, ground);
            buildFences();
            buildStraw();
        } catch (const std::exception& e) {
            SDL_Log("Britannia3d: stable scene not loaded: %s", e.what());
            m_stableProps.reset();
            m_stableTools.reset();
        }
    }
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
                for (const auto& p : it->second) triangles.push_back(planet(p));
    if (layerVisible[1])
        for (const auto& door : m_doors) {
            if (glm::distance(glm::vec2(door.pivot.x, door.pivot.z), glm::vec2(here.x, here.z)) > 12.f) continue;
            const auto model = doorModel(door);
            for (const auto& p : m_doorLeaf) triangles.push_back(planet(glm::vec3(model * glm::vec4(p, 1))));
        }
    collision.setSurfaceGeometry(position, height, triangles);
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
        drawDoors(GLuint(program));
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
        if (const int door = pickDoor(ndc); door >= 0) toggleDoor(size_t(door));
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
