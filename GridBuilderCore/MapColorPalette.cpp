#include "pch.h"

#include "MapColorPalette.h"

#include <algorithm>
#include <cstddef>

MapColorPalette::MapColorPalette()
{
    m_colors.push_back(
        MapColor
        {
            "color-1",
            EdgeColor
            {
                255,
                255,
                255,
                255
            }
        }
    );
}

const std::vector<MapColor>&
MapColorPalette::colors() const
{
    return m_colors;
}

MapColor* MapColorPalette::find(
    const std::string& colorId
)
{
    const auto position =
        std::find_if(
            m_colors.begin(),
            m_colors.end(),
            [&colorId](
                const MapColor& mapColor
                )
            {
                return mapColor.id ==
                    colorId;
            }
        );

    if (position == m_colors.end())
    {
        return nullptr;
    }

    return &*position;
}

const MapColor* MapColorPalette::find(
    const std::string& colorId
) const
{
    const auto position =
        std::find_if(
            m_colors.begin(),
            m_colors.end(),
            [&colorId](
                const MapColor& mapColor
                )
            {
                return mapColor.id ==
                    colorId;
            }
        );

    if (position == m_colors.end())
    {
        return nullptr;
    }

    return &*position;
}

std::string MapColorPalette::addColor(
    const EdgeColor& color
)
{
    const std::string colorId =
        nextColorId();

    m_colors.push_back(
        MapColor
        {
            colorId,
            color
        }
    );

    return colorId;
}

bool MapColorPalette::addColor(
    const std::string& colorId,
    const EdgeColor& color
)
{
    if (colorId.empty() ||
        find(colorId) != nullptr)
    {
        return false;
    }

    m_colors.push_back(
        MapColor
        {
            colorId,
            color
        }
    );

    return true;
}

bool MapColorPalette::removeColor(
    const std::string& colorId
)
{
    const auto position =
        std::find_if(
            m_colors.begin(),
            m_colors.end(),
            [&colorId](
                const MapColor& mapColor
                )
            {
                return mapColor.id ==
                    colorId;
            }
        );

    if (position == m_colors.end())
    {
        return false;
    }

    m_colors.erase(position);

    return true;
}

void MapColorPalette::clear()
{
    m_colors.clear();
}

std::string MapColorPalette::nextColorId() const
{
    std::size_t number = 1;

    while (true)
    {
        const std::string colorId =
            "color-" +
            std::to_string(number);

        if (find(colorId) == nullptr)
        {
            return colorId;
        }

        ++number;
    }
}