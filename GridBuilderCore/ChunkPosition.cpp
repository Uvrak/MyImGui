#include "pch.h"
#include "ChunkPosition.h"

bool ChunkPosition::operator==(const ChunkPosition& other) const
{
    return x == other.x &&
        y == other.y;
}