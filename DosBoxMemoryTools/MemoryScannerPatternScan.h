#pragma once

#include <cstddef>

namespace DosBoxMemoryTools
{
    class MemoryScanner;

    class MemoryScannerPatternScan
    {
    public:
        explicit MemoryScannerPatternScan(
            MemoryScanner& scanner
        );

        void draw();

    private:
        MemoryScanner& m_scanner;

        char m_patternText[256]{};
    };
}