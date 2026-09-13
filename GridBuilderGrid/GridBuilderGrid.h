#pragma once

#include "WorldViewWindow.h"
#include "EditorToolBox.h"
#include "EditorEdgeBox.h"
#include "EditorMiscBox.h"

#include <d3d11.h>

class GridBuilderGrid
{
public:
    explicit GridBuilderGrid(
        ID3D11Device* device,
        int chunkSize
    );

    void draw(
        bool* isOpen
    );

    enum class GridPaintTarget
    {
        Edge,
        Misc
    };

private:
    WorldViewWindow m_worldViewWindow;
    EditorToolbox m_toolBox;
    EditorEdgeBox m_edgeBox;
    EditorMiscBox m_miscBox;

    GridPaintTarget m_paintTarget =
        GridPaintTarget::Edge;
};