#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace DosBoxMemoryTools
{
    class MemoryPatternSearch
    {
    public:
        static std::vector<size_t> find(
            const uint8_t* data,
            size_t size,
            const std::vector<uint8_t>& pattern
        );
    };
}