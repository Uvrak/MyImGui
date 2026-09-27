// Stable tools of Ultima7Remake (StoneLayer::loadTools / transform / render), imported into
// Britannia3d at their start places. Keep in step with Ultima7Remake/StoneLayer.h.
#include "StableScene.h"
#include <Canegm/GltfModel.h>
#include <glad/gl.h>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

std::string text(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Missing shader: " + path.string());
    return {std::istreambuf_iterator<char>(in), {}};
}

// Map direction of each tool's own handle-to-head axis (degrees clockwise from east).
constexpr float ToolAxis[StableTools::ToolCount] = {-34.4f, 144.9f, 140.2f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};

}

void StableTools::load(const std::filesystem::path& assets, const std::filesystem::path& data, const Ground& ground) {
    if (!m_shader.create(text(data / "Shaders/static-prop.vert").c_str(), text(data / "Shaders/static-prop.frag").c_str()))
        throw std::runtime_error("Tool shader failed");
    const char* files[ToolCount] = {"Props/GardenTools/Pitchfork/Pitchfork.glb", "Props/GardenTools/Shovel/Shovel.glb",
                                    "Props/GardenTools/Rake/Rake.glb", "Props/Stall/hufeisen.glb", "Props/Stall/zange.glb",
                                    "Props/Stall/ritualkerze.glb", "Props/Stall/schluessel.glb",
                                    "Props/Stall/beutel.glb", "Props/Stall/gargoyle-schmuck.glb"};
    for (int t = 0; t < ToolCount; ++t) {
        GltfModel model;
        if (!loadGltf(assets / files[t], model) || model.meshes.empty()) throw std::runtime_error(std::string("Missing stable tool: ") + files[t]);
        std::vector<float> vertices, glow;
        for (const auto& part : model.meshes) {
            const std::string material = part.materialIndex >= 0 && size_t(part.materialIndex) < model.materials.size() ? model.materials[part.materialIndex].name : "";
            // static-prop shader surfaces: iron, dark iron, wax, flame, brass, tool handle ...
            const float surface = material == "worn-iron" ? 16.f : material == "iron-dark" ? 23.f : material == "wax" ? 24.f :
                                  material == "flame" ? 25.f : material == "brass" ? 26.f : material == "glow" ? 27.f : material == "sackcloth" ? 29.f :
                                  material == "binding" ? 21.f : material == "gold" ? 30.f : material == "gem" ? 31.f : 17.f;
            // A flame's light pool is drawn separately, added onto whatever lies below it.
            auto& target = surface == 27.f ? glow : vertices;
            for (auto index : part.indices) {
                if (index >= part.vertices.size()) throw std::runtime_error("Stable tool index out of range");
                const auto& v = part.vertices[index];
                target.insert(target.end(), {v.px, v.py, v.pz, v.nx, v.ny, v.nz, v.px, v.pz, surface});
            }
        }
        if (!m_meshes[t].create(vertices.data(), unsigned(vertices.size() / 9), 9)) throw std::runtime_error("Stable tool upload failed");
        if (!glow.empty() && !m_glow[t].create(glow.data(), unsigned(glow.size() / 9), 9)) throw std::runtime_error("Stable tool glow upload failed");
    }
    // Maps/stable-tools.txt: sourceId tool x y yaw scale height (metres).
    std::ifstream in(data / "Maps/stable-tools.txt");
    std::string tag;
    size_t count = 0;
    if (!(in >> tag >> count) || tag != "tool-catalog-v2" || count > 64) throw std::runtime_error("Invalid stable tool catalog");
    m_items.clear();
    for (size_t i = 0; i < count; ++i) {
        unsigned id = 0;
        Item item;
        if (!(in >> id >> item.tool >> item.x >> item.y >> item.yaw >> item.scale >> item.height) || item.tool < 0 || item.tool >= ToolCount)
            throw std::runtime_error("Invalid stable tool entry");
        if (skip[size_t(item.tool)]) continue;
        // As StoneLayer::transform: GLB space (y up), model xz on the map plane (x east, z south),
        // turned so the tool's own handle-to-head axis points along yaw.
        const auto p = ground(item.x, item.y), up = glm::normalize(p);
        const auto east = glm::normalize(glm::cross(glm::vec3(0, 1, 0), up)), south = -glm::cross(up, east);
        const float turn = glm::radians(item.yaw - ToolAxis[item.tool]), size = item.scale * StableScene::Metre;
        const auto e = east * std::cos(turn) + south * std::sin(turn), z = south * std::cos(turn) - east * std::sin(turn);
        item.model[0] = glm::vec4(e * size, 0);
        item.model[1] = glm::vec4(up * size, 0);
        item.model[2] = glm::vec4(z * size, 0);
        item.model[3] = glm::vec4(p + up * (item.height * StableScene::Metre), 1);
        m_items.push_back(item);
    }
}

void StableTools::render(const glm::mat4& viewProjection, const glm::vec3& eye) {
    if (m_items.empty()) return;
    m_shader.bind();
    m_shader.setMat4("viewProjection", viewProjection);
    m_shader.setVec3("eye", eye);
    m_shader.setFloat("flipZ", 1.f);
    m_shader.setInt("rigged", 0);
    m_shader.setInt("joloAnimated", 0);
    glDisable(GL_CULL_FACE);
    for (const auto& item : m_items) {
        m_shader.setMat4("model", item.model);
        m_shader.setFloat("unitMetres", item.scale);
        m_meshes[size_t(item.tool)].bind();
        glDrawArrays(GL_TRIANGLES, 0, GLsizei(m_meshes[size_t(item.tool)].vertexCount()));
    }
    // Candle light on the floor: additive, no depth writes, pulled towards the camera.
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-2.f, -2.f);
    for (const auto& item : m_items)
        if (m_glow[size_t(item.tool)].vertexCount()) {
            m_shader.setMat4("model", item.model);
            m_shader.setFloat("unitMetres", item.scale);
            m_glow[size_t(item.tool)].bind();
            glDrawArrays(GL_TRIANGLES, 0, GLsizei(m_glow[size_t(item.tool)].vertexCount()));
        }
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}
