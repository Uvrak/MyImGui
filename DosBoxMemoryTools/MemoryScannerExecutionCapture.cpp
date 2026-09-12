#include "MemoryScanner.h"

#include <sstream>
#include <cstdlib>
#include <utility>

namespace DosBoxMemoryTools
{
    bool MemoryScanner::
        setExecutionCaptureTarget(
            size_t address
        )
    {
        std::string response;

        const std::string command =
            "EXECUTIONCAPTURE:TARGET:" +
            std::to_string(
                address
            );

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            m_status =
                "Could not set execution capture target.";

            return false;
        }

        if (response != "OK")
        {
            m_status =
                "Setting execution capture target failed: " +
                response;

            return false;
        }

        m_status =
            "Execution capture target set.";

        return true;
    }

    bool MemoryScanner::
        getExecutionCaptureTarget(
            size_t& address
        )
    {
        std::string response;

        if (!m_pipeClient.request(
            "EXECUTIONCAPTURE:TARGETGET",
            response
        ))
        {
            m_status =
                "Could not get execution capture target.";

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
                "Invalid execution capture target: " +
                response;

            return false;
        }

        return true;
    }

    bool MemoryScanner::
        clearExecutionCapture()
    {
        std::string response;

        if (!m_pipeClient.request(
            "EXECUTIONCAPTURE:CLEAR",
            response
        ))
        {
            m_status =
                "Could not clear execution capture.";

            return false;
        }

        if (response != "OK")
        {
            m_status =
                "Clearing execution capture failed: " +
                response;

            return false;
        }

        m_status =
            "Execution capture cleared.";

        return true;
    }

    bool MemoryScanner::
        getExecutionCaptureHit(
            bool& hit
        )
    {
        std::string response;

        if (!m_pipeClient.request(
            "EXECUTIONCAPTURE:HIT",
            response
        ))
        {
            m_status =
                "Could not get execution capture state.";

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
                "Invalid execution capture state: " +
                response;

            return false;
        }

        return true;
    }

    bool MemoryScanner::
        getExecutionCapture(
            RuntimeInstruction& instruction
        )
    {
        std::string response;

        if (!m_pipeClient.request(
            "EXECUTIONCAPTURE:GET",
            response
        ))
        {
            m_status =
                "Could not get execution capture.";

            return false;
        }

        if (response.rfind(
            "ERROR",
            0
        ) == 0)
        {
            m_status =
                "Execution capture failed: " +
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
                        "Too many execution capture bytes.";

                    return false;
                }

                const unsigned long value =
                    std::stoul(
                        byteText
                    );

                if (value > 255)
                {
                    m_status =
                        "Invalid execution capture byte.";

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
                    "Invalid execution capture byte count.";

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
                        "Too many execution capture stack bytes.";

                    return false;
                }

                const unsigned long value =
                    std::stoul(
                        stackByteText
                    );

                if (value > 255)
                {
                    m_status =
                        "Invalid execution capture stack byte.";

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
                    "Invalid execution capture stack byte count.";

                return false;
            }

            instruction =
                result;
        }
        catch (...)
        {
            m_status =
                "Invalid execution capture data.";

            return false;
        }

        m_status =
            "Execution capture loaded.";

        return true;
    }

}
