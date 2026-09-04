#pragma once

#include <cstdint>

namespace DosBoxX
{
    class FrameTexture;
    class Mouse;
    class NamedPipeClient;
}

namespace GridBuilderHost
{
    class DosBoxWindow
    {
    public:
        void draw(
            DosBoxX::FrameTexture& frameTexture,
            uint32_t contentWidth,
            uint32_t contentHeight,
            bool showCoordinates,
            DosBoxX::Mouse& dosBoxMouse,
            DosBoxX::NamedPipeClient& dosBoxPipeClient
        );
    };
}