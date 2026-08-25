#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_set>
#include <utility>
#include <array>

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

    struct RegisterSnapshot
    {
        uint16_t ax = 0;
        uint16_t bx = 0;
        uint16_t cx = 0;
        uint16_t dx = 0;

        uint16_t si = 0;
        uint16_t di = 0;
        uint16_t bp = 0;
        uint16_t sp = 0;

        uint16_t ds = 0;
        uint16_t es = 0;
        uint16_t ss = 0;
    };

    struct RuntimeInstruction
    {
        size_t address = 0;

        uint16_t cs = 0;
        uint16_t ip = 0;

        RegisterSnapshot registers;

        std::array<uint8_t, 16> bytes{};

        std::array<uint8_t, 32>
            stackBytes{};
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

        bool setReadTrackingTransitionTarget(
            size_t address
        );

        bool setExecutionCaptureTarget(
            size_t address
        );

        bool getExecutionCaptureTarget(
            size_t& address
        );

        bool clearExecutionCapture();

        bool getExecutionCaptureHit(
            bool& hit
        );

        bool getExecutionCapture(
            RuntimeInstruction& instruction
        );

        bool setReadTraceTarget(
            size_t address
        );

        bool getReadTraceTarget(
            size_t& address
        );


        bool getReadTraceActive(
            bool& active
        );

        bool getReadTraceCount(
            size_t& count
        );

        bool getReadTraceInstruction(size_t index, RuntimeInstruction& instruction);

        bool getReadTraceArmed(
            bool& armed
        );

        bool getReadTrackingCount(
            size_t& count
        );

        bool getReadTrackingTransitionCount(
            size_t& count
        );

        bool getReadTrackingTransitionContextCount(
            size_t& count
        );

        bool getReadTrackingTransitionContextBlock(
            size_t start,
            size_t count,
            std::vector<std::pair<uint16_t, uint16_t>>& contexts
        );

        bool getReadTrackingTransitionContexts(
            std::vector<std::pair<uint16_t, uint16_t>>& contexts
        );

        bool getReadTrackingTransitionBlock(
            size_t start,
            size_t count,
            std::vector<std::pair<size_t, size_t>>& transitions
        );

        bool getReadTrackingTransitions(
            std::vector<std::pair<size_t, size_t>>& transitions
        );

        bool getReadTrackingTransitionByteCount(
            size_t& count
        );

        bool getReadTrackingTransitionByteBlock(
            size_t start,
            size_t count,
            std::vector<std::array<uint8_t, 16>>& bytes
        );

        bool getReadTrackingTransitionBytes(
            std::vector<std::array<uint8_t, 16>>& bytes
        );

        bool getReadTrackingTransitionHistory(
            size_t transitionIndex,
            std::vector<RuntimeInstruction>& history
        );

        bool getReadTrackingTransitionNextInstruction(
            size_t transitionIndex,
            RuntimeInstruction& instruction
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

        const std::string&
            lastTransitionContextResponse() const;

        bool compareMemoryAddress(
            size_t address,
            std::string& result
        );

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

        std::string
            m_lastTransitionContextResponse;
        
};
}