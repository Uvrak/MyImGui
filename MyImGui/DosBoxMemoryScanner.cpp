#include "pch.h"
#include "DosBoxMemoryScanner.h"

#include <algorithm>
#include <utility>
#include <sstream>

namespace
{
    constexpr const char*
        DosBoxMemoryScannerPipeName =
        R"(\\.\pipe\DosBoxMemoryScanner)";

    uint16_t readUInt16(
        const std::vector<uint8_t>& memory,
        size_t address
    )
    {
        return
            static_cast<uint16_t>(
                memory[address]
                ) |
            static_cast<uint16_t>(
                memory[address + 1]
                ) << 8;
    }
}

namespace MyImGui
{
    DosBoxMemoryScanner::
        DosBoxMemoryScanner(
            DosBoxMemoryReader& memoryReader
        )
        : m_pipeClient(
            DosBoxMemoryScannerPipeName
        ),
        m_memoryReader(
            memoryReader
        )
    {}

    bool DosBoxMemoryScanner::scan(
        DosBoxMemoryScanMode mode,
        DosBoxMemoryValueType valueType,
        uint32_t exactValue
    )
    {
        if (!requestSnapshot())
        {
            return false;
        }

        const std::vector<uint8_t>&
            currentMemory =
            m_memoryReader.memory();

        if (currentMemory.empty())
        {
            m_status =
                "Snapshot is empty.";

            return false;
        }
        if (mode ==
            DosBoxMemoryScanMode::NewScan ||
            mode ==
            DosBoxMemoryScanMode::UnknownInitialValue ||
            (
                mode ==
                DosBoxMemoryScanMode::ExactValue &&
                m_previousMemory.empty()
                ))
        {
            initializeCandidates(
                currentMemory,
                valueType,
                mode ==
                DosBoxMemoryScanMode::ExactValue,
                exactValue
            );

            m_previousMemory =
                currentMemory;

            m_status =
                "New scan initialized.";

            return true;
        }

        refineCandidates(
            mode,
            valueType,
            exactValue,
            m_previousMemory,
            currentMemory
        );

        m_previousMemory =
            currentMemory;

        m_status =
            "Scan refined to " +
            std::to_string(
                m_candidates.size()
            ) +
            " candidates.";

        return true;
    }

    bool DosBoxMemoryScanner::scanBytePattern(
        const std::vector<uint8_t>& pattern
    )
    {
        if (pattern.empty())
        {
            m_status =
                "Byte pattern is empty.";

            return false;
        }

        if (!requestSnapshot())
        {
            return false;
        }

        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        if (memory.size() < pattern.size())
        {
            m_status =
                "Byte pattern is larger than memory.";

            return false;
        }

        m_candidates.clear();

        const size_t startAddress =
            m_scanRangeEnabled
            ? (std::min)(
                m_scanStartAddress,
                memory.size()
                )
            : 0;

        const size_t endAddress =
            m_scanRangeEnabled
            ? (std::min)(
                m_scanEndAddress + 1,
                memory.size()
                )
            : memory.size();

        if (endAddress < startAddress ||
            endAddress - startAddress < pattern.size())
        {
            m_status =
                "Scan range is smaller than byte pattern.";

            return false;
        }

        for (size_t address = startAddress;
            address + pattern.size() <= endAddress;
            ++address)
        {
            bool matches = true;

            for (size_t i = 0;
                i < pattern.size();
                ++i)
            {
                if (memory[address + i] !=
                    pattern[i])
                {
                    matches = false;
                    break;
                }
            }

            if (!matches)
            {
                continue;
            }

            m_candidates.push_back(
                DosBoxMemoryCandidate{
                    address,
                    memory[address],
                    memory[address]
                }
            );
        }

        m_status =
            "Byte pattern scan found " +
            std::to_string(
                m_candidates.size()
            ) +
            " matches.";

        return true;
    }

    bool DosBoxMemoryScanner::
        requestSnapshot()
    {
        std::string response;

        if (!m_pipeClient.request(
            "PUBLISH",
            response
        ))
        {
            m_status =
                "Could not request DOSBox snapshot.";

            return false;
        }

        if (response != "PUBLISHED")
        {
            m_status =
                "DOSBox snapshot request failed: " +
                response;

            return false;
        }

        if (!m_memoryReader.readSnapshot())
        {
            m_status =
                "Could not read shared memory snapshot.";

            return false;
        }

        return true;
    }

    void DosBoxMemoryScanner::
        initializeCandidates(
            const std::vector<uint8_t>& memory,
            DosBoxMemoryValueType valueType,
            bool filterExactValue,
            uint32_t exactValue
        )
    {
        m_candidates.clear();

        m_candidates.reserve(
            memory.size()
        );


        const size_t startAddress =
            m_scanRangeEnabled
            ? (std::min)(
                m_scanStartAddress,
                memory.size()
            )
            : 0;

        const size_t endAddress =
            m_scanRangeEnabled
            ? (std::min)(
                m_scanEndAddress + 1,
                memory.size()
            )
            : memory.size();

        for (size_t address = startAddress;
            address < endAddress;
            ++address)
        {
            uint32_t value = 0;

            if (valueType ==
                DosBoxMemoryValueType::Byte)
            {
                value =
                    memory[address];
            }
            else if (valueType ==
                DosBoxMemoryValueType::Short)
            {
                if (address + 1 >= endAddress ||
                    address + 1 >= memory.size())
                {
                    break;
                }

                value =
                    readUInt16(
                        memory,
                        address
                    );
            }
            else
            {
                // Int kommt später.
                continue;
            }

            if (filterExactValue &&
                value != exactValue)
            {
                continue;
            }

            m_candidates.push_back(
                DosBoxMemoryCandidate{
                    address,
                    value,
                    value
                }
            );
        }
    }

    void DosBoxMemoryScanner::
        refineCandidates(
            DosBoxMemoryScanMode mode,
            DosBoxMemoryValueType valueType,
            uint32_t exactValue,
            const std::vector<uint8_t>&
            previousMemory,
            const std::vector<uint8_t>&
            currentMemory
        )
    {
        std::vector<
            DosBoxMemoryCandidate
        > refined;

        refined.reserve(
            m_candidates.size()
        );

        for (const DosBoxMemoryCandidate&
            candidate : m_candidates)
        {
            uint32_t previousValue = 0;
            uint32_t currentValue = 0;

            if (valueType ==
                DosBoxMemoryValueType::Byte)
            {
                if (candidate.address >=
                    previousMemory.size() ||
                    candidate.address >=
                    currentMemory.size())
                {
                    continue;
                }

                previousValue =
                    previousMemory[
                        candidate.address
                    ];

                currentValue =
                    currentMemory[
                        candidate.address
                    ];
            }
            else if (valueType ==
                DosBoxMemoryValueType::Short)
            {
                if (candidate.address + 1 >=
                    previousMemory.size() ||
                    candidate.address + 1 >=
                    currentMemory.size())
                {
                    continue;
                }

                previousValue =
                    readUInt16(
                        previousMemory,
                        candidate.address
                    );

                currentValue =
                    readUInt16(
                        currentMemory,
                        candidate.address
                    );
            }
            else
            {
                // Int kommt später.
                continue;
            }

            bool accepted = false;

            switch (mode)
            {
            case DosBoxMemoryScanMode::Changed:
                accepted =
                    currentValue !=
                    previousValue;
                break;

            case DosBoxMemoryScanMode::ExactValue:
                accepted =
                    currentValue ==
                    exactValue;
                break;

            case DosBoxMemoryScanMode::Unchanged:
                accepted =
                    currentValue ==
                    previousValue;
                break;

            case DosBoxMemoryScanMode::Increased:
                accepted =
                    currentValue >
                    previousValue;
                break;

            case DosBoxMemoryScanMode::Decreased:
                accepted =
                    currentValue <
                    previousValue;
                break;
            
            case DosBoxMemoryScanMode::UnknownInitialValue:
                accepted = true;
                break;

            case DosBoxMemoryScanMode::NewScan:
                accepted = true;
                break;
            }

            const bool pinned =
                m_pinnedAddresses.contains(
                    candidate.address
                );

            if (pinned)
            {
                accepted = true;
            }

            if (accepted)
            {
                refined.push_back(
                    DosBoxMemoryCandidate{
                        candidate.address,
                        previousValue,
                        currentValue
                    }
                );
            }
        }

        m_candidates =
            std::move(
                refined
            );
    }

    void DosBoxMemoryScanner::reset()
    {
        m_previousMemory.clear();
        m_candidates.clear();

        m_status =
            "Scanner reset.";
    }

    bool DosBoxMemoryScanner::readPreviousValue(
        size_t address,
        uint8_t& value
    ) const
    {
        if (address >= m_previousMemory.size())
        {
            return false;
        }

        value =
            m_previousMemory[address];

        return true;
    }

    const std::vector<
        DosBoxMemoryCandidate
    >& DosBoxMemoryScanner::
        candidates() const
    {
        return m_candidates;
    }

    const std::string&
        DosBoxMemoryScanner::status() const
    {
        return m_status;
    }

    void DosBoxMemoryScanner::pinAddress(
        size_t address
    )
    {
        m_pinnedAddresses.insert(
            address
        );
    }

    void DosBoxMemoryScanner::unpinAddress(
        size_t address
    )
    {
        m_pinnedAddresses.erase(
            address
        );
    }

    void DosBoxMemoryScanner::clearPinnedAddresses()
    {
        m_pinnedAddresses.clear();
    }

    const std::unordered_set<size_t>&
        DosBoxMemoryScanner::pinnedAddresses() const
    {
        return m_pinnedAddresses;
    }
    bool DosBoxMemoryScanner::readCurrentValue(
        size_t address,
        uint8_t& value
    ) const
    {
        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        if (address >= memory.size())
        {
            return false;
        }

        value = memory[address];

        return true;
    }

    bool DosBoxMemoryScanner::refreshMemory()
    {
        if (!requestSnapshot())
        {
            return false;
        }

        m_status =
            "Memory refreshed.";

        return true;
    }

    bool DosBoxMemoryScanner::writeValue(
        size_t address,
        uint8_t value
    )
    {
        std::string response;

        const std::string command =
            "WRITE:" +
            std::to_string(address) +
            ":" +
            std::to_string(
                static_cast<unsigned int>(
                    value
                    )
            );

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            m_status =
                "Could not write memory.";

            return false;
        }

        if (response != "OK")
        {
            m_status =
                "Memory write failed: " +
                response;

            return false;
        }

        m_status =
            "Memory value written.";

        return true;
    }
    
    bool DosBoxMemoryScanner::startReadTracking()
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

    bool DosBoxMemoryScanner::stopReadTracking()
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

    bool DosBoxMemoryScanner::clearReadTracking()
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

    bool DosBoxMemoryScanner::
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

    bool DosBoxMemoryScanner::getReadTrackingCount(
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
                "Invalid read tracking count: " +
                response;

            return false;
        }

        m_status =
            "Read tracking count: " +
            std::to_string(count);

        return true;
    }

    bool DosBoxMemoryScanner::
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

    bool DosBoxMemoryScanner::
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

    bool DosBoxMemoryScanner::
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

    bool DosBoxMemoryScanner::
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

    bool DosBoxMemoryScanner::
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

    bool DosBoxMemoryScanner::
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

    bool DosBoxMemoryScanner::getReadTrackingAddress(
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

    bool DosBoxMemoryScanner::getReadTrackingAddressBlock(
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

    bool DosBoxMemoryScanner::getReadTrackingAddresses(
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

    bool DosBoxMemoryScanner::
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

    bool DosBoxMemoryScanner::
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

    bool DosBoxMemoryScanner::
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
    
    void DosBoxMemoryScanner::setCandidatesFromAddresses(
        const std::vector<size_t>& addresses
    )
    {
        if (!requestSnapshot())
        {
            return;
        }

        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        m_previousMemory =
            memory;

        m_candidates.clear();

        m_candidates.reserve(
            addresses.size()
        );

        for (const size_t address :
        addresses)
        {
            if (address >= memory.size())
            {
                continue;
            }

            const uint8_t value =
                memory[address];

            m_candidates.push_back(
                DosBoxMemoryCandidate{
                    address,
                    value,
                    value
                }
            );
        }

        m_status =
            "Read tracking candidates: " +
            std::to_string(
                m_candidates.size()
            );
    }

    void DosBoxMemoryScanner::refreshValues()
    {
        for (DosBoxMemoryCandidate& candidate :
            m_candidates)
        {
            candidate.previousValue =
                candidate.currentValue;
        }

        if (!requestSnapshot())
        {
            return;
        }

        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        for (DosBoxMemoryCandidate& candidate :
            m_candidates)
        {
            if (candidate.address <
                memory.size())
            {
                candidate.currentValue =
                    memory[
                        candidate.address
                    ];
            }
        }
    }

    void DosBoxMemoryScanner::setScanRange(
        size_t startAddress,
        size_t endAddress
    )
    {
        m_scanStartAddress =
            startAddress;

        m_scanEndAddress =
            endAddress;

        m_scanRangeEnabled =
            true;
    }

    void DosBoxMemoryScanner::clearScanRange()
    {
        m_scanRangeEnabled =
            false;
    }

    const std::string&
        DosBoxMemoryScanner::
        lastTransitionContextResponse() const
    {
        return m_lastTransitionContextResponse;
    }

}