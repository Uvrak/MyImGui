#pragma once

#include <cstdint>
#include <unordered_map>

namespace GridBuilder3D
{
    struct WorldCell
    {
        bool northWall = false;
        bool eastWall = false;
        bool southWall = false;
        bool westWall = false;
    };

    class World3D
    {
    public:
        void setCell(
            int x,
            int y,
            const WorldCell& cell
        );

        const WorldCell* cell(
            int x,
            int y
        ) const;

        void clear();

    private:
        static std::uint64_t makeKey(
            int x,
            int y
        );

        std::unordered_map<std::uint64_t, WorldCell> m_cells;
    };
}