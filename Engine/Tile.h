#pragma once

#include "TileMaterial.h"
#include <glm/glm.hpp>

namespace ow3d
{
    struct Tile
    {
        // Texture coordinates and arbitrary scalar data for custom tile shaders.
        glm::vec2 uvOrigin{ 0.0f };
        glm::vec2 uvScale{ 1.0f };
        // Top-left, top-right, bottom-left, bottom-right.
        glm::vec4 surfaceValues{ 0.0f };
        bool visible = false;
        float height = 0.0f;

        TileMaterialId materialId =
            TileMaterialId::Material0;
    };
}