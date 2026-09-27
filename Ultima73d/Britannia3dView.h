#pragma once
#include <functional>
#include <imgui.h>
#include "U73dScale.h"
#include "U7Data.h"
#include "DoorModel.h"
#include "StableScene.h"
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
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

// "Britannia3d": Britannia as a sphere world, built from the original U7 data with the Engine
// (Shader, Mesh, Camera, AnimatedCharacter), in real metric sizes (see U73dScale).
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
    static constexpr int TexelsPerTile = 64;          // U7's 8 pixels per tile, upscaled 8 x
    static constexpr int Samples = 4;                 // multisampling of the view

    ~Britannia3dView();
    // Layer of an object (lower-case name given): 1 and 3 can be hidden, anything else is layer 2.
    using LayerOf = std::function<int(const U7::WorldObject&, const std::string& name)>;

    // Detail chunks chunkX0..chunkX1 and chunkY0..chunkY1 (inclusive). Needs a current OpenGL 4.6 context.
    void build(const U7::Data& data, int chunkX0, int chunkY0, int chunkX1, int chunkY1, const LayerOf& layerOf,
               const char* title = "");
    // Loads Sir Canegm from an Ultima7Remake character folder and places him on a tile position
    // (fractional), looking north.
    bool loadCharacter(const std::filesystem::path& folder, float tileX, float tileY);
    ow3d::Camera& camera() { return m_camera; }
    // The ground under a screen point (normalised device coordinates), in world coordinates.
    bool pickGround(float x, float y, glm::vec3& ground) const;
    // Drawn over the view after it (the backpack); returns whether the mouse is over it.
    std::function<bool(ImVec2, ImVec2)> overlay;
    // Whether a screen point lies over the open backpack (things dropped there go into it).
    std::function<bool(ImVec2)> overBag;
    // The things in the backpack, drawn into its panel (top left, size); they can be dragged
    // back out onto the ground.
    void drawBagItems(ImVec2 panel, float size);
    float bagWeight() const;
    static constexpr float BagCapacityKg = 30.f;
    // The weight of what else the backpack holds (its equipment and compass), kg.
    std::function<float()> equipmentKg;
    // ImGui window "Britannia3d".
    void draw(bool* open);
    // Renders one image with the current camera, for exports and tests (RGBA, top row first).
    std::vector<std::uint8_t> renderImage(int width, int height);
    // Free camera looking at a tile position from distance metres, yaw/pitch in degrees.
    void setFreeCamera(float tileX, float tileY, float distance, float yaw, float pitch);
    // Advances Sir Canegm (walking when forward) and his camera, as a frame of the view does.
    void step(float dt, bool forward, float turn = 0, float tilt = 0);

    std::filesystem::path assetDirectory = "assets";
    // Ultima7Remake's stable scene (see StableScene.h); when the folder exists, it replaces the
    // U7 furnishings of the stable (the walls, fences and doors stay).
    std::filesystem::path stableSceneDirectory;
    // Text data kept in the repository: placements (Maps) and shaders.
    std::filesystem::path dataDirectory;
    bool stableSceneVisible = true;
    const StableSceneProps* stableProps() const { return m_stableProps.get(); }
    // The wooden shed doors (DoorModel), opened and closed by a click on them.
    size_t doorCount() const { return m_doors.size(); }
    bool doorOpen(size_t door) const { return m_doors[door].open; }
    void toggleDoor(size_t door, bool immediately = false);
    size_t stablePropCount() const { return (m_stableProps ? m_stableProps->size() : 0) + (m_stableTools ? m_stableTools->size() : 0); }
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

    // Flat U7 position (x east and y south in tiles, height up in metres) <-> planet position.
    // One tile is U73dScale::TileMetres.
    glm::vec3 planet(glm::vec3 flat) const;
    glm::vec3 flat(glm::vec3 planet) const;

    // Loads the OpenGL functions the Engine uses (glad); call once after creating the context.
    static bool loadOpenGl();

private:
    struct Vertex { float x, y, z, nx, ny, nz, u, v, shade; };
    // array: texture is a GL_TEXTURE_2D_ARRAY of ground tiles, the vertex "shade" is the layer.
    // planks: drawn with a high resolution material instead of the U7 graphic: 0 no (U7 graphic),
    // 1 horizontal boards, 2 upright boards (WoodMaterial, tinted), 3 thatch, 4 stone wall
    // (StoneMaterial), 5 slate (SlateMaterial).
    // wind: 0 still, 1 sways (bark), 2 sways and flutters (leaves).
    // cutout: may be cut away by the see-through circle (not the ground, kerbs and furniture).
    struct Batch { unsigned texture = 0; int layer = 0; bool roof = false, array = false, cutout = true; int planks = 0, wind = 0; glm::vec3 tint{1};
                   std::vector<Vertex> vertices; ow3d::Mesh mesh; };
    // planks: the original graphic is a wooden plank wall (brown, not light plaster); tint: its
    // mean colour, so the material keeps the tone of each wall.
    struct Model { unsigned texture = 0; U7ObjectModel::Kind kind = U7ObjectModel::Kind::File; std::vector<U7ObjectModel::Vertex> vertices;
                   bool planks = false, post = false, stone = false, fortress = false; glm::vec3 tint{1}; };

    void addQuad(Batch& batch, const glm::vec3 (&p)[4], const glm::vec2 (&uv)[4], const glm::vec3& normal);
    // liftMetres: real height of one lift for this object (U73dScale), for its lift and its model.
    // thinWall: a wall, door or window one tile thick, drawn U73dScale::WallThickness thick.
    void addObject(const U7::Data& data, const U7::WorldObject& object, int layer, bool roof, float liftMetres, int planks, bool thinWall);
    // True when the tile a step from `point` (flat) along `direction` lies under a roof.
    bool indoors(glm::vec3 point, glm::vec3 direction) const;
    void loadModels(const U7::Data& data, const std::vector<std::pair<int, int>>& graphics);
    void buildGround(const U7::Data& data);
    // Streets: U7's street tiles (grey cobbles) outside the houses get the high resolution
    // cobblestone (CobbleMaterial) and a raised kerb where they meet other ground.
    void buildRoads();
    // Portcullises, winches and wooden stairs as high resolution models (GateModels).
    void buildGateModels(const U7::Data& data, const std::vector<std::pair<U7::WorldObject, float>>& parts);
public:
    // Town gates: a click on the winch raises the portcullis into the gateway above it (or lets
    // it down), unless the winch is locked; Shift + click locks or unlocks the winch.
    size_t gateCount() const { return m_gates.size(); }
    bool gateOpen(size_t gate) const { return m_gates[gate].open; }
    bool gateLocked(size_t gate) const { return m_gates[gate].locked; }
    void setGateLocked(size_t gate, bool locked) { m_gates[gate].locked = locked; }
    // Returns false (and does nothing) while the gate's winch is locked.
    bool operateGate(size_t gate, bool immediately = false);
private:
    struct Gate {
        glm::vec3 origin{0};              // flat: west / north end of the portcullis, at its lift
        bool alongX = true;
        float length = 4.f, height = 3.f; // tiles, metres
        glm::vec3 winch{0};               // flat centre of its winch
        bool winchAlongX = true;          // the drum's axis
        bool open = false, locked = false;
        float raised = 0.f;               // metres the portcullis is drawn up
    };
    std::vector<Gate> m_gates;
    ow3d::Mesh m_gateIron, m_gateWood, m_drumIron, m_drumWood, m_drumDark;
    std::string m_gateMessage;
    float m_gateMessageTime = 0.f;
    glm::mat4 gateModel(const Gate& gate) const;
    int pickWinch(glm::vec2 ndc) const;
    void drawGates(unsigned program);
    std::set<std::pair<int, int>> m_gatewayTiles;               // town gate passages (no kerbs there)
    std::set<std::pair<int, int>> m_fortressTiles;              // town wall and gateway footprints (no kerbs there)
    std::vector<glm::vec3> m_stepTops;                          // stair treads (flat, triangles): Sir Canegm climbs them
    size_t m_groundBatch = 0;
    glm::ivec4 m_groundRect{0};                                 // tiles x0, y0, x1, y1 (exclusive)
    std::vector<int> m_tileLayer;                               // ground layer per tile of the rect
    std::vector<bool> m_roadLayer;                              // per ground layer: a street tile
    std::vector<glm::vec3> m_roadColours;                       // U7 street pixels
    std::vector<bool> m_mixedLayer;                             // per ground layer: street stones with dirt
    std::vector<bool> m_grassLayer;
    std::vector<int> m_floorLayer;                              // per ground layer, a house floor: 0 no, 1 boards, 2 flagstones, 3 bricks, 4 carpet
    std::vector<bool> m_grassEdgeLayer;                         // per ground layer: lawn with some earth                             // per ground layer: a lawn tile
    std::vector<glm::vec3> m_grassColours;                      // U7 grass pixels
    unsigned m_grassAlbedo = 0, m_grassNormal = 0, m_mudAlbedo = 0, m_mudNormal = 0;
    std::vector<glm::vec3> m_mudColours;                        // U7 earth pixels of the lawn edges
    unsigned m_cobbleAlbedo = 0, m_cobbleNormal = 0;
    std::set<std::pair<int, int>> m_roadTiles;                  // raised street tiles (Sir Canegm walks on them)
    static constexpr float RoadHeight = 0.09f;                  // street surface: 3 cm below the kerb (0.12 m)
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
    unsigned m_woodAlbedo = 0, m_woodNormal = 0;              // WoodMaterial
    // A shed door: hinge axis at pivot (flat), closed leaf along -x turned by baseYaw; open is
    // turned a further openAngle (radians); angle follows the target.
    // kind 0: door leaf (DoorModel, 4 tiles long); kind 1: a window casement (leaf `length`
    // tiles long, `height` metres high, from the pivot's height).
    struct Door { glm::vec3 pivot{0}; float baseYaw = 0, openAngle = 0, angle = 0; bool open = false;
                  int kind = 0; float length = 4.f, height = U73dScale::DoorHeight; };
    void ensureDoorMeshes();
    ow3d::Mesh m_windowFrame, m_windowGlass;
    unsigned m_windowWoodTexture = 0, m_windowGlassTexture = 0;
    std::set<std::pair<int, int>> m_stoneTiles;                // tiles of stone walls
    std::set<std::pair<int, int>> m_wallFootprint;             // tiles of walls, doors and windows (known before placing)
    std::map<std::pair<int, int>, float> m_wallTop;             // tile -> top of its wall (metres)
    std::vector<Door> m_doors;
    std::vector<U7::WorldObject> m_extraObjects;              // added blood of the stable scene
    ow3d::Mesh m_doorPlanks, m_doorBattens, m_doorIron;
    unsigned m_ironTexture = 0;
    std::vector<glm::vec3> m_doorLeaf;                         // leaf box, local, for collision
    glm::mat4 doorModel(const Door& door) const;
    int pickDoor(glm::vec2 ndc) const;
    void drawDoors(unsigned program);
    // Ultima7Remake's stall and paddock fences (Maps/stable-fences.txt), as KnownGeometry builds them.
    void buildFences();
    // U7's fences in the whole town, rebuilt as the stable's rail fences (two rails, posts).
    void buildTownFences(const U7::Data& data, const std::vector<U7::WorldObject>& fences,
                         const std::vector<std::pair<glm::vec2, glm::vec2>>& runs = {});
    std::vector<std::pair<glm::vec2, glm::vec2>> m_fenceRuns;  // extra rail runs (tiles) from stable-fences.txt
    unsigned railTexture();
    unsigned m_railTexture = 0;
    // Ultima7Remake's loose straw over the stable floor (Maps/stable-straw.txt, Materials/Straw).
    void buildStraw();
    // Thatched gable roof over the shed (ThatchMaterial) in place of U7's flat wood roof.
    void buildThatchRoof();
    // Gable roof over a rectangle (tiles) with its eaves at eave metres: roof faces into roof
    // (uv in metres: along the ridge, down the slope), the gable triangles into gable.
    void addGableRoof(Batch& roof, Batch& gable, float x0, float z0, float x1, float z1, float eave, float pitchDegrees,
                      float overhang, float thick, bool ridgeCap);
    // Slate gable roofs (SlateMaterial) over the stone houses, from U7's flat slate roof pieces.
    void buildSlateRoofs(const std::vector<glm::ivec4>& tiles);
    // High resolution windows (frame, mullion, sill, leaded yellow panes as in U7), stone door
    // frames, and U7's paintings and tapestries as sharp pictures on the inner wall faces.
    // shutters: U7's closed / open shutters, as board shutters outside the nearest window.
    void buildOpenings(const U7::Data& data, const std::vector<U7::WorldObject>& windows, const std::vector<U7::WorldObject>& doors,
                       const std::vector<U7::WorldObject>& shutters);
    void buildPictures(const U7::Data& data, const std::vector<U7::WorldObject>& pictures);
    unsigned m_slateAlbedo = 0, m_slateNormal = 0, m_stoneAlbedo = 0, m_stoneNormal = 0, m_ashlarAlbedo = 0, m_ashlarNormal = 0;
    // The horse sign as a high resolution wrought iron silhouette on its bracket at the shed wall,
    // and the pitchfork in the gargoyle's chest drawn with U7's own pitchfork graphic.
    void buildSignAndFork(const U7::Data& data);
    // 3D trees (TreeModel) in place of U7's tree graphics, each of its own height.
    void buildTrees(const std::vector<glm::vec2>& places);
    // 3D street lamps (LampModel) in place of U7's lamp posts.
    void buildLamps(const std::vector<glm::vec2>& places);
    // 3D wells (WellModel) in place of U7's well and its windlass.
    void buildWells(const std::vector<glm::vec2>& places);
    // Furniture, stones and plants as high resolution models (PropModels): one model per kind
    // and size, shared by all objects of that kind. base: height of the object's lift, metres.
    struct PropPlace { U7::WorldObject object; std::string name; int layer; float base; };
    void buildProps(const U7::Data& data, const std::vector<PropPlace>& props);
    // U7's rugs as woven rugs with fringes (TextileArt), lying on the floor.
    void buildRugs(const U7::Data& data, const std::vector<U7::WorldObject>& rugs);
    // Every placed thing with its box (flat tiles x / z, metres y) and weight, for the tooltip.
    // spans: its vertices (batch, first, count); things under 100 kg can be dragged
    // with the left button, over the floor or into the backpack.
    struct Span { size_t batch, first, count; };
    struct Item { std::string name; glm::vec3 low, high; float kg = 0; std::vector<Span> spans; bool inBag = false;
                  bool movable() const { return kg < 100.f && !spans.empty(); } };
    void putInBag(size_t item);
    void placeItem(size_t item, glm::vec3 flatPoint);
    void uploadItem(size_t item);
    bool m_overlayHovered = false;
    std::string m_bagMessage;
    float m_bagMessageTime = 0;
    std::vector<Item> m_items;
    int pickItem(glm::vec2 ndc) const;
    // Where the mouse ray meets the level y (metres) in the flat world.
    glm::vec3 groundUnder(glm::vec2 ndc, float y) const;
    void moveItem(size_t item, glm::vec2 tiles);
    int m_dragItem = -1;
    bool m_itemGrabbed = false;
    int m_hoverItem = -1, m_hoverDoor = -1, m_hoverGate = -1;     // what the mouse points at (lit up)
    static constexpr float HoverBrighten = 1.45f;             // this press took a thing (no door click on release)
    glm::vec3 m_dragGrab{0};
    // 3D signposts from U7's post (713) with its arrow boards (379); a post or board that does
    // not belong to a signpost stays U7's graphic.
    void buildSignposts(const U7::Data& data, const std::vector<U7::WorldObject>& posts, const std::vector<U7::WorldObject>& signs);
    unsigned m_thatchAlbedo = 0, m_thatchNormal = 0;
    std::unique_ptr<StableSceneProps> m_stableProps;
    std::unique_ptr<StableTools> m_stableTools;
    glm::ivec4 m_stableArea{0};                                // tiles x0, y0, x1, y1 (exclusive)
    std::map<std::pair<int, int>, std::vector<glm::vec3>> m_walls;   // 8 x 8 tile cell -> flat wall triangles
    std::set<std::pair<int, int>> m_roofTiles;
    std::set<std::pair<int, int>> m_solidTiles;                // tiles of walls, doors, windows (trees keep clear)               // tiles under a roof: roofs hide above Sir Canegm
    ow3d::Shader m_shader;
    ow3d::Camera m_camera;
    unsigned m_framebuffer = 0, m_color = 0;                          // resolved image
    unsigned m_msFramebuffer = 0, m_msColor = 0, m_msDepth = 0;       // 4 x multisampled target
    int m_targetWidth = 0, m_targetHeight = 0;
    glm::vec3 m_target{0.f};                                 // free camera target, flat
    float m_yaw = 45.f, m_pitch = 40.f, m_distance = 26.f;   // degrees, metres
    bool m_mouseCaptured = false;
    // Jumping down: the height (flat metres) Sir Canegm falls from, and his vertical speed.
    bool m_falling = false;
    float m_fallHeight = 0.f, m_fallSpeed = 0.f, m_lastHeight = 0.f;
};
