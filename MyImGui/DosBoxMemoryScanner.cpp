#include "pch.h"
#include "DosBoxMemoryScanner.h"

#include <algorithm>
#include <utility>

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
    void DosBoxMemoryScanner::refreshValues()
    {
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
}