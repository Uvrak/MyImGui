#include "MM3GameModule.h"
#include <SDL3/SDL.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <Windows.h>
#include <limits>
#include <cmath>
#include "../MouseLatencyTrace.h"

namespace MightAndMagic3
{
    namespace
    {
        constexpr int InventoryRowsPerPage = 9;
        constexpr auto InventoryTransitionDelay = std::chrono::milliseconds(1200);
        constexpr int InventoryPageCount =
            (CharacterInventory::SlotCount + InventoryRowsPerPage - 1) /
            InventoryRowsPerPage;
    }

    MM3GameModule::MM3GameModule(DosBoxX::Controller& controller,
                                 DosBoxX::NamedPipeClient& pipe,
                                 std::optional<std::size_t> mapIdAddress,
                                 const std::string& buttonConfigPath)
        : m_launcher(controller, pipe),
          m_mapDetector(m_memoryReader, mapIdAddress),
          m_stateReader(m_memoryReader),
          m_buttonConfigPath(buttonConfigPath)
    {
        m_keyBindings.setDefaultKey(KeyAction::ToggleButtonMode, SDLK_END);
        m_keyBindings.setDefaultKey(KeyAction::TogglePortraitMode, SDLK_DELETE);
        m_keyBindings.setDefaultKey(KeyAction::ButtonUp, SDLK_UP);
        m_keyBindings.setDefaultKey(KeyAction::ButtonDown, SDLK_DOWN);
        m_keyBindings.setDefaultKey(KeyAction::ButtonLeft, SDLK_LEFT);
        m_keyBindings.setDefaultKey(KeyAction::ButtonRight, SDLK_RIGHT);
        m_keyBindings.setDefaultKey(KeyAction::ActivateButton, SDLK_RETURN);
        loadButtons(buttonConfigPath);
    }

    void MM3GameModule::start() { m_launcher.start(); }

    bool MM3GameModule::blockDirectDosBoxKeyboard() const { return buttonEditingActive(); }

    std::string MM3GameModule::buttonViewName() const
    {
        if (m_portraitMode) return "Portrait";
        constexpr const char* names[] = {
            "Unknown", "Loading Screen", "Load Game", "Main Game",
            "Character Screen", "Inventory"
        };
        return names[m_activeButtons];
    }

    const std::vector<GameButtonRect>& MM3GameModule::buttons() const
    {
        return m_portraitMode ? m_portraitControls : m_buttons[m_activeButtons];
    }

    std::vector<GameButtonRect>& MM3GameModule::buttons()
    {
        return m_portraitMode ? m_portraitControls : m_buttons[m_activeButtons];
    }

    std::vector<std::string> MM3GameModule::activeViewNames() const
    {
        auto names = m_detectedViews;
        if (names.empty()) names.push_back("Unknown");
        if (m_portraitMode) names.push_back("Portrait");
        return names;
    }

    void MM3GameModule::loadButtons(const std::string& path)
    {
        std::ifstream input(path);
        std::string line;
        int window = 0;
        while (std::getline(input, line))
        {
            if (line.rfind("window ", 0) == 0)
            {
                const auto name = line.substr(7);
                window = name == "Loading Screen" ? 1 :
                    name == "Load Game" ? 2 :
                    name == "Main Game" ? 3 :
                    name == "Character Screen" ? 4 :
                    name == "Inventory" ? 5 : 0;
            }
            else if (line.rfind("inventory_selection ", 0) == 0)
            {
                std::istringstream fields(line.substr(20));
                int character = -1, index = -1;
                if (fields >> character >> index && character >= 0 &&
                    character < static_cast<int>(m_characterInventories.size()) &&
                    index >= 0 && index < CharacterInventory::SlotCount)
                    m_characterInventories[character].selectedIndex = index;
            }
            else if (line == "portraits")
                continue;
            else if (line.rfind("button ", 0) == 0 || line.rfind("portrait ", 0) == 0)
            {
                const bool portrait = line.rfind("portrait ", 0) == 0;
                GameButtonRect rect;
                std::istringstream fields(line.substr(portrait ? 9 : 7));
                int r = 255, g = 255, b = 0, a = 255;
                if (!(fields >> rect.x >> rect.y >> rect.width >> rect.height) ||
                    rect.width <= 0 || rect.height <= 0) continue;
                if (fields >> r >> g >> b >> a)
                {
                    rect.r = static_cast<uint8_t>(r);
                    rect.g = static_cast<uint8_t>(g);
                    rect.b = static_cast<uint8_t>(b);
                    rect.a = static_cast<uint8_t>(a);
                }
                fields >> rect.dosKey;
                (portrait ? m_portraitControls : m_buttons[window]).push_back(rect);
            }
            else if (!line.empty())
                m_otherButtonConfigLines.push_back(line);
        }
    }

    bool MM3GameModule::saveButtons() const
    {
        const std::filesystem::path target(m_buttonConfigPath);
        auto temporary = target;
        temporary += ".tmp";
        std::ofstream output(temporary, std::ios::trunc);
        if (!output) return false;
        output << std::setprecision(9);
        constexpr const char* names[] = {
            "Unknown", "Loading Screen", "Load Game", "Main Game",
            "Character Screen", "Inventory"
        };
        for (size_t window = 0; window < m_buttons.size(); ++window)
        {
            output << "window " << names[window] << '\n';
            for (const auto& rect : m_buttons[window])
            {
                output << "button " << rect.x << ' ' << rect.y << ' '
                    << rect.width << ' ' << rect.height << ' '
                    << static_cast<int>(rect.r) << ' '
                    << static_cast<int>(rect.g) << ' '
                    << static_cast<int>(rect.b) << ' '
                    << static_cast<int>(rect.a);
                if (!rect.dosKey.empty()) output << ' ' << rect.dosKey;
                output << '\n';
            }
        }
        for (const auto& line : m_otherButtonConfigLines)
            output << line << '\n';
        output << "portraits\n";
        for (const auto& rect : m_portraitControls)
        {
            output << "portrait " << rect.x << ' ' << rect.y << ' '
                << rect.width << ' ' << rect.height << ' '
                << static_cast<int>(rect.r) << ' ' << static_cast<int>(rect.g) << ' '
                << static_cast<int>(rect.b) << ' ' << static_cast<int>(rect.a);
            if (!rect.dosKey.empty()) output << ' ' << rect.dosKey;
            output << '\n';
        }
        for (std::size_t character = 0; character < m_characterInventories.size(); ++character)
            output << "inventory_selection " << character << ' '
                << m_characterInventories[character].selectedIndex << '\n';
        output.close();
        if (!output) return false;
        return MoveFileExW(temporary.c_str(), target.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    }

    bool MM3GameModule::addButton(const GameButtonRect& rect)
    {
        if (!buttonEditingActive() || !buttonViewAvailable() ||
            rect.width < 2 || rect.height < 2) return false;
        auto& viewButtons = buttons();
        viewButtons.push_back(rect);
        if (!saveButtons())
        {
            viewButtons.pop_back();
            return false;
        }
        m_selectedButton = static_cast<int>(viewButtons.size()) - 1;
        return true;
    }

    bool MM3GameModule::modifyButton(const GameButtonEdit& edit)
    {
        if (!buttonEditingActive() || !m_showButtonOutlines || !buttonViewAvailable() ||
            edit.rect.width < 2 || edit.rect.height < 2) return false;
        auto& viewButtons = buttons();
        if (edit.index >= viewButtons.size()) return false;
        const auto previous = viewButtons[edit.index];
        viewButtons[edit.index] = edit.rect;
        if (!saveButtons())
        {
            viewButtons[edit.index] = previous;
            return false;
        }
        m_selectedButton = static_cast<int>(edit.index);
        m_pendingButtonClick = false;
        return true;
    }

    bool MM3GameModule::deleteButton(std::size_t index)
    {
        if (!buttonEditingActive() || !m_showButtonOutlines || !buttonViewAvailable())
            return false;
        auto& viewButtons = buttons();
        if (index >= viewButtons.size()) return false;
        const auto removed = viewButtons[index];
        viewButtons.erase(viewButtons.begin() + index);
        if (!saveButtons())
        {
            viewButtons.insert(viewButtons.begin() + index, removed);
            return false;
        }
        if (viewButtons.empty()) m_selectedButton = -1;
        else if (m_selectedButton > static_cast<int>(index)) --m_selectedButton;
        else if (m_selectedButton >= static_cast<int>(viewButtons.size()))
            m_selectedButton = static_cast<int>(viewButtons.size()) - 1;
        m_pendingButtonClick = false;
        return true;
    }

    std::optional<GameButtonRect> MM3GameModule::selectedButtonRect() const
    {
        if (!buttonEditingActive() || m_selectedButton < 0 ||
            m_selectedButton >= static_cast<int>(buttons().size()))
            return std::nullopt;
        return buttons()[m_selectedButton];
    }

    std::vector<GameButtonRect> MM3GameModule::buttonRects() const
    {
        return buttonEditingActive() && m_showButtonOutlines && buttonViewAvailable()
            ? buttons() : std::vector<GameButtonRect>{};
    }

    std::optional<GameButtonPoint> MM3GameModule::takeButtonClick()
    {
        const auto now = std::chrono::steady_clock::now();
        if (!m_pendingInventoryClicks.empty() &&
            now >= m_nextInventoryClickTime)
        {
            const auto click = m_pendingInventoryClicks.front();
            m_pendingInventoryClicks.pop_front();
            if (m_inventoryActionSelection && !m_inventoryActionSelection->dispatched)
            {
                m_inventoryActionSelection->dispatched = true;
                m_inventoryActionSelection->snapshot = m_memoryReader.snapshotId();
            }
            const bool changesPage = (click.x == 127 || click.x == 53) && click.y == 236;
            if (click.x == 127 && click.y == 236)
                m_inventoryPage = (m_inventoryPage + 1) % InventoryPageCount;
            else if (click.x == 53 && click.y == 236)
                m_inventoryPage = (m_inventoryPage + InventoryPageCount - 1) %
                    InventoryPageCount;
            if (m_inventorySelectionClick &&
                click.x == m_inventorySelectionClick->x &&
                click.y == m_inventorySelectionClick->y)
            {
                m_inventorySelectionClickPending = false;
                m_inventorySelectionPendingUntil = now + std::chrono::milliseconds(1100);
            }
            // MM3 must finish changing pages before receiving the item click.
            // Keep that click queued instead of requiring another key press.
            m_nextInventoryClickTime = now + (changesPage ?
                InventoryTransitionDelay : std::chrono::milliseconds(150));
            return click;
        }
        if (!m_pendingButtonClick) return std::nullopt;
        m_pendingButtonClick = false;
        const auto rect = selectedButtonRect();
        if (!rect) return std::nullopt;
        return GameButtonPoint{
            static_cast<int>(rect->x + rect->width * 0.5f),
            static_cast<int>(rect->y + rect->height * 0.5f)
        };
    }

    void MM3GameModule::setFrame(const uint8_t* pixels, uint32_t width,
                                 uint32_t height, uint32_t pitch)
    {
        m_framePixels = pixels;
        m_frameWidth = width;
        m_frameHeight = height;
        m_framePitch = pitch;
    }

    bool MM3GameModule::matchesScreen(const ScreenSignature& signature) const
    {
        if (!m_framePixels || !m_frameWidth || !m_frameHeight ||
            m_framePitch < m_frameWidth * 3 || signature.pixels.empty())
            return false;
        const uint32_t bytesPerPixel = m_framePitch / m_frameWidth;
        for (const auto& expected : signature.pixels)
        {
            if (expected.x < 0 || expected.y < 0 ||
                static_cast<uint32_t>(expected.x) >= m_frameWidth ||
                static_cast<uint32_t>(expected.y) >= m_frameHeight)
                return false;
            const auto* pixel = m_framePixels +
                static_cast<size_t>(expected.y) * m_framePitch +
                static_cast<size_t>(expected.x) * bytesPerPixel;
            if (pixel[2] != expected.r || pixel[1] != expected.g ||
                pixel[0] != expected.b) return false;
        }
        return true;
    }

    void MM3GameModule::keyDown(int key)
    {
        const auto matches = [this, key](KeyAction action) {
            const int configured = m_keyBindings.key(action);
            return configured != 0 && configured == key;
        };

        if (matches(KeyAction::ToggleButtonMode))
        {
            m_inventoryActionSelection.reset();
            m_buttonMode = !m_buttonMode;
            m_inventoryMenu = InventoryMenu::None;
            m_pendingInventoryClicks.clear();
            m_inventorySelectionClickPending = false;
            if (m_buttonMode) m_portraitMode = false;
            m_selectedButton = m_buttonMode ? 0 : -1;
            if (!m_buttonMode) m_showButtonOutlines = false;
            m_pendingButtonClick = false;
            return;
        }
        if (matches(KeyAction::TogglePortraitMode))
        {
            m_inventoryActionSelection.reset();
            m_portraitMode = !m_portraitMode;
            m_inventoryMenu = InventoryMenu::None;
            m_pendingInventoryClicks.clear();
            m_inventorySelectionClickPending = false;
            if (m_portraitMode) m_buttonMode = false;
            m_selectedButton = m_portraitMode && !buttons().empty() ? 0 : -1;
            if (!m_portraitMode) m_showButtonOutlines = false;
            m_pendingButtonClick = false;
            return;
        }
        if (buttonEditingActive() && matches(KeyAction::ActivateButton))
        {
            m_inventoryActionSelection.reset();
            m_pendingButtonClick = true;
            return;
        }
        if (m_inventoryVisible && !buttonEditingActive() &&
            m_inventoryMenu != InventoryMenu::None)
        {
            const auto entries = inventoryMenuEntries();
            if (matches(KeyAction::ButtonUp) && m_menuSelection > 0) --m_menuSelection;
            else if (matches(KeyAction::ButtonDown) &&
                m_menuSelection + 1 < static_cast<int>(entries.size())) ++m_menuSelection;
            else if (key == SDLK_ESCAPE ||
                (m_inventoryMenu == InventoryMenu::Party && matches(KeyAction::ButtonLeft)))
            {
                const bool returningFromParty = m_inventoryMenu == InventoryMenu::Party;
                m_inventoryMenu = m_inventoryMenu == InventoryMenu::Party
                    ? InventoryMenu::Actions : InventoryMenu::None;
                m_menuSelection = returningFromParty ? 2 : 0;
            }
            else if (matches(KeyAction::ActivateButton))
            {
                if (m_inventoryMenu == InventoryMenu::Actions && m_menuSelection == 2)
                {
                    m_partyMenuSlots.clear();
                    for (int slot = 0; slot < 8; ++slot)
                        if (slot != m_inventoryCharacter && !characterName(slot).empty())
                            m_partyMenuSlots.push_back(slot);
                    m_inventoryMenu = InventoryMenu::Party;
                    m_menuSelection = 0;
                }
                else
                {
                    // Use the same mouse queue and release handling as all MM3 clicks.
                    if (!selectedInventoryItemActive())
                    {
                        m_inventoryMenu = InventoryMenu::None;
                        return;
                    }
                    std::optional<GameButtonPoint> actionClick;
                    if (m_inventoryMenu == InventoryMenu::Party)
                    {
                        if (m_menuSelection >= 0 &&
                            m_menuSelection < static_cast<int>(m_partyMenuSlots.size()))
                        {
                            const int slot = m_partyMenuSlots[m_menuSelection];
                            if (slot != m_inventoryCharacter && slot >= 0 &&
                                slot < static_cast<int>(m_portraitControls.size()) &&
                                !characterName(slot).empty())
                            {
                                const auto& portrait = m_portraitControls[slot];
                                actionClick = GameButtonPoint{
                                    static_cast<int>(portrait.x + portrait.width * 0.5f),
                                    static_cast<int>(portrait.y + portrait.height * 0.5f)};
                            }
                        }
                    }
                    else
                    {
                        switch (m_menuSelection)
                        {
                        case 0: actionClick = GameButtonPoint{202, 241}; break;
                        case 1: actionClick = GameButtonPoint{277, 238}; break;
                        case 3: actionClick = GameButtonPoint{350, 239}; break;
                        case 4: actionClick = GameButtonPoint{435, 237}; break;
                        }
                    }
                    if (actionClick)
                    {
                        m_inventoryActionSelection = InventoryActionSelection{
                            m_inventoryCharacter,
                            m_characterInventories[m_inventoryCharacter].selectedIndex};
                        m_pendingInventoryClicks.clear();
                        m_inventorySelectionClickPending = false;
                        m_pendingInventoryClicks.push_back(*actionClick);
                        m_nextInventoryClickTime = {};
                        m_pendingButtonClick = false;
                    }
                    m_inventoryMenu = InventoryMenu::None;
                }
            }
            return;
        }
        if (m_inventoryVisible && !buttonEditingActive() &&
            matches(KeyAction::ActivateButton))
        {
            if (selectedInventoryItemActive())
            {
                m_inventoryMenu = InventoryMenu::Actions;
                m_menuSelection = 0;
            }
            return;
        }
        if (m_inventoryVisible && !buttonEditingActive() &&
            (matches(KeyAction::ButtonUp) || matches(KeyAction::ButtonDown)))
        {
            if (m_inventoryCharacter < 0) return;
            m_inventoryActionSelection.reset();
            auto& index = m_characterInventories[m_inventoryCharacter].selectedIndex;
            const int next = index + (matches(KeyAction::ButtonDown) ? 1 : -1);
            if (next < 0 || next >= CharacterInventory::SlotCount) return;
            constexpr std::size_t firstItemIds = 0x2BFEE;
            constexpr std::size_t characterRecordSize = 0x12F;
            const auto itemAddress = firstItemIds +
                static_cast<std::size_t>(m_inventoryCharacter) * characterRecordSize + next;
            const auto& memory = m_memoryReader.memory();
            if (itemAddress >= memory.size() || memory[itemAddress] == 0) return;
            index = next;
            queueInventorySelection(index, false);
            m_pendingButtonClick = false;
            // A memory snapshot may still describe the selection before this click.
            m_inventorySelectionPendingUntil = std::chrono::steady_clock::now() +
                std::chrono::milliseconds(1000);
            saveButtons();
            return;
        }

        if (!buttonEditingActive()) return;
        if (key == SDLK_K)
        {
            m_showButtonOutlines = !m_showButtonOutlines;
            return;
        }

        const auto count = static_cast<int>(buttons().size());
        if (count == 0) return;
        int dx = 0, dy = 0;
        if (matches(KeyAction::ButtonUp)) dy = -1;
        else if (matches(KeyAction::ButtonDown)) dy = 1;
        else if (matches(KeyAction::ButtonLeft)) dx = -1;
        else if (matches(KeyAction::ButtonRight)) dx = 1;
        if (!dx && !dy) return;
        if (m_selectedButton < 0 || m_selectedButton >= count)
        {
            m_selectedButton = 0;
            return;
        }

        if (dx != 0 && (m_portraitMode || m_activeButtons == 5))
        {
            const auto& current = buttons()[m_selectedButton];
            const float centerX = current.x + current.width * 0.5f;
            float nearest = (std::numeric_limits<float>::max)();
            int next = m_selectedButton;
            for (int i = 0; i < count; ++i)
            {
                const auto& candidate = buttons()[i];
                const float delta = dx * (candidate.x + candidate.width * 0.5f - centerX);
                if (delta > 2.0f && delta < nearest)
                {
                    nearest = delta;
                    next = i;
                }
            }
            m_selectedButton = next;
            return;
        }

        const auto& current = buttons()[m_selectedButton];
        const float centerX = current.x + current.width * 0.5f;
        const float centerY = current.y + current.height * 0.5f;
        float bestScore = (std::numeric_limits<float>::max)();
        int best = m_selectedButton;
        for (int i = 0; i < count; ++i)
        {
            if (i == m_selectedButton) continue;
            const auto& candidate = buttons()[i];
            const float deltaX = candidate.x + candidate.width * 0.5f - centerX;
            const float deltaY = candidate.y + candidate.height * 0.5f - centerY;
            const float forward = dx * deltaX + dy * deltaY;
            if (forward <= 2.0f) continue;
            const float sideways = std::abs(dy * deltaX - dx * deltaY);
            const float score = forward + 4.0f * sideways;
            if (score < bestScore)
            {
                bestScore = score;
                best = i;
            }
        }
        m_selectedButton = best;
    }

    void MM3GameModule::onDosBoxMouseClick(GameButtonPoint point)
    {
        if (!m_inventoryVisible || m_inventoryCharacter < 0) return;
        m_inventoryActionSelection.reset();
        const auto& inventoryButtons = m_buttons[5];
        const auto hitsButton = [&](std::size_t index) {
            if (index >= inventoryButtons.size()) return false;
            const auto& rect = inventoryButtons[index];
            return point.x >= rect.x && point.x < rect.x + rect.width &&
                point.y >= rect.y && point.y < rect.y + rect.height;
        };
        if (hitsButton(0) || hitsButton(1))
        {
            m_pendingInventoryClicks.clear();
            m_inventorySelectionClickPending = false;
            m_inventoryPage = (m_inventoryPage +
                (hitsButton(1) ? 1 : InventoryPageCount - 1)) % InventoryPageCount;
            return;
        }

        // The nine visible item rows form one inventory page.
        if (point.x < 30 || point.x > 320 || point.y < 36 || point.y >= 198)
            return;
        const int visibleRow = (point.y - 36) / 18;
        const int absoluteIndex = m_inventoryPage * InventoryRowsPerPage + visibleRow;
        if (absoluteIndex >= CharacterInventory::SlotCount) return;
        constexpr std::size_t firstItemIds = 0x2BFEE;
        constexpr std::size_t characterRecordSize = 0x12F;
        const auto itemAddress = firstItemIds +
            static_cast<std::size_t>(m_inventoryCharacter) * characterRecordSize +
            absoluteIndex;
        const auto& memory = m_memoryReader.memory();
        if (itemAddress >= memory.size() || memory[itemAddress] == 0) return;

        m_pendingInventoryClicks.clear();
        m_inventorySelectionClick = point;
        m_inventorySelectionClickPending = false;
        m_inventorySelectionRestored = false;
        m_inventoryMenu = InventoryMenu::None;
        m_inventoryVisibleRow = visibleRow;
        m_characterInventories[m_inventoryCharacter].selectedIndex = absoluteIndex;
        m_inventorySelectionPendingUntil = std::chrono::steady_clock::now() +
            std::chrono::milliseconds(1000);
        saveButtons();
    }

    void MM3GameModule::rememberInventorySelection()
    {
        // Same MM3 selection fields and mirrors used by ItemSource.
        constexpr std::size_t characterAddress = 0x2068E;
        constexpr std::size_t characterMirrorAddress = 0x304F4;
        constexpr std::size_t indexAddress = 0x304D0;
        constexpr std::size_t indexMirrorAddress = 0x304D2;
        const auto& memory = m_memoryReader.memory();
        if (memory.size() <= characterMirrorAddress) return;
        const auto readWord = [&](std::size_t address) {
            return static_cast<int>(memory[address]) |
                (static_cast<int>(memory[address + 1]) << 8);
        };
        const auto character = memory[characterAddress];
        const int index = readWord(indexAddress);
        const std::uint64_t traceState = static_cast<std::uint64_t>(character) |
            (static_cast<std::uint64_t>(memory[characterMirrorAddress]) << 8) |
            (static_cast<std::uint64_t>(index) << 16) |
            (static_cast<std::uint64_t>(readWord(indexMirrorAddress)) << 32);
        if (traceState != m_inventorySelectionTraceState)
        {
            m_inventorySelectionTraceState = traceState;
            TraceGridBuilderMouse("MM3_SELECTION_STATE", traceState);
        }
        if (character >= m_characterInventories.size()) return;

        auto& selected = m_characterInventories[character].selectedIndex;
        if (m_inventoryCharacter != character)
        {
            m_inventoryActionSelection.reset();
            m_inventoryCharacter = character;
            m_inventoryPage = 0;
            m_inventoryVisibleRow = 0;
            m_inventoryMenu = InventoryMenu::None;
            queueInventorySelection(selected, true);
            m_pendingButtonClick = false;
            m_inventorySelectionPendingUntil = std::chrono::steady_clock::now() +
                std::chrono::milliseconds(1000);
            return;
        }
        if (m_inventoryActionSelection)
        {
            // Do not copy MM3's temporary deselection into the remembered index.
            // Restore only after a newer snapshot actually shows focus was lost.
            const auto action = *m_inventoryActionSelection;
            if (!action.dispatched || m_memoryReader.snapshotId() == action.snapshot ||
                selectedInventoryItemActive()) return;

            constexpr std::size_t firstItemIds = 0x2BFEE;
            constexpr std::size_t characterRecordSize = 0x12F;
            const auto occupied = [&](int slot) {
                const auto address = firstItemIds +
                    static_cast<std::size_t>(action.character) * characterRecordSize + slot;
                return slot >= 0 && slot < CharacterInventory::SlotCount &&
                    address < memory.size() && memory[address] != 0;
            };
            int target = action.index;
            if (!occupied(target))
            {
                target = -1;
                for (int slot = action.index + 1; slot < CharacterInventory::SlotCount; ++slot)
                    if (occupied(slot)) { target = slot; break; }
                if (target < 0)
                    for (int slot = action.index - 1; slot >= 0; --slot)
                        if (occupied(slot)) { target = slot; break; }
            }
            m_inventoryActionSelection.reset();
            if (target >= 0)
            {
                selected = target;
                queueInventorySelection(target, false);
                // Losing the selection is an intermediate step of the action.
                // Let MM3 finish it before sending the one restoration click.
                m_nextInventoryClickTime = std::chrono::steady_clock::now() +
                    InventoryTransitionDelay;
                saveButtons();
            }
            else
            {
                selected = 0;
                m_inventorySelectionClick.reset();
                saveButtons();
            }
            return;
        }
        // The character mirror can remain on the previous party slot even
        // with a visibly selected item. Only the item selection fields agree.
        if (index < 0 || index >= CharacterInventory::SlotCount ||
            index != readWord(indexMirrorAddress)) return;
        if (selected != index &&
            (!m_pendingInventoryClicks.empty() ||
             std::chrono::steady_clock::now() < m_inventorySelectionPendingUntil)) return;
        m_inventorySelectionPendingUntil = {};
        if (selected == index) return;
        const int previous = selected;
        selected = index;
        if (!saveButtons()) selected = previous;
    }

    bool MM3GameModule::selectedInventoryItemActive() const
    {
        if (!m_inventoryVisible || m_inventoryCharacter < 0 ||
            !m_pendingInventoryClicks.empty()) return false;

        constexpr std::size_t characterAddress = 0x2068E;
        constexpr std::size_t indexAddress = 0x304D0;
        constexpr std::size_t indexMirrorAddress = 0x304D2;
        constexpr std::size_t firstItemIds = 0x2BFEE;
        constexpr std::size_t characterRecordSize = 0x12F;
        const auto& memory = m_memoryReader.memory();
        if (indexMirrorAddress + 1 >= memory.size()) return false;
        const int index = m_characterInventories[m_inventoryCharacter].selectedIndex;
        const auto itemAddress = firstItemIds +
            static_cast<std::size_t>(m_inventoryCharacter) * characterRecordSize + index;
        if (itemAddress >= memory.size() || memory[itemAddress] == 0) return false;
        const auto readWord = [&](std::size_t address) {
            return static_cast<int>(memory[address]) |
                (static_cast<int>(memory[address + 1]) << 8);
        };
        return memory[characterAddress] == m_inventoryCharacter &&
            readWord(indexAddress) == index &&
            readWord(indexMirrorAddress) == index;
    }

    void MM3GameModule::queueInventorySelection(int index, bool restore)
    {
        constexpr GameButtonPoint nextPage{127, 236};
        constexpr GameButtonPoint previousPage{53, 236};
        m_pendingInventoryClicks.clear();
        const int targetPage = index / InventoryRowsPerPage;
        m_inventoryVisibleRow = index % InventoryRowsPerPage;
        for (int page = m_inventoryPage; page < targetPage; ++page)
            m_pendingInventoryClicks.push_back(nextPage);
        for (int page = m_inventoryPage; page > targetPage; --page)
            m_pendingInventoryClicks.push_back(previousPage);
        m_inventorySelectionClick = {85, 45 + 18 * m_inventoryVisibleRow};
        m_inventorySelectionClickPending = true;
        m_inventorySelectionRestored = restore;
        m_pendingInventoryClicks.push_back(*m_inventorySelectionClick);
        if (restore)
            // The inventory signature appears before MM3 finishes opening it.
            // Send one delayed click; repeating a click toggles the item off.
            m_nextInventoryClickTime = std::chrono::steady_clock::now() +
                InventoryTransitionDelay;
    }

    std::string MM3GameModule::characterName(int slot) const
    {
        if (slot < 0 || slot >= 8) return {};
        constexpr std::size_t firstCharacter = 0x2BF00;
        constexpr std::size_t recordSize = 0x12F;
        const auto& memory = m_memoryReader.memory();
        constexpr std::size_t nameOffset = 0x12;
        const auto start = firstCharacter + static_cast<std::size_t>(slot) * recordSize +
            nameOffset;
        if (start + 9 >= memory.size()) return {};
        std::string result;
        for (std::size_t i = 0; i < 10; ++i)
        {
            const auto value = memory[start + i];
            if (value == 0) break;
            if (value < 32 || value > 126) return {};
            result += static_cast<char>(value);
        }
        return result;
    }

    std::optional<std::string> MM3GameModule::inventoryDebugLine() const
    {
        if (!m_inventoryVisible || m_inventoryCharacter < 0) return std::nullopt;
        return "Character: " + characterName(m_inventoryCharacter) +
            "   Item Index: [" +
            std::to_string(m_characterInventories[m_inventoryCharacter].selectedIndex) +
            "]   MM3-Auswahl: " +
            (selectedInventoryItemActive() ? "ja" : "nein");
    }

    std::optional<std::string> MM3GameModule::inventoryClickDebugLine() const
    {
        if (!m_inventoryVisible) return std::nullopt;
        if (!m_inventorySelectionClick)
            return "Item-Klick: wartet auf Charakterdaten";
        return std::string("Item-Klick ") +
            (m_inventorySelectionRestored ? "(Oeffnen/Charakterwechsel)" : "(Navigation)") +
            ": (" + std::to_string(m_inventorySelectionClick->x) + ", " +
            std::to_string(m_inventorySelectionClick->y) + ") - " +
            (m_inventorySelectionClickPending ? "wartet" :
             selectedInventoryItemActive() ? "von MM3 bestaetigt" :
             "gesendet, wartet auf MM3");
    }

    std::vector<std::string> MM3GameModule::inventoryMenuEntries() const
    {
        if (m_inventoryMenu == InventoryMenu::Actions)
            return {"Anlegen", "Ablegen", "Geben an", "Benutzen", "Wegwerfen"};
        std::vector<std::string> names;
        if (m_inventoryMenu == InventoryMenu::Party)
            for (int slot : m_partyMenuSlots) names.push_back(characterName(slot));
        return names;
    }

    std::string MM3GameModule::inventoryMenuTitle() const
    {
        return m_inventoryMenu == InventoryMenu::Actions ? "Item" :
            m_inventoryMenu == InventoryMenu::Party ? "Geben an" : "";
    }

    std::optional<GameButtonPoint> MM3GameModule::inventoryMenuPosition() const
    {
        if (m_inventoryMenu == InventoryMenu::None || m_inventoryCharacter < 0)
            return std::nullopt;
        const int index = m_characterInventories[m_inventoryCharacter].selectedIndex;
        return GameButtonPoint{85, 45 + 18 * m_inventoryVisibleRow};
    }

    void MM3GameModule::update()
    {
        int detected = 0;
        m_detectedViews.clear();
        const auto detect = [&](const ScreenSignature& signature, int id, const char* name) {
            if (!matchesScreen(signature)) return;
            m_detectedViews.emplace_back(name);
            if (detected == 0) detected = id;
        };
        detect(ScreenSignatures::mainMenu(), 1, "Loading Screen");
        detect(ScreenSignatures::loadGame(), 2, "Load Game");
        detect(ScreenSignatures::characterScreen(), 4, "Character Screen");
        const bool inventoryVisible = matchesScreen(ScreenSignatures::inventory());
        m_inventoryVisible = inventoryVisible;
        detect(ScreenSignatures::inventory(), 5, "Inventory");
        detect(ScreenSignatures::mainGame(), 3, "Main Game");
        if (detected != m_activeButtons)
        {
            m_activeButtons = detected;
            if (!m_portraitMode)
                m_selectedButton = m_buttonMode && !buttons().empty() ? 0 : -1;
            m_pendingButtonClick = false;
        }
        if (!inventoryVisible)
        {
            m_inventoryActionSelection.reset();
            m_inventoryCharacter = -1;
            m_inventoryPage = 0;
            m_inventoryVisibleRow = 0;
            m_pendingInventoryClicks.clear();
            m_inventorySelectionClick.reset();
            m_inventorySelectionClickPending = false;
            m_inventoryMenu = InventoryMenu::None;
            m_inventorySelectionPendingUntil = {};
        }
        m_launcher.update();
        // Map detection must also work when the launcher is still waiting for
        // DOSBox (or MM3 was started before its launch sequence completed).
        m_mapDetector.update();
        if (m_mapDetector.mapId()) m_stateReader.update(m_state);
        if (inventoryVisible) rememberInventorySelection();
    }

    std::optional<std::string> MM3GameModule::currentMapKey() const
    {
        if (const auto id = m_mapDetector.mapId(); id && *id != 0)
            return "mm3/" + std::to_string(*id);
        return std::nullopt;
    }

    std::optional<std::string> MM3GameModule::currentMapName() const
    {
        const auto id = m_mapDetector.mapId();
        if (!id || *id > 66) return std::nullopt;

        const auto& memory = m_memoryReader.memory();
        constexpr std::size_t table = 0x25CAE;
        constexpr std::size_t stringBase = 0x20530;
        const std::size_t pointerAddress = table + 2 * *id;
        if (pointerAddress + 1 >= memory.size()) return std::nullopt;

        const std::size_t offset = static_cast<std::size_t>(memory[pointerAddress]) |
            (static_cast<std::size_t>(memory[pointerAddress + 1]) << 8);
        const std::size_t nameAddress = stringBase + offset;
        if (nameAddress >= memory.size()) return std::nullopt;

        std::string name;
        for (std::size_t i = nameAddress;
             i < memory.size() && i < nameAddress + 64; ++i)
        {
            const auto character = memory[i];
            if (character == 0) return name.empty() ? std::nullopt
                                                     : std::optional<std::string>(name);
            if (character == '*') name += "\xC3\xB6"; // MM3's oe glyph
            else if (character == '@') name += "\xC3\x9F"; // MM3's sharp s glyph
            else if (character >= 32 && character < 127)
                name += static_cast<char>(character);
            else return std::nullopt;
        }
        return std::nullopt;
    }

    std::optional<GamePosition> MM3GameModule::currentPosition() const
    {
        if (!m_mapDetector.mapId()) return std::nullopt;
        const auto& position = m_state.position();
        if (!position.valid) return std::nullopt;

        GamePosition result;
        result.x = position.x;
        // MM3 counts northward from zero; GridBuilder draws north at negative Y.
        result.y = -position.y;
        switch (position.direction)
        {
        case Direction::North: result.direction = GameFacingDirection::North; break;
        case Direction::East: result.direction = GameFacingDirection::East; break;
        case Direction::South: result.direction = GameFacingDirection::South; break;
        case Direction::West: result.direction = GameFacingDirection::West; break;
        default: return std::nullopt;
        }
        return result;
    }
}
