#include "pch.h"
#include "MemoryWriteComparison.h"

#include <cstdio>

#include "imgui.h"

namespace DosBoxMemoryTools
{
    void MemoryWriteComparison::draw(
        ScannerAddress& scannerAddress
    )
    {
        ImGui::PushID("MemoryWriteComparison");
        ImGui::Checkbox(
            "Unique only",
            &m_showUniqueOnly
        );

        ImGui::SameLine();

        ImGui::Checkbox(
            "Hide stack writes",
            &m_hideStackWrites
        );

        std::unordered_set<size_t>
            shownInstructionsA;

        std::unordered_set<size_t>
            shownInstructionsB;

        // HIER EINFÜGEN
        ImGui::TextUnformatted("Instruction Summary");

        ImGui::PushID("SummaryA");

        for (const RuntimeInstruction& capture : m_a)
        {
            if (m_hideStackWrites &&
                isStackWrite(capture))
            {
                continue;
            }

            if (!shownInstructionsA.insert(
                capture.address
            ).second)
            {
                continue;
            }

            const size_t countA =
                countInstructionOccurrences(
                    m_a,
                    capture.address
                );

            const size_t countB =
                countInstructionOccurrences(
                    m_b,
                    capture.address
                );

            char summaryText[128];

            std::snprintf(
                summaryText,
                sizeof(summaryText),
                "I: 0x%zX   A:%zu B:%zu",
                capture.address,
                countA,
                countB
            );

            ImGui::Selectable(
                summaryText,
                false,
                ImGuiSelectableFlags_AllowDoubleClick
            );

            if (ImGui::IsItemHovered() &&
                ImGui::IsMouseDoubleClicked(
                    ImGuiMouseButton_Left
                ))
            {
                scannerAddress.value =
                    capture.address;
            }

			

        }

        ImGui::PopID();

        ImGui::Separator();
        
        ImGui::PushID("SummaryB");

for (const RuntimeInstruction& capture : m_b)
{
    if (m_hideStackWrites &&
        isStackWrite(capture))
    {
        continue;
    }

    if (!shownInstructionsB.insert(
        capture.address
    ).second)
    {
        continue;
    }

    const size_t countA =
        countInstructionOccurrences(
            m_a,
            capture.address
        );

    const size_t countB =
        countInstructionOccurrences(
            m_b,
            capture.address
        );

    char summaryText[128];

    std::snprintf(
        summaryText,
        sizeof(summaryText),
        "I: 0x%zX   A:%zu B:%zu",
        capture.address,
        countA,
        countB
    );

    ImGui::Selectable(
        summaryText,
        false,
        ImGuiSelectableFlags_AllowDoubleClick
    );

    if (ImGui::IsItemHovered() &&
        ImGui::IsMouseDoubleClicked(
            ImGuiMouseButton_Left
        ))
    {
        scannerAddress.value =
            capture.address;
    }
    
}
ImGui::PopID();

        // BISHERIGER CODE GEHT HIER WEITER
        ImGui::Columns(
            2,
            "MemoryWriteComparisonColumns",
            true
        );

        ImGui::Columns(
            2,
            "MemoryWriteComparisonColumns",
            true
        );

        ImGui::Text(
            "A: %zu   Unique: %zu",
            m_a.size(),
            countUnique(
                m_a,
                m_b
            )
        );

        std::unordered_set<size_t>
            shownWriteAddresses;

        ImGui::PushID("A");

        for (size_t i = 0;
            i < m_a.size();
            ++i)
        {
            const RuntimeInstruction& capture =
                m_a[i];
            
            if (m_hideStackWrites &&
                isStackWrite(capture))
            {
                continue;
            }

            const size_t matchingIndex =
                findMatchingIndex(
                    m_b,
                    capture,
                    0
                );

            const bool isDifferent =
                matchingIndex ==
                m_b.size();

            if (!shownWriteAddresses.insert(
                capture.writeAddress
            ).second)
            {
                continue;
            }

            if (m_showUniqueOnly &&
                !isDifferent)
            {
                continue;
            }

            if (m_showUniqueOnly &&
                !isDifferent)
            {
                continue;
            }

            const size_t countA =
                countOccurrences(
                    m_a,
                    capture
                );

            const size_t countB =
                countOccurrences(
                    m_b,
                    capture
                );

            ImGui::PushID(
                static_cast<int>(i)
            );

            if (isDifferent)
            {
                ImGui::PushStyleColor(
                    ImGuiCol_Text,
                    ImVec4(
                        1.0f,
                        1.0f,
                        0.0f,
                        1.0f
                    )
                );
            }

            ImGui::Text(
                "%zu",
                i
            );

            ImGui::SameLine();

            char instructionText[64];

            std::snprintf(
                instructionText,
                sizeof(instructionText),
                "I: 0x%zX",
                capture.address
            );

            ImGui::Selectable(
                instructionText,
                false,
                ImGuiSelectableFlags_AllowDoubleClick,
                ImVec2(
                    ImGui::CalcTextSize(
                        instructionText
                    ).x,
                    0.0f
                )
            );

            if (ImGui::IsItemHovered() &&
                ImGui::IsMouseDoubleClicked(
                    ImGuiMouseButton_Left
                ))
            {
                scannerAddress.value =
                    capture.address;
            }

            ImGui::SameLine();

            char writeText[64];

            std::snprintf(
                writeText,
                sizeof(writeText),
                "W: 0x%zX",
                capture.writeAddress
            );

            ImGui::Selectable(
                writeText,
                false,
                ImGuiSelectableFlags_AllowDoubleClick,
                ImVec2(
                    ImGui::CalcTextSize(
                        writeText
                    ).x,
                    0.0f
                )
            );

            if (ImGui::IsItemHovered() &&
                ImGui::IsMouseDoubleClicked(
                    ImGuiMouseButton_Left
                ))
            {
                scannerAddress.value =
                    capture.writeAddress;
            }

            ImGui::SameLine();

            ImGui::Text(
                "V: 0x%02X   A:%zu B:%zu",
                static_cast<unsigned int>(
                    capture.writeValue
                    ),
                countA,
                countB
            );

            if (isDifferent)
            {
                ImGui::PopStyleColor();
            }

            ImGui::PopID();
        }

        ImGui::PopID();

        ImGui::NextColumn();

        ImGui::Text(
            "B: %zu   Unique: %zu",
            m_b.size(),
            countUnique(
                m_b,
                m_a
            )
        );

        std::unordered_set<size_t>
            shownWriteAddressesB;

        ImGui::PushID("B");

        for (size_t i = 0;
            i < m_b.size();
            ++i)
        {
            const RuntimeInstruction& capture =
                m_b[i];

            if (m_hideStackWrites &&
                isStackWrite(capture))
            {
                continue;
            }

            const size_t matchingIndex =
                findMatchingIndex(
                    m_a,
                    capture,
                    0
                );

            const bool isDifferent =
                matchingIndex ==
                m_a.size();

            if (!shownWriteAddressesB.insert(
                capture.writeAddress
            ).second)
            {
                continue;
            }

            if (m_showUniqueOnly &&
                !isDifferent)
            {
                continue;
            }

            const size_t countA =
                countOccurrences(
                    m_a,
                    capture
                );

            const size_t countB =
                countOccurrences(
                    m_b,
                    capture
                );

            ImGui::PushID(
                static_cast<int>(i)
            );

            if (isDifferent)
            {
                ImGui::PushStyleColor(
                    ImGuiCol_Text,
                    ImVec4(
                        1.0f,
                        1.0f,
                        0.0f,
                        1.0f
                    )
                );
            }

            ImGui::Text(
                "%zu",
                i
            );

            ImGui::SameLine();

            char instructionText[64];

            std::snprintf(
                instructionText,
                sizeof(instructionText),
                "I: 0x%zX",
                capture.address
            );

            ImGui::Selectable(
                instructionText,
                false,
                ImGuiSelectableFlags_AllowDoubleClick,
                ImVec2(
                    ImGui::CalcTextSize(
                        instructionText
                    ).x,
                    0.0f
                )
            );

            if (ImGui::IsItemHovered() &&
                ImGui::IsMouseDoubleClicked(
                    ImGuiMouseButton_Left
                ))
            {
                scannerAddress.value =
                    capture.address;
            }

            ImGui::SameLine();

            char writeText[64];

            std::snprintf(
                writeText,
                sizeof(writeText),
                "W: 0x%zX",
                capture.writeAddress
            );

            ImGui::Selectable(
                writeText,
                false,
                ImGuiSelectableFlags_AllowDoubleClick,
                ImVec2(
                    ImGui::CalcTextSize(
                        writeText
                    ).x,
                    0.0f
                )
            );

            if (ImGui::IsItemHovered() &&
                ImGui::IsMouseDoubleClicked(
                    ImGuiMouseButton_Left
                ))
            {
                scannerAddress.value =
                    capture.writeAddress;
            }

            ImGui::SameLine();

            ImGui::Text(
                "V: 0x%02X   A:%zu B:%zu",
                static_cast<unsigned int>(
                    capture.writeValue
                    ),
                countA,
                countB
            );

            if (isDifferent)
            {
                ImGui::PopStyleColor();
            }

            ImGui::PopID();
        }

        ImGui::PopID();

        ImGui::Columns(
            1
        );
        ImGui::PopID();
    }

    bool MemoryWriteComparison::different(
        size_t index
    ) const
    {
        if (index >= m_a.size() ||
            index >= m_b.size())
        {
            return true;
        }

        return
            m_a[index].address !=
            m_b[index].address ||
            m_a[index].writeAddress !=
            m_b[index].writeAddress ||
            m_a[index].writeValue !=
            m_b[index].writeValue;
    }

    bool MemoryWriteComparison::sameWrite(
        const RuntimeInstruction& a,
        const RuntimeInstruction& b
    ) const
    {
        return
            a.writeAddress ==
            b.writeAddress;
    }

    size_t MemoryWriteComparison::findMatchingIndex(
        const std::vector<RuntimeInstruction>& captures,
        const RuntimeInstruction& target,
        size_t startIndex
    ) const
    {
        for (size_t i = startIndex;
            i < captures.size();
            ++i)
        {
            if (sameWrite(
                captures[i],
                target
            ))
            {
                return i;
            }
        }

        return captures.size();
    }

    size_t MemoryWriteComparison::countUnique(
        const std::vector<RuntimeInstruction>& source,
        const std::vector<RuntimeInstruction>& other
    ) const
    {
        size_t count = 0;

        for (const RuntimeInstruction& capture : source)
        {
            if (m_hideStackWrites &&
                isStackWrite(capture))
            {
                continue;
            }

            if (findMatchingIndex(
                other,
                capture,
                0
            ) == other.size())
            {
                ++count;
            }
        }

        return count;
    }

    size_t MemoryWriteComparison::countOccurrences(
        const std::vector<RuntimeInstruction>& captures,
        const RuntimeInstruction& target
    ) const
    {
        size_t count = 0;

        for (const RuntimeInstruction& capture : captures)
        {
            if (m_hideStackWrites &&
                isStackWrite(capture))
            {
                continue;
            }

            if (capture.address == target.address &&
                capture.writeAddress == target.writeAddress)
            {
                ++count;
            }
        }

        return count;
    }

    size_t MemoryWriteComparison::countInstructionOccurrences(
        const std::vector<RuntimeInstruction>& captures,
        size_t instructionAddress
    ) const
    {
        size_t count = 0;

        for (const RuntimeInstruction& capture : captures)
        {
            if (m_hideStackWrites &&
                isStackWrite(capture))
            {
                continue;
            }

            if (capture.address == instructionAddress)
            {
                ++count;
            }
        }

        return count;
    }

    bool MemoryWriteComparison::isStackWrite(
        const RuntimeInstruction& capture
    ) const
    {
        const size_t stackBase =
            static_cast<size_t>(
                capture.registers.ss
                ) << 4;

        const size_t stackPointer =
            stackBase +
            capture.registers.sp;

        return
            capture.writeAddress >=
            stackPointer - 0x20 &&
            capture.writeAddress <=
            stackPointer + 0x20;
    }

    void MemoryWriteComparison::setA(
        const std::vector<RuntimeInstruction>& captures
    )
    {
        m_a =
            captures;
    }

    void MemoryWriteComparison::setB(
        const std::vector<RuntimeInstruction>& captures
    )
    {
        m_b =
            captures;
    }
}