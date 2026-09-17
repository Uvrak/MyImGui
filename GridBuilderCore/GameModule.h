#pragma once

#include <optional>
#include <string>

enum class GameFacingDirection { North, East, South, West };

struct GamePosition
{
    int x = 0;
    int y = 0;
    GameFacingDirection direction = GameFacingDirection::North;
};

class GameModule
{
public:
    virtual ~GameModule() = default;
    virtual void start() = 0;
    virtual void update() = 0;
    virtual std::optional<std::string> currentMapKey() const = 0;
    virtual std::optional<std::string> currentMapName() const = 0;
    virtual std::optional<GamePosition> currentPosition() const = 0;
};
