#include "pch.h"

#include "Chunk.h"
Chunk::Chunk(const ChunkPosition& position, int chunkSize)
    : m_position(position),
    m_chunkSize(chunkSize),
    m_cells(
        static_cast<std::size_t>(chunkSize) *
        static_cast<std::size_t>(chunkSize)
    )
{}

Cell& Chunk::cell(int x, int y)
{
    return m_cells[index(x, y)];
}

const Cell& Chunk::cell(int x, int y) const
{
    return m_cells[index(x, y)];
}

const ChunkPosition& Chunk::position() const
{
    return m_position;
}

int Chunk::size() const
{
    return m_chunkSize;
}

std::size_t Chunk::index(int x, int y) const
{
    return static_cast<std::size_t>(y) *
        static_cast<std::size_t>(m_chunkSize) +
        static_cast<std::size_t>(x);
}