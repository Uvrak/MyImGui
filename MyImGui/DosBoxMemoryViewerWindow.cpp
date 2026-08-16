#include "pch.h"
#include "DosBoxMemoryViewerWindow.h"

#include <cstdio>
#include <cstdlib>

#include "imgui.h"

namespace MyImGui
{
    DosBoxMemoryViewerWindow::
        DosBoxMemoryViewerWindow(
            DosBoxMemoryReader& memoryReader
        )
        : m_memoryReader(
            memoryReader
        ),
        m_memoryScanner(
            memoryReader
        )
    {}

    void DosBoxMemoryViewerWindow::draw(
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
            "DOSBox Memory Viewer",
            isOpen
        ))
        {
            ImGui::End();
            return;
        }

        const auto& memory =
            m_memoryReader.memory();

        ImGui::Text(
            "Snapshot ID: %llu",
            static_cast<unsigned long long>(
                m_memoryReader.snapshotId()
                )
        );

        ImGui::Text(
            "Memory size: %zu bytes",
            memory.size()
        );

        ImGui::InputText(
            "Search",
            m_searchText,
            sizeof(m_searchText)
        );

        if (ImGui::IsItemActive())
        {
            m_memoryViewActive = false;
        }

        ImGui::SameLine();

        if (ImGui::Button(
            "Search##MemorySearchButton"
        ))
        {
            m_hasSearchResult = false;

            const std::string searchText =
                m_searchText;

            if (!searchText.empty())
            {
                for (size_t address = 0;
                    address + searchText.size() <= memory.size();
                    ++address)
                {
                    bool match = true;

                    for (size_t i = 0;
                        i < searchText.size();
                        ++i)
                    {
                        if (memory[address + i] !=
                            static_cast<uint8_t>(
                                searchText[i]
                                ))
                        {
                            match = false;
                            break;
                        }
                    }

                    if (match)
                    {
                        m_searchResult = address;
                        m_hasSearchResult = true;
                        m_scrollToSearchResult = true;
                        break;
                    }
                }
            }
        }

        ImGui::InputText(
            "Address",
            m_addressText,
            sizeof(m_addressText)
        );

        if (ImGui::IsItemActive())
        {
            m_memoryViewActive = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("Go"))
        {
            char* end = nullptr;

            const unsigned long long address =
                std::strtoull(
                    m_addressText,
                    &end,
                    0
                );

            if (end != m_addressText &&
                *end == '\0' &&
                address < memory.size())
            {
                m_searchResult =
                    static_cast<size_t>(
                        address
                        );

                m_hasSearchResult = true;
                m_scrollToSearchResult = true;
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Refresh"))
        {
            m_memoryScanner.refreshMemory();
        }

        ImGui::SameLine();

        ImGui::Checkbox(
            "Live",
            &liveView
        );

        if (m_hasSearchResult)
        {
            ImGui::Text(
                "Found at: 0x%05zX",
                m_searchResult
            );
        }

        if (m_hasSelectedAddress)
        {
            ImGui::Text(
                "Selected: 0x%05zX",
                m_selectedAddress
            );
        }

        ImGui::Separator();

        constexpr size_t BytesPerRow = 16;

        const int rowCount =
            static_cast<int>(
                (memory.size() + BytesPerRow - 1) /
                BytesPerRow
                );

        if (ImGui::BeginTable(
            "##MemoryViewer",
            18,
            ImGuiTableFlags_Borders |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollY,
            ImVec2(
                0.0f,
                0.0f
            )
        ))
        {
            ImGui::TableSetupColumn(
                "Address"
            );

            for (int column = 0;
                column < 16;
                ++column)
            {
                char label[3];

                std::snprintf(
                    label,
                    sizeof(label),
                    "%02X",
                    column
                );

                ImGui::TableSetupColumn(
                    label
                );
            }

            

            ImGui::TableSetupColumn(
                "ASCII",
                ImGuiTableColumnFlags_WidthFixed,
                150.0f
            );
            

            ImGui::TableHeadersRow();

            if (m_scrollToSearchResult)
            {
                const float rowHeight =
                    ImGui::GetTextLineHeightWithSpacing();

                const float targetY =
                    static_cast<float>(
                        m_searchResult /
                        BytesPerRow
                        ) * rowHeight;

                ImGui::SetScrollY(
                    targetY
                );

                m_scrollToSearchResult =
                    false;
            }

            ImGuiListClipper clipper;

            clipper.Begin(
                rowCount
            );

            while (clipper.Step())
            {
                for (int row =
                    clipper.DisplayStart;
                    row < clipper.DisplayEnd;
                    ++row)
                {
                    const size_t address =
                        static_cast<size_t>(row) *
                        BytesPerRow;

                    ImGui::TableNextRow();

                    const size_t selectedRow =
                        m_selectedAddress /
                        BytesPerRow;

                    if (m_keepSelectedVisible &&
                        m_hasSelectedAddress &&
                        static_cast<size_t>(row) == selectedRow)
                    {
                        ImGui::SetScrollHereY(
                            0.5f
                        );

                        m_keepSelectedVisible =
                            false;
                    }

                    const size_t activeRow =
                        m_searchResult /
                        BytesPerRow;

                    if (m_hasSearchResult &&
                        static_cast<size_t>(row) == activeRow)
                    {
                        ImGui::TableSetBgColor(
                            ImGuiTableBgTarget_RowBg0,
                            ImGui::GetColorU32(
                                ImVec4(
                                    0.25f,
                                    0.25f,
                                    0.10f,
                                    1.0f
                                )
                            )
                        );
                    }

                    ImGui::TableSetColumnIndex(0);

                    ImGui::Text(
                        "%05zX",
                        address
                    );

                    for (size_t column = 0;
                        column < BytesPerRow;
                        ++column)
                    {
                        const size_t byteAddress =
                            address + column;

                        ImGui::TableSetColumnIndex(
                            static_cast<int>(
                                column + 1
                                )
                        );

                        if (byteAddress <
                            memory.size())
                        {
                            char byteText[3];

                            std::snprintf(
                                byteText,
                                sizeof(byteText),
                                "%02X",
                                static_cast<unsigned int>(
                                    memory[byteAddress]
                                    )
                            );

                            const bool selected =
                                m_hasSelectedAddress &&
                                m_selectedAddress == byteAddress;

                            if (selected)
                            {
                                ImGui::TableSetBgColor(
                                    ImGuiTableBgTarget_CellBg,
                                    ImGui::GetColorU32(
                                        ImGuiCol_Header
                                    )
                                );
                            }

                            ImGui::PushID(
                                static_cast<int>(byteAddress)
                            );

                            ImGui::TextUnformatted(
                                byteText
                            );

                            if (ImGui::IsItemClicked(
                                ImGuiMouseButton_Left
                            ))
                            {
                                m_selectedAddress =
                                    byteAddress;

                                m_hasSelectedAddress =
                                    true;

                                m_memoryViewActive =
                                    true;
                            }

                            ImGui::PopID();
                        }
                    }
                    
                    ImGui::TableSetColumnIndex(
                        17
                    );

                    char ascii[17] = {};

                    for (size_t column = 0;
                        column < BytesPerRow;
                        ++column)
                    {
                        const size_t byteAddress =
                            address + column;

                        if (byteAddress >=
                            memory.size())
                        {
                            break;
                        }

                        const uint8_t value =
                            memory[byteAddress];

                        ascii[column] =
                            (value >= 32 &&
                                value <= 126)
                            ? static_cast<char>(value)
                            : '.';
                    }

                    ImGui::TextUnformatted(
                        ascii
                    );
                }
            }

            ImGui::EndTable();

            if (m_memoryViewActive &&
                m_hasSelectedAddress &&
                ImGui::IsWindowFocused(
                    ImGuiFocusedFlags_RootAndChildWindows
                ))
            {
                if (ImGui::IsKeyPressed(
                    ImGuiKey_LeftArrow,
                    false
                ))
                {
                    if (m_selectedAddress > 0)
                    {
                        --m_selectedAddress;
                        m_keepSelectedVisible = true;
                    }
                }

                if (ImGui::IsKeyPressed(
                    ImGuiKey_RightArrow,
                    false
                ))
                {
                    if (m_selectedAddress + 1 <
                        memory.size())
                    {
                        ++m_selectedAddress;
                        m_keepSelectedVisible = true;
                    }
                }

                if (ImGui::IsKeyPressed(
                    ImGuiKey_UpArrow,
                    false
                ))
                {
                    if (m_selectedAddress >= 16)
                    {
                        m_selectedAddress -= 16;
                        m_keepSelectedVisible = true;
                    }
                }

                if (ImGui::IsKeyPressed(
                    ImGuiKey_DownArrow,
                    false
                ))
                {
                    if (m_selectedAddress + 16 <
                        memory.size())
                    {
                        m_selectedAddress += 16;
                        m_keepSelectedVisible = true;
                    }
                }
            }
                    
        }

        ImGui::End();
    }

    void DosBoxMemoryViewerWindow::goToAddress(
        size_t address
    )
    {
        m_searchResult = address;
        m_hasSearchResult = true;
        m_scrollToSearchResult = true;
    }
}
