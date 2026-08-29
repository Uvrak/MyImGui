#pragma once

#include "Chunk.h"
#include "ChunkPositionHash.h"

#include <string>
#include <unordered_map>

#include "MapColorPalette.h"

class ChunkManager
{
public:
    using ChunkCollection =
        std::unordered_map<
        ChunkPosition,
        Chunk,
        ChunkPositionHash
        >;

    using ColorAssignmentCollection =
        std::unordered_map<
        std::string,
        std::string
        >;

    explicit ChunkManager();
    explicit ChunkManager(int chunkSize);

    Chunk& chunk(
        const ChunkPosition& position
    );

    Cell& cell(
        int worldX,
        int worldY
    );

    bool hasChunk(
        const ChunkPosition& position
    ) const;

    const ChunkCollection& chunks() const;

    int chunkSize() const;

    bool removeMapColor(
        const std::string& colorId
    );

    bool isMapColorUsedByCells(
        const std::string& colorId
    ) const;

    MapColorPalette& colorPalette();

    const MapColorPalette&
        colorPalette() const;
    MapColorPalette m_colorPalette;

    ColorAssignmentCollection
        m_edgeColorAssignments;

    ColorAssignmentCollection
        m_miscColorAssignments;

    int m_activeLayer = 0;

    const ColorAssignmentCollection&
        edgeColorAssignments() const;

    const ColorAssignmentCollection&
        miscColorAssignments() const;

    void setEdgeColorAssignment(
        const std::string& edgeId,
        const std::string& colorId
    );

    void setMiscColorAssignment(
        const std::string& miscId,
        const std::string& colorId
    );

    const std::string& edgeColorId(
        const std::string& edgeId
    ) const;

    const std::string& miscColorId(
        const std::string& miscId
    ) const;

    int activeLayer() const;
    void setActiveLayer(int layer);

    bool hasLayer(int layer) const;

    const std::unordered_map<
        int,
        ChunkCollection
    >& layers() const;

    void removeEdgeFromAllCells(
        const std::string& edgeId
    );

    void renameEdgeInAllCells(
        const std::string& oldEdgeId,
        const std::string& newEdgeId
    );

    void renameMiscInAllCells(
        const std::string& oldMiscId,
        const std::string& newMiscId
    );

    void clear();

    const Cell* findCell(
        int worldX,
        int worldY,
        int layer
    ) const;

private:
    ChunkCollection& activeChunks();
    const ChunkCollection& activeChunks() const;

private:
    std::unordered_map<
        int,
        ChunkCollection
    > m_layers;

    int m_chunkSize;
};
