#include "TraceComparisonFilter.h"
#include "TraceAlignment.h"

#include <algorithm>

namespace DosBoxMemoryTools
{    
    std::vector<TraceComparisonDisplayEntry>
        TraceComparisonFilter::build(
            const std::vector<RuntimeInstruction>& traceA,
            const std::vector<RuntimeInstruction>& traceB,
            ControlFlowFilter controlFlowFilter
        )
    {
        std::vector<TraceComparisonDisplayEntry>
            entries;

        const std::vector<TraceAlignment> alignments =
            TraceAligner::align(
                traceA,
                traceB
            );

        entries.reserve(
            alignments.size()
        );

        for (const TraceAlignment& alignment :
            alignments)
        {
            if (alignment.synchronized)
            {
                TraceComparisonDisplayEntry entry{};

                entry.indexA =
                    alignment.indexA;

                entry.indexB =
                    alignment.indexB;

                entry.hasA =
                    true;

                entry.hasB =
                    true;

                entry.synchronized =
                    true;

                entries.push_back(
                    entry
                );

                continue;
            }

            for (size_t indexA = alignment.indexA;
                indexA < alignment.endIndexA;
                ++indexA)
            {
                TraceComparisonDisplayEntry entry{};

                entry.indexA =
                    indexA;

                entry.indexB =
                    alignment.indexB;

                entry.hasA =
                    true;

                entry.hasB =
                    false;

                entry.synchronized =
                    false;

                entries.push_back(
                    entry
                );
            }

            for (size_t indexB = alignment.indexB;
                indexB < alignment.endIndexB;
                ++indexB)
            {
                TraceComparisonDisplayEntry entry{};

                entry.indexA =
                    alignment.indexA;

                entry.indexB =
                    indexB;

                entry.hasA =
                    false;

                entry.hasB =
                    true;

                entry.synchronized =
                    false;

                entries.push_back(
                    entry
                );
            }

            if (!alignment.synchronized &&
                controlFlowFilter &&
                !controlFlowFilter(
                    alignment
                ))
            {
                continue;
            }
        }

        size_t nextIndexA = 0;
        size_t nextIndexB = 0;

        if (!alignments.empty())
        {
            const TraceAlignment& lastAlignment =
                alignments.back();

            nextIndexA =
                lastAlignment.endIndexA;

            nextIndexB =
                lastAlignment.endIndexB;
        }

        while (nextIndexA < traceA.size())
        {
            TraceComparisonDisplayEntry entry{};

            entry.indexA =
                nextIndexA;

            entry.indexB =
                nextIndexB;

            entry.hasA =
                true;

            entry.hasB =
                false;

            entry.synchronized =
                false;

            entries.push_back(
                entry
            );

            ++nextIndexA;
        }

        while (nextIndexB < traceB.size())
        {
            TraceComparisonDisplayEntry entry{};

            entry.indexA =
                nextIndexA;

            entry.indexB =
                nextIndexB;

            entry.hasA =
                false;

            entry.hasB =
                true;

            entry.synchronized =
                false;

            entries.push_back(
                entry
            );

            ++nextIndexB;
        }



        return entries;
    }
}
