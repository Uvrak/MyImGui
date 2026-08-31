#include "GameModule.h"

#include "FrameReader.h"

bool GameModule::matchesScreen(
    const ScreenSignature& signature
) const
{
    if (m_frameHeader == nullptr ||
        m_framePixels == nullptr ||
        m_frameHeader->width <= 0 ||
        m_frameHeader->height <= 0 ||
        m_frameHeader->pitch <= 0)
    {
        return false;
    }

    const int bytesPerPixel =
        m_frameHeader->pitch /
        m_frameHeader->width;

    if (bytesPerPixel < 3)
    {
        return false;
    }

    for (const ScreenPixel& expected :
        signature.pixels)
    {
        if (expected.x < 0 ||
            expected.y < 0 ||
            expected.x >=
            m_frameHeader->contentWidth ||
            expected.y >=
            m_frameHeader->contentHeight)
        {
            return false;
        }

        const uint8_t* pixel =
            m_framePixels +
            expected.y *
            m_frameHeader->pitch +
            expected.x *
            bytesPerPixel;

        if (pixel[2] != expected.r ||
            pixel[1] != expected.g ||
            pixel[0] != expected.b)
        {
            return false;
        }
    }

    return !signature.pixels.empty();
}