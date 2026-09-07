#include "pch.h"
#include "TraceAlignment.h"

#include <algorithm>


namespace DosBoxMemoryTools
{

    namespace
    {
        bool hasMatchingSequence(
            const std::vector<RuntimeInstruction>& traceA,
            size_t indexA,
            const std::vector<RuntimeInstruction>& traceB,
            size_t indexB
        )
        {
            constexpr size_t requiredMatches =
                3;

            if (indexA + requiredMatches >
                traceA.size() ||
                indexB + requiredMatches >
                traceB.size())
            {
                return false;
            }

            for (size_t offset = 0;
                offset < requiredMatches;
                ++offset)
            {
                if (traceA[indexA + offset].address !=
                    traceB[indexB + offset].address)
                {
                    return false;
                }
            }

            return true;
        }
    }

    std::vector<TraceAlignment>
        TraceAligner::align(
            const std::vector<RuntimeInstruction>& traceA,
            const std::vector<RuntimeInstruction>& traceB
        )
    {
        std::vector<TraceAlignment> result;

        size_t indexA = 0;
        size_t indexB = 0;

        constexpr size_t searchDistance =
            64;

        while (indexA < traceA.size() &&
            indexB < traceB.size())
        {
            if (traceA[indexA].address ==
                traceB[indexB].address)
            {
                TraceAlignment alignment;

                alignment.indexA =
                    indexA;

                alignment.indexB =
                    indexB;

                alignment.endIndexA =
                    indexA + 1;

                alignment.endIndexB =
                    indexB + 1;

                alignment.synchronized =
                    true;

                result.push_back(
                    alignment
                );

                ++indexA;
                ++indexB;

                continue; struct TraceAlignment
                {
                    size_t indexA = 0;
                    size_t indexB = 0;

                    size_t endIndexA = 0;
                    size_t endIndexB = 0;

                    bool synchronized = true;
                };
            }

            size_t bestA =
                static_cast<size_t>(-1);

            size_t bestB =
                static_cast<size_t>(-1);

            size_t bestDistance =
                static_cast<size_t>(-1);

            const size_t endA =
                (std::min)(
                    traceA.size(),
                    indexA + searchDistance
                    );

            const size_t endB =
                (std::min)(
                    traceB.size(),
                    indexB + searchDistance
                    );

            for (size_t searchA = indexA;
                searchA < endA;
                ++searchA)
            {
                for (size_t searchB = indexB;
                    searchB < endB;
                    ++searchB)
                {
                    if (!hasMatchingSequence(
                        traceA,
                        searchA,
                        traceB,
                        searchB
                    ))
                    {
                        continue;
                    }

                    const size_t distance =
                        (searchA - indexA) +
                        (searchB - indexB);

                    if (distance <
                        bestDistance)
                    {
                        bestDistance =
                            distance;

                        bestA =
                            searchA;

                        bestB =
                            searchB;
                    }
                }
            }

            TraceAlignment divergence;

            divergence.indexA =
                indexA;

            divergence.indexB =
                indexB;

            divergence.synchronized =
                false;

            if (bestA !=
                static_cast<size_t>(-1) &&
                bestB !=
                static_cast<size_t>(-1))
            {
                divergence.endIndexA =
                    bestA;

                divergence.endIndexB =
                    bestB;
            }
            else
            {
                divergence.endIndexA =
                    (std::min)(
                        traceA.size(),
                        indexA + 1
                        );

                divergence.endIndexB =
                    (std::min)(
                        traceB.size(),
                        indexB + 1
                        );
            }

            result.push_back(
                divergence
            );

            if (bestA ==
                static_cast<size_t>(-1) ||
                bestB ==
                static_cast<size_t>(-1))
            {
                ++indexA;
                ++indexB;

                continue;
            }

            indexA =
                bestA;

            indexB =
                bestB;
        }

        return result;
    }
}