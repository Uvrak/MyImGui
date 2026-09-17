#pragma once

#include <optional>
#include <string>
#include <cstdint>
#include <vector>

enum class GameFacingDirection { North, East, South, West };

struct GamePosition
{
    int x = 0;
    int y = 0;
    GameFacingDirection direction = GameFacingDirection::North;
};

struct GameButtonRect
{
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
    uint8_t r = 255, g = 255, b = 0, a = 255;
    std::string dosKey;
};

struct GameButtonPoint { int x = 0; int y = 0; };

struct GameButtonEdit
{
    std::size_t index = 0;
    GameButtonRect rect;
};

class GameModule
{
public:
    virtual ~GameModule() = default;
    virtual void start() = 0;
    virtual void update() = 0;
    virtual void keyDown(int) {}
    // Called before sending a direct click. True means the module consumed it.
    virtual bool onDosBoxMouseClick(GameButtonPoint) { return false; }
    virtual void keyUp(int) {}
    virtual bool blockDirectDosBoxKeyboard() const { return false; }
    virtual bool blockDirectDosBoxVerticalKeys() const { return false; }
    virtual bool blockDirectDosBoxInventoryKeys() const { return false; }
    virtual std::optional<std::string> inventoryDebugLine() const { return std::nullopt; }
    virtual std::optional<std::string> inventoryClickDebugLine() const { return std::nullopt; }
    virtual std::vector<std::string> inventoryMenuEntries() const { return {}; }
    virtual int inventoryMenuSelection() const { return -1; }
    virtual std::string inventoryMenuTitle() const { return {}; }
    virtual std::optional<GameButtonPoint> inventoryMenuPosition() const { return std::nullopt; }
    virtual std::optional<GameButtonRect> selectedButtonRect() const { return std::nullopt; }
    virtual std::optional<GameButtonRect> selectedInventoryRect() const { return std::nullopt; }
    virtual std::vector<GameButtonRect> buttonRects() const { return {}; }
    virtual std::size_t buttonCount() const { return 0; }
    virtual std::optional<GameButtonPoint> takeButtonClick() { return std::nullopt; }
    virtual bool buttonEditingActive() const { return false; }
    virtual bool buttonViewAvailable() const { return false; }
    virtual std::string buttonViewName() const { return {}; }
    virtual std::vector<std::string> activeViewNames() const { return {}; }
    virtual bool addButton(const GameButtonRect&) { return false; }
    virtual bool deleteButton(std::size_t) { return false; }
    virtual bool modifyButton(const GameButtonEdit&) { return false; }
    virtual void setFrame(const uint8_t*, uint32_t, uint32_t, uint32_t) {}
    virtual std::optional<std::string> currentMapKey() const = 0;
    virtual std::optional<std::string> currentMapName() const = 0;
    virtual std::optional<GamePosition> currentPosition() const = 0;
};
