#pragma once

#include "CubeFace.h"

namespace ow3d
{
    struct TileCoord
    {
        CubeFace face = CubeFace::PositiveZ;
        int x = 0;
        int y = 0;
    };
}