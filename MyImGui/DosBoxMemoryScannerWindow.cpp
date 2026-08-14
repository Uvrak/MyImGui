#include "pch.h"
#include "DosBoxMemoryScannerWindow.h"

#include <cstdio>
#include <cstdlib>

#include "imgui.h"

namespace MyImGui
{
    void DosBoxMemoryScannerWindow::draw(
        bool* isOpen
    )
    {
        if (isOpen &&
            !*isOpen)
        {
            return;
        }

        ImGui::SetNextWindowSize(
            ImVec2(
                700.0f,
                520.0f
            ),
            ImGuiCond_FirstUseEver
        );

        if (!ImGui::Begin(
            "DOSBox Memory Scanner",
            isOpen
        ))
        {
            ImGui::End();
            return;
        }
        if (m_toolbarWindow.begin())
        {
            m_toolbarLayout.begin();

            const ImVec2 framePadding =
                ImGui::GetStyle().FramePadding;

            // New Scan
            const ImVec2 newScanSize(
                ImGui::CalcTextSize("New Scan").x +
                framePadding.x * 2.0f,
                ImGui::GetFrameHeight()
            );

            m_toolbarLayout.beginItem(
                newScanSize
            );

            if (ImGui::Button(
                "New Scan"
            ))
            {
                m_scanner.scan(
                    DosBoxMemoryScanMode::NewScan
                );
            }

            m_toolbarLayout.endItem();


            // Scan mode
            const char* scanModes[] =
            {
                "Exact Value",
                "Changed",
                "Unchanged",
                "Increased",
                "Decreased"
            };

            int selectedMode =
                static_cast<int>(
                    m_scanMode
                    ) - 1;

            m_toolbarLayout.beginItem(
                ImVec2(
                    160.0f,
                    ImGui::GetFrameHeight()
                )
            );

            ImGui::SetNextItemWidth(
                160.0f
            );

            if (ImGui::Combo(
                "##ScanMode",
                &selectedMode,
                scanModes,
                IM_ARRAYSIZE(scanModes)
            ))
            {
                m_scanMode =
                    static_cast<
                    DosBoxMemoryScanMode
                    >(
                        selectedMode + 1
                        );
            }

            m_toolbarLayout.endItem();

            // Previous
            
            // Exact value
            if (m_scanMode ==
                DosBoxMemoryScanMode::ExactValue)
            {
                m_toolbarLayout.beginItem(
                    ImVec2(
                        80.0f,
                        ImGui::GetFrameHeight()
                    )
                );

                ImGui::SetNextItemWidth(
                    80.0f
                );

                ImGui::InputInt(
                    "##ExactValue",
                    &m_exactValue
                );

                m_toolbarLayout.endItem();

                if (m_exactValue < 0)
                {
                    m_exactValue = 0;
                }

                if (m_exactValue > 255)
                {
                    m_exactValue = 255;
                }
            }


            // Next Scan
            const ImVec2 nextScanSize(
                ImGui::CalcTextSize("Next Scan").x +
                framePadding.x * 2.0f,
                ImGui::GetFrameHeight()
            );

            m_toolbarLayout.beginItem(
                nextScanSize
            );

            if (ImGui::Button(
                "Next Scan"
            ))
            {
                m_scanner.scan(
                    m_scanMode,
                    static_cast<uint8_t>(
                        m_exactValue
                        )
                );
            }

            m_toolbarLayout.endItem();


            // Reset
            const ImVec2 resetSize(
                ImGui::CalcTextSize("Reset").x +
                framePadding.x * 2.0f,
                ImGui::GetFrameHeight()
            );

            m_toolbarLayout.beginItem(
                resetSize
            );

            if (ImGui::Button(
                "Reset"
            ))
            {
                m_scanner.reset();
            }

            m_toolbarLayout.endItem();

            // Clear Filters
            const ImVec2 clearFiltersSize(
                ImGui::CalcTextSize("Clear Filters").x +
                framePadding.x * 2.0f,
                ImGui::GetFrameHeight()
            );

            m_toolbarLayout.beginItem(
                clearFiltersSize
            );

            if (ImGui::Button(
                "Clear Filters"
            ))
            {
                m_filterPrevious = false;
                m_previousValue = 0;

                m_filterCurrent = false;
                m_currentValue = 0;

                m_filterDifference = false;
                m_differenceValue = 0;
            }

            m_toolbarLayout.endItem();

            m_toolbarLayout.end();
        }

        m_toolbarWindow.end();

        ImGui::Separator();

        ImGui::Text(
            "Candidates: %zu",
            m_scanner.
            candidates().
            size()
        );

        ImGui::TextWrapped(
            "%s",
            m_scanner.
            status().
            c_str()
        );

        ImGui::Separator();

        std::vector<int> filteredIndices;
        std::vector<int> pinnedIndices;
        
        filteredIndices.reserve(
            static_cast<int>(
                m_scanner.
                candidates().
                size()
                )
        );

        for (int index = 0;
            index <
            static_cast<int>(
                m_scanner.
                candidates().
                size()
                );
            ++index)
        {
            const DosBoxMemoryCandidate&
                candidate =
                m_scanner.
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
                m_pinnedAddresses.contains(
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
                ((!m_filterPrevious ||
                    candidate.previousValue ==
                    static_cast<uint8_t>(
                        m_previousValue
                        )) &&
                    (!m_filterCurrent ||
                        candidate.currentValue ==
                        static_cast<uint8_t>(
                            m_currentValue
                            )) &&
                    (!m_filterDifference ||
                        difference ==
                        m_differenceValue)))
            {
                filteredIndices.push_back(
                    index
                );
            }
        }

        filteredIndices.insert(
            filteredIndices.begin(),
            pinnedIndices.begin(),
            pinnedIndices.end()
        );

        ImGui::Text(
            "Visible: %zu / %zu",
            filteredIndices.size(),
            m_scanner.
            candidates().
            size()
        );

        ImGui::SetNextItemWidth(
            120.0f
        );

        const bool addressEnter =
            ImGui::InputText(
                "Address",
                m_addressSearch,
                sizeof(m_addressSearch),
                ImGuiInputTextFlags_EnterReturnsTrue
            );

        ImGui::SameLine();

        if (ImGui::Button(
            "Find"
        ) || addressEnter)
        {
            m_addressSearchAttempted = true;
            char* end = nullptr;

            const unsigned long long address =
                std::strtoull(
                    m_addressSearch,
                    &end,
                    0
                );

            if (end != m_addressSearch &&
                *end == '\0')
            {
                m_foundAddress =
                    static_cast<size_t>(
                        address
                        );

                m_hasFoundAddress = true;
            }
            else
            {
                m_hasFoundAddress = false;
            }

            bool foundCandidate = false;

            for (const DosBoxMemoryCandidate&
                candidate :
                m_scanner.candidates())
            {
                if (candidate.address ==
                    m_foundAddress)
                {
                    foundCandidate = true;
                    break;
                }
            }

            m_hasFoundAddress =
                foundCandidate;

            if (foundCandidate)
            {
                m_pinnedAddresses.insert(
                    m_foundAddress
                );

                m_scanner.pinAddress(
                    m_foundAddress
                );
            }
        }

        ImGui::SameLine();

        if (m_addressSearchAttempted)
        {
            if (m_hasFoundAddress)
            {
                ImGui::TextUnformatted("Found");
            }
            else
            {
                ImGui::TextUnformatted("Not found");
            }
        }

        ImGui::NewLine();

        if (!m_pinnedAddresses.empty())
        {
            ImGui::SameLine();

            if (ImGui::Button(
                "Unpin All"
            ))
            {
                m_pinnedAddresses.clear();
                m_scanner.clearPinnedAddresses();
            }
        }

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
                    "Previous"
                );

                ImGui::TableSetupColumn(
                    "Current"
                );

                ImGui::TableSetupColumn(
                    "Difference"
                );

                ImGui::TableHeadersRow();

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(1);

                ImGui::Checkbox(
                    "##FilterPrevious",
                    &m_filterPrevious
                );

                ImGui::SameLine();

                ImGui::SetNextItemWidth(
                    80.0f
                );

                ImGui::InputInt(
                    "##PreviousValue",
                    &m_previousValue
                );

                ImGui::TableSetColumnIndex(2);

                ImGui::Checkbox(
                    "##FilterCurrent",
                    &m_filterCurrent
                );

                ImGui::SameLine();

                ImGui::SetNextItemWidth(
                    80.0f
                );

                ImGui::InputInt(
                    "##CurrentValue",
                    &m_currentValue
                );

                ImGui::TableSetColumnIndex(3);

                ImGui::Checkbox(
                    "##FilterDifference",
                    &m_filterDifference
                );

                ImGui::SameLine();

                ImGui::SetNextItemWidth(
                    80.0f
                );

                ImGui::InputInt(
                    "##DifferenceValue",
                    &m_differenceValue
                );              

                ImGuiListClipper clipper;

                clipper.Begin(
                    static_cast<int>(
                        filteredIndices.size()
                        )
                );

                while (clipper.Step())
                {
                    for (int index =
                        clipper.DisplayStart;
                        index <
                        clipper.DisplayEnd;
                        ++index)
                    {
                        const int candidateIndex =
                            filteredIndices[
                                index
                            ];

                        const DosBoxMemoryCandidate&
                            candidate =
                            m_scanner.
                            candidates()[
                                candidateIndex
                            ];

                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);

                        const bool pinned =
                            m_pinnedAddresses.contains(
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

                            if (ImGui::Selectable(
                                addressText,
                                pinned,
                                ImGuiSelectableFlags_SpanAllColumns
                            ))
                            {
                                if (pinned)
                                {
                                    m_pinnedAddresses.erase(
                                        candidate.address
                                    );

                                    m_scanner.unpinAddress(
                                        candidate.address
                                    );
                                }
                                else
                                {
                                    m_pinnedAddresses.insert(
                                        candidate.address
                                    );

                                    m_scanner.pinAddress(
                                        candidate.address
                                    );
                                }
                            }

                            if (ImGui::BeginPopupContextItem())
                            {
                                if (ImGui::MenuItem(
                                    "Copy Address"
                                ))
                                {
                                    char addressText[32];

                                    std::snprintf(
                                        addressText,
                                        sizeof(addressText),
                                        "0x%05zX",
                                        candidate.address
                                    );

                                    ImGui::SetClipboardText(
                                        addressText
                                    );
                                }

                                if (ImGui::MenuItem(
                                    "Copy Current Value"
                                ))
                                {
                                    char valueText[32];

                                    std::snprintf(
                                        valueText,
                                        sizeof(valueText),
                                        "%u",
                                        static_cast<unsigned int>(
                                            candidate.currentValue
                                            )
                                    );

                                    ImGui::SetClipboardText(
                                        valueText
                                    );
                                }

                                if (ImGui::MenuItem(
                                    "Copy Previous Value"
                                ))
                                {
                                    char valueText[32];

                                    std::snprintf(
                                        valueText,
                                        sizeof(valueText),
                                        "%u",
                                        static_cast<unsigned int>(
                                            candidate.previousValue
                                            )
                                    );

                                    ImGui::SetClipboardText(
                                        valueText
                                    );
                                }
                                if (ImGui::MenuItem(
                                    "Copy Difference"
                                ))
                                {
                                    const int difference =
                                        static_cast<int>(
                                            candidate.currentValue
                                            ) -
                                        static_cast<int>(
                                            candidate.previousValue
                                            );

                                    char valueText[32];

                                    std::snprintf(
                                        valueText,
                                        sizeof(valueText),
                                        "%d",
                                        difference
                                    );

                                    ImGui::SetClipboardText(
                                        valueText
                                    );
                                }

                                ImGui::EndPopup();
                            }

                        ImGui::TableSetColumnIndex(1);

                        ImGui::Text(
                            "%u",
                            static_cast<unsigned int>(
                                candidate.previousValue
                                )
                        );

                        ImGui::TableSetColumnIndex(2);

                        ImGui::Text(
                            "%u",
                            static_cast<unsigned int>(
                                candidate.currentValue
                                )
                        );

                        ImGui::TableSetColumnIndex(3);

                        ImGui::Text(
                            "%d",
                            static_cast<int>(
                                candidate.currentValue
                                ) -
                            static_cast<int>(
                                candidate.previousValue
                                )
                        );
                    }
                }

                ImGui::EndTable();
            }
        
        ImGui::End();
    }
}