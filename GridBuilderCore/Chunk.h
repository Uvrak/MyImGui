#pragma once

#include <vector>
#include <cstddef>
#include <string>

#include "Cell.h"
#include "ChunkPosition.h"

class Chunk
{
public:
    Chunk(
        const ChunkPosition& position,
        int chunkSize
    );

    Cell& cell(int x, int y);
    const Cell& cell(int x, int y) const;

    const ChunkPosition& position() const;
    int size() const;

private:
    std::size_t index(int x, int y) const;

private:
    ChunkPosition m_position;
    int m_chunkSize;
    std::vector<Cell> m_cells;
};