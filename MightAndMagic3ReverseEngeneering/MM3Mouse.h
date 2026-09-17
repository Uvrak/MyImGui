#pragma once

#include <cstdint>

namespace MightAndMagic3
{
    // Independent of UI and emulator types, so every DOSBox client uses the
    // same MM3 policy. The emulator supplies time and applies returned actions.
    class MouseLatch
    {
    public:
        enum class Action { None, Press, Release };

        void reset() { *this = MouseLatch{}; }

        Action press(bool guestDown)
        {
            reconcile(guestDown);
            if (active)
            {
                nextPress = true;
                nextRelease = false;
                return Action::None;
            }
            start(false);
            return Action::Press;
        }

        Action release(std::uint64_t now, bool guestDown)
        {
            reconcile(guestDown);
            if (nextPress)
            {
                nextRelease = true;
                return Action::None;
            }
            if (!active)
                return Action::None;
            releaseRequested = true;
            return finish(now);
        }

        // Called after returning the current button state to the game.
        Action observe(std::uint64_t now, bool guestDown)
        {
            reconcile(guestDown);
            if (active)
            {
                if (polls == 0)
                    firstObserved = now;
                if (polls < 3)
                    ++polls;
                return finish(now);
            }
            if (nextPress)
            {
                const bool released = nextRelease;
                start(released);
                return Action::Press;
            }
            return Action::None;
        }

        Action cancel()
        {
            const bool wasActive = active;
            reset();
            return wasActive ? Action::Release : Action::None;
        }

    private:
        void reconcile(bool guestDown)
        {
            // MM3 resets its mouse driver during startup. Never keep a
            // phantom held click after the driver's button state was cleared.
            if (active && !guestDown)
                reset();
        }

        void start(bool released)
        {
            reset();
            active = true;
            releaseRequested = released;
        }

        Action finish(std::uint64_t now)
        {
            if (!releaseRequested || polls < 3 || now - firstObserved < 60)
                return Action::None;
            active = false;
            releaseRequested = false;
            return Action::Release;
        }

        bool active = false;
        bool releaseRequested = false;
        bool nextPress = false;
        bool nextRelease = false;
        unsigned polls = 0;
        std::uint64_t firstObserved = 0;
    };
}
