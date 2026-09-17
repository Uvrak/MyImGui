#pragma once

#include <d3d11.h>

#include <functional>
#include <string>
#include <memory>
#include "MapPlayerMarker.h"

class ChunkManager;

class GridBuilderGrid
{
public:
    GridBuilderGrid(
        ID3D11Device* device,
        int chunkSize
    );

    ~GridBuilderGrid();

    GridBuilderGrid(
        const GridBuilderGrid&
    ) = delete;

    GridBuilderGrid& operator=(
        const GridBuilderGrid&
        ) = delete;

    void draw(
        bool* isOpen
    );

    void registerMap(const std::string& key, const std::string& filename);
    void setMapDirectory(const std::string& directory);
    bool openMap(const std::string& key, const std::string& displayName = {});
    bool saveCurrentMap();
    void setPlayerMarker(const MapPlayerMarker& marker);

    using DebugCallback =
        std::function<void(const std::string&)>;

    void setDebugCallback(
        DebugCallback callback
    );

    const ChunkManager& map() const;

private:
    class Impl;

    std::unique_ptr<Impl> m_impl;
};
