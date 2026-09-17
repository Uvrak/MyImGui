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
        void onLeftMouseButtonDown(
            float mouseX,
            float mouseY,
            DosBoxX::Mouse& dosBoxMouse,
            DosBoxX::NamedPipeClient& dosBoxPipeClient
        );
        void draw(
            DosBoxX::FrameTexture& frameTexture,
            uint32_t contentWidth,
            uint32_t contentHeight,
            bool showCoordinates,
            DosBoxX::Mouse& dosBoxMouse,
            DosBoxX::NamedPipeClient& dosBoxPipeClient
        );
    private:
        float m_imageLeft = 0.0f;
        float m_imageTop = 0.0f;
        float m_imageWidth = 0.0f;
        float m_imageHeight = 0.0f;
        uint32_t m_contentWidth = 0;
        uint32_t m_contentHeight = 0;
    };
}
