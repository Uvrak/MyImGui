#include "pch.h"
#include "MemoryScannerCandidateTable.h"
#include "MemoryScannerWindow.h"
#include "MemoryScannerCandidateRow.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

#include "imgui.h"

namespace DosBoxMemoryTools
{
    void MemoryScannerCandidateTable::draw(MemoryScannerWindow& window, bool liveView)
    {
        std::vector<int> filteredIndices;
        std::vector<int> pinnedIndices;
        std::vector<int> describedIndices;

        std::vector<size_t> pinnedOnlyAddresses;

        std::vector<size_t> describedOnlyAddresses;

        filteredIndices.reserve(
            static_cast<int>(
                window.m_scanner.
                candidates().
                size()
                )
        );

        for (int index = 0;
            index <
            static_cast<int>(
                window.m_scanner.
                candidates().
                size()
                );
            ++index)
        {
            const MemoryCandidate&
                candidate =
                window.m_scanner.
                candidates()[
                    index
                ];

            const int difference =
                static_cast<int>(
                    candidate.currentValue
                    ) -
                static_cast<int>(
                    candidate.previousValue
                    );

            const bool pinned =
                window.m_pinnedAddresses.contains(
                    candidate.address
                );

            if (pinned)
            {
                pinnedIndices.push_back(
                    index
                );

                continue;
            }

            if (pinned ||
                ((!window.m_filterPrevious ||
                    candidate.previousValue ==
                    static_cast<uint8_t>(
                        window.m_previousValue
                        )) &&
                    (!window.m_filterCurrent ||
                        candidate.currentValue ==
                        static_cast<uint8_t>(
                            window.m_currentValue
                            )) &&
                    (!window.m_filterDifference ||
                        difference ==
                        window.m_differenceValue)))
            {
                if (window.m_descriptionsFirst &&
                    window.hasDescription(
                        candidate.address
                    ))
                {
                    describedIndices.push_back(
                        index
                    );
                }
                else
                {
                    filteredIndices.push_back(
                        index
                    );
                }
            }
        }

        for (const size_t pinnedAddress :
        window.m_pinnedAddresses)
        {
            bool foundCandidate = false;

            for (const MemoryCandidate&
                candidate :
                window.m_scanner.candidates())
            {
                if (candidate.address ==
                    pinnedAddress)
                {
                    foundCandidate = true;
                    break;
                }
            }

            if (!foundCandidate)
            {
                pinnedOnlyAddresses.push_back(
                    pinnedAddress
                );
            }
        }

        if (window.m_descriptionsFirst)
        {
            if (window.m_descriptionsFirst)
            {
                for (const auto& [address, description] :
                    window.m_pinnedDescriptions)
                {
                    if (description.empty())
                    {
                        continue;
                    }

                    if (window.m_scanner.pinnedAddresses().contains(
                        address
                    ))
                    {
                        continue;
                    }

                    bool foundCandidate = false;

                    for (const MemoryCandidate&
                        candidate :
                        window.m_scanner.candidates())
                    {
                        if (candidate.address ==
                            address)
                        {
                            foundCandidate = true;
                            break;
                        }
                    }

                    if (!foundCandidate)
                    {
                        describedOnlyAddresses.push_back(
                            address
                        );
                    }
                }
            }
        }

        if (window.m_descriptionsFirst)
        {
            filteredIndices.insert(
                filteredIndices.begin(),
                describedIndices.begin(),
                describedIndices.end()
            );
        }


        filteredIndices.insert(
            filteredIndices.begin(),
            pinnedIndices.begin(),
            pinnedIndices.end()
        );

        if (window.m_hasFoundAddress)
        {
            const auto foundIt =
                std::find_if(
                    filteredIndices.begin(),
                    filteredIndices.end(),
                    [&window](int index)
                    {
                        return
                            window.m_scanner.candidates()[
                                index
                            ].address ==
                            window.m_foundAddress;
                    }
                );

            if (foundIt !=
                filteredIndices.end())
            {
                const int foundIndex =
                    *foundIt;

                filteredIndices.erase(
                    foundIt
                );

                filteredIndices.insert(
                    filteredIndices.begin(),
                    foundIndex
                );
            }
        }

        ImGui::Text(
            "Visible: %zu / %zu",
            filteredIndices.size(),
            window.m_scanner.
            candidates().
            size()
        );

        ImGui::SetNextItemWidth(
            120.0f
        );

        ImGui::NewLine();            

        if (ImGui::Button(
            "Pin Address"
        ))
        {
            const size_t address =
                window.m_scannerAddress.value;

            window.m_pinnedAddresses.insert(
                address
            );

            window.m_scanner.pinAddress(
                address
            );

            window.savePinnedAddresses();
        }

        ImGui::SameLine();

        ImGui::BeginDisabled(
            window.m_selectedAddresses.empty()
        );

        if (ImGui::Button(
            "Pin Selected"
        ))
        {
            for (size_t address :
            window.m_selectedAddresses)
            {
                window.m_pinnedAddresses.insert(
                    address
                );

                window.m_scanner.pinAddress(
                    address
                );
            }

            window.savePinnedAddresses();

            window.m_selectedAddresses.clear();
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(
            window.m_pinnedAddresses.empty()
        );

        if (ImGui::Button(
            "Unpin All"
        ))
        {
            window.m_pinnedAddresses.clear();
            window.m_scanner.clearPinnedAddresses();
            window.m_selectedAddresses.clear();

            window.savePinnedAddresses();
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Checkbox(
            "Descriptions first",
            &window.m_descriptionsFirst
        ))
        {
            window.saveScannerSettings();
        }

        ImGui::Text(
            "Known descriptions: %zu",
            window.m_pinnedDescriptions.size()
        );

        ImGui::Separator();

        if (ImGui::BeginTable(
            "##MemoryCandidates",
            4,
            ImGuiTableFlags_Borders |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_Resizable,
            ImVec2(
                0.0f,
                0.0f
            )
            ))
            {
           

                ImGui::TableSetupColumn(
                    "Address"
                );

                ImGui::TableSetupColumn(
                    "Previous",
                    ImGuiTableColumnFlags_WidthFixed,
                    105.0f
                );

                ImGui::TableSetupColumn(
                    "Current",
                    ImGuiTableColumnFlags_WidthFixed,
                    105.0f
                );

                ImGui::TableSetupColumn(
                    "Difference",
                    ImGuiTableColumnFlags_WidthFixed,
                    105.0f
                );

                ImGui::TableHeadersRow();

                ImGui::TableNextRow();


                ImGui::TableSetColumnIndex(1);

                if (ImGui::Checkbox(
                    "##FilterPrevious",
                    &window.m_filterPrevious
                ))
                {
                    window.saveScannerSettings();
                }

                ImGui::SameLine();

                ImGui::SetNextItemWidth(
                    110.0f
                );

                if (ImGui::InputInt(
                    "##PreviousValue",
                    &window.m_previousValue
                ))
                {
                    window.saveScannerSettings();
                }

                ImGui::TableSetColumnIndex(2);

                if (ImGui::Checkbox(
                    "##FilterCurrent",
                    &window.m_filterCurrent
                ))
                {
                    window.saveScannerSettings();
                }

                ImGui::SameLine();

                ImGui::SetNextItemWidth(
                    110.0f
                );

                if (ImGui::InputInt(
                    "##CurrentValue",
                    &window.m_currentValue
                ))
                {
                    window.saveScannerSettings();
                }

                ImGui::TableSetColumnIndex(3);

                if (ImGui::Checkbox(
                    "##FilterDifference",
                    &window.m_filterDifference
                ))
                {
                    if (window.m_filterDifference &&
                        window.m_deleteOtherDifferenceCandidates)
                    {
                        window.m_scanner.keepDifference(
                            window.m_differenceValue
                        );
                    }

                    window.saveScannerSettings();
                }

                ImGui::SameLine();

                ImGui::SetNextItemWidth(
                    110.0f
                );

                if (ImGui::InputInt(
                    "##DifferenceValue",
                    &window.m_differenceValue
                ))
                {
                    window.saveScannerSettings();
                }

                if (ImGui::Checkbox(
                    "Delete others",
                    &window.m_deleteOtherDifferenceCandidates
                ))
                {
                    window.saveScannerSettings();
                }

                if (window.m_deleteOtherDifferenceCandidates)
                {
                    ImGui::SameLine();

                    if (ImGui::Button(
                        "Apply"
                    ))
                    {
                        window.m_applyDifferenceDeleteRequested =
                            true;
                    }
                }

                ImGuiListClipper clipper;

                clipper.Begin(
                    static_cast<int>(
                        pinnedOnlyAddresses.size() +
                        describedOnlyAddresses.size() +
                        filteredIndices.size()
                        ),
                    ImGui::GetTextLineHeightWithSpacing()
                );

                while (clipper.Step())
                {
                    for (int index =
                        clipper.DisplayStart;
                        index <
                        clipper.DisplayEnd;
                        ++index)
                    {
                        MemoryScannerCandidateRow::draw(
                            window, liveView, index,
                            pinnedOnlyAddresses, describedOnlyAddresses, filteredIndices
                        );
                    }
                }

                ImGui::EndTable();
            }
    }
}
