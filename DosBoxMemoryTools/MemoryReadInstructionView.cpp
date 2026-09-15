#include "MemoryReadInstructionView.h"
#include "MemoryReadTrackerWindow.h"

#include <cstdlib>

#include "imgui.h"

namespace DosBoxMemoryTools
{
    void MemoryReadInstructionView::draw(
        MemoryReadTrackerWindow& window
    )
    {
        ImGui::Separator();

        ImGui::Checkbox(
            "Limit Address Range",
            &window.m_limitAddressRange
        );

        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            110.0f
        );

        ImGui::TextUnformatted("From");
        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            110.0f
        );

        ImGui::InputText(
            "##RangeFrom",
            window.m_rangeStartText,
            sizeof(window.m_rangeStartText)
        );

        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            110.0f
        );

        ImGui::TextUnformatted("To");
        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            110.0f
        );

        ImGui::InputText(
            "##RangeTo",
            window.m_rangeEndText,
            sizeof(window.m_rangeEndText)
        );

        ImGui::SameLine();

        ImGui::Checkbox(
            "Limit Instruction Range",
            &window.m_limitInstructionRange
        );

        ImGui::SameLine();

        ImGui::TextUnformatted("From");
        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            110.0f
        );

        ImGui::InputText(
            "##InstructionRangeFrom",
            window.m_instructionRangeStartText,
            sizeof(window.m_instructionRangeStartText)
        );

        ImGui::SameLine();

        ImGui::TextUnformatted("To");
        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            110.0f
        );

        ImGui::InputText(
            "##InstructionRangeTo",
            window.m_instructionRangeEndText,
            sizeof(window.m_instructionRangeEndText)
        );
        size_t rangeStart = 0;
        size_t rangeEnd =
            static_cast<size_t>(-1);

        bool validRange =
            !window.m_limitAddressRange;

        if (window.m_limitAddressRange)
        {
            char* startEnd = nullptr;
            char* endEnd = nullptr;

            const unsigned long long parsedStart =
                std::strtoull(
                    window.m_rangeStartText,
                    &startEnd,
                    0
                );

            const unsigned long long parsedEnd =
                std::strtoull(
                    window.m_rangeEndText,
                    &endEnd,
                    0
                );

            if (startEnd != window.m_rangeStartText &&
                *startEnd == '\0' &&
                endEnd != window.m_rangeEndText &&
                *endEnd == '\0' &&
                parsedStart <= parsedEnd)
            {
                rangeStart =
                    static_cast<size_t>(
                        parsedStart
                        );

                rangeEnd =
                    static_cast<size_t>(
                        parsedEnd
                        );

                validRange = true;
            }
        }

        size_t instructionRangeStart = 0;
        size_t instructionRangeEnd =
            static_cast<size_t>(-1);

        bool validInstructionRange =
            !window.m_limitInstructionRange;

        if (window.m_limitInstructionRange)
        {
            char* startEnd = nullptr;
            char* endEnd = nullptr;

            const unsigned long long parsedStart =
                std::strtoull(
                    window.m_instructionRangeStartText,
                    &startEnd,
                    0
                );

            const unsigned long long parsedEnd =
                std::strtoull(
                    window.m_instructionRangeEndText,
                    &endEnd,
                    0
                );

            if (startEnd != window.m_instructionRangeStartText &&
                *startEnd == '\0' &&
                endEnd != window.m_instructionRangeEndText &&
                *endEnd == '\0' &&
                parsedStart <= parsedEnd)
            {
                instructionRangeStart =
                    static_cast<size_t>(
                        parsedStart
                        );

                instructionRangeEnd =
                    static_cast<size_t>(
                        parsedEnd
                        );

                validInstructionRange = true;
            }
        }

        size_t visibleInstructionReads = 0;

        for (const auto& entry :
            window.m_attackReadInstructions)
        {
            const size_t memoryAddress =
                entry.first;

            const size_t instructionAddress =
                entry.second;

            if (window.m_limitAddressRange &&
                validRange &&
                (memoryAddress < rangeStart ||
                    memoryAddress > rangeEnd))
            {
                continue;
            }

            if (window.m_limitInstructionRange &&
                validInstructionRange &&
                (instructionAddress < instructionRangeStart ||
                    instructionAddress > instructionRangeEnd))
            {
                continue;
            }

            ++visibleInstructionReads;
        }

        ImGui::Text(
            "Instruction reads: %zu / %zu",
            visibleInstructionReads,
            window.m_attackReadInstructions.size()
        );

        for (const auto& entry :
            window.m_attackReadInstructions)
        {
            const size_t memoryAddress =
                entry.first;

            const size_t instructionAddress =
                entry.second;

            if (window.m_limitAddressRange &&
                validRange &&
                (memoryAddress < rangeStart ||
                    memoryAddress > rangeEnd))
            {
                continue;
            }

            if (window.m_limitInstructionRange &&
                validInstructionRange &&
                (instructionAddress < instructionRangeStart ||
                    instructionAddress > instructionRangeEnd))
            {
                continue;
            }

            ImGui::Text(
                "0x%zX -> 0x%zX",
                memoryAddress,
                instructionAddress
            );
        }
    }
}
