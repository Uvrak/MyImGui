#pragma once
#include "U7Data.h"
#include <GroundLayer.h>
#include <functional>
#include <string>
#include <vector>

// Builds a generic GridBuilderGrid sprite layer from the U7 objects a filter selects (it gets
// the object and its lower-case shape name). Each object becomes one sprite at its U7 screen
// position: hot spot on the lower right pixel of its tile, 4 px up and left per lift, in cell
// units (1 cell = 8 px). Sprites are ordered back to front: lift, then x + y.
class U7ObjectLayer {
public:
    ~U7ObjectLayer();
    using Filter = std::function<bool(const U7::WorldObject&, const std::string& name)>;
    GroundSpriteLayer build(const U7::Data& data, const std::string& name, const Filter& filter);
    size_t textureCount() const { return m_textures.size(); }

    // Layer 1: walls, doors, windows, town walls, rock walls ...
    static bool isStructure(const std::string& name);

private:
    std::vector<unsigned> m_textures;
};
