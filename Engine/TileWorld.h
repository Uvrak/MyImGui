#pragma once

#include "IWorld.h"
#include "Tile.h"
#include "CubeFace.h"
#include "Mesh.h"
#include "TileChunk.h"

#include <vector>
#include <glm/glm.hpp>

namespace ow3d
{
    class TileWorld : public IWorld
    {
    public:
        // Storage resolution; visual material scale is independent of this grid.
        static constexpr int TilesPerFace = 256;
        static constexpr int TilesPerChunk = TileChunk::Side;
        static constexpr int ChunksPerFace = TilesPerFace / TilesPerChunk;
        static constexpr int ChunkCount = 6 * ChunksPerFace * ChunksPerFace;
        static_assert(TilesPerFace % TilesPerChunk == 0);

        static ChunkCoord chunkForTile(CubeFace face, int x, int y);
        const TileChunk& chunk(CubeFace face, int chunkX, int chunkY) const;
        // Required when modifying a retained Tile& after an intervening render.
        void invalidateChunk(CubeFace face, int chunkX, int chunkY);
        const ChunkRenderStats& renderStats() const { return m_renderStats; }

        TileWorld();
        void setTileMaterial(CubeFace face, int x, int y, TileMaterialId materialId);
        // Mutable access marks only the owning chunk dirty; use const access for reads.
        Tile& tile(CubeFace face, int x, int y);
        const Tile& tile(CubeFace face, int x, int y) const;
        void update(float deltaTime) override;
        void render(Renderer& renderer) override;

        glm::vec3 tileCenterDirection(
            CubeFace face,
            int x,
            int y
        ) const;

        glm::vec3 tileCenterPosition(
            CubeFace face,
            int x,
            int y
        ) const;

        void tileCorners(
            CubeFace face,
            int x,
            int y,
            glm::vec3& topLeft,
            glm::vec3& topRight,
            glm::vec3& bottomLeft,
            glm::vec3& bottomRight
        ) const;
    private:
        static void validateTile(CubeFace face, int x, int y);
        static int chunkIndex(CubeFace face, int x, int y);
        void rebuildChunk(ChunkCoord coordinate, TileChunk& chunk);
        std::vector<TileChunk> m_chunks;
        ChunkRenderStats m_renderStats;
    };
}
