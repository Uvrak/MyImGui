#pragma once

#include <vector>
#include <unordered_set>

#include "MemoryScannerTypes.h"
#include "ScannerAddress.h"

namespace DosBoxMemoryTools
{
    class MemoryWriteComparison
    {
    public:
        void draw(
            ScannerAddress& scannerAddress
        );

        void setA(
            const std::vector<RuntimeInstruction>& captures
        );

        void setB(
            const std::vector<RuntimeInstruction>& captures
        );

        const std::vector<RuntimeInstruction>& a() const
        {
            return m_a;
        }

        const std::vector<RuntimeInstruction>& b() const
        {
            return m_b;
        }

    private:
        bool isStackWrite(
            const RuntimeInstruction& capture
        ) const;

        bool different(
            size_t index

        ) const;

        bool sameWrite(
            const RuntimeInstruction& a,
            const RuntimeInstruction& b
        ) const;

        size_t findMatchingIndex(
            const std::vector<RuntimeInstruction>& captures,
            const RuntimeInstruction& target,
            size_t startIndex
        ) const;

        size_t countUnique(
            const std::vector<RuntimeInstruction>& source,
            const std::vector<RuntimeInstruction>& other
        ) const;

        size_t countOccurrences(
            const std::vector<RuntimeInstruction>& captures,
            const RuntimeInstruction& target
        ) const;

        size_t countInstructionOccurrences(
            const std::vector<RuntimeInstruction>& captures,
            size_t instructionAddress
        ) const;

        std::vector<RuntimeInstruction>
            m_a;

        std::vector<RuntimeInstruction>
            m_b;

        bool m_showUniqueOnly =
            false;

        bool m_hideStackWrites =
            false;
    };
}