#pragma once

#include "PixelColor.h"

#include <vector>

class PixelImage
{
public:
    PixelImage(
        int width,
        int height
    );

    int width() const;
    int height() const;

    PixelColor& pixel(int x, int y);
    const PixelColor& pixel(int x, int y) const;
    PixelImage rotatedClockwise() const;

private:
    int index(int x, int y) const;

private:
    int m_width;
    int m_height;

    std::vector<PixelColor> m_pixels;
};
