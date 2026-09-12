#pragma once

#include <cstddef>

namespace DosBoxMemoryTools
{
    struct ScannerRange
    {
        bool enabled = false;

        size_t start = 0;
        size_t end = 0;
    };
}