#pragma once

namespace DosBoxMemoryTools
{
    class MemoryReadTrackerWindow;

    class MemoryReadPersistence
    {
    public:
        static void save(
            const MemoryReadTrackerWindow& window
        );

        static void load(
            MemoryReadTrackerWindow& window
        );
    };
}
