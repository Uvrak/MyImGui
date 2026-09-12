#pragma once

#include "ScannerAddress.h"
#include "ScannerRange.h"

namespace DosBoxMemoryTools
{
    class ScannerMenuBar
    {
    public:
        bool draw(
            ScannerAddress& address,
            ScannerRange& range
        );
    };
}