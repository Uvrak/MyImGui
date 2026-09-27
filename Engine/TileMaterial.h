#pragma once

namespace ow3d
{
    enum class TileMaterialId
    {
        Material0 = 0,
        Material1 = 1,
        Material2 = 2
    };

    struct TileMaterial
    {
        TileMaterialId id = TileMaterialId::Material0;

        bool animated = false;

        int firstFrame = 0;
        int frameCount = 1;

        float secondsPerFrame = 0.1f;
        bool loop = true;
    };
}