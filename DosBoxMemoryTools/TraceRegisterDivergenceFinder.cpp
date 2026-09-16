#include "pch.h"
#include "TraceRegisterDivergenceFinder.h"

#include <cstdint>
#include <algorithm>

namespace DosBoxMemoryTools
{
    namespace
    {
        uint16_t registerValue(
            const RuntimeInstruction& instruction,
            TraceRegister traceRegister
        )
        {
            switch (traceRegister)
            {
            case TraceRegister::AX:
                return instruction.registers.ax;

            case TraceRegister::BX:
                return instruction.registers.bx;

            case TraceRegister::CX:
                return instruction.registers.cx;

            case TraceRegister::DX:
                return instruction.registers.dx;

            case TraceRegister::SI:
                return instruction.registers.si;

            case TraceRegister::DI:
                return instruction.registers.di;

            case TraceRegister::BP:
                return instruction.registers.bp;

            case TraceRegister::SP:
                return instruction.registers.sp;

            case TraceRegister::DS: 
                return instruction.registers.ds;

            case TraceRegister::ES:
                return instruction.registers.es;

            case TraceRegister::SS:
                return instruction.registers.ss;
            }

            return 0;
        }
    }

    bool TraceRegisterDivergenceFinder::findNext(
        const std::vector<RuntimeInstruction>& traceA,
        const std::vector<RuntimeInstruction>& traceB,
        const std::vector<TraceAlignment>& alignment,
        TraceRegister traceRegister,
        size_t startIndexA,
        size_t startIndexB,
        size_t& resultIndexA,
        size_t& resultIndexB
    )
    {
        bool havePrevious = false;
        bool previousEqual = false;

        for (const TraceAlignment& entry : alignment)
        {
            if (!entry.synchronized)
            {
                continue;
            }

            if (entry.indexA >= traceA.size() ||
                entry.indexB >= traceB.size())
            {
                continue;
            }

            const bool equal =
                registerValue(
                    traceA[entry.indexA],
                    traceRegister
                ) ==
                registerValue(
                    traceB[entry.indexB],
                    traceRegister
                );

            const bool afterStart =
                entry.indexA > startIndexA ||
                entry.indexB > startIndexB;

            if (afterStart &&
                havePrevious &&
                previousEqual &&
                !equal)
            {
                resultIndexA = entry.indexA;
                resultIndexB = entry.indexB;

                return true;
            }

            previousEqual = equal;
            havePrevious = true;
        }

        return false;
    }
    
    bool TraceRegisterDivergenceFinder::findPrevious(
        const std::vector<RuntimeInstruction>& traceA,
        const std::vector<RuntimeInstruction>& traceB,
        const std::vector<TraceAlignment>& alignment,
        TraceRegister traceRegister,
        size_t startIndexA,
        size_t startIndexB,
        size_t& resultIndexA,
        size_t& resultIndexB
    )
    {
        bool havePrevious = false;
        bool previousEqual = false;
        bool found = false;

        for (const TraceAlignment& entry : alignment)
        {
            if (!entry.synchronized)
            {
                continue;
            }

            if (entry.indexA >= traceA.size() ||
                entry.indexB >= traceB.size())
            {
                continue;
            }

            const bool equal =
                registerValue(
                    traceA[entry.indexA],
                    traceRegister
                ) ==
                registerValue(
                    traceB[entry.indexB],
                    traceRegister
                );

            const bool beforeStart =
                entry.indexA < startIndexA ||
                entry.indexB < startIndexB;

            if (beforeStart &&
                havePrevious &&
                previousEqual &&
                !equal)
            {
                resultIndexA = entry.indexA;
                resultIndexB = entry.indexB;
                found = true;
            }

            previousEqual = equal;
            havePrevious = true;
        }

        return found;
    }
   
    bool TraceRegisterDivergenceFinder::findPreviousChange(
        const std::vector<RuntimeInstruction>& trace,
        TraceRegister traceRegister,
        size_t startIndex,
        size_t& resultIndex
    )
    {
        if (trace.empty() ||
            startIndex == 0)
        {
            return false;
        }

        size_t index =
            (std::min)(startIndex, trace.size() - 1);

        while (index > 0)
        {
            const uint16_t currentValue =
                registerValue(
                    trace[index],
                    traceRegister
                );

            const uint16_t previousValue =
                registerValue(
                    trace[index - 1],
                    traceRegister
                );

            if (currentValue != previousValue)
            {
                const uint16_t difference =
                    static_cast<uint16_t>(
                        currentValue - previousValue
                        );

                // Ignore automatic SI advancement caused by
                // string-load instructions such as LODSB/LODSW.
                if (traceRegister == TraceRegister::SI &&
                    (difference == 1 || difference == 2))
                {
                    --index;
                    continue;
                }

                resultIndex = index;
                return true;
            }

            --index;
        }

        return false;
    }
}