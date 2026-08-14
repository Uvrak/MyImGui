#include "pch.h"
#include "DosBoxMemoryViewerWindow.h"

#include <algorithm>

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

        ImGuiListClipper clipper;

        clipper.Begin(
            rowCount
        );

        while (clipper.Step())
        {
            for (int row =
                clipper.DisplayStart;
                row <
                clipper.DisplayEnd;
                ++row)
            {
                const size_t address =
                    static_cast<size_t>(row) *
                    BytesPerRow;

                ImGui::Text(
                    "%05zX",
                    address
                );

                ImGui::SameLine();

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

                    ImGui::SameLine();

                    ImGui::Text(
                        "%02X",
                        static_cast<unsigned int>(
                            memory[byteAddress]
                            )
                    );
                }
            }
        }

        ImGui::End();
    }
}
