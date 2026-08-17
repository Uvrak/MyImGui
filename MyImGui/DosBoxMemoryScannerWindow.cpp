#include "pch.h"
#include "DosBoxMemoryScannerWindow.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

#include "imgui.h"

#include "DosBoxView.h"

namespace MyImGui
{
    DosBoxMemoryScannerWindow::DosBoxMemoryScannerWindow(
        DosBoxMemoryReader& memoryReader,
        const std::string& gameId,
        DosBoxView* dosBoxView
    )
        : m_scanner(memoryReader),
        m_gameId(gameId),
        m_dosBoxView(dosBoxView)
    {
        loadPinnedAddresses();
    }

    void DosBoxMemoryScannerWindow::draw(
        bool* isOpen,
        bool& liveView
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
                if (m_limitScanRange)
                {
                    char* startEnd = nullptr;
                    char* endEnd = nullptr;

                    const unsigned long long startAddress =
                        std::strtoull(
                            m_scanStartAddress,
                            &startEnd,
                            0
                        );

                    const unsigned long long endAddress =
                        std::strtoull(
                            m_scanEndAddress,
                            &endEnd,
                            0
                        );

                    if (startEnd != m_scanStartAddress &&
                        *startEnd == '\0' &&
                        endEnd != m_scanEndAddress &&
                        *endEnd == '\0' &&
                        startAddress <= endAddress)
                    {
                        m_scanner.setScanRange(
                            static_cast<size_t>(startAddress),
                            static_cast<size_t>(endAddress)
                        );
                    }
                }
                else
                {
                    m_scanner.clearScanRange();
                }

                m_scanner.scan(
                    DosBoxMemoryScanMode::NewScan,
                    m_valueType
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

            // Value type
            const char* valueTypes[] =
            {
                "Byte",
                "Short",
                "Int"
            };

            int selectedValueType =
                static_cast<int>(
                    m_valueType
                    );

            m_toolbarLayout.beginItem(
                ImVec2(
                    80.0f,
                    ImGui::GetFrameHeight()
                )
            );

            ImGui::SetNextItemWidth(
                80.0f
            );

            if (ImGui::Combo(
                "##ValueType",
                &selectedValueType,
                valueTypes,
                IM_ARRAYSIZE(valueTypes)
            ))
            {
                m_valueType =
                    static_cast<
                    DosBoxMemoryValueType
                    >(
                        selectedValueType
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
                    m_valueType,
                    static_cast<uint32_t>(
                        m_exactValue
                        )
                );
            }

            m_toolbarLayout.endItem();

            // Refresh
            const ImVec2 refreshSize(
                ImGui::CalcTextSize("Refresh").x +
                framePadding.x * 2.0f,
                ImGui::GetFrameHeight()
            );

            m_toolbarLayout.beginItem(
                refreshSize
            );

            if (ImGui::Button("Refresh"))
            {
                m_scanner.refreshValues();
            }

            m_toolbarLayout.endItem();

            // Live View
            m_toolbarLayout.beginItem(
                ImVec2(
                    ImGui::CalcTextSize("Live").x +
                    30.0f,
                    ImGui::GetFrameHeight()
                )
            );

            ImGui::Checkbox(
                "Live",
                &liveView
            );

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

                m_selectedAddresses.clear();
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

        ImGui::SameLine();

        ImGui::Checkbox(
            "Limit Range",
            &m_limitScanRange
        );

        if (m_limitScanRange)
        {
            ImGui::SameLine();

            ImGui::SetNextItemWidth(
                120.0f
            );

            ImGui::InputText(
                "Start",
                m_scanStartAddress,
                sizeof(m_scanStartAddress)
            );

            ImGui::SameLine();

            ImGui::SetNextItemWidth(
                120.0f
            );

            ImGui::InputText(
                "End",
                m_scanEndAddress,
                sizeof(m_scanEndAddress)
            );
        }

        ImGui::Separator();

        ImGui::TextWrapped(
            "%s",
            m_scanner.
            status().
            c_str()
        );

        ImGui::Separator();

        std::vector<int> filteredIndices;
        std::vector<int> pinnedIndices;
        
        std::vector<size_t> pinnedOnlyAddresses;

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

        for (size_t pinnedAddress :
        m_scanner.pinnedAddresses())
        {
            bool foundCandidate = false;

            for (const DosBoxMemoryCandidate&
                candidate :
                m_scanner.candidates())
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

        if (m_descriptionsFirst)
        {
            std::stable_partition(
                filteredIndices.begin(),
                filteredIndices.end(),
                [this](int index)
                {
                    return hasDescription(
                        m_scanner.candidates()[
                            index
                        ].address
                    );
                }
            );
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

				savePinnedAddresses();
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

        if (!m_scanner.candidates().empty())
        {
            if (ImGui::Button(
                "Pin All"
            ))
            {
                for (const DosBoxMemoryCandidate& candidate :
                    m_scanner.candidates())
                {
                    m_pinnedAddresses.insert(
                        candidate.address
                    );

                    m_scanner.pinAddress(
                        candidate.address
                    );
                }

                savePinnedAddresses();
            }
        }

        if (!m_pinnedAddresses.empty())
        {
            ImGui::SameLine();

            if (ImGui::Button(
                "Unpin All"
            ))
            {
                m_pinnedAddresses.clear();
                m_scanner.clearPinnedAddresses();
                m_selectedAddresses.clear();

                savePinnedAddresses();
            }
        }

        if (!m_selectedAddresses.empty())
        {
            if (ImGui::Button(
                "Pin Selected"
            ))
                ImGui::SameLine();
            {
                for (size_t address :
                m_selectedAddresses)
                {
                    m_pinnedAddresses.insert(
                        address
                    );

                    m_scanner.pinAddress(
                        address
                    );
                }

                savePinnedAddresses();

                m_selectedAddresses.clear();
            }
        }

        ImGui::Separator();
        ImGui::Checkbox(
            "Descriptions first",
            &m_descriptionsFirst
        );

        ImGui::Text(
            "Known descriptions: %zu",
            m_pinnedDescriptions.size()
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
                        pinnedOnlyAddresses.size() +
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
                        const int pinnedOnlyCount =
                            static_cast<int>(
                                pinnedOnlyAddresses.size()
                                );

                        const bool pinnedOnly =
                            index < pinnedOnlyCount;
                        
                        const int candidateIndex =
                            pinnedOnly
                            ? -1
                            : filteredIndices[
                                index - pinnedOnlyCount
                            ];

                        if (pinnedOnly)
                        {
                            const size_t address =
                                pinnedOnlyAddresses[
                                    index
                                ];

                            uint8_t currentValue = 0;

                            m_scanner.readCurrentValue(
                                address,
                                currentValue
                            );

                            ImGui::TableNextRow();

                            if (hasDescription(
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
                                m_selectedAddresses.contains(
                                    address
                                );

                            if (ImGui::Selectable(
                                addressText,
                                selected,
                                ImGuiSelectableFlags_SpanAllColumns
                            ))
                            {
                                m_lastSelectedAddress =
                                    address;

                                m_hasSelectedAddress =
                                    true;

                                if (selected)
                                {
                                    m_selectedAddresses.erase(
                                        address
                                    );
                                }
                                else
                                {
                                    m_selectedAddresses.insert(
                                        address
                                    );
                                }
                            }

                            const auto description =
                                m_pinnedDescriptions.find(
                                    address
                                );

                            if (ImGui::IsItemHovered() &&
                                description !=
                                m_pinnedDescriptions.end() &&
                                !description->second.empty())
                            {
                                ImGui::SetTooltip(
                                    "%s",
                                    description->second.c_str()
                                );
                            }

                            if (ImGui::IsItemClicked(
                                ImGuiMouseButton_Right
                            ))
                            {
                                m_descriptionAddress =
                                    address;

                                const auto description =
                                    m_pinnedDescriptions.find(
                                        address
                                    );

                                if (description !=
                                    m_pinnedDescriptions.end())
                                {
                                    std::snprintf(
                                        m_descriptionBuffer,
                                        sizeof(m_descriptionBuffer),
                                        "%s",
                                        description->second.c_str()
                                    );
                                }
                                else
                                {
                                    m_descriptionBuffer[0] =
                                        '\0';
                                }
                            }

                            if (ImGui::BeginPopupContextItem())
                            {
                                if (ImGui::MenuItem(
                                    "Unpin Address"
                                ))
                                {
                                    m_pinnedAddresses.erase(
                                        address
                                    );

                                    m_scanner.unpinAddress(
                                        address
                                    );

                                    savePinnedAddresses();

                                    m_selectedAddresses.erase(
                                        address
                                    );

                                    m_selectedAddresses.clear();
                                }

                                if (ImGui::MenuItem(
                                    "Write Value..."
                                ))
                                {
                                    m_writeAddress =
                                        address;

                                    m_writeValue =
                                        currentValue;

                                    m_showWriteValuePopup =
                                        true;
                                }
                                ImGui::Separator();

                                ImGui::SetNextItemWidth(
                                    220.0f
                                );

                                ImGui::InputText(
                                    "Description",
                                    m_descriptionBuffer,
                                    sizeof(m_descriptionBuffer)
                                );

                                if (ImGui::Button(
                                    "Save Description"
                                ))
                                {
                                    m_pinnedDescriptions[
                                        m_descriptionAddress
                                    ] = m_descriptionBuffer;

                                    savePinnedAddresses();
                                }

                                if (ImGui::MenuItem(
                                    "Copy Address"
                                ))
                                {
                                    char copyText[32];

                                    std::snprintf(
                                        copyText,
                                        sizeof(copyText),
                                        "0x%05zX",
                                        address
                                    );

                                    ImGui::SetClipboardText(
                                        copyText
                                    );
                                }

                                ImGui::Separator();

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
                                            currentValue
                                            )
                                    );

                                    ImGui::SetClipboardText(
                                        valueText
                                    );
                                }

                                if (ImGui::MenuItem(
                                    "Copy All"
                                ))
                                {
                                    char text[128];

                                    std::snprintf(
                                        text,
                                        sizeof(text),
                                        "0x%05zX  Previous: -  Current: %u  Difference: -",
                                        address,
                                        static_cast<unsigned int>(
                                            currentValue
                                            )
                                    );

                                    ImGui::SetClipboardText(
                                        text
                                    );
                                }

                                ImGui::EndPopup();
                            }
                            ImGui::TableSetColumnIndex(1);

                            ImGui::TextUnformatted("-");

                            ImGui::TableSetColumnIndex(2);

                            uint8_t liveCurrentValue =
                                currentValue;

                            m_scanner.readCurrentValue(
                                address,
                                liveCurrentValue
                            );

                            ImGui::Text(
                                "%u",
                                static_cast<unsigned int>(
                                    liveCurrentValue
                                    )
                            );

                            ImGui::TableSetColumnIndex(3);

                            ImGui::TextUnformatted("-");

                            continue;
                        }


                        const DosBoxMemoryCandidate&
                            candidate =
                            m_scanner.
                            candidates()[
                                candidateIndex
                            ];

                        ImGui::TableNextRow();

                        if (hasDescription(
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

                            const bool selected =
                                m_selectedAddresses.contains(
                                    candidate.address
                                );

                            if (ImGui::Selectable(
                                addressText,
                                selected,
                                ImGuiSelectableFlags_SpanAllColumns
                            ))
                            {
                                m_lastSelectedAddress =
                                    candidate.address;

                                m_hasSelectedAddress =
                                    true;

                                if (selected)
                                {
                                    m_selectedAddresses.erase(
                                        candidate.address
                                    );
                                }
                                else
                                {
                                    m_selectedAddresses.insert(
                                        candidate.address
                                    );
                                }
                            }

                            const auto description =
                                m_pinnedDescriptions.find(
                                    candidate.address
                                );

                            if (ImGui::IsItemHovered() &&
                                description !=
                                m_pinnedDescriptions.end() &&
                                !description->second.empty())
                            {
                                ImGui::SetTooltip(
                                    "%s",
                                    description->second.c_str()
                                );
                            }

                            if (ImGui::IsItemClicked(
                                ImGuiMouseButton_Right
                            ))
                            
                            {
                                m_descriptionAddress =
                                    candidate.address;

                                const auto description =
                                    m_pinnedDescriptions.find(
                                        candidate.address
                                    );

                                if (description !=
                                    m_pinnedDescriptions.end())
                                {
                                    std::snprintf(
                                        m_descriptionBuffer,
                                        sizeof(m_descriptionBuffer),
                                        "%s",
                                        description->second.c_str()
                                    );
                                }
                                else
                                {
                                    m_descriptionBuffer[0] =
                                        '\0';
                                }
                            }

                            if (ImGui::BeginPopupContextItem())
                            {

                                if (ImGui::MenuItem(
                                    "Write Value..."
                                ))
                                {
                                    m_writeAddress =
                                        candidate.address;

                                    m_writeValue =
                                        candidate.currentValue;

                                    m_showWriteValuePopup =
                                        true;
                                }

                                ImGui::Separator();

                                if (pinned)
                                {
                                    if (ImGui::MenuItem(
                                        "Unpin Address"
                                    ))
                                    {
                                        m_pinnedAddresses.erase(
                                            candidate.address
                                        );

                                        m_scanner.unpinAddress(
                                            candidate.address
                                        );
                                        savePinnedAddresses();
                                    }
                                }
                                else
                                {
                                    if (ImGui::MenuItem(
                                        "Pin Address"
                                    ))
                                    {
                                        m_pinnedAddresses.insert(
                                            candidate.address
                                        );

                                        m_scanner.pinAddress(
                                            candidate.address
                                        );
                                        savePinnedAddresses();
                                    }
                                }

                                if (pinned)
                                {
                                    ImGui::Separator();

                                    ImGui::SetNextItemWidth(
                                        220.0f
                                    );

                                    ImGui::InputText(
                                        "Description",
                                        m_descriptionBuffer,
                                        sizeof(m_descriptionBuffer)
                                    );

                                    if (ImGui::Button(
                                        "Save Description"
                                    ))
                                    {
                                        m_pinnedDescriptions[
                                            m_descriptionAddress
                                        ] = m_descriptionBuffer;

                                        savePinnedAddresses();
                                    }
                                }

                                ImGui::Separator();

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

                                if (ImGui::MenuItem(
                                    "Copy All"
                                ))
                                {
                                    const int difference =
                                        static_cast<int>(
                                            candidate.currentValue
                                            ) -
                                        static_cast<int>(
                                            candidate.previousValue
                                            );

                                    char text[128];

                                    std::snprintf(
                                        text,
                                        sizeof(text),
                                        "0x%05zX  Previous: %u  Current: %u  Difference: %d",
                                        candidate.address,
                                        static_cast<unsigned int>(
                                            candidate.previousValue
                                            ),
                                        static_cast<unsigned int>(
                                            candidate.currentValue
                                            ),
                                        difference
                                    );

                                    ImGui::SetClipboardText(
                                        text
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

                        uint8_t liveCurrentValue =
                            candidate.currentValue;

                        m_scanner.readCurrentValue(
                            candidate.address,
                            liveCurrentValue
                        );

                        ImGui::Text(
                            "%u",
                            static_cast<unsigned int>(
                                liveCurrentValue
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
            if (m_showWriteValuePopup)
            {
                ImGui::OpenPopup(
                    "Write Memory Value"
                );

                m_showWriteValuePopup = false;
            }

            if (ImGui::BeginPopupModal(
                "Write Memory Value",
                nullptr,
                ImGuiWindowFlags_AlwaysAutoResize
            ))
            {
                ImGui::InputInt(
                    "Value",
                    &m_writeValue
                );

                if (m_writeValue < 0)
                {
                    m_writeValue = 0;
                }

                if (m_writeValue > 255)
                {
                    m_writeValue = 255;
                }


                if (ImGui::Button(
                    "Write"
                ))
                {
                    if (m_scanner.writeValue(
                        m_writeAddress,
                        static_cast<uint8_t>(
                            m_writeValue
                            )
                    ))
                    {
                        m_scanner.refreshValues();
                    }

                    if (m_dosBoxView != nullptr)
                    {
                        m_dosBoxView->requestRefresh();
                    }

                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

        ImGui::End();
    }

    std::string DosBoxMemoryScannerWindow::
        pinnedAddressesFilePath() const
    {
        return
            "settings/memory_pins_" +
            m_gameId +
            ".cfg";
    }

    bool DosBoxMemoryScannerWindow::hasDescription(
        size_t address
    ) const
    {
        const auto it =
            m_pinnedDescriptions.find(
                address
            );

        return
            it != m_pinnedDescriptions.end() &&
            !it->second.empty();
    }
    void DosBoxMemoryScannerWindow::setGameId(
        const std::string& gameId
    )
    {
        if (m_gameId == gameId)
        {
            return;
        }

        m_gameId = gameId;

        m_pinnedAddresses.clear();
        m_pinnedDescriptions.clear();
        m_scanner.clearPinnedAddresses();

        loadPinnedAddresses();
    }

    bool DosBoxMemoryScannerWindow::
        refreshMemory()
    {
        return m_scanner.refreshMemory();
    }

    bool DosBoxMemoryScannerWindow::takeSelectedAddress(
        size_t& address
    )
    {
        if (!m_hasSelectedAddress)
        {
            return false;
        }

        address =
            m_lastSelectedAddress;

        m_hasSelectedAddress =
            false;

        return true;
    }

    DosBoxMemoryScanner&
        DosBoxMemoryScannerWindow::scanner()
    {
        return m_scanner;
    }

    void DosBoxMemoryScannerWindow::
        loadPinnedAddresses()
    {
        std::ifstream file(
            pinnedAddressesFilePath()
        );

        if (!file.is_open())
        {
            return;
        }

        std::string line;

        while (std::getline(
            file,
            line
        ))
        {
            if (line.empty())
            {
                continue;
            }

            const size_t firstSeparator =
                line.find('|');

            if (firstSeparator ==
                std::string::npos)
            {
                continue;
            }

            const size_t secondSeparator =
                line.find(
                    '|',
                    firstSeparator + 1
                );

            if (secondSeparator ==
                std::string::npos)
            {
                continue;
            }

            const std::string addressText =
                line.substr(
                    0,
                    firstSeparator
                );

            const std::string pinnedText =
                line.substr(
                    firstSeparator + 1,
                    secondSeparator -
                    firstSeparator - 1
                );

            const std::string description =
                line.substr(
                    secondSeparator + 1
                );

            const size_t address =
                static_cast<size_t>(
                    std::stoull(
                        addressText,
                        nullptr,
                        0
                    )
                    );

            const bool pinned =
                pinnedText == "1";

            if (pinned)
            {
                m_pinnedAddresses.insert(
                    address
                );

                m_scanner.pinAddress(
                    address
                );
            }

            m_pinnedDescriptions[
                address
            ] = description;
        }
    }
    void DosBoxMemoryScannerWindow::
        savePinnedAddresses() const
    {

        const std::string filename =
            pinnedAddressesFilePath();

        std::ofstream file(
            filename
        );

        if (!file.is_open())
        {
            return;
        }

        for (const auto& entry :
            m_pinnedDescriptions)
        {
            const size_t address =
                entry.first;

            const std::string& description =
                entry.second;

            const bool pinned =
                m_pinnedAddresses.contains(
                    address
                );

            file
                << "0x"
                << std::hex
                << address
                << "|"
                << (pinned ? 1 : 0)
                << "|"
                << description
                << '\n';
        }
    }
}