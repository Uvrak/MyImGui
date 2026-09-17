#pragma once

#include <array>
#include <cstddef>

namespace MightAndMagic3
{
    struct CharacterInventory
    {
        static constexpr int SlotCount = 18;
        std::array<int, SlotCount> slots{};
        int selectedIndex = 0;

        CharacterInventory()
        {
            for (int index = 0; index < SlotCount; ++index)
                slots[index] = index;
        }
    };

    using CharacterInventories = std::array<CharacterInventory, 8>;

    template <class IsOccupied>
    int resolveInventoryIndex(int preferred, const IsOccupied& occupied)
    {
        if (preferred >= 0 && preferred < CharacterInventory::SlotCount && occupied(preferred))
            return preferred;
        for (int slot = CharacterInventory::SlotCount - 1; slot >= 0; --slot)
            if (occupied(slot)) return slot;
        return -1;
    }
}
