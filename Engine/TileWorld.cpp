#include "TileWorld.h"
#include "WorldSettings.h"
#include "Renderer.h"
#include <vector>
#include <utility>
#include <map>
#include <limits>
#include <stdexcept>

namespace ow3d
{
    TileWorld::TileWorld() : m_chunks(ChunkCount) {}

    void TileWorld::validateTile(CubeFace face, int x, int y)
    {
        if (static_cast<int>(face) < 0 || static_cast<int>(face) >= 6 ||
            x < 0 || y < 0 || x >= TilesPerFace || y >= TilesPerFace)
            throw std::out_of_range("Tile coordinates outside the cube face");
    }

    int TileWorld::chunkIndex(CubeFace face, int x, int y)
    {
        if (static_cast<int>(face) < 0 || static_cast<int>(face) >= 6 ||
            x < 0 || y < 0 || x >= ChunksPerFace || y >= ChunksPerFace)
            throw std::out_of_range("Chunk coordinates outside the cube face");
        return static_cast<int>(face) * ChunksPerFace * ChunksPerFace + y * ChunksPerFace + x;
    }

    ChunkCoord TileWorld::chunkForTile(CubeFace face, int x, int y)
    {
        validateTile(face, x, y);
        return {face, x / TilesPerChunk, y / TilesPerChunk};
    }

    const TileChunk& TileWorld::chunk(CubeFace face, int x, int y) const
    {
        return m_chunks[chunkIndex(face, x, y)];
    }

    void TileWorld::invalidateChunk(CubeFace face, int x, int y)
    {
        m_chunks[chunkIndex(face, x, y)].dirty = true;
    }

    Tile& TileWorld::tile(CubeFace face, int x, int y)
    {
        const auto coord = chunkForTile(face, x, y);
        auto& owner = m_chunks[chunkIndex(face, coord.x, coord.y)];
        owner.dirty = true;
        return owner.tiles[(y % TilesPerChunk) * TilesPerChunk + x % TilesPerChunk];
    }

    const Tile& TileWorld::tile(CubeFace face, int x, int y) const
    {
        const auto coord = chunkForTile(face, x, y);
        return chunk(face, coord.x, coord.y).tiles[(y % TilesPerChunk) * TilesPerChunk + x % TilesPerChunk];
    }

    void TileWorld::update(float deltaTime) { (void)deltaTime; }

    void TileWorld::render(Renderer& renderer)
    {
        m_renderStats = {};
        const auto matrix = renderer.camera().projectionMatrix() * renderer.camera().viewMatrix();
        // GLM indexes columns. OpenGL's clip volume is -w..w on all three axes.
        const auto row = [&](int i) { return glm::vec4(matrix[0][i], matrix[1][i], matrix[2][i], matrix[3][i]); };
        const std::array<glm::vec4, 6> planes{
            row(3) + row(0), row(3) - row(0), row(3) + row(1),
            row(3) - row(1), row(3) + row(2), row(3) - row(2)};
        for (int index = 0; index < ChunkCount; ++index)
        {
            auto& current = m_chunks[index];
            if (current.dirty)
            {
                const ChunkCoord coordinate{static_cast<CubeFace>(index / (ChunksPerFace * ChunksPerFace)),
                    index % ChunksPerFace, (index / ChunksPerFace) % ChunksPerFace};
                rebuildChunk(coordinate, current);
                ++m_renderStats.rebuilt;
            }
            if (!current.visibleTiles) continue;
            if (!current.intersectsFrustum(planes))
            {
                ++m_renderStats.culled;
                continue;
            }
            ++m_renderStats.drawn;
            m_renderStats.tilesDrawn += current.visibleTiles;
            for (const auto& batch : current.batches)
            {
                renderer.drawTileMesh(batch.mesh, static_cast<TileMaterialId>(batch.materialId));
                ++m_renderStats.drawCalls;
            }
        }
    }

    glm::vec3 TileWorld::tileCenterDirection(
        CubeFace face,
        int x,
        int y) const
    {
        validateTile(face, x, y);
        const float tileSize =
            2.0f / static_cast<float>(TilesPerFace);

        const float u =
            -1.0f +
            (static_cast<float>(x) + 0.5f) * tileSize;

        const float v =
            -1.0f +
            (static_cast<float>(y) + 0.5f) * tileSize;

        glm::vec3 cubePosition{};

        switch (face)
        {
        case CubeFace::PositiveX:
            cubePosition = glm::vec3(1.0f, v, u);
            break;

        case CubeFace::NegativeX:
            cubePosition = glm::vec3(-1.0f, v, -u);
            break;

        case CubeFace::PositiveY:
            cubePosition = glm::vec3(u, 1.0f, -v);
            break;

        case CubeFace::NegativeY:
            cubePosition = glm::vec3(u, -1.0f, v);
            break;

        case CubeFace::PositiveZ:
            cubePosition = glm::vec3(u, v, 1.0f);
            break;

        case CubeFace::NegativeZ:
            cubePosition = glm::vec3(-u, v, -1.0f);
            break;
        }

        return glm::normalize(cubePosition);
    }

    glm::vec3 TileWorld::tileCenterPosition(
        CubeFace face,
        int x,
        int y) const
    {
        const glm::vec3 direction =
            tileCenterDirection(face, x, y);

        const float height =
            tile(face, x, y).height;

        return direction *
            (PlanetRadius + height);
    }

    void TileWorld::tileCorners(
        CubeFace face,
        int x,
        int y,
        glm::vec3& topLeft,
        glm::vec3& topRight,
        glm::vec3& bottomLeft,
        glm::vec3& bottomRight
    ) const
    {
        validateTile(face, x, y);
        const float tileSize =
            2.0f / static_cast<float>(TilesPerFace);

        const float u0 =
            -1.0f + static_cast<float>(x) * tileSize;

        const float u1 =
            u0 + tileSize;

        const float v0 =
            -1.0f + static_cast<float>(y) * tileSize;

        const float v1 =
            v0 + tileSize;

        auto toSphere = [face](float u, float v)
            {
                glm::vec3 cubePosition{};

                switch (face)
                {
                case CubeFace::PositiveX:
                    cubePosition = glm::vec3(1.0f, v, u);
                    break;

                case CubeFace::NegativeX:
                    cubePosition = glm::vec3(-1.0f, v, -u);
                    break;

                case CubeFace::PositiveY:
                    cubePosition = glm::vec3(u, 1.0f, -v);
                    break;

                case CubeFace::NegativeY:
                    cubePosition = glm::vec3(u, -1.0f, v);
                    break;

                case CubeFace::PositiveZ:
                    cubePosition = glm::vec3(u, v, 1.0f);
                    break;

                case CubeFace::NegativeZ:
                    cubePosition = glm::vec3(-u, v, -1.0f);
                    break;
                }

                return glm::normalize(cubePosition);
            };

        const float radius =
            PlanetRadius + tile(face, x, y).height;

        topLeft =
            toSphere(u0, v1) * radius;

        topRight =
            toSphere(u1, v1) * radius;

        bottomLeft =
            toSphere(u0, v0) * radius;

        bottomRight =
            toSphere(u1, v0) * radius;
    }

    void TileWorld::rebuildChunk(ChunkCoord coordinate, TileChunk& owner)
    {
        std::map<int, std::vector<float>> verticesByMaterial;
        glm::vec3 minimum(std::numeric_limits<float>::max());
        glm::vec3 maximum(std::numeric_limits<float>::lowest());
        unsigned int visible = 0;
        for (int localY = 0; localY < TilesPerChunk; ++localY)
        {
            for (int localX = 0; localX < TilesPerChunk; ++localX)
            {
                const Tile& currentTile = owner.tiles[localY * TilesPerChunk + localX];
                if (!currentTile.visible) continue;
                ++visible;
                glm::vec3 topLeft, topRight, bottomLeft, bottomRight;
                tileCorners(coordinate.face, coordinate.x * TilesPerChunk + localX,
                    coordinate.y * TilesPerChunk + localY, topLeft, topRight, bottomLeft, bottomRight);
                for (const auto& corner : {topLeft, topRight, bottomLeft, bottomRight})
                {
                    minimum = glm::min(minimum, corner);
                    maximum = glm::max(maximum, corner);
                }
                auto& vertices = verticesByMaterial[static_cast<int>(currentTile.materialId)];
                const auto uv = currentTile.uvOrigin;
                const auto extent = currentTile.uvScale;
                const auto data = currentTile.surfaceValues;
                auto append = [&vertices](const glm::vec3& p, float u, float v, float value)
                {
                    vertices.insert(vertices.end(), {p.x, p.y, p.z, u, v, value});
                };
                append(topLeft, uv.x, uv.y, data.x);
                append(bottomLeft, uv.x, uv.y + extent.y, data.z);
                append(bottomRight, uv.x + extent.x, uv.y + extent.y, data.w);
                append(topLeft, uv.x, uv.y, data.x);
                append(bottomRight, uv.x + extent.x, uv.y + extent.y, data.w);
                append(topRight, uv.x + extent.x, uv.y, data.y);
            }
        }
        // Commit only a complete rebuild; retain the old mesh and dirty flag on failure.
        std::vector<TileRenderBatch> batches;
        for (const auto& [material, vertices] : verticesByMaterial)
        {
            TileRenderBatch batch;
            batch.materialId = material;
            if (!batch.mesh.create(vertices.data(), static_cast<unsigned int>(vertices.size() / 6), 6))
                throw std::runtime_error("Could not create tile chunk mesh");
            batches.push_back(std::move(batch));
        }
        owner.batches = std::move(batches);
        owner.visibleTiles = visible;
        owner.boundsMin = visible ? minimum : glm::vec3(0.0f);
        owner.boundsMax = visible ? maximum : glm::vec3(0.0f);
        owner.dirty = false;
    }

    void TileWorld::setTileMaterial(
        CubeFace face,
        int x,
        int y,
        TileMaterialId materialId)
    {
        if (std::as_const(*this).tile(face, x, y).materialId != materialId)
            tile(face, x, y).materialId = materialId;
    }
}
