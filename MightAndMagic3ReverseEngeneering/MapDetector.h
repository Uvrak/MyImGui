#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <chrono>
#include "../DosBoxX/NamedPipeClient.h"

namespace DosBoxMemoryTools { class MemoryReader; }

namespace MightAndMagic3
{
    class MapDetector
    {
    public:
        MapDetector(DosBoxMemoryTools::MemoryReader& reader,
                    std::optional<std::size_t> address);
        bool update();
        std::optional<std::uint8_t> mapId() const;

    private:
        DosBoxMemoryTools::MemoryReader& m_reader;
        DosBoxX::NamedPipeClient m_snapshotPipe{R"(\\.\pipe\DosBoxMemoryScanner)"};
        std::chrono::steady_clock::time_point m_nextPoll{};
        std::optional<std::size_t> m_address;
        std::optional<std::uint8_t> m_mapId;
    };
}
