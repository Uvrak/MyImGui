#include "pch.h"
#include "TraceRegisterDivergenceFinder.h"

#include <cstdint>

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
        bool previousEqual = true;

        for (const TraceAlignment& entry : alignment)
        {
            if (!entry.synchronized)
            {
                previousEqual = true;
                continue;
            }

            if (entry.indexA <= startIndexA &&
                entry.indexB <= startIndexB)
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

            if (previousEqual && !equal)
            {
                resultIndexA = entry.indexA;
                resultIndexB = entry.indexB;

                return true;
            }

            previousEqual = equal;
        }

        return false;
    }
}