#include "MemoryScanner.h"

#include <sstream>
#include <cstdlib>
#include <utility>

namespace DosBoxMemoryTools
{
    bool MemoryScanner::
        startTransitionTracking()
    {
        std::string response;

        if (!m_pipeClient.request(
            "READTRACK:TRANSITIONSTART",
            response
        ))
        {
            m_status =
                "Could not start transition tracking.";

            return false;
        }

        if (response != "OK")
        {
            m_status =
                "Transition tracking start failed: " +
                response;

            return false;
        }

        m_status =
            "Transition tracking started.";

        return true;
    }

    bool MemoryScanner::
        stopTransitionTracking()
    {
        std::string response;

        if (!m_pipeClient.request(
            "READTRACK:TRANSITIONSTOP",
            response
        ))
        {
            m_status =
                "Could not stop transition tracking.";

            return false;
        }

        if (response != "OK")
        {
            m_status =
                "Transition tracking stop failed: " +
                response;

            return false;
        }

        m_status =
            "Transition tracking stopped.";

        return true;
    }

    bool MemoryScanner::
        clearTransitionTracking()
    {
        std::string response;

        if (!m_pipeClient.request(
            "READTRACK:TRANSITIONCLEAR",
            response
        ))
        {
            m_status =
                "Could not clear transition tracking.";

            return false;
        }

        if (response != "OK")
        {
            m_status =
                "Transition tracking clear failed: " +
                response;

            return false;
        }

        return true;
    }

    bool MemoryScanner::
        setReadTrackingTransitionTarget(
            size_t address
        )
    {
        std::string response;

        const std::string command =
            "READTRACK:TRANSITIONTARGET:" +
            std::to_string(address);

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            m_status =
                "Could not set transition target.";

            return false;
        }

        if (response != "OK")
        {
            m_status =
                "Setting transition target failed: " +
                response;

            return false;
        }

        return true;
    }

    bool MemoryScanner::
        getReadTrackingTransitionCount(
            size_t& count
        )
    {
        std::string response;

        if (!m_pipeClient.request(
            "READTRACK:TRANSITIONCOUNT",
            response
        ))
        {
            m_status =
                "Could not get transition count.";

            return false;
        }

        try
        {
            count =
                static_cast<size_t>(
                    std::stoull(response)
                    );
        }
        catch (...)
        {
            m_status =
                "Invalid transition count: " +
                response;

            return false;
        }

        return true;
    }

    bool MemoryScanner::
        getReadTrackingTransitionContextCount(
            size_t& count
        )
    {
        std::string response;

        if (!m_pipeClient.request(
            "READTRACK:TRANSITIONCONTEXTCOUNT",
            response
        ))
        {
            m_status =
                "Could not get transition context count.";

            return false;
        }

        try
        {
            count =
                static_cast<size_t>(
                    std::stoull(response)
                    );
        }
        catch (...)
        {
            m_status =
                "Invalid transition context count: " +
                response;

            return false;
        }

        return true;
    }

    bool MemoryScanner::
        getReadTrackingTransitionContextBlock(
            size_t start,
            size_t count,
            std::vector<std::pair<uint16_t, uint16_t>>&
            contexts
        )
    {
        std::string response;

        const std::string command =
            "READTRACK:TRANSITIONCONTEXTS:" +
            std::to_string(start) +
            ":" +
            std::to_string(count);

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            m_status =
                "Could not get transition context block.";

            return false;
        }

        m_lastTransitionContextResponse =
            response;

        if (response.rfind(
            "ERROR",
            0
        ) == 0)
        {
            m_status =
                "Transition context block failed: " +
                response;

            return false;
        }

        contexts.clear();

        std::stringstream stream(
            response
        );

        std::string item;

        while (std::getline(
            stream,
            item,
            ','
        ))
        {
            if (item.empty())
            {
                continue;
            }

            const size_t separator =
                item.find(':');

            if (separator ==
                std::string::npos)
            {
                contexts.clear();
                return false;
            }

            try
            {
                const uint16_t cs =
                    static_cast<uint16_t>(
                        std::stoul(
                            item.substr(
                                0,
                                separator
                            )
                        )
                        );

                const uint16_t ip =
                    static_cast<uint16_t>(
                        std::stoul(
                            item.substr(
                                separator + 1
                            )
                        )
                        );

                contexts.emplace_back(
                    cs,
                    ip
                );
            }
            catch (...)
            {
                contexts.clear();
                return false;
            }
        }

        return true;
    }

    bool MemoryScanner::
        getReadTrackingTransitionContexts(
            std::vector<std::pair<uint16_t, uint16_t>>&
            contexts
        )
    {
        size_t count = 0;

        if (!getReadTrackingTransitionContextCount(
            count
        ))
        {
            return false;
        }

        m_status =
            "Transition context server count: " +
            std::to_string(count);

        contexts.clear();
        contexts.reserve(
            count
        );

        constexpr size_t blockSize = 64;

        for (size_t start = 0;
            start < count;
            start += blockSize)
        {
            std::vector<std::pair<uint16_t, uint16_t>>
                block;

            if (!getReadTrackingTransitionContextBlock(
                start,
                blockSize,
                block
            ))
            {
                contexts.clear();

                return false;
            }

            contexts.insert(
                contexts.end(),
                block.begin(),
                block.end()
            );
        }

        return true;
    }

    bool MemoryScanner::
        getReadTrackingTransitionBlock(
            size_t start,
            size_t count,
            std::vector<std::pair<size_t, size_t>>&
            transitions
        )
    {
        std::string response;

        const std::string command =
            "READTRACK:TRANSITIONS:" +
            std::to_string(start) +
            ":" +
            std::to_string(count);

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            m_status =
                "Could not get transition block.";

            return false;
        }

        if (response.rfind(
            "ERROR",
            0
        ) == 0)
        {
            m_status =
                "Transition block failed: " +
                response;

            return false;
        }

        transitions.clear();

        std::stringstream stream(
            response
        );

        std::string item;

        while (std::getline(
            stream,
            item,
            ','
        ))
        {
            if (item.empty())
            {
                continue;
            }

            const size_t separator =
                item.find(':');

            if (separator ==
                std::string::npos)
            {
                transitions.clear();
                return false;
            }

            try
            {
                const size_t previous =
                    static_cast<size_t>(
                        std::stoull(
                            item.substr(
                                0,
                                separator
                            )
                        )
                        );

                const size_t current =
                    static_cast<size_t>(
                        std::stoull(
                            item.substr(
                                separator + 1
                            )
                        )
                        );

                transitions.emplace_back(
                    previous,
                    current
                );
            }
            catch (...)
            {
                transitions.clear();
                return false;
            }
        }

        return true;
    }

    bool MemoryScanner::
        getReadTrackingTransitions(
            std::vector<std::pair<size_t, size_t>>&
            transitions
        )
    {
        size_t count = 0;

        if (!getReadTrackingTransitionCount(
            count
        ))
        {
            return false;
        }

        transitions.clear();
        transitions.reserve(
            count
        );

        constexpr size_t blockSize = 64;

        for (size_t start = 0;
            start < count;
            start += blockSize)
        {
            std::vector<std::pair<size_t, size_t>>
                block;

            if (!getReadTrackingTransitionBlock(
                start,
                blockSize,
                block
            ))
            {
                transitions.clear();

                return false;
            }

            transitions.insert(
                transitions.end(),
                block.begin(),
                block.end()
            );
        }

        return true;
    }

    bool MemoryScanner::
        getReadTrackingTransitionByteCount(
            size_t& count
        )
    {
        std::string response;

        if (!m_pipeClient.request(
            "READTRACK:TRANSITIONBYTECOUNT",
            response
        ))
        {
            m_status =
                "Could not get transition byte count.";

            return false;
        }

        try
        {
            count =
                static_cast<size_t>(
                    std::stoull(response)
                    );
        }
        catch (...)
        {
            m_status =
                "Invalid transition byte count: " +
                response;

            return false;
        }

        return true;
    }

    bool MemoryScanner::
        getReadTrackingTransitionByteBlock(
            size_t start,
            size_t count,
            std::vector<std::array<uint8_t, 16>>& bytes
        )
    {
        std::string response;

        const std::string command =
            "READTRACK:TRANSITIONBYTES:" +
            std::to_string(start) +
            ":" +
            std::to_string(count);

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            m_status =
                "Could not get transition byte block.";

            return false;
        }

        if (response.rfind(
            "ERROR",
            0
        ) == 0)
        {
            m_status =
                "Transition byte block failed: " +
                response;

            return false;
        }

        bytes.clear();

        std::stringstream stream(
            response
        );

        std::string item;

        while (std::getline(
            stream,
            item,
            ','
        ))
        {
            if (item.empty())
            {
                continue;
            }

            std::array<uint8_t, 16>
                instructionBytes{};

            std::stringstream itemStream(
                item
            );

            std::string byteText;

            size_t byteIndex = 0;

            while (std::getline(
                itemStream,
                byteText,
                ':'
            ))
            {
                if (byteIndex >=
                    instructionBytes.size())
                {
                    bytes.clear();
                    return false;
                }

                try
                {
                    const unsigned long value =
                        std::stoul(
                            byteText
                        );

                    if (value > 255)
                    {
                        bytes.clear();
                        return false;
                    }

                    instructionBytes[
                        byteIndex
                    ] =
                        static_cast<uint8_t>(
                            value
                            );
                }
                catch (...)
                {
                    bytes.clear();
                    return false;
                }

                ++byteIndex;
            }

            if (byteIndex !=
                instructionBytes.size())
            {
                bytes.clear();
                return false;
            }

            bytes.push_back(
                instructionBytes
            );
        }

        return true;
    }

    bool MemoryScanner::
        getReadTrackingTransitionBytes(
            std::vector<std::array<uint8_t, 16>>& bytes
        )
    {
        size_t count = 0;

        if (!getReadTrackingTransitionByteCount(
            count
        ))
        {
            return false;
        }

        bytes.clear();
        bytes.reserve(
            count
        );

        constexpr size_t blockSize = 64;

        for (size_t start = 0;
            start < count;
            start += blockSize)
        {
            std::vector<std::array<uint8_t, 16>>
                block;

            if (!getReadTrackingTransitionByteBlock(
                start,
                blockSize,
                block
            ))
            {
                bytes.clear();

                return false;
            }

            bytes.insert(
                bytes.end(),
                block.begin(),
                block.end()
            );
        }

        return true;
    }

    bool MemoryScanner::
        getReadTrackingTransitionHistory(
            size_t transitionIndex,
            std::vector<RuntimeInstruction>& history
        )
    {
        std::string response;

        const std::string command =
            "READTRACK:TRANSITIONHISTORY:" +
            std::to_string(
                transitionIndex
            );

        OutputDebugStringA(
            "TRANSITION HISTORY REQUEST: "
        );

        OutputDebugStringA(
            command.c_str()
        );

        OutputDebugStringA(
            "\n"
        );

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            OutputDebugStringA(
                "TRANSITION HISTORY REQUEST FAILED\n"
            );

            m_status =
                "Could not get transition history.";

            return false;
        }

        OutputDebugStringA(
            "TRANSITION HISTORY RESPONSE: "
        );

        OutputDebugStringA(
            response.c_str()
        );

        OutputDebugStringA(
            "\n"
        );

        if (response.rfind(
            "ERROR",
            0
        ) == 0)
        {
            m_status =
                "Transition history failed: " +
                response;

            return false;
        }

        history.clear();

        std::stringstream stream(
            response
        );

        std::string item;

        while (std::getline(
            stream,
            item,
            ','
        ))
        {
            if (item.empty())
            {
                continue;
            }

            std::vector<std::string> fields;

            std::stringstream itemStream(
                item
            );

            std::string field;

            while (std::getline(
                itemStream,
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

                OutputDebugStringA(
                    "HISTORY ERROR: FIELD COUNT\n"
                );

                history.clear();

                m_status =
                    "Invalid transition history field count.";

                return false;
            }

            OutputDebugStringA(
                "TRANSITION HISTORY RAW:\n"
            );

            OutputDebugStringA(
                response.c_str()
            );

            OutputDebugStringA(
                "\n"
            );

            try
            {
                RuntimeInstruction instruction;

                instruction.address =
                    static_cast<size_t>(
                        std::stoull(
                            fields[0]
                        )
                        );

                instruction.cs =
                    static_cast<uint16_t>(
                        std::stoul(
                            fields[1]
                        )
                        );

                instruction.ip =
                    static_cast<uint16_t>(
                        std::stoul(
                            fields[2]
                        )
                        );

                instruction.registers.ax =
                    static_cast<uint16_t>(
                        std::stoul(fields[3])
                        );

                instruction.registers.bx =
                    static_cast<uint16_t>(
                        std::stoul(fields[4])
                        );

                instruction.registers.cx =
                    static_cast<uint16_t>(
                        std::stoul(fields[5])
                        );

                instruction.registers.dx =
                    static_cast<uint16_t>(
                        std::stoul(fields[6])
                        );

                instruction.registers.si =
                    static_cast<uint16_t>(
                        std::stoul(fields[7])
                        );

                instruction.registers.di =
                    static_cast<uint16_t>(
                        std::stoul(fields[8])
                        );

                instruction.registers.bp =
                    static_cast<uint16_t>(
                        std::stoul(fields[9])
                        );

                instruction.registers.sp =
                    static_cast<uint16_t>(
                        std::stoul(fields[10])
                        );

                instruction.registers.ds =
                    static_cast<uint16_t>(
                        std::stoul(fields[11])
                        );

                instruction.registers.es =
                    static_cast<uint16_t>(
                        std::stoul(fields[12])
                        );

                instruction.registers.ss =
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
                        instruction.bytes.size())
                    {
                        OutputDebugStringA(
                            "HISTORY ERROR: TOO MANY BYTES\n"
                        );

                        history.clear();

                        return false;
                    }

                    const unsigned long value =
                        std::stoul(
                            byteText
                        );

                    if (value > 255)
                    {
                        history.clear();

                        m_status =
                            "Invalid transition history byte.";

                        return false;
                    }

                    instruction.bytes[
                        byteIndex
                    ] =
                        static_cast<uint8_t>(
                            value
                            );

                        ++byteIndex;
                }

                if (byteIndex !=
                    instruction.bytes.size())
                {
                    OutputDebugStringA(
                        "HISTORY ERROR: BYTE COUNT\n"
                    );

                    history.clear();

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
                        instruction.stackBytes.size())
                    {
                        history.clear();

                        m_status =
                            "Too many transition history stack bytes.";

                        return false;
                    }

                    const unsigned long value =
                        std::stoul(
                            stackByteText
                        );

                    if (value > 255)
                    {
                        history.clear();

                        m_status =
                            "Invalid transition history stack byte.";

                        return false;
                    }

                    instruction.stackBytes[
                        stackByteIndex
                    ] =
                        static_cast<uint8_t>(
                            value
                            );

                        ++stackByteIndex;
                }

                if (stackByteIndex !=
                    instruction.stackBytes.size())
                {
                    history.clear();

                    m_status =
                        "Invalid transition history stack byte count.";

                    return false;
                }

                history.push_back(
                    instruction
                );
            }

            catch (...)
            {
                OutputDebugStringA(
                    "HISTORY ERROR: EXCEPTION\n"
                );

                history.clear();

                m_status =
                    "Invalid transition history data.";

                return false;
            }
        }

        m_status =
            "Transition history loaded: " +
            std::to_string(
                history.size()
            ) +
            " instructions.";

        return true;
    }

    bool MemoryScanner::
        getReadTrackingTransitionNextInstruction(
            size_t transitionIndex,
            RuntimeInstruction& instruction
        )
    {
        std::string response;

        const std::string command =
            "READTRACK:TRANSITIONNEXT:" +
            std::to_string(
                transitionIndex
            );

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            m_status =
                "Could not get transition next instruction.";

            return false;
        }

        if (response.rfind(
            "ERROR",
            0
        ) == 0)
        {
            m_status =
                "Transition next instruction failed: " +
                response;

            return false;
        }

        std::stringstream stream(
            response
        );

        std::string addressText;
        std::string csText;
        std::string ipText;

        if (!std::getline(
            stream,
            addressText,
            ':'
        ) ||
            !std::getline(
                stream,
                csText,
                ':'
            ) ||
            !std::getline(
                stream,
                ipText,
                ':'
            ))
        {
            m_status =
                "Invalid transition next instruction.";

            return false;
        }

        try
        {
            instruction =
                RuntimeInstruction{};

            instruction.address =
                static_cast<size_t>(
                    std::stoull(
                        addressText
                    )
                    );

            instruction.cs =
                static_cast<uint16_t>(
                    std::stoul(
                        csText
                    )
                    );

            instruction.ip =
                static_cast<uint16_t>(
                    std::stoul(
                        ipText
                    )
                    );
        }
        catch (...)
        {
            m_status =
                "Invalid transition next instruction data.";

            return false;
        }

        return true;
    }

    const std::string&
        MemoryScanner::
        lastTransitionContextResponse() const
    {
        return m_lastTransitionContextResponse;
    }

}
