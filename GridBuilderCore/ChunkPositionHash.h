#pragma once

#include <cstddef>

struct ChunkPosition;

struct ChunkPositionHash
{
    std::size_t operator()(const ChunkPosition& position) const;
};