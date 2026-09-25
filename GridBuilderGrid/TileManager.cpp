#include "TileManager.h"

namespace GridBuilder
{
    bool TileManager::addTile(const TileDefinition& tile)
    {
        if (findTile(tile.id) != nullptr)
            return false;

        m_tiles.push_back(tile);
        return true;
    }

    TileDefinition* TileManager::findTile(std::uint32_t id)
    {
        for (auto& tile : m_tiles)
        {
            if (tile.id == id)
                return &tile;
        }

        return nullptr;
    }

    const TileDefinition* TileManager::findTile(std::uint32_t id) const
    {
        for (const auto& tile : m_tiles)
        {
            if (tile.id == id)
                return &tile;
        }

        return nullptr;
    }
}