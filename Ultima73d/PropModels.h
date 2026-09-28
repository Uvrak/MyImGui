#pragma once
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// High resolution models for Trinsic's furniture, stones and plants, in U7's style. Every
// model is built once per kind and size and then placed at all objects of that kind.
//
// Local space: metres, y up; furniture stands on its footprint from (-width, 0, -depth) to
// (0, 0, 0) (the U7 anchor is the south east corner); stones and plants are centred at the
// origin. Parts are grouped by material, so a model draws with a few shared textures.
namespace PropModels {

struct Vertex { glm::vec3 position, normal; glm::vec2 uv; };

enum Material { Wood, DarkWood, Iron, Cloth, Linen, Stone, Leaves, Needles, Blades, Clay, Pewter, Glass, Leather, Burlap, Flame, Straw, Steel, Gold, Water, Food, Mirror, MaterialCount };

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

// Small things, centred at the origin on the surface they stand on (a sconce: on the wall at
// z = 0, facing +z).
Model candle(bool lit, float stand);
Model sconce(bool lit);
Model cup();
Model plate();
Model pitcher();
Model bottle(unsigned seed);
Model book(unsigned seed);
Model scroll();
Model bag(float size, unsigned seed);
Model bucket();
Model pot();
Model boots();
Model horseshoe();
Model pillar(float width, float depth, float height);
Model haystack(float width, float depth, float height, unsigned seed);
// Weapons and armour lying flat along x; hafted heads: 0 mace, 1 morning star, 2 club,
// 3 hammer, 4 two handed axe.
Model blade(float length, bool hilt);
Model hafted(float length, int head);
Model tongs();
Model shield(float radius, bool wooden);
Model helm(bool leather);
Model armour();
Model gloves();
Model clothHeap(float size, unsigned seed);
Model bread(unsigned seed);
Model potion(unsigned seed);
Model shards();
Model inkwell();
Model coins();
Model utensils();
Model top();
// Larger things, built along x, centred on their footprint.
Model anvil();
Model stove(float width, float depth, float height);
Model firepit(float size);
Model easel(bool withPalette);
Model mirror();
Model sundial();
Model podium();
Model pedestal();
Model trough(float length, float width);
Model lever();
Model ironBars(float length, float height);
Model flag(unsigned seed);
Model basket();
Model bellows();
// Tools (0 rake, 1 shovel, 2 pitchfork), trinkets, the Fellowship's staff and icon, a statue,
// a chimney, a puddle, a painter's palette, a body under a cloth.
Model tool(int kind);
Model key();
Model amulet();
Model staff(bool icon);
Model statue(float height);
Model chimney(float width, float depth, float height);
Model pool(float width, float depth);
Model palette();
Model body();
// Landscape and more (see PropModels.cpp).
Model mountain(float width, float depth, float height, unsigned seed, bool dark);
Model crops(float width, float depth, unsigned seed);
Model reeds(float size, unsigned seed, bool cattails);
Model fern(float size, unsigned seed);
Model pumpkin(unsigned seed);
Model cactus(unsigned seed);
Model lilyPads(float size, unsigned seed);
Model mushrooms(unsigned seed);
Model barrel(float height);
Model bookshelf(float width, float depth, float height);
Model log(unsigned seed);
Model standingStone(float height, unsigned seed);
// A farm wagon along x, the shafts towards +x.
Model wagon(float length, float width);

// A picture of the model for the backpack (RGBA, size x size, transparent around it): seen
// from the front and a little above, each material in its colour, lit from the upper left.
std::vector<std::uint8_t> renderIcon(const Model& model, const std::array<glm::vec3, MaterialCount>& colours, int size);

// U7's name as a kind: without trailing spaces, "/dagger//s" -> dagger, "/kni/fe/ves" -> knife.
std::string kindOf(const std::string& u7Name);
// The view layer of a movable U7 thing, the same in the U7 grid and Britannia3d: 2 furniture,
// 4 things (can be dragged: a known kind under 100 kg), 6 everything else (figures, blood,
// tracks, heavy things, whatever has no model).
int layerOf(const std::string& kind);

// Realistic weight (kg) of a thing of this U7 kind; furniture scales with its size (metres).
float weightKg(const std::string& kind, float width, float depth, float height);
// The German name shown for a kind ("table" -> "Tisch").
std::string germanName(const std::string& kind);

}
