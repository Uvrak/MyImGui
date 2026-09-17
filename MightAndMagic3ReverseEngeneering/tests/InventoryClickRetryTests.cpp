#include "../InventoryClickRetry.h"
#include <cassert>
#include <iostream>

int main()
{
    using Retry = MightAndMagic3::InventoryClickRetry;
    using namespace std::chrono_literals;
    const auto start = Retry::Clock::time_point{};
    Retry retry;
    assert(!retry.poll(start + 200ms, 1, false));
    retry.arm(start, 10);
    assert(!retry.poll(start + 1199ms, 11, false));
    assert(!retry.poll(start + 1200ms, 10, false)); // No newer snapshot.
    assert(retry.poll(start + 1200ms, 11, false));
    assert(!retry.poll(start + 2399ms, 12, false));
    assert(retry.poll(start + 2400ms, 12, false));
    assert(!retry.poll(start + 2450ms, 13, true)); // Stop on confirmation.
    assert(!retry.active());
    assert(!retry.poll(start + 700ms, 14, false));
    retry.arm(start + 800ms, 14);
    retry.reset(); // Leaving inventory, another item, or an item action.
    assert(!retry.poll(start + 1000ms, 15, false));
    retry.arm(start, 20);
    assert(!retry.poll(start + 50ms, 21, true));
    assert(!retry.poll(start + 200ms, 22, false));
    std::cout << "Inventory retry tests passed\n";
}
