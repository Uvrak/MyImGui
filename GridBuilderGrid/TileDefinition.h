#pragma once

#include <cstdint>
#include <string>

namespace GridBuilder
{
    enum class TileType
    {
        Ground,
        Wall,
        Door,
        Object
    };

    struct TileSize
    {
        float width = 1.0f;
        float depth = 1.0f;
        float height = 0.0f;
    };

    struct TileDefinition
    {
        std::uint32_t id = 0;
        std::string name;
        TileType type = TileType::Ground;

        TileSize size;
    };
}