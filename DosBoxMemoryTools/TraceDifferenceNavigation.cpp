#include "pch.h"
#include "TraceDifferenceNavigation.h"

#include <limits>

namespace DosBoxMemoryTools
{
    namespace
    {
        constexpr size_t invalidIndex =
            static_cast<size_t>(-1);

        bool isDifference(
            const std::vector<RuntimeInstruction>& traceA,
            const std::vector<RuntimeInstruction>& traceB,
            const TraceAlignment& alignment,
            const TraceDifferenceNavigation::DifferenceComparer& comparer,
            const TraceDifferenceNavigation::ControlFlowComparer&
        )
        {
            if (!alignment.synchronized)
            {
                return false;
            }

            if (alignment.indexA >= traceA.size() ||
                alignment.indexB >= traceB.size())
            {
                return false;
            }

            return comparer(
                traceA[alignment.indexA],
                traceB[alignment.indexB]
            ).any();
        }
    }

    size_t TraceDifferenceNavigation::findNextDifference(
        const std::vector<RuntimeInstruction>& traceA,
        const std::vector<RuntimeInstruction>& traceB,
        const std::vector<TraceAlignment>& alignments,
        size_t selectedIndexA,
        TraceDifferenceNavigation::DifferenceComparer comparer,
        ControlFlowComparer controlFlowComparer
    )
    {
        for (size_t alignmentIndex = 0;
            alignmentIndex < alignments.size();
            ++alignmentIndex)
        {
            const TraceAlignment& alignment =
                alignments[alignmentIndex];

            if (alignment.indexA <= selectedIndexA)
            {
                continue;
            }

            if (!isDifference(
                traceA,
                traceB,
                alignment,
                comparer,
                controlFlowComparer
            ))
            {
                continue;
            }

            if (alignmentIndex == 0)
            {
                return alignment.indexA;
            }

            const TraceAlignment& previous =
                alignments[alignmentIndex - 1];

            if (!isDifference(
                traceA,
                traceB,
                previous,
                comparer,
                controlFlowComparer
            ))
            {
                return alignment.indexA;
            }

            if (!alignment.synchronized ||
                !previous.synchronized)
            {
                continue;
            }

            const RuntimeInstruction& currentA =
                traceA[alignment.indexA];

            const RuntimeInstruction& currentB =
                traceB[alignment.indexB];

            const RuntimeInstruction& previousA =
                traceA[previous.indexA];

            const RuntimeInstruction& previousB =
                traceB[previous.indexB];

            const TraceInstructionDifference difference =
                comparer(
                    currentA,
                    currentB
                );

            bool sameDifferenceValues =
                true;

            if (difference.ax)
                sameDifferenceValues &=
                currentA.registers.ax == previousA.registers.ax &&
                currentB.registers.ax == previousB.registers.ax;

            if (difference.bx)
                sameDifferenceValues &=
                currentA.registers.bx == previousA.registers.bx &&
                currentB.registers.bx == previousB.registers.bx;

            if (difference.cx)
                sameDifferenceValues &=
                currentA.registers.cx == previousA.registers.cx &&
                currentB.registers.cx == previousB.registers.cx;

            if (difference.dx)
                sameDifferenceValues &=
                currentA.registers.dx == previousA.registers.dx &&
                currentB.registers.dx == previousB.registers.dx;

            if (difference.si)
                sameDifferenceValues &=
                currentA.registers.si == previousA.registers.si &&
                currentB.registers.si == previousB.registers.si;

            if (difference.di)
                sameDifferenceValues &=
                currentA.registers.di == previousA.registers.di &&
                currentB.registers.di == previousB.registers.di;

            if (difference.bp)
                sameDifferenceValues &=
                currentA.registers.bp == previousA.registers.bp &&
                currentB.registers.bp == previousB.registers.bp;

            if (difference.sp)
                sameDifferenceValues &=
                currentA.registers.sp == previousA.registers.sp &&
                currentB.registers.sp == previousB.registers.sp;

            if (difference.ds)
                sameDifferenceValues &=
                currentA.registers.ds == previousA.registers.ds &&
                currentB.registers.ds == previousB.registers.ds;

            if (difference.es)
                sameDifferenceValues &=
                currentA.registers.es == previousA.registers.es &&
                currentB.registers.es == previousB.registers.es;

            if (difference.ss)
                sameDifferenceValues &=
                currentA.registers.ss == previousA.registers.ss &&
                currentB.registers.ss == previousB.registers.ss;

            if (!sameDifferenceValues)
            {
                return alignment.indexA;
            }
        }

        return invalidIndex;
    }

    size_t TraceDifferenceNavigation::findPreviousDifference(
        const std::vector<RuntimeInstruction>& traceA,
        const std::vector<RuntimeInstruction>& traceB,
        const std::vector<TraceAlignment>& alignments,
        size_t selectedIndexA,
        TraceDifferenceNavigation::DifferenceComparer comparer,
        ControlFlowComparer controlFlowComparer
    )
    {
        size_t result =
            invalidIndex;

        for (size_t alignmentIndex = 0;
            alignmentIndex < alignments.size();
            ++alignmentIndex)
        {
            const TraceAlignment& alignment =
                alignments[alignmentIndex];

            if (alignment.indexA >= selectedIndexA)
            {
                break;
            }

            if (!isDifference(
                traceA,
                traceB,
                alignment,
                comparer,
                controlFlowComparer
            ))
            {
                continue;
            }

            bool isDifferenceStart =
                false;

            if (alignmentIndex == 0)
            {
                isDifferenceStart =
                    true;
            }
            else
            {
                const TraceAlignment& previous =
                    alignments[alignmentIndex - 1];

                if (!isDifference(
                    traceA,
                    traceB,
                    previous,
                    comparer,
                    controlFlowComparer
                ))
                {
                    isDifferenceStart =
                        true;
                }
                else if (alignment.synchronized &&
                    previous.synchronized)
                {
                    const RuntimeInstruction& currentA =
                        traceA[alignment.indexA];

                    const RuntimeInstruction& currentB =
                        traceB[alignment.indexB];

                    const RuntimeInstruction& previousA =
                        traceA[previous.indexA];

                    const RuntimeInstruction& previousB =
                        traceB[previous.indexB];

                    const TraceInstructionDifference difference =
                        comparer(
                            currentA,
                            currentB
                        );

                    bool sameDifferenceValues =
                        true;

                    if (difference.ax)
                        sameDifferenceValues &=
                        currentA.registers.ax == previousA.registers.ax &&
                        currentB.registers.ax == previousB.registers.ax;

                    if (difference.bx)
                        sameDifferenceValues &=
                        currentA.registers.bx == previousA.registers.bx &&
                        currentB.registers.bx == previousB.registers.bx;

                    if (difference.cx)
                        sameDifferenceValues &=
                        currentA.registers.cx == previousA.registers.cx &&
                        currentB.registers.cx == previousB.registers.cx;

                    if (difference.dx)
                        sameDifferenceValues &=
                        currentA.registers.dx == previousA.registers.dx &&
                        currentB.registers.dx == previousB.registers.dx;

                    if (difference.si)
                        sameDifferenceValues &=
                        currentA.registers.si == previousA.registers.si &&
                        currentB.registers.si == previousB.registers.si;

                    if (difference.di)
                        sameDifferenceValues &=
                        currentA.registers.di == previousA.registers.di &&
                        currentB.registers.di == previousB.registers.di;

                    if (difference.bp)
                        sameDifferenceValues &=
                        currentA.registers.bp == previousA.registers.bp &&
                        currentB.registers.bp == previousB.registers.bp;

                    if (difference.sp)
                        sameDifferenceValues &=
                        currentA.registers.sp == previousA.registers.sp &&
                        currentB.registers.sp == previousB.registers.sp;

                    if (difference.ds)
                        sameDifferenceValues &=
                        currentA.registers.ds == previousA.registers.ds &&
                        currentB.registers.ds == previousB.registers.ds;

                    if (difference.es)
                        sameDifferenceValues &=
                        currentA.registers.es == previousA.registers.es &&
                        currentB.registers.es == previousB.registers.es;

                    if (difference.ss)
                        sameDifferenceValues &=
                        currentA.registers.ss == previousA.registers.ss &&
                        currentB.registers.ss == previousB.registers.ss;

                    if (!sameDifferenceValues)
                    {
                        isDifferenceStart =
                            true;
                    }
                }
            }

            if (isDifferenceStart)
            {
                result =
                    alignment.indexA;
            }
        }

        return result;
    }
}
