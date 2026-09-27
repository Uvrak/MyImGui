#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

// Procedural 3D deciduous tree for Britannia3d, replacing U7's flat tree graphics: a tapered
// trunk with recursively forking branches (bark), and clusters of leaf cards at the branch tips
// (autumn foliage in the colours of U7's trees). Every tree differs: the seed chooses its
// height, branching and lean. Units: metres, trunk base at the origin, y up.
//
// The view animates the tree in the wind: the higher a vertex, the more it sways.
namespace TreeModel {

struct Vertex { glm::vec3 position, normal; glm::vec2 uv; };

struct Tree {
    float height = 0;
    std::vector<Vertex> bark, leaves;
};

Tree build(std::uint32_t seed, float minimumHeight, float maximumHeight);

// Textures (RGBA, square): bark with furrows; a leaf cluster card with transparent gaps.
std::vector<std::uint8_t> barkTexture(int size);
std::vector<std::uint8_t> leafTexture(int size);

}
