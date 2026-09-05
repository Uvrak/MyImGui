#include "Memory.h"

#include "NamedPipeClient.h"

#include <string>

namespace DosBoxX
{
    bool Memory::readByte(
        NamedPipeClient& pipeClient,
        std::size_t address,
        uint8_t& value
    )
    {
        const std::string command =
            "MEMORYREADBYTE:" +
            std::to_string(address);

        std::string response;

        if (!pipeClient.request(
            command,
            response
        ))
        {
            return false;
        }

        try
        {
            const unsigned long parsedValue =
                std::stoul(
                    response
                );

            if (parsedValue > 255)
            {
                return false;
            }

            value =
                static_cast<uint8_t>(
                    parsedValue
                    );

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool Memory::writeByte(
        NamedPipeClient& pipeClient,
        std::size_t address,
        uint8_t value
    )
    {
        const std::string command =
            "MEMORYWRITEBYTE:" +
            std::to_string(address) +
            ":" +
            std::to_string(
                static_cast<unsigned int>(
                    value
                    )
            );

        return
            pipeClient.send(
                command
            );
    }
}