#pragma once

#include <vector>
#include <cstddef>
#include <string>

namespace MyImGui
{
    class DosBoxMemoryScanner;

    class DosBoxMemoryReadTrackerWindow
    {
    public:
        DosBoxMemoryReadTrackerWindow(
            DosBoxMemoryScanner& scanner,
            const std::string& gameId
        );

        void draw(
            bool* isOpen
        );

        void saveSession() const;
        void loadSession();

        void setGameId(
            const std::string& gameId
        );

    private:
        DosBoxMemoryScanner&
            m_scanner;

        std::string
            m_gameId;

        std::vector<size_t>
            m_idleReadAddresses;

        std::vector<size_t>
            m_attackReadAddresses;

        std::vector<size_t>
            m_attackOnlyReadAddresses;
	};
}
