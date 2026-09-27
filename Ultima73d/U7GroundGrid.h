#pragma once
#include "U7Data.h"
#include <GroundLayer.h>
#include <vector>

// Turns the original U7 world into a generic GridBuilderGrid ground layer: one grid cell per
// U7 tile (3072 x 3072), one OpenGL texture per distinct ground tile (shape, frame).
// Cells whose chunk entry is an object instead of a flat ground tile take the nearest flat
// tile of their row, so the object layers stand on continuous ground.
// GridBuilderGrid itself knows nothing about U7; it only receives the GroundLayer.
class U7GroundGrid {
public:
    ~U7GroundGrid();
    GroundLayer build(const U7::Data& data);

    // RGBA pixels of one flat tile (8 x 8), for exports and tests.
    static std::vector<std::uint8_t> rgba(const U7::Data& data, int shape, int frame);

    // Flat ground tile shown at a world tile: the chunk entry, or for object entries the
    // nearest flat tile of the row (may still be a non-flat entry when none is near).
    static U7::TileRef groundTile(const U7::Data& data, int x, int y);

    size_t materialCount() const { return m_textures.size(); }
    size_t nonFlatCells() const { return m_nonFlat; }

private:
    std::vector<unsigned> m_textures;
    size_t m_nonFlat = 0;
};
