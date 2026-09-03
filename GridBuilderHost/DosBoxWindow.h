#pragma once

#include <cstdint>

namespace DosBoxX
{
    class FrameTexture;
}

namespace GridBuilderHost
{
    class DosBoxWindow
    {
    public:
        void draw(
            DosBoxX::FrameTexture& frameTexture,
            uint32_t contentWidth,
            uint32_t contentHeight
        );
    };
}