#include "pch.h"
#include "MemoryPatternSearch.h"

namespace DosBoxMemoryTools
{
    std::vector<size_t> MemoryPatternSearch::find(
        const uint8_t* data,
        size_t size,
        const std::vector<uint8_t>& pattern
    )
    {
        std::vector<size_t> results;

        if (data == nullptr ||
            pattern.empty() ||
            pattern.size() > size)
        {
            return results;
        }

        for (size_t offset = 0;
            offset <= size - pattern.size();
            ++offset)
        {
            bool matches = true;

            for (size_t i = 0;
                i < pattern.size();
                ++i)
            {
                if (data[offset + i] != pattern[i])
                {
                    matches = false;
                    break;
                }
            }

            if (matches)
            {
                results.push_back(offset);
            }
        }

        return results;
    }
}