#pragma once

namespace MyImGui
{
    class NamedPipeClient;
    struct DosBoxFrameHeader;

    class DosBoxMouse
    {
    public:
        void update(
            NamedPipeClient& NamedPipeClient,
            const DosBoxFrameHeader& frameHeader,
            float imageWidth,
            float imageHeight,
            float imageLeft,
            float imageTop
        );
    };
}
