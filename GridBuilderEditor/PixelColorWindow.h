#pragma once

#include "PixelColor.h"

class PixelColorWindow
{
public:
    void draw(
        bool* isOpen
    );

    PixelColor color() const;

private:
    float m_color[3] =
    {
        1.0f,
        1.0f,
        1.0f
    };
};