#include "MemoryScanner.h"

#include <sstream>
#include <cstdlib>
#include <utility>

namespace DosBoxMemoryTools
{
    bool MemoryScanner::startReadTracking()
    {
        std::string response;

        if (!m_pipeClient.request(
            "READTRACK:START",
            response
        ))
        {
            m_status =
                "Could not start read tracking.";

            return false;
        }

        if (response != "OK")
        {
            m_status =
                "Read tracking start failed: " +
                response;

            return false;
        }

        m_status =
            "Read tracking started.";

        return true;
    }

    bool MemoryScanner::stopReadTracking()
    {
        std::string response;

        if (!m_pipeClient.request(
            "READTRACK:STOP",
            response
        ))
        {
            m_status =
                "Could not stop read tracking.";

            return false;
        }

        if (response != "OK")
        {
            m_status =
                "Read tracking stop failed: " +
                response;

            return false;
        }

        m_status =
            "Read tracking stopped.";

        return true;
    }

    bool MemoryScanner::clearReadTracking()
    {
        std::string response;

        if (!m_pipeClient.request(
            "READTRACK:CLEAR",
            response
        ))
        {
            m_status =
                "Could not clear read tracking.";

            return false;
        }

        if (response != "OK")
        {
            m_status =
                "Read tracking clear failed: " +
                response;

            return false;
        }

        m_status =
            "Read tracking cleared.";

        return true;
    }

    bool MemoryScanner::getReadTrackingCount(
        size_t& count
    )
    {
        std::string response;

        if (!m_pipeClient.request(
            "READTRACK:COUNT",
            response
        ))
        {
            m_status =
                "Could not get read tracking count.";

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
                "Invalid read tracking address: " +
                response;

            return false;
        }

        m_status =
            "Read tracking count: " +
            std::to_string(count);

        return true;
    }

    bool MemoryScanner::getReadTrackingAddress(
        size_t index,
        size_t& address
    )
    {
        std::string response;

        const std::string command =
            "READTRACK:ADDRESS:" +
            std::to_string(index);

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            m_status =
                "Could not get read tracking address.";

            return false;
        }

        try
        {
            address =
                static_cast<size_t>(
                    std::stoull(response)
                    );
        }
        catch (...)
        {
            m_status =
                "Invalid read tracking address: " +
                response;

            return false;
        }

        return true;
    }

    bool MemoryScanner::getReadTrackingAddressBlock(
        size_t start,
        size_t count,
        std::vector<size_t>& addresses
    )
    {
        std::string response;

        const std::string command =
            "READTRACK:ADDRESSES:" +
            std::to_string(start) +
            ":" +
            std::to_string(count);

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            m_status =
                "Could not get read tracking address block.";

            return false;
        }

        m_status =
            "Context block response: " +
            response;

        if (response.rfind(
            "ERROR",
            0
        ) == 0)
        {
            m_status =
                "Read tracking address block failed: " +
                response;

            return false;
        }

        addresses.clear();

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

            try
            {
                addresses.push_back(
                    static_cast<size_t>(
                        std::stoull(item)
                        )
                );
            }
            catch (...)
            {
                addresses.clear();

                m_status =
                    "Invalid read tracking address block.";

                return false;
            }
        }

        return true;
    }

    bool MemoryScanner::getReadTrackingAddresses(
        std::vector<size_t>& addresses
    )
    {
        size_t count = 0;

        if (!getReadTrackingCount(
            count
        ))
        {
            return false;
        }

        addresses.clear();
        addresses.reserve(
            count
        );

        constexpr size_t blockSize = 64;

        for (size_t start = 0;
            start < count;
            start += blockSize)
        {
            std::vector<size_t> block;

            if (!getReadTrackingAddressBlock(
                start,
                blockSize,
                block
            ))
            {
                addresses.clear();

                return false;
            }

            addresses.insert(
                addresses.end(),
                block.begin(),
                block.end()
            );
        }

        m_status =
            "Read tracking addresses loaded: " +
            std::to_string(
                addresses.size()
            );

        return true;
    }

    bool MemoryScanner::
        getReadTrackingInstructionCount(
            size_t& count
        )
    {
        std::string response;

        if (!m_pipeClient.request(
            "READTRACK:INSTRUCTIONCOUNT",
            response
        ))
        {
            m_status =
                "Could not get read tracking instruction count.";

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
                "Invalid read tracking instruction count: " +
                response;

            return false;
        }

        return true;
    }

    bool MemoryScanner::
        getReadTrackingInstructionBlock(
            size_t start,
            size_t count,
            std::vector<std::pair<size_t, size_t>>&
            instructions
        )
    {
        std::string response;

        const std::string command =
            "READTRACK:INSTRUCTIONS:" +
            std::to_string(start) +
            ":" +
            std::to_string(count);

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            m_status =
                "Could not get read tracking instruction block.";

            return false;
        }

        if (response.rfind(
            "ERROR",
            0
        ) == 0)
        {
            m_status =
                "Read tracking instruction block failed: " +
                response;

            return false;
        }

        instructions.clear();

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
                instructions.clear();

                m_status =
                    "Invalid read tracking instruction pair.";

                return false;
            }

            try
            {
                const size_t memoryAddress =
                    static_cast<size_t>(
                        std::stoull(
                            item.substr(
                                0,
                                separator
                            )
                        )
                        );

                const size_t instructionAddress =
                    static_cast<size_t>(
                        std::stoull(
                            item.substr(
                                separator + 1
                            )
                        )
                        );

                instructions.emplace_back(
                    memoryAddress,
                    instructionAddress
                );
            }
            catch (...)
            {
                instructions.clear();

                m_status =
                    "Invalid read tracking instruction block.";

                return false;
            }
        }

        return true;
    }

    bool MemoryScanner::
        getReadTrackingInstructions(
            std::vector<std::pair<size_t, size_t>>&
            instructions
        )
    {
        size_t count = 0;

        if (!getReadTrackingInstructionCount(
            count
        ))
        {
            return false;
        }

        instructions.clear();
        instructions.reserve(
            count
        );

        constexpr size_t blockSize = 64;

        for (size_t start = 0;
            start < count;
            start += blockSize)
        {
            std::vector<std::pair<size_t, size_t>>
                block;

            if (!getReadTrackingInstructionBlock(
                start,
                blockSize,
                block
            ))
            {
                instructions.clear();

                return false;
            }

            instructions.insert(
                instructions.end(),
                block.begin(),
                block.end()
            );
        }

        m_status =
            "Read tracking instructions loaded: " +
            std::to_string(
                instructions.size()
            );

        return true;
    }
    
}
