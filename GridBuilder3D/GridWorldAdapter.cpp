#include "GridWorldAdapter.h"

#include "World3D.h"
#include "ChunkManager.h"

namespace GridBuilder3D
{
    void GridWorldAdapter::rebuild(
        const ChunkManager& map,
        World3D& world
    )
    {
        world.clear();

        for (const auto& [position, chunk] : map.chunks())
        {
            for (int y = 0; y < chunk.size(); ++y)
            {
                for (int x = 0; x < chunk.size(); ++x)
                {
                    const Cell& source = chunk.cell(x, y);
                    const WorldCell cell{
                        source.hasEdge(EdgeDirection::North),
                        source.hasEdge(EdgeDirection::East),
                        source.hasEdge(EdgeDirection::South),
                        source.hasEdge(EdgeDirection::West)
                    };

                    if (cell.northWall || cell.eastWall ||
                        cell.southWall || cell.westWall)
                    {
                        world.setCell(
                            position.x * chunk.size() + x,
                            position.y * chunk.size() + y,
                            cell
                        );
                    }
                }
            }
        }
    }
}
