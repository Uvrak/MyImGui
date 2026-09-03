#pragma once

#include <cstdint>

namespace DosBoxX
{
    class FrameReader;
    class FrameTexture;
}

namespace GridBuilderHost
{
    class DosBoxFramePipeline
    {
    public:
        DosBoxFramePipeline(
            DosBoxX::FrameReader& frameReader,
            DosBoxX::FrameTexture& frameTexture
        );

        ~DosBoxFramePipeline();

        void update();

        uint32_t contentWidth() const;
        uint32_t contentHeight() const;

    private:
        DosBoxX::FrameReader& m_frameReader;
        DosBoxX::FrameTexture& m_frameTexture;

        uint32_t m_contentWidth =
            0;

        uint32_t m_contentHeight =
            0;
    };
}