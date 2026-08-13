#include "pch.h"
#include "DosBoxMemoryScannerWindow.h"

#include <cstdio>

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

        if (ImGui::Button(
            "New Scan"
        ))
        {
            m_scanner.scan(
                DosBoxMemoryScanMode::NewScan
            );
        }

        ImGui::SameLine();

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

        if (m_scanMode ==
            DosBoxMemoryScanMode::ExactValue)
        {
            ImGui::SameLine();

            ImGui::SetNextItemWidth(
                80.0f
            );

            ImGui::InputInt(
                "##ExactValue",
                &m_exactValue
            );

            if (m_exactValue < 0)
            {
                m_exactValue = 0;
            }

            if (m_exactValue > 255)
            {
                m_exactValue = 255;
            }
        }

        ImGui::SameLine();

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

        ImGui::SameLine();

        if (ImGui::Button(
            "Reset"
        ))
        {
            m_scanner.reset();
        }

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

            ImGuiListClipper clipper;

            clipper.Begin(
                static_cast<int>(
                    m_scanner.
                    candidates().
                    size()
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
                    const DosBoxMemoryCandidate&
                        candidate =
                        m_scanner.
                        candidates()[
                            index
                        ];

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);

                    ImGui::Text(
                        "0x%05zX",
                        candidate.address
                    );

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