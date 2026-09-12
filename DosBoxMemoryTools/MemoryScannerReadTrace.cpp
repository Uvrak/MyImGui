#include "MemoryScanner.h"

#include <sstream>
#include <cstdlib>
#include <utility>

namespace DosBoxMemoryTools
{
    bool MemoryScanner::setReadTraceInstructionLimit(
        size_t count
    )
    {
        std::string response;

        if (!m_pipeClient.request(
            "READTRACE:LIMIT:" +
            std::to_string(
                count
            ),
            response
        ))
        {
            return false;
        }

        return response ==
            "OK";
    }

    bool MemoryScanner::
        setReadTraceTarget(
            size_t address
        )
    {
        std::string response;

        const std::string command =
            "READTRACE:TARGET:" +
            std::to_string(
                address
            );

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            m_status =
                "Could not set read trace target.";

            return false;
        }

        if (response != "OK")
        {
            m_status =
                "Setting read trace target failed: " +
                response;

            return false;
        }

        m_status =
            "Read trace target set.";

        return true;
    }

    bool MemoryScanner::
        getReadTraceTarget(
            size_t& address
        )
    {
        std::string response;

        if (!m_pipeClient.request(
            "READTRACE:TARGET",
            response
        ))
        {
            m_status =
                "Could not get read trace target.";

            return false;
        }

        try
        {
            address =
                static_cast<size_t>(
                    std::stoull(
                        response
                    )
                    );
        }
        catch (...)
        {
            m_status =
                "Invalid read trace target: " +
                response;

            return false;
        }

        return true;
    }

    bool MemoryScanner::
        getReadTraceActive(
            bool& active
        )
    {
        std::string response;

        if (!m_pipeClient.request(
            "READTRACE:ACTIVE",
            response
        ))
        {
            m_status =
                "Could not get read trace state.";

            return false;
        }

        if (response == "1")
        {
            active = true;
        }
        else if (response == "0")
        {
            active = false;
        }
        else
        {
            m_status =
                "Invalid read trace state: " +
                response;

            return false;
        }

        return true;
    }

    bool MemoryScanner::
        getReadTraceCount(
            size_t& count
        )
    {
        std::string response;

        if (!m_pipeClient.request(
            "READTRACE:COUNT",
            response
        ))
        {
            m_status =
                "Could not get read trace count.";

            return false;
        }

        try
        {
            count =
                static_cast<size_t>(
                    std::stoull(
                        response
                    )
                    );
        }
        catch (...)
        {
            m_status =
                "Invalid read trace count: " +
                response;

            return false;
        }

        return true;
    }

    bool MemoryScanner::
        getReadTraceInstruction(
            size_t index,
            RuntimeInstruction& instruction
        )
    {
        std::string response;

        const std::string command =
            "READTRACE:GET:" +
            std::to_string(
                index
            );

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            m_status =
                "Could not get read trace instruction.";

            return false;
        }

        if (response.rfind(
            "ERROR",
            0
        ) == 0)
        {
            m_status =
                "Read trace instruction failed: " +
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
                "Invalid read trace field count: " +
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
                    std::stoull(
                        fields[0]
                    )
                    );

            result.cs =
                static_cast<uint16_t>(
                    std::stoul(
                        fields[1]
                    )
                    );

            result.ip =
                static_cast<uint16_t>(
                    std::stoul(
                        fields[2]
                    )
                    );

            result.registers.ax =
                static_cast<uint16_t>(
                    std::stoul(fields[3])
                    );

            result.registers.bx =
                static_cast<uint16_t>(
                    std::stoul(fields[4])
                    );

            result.registers.cx =
                static_cast<uint16_t>(
                    std::stoul(fields[5])
                    );

            result.registers.dx =
                static_cast<uint16_t>(
                    std::stoul(fields[6])
                    );

            result.registers.si =
                static_cast<uint16_t>(
                    std::stoul(fields[7])
                    );

            result.registers.di =
                static_cast<uint16_t>(
                    std::stoul(fields[8])
                    );

            result.registers.bp =
                static_cast<uint16_t>(
                    std::stoul(fields[9])
                    );

            result.registers.sp =
                static_cast<uint16_t>(
                    std::stoul(fields[10])
                    );

            result.registers.ds =
                static_cast<uint16_t>(
                    std::stoul(fields[11])
                    );

            result.registers.es =
                static_cast<uint16_t>(
                    std::stoul(fields[12])
                    );

            result.registers.ss =
                static_cast<uint16_t>(
                    std::stoul(fields[13])
                    );

            std::stringstream byteStream(
                fields[14]
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
                    m_status =
                        "Too many read trace bytes.";

                    return false;
                }

                const unsigned long value =
                    std::stoul(
                        byteText
                    );

                if (value > 255)
                {
                    m_status =
                        "Invalid read trace byte.";

                    return false;
                }

                result.bytes[
                    byteIndex
                ] =
                    static_cast<uint8_t>(
                        value
                        );

                    ++byteIndex;
            }

            if (byteIndex !=
                result.bytes.size())
            {
                m_status =
                    "Invalid read trace byte count.";

                return false;
            }

            std::stringstream stackByteStream(
                fields[15]
            );

            std::string stackByteText;
            size_t stackByteIndex = 0;

            while (std::getline(
                stackByteStream,
                stackByteText,
                '.'
            ))
            {
                if (stackByteIndex >=
                    result.stackBytes.size())
                {
                    m_status =
                        "Too many read trace stack bytes.";

                    return false;
                }

                const unsigned long value =
                    std::stoul(
                        stackByteText
                    );

                if (value > 255)
                {
                    m_status =
                        "Invalid read trace stack byte.";

                    return false;
                }

                result.stackBytes[
                    stackByteIndex
                ] =
                    static_cast<uint8_t>(
                        value
                        );

                    ++stackByteIndex;
            }

            if (stackByteIndex !=
                result.stackBytes.size())
            {
                m_status =
                    "Invalid read trace stack byte count.";

                return false;
            }

            instruction =
                result;
        }
        catch (...)
        {
            m_status =
                "Invalid read trace instruction data.";

            return false;
        }

        return true;
    }

    bool MemoryScanner::
        getReadTraceArmed(
            bool& armed
        )
    {
        std::string response;

        if (!m_pipeClient.request(
            "READTRACE:ARMED",
            response
        ))
        {
            m_status =
                "Could not get read trace armed state.";

            return false;
        }

        if (response == "1")
        {
            armed = true;
            return true;
        }

        if (response == "0")
        {
            armed = false;
            return true;
        }

        m_status =
            "Invalid read trace armed state: " +
            response;

        return false;
    }

}
