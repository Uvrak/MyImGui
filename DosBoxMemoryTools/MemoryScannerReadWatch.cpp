#include "MemoryScanner.h"

#include <cstdlib>
#include <sstream>
#include <vector>
#include <string>

namespace DosBoxMemoryTools
{
    bool MemoryScanner::setMemoryReadWatchTarget(
        size_t address
    )
    {
        std::string response;

        const std::string command =
            "MEMORYREAD:TARGET:" +
            std::to_string(
                address
            );

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            m_status =
                "Could not set memory read target.";

            return false;
        }

        if (response != "OK")
        {
            m_status =
                "Setting memory read target failed: " +
                response;

            return false;
        }

        m_status =
            "Memory read target set.";

        return true;
    }

    bool MemoryScanner::clearMemoryReadWatch()
    {
        std::string response;

        if (!m_pipeClient.request(
            "MEMORYREAD:CLEAR",
            response
        ))
        {
            m_status =
                "Could not clear memory read watch.";

            return false;
        }

        if (response != "OK")
        {
            m_status =
                "Clearing memory read watch failed: " +
                response;

            return false;
        }

        m_status =
            "Memory read watch cleared.";

        return true;
    }

    bool MemoryScanner::getMemoryReadWatchHit(
        bool& hit
    )
    {
        std::string response;

        if (!m_pipeClient.request(
            "MEMORYREAD:HIT",
            response
        ))
        {
            m_status =
                "Could not get memory read watch state.";

            return false;
        }

        if (response == "1")
        {
            hit = true;
        }
        else if (response == "0")
        {
            hit = false;
        }
        else
        {
            m_status =
                "Invalid memory read watch state: " +
                response;

            return false;
        }

        return true;
    }

    bool MemoryScanner::getMemoryReadWatchCapture(
        RuntimeInstruction& capture
    )
    {
        std::string response;

        if (!m_pipeClient.request(
            "MEMORYREAD:GET",
            response
        ))
        {
            m_status =
                "Could not get memory read capture.";

            return false;
        }

        if (response.rfind(
            "ERROR",
            0
        ) == 0)
        {
            m_status =
                "Memory read capture failed: " +
                response;

            return false;
        }

        std::vector<std::string>
            fields;

        std::stringstream stream(
            response
        );

        std::string field;

        while (std::getline(
            stream,
            field,
            ':'
        ))
        {
            fields.push_back(
                field
            );
        }

        if (fields.size() != 16)
        {
            m_status =
                "Invalid memory read capture field count: " +
                std::to_string(
                    fields.size()
                );

            return false;
        }

        try
        {
            RuntimeInstruction result;

            result.address =
                static_cast<size_t>(
                    std::stoull(fields[0])
                    );

            result.readAddress =
                static_cast<size_t>(
                    std::stoull(fields[1])
                    );

            result.cs =
                static_cast<uint16_t>(
                    std::stoul(fields[2])
                    );

            result.ip =
                static_cast<uint16_t>(
                    std::stoul(fields[3])
                    );

            result.registers.ax =
                static_cast<uint16_t>(
                    std::stoul(fields[4])
                    );

            result.registers.bx =
                static_cast<uint16_t>(
                    std::stoul(fields[5])
                    );

            result.registers.cx =
                static_cast<uint16_t>(
                    std::stoul(fields[6])
                    );

            result.registers.dx =
                static_cast<uint16_t>(
                    std::stoul(fields[7])
                    );

            result.registers.si =
                static_cast<uint16_t>(
                    std::stoul(fields[8])
                    );

            result.registers.di =
                static_cast<uint16_t>(
                    std::stoul(fields[9])
                    );

            result.registers.bp =
                static_cast<uint16_t>(
                    std::stoul(fields[10])
                    );

            result.registers.sp =
                static_cast<uint16_t>(
                    std::stoul(fields[11])
                    );
            result.registers.ds =
                static_cast<uint16_t>(
                    std::stoul(fields[12])
                    );

            result.registers.es =
                static_cast<uint16_t>(
                    std::stoul(fields[13])
                    );

            result.registers.ss =
                static_cast<uint16_t>(
                    std::stoul(fields[14])
                    );

            std::stringstream byteStream(
                fields[15]
            );

            std::string byteText;
            size_t byteIndex = 0;

            while (std::getline(
                byteStream,
                byteText,
                '.'
            ))
            {
                if (byteIndex >=
                    result.bytes.size())
                {
                    return false;
                }

                result.bytes[byteIndex] =
                    static_cast<uint8_t>(
                        std::stoul(byteText)
                        );

                ++byteIndex;
            }

            if (byteIndex !=
                result.bytes.size())
            {
                return false;
            }

            capture =
                result;
        }
        catch (...)
        {
            m_status =
                "Invalid memory read capture data.";

            return false;
        }

        m_status =
            "Memory read capture loaded.";

        return true;
    }
}