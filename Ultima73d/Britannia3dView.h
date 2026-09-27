#pragma once
#include "U7Data.h"
#include "U7ObjectModel.h"
#include <AnimatedCharacter.h>
#include <Camera.h>
#include <Mesh.h>
#include <Shader.h>
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

// "Britannia3d": Britannia as a sphere world, built from the original U7 data with the Engine
// (Shader, Mesh, Camera, AnimatedCharacter). 1 tile = 1 m, 1 lift = 0.5 m.
//
// The whole U7 map lies on the Engine's planet (radius ow3d::PlanetRadius, 1 m = Metre units)
// as a coarse globe, one texel per tile, with ocean around it; a rectangle of chunks (Trinsic)
// is built in full detail on top: its ground tiles and every object as its own model file
// (see U7ObjectModel). Flat U7 coordinates (x east, height up, y south; metres) are wrapped
// onto the sphere around the detail centre (azimuthal equidistant), so the detail area keeps
// its true sizes.
//
// Sir Canegm walks on the sphere with the Ultima7Remake controls: right mouse held turns and
// tilts, left + right mouse or W walks, arrow keys turn, wheel zooms (close: frontal and first
// person, far: from above), middle click switches to the free camera and back.
class Britannia3dView {
public:
    static constexpr float Metre = 40.f / 1792.f;   // planet units per metre, as in Ultima7Remake

    ~Britannia3dView();
    // Layer of an object (lower-case name given): 1 and 3 can be hidden, anything else is layer 2.
    using LayerOf = std::function<int(const U7::WorldObject&, const std::string& name)>;

    // Detail chunks chunkX0..chunkX1 and chunkY0..chunkY1 (inclusive). Needs a current OpenGL 4.6 context.
    void build(const U7::Data& data, int chunkX0, int chunkY0, int chunkX1, int chunkY1, const LayerOf& layerOf,
               const char* title = "");
    // Loads Sir Canegm from an Ultima7Remake character folder and places him on a tile position
    // (fractional), looking north.
    bool loadCharacter(const std::filesystem::path& folder, float tileX, float tileY);
    // ImGui window "Britannia3d".
    void draw(bool* open);
    // Renders one image with the current camera, for exports and tests (RGBA, top row first).
    std::vector<std::uint8_t> renderImage(int width, int height);
    // Free camera looking at a tile position from distance metres, yaw/pitch in degrees.
    void setFreeCamera(float tileX, float tileY, float distance, float yaw, float pitch);
    // Advances Sir Canegm (walking when forward) and his camera, as a frame of the view does.
    void step(float dt, bool forward, float turn = 0, float tilt = 0);

    std::filesystem::path assetDirectory = "assets";
    bool roofsVisible = true;
    std::array<bool, 4> layerVisible{true, true, true, true};   // index = layer (0 = ground)
    ow3d::AnimatedCharacter character;
    bool followCharacter() const { return character.loaded() && character.follow; }
    // True while Sir Canegm stands under a roof: the roofs are hidden then, as in U7.
    bool characterUnderRoof() const;

    size_t boxCount() const { return m_boxes; }
    size_t quadObjectCount() const { return m_quadObjects; }
    size_t modelCount() const { return m_models.size(); }
    size_t createdFileCount() const { return m_createdFiles; }

    // Flat U7 position (x east, height up, y south; tiles = metres) <-> planet position.
    glm::vec3 planet(glm::vec3 flat) const;
    glm::vec3 flat(glm::vec3 planet) const;

    // Loads the OpenGL functions the Engine uses (glad); call once after creating the context.
    static bool loadOpenGl();

private:
    struct Vertex { float x, y, z, nx, ny, nz, u, v, shade; };
    struct Batch { unsigned texture = 0; int layer = 0; bool roof = false; std::vector<Vertex> vertices; ow3d::Mesh mesh; };
    struct Model { unsigned texture = 0; U7ObjectModel::Kind kind = U7ObjectModel::Kind::File; std::vector<U7ObjectModel::Vertex> vertices; };

    void addQuad(Batch& batch, const glm::vec3 (&p)[4], const glm::vec2 (&uv)[4], const glm::vec3& normal);
    void addObject(const U7::Data& data, const U7::WorldObject& object, int layer, bool roof);
    void buildGlobe(const U7::Data& data);
    unsigned makeTexture(int width, int height, const std::uint8_t* rgba, bool mipmaps);
    void prepareCollision(ow3d::BuildingCollision& collision, glm::vec3 position, float height) const;
    void updateFreeCamera(float aspect);
    void render(int width, int height);
    void resizeTarget(int width, int height);
    void handleInput(bool hovered);
    void destroy();

    int m_chunkX = 0, m_chunkY = 0, m_chunkX1 = 0, m_chunkY1 = 0;
    std::string m_title;
    glm::vec2 m_centre{0};                                   // flat position on the planet's pole (0, 0, R)
    size_t m_boxes = 0, m_quadObjects = 0, m_createdFiles = 0;
    std::map<std::pair<int, int>, Model> m_models;           // (shape, frame) -> loaded model file
    std::map<std::tuple<int, int, int>, size_t> m_batchOfFrame;   // (shape, frame, layer) -> batch
    std::vector<Batch> m_batches;                            // detail area
    std::vector<Batch> m_globe;                              // coarse world and ocean sphere
    std::vector<unsigned> m_textures;
    std::map<std::pair<int, int>, std::vector<glm::vec3>> m_walls;   // 8 x 8 tile cell -> flat wall triangles
    std::set<std::pair<int, int>> m_roofTiles;               // tiles under a roof: roofs hide above Sir Canegm
    ow3d::Shader m_shader;
    ow3d::Camera m_camera;
    unsigned m_framebuffer = 0, m_color = 0, m_depth = 0;
    int m_targetWidth = 0, m_targetHeight = 0;
    glm::vec3 m_target{0.f};                                 // free camera target, flat
    float m_yaw = 45.f, m_pitch = 40.f, m_distance = 26.f;   // degrees, metres
    bool m_mouseCaptured = false;
};
