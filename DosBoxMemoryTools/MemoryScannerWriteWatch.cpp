#include "MemoryScanner.h"

#include <sstream>
#include <cstdlib>
#include <utility>

namespace DosBoxMemoryTools
{
    void MemoryScanner::setMemoryWriteMarker(
        size_t index
    )
    {
        m_memoryWriteMarker =
            index;

        m_memoryWriteMarkerValid =
            true;
    }

    bool MemoryScanner::getMemoryWriteMarker(
        size_t& index
    ) const
    {
        if (!m_memoryWriteMarkerValid)
        {
            return false;
        }

        index =
            m_memoryWriteMarker;

        return true;
    }

    bool MemoryScanner::
        setMemoryWriteWatchTarget(
            size_t address
        )
    {
        std::string response;

        const std::string command =
            "MEMORYWRITE:TARGET:" +
            std::to_string(
                address
            );

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            m_status =
                "Could not set memory write target.";

            return false;
        }

        if (response != "OK")
        {
            m_status =
                "Setting memory write target failed: " +
                response;

            return false;
        }

        m_memoryWriteMarkerValid =
            false;

        m_status =
            "Memory write target set.";

        return true;
    }

    bool MemoryScanner::
        setMemoryWriteWatchRange(
            size_t startAddress,
            size_t endAddress
        )
    {
        std::string response;

        const std::string command =
            "MEMORYWRITE:RANGE:" +
            std::to_string(
                startAddress
            ) +
            ":" +
            std::to_string(
                endAddress
            );

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            m_status =
                "Could not set memory write range.";

            return false;
        }

        if (response != "OK")
        {
            m_status =
                "Setting memory write range failed: " +
                response;

            return false;
        }

        m_memoryWriteMarkerValid =
            false;

        m_status =
            "Memory write range set.";

        return true;
    }

    bool MemoryScanner::
        clearMemoryWriteWatch()
    {
        std::string response;

        if (!m_pipeClient.request(
            "MEMORYWRITE:CLEAR",
            response
        ))
        {
            m_status =
                "Could not clear memory write watch.";

            return false;
        }

        if (response != "OK")
        {
            m_status =
                "Clearing memory write watch failed: " +
                response;

            return false;
        }

        m_status =
            "Memory write watch cleared.";

        return true;
    }

    bool MemoryScanner::
        getMemoryWriteWatchHit(
            bool& hit
        )
    {
        std::string response;

        if (!m_pipeClient.request(
            "MEMORYWRITE:HIT",
            response
        ))
        {
            m_status =
                "Could not get memory write watch state.";

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
                "Invalid memory write watch state: " +
                response;

            return false;
        }

        return true;
    }

    bool MemoryScanner::
        getMemoryWriteWatchCaptureCount(
            size_t& count
        )
    {

        std::string response;

        if (!m_pipeClient.request(
            "MEMORYWRITE:COUNT",
            response
        ))
        {
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
            return false;
        }

        return true;
    }

    

    bool MemoryScanner::getMemoryWriteWatchCapture(
        size_t index,
        RuntimeInstruction& instruction
    )
    {


        std::string response;

        if (!m_pipeClient.request(
            "MEMORYWRITE:GET:" +
            std::to_string(
                index
            ),
            response
        ))
        {
            m_status =
                "Could not get memory write capture.";

            return false;
        }

        if (response.rfind(
            "ERROR",
            0
        ) == 0)
        {
            m_status =
                "Memory write capture failed: " +
                response;

            return false;
        }
        OutputDebugStringA(
            ("MEMORYWRITE RAW: " + response + "\n").c_str()
        );

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

        if (fields.size() != 18)
        {
            m_status =
                "Invalid memory write capture field count: " +
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
                        "Too many memory write capture bytes.";

                    return false;
                }

                const unsigned long value =
                    std::stoul(
                        byteText
                    );

                if (value > 255)
                {
                    m_status =
                        "Invalid memory write capture byte.";

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
                    "Invalid memory write capture byte count.";

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
                        "Too many memory write capture stack bytes.";

                    return false;
                }

                const unsigned long value =
                    std::stoul(
                        stackByteText
                    );

                if (value > 255)
                {
                    m_status =
                        "Invalid memory write capture stack byte.";

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
                    "Invalid memory write capture stack byte count.";

                return false;
            }

            result.writeAddress =
                static_cast<size_t>(
                    std::stoull(
                        fields[17]
                    )
                    );

            instruction =
                result;

            m_lastMemoryWriteInstruction =
                result.address;
        }
        catch (...)
        {
            m_status =
                "Invalid memory write capture data.";

            return false;
        }

        m_status =
            "Memory write capture loaded.";

        return true;
    }

}
