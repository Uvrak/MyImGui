#pragma once

#include <cstddef>
#include <cstdint>

namespace DosBoxX
{
    class NamedPipeClient;

    class Memory
    {
    public:
        bool readByte(
            NamedPipeClient& pipeClient,
            std::size_t address,
            uint8_t& value
        );

        bool writeValue(
            NamedPipeClient& pipeClient,
            std::size_t address,
            uint32_t value,
            std::size_t size
        );
    };
}