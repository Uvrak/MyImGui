#include "../InventoryPortraitSwitch.h"
#include <cassert>
#include <iostream>

using Switch = MightAndMagic3::InventoryPortraitSwitch;
using Action = Switch::Action;
using namespace std::chrono_literals;

int main()
{
    const auto start = Switch::Clock::time_point{};
    const Switch::Selection selected{0, 4, 4}, cleared{0, 4, 0xFFFF};
    Switch state;
    assert(state.poll(start, 1, selected) == Action::None);
    state.begin(0);
    assert(state.poll(start, 1, selected) == Action::None); // Mouse has drained.
    assert(state.poll(start + 300ms, 1, selected) == Action::None); // Stale.
    assert(state.poll(start + 299ms, 2, selected) == Action::None);
    assert(state.poll(start + 300ms, 2, selected) == Action::DeselectItem);
    assert(state.poll(start + 600ms, 2, cleared) == Action::None); // Stale clear.
    assert(state.poll(start + 600ms, 3, selected) == Action::None);
    assert(state.poll(start + 2099ms, 4, cleared) == Action::None); // Still cooling down.
    assert(state.poll(start + 2100ms, 5, selected) == Action::None); // Slow MM3.
    assert(state.poll(start + 2200ms, 6, cleared) == Action::SelectCharacter);
    assert(state.poll(start + 2400ms, 7, cleared) == Action::None); // No portrait retry.
    assert(state.poll(start + 2500ms, 8, Switch::Selection{1, 0, 0xFFFF}) == Action::None);
    assert(!state.active());

    state.begin(0);
    assert(state.poll(start, 50, selected) == Action::None);
    assert(state.poll(start + 300ms, 51, selected) == Action::DeselectItem);
    assert(state.poll(start + 2100ms, 52, cleared) == Action::SelectCharacter);
    assert(state.poll(start + 2799ms, 53, cleared) == Action::None);
    assert(state.poll(start + 2800ms, 54, selected) == Action::None); // Never give.
    assert(state.poll(start + 2900ms, 55, cleared) == Action::SelectCharacter);
    assert(state.poll(start + 3600ms, 56, cleared) == Action::None); // Retry is bounded.
    assert(state.poll(start + 5100ms, 57, cleared) == Action::RestoreSelection);

    // If deselection cannot be confirmed, abort rather than give the item away.
    state.begin(0);
    assert(state.poll(start, 10, selected) == Action::None);
    assert(state.poll(start + 300ms, 11, selected) == Action::DeselectItem);
    assert(state.poll(start + 3299ms, 12, selected) == Action::None);
    assert(state.poll(start + 3300ms, 13, selected) == Action::RestoreSelection);
    assert(!state.active());

    // A queued mouse command may have selected an item after the original click.
    state.begin(0);
    assert(state.poll(start + 5s, 20, cleared) == Action::None);
    assert(state.poll(start + 5300ms, 20, cleared) == Action::None);
    assert(state.poll(start + 5300ms, 21, selected) == Action::DeselectItem);
    state.reset(); // Leaving the view or replacing the target cancels old work.
    assert(state.poll(start + 6s, 22, cleared) == Action::None);

    // Empty inventories and already cleared selections need just one portrait click.
    state.begin(0);
    assert(state.poll(start, 30, cleared) == Action::None);
    assert(state.poll(start + 300ms, 31, cleared) == Action::SelectCharacter);
    assert(state.poll(start + 999ms, 32, cleared) == Action::None);
    assert(state.poll(start + 1s, 33, cleared) == Action::SelectCharacter);
    assert(state.poll(start + 1700ms, 34, cleared) == Action::None);
    assert(state.poll(start + 3300ms, 35, cleared) == Action::RestoreSelection);

    // Missing / inconsistent memory never authorizes a portrait click.
    state.begin(0);
    assert(state.poll(start, 40, std::nullopt) == Action::None);
    assert(state.poll(start + 300ms, 41, std::nullopt) == Action::None);
    assert(state.poll(start + 600ms, 42, Switch::Selection{0, 4, 3}) == Action::None);
    assert(state.poll(start + 900ms, 43, Switch::Selection{0, 18, 18}) == Action::None);
    assert(state.poll(start + 3s, 44, std::nullopt) == Action::RestoreSelection);

    std::array<bool, MightAndMagic3::CharacterInventory::SlotCount> items{};
    const auto occupied = [&](int slot) { return items[slot]; };
    using MightAndMagic3::resolveInventoryIndex;
    assert(resolveInventoryIndex(0, occupied) == -1);
    items[1] = items[5] = items[12] = true;
    assert(resolveInventoryIndex(1, occupied) == 1);
    assert(resolveInventoryIndex(5, occupied) == 5);
    assert(resolveInventoryIndex(2, occupied) == 12); // Empty gap -> last real item.
    assert(resolveInventoryIndex(17, occupied) == 12);
    assert(resolveInventoryIndex(99, occupied) == 12);
    assert(resolveInventoryIndex(-1, occupied) == 12);
    items[12] = false;
    assert(resolveInventoryIndex(12, occupied) == 5);
    std::cout << "Inventory portrait switch tests passed\n";
}
