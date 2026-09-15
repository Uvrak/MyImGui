#pragma once


namespace DosBoxMemoryTools
{
    class MemoryScannerWindow;

    class MemoryScannerToolbar
    {
    public:
        static void draw(MemoryScannerWindow& window, bool& liveView);
    };
}
