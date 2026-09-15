#include "pch.h"
#include "MemoryScannerCandidateRow.h"
#include "MemoryScannerWindow.h"
#include "MemoryScannerAddressContextMenu.h"

#include <cstdio>

#include "imgui.h"

namespace DosBoxMemoryTools
{
    void MemoryScannerCandidateRow::draw(MemoryScannerWindow& window, bool liveView, int index,
        const std::vector<size_t>& pinnedOnlyAddresses,
        const std::vector<size_t>& describedOnlyAddresses,
        const std::vector<int>& filteredIndices)
    {
        const int pinnedOnlyCount =
            static_cast<int>(
                pinnedOnlyAddresses.size()
                );

        const int describedOnlyCount =
            static_cast<int>(
                describedOnlyAddresses.size()
                );

        const bool pinnedOnly =
            index < pinnedOnlyCount;

        const bool describedOnly =
            !pinnedOnly &&
            index <
            pinnedOnlyCount +
            describedOnlyCount;

        const int candidateIndex =
            pinnedOnly ||
            describedOnly
            ? -1
            : filteredIndices[
                index -
                    pinnedOnlyCount -
                    describedOnlyCount
            ];

        if (pinnedOnly)
        {
            const size_t address =
                pinnedOnlyAddresses[
                    index
                ];

            uint32_t previousValue = 0;
            uint32_t currentValue = 0;

            const bool hasPreviousValue =
                window.m_scanner.readPreviousValue(
                    address,
                    window.m_valueType,
                    previousValue
                );

            const auto pinnedValue =
                window.m_pinnedDisplayValues.find(
                    address
                );

            if (pinnedValue !=
                window.m_pinnedDisplayValues.end())
            {
                currentValue =
                    pinnedValue->second;
            }

            ImGui::TableNextRow();

            if (window.hasDescription(
                address
            ))
            {
                ImGui::TableSetBgColor(
                    ImGuiTableBgTarget_RowBg0,
                    ImGui::GetColorU32(
                        ImVec4(
                            0.10f,
                            0.30f,
                            0.10f,
                            1.0f
                        )
                    )
                );
            }

            ImGui::TableSetColumnIndex(0);

            char addressText[32];

            std::snprintf(
                addressText,
                sizeof(addressText),
                "[PIN] 0x%05zX",
                address
            );

            const bool selected =
                window.m_selectedAddresses.contains(
                    address
                );

            if (ImGui::Selectable(
                addressText,
                selected,
                ImGuiSelectableFlags_SpanAllColumns
            ))
            {
                window.m_lastSelectedAddress =
                    address;

                window.m_hasSelectedAddress =
                    true;

                if (selected)
                {
                    window.m_selectedAddresses.erase(
                        address
                    );
                }
                else
                {
                    window.m_selectedAddresses.insert(
                        address
                    );
                }
            }

            const auto description =
                window.m_pinnedDescriptions.find(
                    address
                );

            if (ImGui::IsItemHovered() &&
                description !=
                window.m_pinnedDescriptions.end() &&
                !description->second.empty())
            {
                ImGui::SetTooltip(
                    "%s",
                    description->second.c_str()
                );
            }

            MemoryScannerAddressContextMenu::drawPinnedOnly(window, address, currentValue);
            ImGui::TableSetColumnIndex(1);
            
            if (hasPreviousValue)
            {
                char valueText[32];

                std::snprintf(
                    valueText,
                    sizeof(valueText),
                    "0x%X",
                    static_cast<unsigned int>(
                        previousValue
                        )
                );

                ImGui::SetCursorPosX(
                    ImGui::GetCursorPosX() +
                    ImGui::GetColumnWidth() -
                    ImGui::CalcTextSize(valueText).x -
                    ImGui::GetStyle().CellPadding.x * 2.0f
                );

                ImGui::TextUnformatted(
                    valueText
                );
            }
            else
            {
                ImGui::TextUnformatted("-");
            }

            ImGui::TableSetColumnIndex(2);

            uint32_t liveCurrentValue =
                currentValue;

            if (liveView)
            {
                window.m_scanner.readCurrentValue(
                    address,
                    window.m_valueType,
                    liveCurrentValue
                );
            }

            char currentText[32];

            std::snprintf(
                currentText,
                sizeof(currentText),
                "0x%X",
                static_cast<unsigned int>(
                    liveCurrentValue
                    )
            );

            ImGui::SetCursorPosX(
                ImGui::GetCursorPosX() +
                ImGui::GetColumnWidth() -
                ImGui::CalcTextSize(currentText).x -
                ImGui::GetStyle().CellPadding.x * 2.0f
            );

            ImGui::TextUnformatted(currentText);

            ImGui::TableSetColumnIndex(3);

            if (hasPreviousValue)
            {
                const int difference =
                    static_cast<int>(
                        liveCurrentValue
                        ) -
                    static_cast<int>(
                        previousValue
                        );

                char differenceText[32];

                std::snprintf(
                    differenceText,
                    sizeof(differenceText),
                    "0x%X",
                    static_cast<unsigned int>(
                        difference
                        )
                );

                ImGui::SetCursorPosX(
                    ImGui::GetCursorPosX() +
                    ImGui::GetColumnWidth() -
                    ImGui::CalcTextSize(differenceText).x -
                    ImGui::GetStyle().CellPadding.x * 2.0f
                );

                ImGui::TextUnformatted(differenceText);
            }
            else
            {
                ImGui::TextUnformatted("-");
            }

            return;
        }

        if (describedOnly)
        {
            const size_t address =
                describedOnlyAddresses[
                    index - pinnedOnlyCount
                ];

            uint32_t currentValue = 0;

            window.m_scanner.readCurrentValue(
                address,
                window.m_valueType,
                currentValue
            );

            ImGui::TableNextRow();

            ImGui::TableSetBgColor(
                ImGuiTableBgTarget_RowBg0,
                ImGui::GetColorU32(
                    ImVec4(
                        0.10f,
                        0.30f,
                        0.10f,
                        1.0f
                    )
                )
            );

            ImGui::TableSetColumnIndex(0);

            char addressText[32];

            std::snprintf(
                addressText,
                sizeof(addressText),
                "0x%05zX",
                address
            );

            ImGui::TextUnformatted(
                addressText
            );

            const auto description =
                window.m_pinnedDescriptions.find(
                    address
                );

            if (ImGui::IsItemHovered() &&
                description !=
                window.m_pinnedDescriptions.end())
            {
                ImGui::SetTooltip(
                    "%s",
                    description->second.c_str()
                );
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted("-");

            ImGui::TableSetColumnIndex(2);

            ImGui::Text(
                "0x%X",
                static_cast<unsigned int>(
                    currentValue
                    )
            );

            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted("-");

            return;
        }

        const MemoryCandidate&
            candidate =
            window.m_scanner.
            candidates()[
                candidateIndex
            ];

        ImGui::TableNextRow();

        if (window.hasDescription(
            candidate.address
        ))
        {
            ImGui::TableSetBgColor(
                ImGuiTableBgTarget_RowBg0,
                ImGui::GetColorU32(
                    ImVec4(
                        0.10f,
                        0.30f,
                        0.10f,
                        1.0f
                    )
                )
            );
        }

        ImGui::TableSetColumnIndex(0);

        const bool pinned =
            window.m_pinnedAddresses.contains(
                candidate.address
            );

        char addressText[32];

        std::snprintf(
            addressText,
            sizeof(addressText),
            pinned
            ? "[PIN] 0x%05zX"
            : "0x%05zX",
            candidate.address
        );

            candidate.address;

            const bool selected =
                window.m_selectedAddresses.contains(
                    candidate.address
                );

            if (ImGui::Selectable(
                addressText,
                selected,
                ImGuiSelectableFlags_SpanAllColumns
            ))
            {
                window.m_lastSelectedAddress =
                    candidate.address;

                window.m_hasSelectedAddress =
                    true;

                if (selected)
                {
                    window.m_selectedAddresses.erase(
                        candidate.address
                    );
                }
                else
                {
                    window.m_selectedAddresses.insert(
                        candidate.address
                    );
                }
            }

            const auto description =
                window.m_pinnedDescriptions.find(
                    candidate.address
                );

            if (ImGui::IsItemHovered() &&
                description !=
                window.m_pinnedDescriptions.end() &&
                !description->second.empty())
            {
                ImGui::SetTooltip(
                    "%s",
                    description->second.c_str()
                );
            }

            MemoryScannerAddressContextMenu::drawCandidate(window, candidate, pinned);

        ImGui::TableSetColumnIndex(1);

        ImGui::Text(
            "0x%X",
            static_cast<unsigned int>(
                candidate.previousValue
                )
        );

        ImGui::TableSetColumnIndex(2);

        uint32_t displayedCurrentValue =
            candidate.currentValue;

        if (liveView)
        {
            window.m_scanner.readCurrentValue(
                candidate.address,
                window.m_valueType,
                displayedCurrentValue
            );
        }

        ImGui::Text(
            "%u",
            static_cast<unsigned int>(
                displayedCurrentValue
                )
        );

        ImGui::TableSetColumnIndex(3);

        ImGui::Text(
            "0x%X",
            static_cast<unsigned int>(
                candidate.currentValue -
                candidate.previousValue
                )
        );
    }
}
