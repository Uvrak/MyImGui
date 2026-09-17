#pragma once

#include "MightAndMagic3Inventory.h"
#include <chrono>
#include <cstdint>
#include <optional>

namespace MightAndMagic3
{
    // MM3 interprets a portrait click with an active item as a gift.
    // Never emit that click until a fresh snapshot confirms deselection.
    class InventoryPortraitSwitch
    {
    public:
        using Clock = std::chrono::steady_clock;
        enum class Action { None, DeselectItem, SelectCharacter, RestoreSelection };
        struct Selection { int character; int index; int activeIndex; };

        bool active() const { return m_stage != Stage::Idle; }
        void reset() { m_stage = Stage::Idle; }
        void begin(int source)
        {
            m_source = source;
            m_stage = Stage::Settle;
        }

        Action poll(Clock::time_point now, std::uint64_t snapshot,
                    std::optional<Selection> selection)
        {
            if (!active()) return Action::None;
            if (selection && selection->character != m_source)
            {
                reset();
                return Action::None;
            }
            if (m_stage == Stage::Settle)
            {
                // The host calls poll only after older mouse input has drained.
                // Start the snapshot barrier here, not at the original mouse down.
                m_snapshot = snapshot;
                m_readyAt = now + std::chrono::milliseconds(300);
                m_deadline = now + std::chrono::seconds(3);
                m_stage = Stage::Inspect;
                return Action::None;
            }
            if (now >= m_deadline)
            {
                reset();
                return Action::RestoreSelection;
            }
            if (!selection || snapshot == m_snapshot || now < m_readyAt)
                return Action::None;

            if (m_stage == Stage::AwaitCharacter)
            {
                // A portrait press can be ignored by MM3. Retry only after a
                // newer snapshot still shows the source and no active item.
                if (selection->activeIndex != 0xFFFF || m_portraitClicks >= 2)
                    return Action::None;
                ++m_portraitClicks;
                m_snapshot = snapshot;
                m_readyAt = now + std::chrono::milliseconds(700);
                return Action::SelectCharacter;
            }

            if (selection->activeIndex == 0xFFFF)
            {
                m_stage = Stage::AwaitCharacter;
                m_portraitClicks = 1;
                m_snapshot = snapshot;
                m_readyAt = now + std::chrono::milliseconds(700);
                m_deadline = now + std::chrono::seconds(3);
                return Action::SelectCharacter;
            }
            if (m_stage == Stage::Inspect && selection->activeIndex >= 0 &&
                selection->activeIndex < CharacterInventory::SlotCount &&
                selection->index == selection->activeIndex)
            {
                m_stage = Stage::AwaitDeselection;
                m_snapshot = snapshot;
                // MM3 can ignore a portrait click immediately after an item
                // click, even when the selection mirror already says cleared.
                m_readyAt = now + std::chrono::milliseconds(1800);
                m_deadline = now + std::chrono::seconds(3);
                return Action::DeselectItem;
            }
            return Action::None;
        }

    private:
        enum class Stage { Idle, Settle, Inspect, AwaitDeselection, AwaitCharacter };
        Stage m_stage = Stage::Idle;
        int m_source = -1;
        int m_portraitClicks = 0;
        std::uint64_t m_snapshot = 0;
        Clock::time_point m_readyAt{}, m_deadline{};
    };
}
