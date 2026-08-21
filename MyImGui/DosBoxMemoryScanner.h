#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_set>
#include <utility>

#include "DosBoxMemoryReader.h"
#include "NamedPipeClient.h"

namespace MyImGui
{
    enum class DosBoxMemoryScanMode
    {
        NewScan,
        UnknownInitialValue,
        ExactValue,
        Changed,
        Unchanged,
        Increased,
        Decreased
    };

    enum class DosBoxMemoryValueType
    {
        Byte,
        Short,
        Int
    };

    struct DosBoxMemoryCandidate
    {
        size_t address = 0;

        uint32_t previousValue = 0;
        uint32_t currentValue = 0;
    };

    class DosBoxMemoryScanner
    {
    public:
        DosBoxMemoryScanner(
            DosBoxMemoryReader& memoryReader
        );

        bool scan(
            DosBoxMemoryScanMode mode,
            DosBoxMemoryValueType valueType,
            uint32_t exactValue = 0
        );

        bool scanBytePattern(
            const std::vector<uint8_t>& pattern
        );

        void reset();

        bool readPreviousValue(
            size_t address,
            uint8_t& value
        ) const;

        const std::vector<
            DosBoxMemoryCandidate
        >& candidates() const;

        const std::string&
            status() const;

        void pinAddress(
            size_t address
        );

        void unpinAddress(
            size_t address
        );

        void clearPinnedAddresses();

        const std::unordered_set<size_t>&
            pinnedAddresses() const;

        bool readCurrentValue(
            size_t address,
            uint8_t& value
        ) const;

        bool refreshMemory();

        bool writeValue(
            size_t address,
            uint8_t value
        );

        bool startReadTracking();
        bool stopReadTracking();
        bool clearReadTracking();

        bool getReadTrackingCount(
            size_t& count
        );

        bool getReadTrackingAddress(
            size_t index,
            size_t& address
        );

        bool getReadTrackingAddressBlock(
            size_t start,
            size_t count,
            std::vector<size_t>& addresses
        );
        bool getReadTrackingAddresses(
            std::vector<size_t>& addresses
        );

        bool getReadTrackingInstructionCount(
            size_t& count
        );

        bool getReadTrackingInstructionBlock(
            size_t start,
            size_t count,
            std::vector<std::pair<size_t, size_t>>& instructions
        );

        bool getReadTrackingInstructions(
            std::vector<std::pair<size_t, size_t>>& instructions
        );

        void setCandidatesFromAddresses(
            const std::vector<size_t>& addresses
        );

        void refreshValues();

        void setScanRange(
            size_t startAddress,
            size_t endAddress
        );

        void clearScanRange();



    private:
        bool requestSnapshot();

        void initializeCandidates(
            const std::vector<uint8_t>& memory,
            DosBoxMemoryValueType valueType,
            bool filterExactValue,
            uint32_t exactValue
        );

        void refineCandidates(
            DosBoxMemoryScanMode mode,
            DosBoxMemoryValueType valueType,
            uint32_t exactValue,
            const std::vector<uint8_t>& previousMemory,
            const std::vector<uint8_t>& currentMemory
        );

        NamedPipeClient m_pipeClient;

        DosBoxMemoryReader&
            m_memoryReader;

        std::vector<uint8_t>
            m_previousMemory;

        std::vector<
            DosBoxMemoryCandidate
        > m_candidates;

        std::string m_status;

        int m_exactValue = 0;


        bool m_filterPrevious = false;

        std::unordered_set<size_t>
            m_pinnedAddresses;

        bool m_scanRangeEnabled = false;

        size_t m_scanStartAddress = 0;
        size_t m_scanEndAddress = 0;
    };
}