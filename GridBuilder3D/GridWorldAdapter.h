#pragma once

class ChunkManager;

namespace GridBuilder3D
{
    class World3D;

    class GridWorldAdapter
    {
    public:
        static void rebuild(
            const ChunkManager& map,
            World3D& world
        );
    };
}
