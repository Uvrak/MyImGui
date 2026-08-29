#pragma once

#include "PixelImage.h"

#include <string>

class SvgFileWriter
{
public:
    static bool save(
        const std::string& filename,
        const PixelImage& image
    );
};
