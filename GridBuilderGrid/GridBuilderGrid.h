#pragma once

#include <d3d11.h>

#include <functional>
#include <string>
#include <memory>
#include "MapPlayerMarker.h"
#include "GroundLayer.h"
#include "EditorTextureResolver.h"

class ChunkManager;

class GridBuilderGrid
{
public:
    GridBuilderGrid(
        ID3D11Device* device,
        int chunkSize,
        const std::string& iconDirectory = {}
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

    // Uses the existing viewport in navigation-only mode; no editor textures required.
    void setGroundLayer(GroundLayer layer, const std::string& title);
    void focusGroundCell(int x, int y);
    GroundViewState groundView() const;
    void setGroundView(GroundViewState view);
    void setGroundCell(int x, int y, GroundMaterial material);
    void setGroundBorder(int x, int y, std::uint8_t border);
    void setGroundDropCallback(GroundDropCallback callback);
    void setGroundNavigationCallback(GroundNavigationCallback callback);
    void setEditorTextureResolver(EditorTextureResolver resolver);
    void setEditorToolsEnabled(bool enabled);

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
