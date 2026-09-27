#pragma once
#include "U7Data.h"
#include "U7ObjectModel.h"
#include <Camera.h>
#include <Mesh.h>
#include <Shader.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <array>
#include <functional>
#include <map>
#include <string>
#include <tuple>
#include <vector>

// "Britannia3d": a 3D view of a rectangle of U7 chunks (16 x 16 tiles each), built from the
// original data with the Engine (Shader, Mesh, Camera). 1 tile = 1 m, 1 lift = 0.5 m.
//
// Every object graphic (shape, frame) is its own model file below assetDirectory, see
// U7ObjectModel; missing files are generated from the U7 data on build.
class Britannia3dView {
public:
    ~Britannia3dView();
    // Layer of an object (lower-case name given): 1 and 3 can be hidden, anything else is layer 2.
    using LayerOf = std::function<int(const U7::WorldObject&, const std::string& name)>;

    // Chunks chunkX0..chunkX1 and chunkY0..chunkY1 (inclusive). Needs a current OpenGL 4.6 context.
    void build(const U7::Data& data, int chunkX0, int chunkY0, int chunkX1, int chunkY1, const LayerOf& layerOf,
               const char* title = "");
    // ImGui window "Britannia3d". Right mouse: turn, middle mouse: move, wheel: zoom.
    void draw(bool* open);
    // Renders one image with the current camera, for exports and tests (RGBA, top row first).
    std::vector<std::uint8_t> renderImage(int width, int height);

    std::filesystem::path assetDirectory = "assets";
    bool roofsVisible = true;
    std::array<bool, 4> layerVisible{true, true, true, true};   // index = layer (0 = ground)
    size_t boxCount() const { return m_boxes; }
    size_t quadObjectCount() const { return m_quadObjects; }
    size_t modelCount() const { return m_models.size(); }
    size_t createdFileCount() const { return m_createdFiles; }

    // Loads the OpenGL functions the Engine uses (glad); call once after creating the context.
    static bool loadOpenGl();

private:
    struct Vertex { float x, y, z, nx, ny, nz, u, v, shade; };
    struct Batch { unsigned texture = 0; int layer = 0; bool roof = false; std::vector<Vertex> vertices; ow3d::Mesh mesh; };

    void addQuad(Batch& batch, const glm::vec3 (&p)[4], const glm::vec2 (&uv)[4], const glm::vec3& normal);
    void addObject(const U7::Data& data, const U7::WorldObject& object, int layer, bool roof);
    void updateCamera(float aspect);
    void render(int width, int height);
    void resizeTarget(int width, int height);
    void destroy();

    int m_chunkX = 0, m_chunkY = 0, m_chunkX1 = 0, m_chunkY1 = 0;   // origin = north west corner of chunk X/Y
    std::string m_title;
    size_t m_boxes = 0, m_quadObjects = 0, m_createdFiles = 0;
    struct Model { unsigned texture = 0; U7ObjectModel::Kind kind = U7ObjectModel::Kind::File; std::vector<U7ObjectModel::Vertex> vertices; };
    std::map<std::pair<int, int>, Model> m_models;           // (shape, frame) -> loaded model file
    std::map<std::tuple<int, int, int>, size_t> m_batchOfFrame;   // (shape, frame, layer) -> batch
    std::vector<Batch> m_batches;
    std::vector<unsigned> m_textures;
    ow3d::Shader m_shader;
    ow3d::Camera m_camera;
    unsigned m_framebuffer = 0, m_color = 0, m_depth = 0;
    int m_targetWidth = 0, m_targetHeight = 0;
    glm::vec3 m_target{8.f, 0.f, 8.f};
    float m_yaw = 45.f, m_pitch = 40.f, m_distance = 26.f;   // degrees, metres
};
