#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "DosBoxMemoryReader.h"
#include "NamedPipeClient.h"

namespace MyImGui
{
    enum class DosBoxMemoryScanMode
    {
        NewScan,
        Changed,
        Unchanged,
        Increased,
        Decreased
    };

    struct DosBoxMemoryCandidate
    {
        size_t address = 0;

        uint8_t previousValue = 0;
        uint8_t currentValue = 0;
    };

    class DosBoxMemoryScanner
    {
    public:
        DosBoxMemoryScanner();

        bool scan(
            DosBoxMemoryScanMode mode
        );

        void reset();

        const std::vector<
            DosBoxMemoryCandidate
        >& candidates() const;

        const std::string&
            status() const;

    private:
        bool requestSnapshot();

        void initializeCandidates(
            const std::vector<uint8_t>&
            memory
        );

        void refineCandidates(
            DosBoxMemoryScanMode mode,
            const std::vector<uint8_t>&
            previousMemory,
            const std::vector<uint8_t>&
            currentMemory
        );

        NamedPipeClient m_pipeClient;

        DosBoxMemoryReader
            m_memoryReader;

        std::vector<uint8_t>
            m_previousMemory;

        std::vector<
            DosBoxMemoryCandidate
        > m_candidates;

        std::string m_status;
    };
}