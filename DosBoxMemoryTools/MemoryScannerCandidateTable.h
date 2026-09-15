#pragma once


namespace DosBoxMemoryTools
{
    class MemoryScannerWindow;

    class MemoryScannerCandidateTable
    {
    public:
        static void draw(MemoryScannerWindow& window, bool liveView);
    };
}
