#pragma once

#include "TileDefinition.h"

#include <cstdint>
#include <vector>

namespace GridBuilder
{
    class TileManager
    {
    public:
        bool addTile(const TileDefinition& tile);

        TileDefinition* findTile(std::uint32_t id);
        const TileDefinition* findTile(std::uint32_t id) const;

    private:
        std::vector<TileDefinition> m_tiles;
    };
}