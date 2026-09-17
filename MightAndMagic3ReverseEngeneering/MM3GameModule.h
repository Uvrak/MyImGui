#pragma once

#include "../GridBuilderCore/GameModule.h"
#include "MapDetector.h"
#include "../DosBoxMemoryTools/MemoryReader.h"
#include "MM3Launcher.h"
#include "StateReader.h"
#include "MM3KeyBindings.h"
#include "ScreenSignatures.h"
#include "InventoryClickRetry.h"
#include "InventoryPortraitSwitch.h"
#include "MightAndMagic3Inventory.h"
#include <vector>
#include <array>
#include <chrono>
#include <deque>

namespace MightAndMagic3
{
    class MM3GameModule final : public GameModule
    {
    public:
        MM3GameModule(DosBoxX::Controller& controller,
                      DosBoxX::NamedPipeClient& pipe,
                      std::optional<std::size_t> mapIdAddress,
                      const std::string& buttonConfigPath);
        void start() override;
        void update() override;
        void keyDown(int key) override;
        bool onDosBoxMouseClick(GameButtonPoint point) override;
        bool blockDirectDosBoxKeyboard() const override;
        bool blockDirectDosBoxVerticalKeys() const override { return m_inventoryVisible || m_spellsVisible; }
        bool blockDirectDosBoxInventoryKeys() const override { return m_inventoryVisible || m_spellsVisible; }
        std::optional<std::string> inventoryDebugLine() const override;
        std::optional<std::string> inventoryClickDebugLine() const override;
        std::vector<std::string> inventoryMenuEntries() const override;
        int inventoryMenuSelection() const override { return m_menuSelection; }
        std::string inventoryMenuTitle() const override;
        std::optional<GameButtonPoint> inventoryMenuPosition() const override;
        std::optional<GameButtonRect> selectedButtonRect() const override;
        std::vector<GameButtonRect> buttonRects() const override;
        std::size_t buttonCount() const override { return buttons().size(); }
        std::optional<GameButtonPoint> takeButtonClick() override;
        bool buttonEditingActive() const override { return m_buttonMode || m_portraitMode; }
        bool buttonViewAvailable() const override { return m_portraitMode || m_activeButtons > 0; }
        std::string buttonViewName() const override;
        std::vector<std::string> activeViewNames() const override;
        bool addButton(const GameButtonRect& rect) override;
        bool deleteButton(std::size_t index) override;
        bool modifyButton(const GameButtonEdit& edit) override;
        void setFrame(const uint8_t* pixels, uint32_t width,
                      uint32_t height, uint32_t pitch) override;
        std::optional<std::string> currentMapKey() const override;
        std::optional<std::string> currentMapName() const override;
        std::optional<GamePosition> currentPosition() const override;
        const CharacterInventories& characterInventories() const { return m_characterInventories; }

    private:
        MM3Launcher m_launcher;
        DosBoxMemoryTools::MemoryReader m_memoryReader;
        MapDetector m_mapDetector;
        StateReader m_stateReader;
        GameState m_state;
        KeyBindings m_keyBindings;
        bool m_buttonMode = false;
        bool m_portraitMode = false;
        bool m_showButtonOutlines = false;
        int m_selectedButton = -1;
        int m_activeButtons = 0;
        std::array<std::vector<GameButtonRect>, 9> m_buttons;
        std::vector<GameButtonRect> m_castSpellButtons;
        std::vector<GameButtonRect> m_portraitControls;
        std::vector<GameButtonRect> m_spellButtons;
        std::optional<GameButtonPoint> m_pendingSpellClick;
        std::chrono::steady_clock::time_point m_spellReadyAt{};
        bool m_restoreReadySpell = false;
        bool m_spellsVisible = false;
        std::vector<std::string> m_detectedViews;
        std::vector<std::string> m_otherButtonConfigLines;
        std::string m_buttonConfigPath;
        bool m_pendingButtonClick = false;
        InventoryPortraitSwitch m_portraitSwitch;
        GameButtonPoint m_portraitSwitchPoint;
        struct InventoryActionSelection
        {
            int character;
            int index;
            std::uint64_t snapshot = 0;
            bool dispatched = false;
        };
        std::optional<InventoryActionSelection> m_inventoryActionSelection;
        std::deque<GameButtonPoint> m_pendingInventoryClicks;
        std::chrono::steady_clock::time_point m_nextInventoryClickTime{};
        std::optional<GameButtonPoint> m_inventorySelectionClick;
        bool m_inventorySelectionClickPending = false;
        bool m_inventorySelectionRestored = false;
        InventoryClickRetry m_inventoryClickRetry;
        CharacterInventories m_characterInventories;
        int m_inventoryCharacter = -1;
        std::uint64_t m_inventorySelectionTraceState = ~std::uint64_t{0};
        int m_inventoryPage = 0;
        int m_inventoryVisibleRow = 0;
        bool m_inventoryVisible = false;
        enum class InventoryMenu { None, Actions, Party };
        InventoryMenu m_inventoryMenu = InventoryMenu::None;
        int m_menuSelection = 0;
        std::vector<int> m_partyMenuSlots;
        std::chrono::steady_clock::time_point m_inventorySelectionPendingUntil{};
        const uint8_t* m_framePixels = nullptr;
        uint32_t m_frameWidth = 0;
        uint32_t m_frameHeight = 0;
        uint32_t m_framePitch = 0;
        bool matchesScreen(const ScreenSignature& signature) const;
        const std::vector<GameButtonRect>& buttons() const;
        std::vector<GameButtonRect>& buttons();
        void loadButtons(const std::string& path);
        bool saveButtons() const;
        void rememberInventorySelection();
        bool beginPortraitSwitch(GameButtonPoint point);
        std::optional<GameButtonPoint> advancePortraitSwitch();
        std::string characterName(int slot) const;
        void queueInventorySelection(int index, bool restore);
        void queueItemActionSelection(int index);
        void trackInventoryActionClick(GameButtonPoint point);
        bool selectedInventoryItemActive() const;
        void initializeSpellSelection();
        void materializeSpellButtons();
        bool castSpellListActive() const
        {
            return m_activeButtons == 7 && m_spellsVisible && !m_portraitMode;
        }
    };
}
