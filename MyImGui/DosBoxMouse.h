#pragma once

namespace MyImGui
{
    class ExternalWindow;
    struct DosBoxFrameHeader;

    class DosBoxMouse
    {
    public:
        void update(
            ExternalWindow& externalWindow,
            const DosBoxFrameHeader& frameHeader,
            float imageWidth,
            float imageHeight,
            float imageLeft,
            float imageTop
        );
    };
}
