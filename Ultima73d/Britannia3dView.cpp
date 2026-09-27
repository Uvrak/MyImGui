#include "Britannia3dView.h"
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

namespace {

constexpr float LiftMetres = 0.5f;
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
uniform vec2 centre;
uniform float metre;
uniform float radius;
uniform bool direct;
out vec3 vNormal;
out vec2 vUv;
out float vShade;
vec3 planet(vec3 f) {
    vec2 t = (f.xz - centre) * metre;
    float a = length(t);
    vec3 d = vec3(0.0, 0.0, 1.0);
    if (a > 1e-9) d = cos(a / radius) * d + sin(a / radius) * vec3(t.x, -t.y, 0.0) / a;
    return d * (radius + f.y * metre);
}
void main() {
    vNormal = aNormal; vUv = aUv; vShade = aShade;
    gl_Position = projection * view * vec4(direct ? aPosition : planet(aPosition), 1.0);
})";

// Lit in the flat frame: every place of Britannia gets the same U7 light from the south east.
const char* FragmentSource = R"(#version 460 core
in vec3 vNormal;
in vec2 vUv;
in float vShade;
uniform sampler2D image;
out vec4 color;
void main() {
    vec4 texel = texture(image, vUv);
    if (texel.a < 0.5) discard;
    vec3 n = normalize(gl_FrontFacing ? vNormal : -vNormal);
    float light = 0.62 + 0.38 * max(dot(n, normalize(vec3(-0.35, 1.0, 0.45))), 0.0);
    color = vec4(texel.rgb * light * vShade, 1.0);
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
    if (!m_textures.empty()) glDeleteTextures(GLsizei(m_textures.size()), m_textures.data());
    m_textures.clear();
    if (m_color) glDeleteTextures(1, &m_color);
    if (m_depth) glDeleteRenderbuffers(1, &m_depth);
    if (m_framebuffer) glDeleteFramebuffers(1, &m_framebuffer);
    m_color = m_depth = m_framebuffer = 0;
    m_targetWidth = m_targetHeight = 0;
}

glm::vec3 Britannia3dView::planet(glm::vec3 f) const {
    const glm::vec2 t = (glm::vec2(f.x, f.z) - m_centre) * Metre;
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
    const glm::vec2 xz = m_centre + t / Metre;
    return {xz.x, (length - Radius) / Metre, xz.y};
}

unsigned Britannia3dView::makeTexture(int width, int height, const std::uint8_t* rgba, bool mipmaps) {
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    if (mipmaps) glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_textures.push_back(id);
    return id;
}

void Britannia3dView::addQuad(Batch& batch, const glm::vec3 (&p)[4], const glm::vec2 (&uv)[4], const glm::vec3& normal) {
    for (int i : {0, 1, 2, 0, 2, 3})
        batch.vertices.push_back({p[i].x, p[i].y, p[i].z, normal.x, normal.y, normal.z, uv[i].x, uv[i].y, 1.f});
}

void Britannia3dView::addObject(const U7::Data& data, const U7::WorldObject& object, int layer, bool roof) {
    auto [model, loaded] = m_models.try_emplace({object.shape, object.frame});
    if (loaded) {
        bool created = false;
        auto file = U7ObjectModel::loadOrCreate(assetDirectory, data, object.shape, object.frame, &created);
        m_createdFiles += created;
        if (!file.empty()) {
            model->second.texture = makeTexture(file.textureWidth, file.textureHeight, file.texture.data(), false);
            model->second.kind = file.kind;
            model->second.vertices = std::move(file.vertices);
        }
    }
    const auto& source = model->second;
    if (source.vertices.empty()) return;
    auto [slot, added] = m_batchOfFrame.try_emplace({object.shape, object.frame, layer}, m_batches.size());
    if (added) {
        auto& created = m_batches.emplace_back();
        created.texture = source.texture;
        created.layer = layer;
        created.roof = roof;
    }
    Batch& batch = m_batches[slot->second];
    // Model origin = south east bottom corner of the object, in flat world metres.
    const glm::vec3 anchor(float(object.x + 1), object.lift * LiftMetres, float(object.y + 1));
    // Walls, doors, fences ... of layer 1 stop Sir Canegm (not the walkways on the town walls).
    const bool wall = layer == 1 && source.kind != U7ObjectModel::Kind::Flat && source.kind != U7ObjectModel::Kind::Upright;
    auto& walls = m_walls[{object.x / 8, object.y / 8}];
    for (const auto& v : source.vertices) {
        // Faces turned away from the light get a little darker, as in U7.
        const float shade = v.normal.x < -0.5f || v.normal.z < -0.5f ? 0.9f : 1.f;
        const auto p = anchor + v.position;
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
    auto wrapped = [&](float x, float y) { return glm::length(glm::vec2(x, y) - m_centre) * Metre / Radius < MaxWrapAngle; };
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

    // Detail ground: per chunk its 16 x 16 flat tiles as one texture, 4 x 4 quads so the
    // ground follows the sphere.
    constexpr int side = c * P, parts = 4;
    std::vector<std::uint8_t> ground(size_t(side) * side * 4);
    for (int cy = chunkY0; cy <= chunkY1; ++cy)
        for (int cx = chunkX0; cx <= chunkX1; ++cx) {
            for (int ty = 0; ty < c; ++ty)
                for (int tx = 0; tx < c; ++tx) {
                    const auto t = U7GroundGrid::groundTile(data, cx * c + tx, cy * c + ty);
                    const auto pixels = U7GroundGrid::rgba(data, t.shape, t.frame);
                    for (int j = 0; j < P; ++j)
                        std::copy_n(pixels.begin() + j * P * 4, P * 4, ground.begin() + ((size_t(ty) * P + j) * side + size_t(tx) * P) * 4);
                }
            auto& batch = m_batches.emplace_back();
            batch.texture = makeTexture(side, side, ground.data(), true);
            for (int j = 0; j < parts; ++j)
                for (int i = 0; i < parts; ++i) {
                    const float s = float(c) / parts, x0 = cx * c + i * s, y0 = cy * c + j * s;
                    const float u0 = float(i) / parts, v0 = float(j) / parts, d = 1.f / parts;
                    addQuad(batch, {glm::vec3(x0, 0, y0), glm::vec3(x0 + s, 0, y0), glm::vec3(x0 + s, 0, y0 + s), glm::vec3(x0, 0, y0 + s)},
                            {glm::vec2(u0, v0), glm::vec2(u0 + d, v0), glm::vec2(u0 + d, v0 + d), glm::vec2(u0, v0 + d)}, {0, 1, 0});
                }
        }

    // Every object whose footprint touches the detail chunks, except markers the game never shows.
    const int x0 = chunkX0 * c, y0 = chunkY0 * c, x1 = (chunkX1 + 1) * c, y1 = (chunkY1 + 1) * c;
    std::map<int, std::string> names;
    for (const auto& object : data.objects()) {
        const auto size = data.shapeSize(object.shape);
        if (object.x < x0 || object.x - size.x + 1 >= x1 || object.y < y0 || object.y - size.y + 1 >= y1) continue;
        auto [it, added] = names.try_emplace(object.shape);
        if (added) it->second = lower(data.name(object.shape));
        const auto& name = it->second;
        if (name == "egg" || name == "path" || name == "light source") continue;
        const int layer = layerOf ? layerOf(object, name) : 2;
        addObject(data, object, layer == 1 || layer == 3 ? layer : 2, name.find("roof") != std::string::npos);
    }
    buildGlobe(data);
    for (auto* batches : {&m_batches, &m_globe})
        for (auto& batch : *batches)
            batch.mesh.create(reinterpret_cast<const float*>(batch.vertices.data()), unsigned(batch.vertices.size()), 9);
    m_target = glm::vec3(m_centre.x, 0, m_centre.y);
    m_distance = std::max(26.f, std::max(chunkX1 - chunkX0 + 1, chunkY1 - chunkY0 + 1) * c * 1.3f);
}

void Britannia3dView::prepareCollision(ow3d::BuildingCollision& collision, glm::vec3 position, float height) const {
    const auto here = flat(position);
    const int cellX = int(std::floor(here.x / 8)), cellY = int(std::floor(here.z / 8));
    std::vector<glm::vec3> triangles;
    for (int y = cellY - 2; y <= cellY + 2; ++y)
        for (int x = cellX - 2; x <= cellX + 2; ++x)
            if (auto it = m_walls.find({x, y}); it != m_walls.end())
                for (const auto& p : it->second) triangles.push_back(planet(p));
    collision.setSurfaceGeometry(position, height, triangles);
}

bool Britannia3dView::loadCharacter(const std::filesystem::path& folder, float tileX, float tileY) {
    if (!std::filesystem::exists(folder) || !character.load(folder)) return false;
    character.outfit.load(folder);
    // Settings of Ultima7Remake (scene scale 0.6 there, so its distances are taken x 0.6).
    character.setHeight(1.8f * Metre);
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
    m_camera.setPosition(planet(m_target + direction * m_distance));
    m_camera.lookAt(target, glm::normalize(target));
    (void)aspect;
}

void Britannia3dView::resizeTarget(int width, int height) {
    if (width == m_targetWidth && height == m_targetHeight && m_framebuffer) return;
    if (!m_framebuffer) {
        glGenFramebuffers(1, &m_framebuffer);
        glGenTextures(1, &m_color);
        glGenRenderbuffers(1, &m_depth);
    }
    m_targetWidth = width; m_targetHeight = height;
    glBindTexture(GL_TEXTURE_2D, m_color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_color, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depth);
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
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glViewport(0, 0, width, height);
    // Sky blue near the ground, dark space high above the globe.
    const float space = glm::smoothstep(0.5f, 12.f, altitude);
    glClearColor(0.42f * (1 - space) + 0.02f * space, 0.6f * (1 - space) + 0.02f * space, 0.82f * (1 - space) + 0.06f * space, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    m_shader.bind();
    m_shader.setMat4("view", m_camera.viewMatrix());
    m_shader.setMat4("projection", m_camera.projectionMatrix());
    m_shader.setInt("image", 0);
    m_shader.setFloat("metre", Metre);
    m_shader.setFloat("radius", Radius);
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glUniform2f(glGetUniformLocation(GLuint(program), "centre"), m_centre.x, m_centre.y);
    const GLint direct = glGetUniformLocation(GLuint(program), "direct");
    glActiveTexture(GL_TEXTURE0);
    const bool roofs = roofsVisible && !(followCharacter() && characterUnderRoof());
    for (const auto* batches : {&m_globe, &m_batches})
        for (const auto& batch : *batches) {
            if (batches == &m_batches && ((batch.roof && !roofs) || !layerVisible[size_t(batch.layer)])) continue;
            glUniform1i(direct, batch.layer < 0);
            glBindTexture(GL_TEXTURE_2D, batch.texture);
            batch.mesh.bind();
            glDrawArrays(GL_TRIANGLES, 0, GLsizei(batch.mesh.vertexCount()));
        }
    glBindVertexArray(0);
    if (character.loaded()) character.draw(m_camera, height);
    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
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
        const float yaw = glm::radians(m_yaw), step = m_distance * 0.0015f;
        const glm::vec3 right(std::cos(yaw), 0, -std::sin(yaw)), back(std::sin(yaw), 0, std::cos(yaw));
        m_target -= (right * io.MouseDelta.x + back * io.MouseDelta.y) * step;
    }
}

void Britannia3dView::draw(bool* open) {
    ImGui::SetNextWindowSize(ImVec2(900, 650), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Britannia3d", open)) { ImGui::End(); return; }
    ImGui::Checkbox("Ebene 1", &layerVisible[1]);
    ImGui::SameLine();
    ImGui::Checkbox("Ebene 3", &layerVisible[3]);
    ImGui::SameLine();
    ImGui::Checkbox("Daecher", &roofsVisible);
    ImGui::SameLine();
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
    const ImVec2 size = ImGui::GetContentRegionAvail();
    const int width = std::max(1, int(size.x)), height = std::max(1, int(size.y));
    const ImVec2 corner = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("Britannia3d view", ImVec2(float(width), float(height)),
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    handleInput(ImGui::IsItemHovered());
    render(width, height);
    ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(m_color), corner,
                                         ImVec2(corner.x + width, corner.y + height), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::End();
}
