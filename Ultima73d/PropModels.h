#pragma once
#include <glm/glm.hpp>
#include <vector>

// High resolution models for Trinsic's furniture, stones and plants, in U7's style. Every
// model is built once per kind and size and then placed at all objects of that kind.
//
// Local space: metres, y up; furniture stands on its footprint from (-width, 0, -depth) to
// (0, 0, 0) (the U7 anchor is the south east corner); stones and plants are centred at the
// origin. Parts are grouped by material, so a model draws with a few shared textures.
namespace PropModels {

struct Vertex { glm::vec3 position, normal; glm::vec2 uv; };

enum Material { Wood, DarkWood, Iron, Cloth, Linen, Stone, Leaves, Needles, Blades, Clay, MaterialCount };

struct Model {
    std::vector<Vertex> parts[MaterialCount];
};

// Furniture. width / depth: footprint in metres (x, z), height: metres.
Model table(float width, float depth, float height);
Model bench(float width, float depth, float height, bool alongX);
// A chair whose back is on the side `back` (0 north, 1 east, 2 south, 3 west).
Model chair(float size, float height, int back);
// A bed along its longer side, the head at the north (alongX = false) or west end.
Model bed(float width, float depth, float height, bool alongX);
Model crate(float width, float depth, float height);
Model chest(float width, float depth, float height);
// A writing desk (a table with a drawer box) and a chest of drawers.
Model desk(float width, float depth, float height);
Model drawers(float width, float depth, float height);

// Nature (seed: every stone and plant differs a little).
Model rock(float size, unsigned seed);
Model weeds(float size, unsigned seed);
Model bush(float size, unsigned seed);
// A bush in a clay pot (U7's indoor plant).
Model pottedPlant(float size, unsigned seed);
Model evergreen(float height, unsigned seed);

}
