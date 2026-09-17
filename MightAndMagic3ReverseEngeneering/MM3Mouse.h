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
        struct Position
        {
            float x = 0, y = 0;
            bool valid = false;
        };

        void reset() { *this = MouseLatch{}; }

        Action press(bool guestDown)
        {
            return press(guestDown, Position{});
        }

        Action pressAt(bool guestDown, float x, float y)
        {
            Position position;
            position.x = x;
            position.y = y;
            position.valid = true;
            return press(guestDown, position);
        }

        Position position() const { return activePosition; }

    private:
        Action press(bool guestDown, Position position)
        {
            reconcile(guestDown);
            if (active || awaitingReleasePoll)
            {
                nextPress = true;
                nextRelease = false;
                nextPosition = position;
                return Action::None;
            }
            start(false, position);
            return Action::Press;
        }

    public:
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
            // The current poll has returned an unpressed button to MM3.
            awaitingReleasePoll = false;
            if (nextPress)
            {
                const bool released = nextRelease;
                start(released, nextPosition);
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

        void start(bool released, Position position)
        {
            reset();
            active = true;
            releaseRequested = released;
            activePosition = position;
        }

        Action finish(std::uint64_t now)
        {
            if (!releaseRequested || polls < 3 || now - firstObserved < 60)
                return Action::None;
            active = false;
            releaseRequested = false;
            awaitingReleasePoll = true;
            return Action::Release;
        }

        bool active = false;
        bool releaseRequested = false;
        bool nextPress = false;
        bool nextRelease = false;
        bool awaitingReleasePoll = false;
        Position activePosition;
        Position nextPosition;
        unsigned polls = 0;
        std::uint64_t firstObserved = 0;
    };
}
