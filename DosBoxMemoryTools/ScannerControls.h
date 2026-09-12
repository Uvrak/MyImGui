#pragma once

#include "ScannerAddress.h"
#include "ScannerRange.h"

namespace DosBoxMemoryTools
{
    class ScannerControls
    {
    public:
        void draw(
            ScannerAddress& address,
            ScannerRange& range
        );
    };
}