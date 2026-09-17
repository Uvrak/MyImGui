#include "../MM3Mouse.h"
#include <cassert>

using Latch = MightAndMagic3::MouseLatch;
using Action = Latch::Action;

int main()
{
    // The portrait is still pressed when the inventory restoration arrives.
    Latch latch;
    assert(latch.pressAt(false, 0.2f, 0.8f) == Action::Press);
    assert(latch.release(10, true) == Action::None);
    assert(latch.pressAt(true, 0.1f, 0.3f) == Action::None);
    assert(latch.release(30, true) == Action::None);
    assert(latch.position().x == 0.2f && latch.position().y == 0.8f);
    assert(latch.observe(40, true) == Action::None);
    assert(latch.observe(70, true) == Action::None);
    assert(latch.observe(100, true) == Action::Release);
    assert(latch.position().x == 0.2f);
    // Only after an UP poll may the queued item position and DOWN take effect.
    assert(latch.observe(110, false) == Action::Press);
    assert(latch.position().x == 0.1f && latch.position().y == 0.3f);
    assert(latch.observe(120, true) == Action::None);
    assert(latch.observe(150, true) == Action::None);
    assert(latch.observe(180, true) == Action::Release);
    assert(latch.observe(190, false) == Action::None);

    // A click arriving between release and the next guest poll must wait too.
    latch.reset();
    assert(latch.press(false) == Action::Press);
    assert(!latch.position().valid);
    assert(latch.observe(0, true) == Action::None);
    assert(latch.observe(30, true) == Action::None);
    assert(latch.observe(60, true) == Action::None);
    assert(latch.release(70, true) == Action::Release);
    assert(latch.pressAt(false, 0.4f, 0.5f) == Action::None);
    assert(latch.release(80, false) == Action::None);
    assert(latch.observe(90, false) == Action::Press);
    assert(latch.position().x == 0.4f && latch.position().y == 0.5f);

    // Cancellation and driver reset discard pending presses and positions.
    assert(latch.pressAt(true, 0.6f, 0.7f) == Action::None);
    assert(latch.cancel() == Action::Release);
    assert(latch.observe(100, false) == Action::None);
    assert(!latch.position().valid);
    assert(latch.pressAt(false, 0.8f, 0.9f) == Action::Press);
    assert(latch.observe(110, false) == Action::None);
    assert(!latch.position().valid);
}
