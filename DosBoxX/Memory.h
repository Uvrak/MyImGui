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

        bool writeByte(
            NamedPipeClient& pipeClient,
            std::size_t address,
            uint8_t value
        );
    };
}