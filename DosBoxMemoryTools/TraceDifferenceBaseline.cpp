#include "pch.h"
#include "TraceDifferenceBaseline.h"

#include <algorithm>

namespace DosBoxMemoryTools
{
    void TraceDifferenceBaseline::add(
        const std::vector<RuntimeInstruction>& traceA,
        const std::vector<RuntimeInstruction>& traceB
    )
    {
        const size_t count =
            (std::min)(
                traceA.size(),
                traceB.size()
                );

        for (size_t index = 0;
            index < count;
            ++index)
        {
            const RuntimeInstruction& instructionA =
                traceA[index];

            const RuntimeInstruction& instructionB =
                traceB[index];

            const TraceInstructionDifference difference =
                compareTraceInstructions(
                    instructionA,
                    instructionB
                );

            if (!difference.any())
            {
                continue;
            }

            const size_t address =
                instructionA.address;

            auto existing =
                std::find_if(
                    m_entries.begin(),
                    m_entries.end(),
                    [address](
                        const TraceDifferenceBaselineEntry& entry
                        )
                    {
                        return entry.address ==
                            address;
                    }
                );

            if (existing ==
                m_entries.end())
            {
                TraceDifferenceBaselineEntry entry;

                entry.address =
                    address;

                entry.difference =
                    difference;

                m_entries.push_back(
                    entry
                );

                continue;
            }

            existing->difference.address |=
                difference.address;

            existing->difference.ax |=
                difference.ax;

            existing->difference.bx |=
                difference.bx;

            existing->difference.cx |=
                difference.cx;

            existing->difference.dx |=
                difference.dx;

            existing->difference.si |=
                difference.si;

            existing->difference.di |=
                difference.di;

            existing->difference.bp |=
                difference.bp;

            existing->difference.sp |=
                difference.sp;

            existing->difference.ds |=
                difference.ds;

            existing->difference.es |=
                difference.es;

            existing->difference.ss |=
                difference.ss;
        }
    }

    TraceInstructionDifference
        TraceDifferenceBaseline::removeKnown(
            size_t address,
            const TraceInstructionDifference& difference
        ) const
    {
        TraceInstructionDifference result =
            difference;

        const auto existing =
            std::find_if(
                m_entries.begin(),
                m_entries.end(),
                [address](
                    const TraceDifferenceBaselineEntry& entry
                    )
                {
                    return entry.address ==
                        address;
                }
            );

        if (existing ==
            m_entries.end())
        {
            return result;
        }

        result.address &=
            !existing->difference.address;

        result.ax &=
            !existing->difference.ax;

        result.bx &=
            !existing->difference.bx;

        result.cx &=
            !existing->difference.cx;

        result.dx &=
            !existing->difference.dx;

        result.si &=
            !existing->difference.si;

        result.di &=
            !existing->difference.di;

        result.bp &=
            !existing->difference.bp;

        result.sp &=
            !existing->difference.sp;

        result.ds &=
            !existing->difference.ds;

        result.es &=
            !existing->difference.es;

        result.ss &=
            !existing->difference.ss;

        return result;
    }

    void TraceDifferenceBaseline::clear()
    {
        m_entries.clear();
    }

    size_t TraceDifferenceBaseline::size() const
    {
        return m_entries.size();
    }
}