#pragma once

#include <vector>
#include <cstddef>

namespace MyImGui
{
    class DosBoxMemoryScanner;

    class DosBoxMemoryReadTrackerWindow
    {
    public:
        DosBoxMemoryReadTrackerWindow(
            DosBoxMemoryScanner& scanner
        );

        void draw(
            bool* isOpen
        );

    private:
        DosBoxMemoryScanner&
            m_scanner;

        std::vector<size_t>
            m_idleReadAddresses;

        std::vector<size_t>
            m_attackReadAddresses;

        std::vector<size_t>
            m_attackOnlyReadAddresses;
	};
}
