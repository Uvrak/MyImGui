#pragma once

#include "MapColor.h"

#include <string>
#include <vector>

class MapColorPalette
{
public:
    MapColorPalette();

    const std::vector<MapColor>&
        colors() const;

    MapColor* find(
        const std::string& colorId
    );

    const MapColor* find(
        const std::string& colorId
    ) const;

    std::string addColor(
        const EdgeColor& color
    );

    bool addColor(
        const std::string& colorId,
        const EdgeColor& color
    );

    bool removeColor(
        const std::string& colorId
    );

    void clear();

private:
    std::string nextColorId() const;

private:
    std::vector<MapColor>
        m_colors;
};