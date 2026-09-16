#include "pch.h"
#include "TraceAlignment.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <unordered_map>

namespace DosBoxMemoryTools
{
    namespace
    {
        constexpr size_t anchorLength = 8;

        uint64_t sequenceHash(const std::vector<RuntimeInstruction>& trace, size_t index)
        {
            uint64_t hash = 14695981039346656037ull;
            for (size_t offset = 0; offset < anchorLength; ++offset)
            {
                const auto& instruction = trace[index + offset];
                hash ^= static_cast<uint64_t>(instruction.address);
                hash *= 1099511628211ull;
                hash ^= (static_cast<uint64_t>(instruction.cs) << 16) | instruction.ip;
                hash *= 1099511628211ull;
            }
            return hash;
        }

        bool sameInstruction(const RuntimeInstruction& a, const RuntimeInstruction& b)
        {
            return a.address == b.address && a.cs == b.cs && a.ip == b.ip;
        }

        bool sameSequence(const std::vector<RuntimeInstruction>& a, size_t ia,
            const std::vector<RuntimeInstruction>& b, size_t ib)
        {
            for (size_t offset = 0; offset < anchorLength; ++offset)
                if (!sameInstruction(a[ia + offset], b[ib + offset]))
                    return false;
            return true;
        }
    }

    std::vector<TraceAlignment> TraceAligner::align(
        const std::vector<RuntimeInstruction>& traceA,
        const std::vector<RuntimeInstruction>& traceB)
    {
        std::vector<TraceAlignment> result;
        std::unordered_map<uint64_t, std::vector<size_t>> positionsB;
        if (traceB.size() >= anchorLength)
        {
            positionsB.reserve(traceB.size());
            for (size_t index = 0; index + anchorLength <= traceB.size(); ++index)
                positionsB[sequenceHash(traceB, index)].push_back(index);
        }

        size_t indexA = 0;
        size_t indexB = 0;
        bool confirmed = false;
        while (indexA < traceA.size() && indexB < traceB.size())
        {
            bool matching = sameInstruction(traceA[indexA], traceB[indexB]);
            if (matching && !confirmed)
            {
                const size_t available = (std::min)(traceA.size() - indexA,
                    traceB.size() - indexB);
                const size_t required = (std::min)(anchorLength, available);
                for (size_t offset = 1; offset < required; ++offset)
                    if (!sameInstruction(traceA[indexA + offset],
                        traceB[indexB + offset]))
                    {
                        matching = false;
                        break;
                    }
            }
            if (matching)
            {
                result.push_back({ indexA, indexB, indexA + 1, indexB + 1, true });
                confirmed = true;
                ++indexA;
                ++indexB;
                continue;
            }

            size_t bestA = traceA.size();
            size_t bestB = traceB.size();
            const size_t remaining = (std::max)(traceA.size() - indexA,
                traceB.size() - indexB);

            // Expand the search only when no nearby stable sequence exists.
            for (size_t distance = 64; ; distance = (std::min)(remaining, distance * 4))
            {
                const size_t endA = indexA + (std::min)(distance, traceA.size() - indexA);
                const size_t endB = indexB + (std::min)(distance, traceB.size() - indexB);
                size_t bestCost = (std::numeric_limits<size_t>::max)();

                for (size_t a = indexA; a + anchorLength <= traceA.size() && a < endA; ++a)
                {
                    if (a - indexA >= bestCost)
                        break;
                    const auto found = positionsB.find(sequenceHash(traceA, a));
                    if (found == positionsB.end())
                        continue;

                    const auto& positions = found->second;
                    auto b = (std::lower_bound)(positions.begin(), positions.end(), indexB);
                    for (; b != positions.end() && *b < endB; ++b)
                    {
                        const size_t cost = (a - indexA) + (*b - indexB);
                        if (cost >= bestCost)
                            break;
                        if (!sameSequence(traceA, a, traceB, *b))
                            continue;
                        bestCost = cost;
                        bestA = a;
                        bestB = *b;
                    }
                }

                if (bestA != traceA.size() || distance >= remaining)
                    break;
            }

            if (bestA == traceA.size())
            {
                result.push_back({ indexA, indexB, traceA.size(), traceB.size(), false });
                break;
            }

            result.push_back({ indexA, indexB, bestA, bestB, false });
            indexA = bestA;
            indexB = bestB;
            confirmed = true;
        }

        return result;
    }
}
