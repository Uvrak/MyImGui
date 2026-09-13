// ScreenSignature.h
#pragma once

#include <cstdint>
#include <vector>

struct ScreenPixel
{
    int x = 0;
    int y = 0;

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};

struct ScreenSignature
{
    std::vector<ScreenPixel> pixels;
};