#pragma once

#include "PixelImage.h"

#include <string>

class ImageFileLoader
{
public:
    static bool load(
        const std::string& filename,
        PixelImage& image
    );
};
