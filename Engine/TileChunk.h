#pragma once

#include "CubeFace.h"
#include "Tile.h"
#include "TileRenderBatch.h"
#include <array>
#include <vector>

namespace ow3d
{
    struct ChunkCoord
    {
        CubeFace face = CubeFace::PositiveZ;
        int x = 0;
        int y = 0;
    };

    struct TileChunk
    {
        static constexpr int Side = 16;
        std::array<Tile, Side * Side> tiles{};
        std::vector<TileRenderBatch> batches;
        glm::vec3 boundsMin{0.0f};
        glm::vec3 boundsMax{0.0f};
        unsigned int visibleTiles = 0;
        // A new chunk is empty and needs no GPU allocation.
        bool dirty = false;

        bool intersectsFrustum(const std::array<glm::vec4, 6>& planes) const
        {
            if (visibleTiles == 0) return false;
            for (const auto& p : planes)
            {
                const glm::vec3 support{
                    p.x >= 0 ? boundsMax.x : boundsMin.x,
                    p.y >= 0 ? boundsMax.y : boundsMin.y,
                    p.z >= 0 ? boundsMax.z : boundsMin.z};
                if (glm::dot(glm::vec3(p), support) + p.w < -0.00001f)
                    return false;
            }
            return true;
        }
    };

    struct ChunkRenderStats
    {
        unsigned int rebuilt = 0;
        unsigned int drawn = 0;
        unsigned int culled = 0;
        unsigned int drawCalls = 0;
        unsigned int tilesDrawn = 0;
    };
}
