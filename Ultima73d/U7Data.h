#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

// Reader for the original Ultima VII (Black Gate) data files in the STATIC folder.
//
//   PALETTES.FLX  Flex archive; entry 0 is the daylight palette, 256 x RGB with 6-bit values.
//   SHAPES.VGA    Flex archive of shapes. Shapes 0..149 are ground tiles ("flats"): every
//                 frame is 8 x 8 raw palette indices (64 bytes), no header. All other shapes
//                 are run-length encoded frames with a hot spot (see Data::frame).
//   TEXT.FLX      Entry n is the name of shape n.
//   U7CHUNKS      3072 chunks, each 16 x 16 tiles of 2 bytes: shape = b0 | (b1 & 3) << 8,
//                 frame = (b1 >> 2) & 31. Non-flat shapes here are terrain objects
//                 (walls, mountains, rocks ...) standing on that tile.
//   U7MAP         12 x 12 superchunks, each 16 x 16 chunk numbers (uint16): the world is
//                 192 x 192 chunks = 3072 x 3072 tiles of 8 x 8 pixels.
//   U7IFIXnn      One flex per superchunk (nn hex), one entry per chunk: fixed objects of
//                 4 bytes (tile x/y, lift, shape, frame).
//   INITGAME.DAT  Flex whose u7iregnn entries hold the movable objects of each superchunk
//                 (doors, fences, furniture ...), 6/12/18-byte records. A 12-byte container
//                 (TFA class 6) with a type word is followed by its contents, ended by a 1.
//   TFA.DAT       3 bytes per shape; byte 1 & 15 is the shape class, the 3D size in tiles is
//                 x = 1 + (byte 2 & 7), y = 1 + (byte 2 >> 3 & 7), height in lifts z = byte 0 >> 5.
//
// Patched installs keep the untouched files as *.u7org; those are preferred when present.
namespace U7 {

inline constexpr int TilePixels = 8;
inline constexpr int ChunkTiles = 16;
inline constexpr int WorldChunks = 192;
inline constexpr int WorldTiles = WorldChunks * ChunkTiles;   // 3072
inline constexpr int FlatShapes = 150;

struct Rgb { std::uint8_t r, g, b; };

struct TileRef {
    std::uint16_t shape = 0;
    std::uint8_t frame = 0;
};

// A decoded frame: RGBA pixels, and the hot spot inside the image. U7 draws an object with
// its hot spot on the lower right pixel of its tile, moved up and left by 4 px per lift.
struct Frame {
    int width = 0, height = 0, hotX = 0, hotY = 0;
    std::vector<std::uint8_t> rgba;
};

enum class Source { Chunk, Fixed, Movable };

struct WorldObject {
    int x = 0, y = 0, lift = 0;          // tile position (0..3071) and height level
    std::uint16_t shape = 0;
    std::uint8_t frame = 0;
    Source source = Source::Chunk;
};

class Data {
public:
    // Loads palette, shapes, names, chunks, map and object lists from a U7 STATIC directory.
    void load(const std::filesystem::path& staticDirectory);

    const std::array<Rgb, 256>& palette() const { return m_palette; }
    // Flat ground tile images (8 x 8 palette indices), addressed by (shape, frame).
    int flatFrameCount(int shape) const;
    const std::uint8_t* flatFrame(int shape, int frame) const;   // nullptr when absent
    // Any shape frame, decoded to RGBA (empty Frame when absent).
    Frame frame(int shape, int frame) const;
    const std::string& name(int shape) const;
    // Ground tile at a world tile position (0..3071).
    TileRef tile(int x, int y) const;
    // Terrain objects in the chunks, fixed objects and movable objects of the whole world.
    const std::vector<WorldObject>& objects() const { return m_objects; }

    std::filesystem::path shapesFile, paletteFile, chunksFile, mapFile, textFile, tfaFile;
    int shapeClass(int shape) const { return shape >= 0 && shape < int(m_classes.size()) ? m_classes[shape] : 0; }
    // 3D extent of a shape: tiles along x (west) and y (north) from its anchor tile, lifts up.
    struct Size { int x = 1, y = 1, z = 0; };
    Size shapeSize(int shape) const { return shape >= 0 && shape < int(m_sizes.size()) ? m_sizes[shape] : Size{}; }

private:
    std::array<Rgb, 256> m_palette{};
    std::vector<std::uint8_t> m_shapes;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> m_shapeEntries;
    std::vector<std::string> m_names;
    std::vector<std::uint8_t> m_classes;                          // TFA shape class per shape
    std::vector<Size> m_sizes;                                    // TFA 3D size per shape
    std::vector<std::uint8_t> m_chunks;                           // 3072 x 512 bytes
    std::vector<std::uint16_t> m_map;                             // chunk number per world chunk (192 x 192)
    std::vector<WorldObject> m_objects;
    void readObjects(const std::filesystem::path& directory);
};

}
