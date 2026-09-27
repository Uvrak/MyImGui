#pragma once
#include "U7Data.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

// 3D model of one U7 object graphic (shape, frame), stored as its own file:
//
//   <asset dir>/Objects/<shape>_<name>/frame<NN>.obj   triangles, metres, Y up, Z south
//                                      frame<NN>.mtl   material, map_Kd = the texture
//                                      frame<NN>.png   texture (the original U7 frame)
//
// The origin is the object's anchor: the south east bottom corner of its footprint at its lift.
// A missing file is generated from the U7 frame and written; an existing file is loaded as it
// is, so every model can be replaced or edited (e.g. in Blender) on its own.
//
// Generated models: 1 tile = 1 m, 1 lift = 0.5 m. U7 draws objects as a parallel projection
// seen from the south east: a point (x, y) in tiles and h in metres lands on frame pixel
// (8x - 8h, 8y - 8h). Frames that fit the box given by their TFA size become boxes whose faces
// take their pixels straight from the frame through that projection (walls, fences ...), the
// hidden north and west faces repeat the south and east faces. Frames reaching far outside
// their box (trees, bushes ...) become two crossed upright quads, flat objects a ground quad.
class U7ObjectModel {
public:
    enum class Kind { Flat, Box, Upright, File };
    struct Vertex { glm::vec3 position, normal; glm::vec2 uv; };   // uv: top-left origin

    Kind kind = Kind::File;
    std::vector<Vertex> vertices;                  // triangle list
    int textureWidth = 0, textureHeight = 0;
    std::vector<std::uint8_t> texture;             // RGBA, top row first

    bool empty() const { return vertices.empty() || texture.empty(); }

    static U7ObjectModel generate(const U7::Data& data, int shape, int frame);
    // Model file of a graphic below an asset directory.
    static std::filesystem::path file(const std::filesystem::path& assetDirectory, const U7::Data& data, int shape, int frame);
    // Loads the model file, or generates and writes it when it does not exist yet.
    static U7ObjectModel loadOrCreate(const std::filesystem::path& assetDirectory, const U7::Data& data, int shape, int frame,
                                      bool* created = nullptr);

    bool save(const std::filesystem::path& objFile) const;
    static U7ObjectModel load(const std::filesystem::path& objFile);
};
