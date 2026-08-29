#pragma once

struct ChunkPosition
{
    int x;
    int y;

    bool operator==(const ChunkPosition& other) const;
};