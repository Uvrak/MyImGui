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
            U7ObjectLayer structures, terrain;
            GridBuilderGrid grid(nullptr, U7::ChunkTiles);
            grid.setGroundLayer(ground.build(data), "U7 Original");
            const auto structureLayer = grid.addSpriteLayer(structures.build(data, "Ebene 1: Waende, Tueren, Fenster, Mauern, Fels", layerOne));
            const auto terrainLayer = grid.addSpriteLayer(terrain.build(data, "Ebene 3: Felsen, Pflanzen, Baeume", layerThree));
            std::cout << "Layer 0: " << ground.materialCount() << " ground tiles; layer 1: "
                      << grid.spriteLayer(structureLayer).sprites.size() << " objects; layer 3: "
                      << grid.spriteLayer(terrainLayer).sprites.size() << " objects" << std::endl;
            grid.focusGroundCell(1056, 2194);            // Trinsic stable
            Britannia3dView britannia3d;
            britannia3d.assetDirectory = AssetDirectory;
            britannia3d.stableSceneDirectory = AssetDirectory;
            britannia3d.dataDirectory = DataDirectory;
            britannia3d.build(data, TrinsicChunkX0, TrinsicChunkY0, TrinsicChunkX1, TrinsicChunkY1, layerOf, "Trinsic");
            if (!britannia3d.loadCharacter(CanegmFolder, CanegmTileX, CanegmTileY))
                std::cerr << "Sir Canegm not found in " << CanegmFolder << '\n';
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
                const std::pair<std::size_t, int> shared[] = {{structureLayer, 1}, {terrainLayer, 3}};
                bool before[2];
                for (int i = 0; i < 2; ++i) before[i] = britannia3d.layerVisible[size_t(shared[i].second)];
                if (britannia3dOpen) britannia3d.draw(&britannia3dOpen);
                for (int i = 0; i < 2; ++i)
                    if (britannia3d.layerVisible[size_t(shared[i].second)] != before[i])
                        grid.setSpriteLayerVisible(shared[i].first, britannia3d.layerVisible[size_t(shared[i].second)]);
                if (ImGui::Begin("Ebenen")) {
                    ImGui::TextUnformatted("Ebene 0: Bodentiles");
                    for (std::size_t i = 0; i < grid.spriteLayerCount(); ++i) {
                        bool visible = grid.spriteLayer(i).visible;
                        if (ImGui::Checkbox(grid.spriteLayer(i).name.c_str(), &visible)) {
                            grid.setSpriteLayerVisible(i, visible);
                            for (const auto& [layer, view] : shared)
                                if (layer == i) britannia3d.layerVisible[size_t(view)] = visible;
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
