#pragma once

#include <cstddef>
#include <cstdint>
#include <array>

namespace DosBoxMemoryTools
{
    enum class MemoryScanMode
    {
        NewScan,
        UnknownInitialValue,
        ExactValue,
        Changed,
        Unchanged,
        Increased,
        Decreased
    };

    enum class MemoryValueType
    {
        Byte,
        Short,
        Int
    };

    struct MemoryCandidate
    {
        size_t address = 0;

        uint32_t previousValue = 0;
        uint32_t currentValue = 0;
    };

    struct RegisterSnapshot
    {
        uint16_t ax = 0;
        uint16_t bx = 0;
        uint16_t cx = 0;
        uint16_t dx = 0;

        uint16_t si = 0;
        uint16_t di = 0;
        uint16_t bp = 0;
        uint16_t sp = 0;

        uint16_t ds = 0;
        uint16_t es = 0;
        uint16_t ss = 0;
    };

    struct RuntimeInstruction
    {
        size_t address = 0;
        size_t writeAddress = 0;

        uint16_t cs = 0;
        uint16_t ip = 0;

        RegisterSnapshot registers;

        std::array<uint8_t, 16>
            bytes{};

        std::array<uint8_t, 32>
            stackBytes{};

        size_t readAddress = 0;
        uint8_t writeValue = 0;
    };

}
