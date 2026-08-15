#include "pch.h"
#include "DosBoxMemoryScanner.h"

#include <algorithm>
#include <utility>

namespace
{
    constexpr const char*
        DosBoxMemoryScannerPipeName =
        R"(\\.\pipe\DosBoxMemoryScanner)";
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
        uint8_t exactValue
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
            bool filterExactValue,
            uint8_t exactValue
        )
    {
        m_candidates.clear();

        m_candidates.reserve(
            memory.size()
        );


        for (size_t address = 0;
            address < memory.size();
            ++address)
        {
            if (filterExactValue &&
                memory[address] != exactValue)
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
    }

    void DosBoxMemoryScanner::
        refineCandidates(
            DosBoxMemoryScanMode mode,
            uint8_t exactValue,
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
            if (candidate.address >=
                previousMemory.size() ||
                candidate.address >=
                currentMemory.size())
            {
                continue;
            }

            const uint8_t previousValue =
                previousMemory[
                    candidate.address
                ];

            const uint8_t currentValue =
                currentMemory[
                    candidate.address
                ];

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
}