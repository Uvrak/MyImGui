#include "MapDetector.h"
#include "MemoryReader.h"

namespace MightAndMagic3
{
    MapDetector::MapDetector(DosBoxMemoryTools::MemoryReader& reader,
                             std::optional<std::size_t> address)
        : m_reader(reader), m_address(address) {}

    bool MapDetector::update()
    {
        if (!m_address) return false;

        const auto now = std::chrono::steady_clock::now();
        if (now < m_nextPoll) return false;
        m_nextPoll = now + std::chrono::milliseconds(200);

        std::string response;
        if (!m_snapshotPipe.request("PUBLISH", response) ||
            response != "PUBLISHED" || !m_reader.readSnapshot() ||
            *m_address >= m_reader.memory().size())
        {
            m_mapId.reset();
            return false;
        }

        const auto id = m_reader.memory()[*m_address];
        const bool changed = !m_mapId || *m_mapId != id;
        m_mapId = id;
        return changed;
    }

    std::optional<std::uint8_t> MapDetector::mapId() const
    {
        return m_mapId;
    }
}
