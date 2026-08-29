#pragma once

#include <cstdint>

struct EdgeColor
{
    std::uint8_t red = 255;
    std::uint8_t green = 255;
    std::uint8_t blue = 255;
    std::uint8_t alpha = 255;

    bool operator==(
        const EdgeColor& other
        ) const = default;
};