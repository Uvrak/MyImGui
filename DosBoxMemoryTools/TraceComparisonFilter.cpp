#include "TraceComparisonFilter.h"
#include "TraceAlignment.h"

#include <algorithm>

namespace DosBoxMemoryTools
{    
    std::vector<TraceComparisonDisplayEntry>
        TraceComparisonFilter::build(
            const std::vector<RuntimeInstruction>& traceA,
            const std::vector<RuntimeInstruction>& traceB
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
            TraceComparisonDisplayEntry entry{};

            entry.indexA =
                alignment.indexA;

            entry.indexB =
                alignment.indexB;

            entry.synchronized =
                alignment.synchronized;

            if (!alignment.synchronized)
            {
                const size_t countA =
                    alignment.endIndexA -
                    alignment.indexA;

                const size_t countB =
                    alignment.endIndexB -
                    alignment.indexB;

                entry.collapsedCount =
                    (std::max)(
                        countA,
                        countB
                        );
            }

            entries.push_back(
                entry
            );
        }

        return entries;
    }
}