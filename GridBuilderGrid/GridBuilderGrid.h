#pragma once

#include <d3d11.h>

#include <functional>
#include <string>
#include <memory>

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
