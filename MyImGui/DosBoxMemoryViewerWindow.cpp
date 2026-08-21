#include "pch.h"
#include "DosBoxMemoryViewerWindow.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>

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

        const char* searchTypes[] =
        {
            "String",
            "Byte Pattern"
        };

        int selectedSearchType =
            static_cast<int>(
                m_searchType
                );

        ImGui::SetNextItemWidth(
            120.0f
        );

        if (ImGui::Combo(
            "##SearchType",
            &selectedSearchType,
            searchTypes,
            IM_ARRAYSIZE(searchTypes)
        ))
        {
            m_searchType =
                static_cast<MemorySearchType>(
                    selectedSearchType
                    );
        }

        ImGui::SameLine();

        ImGui::InputText(
            "##Search",
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
            m_searchPerformed = true;

            std::vector<int> pattern;

            if (m_searchType ==
                MemorySearchType::String)
            {
                const std::string searchText =
                    m_searchText;

                for (char character : searchText)
                {
                    pattern.push_back(
                        static_cast<uint8_t>(
                            character
                            )
                    );
                }
            }
            else
            {
                std::istringstream stream(
                    m_searchText
                );

                std::string byteText;

                while (stream >> byteText)
                {
                    if (byteText == "??")
                    {
                        pattern.push_back(-1);
                        continue;
                    }

                    if (byteText.starts_with("??{") &&
                        byteText.ends_with("}"))
                    {
                        const std::string countText =
                            byteText.substr(
                                3,
                                byteText.size() - 4
                            );

                        if (countText == "N" ||
                            countText == "n")
                        {
                            for (int i = 0;
                                i < m_patternN;
                                ++i)
                            {
                                pattern.push_back(-1);
                            }

                            continue;
                        }

                        char* end = nullptr;

                        const unsigned long count =
                            std::strtoul(
                                countText.c_str(),
                                &end,
                                10
                            );

                        if (end == countText.c_str() ||
                            *end != '\0' )
                        {
                            pattern.clear();
                            break;
                        }

                        for (unsigned long i = 0;
                            i < count;
                            ++i)
                        {
                            pattern.push_back(-1);
                        }

                        continue;
                    }
                    char* end = nullptr;

                    const unsigned long value =
                        std::strtoul(
                            byteText.c_str(),
                            &end,
                            16
                        );

                    if (end == byteText.c_str() ||
                        *end != '\0' ||
                        value > 0xFF)
                    {
                        pattern.clear();
                        break;
                    }

                    pattern.push_back(
                        static_cast<uint8_t>(
                            value
                            )
                    );
                }
            }

            if (!pattern.empty())
            {
                for (size_t address = 0;
                    address + pattern.size() <= memory.size();
                    ++address)
                {
                    bool match = true;

                    for (size_t i = 0;
                        i < pattern.size();
                        ++i)
                    {
                        if (pattern[i] != -1 &&
                            memory[address + i] !=
                            static_cast<uint8_t>(
                                pattern[i]
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

        if (m_searchType ==
            MemorySearchType::BytePattern)
        {
            constexpr float PatternInputWidth =
                70.0f;

            ImGui::TextUnformatted("N");
            ImGui::SameLine();

            ImGui::SetNextItemWidth(80.0f);
            ImGui::InputInt(
                "##PatternN",
                &m_patternN
            );

            ImGui::SameLine();
            ImGui::TextUnformatted("From");
            ImGui::SameLine();

            ImGui::SetNextItemWidth(100.0f);
            ImGui::InputInt(
                "##PatternFrom",
                &m_patternFrom
            );

            ImGui::SameLine();
            ImGui::TextUnformatted("To");
            ImGui::SameLine();

            ImGui::SetNextItemWidth(100.0f);
            ImGui::InputInt(
                "##PatternTo",
                &m_patternTo
            );

            ImGui::SameLine();

            if (ImGui::Button(
                "Search Range##PatternRange"
            ))
            {
                m_hasSearchResult = false;
                m_searchPerformed = true;

                for (int n = m_patternFrom;
                    n <= m_patternTo;
                    ++n)
                {
                    size_t resultAddress = 0;

                    if (searchBytePattern(
                        n,
                        resultAddress
                    ))
                    {
                        m_patternN = n;

                        m_searchResult =
                            resultAddress;

                        m_hasSearchResult =
                            true;

                        m_scrollToSearchResult =
                            true;

                        break;
                    }
                }
            }

            if (m_patternN < 0)
            {
                m_patternN = 0;
            }

            if (m_patternFrom < 0)
            {
                m_patternFrom = 0;
            }

            if (m_patternTo < m_patternFrom)
            {
                m_patternTo = m_patternFrom;
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

        if (ImGui::IsItemDeactivated())
        {
            m_memoryViewActive = true;
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
                goToAddress(
                    static_cast<size_t>(
                        address
                        )
                );
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
        else if (m_searchPerformed)
        {
            ImGui::Text("Not found");
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
                "Address",
                ImGuiTableColumnFlags_WidthFixed,
                75.0f
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
            
            ImGui::TableSetupScrollFreeze(
                0,
                1
            );

            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;

            

            clipper.Begin(
                rowCount
            );

            if (m_scrollToSearchResult)
            {
                const int targetRow =
                    static_cast<int>(
                        m_searchResult /
                        BytesPerRow
                        );

                const float scrollMaxY =
                    ImGui::GetScrollMaxY();

                if (rowCount > 1 &&
                    scrollMaxY > 0.0f)
                {
                    const float fraction =
                        static_cast<float>(
                            targetRow
                            ) /
                        static_cast<float>(
                            rowCount - 1
                            );

                    const float visibleHeight =
                        ImGui::GetWindowHeight();

                    float targetY =
                        fraction * scrollMaxY +
                        (fraction - 0.5f) *
                        visibleHeight;

                    if (targetY < 0.0f)
                    {
                        targetY = 0.0f;
                    }

                    if (targetY > scrollMaxY)
                    {
                        targetY = scrollMaxY;
                    }

                    ImGui::SetScrollY(
                        targetY
                    );
                }

                m_scrollToSearchResult = false;
            }

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

                    if (m_hasSelectedAddress &&
                        static_cast<size_t>(row) == selectedRow)
                    {
                        ImGui::TableSetBgColor(
                            ImGuiTableBgTarget_RowBg0,
                            ImGui::GetColorU32(
                                ImGuiCol_HeaderHovered
                            )
                        );
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
                                        ImVec4(
                                            1.0f,
                                            1.0f,
                                            1.0f,
                                            0.65f
                                        )
                                    )
                                );
                            }

                            ImGui::PushID(
                                static_cast<int>(
                                    byteAddress
                                    )
                            );

                            if (ImGui::Selectable(
                                byteText,
                                selected
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

            if (m_hasSelectedAddress &&
                ImGui::IsWindowFocused(
                    ImGuiFocusedFlags_RootAndChildWindows
                ) &&
                !ImGui::GetIO().WantTextInput)
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
        m_selectedAddress = address;
        m_hasSelectedAddress = true;

        m_searchResult = address;
        m_hasSearchResult = true;
        m_scrollToSearchResult = true;
    }
    
    bool DosBoxMemoryViewerWindow::searchBytePattern(
        int patternN,
        size_t& resultAddress
    )
    {
        std::vector<int> pattern;

        std::istringstream stream(
            m_searchText
        );

        std::string byteText;

        while (stream >> byteText)
        {
            if (byteText == "??")
            {
                pattern.push_back(-1);
                continue;
            }

            if (byteText.starts_with("??{") &&
                byteText.ends_with("}"))
            {
                const std::string countText =
                    byteText.substr(
                        3,
                        byteText.size() - 4
                    );

                if (countText == "N" ||
                    countText == "n")
                {
                    for (int i = 0;
                        i < patternN;
                        ++i)
                    {
                        pattern.push_back(-1);
                    }

                    continue;
                }

                char* end = nullptr;

                const unsigned long count =
                    std::strtoul(
                        countText.c_str(),
                        &end,
                        10
                    );

                if (end == countText.c_str() ||
                    *end != '\0')
                {
                    return false;
                }

                for (unsigned long i = 0;
                    i < count;
                    ++i)
                {
                    pattern.push_back(-1);
                }

                continue;
            }

            char* end = nullptr;

            const unsigned long value =
                std::strtoul(
                    byteText.c_str(),
                    &end,
                    16
                );

            if (end == byteText.c_str() ||
                *end != '\0' ||
                value > 0xFF)
            {
                return false;
            }

            pattern.push_back(
                static_cast<uint8_t>(value)
            );
        }

        if (pattern.empty())
        {
            return false;
        }

        const auto& memory =
            m_memoryReader.memory();

        for (size_t address = 0;
            address + pattern.size() <= memory.size();
            ++address)
        {
            bool match = true;

            for (size_t i = 0;
                i < pattern.size();
                ++i)
            {
                if (pattern[i] != -1 &&
                    memory[address + i] !=
                    static_cast<uint8_t>(
                        pattern[i]
                        ))
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                resultAddress = address;
                return true;
            }
        }

        return false;
    }
}
