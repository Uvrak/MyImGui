#pragma once
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

// The 3D variants of U7's ground tiles, so the U7 grid and Britannia3d show the same ground:
//
//   data/Maps/ground-variants.txt   one line per U7 tile: shape frame outside inside file
//                                   outside: what Britannia3d draws for it in the open (cobble,
//                                   cobble-dirt, grass, grass-mud, or tile: its own picture);
//                                   inside: under a roof (boards, flagstones, bricks, carpet, or
//                                   tile); file: the tile's high resolution picture (under the
//                                   asset directory), used for "tile" and wherever a material
//                                   needs the tile's colours.
//   <assets>/Materials/Ground/<shape>-<frame>.png
//                                   the high resolution tile, made from U7's tile when missing
//                                   (existing files are used as they are, so they can be
//                                   repainted).
//
// Tiles not listed yet get the variant Britannia3d recognises from their pixels, and the file
// is written back with them.
namespace GroundTiles {

using Key = std::pair<int, int>;                         // shape, frame

struct Variant { std::string outside = "tile", inside = "tile"; };
std::map<Key, Variant> loadVariants(const std::filesystem::path& file);
void saveVariants(const std::filesystem::path& file, const std::map<Key, Variant>& variants);
std::string tileFile(Key key);                           // relative to the asset directory

// Layer 1 (walls, doors, windows, town walls, rock): data/Maps/structure-variants.txt, one line
// per U7 graphic: shape frame variant file - variant: planks, post, stone, ashlar, half-timber
// or graphic (its own high resolution picture, <assets>/Materials/Objects/<shape>-<frame>.png).
std::map<Key, std::string> loadStructureVariants(const std::filesystem::path& file);
// (Also for layer 3, data/Maps/terrain-variants.txt; what and variants: the header lines.)
void saveStructureVariants(const std::filesystem::path& file, const std::map<Key, std::string>& variants, const std::string& what,
                           const std::string& choices);
std::string objectFile(Key key);                         // relative to the asset directory
// A high resolution picture (RGBA, width x height): the file (sampled to the size), or the
// upscaled U7 graphic with grain, mottling and specks on its opaque texels, saved there.
std::vector<std::uint8_t> loadOrCreatePicture(const std::filesystem::path& file, std::vector<std::uint8_t> upscaled, int width, int height, unsigned seed);

// The high resolution tile (RGBA, size x size): the file, or made from the upscaled U7 tile
// (grain, soft mottling and speckles in its own colours, seamless) and saved.
std::vector<std::uint8_t> loadOrCreate(const std::filesystem::path& assetDirectory, Key key, const std::vector<std::uint8_t>& upscaled, int size);

}
