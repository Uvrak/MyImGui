// Standalone test: link the MM3 and DosBoxX libraries with this MemoryReader
// substitute. No emulator IPC is opened and no live save/configuration is used.
#include "../MM3GameModule.h"
#include "../../DosBoxX/Controller.h"
#include <SDL3/SDL.h>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>

namespace Fixture
{
    std::vector<uint8_t> memory(0x40000);
    uint64_t snapshot = 0;
}

namespace DosBoxMemoryTools
{
    MemoryReader::MemoryReader() = default;
    MemoryReader::~MemoryReader() = default;
    bool MemoryReader::tryOpen() { return true; }
    bool MemoryReader::readSnapshot() { return true; }
    bool MemoryReader::isOpen() const { return true; }
    uint64_t MemoryReader::snapshotId() const { return Fixture::snapshot; }
    const std::vector<uint8_t>& MemoryReader::memory() const { return Fixture::memory; }
    void MemoryReader::close() {}
}

using Module = MightAndMagic3::MM3GameModule;
using namespace std::chrono_literals;

struct InventoryScenario
{
    static constexpr int PortraitY = 330;
    DosBoxX::Controller controller;
    DosBoxX::NamedPipeClient pipe{"unused-inventory-test"};
    std::filesystem::path config = std::filesystem::temp_directory_path() /
        ("mm3-inventory-test-" + std::to_string(GetCurrentProcessId()) + ".cfg");
    std::vector<uint8_t> frame = std::vector<uint8_t>(640 * 400 * 4);
    std::unique_ptr<Module> module;
    std::vector<GameButtonPoint> clicks;
    int page = 0, transfers = 0;
    bool ignoreNextPortrait = false;
    bool ignoreNextItemClick = false;
    std::chrono::steady_clock::time_point deselectedAt{};

    static uint8_t& item(int character, int slot)
    {
        return Fixture::memory[0x2BFEE + character * 0x12F + slot];
    }
    static void word(int address, int value)
    {
        Fixture::memory[address] = static_cast<uint8_t>(value);
        Fixture::memory[address + 1] = static_cast<uint8_t>(value >> 8);
    }
    static int word(int address)
    {
        return Fixture::memory[address] | (Fixture::memory[address + 1] << 8);
    }
    static int character() { return Fixture::memory[0x2068E]; }
    static void select(int index)
    {
        word(0x304D0, index);
        word(0x304D2, index);
    }
    static GameButtonPoint portrait(int slot) { return {50 + slot * 75, PortraitY}; }

    InventoryScenario(int preferred, std::initializer_list<int> targetItems,
        bool actionButtons = false)
    {
        Fixture::memory.assign(Fixture::memory.size(), 0);
        for (int slot = 0; slot < 3; ++slot)
            Fixture::memory[0x2BF12 + slot * 0x12F] = 'A' + slot;
        item(0, 0) = 11;
        item(0, 4) = 12;
        select(4);
        for (int slot : targetItems) item(1, slot) = 20 + slot;
        {
            std::ofstream out(config);
            out << "inventory_selection 0 4\ninventory_selection 1 " << preferred << '\n';
            // Inventory button mode can also contain portrait rectangles.
            out << "window Inventory\n";
            if (actionButtons)
            {
                out << "button 30 219 46 37\nbutton 107 219 46 37\n"
                    << "button 182 218 47 38\nbutton 258 218 47 38\n"
                    << "button 334 218 47 38\nbutton 410 218 47 38\n";
            }
            else out << "button 100 300 50 60\n";
            for (int slot = 0; slot < 3; ++slot)
                out << "portrait " << 25 + slot * 75 << " 300 50 60\n";
        }
        module = std::make_unique<Module>(controller, pipe, std::nullopt, config.string());
        for (const auto& pixel : MightAndMagic3::ScreenSignatures::inventory().pixels)
        {
            const int offset = (pixel.y * 640 + pixel.x) * 4;
            frame[offset] = pixel.b;
            frame[offset + 1] = pixel.g;
            frame[offset + 2] = pixel.r;
        }
        module->setFrame(frame.data(), 640, 400, 640 * 4);
        tick(); // Opening the view queues selection; the portrait must cancel it.
    }
    ~InventoryScenario()
    {
        module.reset();
        std::filesystem::remove(config);
    }
    void tick()
    {
        ++Fixture::snapshot;
        module->update();
    }
    void apply(GameButtonPoint point)
    {
        clicks.push_back(point);
        if (point.y == PortraitY)
        {
            const int target = (point.x - 50) / 75;
            const int active = word(0x304D2);
            if (active != 0xFFFF)
            {
                // MM3's documented transfer rule: a selected item + portrait.
                int free = 0;
                while (free < 18 && item(target, free)) ++free;
                assert(free < 18);
                item(target, free) = item(character(), active);
                item(character(), active) = 0;
                ++transfers;
            }
            else
            {
                if (ignoreNextPortrait)
                {
                    ignoreNextPortrait = false;
                    return;
                }
                // The real game ignores the first portrait when it follows
                // item deselection too closely (observed in runtime logs).
                if (deselectedAt != std::chrono::steady_clock::time_point{} &&
                    std::chrono::steady_clock::now() - deselectedAt < 1500ms)
                    return;
                Fixture::memory[0x2068E] = static_cast<uint8_t>(target);
                page = 0;
            }
            word(0x304D2, 0xFFFF);
        }
        else if (point.x >= 182 && point.x < 457 &&
            point.y >= 218 && point.y < 256)
            word(0x304D2, 0xFFFF); // MM3 clears focus after an item action.
        else if (point.y == 236)
            page = 1 - page;
        else
        {
            if (ignoreNextItemClick)
            {
                ignoreNextItemClick = false;
                return;
            }
            const int index = page * 9 + (point.y - 45) / 18;
            assert(point.x == 85 && index >= 0 && index < 18 && item(character(), index));
            const bool alreadySelected = word(0x304D2) == index;
            word(0x304D0, index);
            word(0x304D2, alreadySelected ? 0xFFFF : index);
            if (alreadySelected) deselectedAt = std::chrono::steady_clock::now();
        }
    }
    void directPortrait(int target)
    {
        const auto point = portrait(target);
        if (!module->onDosBoxMouseClick(point)) apply(point);
    }
    template<class Done> void runUntil(Done done)
    {
        const auto deadline = std::chrono::steady_clock::now() + 6s;
        while (!done() && std::chrono::steady_clock::now() < deadline)
        {
            tick();
            if (const auto click = module->takeButtonClick()) apply(*click);
            std::this_thread::sleep_for(20ms);
        }
        tick();
        assert(done());
    }
    void expectSelection(int target, int index)
    {
        runUntil([&] { return character() == target && word(0x304D2) == index; });
        assert(module->characterInventories()[target].selectedIndex ==
            (index == 0xFFFF ? -1 : index));
    }
};

int main()
{
    for (int action = 2; action <= 4; ++action)
    for (int mode = 0; mode < 3; ++mode)
    {
        InventoryScenario test(2, {0, 2}, true);
        InventoryScenario::word(0x304D2, 0xFFFF);
        test.expectSelection(0, 4);
        const GameButtonPoint button{action == 2 ? 205 : action == 3 ? 281 : 357,
            237};
        if (mode == 0)
        {
            assert(!test.module->onDosBoxMouseClick(button));
            test.apply(button);
        }
        else if (mode == 1)
        {
            test.module->keyDown(SDLK_END);
            for (int i = 0; i < action; ++i) test.module->keyDown(SDLK_RIGHT);
            const auto selected = test.module->selectedButtonRect();
            assert(selected && static_cast<int>(selected->x) ==
                (action == 2 ? 182 : action == 3 ? 258 : 334));
            test.module->keyDown(SDLK_RETURN);
            const auto click = test.module->takeButtonClick();
            assert(click && click->x == button.x && click->y == button.y);
            test.apply(*click);
        }
        else
        {
            test.module->keyDown(SDLK_RETURN);
            assert(test.module->inventoryMenuTitle() == "Item");
            const int menuIndex = action == 4 ? 3 : action - 2;
            for (int i = 0; i < menuIndex; ++i) test.module->keyDown(SDLK_DOWN);
            test.module->keyDown(SDLK_RETURN);
            const auto click = test.module->takeButtonClick();
            assert(click);
            test.apply(*click);
        }
        assert(InventoryScenario::word(0x304D2) == 0xFFFF);
        test.expectSelection(0, 4);
        assert(InventoryScenario::item(0, 4) == 12);
        assert(test.clicks.back().x == 85 && test.clicks.back().y == 117);
    }
    {
        InventoryScenario test(2, {0, 2}, true);
        InventoryScenario::word(0x304D2, 0xFFFF);
        test.expectSelection(0, 4);
        assert(!test.module->onDosBoxMouseClick({357, 237}));
        test.apply({357, 237});
        InventoryScenario::item(0, 4) = 0; // Benutzen consumed the selected item.
        test.expectSelection(0, 0);
        assert(test.clicks.back().x == 85 && test.clicks.back().y == 45);
    }
    for (int mode = 0; mode < 2; ++mode)
    {
        InventoryScenario test(2, {0, 2});
        InventoryScenario::item(0, 3) = 13;
        InventoryScenario::item(0, 5) = 14;
        test.module->keyDown(mode == 0 ? SDLK_DELETE : SDLK_END);
        const auto button = test.module->selectedButtonRect();
        assert(button);
        test.ignoreNextItemClick = mode == 0;

        test.module->keyDown(SDLK_DOWN);
        test.runUntil([&] { return InventoryScenario::word(0x304D2) == 5; });
        assert(test.module->characterInventories()[0].selectedIndex == 5);
        assert(test.clicks.back().x == 85 && test.clicks.back().y == 135);
        assert(test.module->selectedButtonRect()->x == button->x);

        test.module->keyDown(SDLK_UP);
        test.runUntil([&] { return InventoryScenario::word(0x304D2) == 4; });
        assert(test.module->characterInventories()[0].selectedIndex == 4);
        assert(test.clicks.back().x == 85 && test.clicks.back().y == 117);
        assert(test.module->selectedButtonRect()->x == button->x);
    }
    // Normal mouse, portrait navigation, and a configured button all use protection.
    for (int mode = 0; mode < 3; ++mode)
    {
        InventoryScenario test(2, {0, 2, 6});
        const auto before = Fixture::memory;
        if (mode == 0) test.directPortrait(1);
        else
        {
            test.module->keyDown(mode == 1 ? SDLK_DELETE : SDLK_END);
            if (mode == 1) test.module->keyDown(SDLK_RIGHT);
            test.module->keyDown(SDLK_RETURN);
        }
        test.expectSelection(1, 2);
        assert(test.transfers == 0);
        assert(test.clicks.size() == 3); // Deselect, single portrait, restore.
        if (mode == 0)
        {
            test.module->keyDown(SDLK_DELETE); // Enter portrait mode after the switch.
            const auto selected = test.module->selectedButtonRect();
            assert(selected && static_cast<int>(selected->x) == 100);
        }
        if (mode == 1)
        {
            test.module->keyDown(SDLK_RETURN); // Same portrait, second Return.
            assert(test.module->inventoryMenuTitle() == "Item");
            assert(!test.module->takeButtonClick());
        }
        assert(test.module->characterInventories()[0].selectedIndex == 4);
        for (int character = 0; character < 3; ++character)
            for (int slot = 0; slot < 18; ++slot)
                assert(InventoryScenario::item(character, slot) ==
                    before[0x2BFEE + character * 0x12F + slot]);
    }
    {
        InventoryScenario test(17, {0, 5, 12});
        test.directPortrait(1);
        test.expectSelection(1, 12); // Fallback includes page two.
        test.directPortrait(0);
        test.expectSelection(0, 4); // Original remembered index survives deselection.
        assert(test.transfers == 0);
    }
    {
        InventoryScenario test(2, {0, 2});
        test.ignoreNextPortrait = true;
        test.directPortrait(1); // A single user click starts the whole sequence.
        test.expectSelection(1, 2);
        assert(test.transfers == 0 && test.clicks.size() == 4);
        assert(test.clicks[1].x == test.clicks[2].x); // Safe confirmed retry.
    }
    {
        InventoryScenario test(2, {0, 2});
        test.module->keyDown(SDLK_DELETE);
        test.module->keyDown(SDLK_RIGHT);
        test.module->keyDown(SDLK_RETURN);
        test.runUntil([&] { return test.clicks.size() == 1; }); // Deselect sent.
        test.module->keyDown(SDLK_RETURN); // Impatient repeat must not restart it.
        test.expectSelection(1, 2);
        assert(test.transfers == 0 && test.clicks.size() == 3);
    }
    {
        InventoryScenario test(5, {});
        test.directPortrait(1);
        test.expectSelection(1, 0xFFFF);
        test.module->keyDown(SDLK_DELETE);
        const auto selected = test.module->selectedButtonRect();
        assert(selected && static_cast<int>(selected->x) == 100);
        test.module->keyDown(SDLK_RETURN);
        assert(test.module->inventoryMenuTitle().empty()); // No item to act on.
        assert(!test.module->takeButtonClick());
        assert(test.module->characterInventories()[1].selectedIndex == -1);
        test.directPortrait(0); // An empty source needs no deselection click.
        test.expectSelection(0, 4);
        assert(test.transfers == 0);
    }
    for (int portraitMode = 0; portraitMode < 2; ++portraitMode)
    {
        InventoryScenario test(2, {0, 2});
        // Let the initial queued selection complete before opening the action menu.
        InventoryScenario::word(0x304D2, 0xFFFF);
        test.expectSelection(0, 4);
        if (portraitMode) test.module->keyDown(SDLK_DELETE);
        test.module->keyDown(SDLK_RETURN);
        assert(test.module->inventoryMenuTitle() == "Item");
        assert(!test.module->takeButtonClick()); // Current portrait opens the menu.
        test.module->keyDown(SDLK_DOWN);
        test.module->keyDown(SDLK_DOWN);
        test.module->keyDown(SDLK_RETURN);
        assert(test.module->inventoryMenuTitle() == "Geben an");
        test.module->keyDown(SDLK_RETURN);
        test.runUntil([&] { return test.transfers == 1; });
        assert(test.clicks.back().y == InventoryScenario::PortraitY);
        assert(test.transfers == 1 && InventoryScenario::character() == 0);
        assert(InventoryScenario::item(0, 4) == 0 && InventoryScenario::item(1, 1) == 12);
        test.expectSelection(0, 0); // Remaining item is selected after giving.
    }
    std::cout << "Inventory portrait integration tests passed (21 scenarios)\n";
}
