#include "pch.h"

#include "PixelImage.h"

#include <stdexcept>

PixelImage::PixelImage(
    int width,
    int height
)
    : m_width(width),
    m_height(height)
{
    if (m_width <= 0 || m_height <= 0)
    {
        throw std::invalid_argument(
            "Pixel image dimensions must be positive."
        );
    }

    m_pixels.resize(
        static_cast<std::size_t>(
            m_width * m_height
            )
    );
}

int PixelImage::width() const
{
    return m_width;
}

int PixelImage::height() const
{
    return m_height;
}

PixelColor& PixelImage::pixel(int x, int y)
{
    return m_pixels[index(x, y)];
}

const PixelColor& PixelImage::pixel(
    int x,
    int y
) const
{
    return m_pixels[index(x, y)];
}

PixelImage PixelImage::rotatedClockwise() const
{
    PixelImage rotatedImage(
        m_height,
        m_width
    );

    for (int y = 0;
        y < m_height;
        ++y)
    {
        for (int x = 0;
            x < m_width;
            ++x)
        {
            const int rotatedX =
                m_height - 1 - y;

            const int rotatedY =
                x;

            rotatedImage.pixel(
                rotatedX,
                rotatedY
            ) = pixel(
                x,
                y
            );
        }
    }

    return rotatedImage;
}

int PixelImage::index(int x, int y) const
{
    if (x < 0 ||
        x >= m_width ||
        y < 0 ||
        y >= m_height)
    {
        throw std::out_of_range(
            "Pixel coordinates are outside the image."
        );
    }

    return y * m_width + x;
}