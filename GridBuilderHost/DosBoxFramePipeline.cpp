#include "DosBoxFramePipeline.h"

#include "FrameReader.h"
#include "FrameTexture.h"

#include <Windows.h>
#include <cstdio>

namespace GridBuilderHost
{
    DosBoxFramePipeline::DosBoxFramePipeline(
        DosBoxX::FrameReader& frameReader,
        DosBoxX::FrameTexture& frameTexture
    )
        : m_frameReader(
            frameReader
        ),
        m_frameTexture(
            frameTexture
        )
    {}

    DosBoxFramePipeline::~DosBoxFramePipeline()
    {}

    void DosBoxFramePipeline::update()
    {
        if (!m_frameReader.tryOpen())
        {
            return;
        }

        const DosBoxX::DosBoxFrameHeader* frameHeader =
            m_frameReader.header();

        const uint8_t* pixels =
            m_frameReader.pixels();

        if (frameHeader == nullptr ||
            pixels == nullptr)
        {
            return;
        }

        static uint64_t lastFrameCounter =
            UINT64_MAX;

        m_contentWidth =
            frameHeader->contentWidth;

        m_contentHeight =
            frameHeader->contentHeight;
            0;

        m_frameTexture.update(
            pixels,
            frameHeader->width,
            frameHeader->height,
            frameHeader->pitch
        );
    }

    uint32_t DosBoxFramePipeline::contentWidth() const
    {
        return m_contentWidth;
    }

    uint32_t DosBoxFramePipeline::contentHeight() const
    {
        return m_contentHeight;
    }
}