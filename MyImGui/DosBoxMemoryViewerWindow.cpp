#include "pch.h"
#include "DosBoxMemoryViewerWindow.h"

#include <cstdio>

#include "imgui.h"

namespace MyImGui
{
    DosBoxMemoryViewerWindow::
        DosBoxMemoryViewerWindow(
            DosBoxMemoryReader& memoryReader
        )
        : m_memoryReader(
            memoryReader
        )
    {}

    void DosBoxMemoryViewerWindow::draw(
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
                            ImGui::Text(
                                "%02X",
                                static_cast<unsigned int>(
                                    memory[byteAddress]
                                    )
                            );
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
        }

        ImGui::End();
    }
}
