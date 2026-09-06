#include "TraceComparisonFilter.h"
#include "TraceComparison.h"

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

        const size_t count =
            (std::min)(
                traceA.size(),
                traceB.size()
                );

        size_t collapsedCount = 0;

        for (size_t i = 0;
            i < count;
            ++i)
        {
            const bool different =
                compareTraceInstructions(
                    traceA[i],
                    traceB[i]
                ).any();

            if (!different)
            {
                ++collapsedCount;
                continue;
            }

            if (collapsedCount > 0)
            {
                entries.push_back(
                    {
                        i - collapsedCount,
                        collapsedCount
                    }
                );

                collapsedCount = 0;
            }

            entries.push_back(
                {
                    i,
                    0
                }
            );
        }

        if (collapsedCount > 0)
        {
            entries.push_back(
                {
                    count - collapsedCount,
                    collapsedCount
                }
            );
        }

        return entries;
    }
}