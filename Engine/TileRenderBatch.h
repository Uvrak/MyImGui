#pragma once

#include "Mesh.h"

namespace ow3d
{
    struct TileRenderBatch
    {
        int materialId = 0;
        Mesh mesh;
    };
}