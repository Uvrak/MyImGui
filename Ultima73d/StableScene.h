#pragma once
#include <Mesh.h>
#include <Shader.h>
#include <array>
#include <filesystem>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
struct GltfAnimationData;

// The stable scene of Ultima7Remake, imported into Britannia3d: its GLB props (the corpse, the
// dead gargoyle, tables, water trough, hay bundles, the pitchfork in the gargoyle, Jolo) and
// its stable tools (shovel, rake, horseshoes, tongs, ritual candles, keys, pouch, gargoyle
// jewellery), with Ultima7Remake's procedural materials (static-prop shaders).
//
// Files (copied from Ultima7Remake/assets): placements (Maps/static-scene-props.txt,
// Maps/stable-tools.txt ...) and Shaders/static-prop.* in the repository's data folder; the
// GLBs (Characters/{Leiche,ToterGargoyle,Jolo}, Props/Stall, Props/GardenTools) in the assets
// folder outside the repository.
// Places are Ultima7Remake map positions (1 unit = 2 U7 tiles = 1 m); the ground function
// turns one into a planet position. Sizes and heights are metres.
namespace StableScene {

inline constexpr float Metre = 40.f / 1792.f;      // planet units per metre (Ultima7Remake, Britannia3d)
using Ground = std::function<glm::vec3(float x, float y)>;

// Ultima7Remake map position -> U7 tile (fractional). One Ultima7Remake unit is about two U7
// tiles, but its stable is not exactly U7's: the map is fitted to U7's own walls so that
// everything stands in U7's building. West wall 655 -> 1056.5, chamber wall 664 -> 1072.5;
// north wall 1217 -> 2180.5, chamber wall 1221 -> 2188.5, south wall 1231 -> 2207.5.
inline glm::vec2 u7Tile(float x, float y) {
    const float tx = 1056.5f + (x - 655.f) * (16.f / 9.f);
    const float ty = y <= 1221.f ? 2180.5f + (y - 1217.f) * 2.f : 2188.5f + (y - 1221.f) * 1.9f;
    return {tx, ty};
}

}

// Ultima7Remake's StaticSceneProps (see StableSceneProps.cpp).
class StableSceneProps {
public:
    using Ground = StableScene::Ground;
    static constexpr float Metre = StableScene::Metre;
    ~StableSceneProps();
    // Props whose file path contains one of these are left out (e.g. "Pitchfork").
    std::vector<std::string> skipFiles;
    // assets: models (GLB); data: placements (Maps) and shaders, kept in the repository.
    void load(const std::filesystem::path& assets, const std::filesystem::path& data, const Ground& ground);
    void render(const glm::mat4& viewProjection, const glm::vec3& eye, float time);
    size_t size() const { return m_props.size(); }
    // Extent of each placed prop in metres around its place (east, up, north), for checks.
    struct Extent { std::string file; float x = 0, y = 0; glm::vec3 low{0}, high{0}; };
    const std::vector<Extent>& extents() const { return m_extents; }
private:
    struct Prop { std::array<ow3d::Mesh,3> lod; glm::mat4 model{1},restModel{1}; glm::vec3 center{},localCenter{}; float radius=0,metres=1,flipZ=1; unsigned texture=0; int style=0,skin=-1; std::array<int,3> clips{-1,-1,-1}; std::shared_ptr<const GltfAnimationData> animation; };
    std::vector<Prop> m_props;
    std::vector<Extent> m_extents;
    ow3d::Shader m_shader;
    bool m_joloLeatherEquipped=true;
    int m_joloMotion=0;
    float m_joloPhase=0;
};

// The stable tools of Ultima7Remake's StoneLayer (catalog Maps/stable-tools.txt), at their
// start places; drawn with the same shader, ritual candles with their light pools.
class StableTools {
public:
    using Ground = StableScene::Ground;
    static constexpr int ToolCount = 9;
    // Tools left out (0 pitchfork, 1 shovel, 2 rake, 3 horseshoe, 4 tongs, 5 candle, 6 key, 7 bag, 8 jewellery).
    std::array<bool, ToolCount> skip{};
    // assets: models (GLB); data: placements (Maps) and shaders, kept in the repository.
    void load(const std::filesystem::path& assets, const std::filesystem::path& data, const Ground& ground);
    void render(const glm::mat4& viewProjection, const glm::vec3& eye);
    size_t size() const { return m_items.size(); }
private:
    struct Item { int tool = 0; float x = 0, y = 0, yaw = 0, scale = 1, height = 0; glm::mat4 model{1}; };
    std::array<ow3d::Mesh, ToolCount> m_meshes, m_glow;
    std::vector<Item> m_items;
    ow3d::Shader m_shader;
};
