#pragma once

#include "PixelColor.h"

#include <array>

class ColorPaletteWindow
{
public:
    void draw(
        bool* isOpen,
        bool eyedropperActive
    );

    const PixelColor& selectedColor() const;

private:
    PixelColor pickerColor() const;

private:
    float m_pickerColor[3] =
    {
        1.0f,
        1.0f,
        1.0f
    };

    std::array<PixelColor, 8>
        m_colors =
    { {
        { 255, 255, 255, 255 },
        {   0,   0,   0, 255 },
        { 255,   0,   0, 255 },
        {   0, 255,   0, 255 },
        {   0,   0, 255, 255 },
        { 255, 255,   0, 255 },
        { 255,   0, 255, 255 },
        {   0, 255, 255, 255 }
    } };

    int m_selectedColorIndex = 0;
};
