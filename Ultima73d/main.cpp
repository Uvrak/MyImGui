// Ultima73d: loads the original Ultima VII world into a GridBuilderGrid instance.
//
//   Layer 0  ground tiles (flats)
//   Layer 1  walls, doors, windows, town walls, rock walls ... (sprite layer, can be hidden)
//   Layer 3  other terrain objects standing in the chunks: rocks, weeds, bushes, trees ...
//   Britannia3d  3D view of one chunk (see Britannia3dView).
//
//   Ultima73d.exe [U7 STATIC directory]
//   Ultima73d.exe --export <out.bmp> <x0> <y0> <width> <height> [U7 STATIC directory]
//       writes a tile rectangle with all layers as a bitmap (no window), for checks.
//   Ultima73d.exe --export3d <out.bmp> [--no-layers] [--no-roofs] [--globe] [--canegm] [--walk]
//                 [--close] [--at <tile x> <tile y>] [--toggle-doors] [--look <tile x> <tile y> <metres> <yaw> <pitch>]
//       renders the Britannia3d view of Trinsic as a bitmap, for checks: from above, the whole
//       globe, or Sir Canegm's camera (after walking 3 s north with --walk; close up; placed).
#include "Britannia3dView.h"
#include "PropModels.h"
#include <set>
#include <chrono>
#include "Inventory/BackpackPanel.h"
#include "U7Data.h"
#include "U7GroundGrid.h"
#include "U7ObjectLayer.h"
#include <GridBuilderGrid.h>
#include <MyImGui.h>
#include <imgui.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <algorithm>
#include <vector>
#include <cstring>
#include <iostream>
#include <string>

namespace {

const char* DefaultStatic = "C:/GOG Galaxy/Games/Ultima 7/STATIC";
// All assets live outside the repository: model files per object graphic (Objects, see
// U7ObjectModel), materials, the stable scene and the characters.
const char* AssetDirectory = "C:/Projects/U73dAssets";
// Positions and other text data (placements, shaders) stay in the repository.
const char* DataDirectory = U73D_PROJECT_DIR "data";

// Structures, plus raised floors such as the walkways on top of the town walls.
bool layerOne(const U7::WorldObject& object, const std::string& name) {
    return U7ObjectLayer::isStructure(name) || (name == "floor" && object.lift > 0);
}
bool layerThree(const U7::WorldObject& object, const std::string& name) {
    return object.source == U7::Source::Chunk && !U7ObjectLayer::isStructure(name);
}

// The movable things (neither structure nor part of the terrain), split for the U7 grid view:
// furniture, and all the smaller things. Markers the game never shows are left out.
bool movableThing(const U7::WorldObject& object, const std::string& name) {
    return !layerOne(object, name) && !layerThree(object, name) && name.find("roof") == std::string::npos && name != "egg" && name != "path" &&
           name != "light source";
}
// Roofs: a layer of their own in the grid (in the 3D view the roofs switch).
// (U7's roofs are fixed objects of the map, which the grid otherwise puts into layer 3.)
bool roof(const U7::WorldObject&, const std::string& name) {
    return name.find("roof") != std::string::npos;
}
bool terrainNoRoof(const U7::WorldObject& object, const std::string& name) {
    return layerThree(object, name) && !roof(object, name);
}
bool structureNoRoof(const U7::WorldObject& object, const std::string& name) {
    return layerOne(object, name) && !roof(object, name);
}
// The same rule as Britannia3d (PropModels::layerOf): 2 furniture, 4 things one can drag,
// 6 everything else (figures, blood, tracks, heavy or fixed things).
bool furniture(const U7::WorldObject& object, const std::string& name) {
    return movableThing(object, name) && PropModels::layerOf(PropModels::kindOf(name)) == 2;
}
bool smallThing(const U7::WorldObject& object, const std::string& name) {
    return movableThing(object, name) && PropModels::layerOf(PropModels::kindOf(name)) == 4;
}
bool miscThing(const U7::WorldObject& object, const std::string& name) {
    return movableThing(object, name) && PropModels::layerOf(PropModels::kindOf(name)) == 6;
}

// Paints one decoded frame into an RGBA image, hot spot at (hx, hy), with alpha.
void blit(SDL_Surface* image, const U7::Frame& frame, int hx, int hy) {
    for (int y = 0; y < frame.height; ++y)
        for (int x = 0; x < frame.width; ++x) {
            const auto* in = frame.rgba.data() + (size_t(y) * frame.width + x) * 4;
            if (!in[3]) continue;
            const int px = hx - frame.hotX + x, py = hy - frame.hotY + y;
            if (px < 0 || py < 0 || px >= image->w || py >= image->h) continue;
            std::memcpy(static_cast<std::uint8_t*>(image->pixels) + size_t(py) * image->pitch + size_t(px) * 4, in, 4);
        }
}

// Layer of an object in the 3D view (1, 3, or 2 for everything else).
int layerOf(const U7::WorldObject& object, const std::string& name) {
    return layerOne(object, name) ? 1 : layerThree(object, name) ? 3 : 2;
}

bool listing = false;   // --list: print the exported objects

int exportRegion(const U7::Data& data, const char* file, int x0, int y0, int width, int height) {
    constexpr int p = U7::TilePixels;
    auto* image = SDL_CreateSurface(width * p, height * p, SDL_PIXELFORMAT_RGBA32);
    if (!image) throw std::runtime_error(SDL_GetError());
    for (int ty = 0; ty < height; ++ty)
        for (int tx = 0; tx < width; ++tx) {
            const auto t = data.tile(x0 + tx, y0 + ty);
            auto pixels = U7GroundGrid::rgba(data, t.shape, t.frame);
            for (int j = 0; j < p; ++j)
                std::memcpy(static_cast<std::uint8_t*>(image->pixels) + (ty * p + j) * image->pitch + tx * p * 4,
                            pixels.data() + j * p * 4, p * 4);
        }
    // Objects of layers 1 and 3 near the rectangle, back to front.
    std::vector<const U7::WorldObject*> objects;
    for (const auto& o : data.objects()) {
        if (o.x < x0 - 8 || o.y < y0 - 8 || o.x >= x0 + width + 8 || o.y >= y0 + height + 8) continue;
        std::string name = data.name(o.shape);
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        if (layerOne(o, name) || layerThree(o, name)) objects.push_back(&o);
        if (listing) {
            const auto f = data.frame(o.shape, o.frame);
            std::cout << int(o.source) << ' ' << o.x << ',' << o.y << " lift " << o.lift << ' ' << o.shape << ':' << int(o.frame)
                      << ' ' << name << ' ' << f.width << 'x' << f.height << (layerOne(o, name) || layerThree(o, name) ? " drawn" : "") << '\n';
        }
    }
    std::stable_sort(objects.begin(), objects.end(), [](auto a, auto b) {
        return a->lift != b->lift ? a->lift < b->lift : a->x + a->y < b->x + b->y;
    });
    for (const auto* o : objects) {
        const auto frame = data.frame(o->shape, o->frame);
        if (frame.rgba.empty()) continue;
        blit(image, frame, (o->x - x0 + 1) * p - 1 - 4 * o->lift, (o->y - y0 + 1) * p - 1 - 4 * o->lift);
    }
    const bool saved = SDL_SaveBMP(image, file);
    SDL_DestroySurface(image);
    if (!saved) throw std::runtime_error(SDL_GetError());
    std::cout << "Exported tiles " << x0 << ',' << y0 << " (" << width << " x " << height << ") to " << file << '\n';
    return 0;
}

// Trinsic with its town walls, the fields around it and the docks in the east.
constexpr int TrinsicChunkX0 = 57, TrinsicChunkY0 = 129, TrinsicChunkX1 = 70, TrinsicChunkY1 = 146;
// Sir Canegm from Ultima7Remake, on the street in front of the stable (its door faces south).
const std::string CanegmFolder = std::string(AssetDirectory) + "/Characters/SirCanegm";
constexpr float CanegmTileX = 1068.5f, CanegmTileY = 2213.f;

SDL_Window* createWindow(const char* title, int width, int height, SDL_WindowFlags flags, SDL_GLContext& context) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow(title, width, height, SDL_WINDOW_OPENGL | flags);
    if (!window) throw std::runtime_error(SDL_GetError());
    context = SDL_GL_CreateContext(window);
    if (!context) throw std::runtime_error(SDL_GetError());
    SDL_GL_MakeCurrent(window, context);
    if (!Britannia3dView::loadOpenGl()) throw std::runtime_error("OpenGL functions could not be loaded");
    return window;
}

int export3d(const U7::Data& data, const char* file, const std::vector<std::string>& options) {
    auto has = [&](const char* option) { return std::find(options.begin(), options.end(), option) != options.end(); };
    const int x0 = TrinsicChunkX0, y0 = TrinsicChunkY0, x1 = TrinsicChunkX1, y1 = TrinsicChunkY1;
    if (!SDL_Init(SDL_INIT_VIDEO)) throw std::runtime_error(SDL_GetError());
    SDL_GLContext context = nullptr;
    SDL_Window* window = createWindow("Britannia3d", 64, 64, SDL_WINDOW_HIDDEN, context);
    int result = 0;
    {
        Britannia3dView view;
        view.assetDirectory = AssetDirectory;
        view.stableSceneDirectory = AssetDirectory;
        view.dataDirectory = DataDirectory;
        if (has("--world-ground")) view.prepareWorldGround(data);   // once: HD ground tiles for the whole world
        view.build(data, x0, y0, x1, y1, layerOf, "Trinsic");
        view.roofsVisible = !has("--no-roofs");
        if (has("--toggle-doors"))
            for (size_t d = 0; d < view.doorCount(); ++d) view.toggleDoor(d, true);
        if (has("--open-gates"))
            for (size_t g = 0; g < view.gateCount(); ++g) view.operateGate(g, true);
        if (has("--locked-gates")) {
            for (size_t g = 0; g < view.gateCount(); ++g) view.setGateLocked(g, true);
            size_t opened = 0;
            for (size_t g = 0; g < view.gateCount(); ++g) opened += view.operateGate(g, true);
            std::cout << "Locked gates opened: " << opened << " of " << view.gateCount() << '\n';
        }
        std::cout << "Doors: " << view.doorCount() << '\n';
        if (const auto* props = view.stableProps())
            for (const auto& e : props->extents())
                if (e.file.find("Gargoyle") != std::string::npos)
                    std::cout << e.file << " at " << e.x << ',' << e.y << ": east " << e.low.x << ".." << e.high.x << ", up " << e.low.y
                              << ".." << e.high.y << ", north " << e.low.z << ".." << e.high.z << " m\n";
        view.layerVisible[1] = view.layerVisible[3] = !has("--no-layers");
        if (const auto look = std::find(options.begin(), options.end(), "--look"); look != options.end() && options.end() - look >= 6)
            view.setFreeCamera(std::stof(look[1]), std::stof(look[2]), std::stof(look[3]), std::stof(look[4]), std::stof(look[5]));
        if (has("--globe")) view.setFreeCamera((x0 + x1 + 1) * 8.f, (y0 + y1 + 1) * 8.f, 5200.f, 20.f, 55.f);
        if (has("--canegm")) {
            float tileX = CanegmTileX, tileY = CanegmTileY;
            if (const auto at = std::find(options.begin(), options.end(), "--at"); at != options.end() && options.end() - at >= 3) {
                tileX = std::stof(at[1]); tileY = std::stof(at[2]);
            }
            if (!view.loadCharacter(CanegmFolder, tileX, tileY)) throw std::runtime_error("Sir Canegm could not be loaded");
            if (has("--close")) view.character.setCameraDistance(.07f);
            if (has("--wear")) {
                // Times putting clothes on (outfit rebuild), e.g. the trousers.
                view.character.outfit.load(CanegmFolder);
                for (const unsigned mask : {4u, 32u, 64u, 128u, 256u, 512u, 2u + 32u + 8u + 16u}) {
                    const auto start = std::chrono::steady_clock::now();
                    view.character.outfit.setWorn(mask);
                    std::cout << "Wear mask " << mask << ": " << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << " s\n";
                }
            }
            const auto start = view.character.position();
            if (has("--walk")) for (int i = 0; i < 180; ++i) view.step(1.f / 60.f, true);
            if (has("--climb")) {
                // Walk north 8 s and report the height reached (stairs up the town wall).
                float highest = 0;
                for (int i = 0; i < 480; ++i) { view.step(1.f / 60.f, true); highest = (std::max)(highest, view.flat(view.character.position()).y); }
                std::cout << "Climb: highest " << highest << " m, now " << view.flat(view.character.position()).y << " m" << std::endl;
            }
            for (int i = 0; i < 30; ++i) view.step(1.f / 60.f, false);
            const auto at = view.flat(view.character.position());
            std::cout << "Sir Canegm at tile " << at.x << ',' << at.z << ", walked " << glm::length(view.character.position() - start) / Britannia3dView::Metre << " m\n";
        }
        constexpr int width = 1200, height = 900;
        auto pixels = view.renderImage(width, height);
        auto* image = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, pixels.data(), width * 4);
        if (!image || !SDL_SaveBMP(image, file)) result = 1;
        SDL_DestroySurface(image);
        std::cout << "Britannia3d chunks " << x0 << ',' << y0 << " - " << x1 << ',' << y1 << ": " << view.boxCount() << " boxes, "
                  << view.quadObjectCount() << " upright objects, " << view.modelCount() << " model files ("
                  << view.createdFileCount() << " new), stable scene " << view.stablePropCount() << " props -> " << file << '\n';
    }
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}

}

int main(int argc, char** argv) {
    try {
        if (argc >= 2 && std::string(argv[argc - 1]) == "--list") { listing = true; --argc; }
        if (argc >= 3 && std::string(argv[1]) == "--export3d") {
            U7::Data data;
            data.load(DefaultStatic);
            return export3d(data, argv[2], std::vector<std::string>(argv + 3, argv + argc));
        }
        const bool exporting = argc >= 7 && std::string(argv[1]) == "--export";
        const std::string staticDirectory = exporting ? (argc >= 8 ? argv[7] : DefaultStatic) : (argc >= 2 ? argv[1] : DefaultStatic);
        U7::Data data;
        data.load(staticDirectory);
        std::cout << "U7 data: " << data.shapesFile.string() << ", " << data.objects().size() << " objects" << std::endl;
        if (exporting)
            return exportRegion(data, argv[2], std::stoi(argv[3]), std::stoi(argv[4]), std::stoi(argv[5]), std::stoi(argv[6]));

        if (!SDL_Init(SDL_INIT_VIDEO)) throw std::runtime_error(SDL_GetError());
        SDL_GLContext context = nullptr;
        SDL_Window* window = createWindow("Ultima73d", 1600, 1000, SDL_WINDOW_RESIZABLE, context);
        SDL_GL_SetSwapInterval(1);
        if (!MyImGui::initialize(window)) throw std::runtime_error("ImGui initialisation failed");

        {
            U7GroundGrid ground;
            U7ObjectLayer structures, terrain, furnishings, things, roofs, misc;
            GridBuilderGrid grid(nullptr, U7::ChunkTiles);
            grid.setGroundLayer(ground.build(data), "U7 Original");
            const auto structureLayer = grid.addSpriteLayer(structures.build(data, "Ebene 1: Waende, Tueren, Fenster, Zaeune, Mauern, Fels", structureNoRoof));
            const auto terrainLayer = grid.addSpriteLayer(terrain.build(data, "Ebene 3: Felsen, Pflanzen, Baeume", terrainNoRoof));
            const auto furnitureLayer = grid.addSpriteLayer(furnishings.build(data, "Ebene 2: Moebel", furniture));
            const auto thingsLayer = grid.addSpriteLayer(things.build(data, "Ebene 4: Gegenstaende", smallThing));
            const auto roofLayer = grid.addSpriteLayer(roofs.build(data, "Ebene 5: Daecher", roof));
            const auto miscLayer = grid.addSpriteLayer(misc.build(data, "Ebene 6: Sonstiges (Figuren, Spuren, Blut ...)", miscThing));
            std::cout << "Layer 5 (roofs): " << grid.spriteLayer(roofLayer).sprites.size() << " objects" << std::endl;
            std::cout << "Layer 0: " << ground.materialCount() << " ground tiles; layer 1: "
                      << grid.spriteLayer(structureLayer).sprites.size() << " objects; layer 3: "
                      << grid.spriteLayer(terrainLayer).sprites.size() << " objects" << std::endl;
            grid.focusGroundCell(1056, 2194);            // Trinsic stable
            Britannia3dView britannia3d;
            britannia3d.assetDirectory = AssetDirectory;
            britannia3d.stableSceneDirectory = AssetDirectory;
            britannia3d.dataDirectory = DataDirectory;
            britannia3d.prepareWorldGround(data);         // once: HD tiles for the whole world
            britannia3d.build(data, TrinsicChunkX0, TrinsicChunkY0, TrinsicChunkX1, TrinsicChunkY1, layerOf, "Trinsic");
            if (!britannia3d.loadCharacter(CanegmFolder, CanegmTileX, CanegmTileY))
                std::cerr << "Sir Canegm not found in " << CanegmFolder << '\n';
            // Ultima7Remake's backpack (B): equipment, compass, portrait; its state in
            // U73dAssets/settings/Canegm. Things picked up in Trinsic go into it too.
            struct Scene : BackpackScene {
                Britannia3dView& view;
                explicit Scene(Britannia3dView& v) : view(v) {}
                ow3d::AnimatedCharacter& character() override { return view.character; }
                ow3d::Camera& camera() override { return view.camera(); }
                bool pickGround(float x, float y, glm::vec3& ground) const override { return view.pickGround(x, y, ground); }
            } scene(britannia3d);
            BackpackPanel backpack;
            bool backpackReady = false;
            try {
                const std::filesystem::path assets = AssetDirectory, settings = assets / "settings/Canegm";
                std::filesystem::create_directories(settings);
                backpack.load(assets / "Backpack", settings);
                if (britannia3d.character.loaded()) {
                    britannia3d.character.outfit.load(CanegmFolder);
                    backpack.attach(scene);
                }
                backpack.extraItems = [&](ImVec2 p, float size) { britannia3d.drawBagItems(p, size); };
                britannia3d.equipmentKg = [&] { return backpack.carriedKg(); };
                // Clothing found in Trinsic is Sir Canegm's own equipment when he puts it on.
                auto equipmentOf = [&](int id) -> int {
                    const auto& kind = britannia3d.itemKind(size_t(id));
                    if (kind == "leather helm" || kind == "crested helm") return 0;          // helmet
                    if (kind == "leather armour") return 1;                                  // armor
                    if (kind == "leather leggings") return 2;                                // trousers
                    if (kind == "boots" || kind == "swamp boots") return 3;                  // boots
                    return -1;
                };
                backpack.wearableEquipment = equipmentOf;
                backpack.wornFromWorld = [&](int id) { britannia3d.wearItem(size_t(id)); };
                backpack.groundIcons = false;
                britannia3d.equipmentOnGround = [&](int e) { return backpack.equipmentOnGround(e); };
                britannia3d.equipmentToBag = [&](int e) { backpack.putEquipmentInBag(e); };
                britannia3d.equipmentDrop = [&](int e, glm::vec3 point) { backpack.dropEquipment(e, point); };
                britannia3d.placeGroundEquipment([&](int e) { return backpack.groundPoint(e); });
                britannia3d.dropOnCharacter = [&](size_t id, ImVec2 mouse) {
                    const int own = britannia3d.itemEquipment(id);
                    const int item = own >= 0 ? own : equipmentOf(int(id));
                    if (item < 0 || !backpack.overCharacterAt(mouse)) return false;
                    backpack.setEquipment(item, true);
                    return true;
                };
                britannia3d.overlay = [&](ImVec2 a, ImVec2 b) { return backpack.draw(a, b); };
                britannia3d.bagPanel = [&] { return backpack.panel; };
                britannia3d.overBag = [&](ImVec2 m) {
                    const auto [p, size] = backpack.panel;
                    return size > 0 && m.x >= p.x && m.x <= p.x + size && m.y >= p.y && m.y <= p.y + size;
                };
                backpackReady = true;
            } catch (const std::exception& e) {
                std::cerr << "Backpack: " << e.what() << '\n';
            }
            bool open = true, running = true, britannia3dOpen = true;
            while (running) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    MyImGui::processEvent(event);
                    if (event.type == SDL_EVENT_QUIT) running = false;
                }
                int width = 0, height = 0;
                SDL_GetWindowSizeInPixels(window, &width, &height);
                glViewport(0, 0, width, height);
                glClearColor(.08f, .08f, .1f, 1);
                glClear(GL_COLOR_BUFFER_BIT);
                MyImGui::beginFrame();
                MyImGui::beginDockspace();
                grid.draw(&open);
                // Layers 1 and 3 are one switch each for the grid and the 3D view: a change in
                // either place is applied to the other.
                // (The same numbers in both: 1 structures, 2 furniture, 3 terrain, 4 things.)
                const std::pair<std::size_t, int> shared[] = {{structureLayer, 1}, {terrainLayer, 3}, {furnitureLayer, 2}, {thingsLayer, 4}, {miscLayer, 6}};
                bool before[5];
                for (int i = 0; i < 5; ++i) before[i] = britannia3d.layerVisible[size_t(shared[i].second)];
                const bool roofsBefore = britannia3d.roofsVisible;
                if (britannia3dOpen) britannia3d.draw(&britannia3dOpen);
                // Switching between the U7 grid and Britannia3d keeps the part of the map shown.
                {
                    static bool shown3d = false, first = true;
                    const bool now3d = britannia3dOpen && britannia3d.visible;
                    if (!first && now3d != shown3d) {
                        if (now3d) {
                            const auto view = grid.groundView();
                            // Outside the built area: the area (Trinsic's size) is built anew there,
                            // Sir Canegm stands in its middle.
                            const auto region = britannia3d.chunkRegion();
                            const int cx = int(view.centerX) / U7::ChunkTiles, cy = int(view.centerY) / U7::ChunkTiles;
                            if (cx < region.x || cx > region.z || cy < region.y || cy > region.w) {
                                const int w = region.z - region.x + 1, h = region.w - region.y + 1;
                                const int x0 = std::clamp(cx - w / 2, 0, U7::WorldChunks - w), y0 = std::clamp(cy - h / 2, 0, U7::WorldChunks - h);
                                britannia3d.build(data, x0, y0, x0 + w - 1, y0 + h - 1, layerOf, "Britannia");
                                if (britannia3d.character.loaded())
                                    britannia3d.character.setPosition(britannia3d.planet(glm::vec3(view.centerX, 0, view.centerY)));
                                if (backpackReady) britannia3d.placeGroundEquipment([&](int e) { return backpack.groundPoint(e); });
                            }
                            britannia3d.showArea(view.centerX, view.centerY, view.visibleHeight);
                        } else {
                            float x, y, high;
                            britannia3d.shownArea(x, y, high);
                            auto view = grid.groundView();
                            view.centerX = x; view.centerY = y; view.visibleHeight = high; view.visibleWidth = 0;
                            grid.setGroundView(view);
                        }
                    }
                    shown3d = now3d;
                    first = false;
                }
                if (britannia3d.roofsVisible != roofsBefore) grid.setSpriteLayerVisible(roofLayer, britannia3d.roofsVisible);
                for (int i = 0; i < 5; ++i)
                    if (britannia3d.layerVisible[size_t(shared[i].second)] != before[i])
                        grid.setSpriteLayerVisible(shared[i].first, britannia3d.layerVisible[size_t(shared[i].second)]);
                if (ImGui::Begin("Ebenen", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::TextUnformatted("Ebene 0: Bodentiles");
                    for (std::size_t i = 0; i < grid.spriteLayerCount(); ++i) {
                        bool visible = grid.spriteLayer(i).visible;
                        if (ImGui::Checkbox(grid.spriteLayer(i).name.c_str(), &visible)) {
                            grid.setSpriteLayerVisible(i, visible);
                            for (const auto& [layer, view] : shared)
                                if (layer == i) britannia3d.layerVisible[size_t(view)] = visible;
                            if (i == roofLayer) britannia3d.roofsVisible = visible;
                        }
                    }
                    ImGui::Separator();
                    ImGui::Checkbox("Britannia3d", &britannia3dOpen);
                }
                ImGui::End();
                MyImGui::endFrame();
                SDL_GL_SwapWindow(window);
                if (!open) running = false;
            }
            if (backpackReady) backpack.saveNow();
        }
        MyImGui::shutdown();
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Ultima73d: " << e.what() << '\n';
        return 1;
    }
}
