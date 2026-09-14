#include "MemoryScanner.h"
#include "MemoryPatternSearch.h"

#include <algorithm>
#include <utility>
#include <sstream>
#include <cstdlib>
#include <windows.h>

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

namespace DosBoxMemoryTools
{
    MemoryScanner::
        MemoryScanner(
          MemoryReader& memoryReader
        )
        : m_pipeClient(
            DosBoxMemoryScannerPipeName
        ),
        m_memoryReader(
            memoryReader
        )
    {}

    bool MemoryScanner::scan(
        MemoryScanMode mode,
        MemoryValueType valueType,
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
            MemoryScanMode::NewScan ||
            mode ==
            MemoryScanMode::UnknownInitialValue ||
            (
                mode ==
                MemoryScanMode::ExactValue &&
                m_previousMemory.empty()
                ))
        {
            initializeCandidates(
                currentMemory,
                valueType,
                mode ==
                MemoryScanMode::ExactValue,
                exactValue
            );

            m_previousMemory =
                currentMemory;

            m_status =
                "size=" +
                std::to_string(currentMemory.size()) +
                " enabled=" +
                std::to_string(m_scanRangeEnabled ? 1 : 0) +
                " start=" +
                std::to_string(m_scanStartAddress) +
                " end=" +
                std::to_string(m_scanEndAddress) +
                " candidates=" +
                std::to_string(m_candidates.size());

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

    bool MemoryScanner::scanBytePattern(
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

        const auto matches =
            MemoryPatternSearch::find(
                memory.data() + startAddress,
                endAddress - startAddress,
                pattern
            );

        m_candidates.clear();

        for (const size_t offset : matches)
        {
            const size_t address =
                startAddress + offset;

            m_candidates.push_back(
                MemoryCandidate{
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

    void MemoryScanner::keepDifference(
        int difference
    )
    {
        m_candidates.erase(
            std::remove_if(
                m_candidates.begin(),
                m_candidates.end(),
                [difference](
                    const MemoryCandidate& candidate
                    )
                {
                    const int candidateDifference =
                        std::abs(
                            static_cast<int>(
                                candidate.currentValue
                                ) -
                            static_cast<int>(
                                candidate.previousValue
                                )
                        );

                    return candidateDifference !=
                        difference;
                }
            ),
            m_candidates.end()
        );

        m_status =
            "Candidates kept by difference: " +
            std::to_string(
                m_candidates.size()
            );
    }

    bool MemoryScanner::
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

    void MemoryScanner::
        initializeCandidates(
            const std::vector<uint8_t>& memory,
            MemoryValueType valueType,
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

        m_status =
            "size=" +
            std::to_string(memory.size()) +
            " start=" +
            std::to_string(startAddress) +
            " end=" +
            std::to_string(endAddress);

        for (size_t address = startAddress;
            address < endAddress;
            ++address)
        {
            uint32_t value = 0;

            if (valueType ==
                MemoryValueType::Byte)
            {
                value =
                    memory[address];
            }
            else if (valueType ==
                MemoryValueType::Short)
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
                MemoryCandidate{
                    address,
                    value,
                    value
                }
            );
        }
    }

    void MemoryScanner::
        refineCandidates(
            MemoryScanMode mode,
            MemoryValueType valueType,
            uint32_t exactValue,
            const std::vector<uint8_t>&
            previousMemory,
            const std::vector<uint8_t>&
            currentMemory
        )
    {
        std::vector<
            MemoryCandidate
        > refined;

        refined.reserve(
            m_candidates.size()
        );

        for (const MemoryCandidate&
            candidate : m_candidates)
        {
            uint32_t previousValue = 0;
            uint32_t currentValue = 0;

            if (valueType ==
                MemoryValueType::Byte)
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
                MemoryValueType::Short)
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
            case MemoryScanMode::Changed:
                accepted =
                    currentValue !=
                    previousValue;
                break;

            case MemoryScanMode::ExactValue:
                accepted =
                    currentValue ==
                    exactValue;
                break;

            case MemoryScanMode::Unchanged:
                accepted =
                    currentValue ==
                    previousValue;
                break;

            case MemoryScanMode::Increased:
                accepted =
                    currentValue >
                    previousValue;
                break;

            case MemoryScanMode::Decreased:
                accepted =
                    currentValue <
                    previousValue;
                break;
            
            case MemoryScanMode::UnknownInitialValue:
                accepted = true;
                break;

            case MemoryScanMode::NewScan:
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
                    MemoryCandidate{
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

    void MemoryScanner  ::reset()
    {
        m_previousMemory.clear();
        m_candidates.clear();

        m_status =
            "Scanner reset.";
    }

    bool MemoryScanner::readPreviousValue(
        size_t address,
        MemoryValueType valueType,
        uint32_t& value
    ) const
    {
        switch (valueType)
        {
        case MemoryValueType::Byte:
            if (address >= m_previousMemory.size())
            {
                return false;
            }

            value =
                m_previousMemory[address];
            return true;

        case MemoryValueType::Short:
            if (address + 1 >= m_previousMemory.size())
            {
                return false;
            }

            value =
                static_cast<uint32_t>(
                    m_previousMemory[address]
                    ) |
                (
                    static_cast<uint32_t>(
                        m_previousMemory[address + 1]
                        ) << 8
                    );

            return true;

        case MemoryValueType::Int:
            if (address + 3 >= m_previousMemory.size())
            {
                return false;
            }

            value =
                static_cast<uint32_t>(
                    m_previousMemory[address]
                    ) |
                (
                    static_cast<uint32_t>(
                        m_previousMemory[address + 1]
                        ) << 8
                    ) |
                (
                    static_cast<uint32_t>(
                        m_previousMemory[address + 2]
                        ) << 16
                    ) |
                (
                    static_cast<uint32_t>(
                        m_previousMemory[address + 3]
                        ) << 24
                    );

            return true;
        }

        return false;
    }
    const std::vector<
        MemoryCandidate
    >& MemoryScanner::
        candidates() const
    {
        return m_candidates;
    }

    const std::string&
        MemoryScanner::status() const
    {
        return m_status;
    }

    void MemoryScanner::pinAddress(
        size_t address
    )
    {
        m_pinnedAddresses.insert(
            address
        );
    }

    void MemoryScanner::unpinAddress(
        size_t address
    )
    {
        m_pinnedAddresses.erase(
            address
        );
    }

    void MemoryScanner::clearPinnedAddresses()
    {
        m_pinnedAddresses.clear();
    }

    const std::unordered_set<size_t>&
        MemoryScanner::pinnedAddresses() const
    {
        return m_pinnedAddresses;
    }
    bool MemoryScanner::readCurrentValue(
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

    bool MemoryScanner::readCurrentValue(
        size_t address,
        MemoryValueType valueType,
        uint32_t& value
    ) const
    {
        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        switch (valueType)
        {
        case MemoryValueType::Byte:
            if (address >= memory.size())
            {
                return false;
            }

            value = memory[address];
            return true;

        case MemoryValueType::Short:
            if (address + 1 >= memory.size())
            {
                return false;
            }

            value =
                static_cast<uint32_t>(memory[address]) |
                (static_cast<uint32_t>(memory[address + 1]) << 8);

            return true;

        case MemoryValueType::Int:
            if (address + 3 >= memory.size())
            {
                return false;
            }

            value =
                static_cast<uint32_t>(memory[address]) |
                (static_cast<uint32_t>(memory[address + 1]) << 8) |
                (static_cast<uint32_t>(memory[address + 2]) << 16) |
                (static_cast<uint32_t>(memory[address + 3]) << 24);

            return true;
        }

        return false;
    }

    bool MemoryScanner::refreshMemory()
    {
        if (!requestSnapshot())
        {
            return false;
        }

        m_status =
            "Memory refreshed.";

        return true;
    }
    bool MemoryScanner::writeValue(
    size_t address,
    uint32_t value,
    MemoryValueType valueType
)
{
    size_t size = 1;

    switch (valueType)
    {
    case MemoryValueType::Byte:
        size = 1;
        break;

    case MemoryValueType::Short:
        size = 2;
        break;

    case MemoryValueType::Int:
        size = 4;
        break;
    }

    std::string response;

    const std::string command =
        "WRITE:" +
        std::to_string(address) +
        ":" +
        std::to_string(value) +
        ":" +
        std::to_string(size);

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

    m_lastMemoryWriteTarget =
        address;

    m_status =
        "Memory value written.";

    return true;
}
    
    void MemoryScanner::setCandidatesFromAddresses(
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
                MemoryCandidate{
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

    void MemoryScanner::refreshValues(
        MemoryValueType valueType
    )
    {
        for (MemoryCandidate& candidate :
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

        for (MemoryCandidate& candidate :
            m_candidates)
        {
            switch (valueType)
            {
            case MemoryValueType::Byte:
                if (candidate.address >= memory.size())
                {
                    continue;
                }

                candidate.currentValue =
                    memory[candidate.address];
                break;

            case MemoryValueType::Short:
                if (candidate.address + 1 >= memory.size())
                {
                    continue;
                }

                candidate.currentValue =
                    static_cast<uint32_t>(
                        memory[candidate.address]
                        ) |
                    (static_cast<uint32_t>(
                        memory[candidate.address + 1]
                        ) << 8);
                break;

            case MemoryValueType::Int:
                if (candidate.address + 3 >= memory.size())
                {
                    continue;
                }

                candidate.currentValue =
                    static_cast<uint32_t>(
                        memory[candidate.address]
                        ) |
                    (static_cast<uint32_t>(
                        memory[candidate.address + 1]
                        ) << 8) |
                    (static_cast<uint32_t>(
                        memory[candidate.address + 2]
                        ) << 16) |
                    (static_cast<uint32_t>(
                        memory[candidate.address + 3]
                        ) << 24);
                break;
            }
        }
    }

    void MemoryScanner::setScanRange(
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

    void MemoryScanner::clearScanRange()
    {
        m_scanRangeEnabled =
            false;
    }

    bool MemoryScanner::compareMemoryAddress(
        size_t address,
        std::string& result
    )
    {
        const std::string command =
            "MEMCOMPARE:" +
            std::to_string(address);

        std::string response;

        if (!m_pipeClient.request(
            command,
            response
        ))
        {
            m_status =
                "MEMCOMPARE request failed.";

            return false;
        }

        if (response.rfind(
            "ERROR",
            0
        ) == 0)
        {
            m_status =
                "MEMCOMPARE failed: " +
                response;

            return false;
        }

        result =
            response;

        m_status =
            response;

        return true;
    }

}
