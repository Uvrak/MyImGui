#pragma once

#include "ScannerAddress.h"
#include "ScannerRange.h"

namespace DosBoxMemoryTools
{
    class ScannerSettings
    {
    public:
        void load(
            ScannerAddress& address,
            ScannerRange& range
        );

        void save(
            const ScannerAddress& address,
            const ScannerRange& range
        );
    };
}