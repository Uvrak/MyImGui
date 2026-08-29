#include "pch.h"

#include "ChunkManager.h"

#include <cassert>

namespace
{
    bool isValidChunkSize(int size)
    {
        switch (size)
        {
        case 8:
        case 16:
        case 32:
        case 64:
        case 128:
            return true;

        default:
            return false;
        }
    }
}

ChunkManager::ChunkManager()
    : m_chunkSize(16)
{
    m_layers.try_emplace(0);
}

ChunkManager::ChunkManager(int chunkSize)
    : m_chunkSize(chunkSize)
{
    assert(
        isValidChunkSize(chunkSize)
    );

    m_layers.try_emplace(0);
}

ChunkManager::ChunkCollection&
ChunkManager::activeChunks()
{
    return m_layers[m_activeLayer];
}

const ChunkManager::ChunkCollection&
ChunkManager::activeChunks() const
{
    return m_layers.at(m_activeLayer);
}

Chunk& ChunkManager::chunk(
    const ChunkPosition& position
)
{
    ChunkCollection& chunks =
        activeChunks();

    auto [iterator, inserted] =
        chunks.try_emplace(
            position,
            position,
            m_chunkSize
        );

    return iterator->second;
}

Cell& ChunkManager::cell(
    int worldX,
    int worldY
)
{
    int chunkX =
        worldX / m_chunkSize;

    int chunkY =
        worldY / m_chunkSize;

    int cellX =
        worldX % m_chunkSize;

    int cellY =
        worldY % m_chunkSize;

    if (cellX < 0)
    {
        cellX += m_chunkSize;
        --chunkX;
    }

    if (cellY < 0)
    {
        cellY += m_chunkSize;
        --chunkY;
    }

    const ChunkPosition chunkPosition
    {
        chunkX,
        chunkY
    };

    return chunk(
        chunkPosition
    ).cell(
        cellX,
        cellY
    );
}

bool ChunkManager::hasChunk(
    const ChunkPosition& position
) const
{
    const ChunkCollection& chunks =
        activeChunks();

    return chunks.find(position) !=
        chunks.end();
}

const ChunkManager::ChunkCollection&
ChunkManager::chunks() const
{
    return activeChunks();
}

int ChunkManager::chunkSize() const
{
    return m_chunkSize;
}

int ChunkManager::activeLayer() const
{
    return m_activeLayer;
}

void ChunkManager::setActiveLayer(
    int layer
)
{
    m_activeLayer = layer;

    m_layers.try_emplace(
        m_activeLayer
    );
}

bool ChunkManager::hasLayer(
    int layer
) const
{
    return m_layers.find(layer) !=
        m_layers.end();
}

const std::unordered_map<
    int,
    ChunkManager::ChunkCollection
>& ChunkManager::layers() const
{
    return m_layers;
}

void ChunkManager::removeEdgeFromAllCells(
    const std::string& edgeId
)
{
    const EdgeDirection directions[] =
    {
        EdgeDirection::North,
        EdgeDirection::East,
        EdgeDirection::South,
        EdgeDirection::West
    };

    for (auto& [layer, chunks] :
        m_layers)
    {
        (void)layer;

        for (auto& [position, chunk] :
            chunks)
        {
            (void)position;

            for (int y = 0;
                y < chunk.size();
                ++y)
            {
                for (int x = 0;
                    x < chunk.size();
                    ++x)
                {
                    Cell& cell =
                        chunk.cell(x, y);

                    for (
                        EdgeDirection direction :
                    directions
                        )
                    {
                        if (
                            cell.edgeId(direction) ==
                            edgeId
                            )
                        {
                            cell.removeEdge(
                                direction
                            );
                        }
                    }
                }
            }
        }
    }
}

void ChunkManager::renameEdgeInAllCells(
    const std::string& oldEdgeId,
    const std::string& newEdgeId
)
{
    const EdgeDirection directions[] =
    {
        EdgeDirection::North,
        EdgeDirection::East,
        EdgeDirection::South,
        EdgeDirection::West
    };

    for (auto& [layer, chunks] :
        m_layers)
    {
        (void)layer;

        for (auto& [position, chunk] :
            chunks)
        {
            (void)position;

            for (int y = 0;
                y < chunk.size();
                ++y)
            {
                for (int x = 0;
                    x < chunk.size();
                    ++x)
                {
                    Cell& cell =
                        chunk.cell(x, y);

                    for (
                        EdgeDirection direction :
                    directions
                        )
                    {
                        if (cell.edgeId(direction) ==
                            oldEdgeId)
                        {
                            const std::string colorId =
                                cell.edgeColorId(
                                    direction
                                );

                            cell.setEdge(
                                direction,
                                newEdgeId,
                                colorId
                            );
                        }
                    }
                }
            }
        }
    }
}


void ChunkManager::renameMiscInAllCells(
    const std::string& oldMiscId,
    const std::string& newMiscId
)
{
    for (auto& [layer, chunks] :
        m_layers)
    {
        (void)layer;

        for (auto& [position, chunk] :
            chunks)
        {
            (void)position;

            for (int y = 0;
                y < chunk.size();
                ++y)
            {
                for (int x = 0;
                    x < chunk.size();
                    ++x)
                {
                    Cell& cell =
                        chunk.cell(x, y);

                    if (cell.miscId() ==
                        oldMiscId)
                    {
                        const std::string colorId =
                            cell.miscColorId();

                        cell.setMisc(
                            newMiscId,
                            colorId
                        );
                    }
                }
            }
        }
    }
}

bool ChunkManager::isMapColorUsedByCells(
    const std::string& colorId
) const
{
    if (colorId.empty())
    {
        return false;
    }

    for (const auto& [layer, chunks] :
        m_layers)
    {
        (void)layer;

        for (const auto& [position, chunk] :
            chunks)
        {
            (void)position;

            for (int localY = 0;
                localY < chunk.size();
                ++localY)
            {
                for (int localX = 0;
                    localX < chunk.size();
                    ++localX)
                {
                    const Cell& cell =
                        chunk.cell(
                            localX,
                            localY
                        );

                    if (cell.referencesColor(
                        colorId
                    ))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool ChunkManager::removeMapColor(
    const std::string& colorId
)
{
    const std::vector<MapColor>& colors =
        m_colorPalette.colors();

    if (colors.empty() ||
        colorId.empty())
    {
        return false;
    }

    const std::string fallbackColorId =
        colors.front().id;

    if (colorId == fallbackColorId ||
        m_colorPalette.find(colorId) ==
        nullptr ||
        isMapColorUsedByCells(colorId))
    {
        return false;
    }

    for (auto& [edgeId, assignedColorId] :
        m_edgeColorAssignments)
    {
        (void)edgeId;

        if (assignedColorId == colorId)
        {
            assignedColorId =
                fallbackColorId;
        }
    }

    for (auto& [miscId, assignedColorId] :
        m_miscColorAssignments)
    {
        (void)miscId;

        if (assignedColorId == colorId)
        {
            assignedColorId =
                fallbackColorId;
        }
    }

    return m_colorPalette.removeColor(
        colorId
    );
}

MapColorPalette&
ChunkManager::colorPalette()
{
    return m_colorPalette;
}

const MapColorPalette&
ChunkManager::colorPalette() const
{
    return m_colorPalette;
}

void ChunkManager::clear()
{
    m_layers.clear();

    m_colorPalette =
        MapColorPalette{};

    m_edgeColorAssignments.clear();
    m_miscColorAssignments.clear();

    m_activeLayer = 0;

    m_layers.try_emplace(0);
}

const Cell* ChunkManager::findCell(
    int worldX,
    int worldY,
    int layer
) const
{
    const auto layerIterator =
        m_layers.find(layer);

    if (layerIterator ==
        m_layers.end())
    {
        return nullptr;
    }

    int chunkX =
        worldX / m_chunkSize;

    int chunkY =
        worldY / m_chunkSize;

    int cellX =
        worldX % m_chunkSize;

    int cellY =
        worldY % m_chunkSize;

    if (cellX < 0)
    {
        cellX += m_chunkSize;
        --chunkX;
    }

    if (cellY < 0)
    {
        cellY += m_chunkSize;
        --chunkY;
    }

    const ChunkPosition position
    {
        chunkX,
        chunkY
    };

    const ChunkCollection& chunks =
        layerIterator->second;

    const auto chunkIterator =
        chunks.find(position);

    if (chunkIterator ==
        chunks.end())
    {
        return nullptr;
    }

    return &chunkIterator->second.cell(
        cellX,
        cellY
    );
}

const ChunkManager::ColorAssignmentCollection&
ChunkManager::edgeColorAssignments() const
{
    return m_edgeColorAssignments;
}

const ChunkManager::ColorAssignmentCollection&
ChunkManager::miscColorAssignments() const
{
    return m_miscColorAssignments;
}

void ChunkManager::setEdgeColorAssignment(
    const std::string& edgeId,
    const std::string& colorId
)
{
    if (edgeId.empty() ||
        m_colorPalette.find(colorId) == nullptr)
    {
        return;
    }

    m_edgeColorAssignments[edgeId] =
        colorId;
}

void ChunkManager::setMiscColorAssignment(
    const std::string& miscId,
    const std::string& colorId
)
{
    if (miscId.empty() ||
        m_colorPalette.find(colorId) == nullptr)
    {
        return;
    }

    m_miscColorAssignments[miscId] =
        colorId;
}

const std::string& ChunkManager::edgeColorId(
    const std::string& edgeId
) const
{
    static const std::string emptyId;

    const auto position =
        m_edgeColorAssignments.find(
            edgeId
        );

    if (position ==
        m_edgeColorAssignments.end())
    {
        return emptyId;
    }

    return position->second;
}

const std::string& ChunkManager::miscColorId(
    const std::string& miscId
) const
{
    static const std::string emptyId;

    const auto position =
        m_miscColorAssignments.find(
            miscId
        );

    if (position ==
        m_miscColorAssignments.end())
    {
        return emptyId;
    }

    return position->second;
}
