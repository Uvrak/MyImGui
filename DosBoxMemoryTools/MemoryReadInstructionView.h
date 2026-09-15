#pragma once

namespace DosBoxMemoryTools
{
    class MemoryReadTrackerWindow;

    class MemoryReadInstructionView
    {
    public:
        static void draw(
            MemoryReadTrackerWindow& window
        );
    };
}
