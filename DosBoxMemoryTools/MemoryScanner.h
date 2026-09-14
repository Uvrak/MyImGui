#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_set>
#include <utility>
#include <array>

#include "MemoryScannerTypes.h"
#include "MemoryReader.h"
#include "NamedPipeClient.h"

namespace DosBoxMemoryTools
{
    class MemoryScanner
    {
    public:
        MemoryScanner(
            MemoryReader& memoryReader
        );

        bool scan(
            MemoryScanMode mode,
            MemoryValueType valueType,
            uint32_t exactValue = 0
        );

        bool scanBytePattern(
            const std::vector<uint8_t>& pattern
        );

        void keepDifference(
            int difference
        );

        void reset();

        bool readPreviousValue(
            size_t address,
            MemoryValueType valueType,
            uint32_t& value
        ) const;

        bool setReadTraceInstructionLimit(
            size_t count
        );

        const std::vector<
            MemoryCandidate
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

        bool readCurrentValue(
            size_t address,
            MemoryValueType valueType,
            uint32_t& value
        ) const;

        bool refreshMemory();

        bool writeValue(
            size_t address,
            uint32_t value,
            MemoryValueType valueType
        );

        bool startReadTracking();
        bool stopReadTracking();
        bool clearReadTracking();

        bool startTransitionTracking();
        bool stopTransitionTracking();
        bool clearTransitionTracking();

        bool setReadTrackingTransitionTarget(
            size_t address
        );

        void setMemoryWriteMarker(
            size_t index
        );

        bool getMemoryWriteMarker(
            size_t& index
        ) const;

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

        void refreshValues(
            MemoryValueType valueType
        );

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

        bool setMemoryWriteWatchTarget(
            size_t address
        );

        bool setMemoryWriteWatchRange(size_t startAddress, size_t endAddress);

        bool clearMemoryWriteWatch();

        bool getMemoryWriteWatchHit(
            bool& hit
        );

        bool getMemoryWriteWatchCaptureCount(
            size_t& count
        );

        bool getMemoryWriteWatchCapture(
            size_t index,
            RuntimeInstruction& instruction
        );

        size_t lastMemoryWriteTarget() const
        {
            return m_lastMemoryWriteTarget;
        }

        size_t lastMemoryWriteInstruction() const
        {
            return m_lastMemoryWriteInstruction;
        }

    private:
        bool requestSnapshot();

        void initializeCandidates(
            const std::vector<uint8_t>& memory,
            MemoryValueType valueType,
            bool filterExactValue,
            uint32_t exactValue
        );

        void refineCandidates(
            MemoryScanMode mode,
            MemoryValueType valueType,
            uint32_t exactValue,
            const std::vector<uint8_t>& previousMemory,
            const std::vector<uint8_t>& currentMemory
        );

        DosBoxX::NamedPipeClient m_pipeClient;

        MemoryReader&
            m_memoryReader;

        std::vector<uint8_t>
            m_previousMemory;

        std::vector<
            MemoryCandidate
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
        
        size_t m_memoryWriteMarker = 0;

        bool m_memoryWriteMarkerValid = false;

        size_t m_lastMemoryWriteTarget = 0;
        size_t m_lastMemoryWriteInstruction = 0;
};
}