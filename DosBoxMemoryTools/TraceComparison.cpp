#include "TraceComparison.h"

namespace DosBoxMemoryTools
{
    TraceInstructionDifference
        compareTraceInstructions(
            const RuntimeInstruction& current,
            const RuntimeInstruction& reference
        )
    {
        TraceInstructionDifference result;

        result.address =
            current.address !=
            reference.address;

        result.ax =
            current.registers.ax !=
            reference.registers.ax;

        result.bx =
            current.registers.bx !=
            reference.registers.bx;

        result.cx =
            current.registers.cx !=
            reference.registers.cx;

        result.dx =
            current.registers.dx !=
            reference.registers.dx;

        result.si =
            current.registers.si !=
            reference.registers.si;

        result.di =
            current.registers.di !=
            reference.registers.di;

        result.bp =
            current.registers.bp !=
            reference.registers.bp;

        result.sp =
            current.registers.sp !=
            reference.registers.sp;

        result.ds =
            current.registers.ds !=
            reference.registers.ds;

        result.es =
            current.registers.es !=
            reference.registers.es;

        result.ss =
            current.registers.ss !=
            reference.registers.ss;

        return result;
    }
}