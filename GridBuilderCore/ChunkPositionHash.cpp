#include "pch.h"

#include "ChunkPositionHash.h"
#include "ChunkPosition.h"

#include <functional>

std::size_t ChunkPositionHash::operator()(const ChunkPosition& position) const
{
    std::size_t hashX = std::hash<int>{}(position.x);
    std::size_t hashY = std::hash<int>{}(position.y);

    return hashX ^ (hashY << 1);
}