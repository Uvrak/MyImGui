#include "Britannia3dView.h"
#include "U7GroundGrid.h"
#include "U7ObjectModel.h"
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

const char* VertexSource = R"(#version 460 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;
layout(location = 3) in float aShade;
uniform mat4 view;
uniform mat4 projection;
out vec3 vNormal;
out vec2 vUv;
out float vShade;
void main() {
    vNormal = aNormal; vUv = aUv; vShade = aShade;
    gl_Position = projection * view * vec4(aPosition, 1.0);
})";

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

Britannia3dView::~Britannia3dView() { destroy(); }

void Britannia3dView::destroy() {
    m_batches.clear();
    m_batchOfFrame.clear();
    if (!m_textures.empty()) glDeleteTextures(GLsizei(m_textures.size()), m_textures.data());
    m_textures.clear();
    if (m_color) glDeleteTextures(1, &m_color);
    if (m_depth) glDeleteRenderbuffers(1, &m_depth);
    if (m_framebuffer) glDeleteFramebuffers(1, &m_framebuffer);
    m_color = m_depth = m_framebuffer = 0;
    m_targetWidth = m_targetHeight = 0;
}

void Britannia3dView::addQuad(Batch& batch, const glm::vec3 (&p)[4], const glm::vec2 (&uv)[4], const glm::vec3& normal) {
    // Faces facing away from the light get a little darker, as in U7.
    const float shade = normal.x < -0.5f || normal.z < -0.5f ? 0.9f : 1.f;
    for (int i : {0, 1, 2, 0, 2, 3})
        batch.vertices.push_back({p[i].x, p[i].y, p[i].z, normal.x, normal.y, normal.z, uv[i].x, uv[i].y, shade});
}

void Britannia3dView::addObject(const U7::Data& data, const U7::WorldObject& object, int layer, bool roof) {
    auto [model, loaded] = m_models.try_emplace({object.shape, object.frame});
    if (loaded) {
        bool created = false;
        auto file = U7ObjectModel::loadOrCreate(assetDirectory, data, object.shape, object.frame, &created);
        m_createdFiles += created;
        if (!file.empty()) {
            GLuint id = 0;
            glGenTextures(1, &id);
            glBindTexture(GL_TEXTURE_2D, id);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, file.textureWidth, file.textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, file.texture.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            m_textures.push_back(id);
            model->second.texture = id;
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
    // Model origin = south east bottom corner of the object, placed relative to the north west
    // corner of the first chunk.
    const glm::vec3 anchor(float(object.x + 1 - m_chunkX * U7::ChunkTiles), object.lift * LiftMetres,
                           float(object.y + 1 - m_chunkY * U7::ChunkTiles));
    for (const auto& v : source.vertices) {
        // Faces turned away from the light get a little darker, as in U7.
        const float shade = v.normal.x < -0.5f || v.normal.z < -0.5f ? 0.9f : 1.f;
        const auto p = anchor + v.position;
        batch.vertices.push_back({p.x, p.y, p.z, v.normal.x, v.normal.y, v.normal.z, v.uv.x, v.uv.y, shade});
    }
    ++(source.kind == U7ObjectModel::Kind::Upright ? m_quadObjects : m_boxes);
}

void Britannia3dView::build(const U7::Data& data, int chunkX0, int chunkY0, int chunkX1, int chunkY1, const LayerOf& layerOf,
                            const char* title) {
    destroy();
    m_chunkX = chunkX0; m_chunkY = chunkY0; m_chunkX1 = chunkX1; m_chunkY1 = chunkY1;
    m_title = title;
    m_boxes = m_quadObjects = m_createdFiles = 0;
    m_models.clear();
    if (!m_shader.create(VertexSource, FragmentSource)) throw std::runtime_error("Britannia3d shader failed");

    // Ground: per chunk its 16 x 16 flat tiles as one texture on a 16 m x 16 m square.
    constexpr int side = U7::ChunkTiles * P;
    constexpr float s = float(U7::ChunkTiles);
    std::vector<std::uint8_t> ground(size_t(side) * side * 4);
    for (int cy = chunkY0; cy <= chunkY1; ++cy)
        for (int cx = chunkX0; cx <= chunkX1; ++cx) {
            for (int ty = 0; ty < U7::ChunkTiles; ++ty)
                for (int tx = 0; tx < U7::ChunkTiles; ++tx) {
                    const auto t = U7GroundGrid::groundTile(data, cx * U7::ChunkTiles + tx, cy * U7::ChunkTiles + ty);
                    const auto pixels = U7GroundGrid::rgba(data, t.shape, t.frame);
                    for (int j = 0; j < P; ++j)
                        std::copy_n(pixels.begin() + j * P * 4, P * 4, ground.begin() + ((size_t(ty) * P + j) * side + size_t(tx) * P) * 4);
                }
            GLuint groundTexture = 0;
            glGenTextures(1, &groundTexture);
            glBindTexture(GL_TEXTURE_2D, groundTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, side, side, 0, GL_RGBA, GL_UNSIGNED_BYTE, ground.data());
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            m_textures.push_back(groundTexture);
            auto& groundBatch = m_batches.emplace_back();
            groundBatch.texture = groundTexture;
            const float gx = float(cx - chunkX0) * s, gz = float(cy - chunkY0) * s;
            addQuad(groundBatch, {glm::vec3(gx, 0, gz), glm::vec3(gx + s, 0, gz), glm::vec3(gx + s, 0, gz + s), glm::vec3(gx, 0, gz + s)},
                    {glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(1, 1), glm::vec2(0, 1)}, {0, 1, 0});
        }

    // Every object whose footprint touches the chunks, except markers the game never shows.
    const int x0 = chunkX0 * U7::ChunkTiles, y0 = chunkY0 * U7::ChunkTiles;
    const int x1 = (chunkX1 + 1) * U7::ChunkTiles, y1 = (chunkY1 + 1) * U7::ChunkTiles;
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
    for (auto& batch : m_batches)
        batch.mesh.create(reinterpret_cast<const float*>(batch.vertices.data()), unsigned(batch.vertices.size()), 9);
    m_target = glm::vec3((chunkX1 - chunkX0 + 1) * s / 2, 0, (chunkY1 - chunkY0 + 1) * s / 2);
    m_distance = std::max(26.f, std::max(chunkX1 - chunkX0 + 1, chunkY1 - chunkY0 + 1) * s * 1.3f);
}

void Britannia3dView::updateCamera(float aspect) {
    const float yaw = glm::radians(m_yaw), pitch = glm::radians(m_pitch);
    const glm::vec3 direction(std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch));
    m_camera.setPosition(m_target + direction * m_distance);
    m_camera.lookAt(m_target, glm::vec3(0, 1, 0));
    m_camera.setPerspective(45.f, aspect, 0.1f, 1000.f);
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
    updateCamera(float(width) / float(height));
    GLint previous = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glViewport(0, 0, width, height);
    glClearColor(0.42f, 0.6f, 0.82f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    m_shader.bind();
    m_shader.setMat4("view", m_camera.viewMatrix());
    m_shader.setMat4("projection", m_camera.projectionMatrix());
    m_shader.setInt("image", 0);
    glActiveTexture(GL_TEXTURE0);
    for (const auto& batch : m_batches) {
        if ((batch.roof && !roofsVisible) || !layerVisible[size_t(batch.layer)]) continue;
        glBindTexture(GL_TEXTURE_2D, batch.texture);
        batch.mesh.bind();
        glDrawArrays(GL_TRIANGLES, 0, GLsizei(batch.mesh.vertexCount()));
    }
    glBindVertexArray(0);
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

void Britannia3dView::draw(bool* open) {
    ImGui::SetNextWindowSize(ImVec2(900, 650), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Britannia3d", open)) { ImGui::End(); return; }
    ImGui::Text("%s Chunks (%d, %d) - (%d, %d)", m_title.c_str(), m_chunkX, m_chunkY, m_chunkX1, m_chunkY1);
    ImGui::SameLine();
    ImGui::Checkbox("Ebene 1", &layerVisible[1]);
    ImGui::SameLine();
    ImGui::Checkbox("Ebene 3", &layerVisible[3]);
    ImGui::SameLine();
    ImGui::Checkbox("Daecher", &roofsVisible);
    ImGui::SameLine();
    ImGui::TextDisabled("rechte Maus: drehen, mittlere Maus: verschieben, Rad: zoomen");
    const ImVec2 size = ImGui::GetContentRegionAvail();
    const int width = std::max(1, int(size.x)), height = std::max(1, int(size.y));
    render(width, height);
    ImGui::Image(static_cast<ImTextureID>(m_color), size, ImVec2(0, 1), ImVec2(1, 0));
    if (ImGui::IsItemHovered()) {
        const auto& io = ImGui::GetIO();
        if (io.MouseWheel != 0) m_distance = std::clamp(m_distance * std::exp(-io.MouseWheel * 0.12f), 2.f, 900.f);
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0)) {
            m_yaw -= io.MouseDelta.x * 0.3f;
            m_pitch = std::clamp(m_pitch + io.MouseDelta.y * 0.3f, 5.f, 89.f);
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0)) {
            const float yaw = glm::radians(m_yaw), step = m_distance * 0.0015f;
            const glm::vec3 right(std::cos(yaw), 0, -std::sin(yaw)), back(std::sin(yaw), 0, std::cos(yaw));
            m_target -= (right * io.MouseDelta.x + back * io.MouseDelta.y) * step;
        }
    }
    ImGui::End();
}
