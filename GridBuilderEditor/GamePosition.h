#pragma once

enum class GameDirection
{
    Unknown,
    North,
    East,
    South,
    West
};

struct GamePosition
{
    int x = 0;
    int y = 0;

    GameDirection direction =
        GameDirection::Unknown;

    bool valid = false;
};