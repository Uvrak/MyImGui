#pragma once
#include <glm/glm.hpp>
#include <vector>

// High resolution barn door for Britannia3d's wooden sheds, built in code: a leaf of upright
// planks, two cross battens and a Z brace on the inner side, iron strap hinges, the hinge pin
// and a ring pull on the outer side.
//
// Local space (tiles horizontally, metres up, like the flat world): the hinge axis is the
// y axis at x = z = 0; the closed leaf runs along -x (length tiles), its outer side faces +z.
namespace DoorModel {

struct Vertex { glm::vec3 position, normal; };

struct Parts {
    std::vector<Vertex> planks;    // leaf: upright boards (plank material, vertical)
    std::vector<Vertex> battens;   // battens and brace (plank material, horizontal)
    std::vector<Vertex> iron;      // hinges, pin, ring pull
    std::vector<glm::vec3> leaf;   // the leaf's outline box as triangles, for collision
};

// length: leaf length in tiles; height: metres; tileMetres: metres per tile.
Parts build(float length, float height, float tileMetres);

}
