#pragma once

namespace MightAndMagic3
{
    enum class Direction
    {
        Unknown,
        North,
        East,
        South,
        West
    };

    struct Position
    {
        int x = 0;
        int y = 0;

        Direction direction =
            Direction::Unknown;

        bool valid = false;
    };
}