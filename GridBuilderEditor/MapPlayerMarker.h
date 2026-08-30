#pragma once

enum class MapFacingDirection
{
    North,
    East,
    South,
    West
};

struct MapPlayerMarker
{
    int x = 0;
    int y = 0;

    MapFacingDirection direction =
        MapFacingDirection::North;

    bool visible = false;
};