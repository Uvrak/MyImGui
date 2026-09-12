#include "pch.h"
#include "ScannerSettings.h"

#include <filesystem>
#include <fstream>

namespace DosBoxMemoryTools
{
    void ScannerSettings::load(
        ScannerAddress& address,
        ScannerRange& range
    )
    {
        std::ifstream file(
            "../settings/scanner.cfg"
        );

        if (!file)
        {
            return;
        }

        file
            >> address.value
            >> range.enabled
            >> range.start
            >> range.end;
    }

    void ScannerSettings::save(
        const ScannerAddress& address,
        const ScannerRange& range
    )
    {
        std::filesystem::create_directories(
            "../settings"
        );

        std::ofstream file(
            "../settings/scanner.cfg"
        );

        if (!file)
        {
            return;
        }

        file
            << address.value << '\n'
            << range.enabled << '\n'
            << range.start << '\n'
            << range.end << '\n';
    }
}