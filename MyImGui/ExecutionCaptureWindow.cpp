#include "pch.h"
#include "ExecutionCaptureWindow.h"

#include <cstdlib>

#include "imgui.h"

namespace MyImGui
{
    ExecutionCaptureWindow::
        ExecutionCaptureWindow(
            DosBoxMemoryScanner& scanner
        )
        : m_scanner(
            scanner
        )
    {}

    void ExecutionCaptureWindow::draw(
        bool* isOpen
    )
    {
        if (isOpen &&
            !*isOpen)
        {
            return;
        }

        if (!ImGui::Begin(
            "Execution Capture",
            isOpen
        ))
        {
            ImGui::End();
            return;
        }

        ImGui::SetNextItemWidth(
            140.0f
        );

        ImGui::InputText(
            "Execution Target",
            m_targetText,
            sizeof(m_targetText)
        );

        if (ImGui::Button(
            "Set Execution Target"
        ))
        {
            char* end = nullptr;

            const unsigned long long targetAddress =
                std::strtoull(
                    m_targetText,
                    &end,
                    0
                );

            if (end != m_targetText &&
                *end == '\0')
            {
                if (m_scanner.setExecutionCaptureTarget(
                    static_cast<size_t>(
                        targetAddress
                        )
                ))
                {
                    m_captureHit = false;

                    m_capture =
                        RuntimeInstruction{};
                }
            }
        }

        ImGui::SameLine();

        if (ImGui::Button(
            "Stop Execution Capture"
        ))
        {
            if (m_scanner.clearExecutionCapture())
            {
                m_captureHit = false;

                m_capture =
                    RuntimeInstruction{};
            }
        }

        ImGui::SameLine();

        if (ImGui::Button(
            "Get Execution Capture"
        ))
        {
            RuntimeInstruction instruction;

            if (m_scanner.getExecutionCapture(
                instruction
            ))
            {
                m_capture =
                    instruction;

                m_captureHit =
                    true;
            }
        }

        bool executionHit = false;

        if (m_scanner.getExecutionCaptureHit(
            executionHit
        ))
        {
            ImGui::Text(
                "State: %s",
                executionHit
                ? "HIT"
                : "ARMED / NO HIT"
            );
        }
        else
        {
            ImGui::TextDisabled(
                "State unavailable."
            );
        }

        ImGui::Text(
            "Scanner status: %s",
            m_scanner.status().c_str()
        );

        if (m_captureHit)
        {
            ImGui::Separator();

            ImGui::Text(
                "Address: 0x%zX",
                m_capture.address
            );

            ImGui::Text(
                "CS:IP %04X:%04X",
                static_cast<unsigned int>(
                    m_capture.cs
                    ),
                static_cast<unsigned int>(
                    m_capture.ip
                    )
            );

            ImGui::Text(
                "AX=%04X BX=%04X CX=%04X DX=%04X",
                static_cast<unsigned int>(
                    m_capture.registers.ax
                    ),
                static_cast<unsigned int>(
                    m_capture.registers.bx
                    ),
                static_cast<unsigned int>(
                    m_capture.registers.cx
                    ),
                static_cast<unsigned int>(
                    m_capture.registers.dx
                    )
            );

            ImGui::Text(
                "SI=%04X DI=%04X BP=%04X SP=%04X",
                static_cast<unsigned int>(
                    m_capture.registers.si
                    ),
                static_cast<unsigned int>(
                    m_capture.registers.di
                    ),
                static_cast<unsigned int>(
                    m_capture.registers.bp
                    ),
                static_cast<unsigned int>(
                    m_capture.registers.sp
                    )
            );

            ImGui::Text(
                "DS=%04X ES=%04X SS=%04X",
                static_cast<unsigned int>(
                    m_capture.registers.ds
                    ),
                static_cast<unsigned int>(
                    m_capture.registers.es
                    ),
                static_cast<unsigned int>(
                    m_capture.registers.ss
                    )
            );

            ImGui::Separator();

            ImGui::Text(
                "Stack at SS:SP:"
            );

            for (size_t i = 0;
                i < m_capture.stackBytes.size();
                i += 8)
            {
                ImGui::Text(
                    "+%02zX: %02X %02X %02X %02X %02X %02X %02X %02X",
                    i,
                    static_cast<unsigned int>(
                        m_capture.stackBytes[i + 0]
                        ),
                    static_cast<unsigned int>(
                        m_capture.stackBytes[i + 1]
                        ),
                    static_cast<unsigned int>(
                        m_capture.stackBytes[i + 2]
                        ),
                    static_cast<unsigned int>(
                        m_capture.stackBytes[i + 3]
                        ),
                    static_cast<unsigned int>(
                        m_capture.stackBytes[i + 4]
                        ),
                    static_cast<unsigned int>(
                        m_capture.stackBytes[i + 5]
                        ),
                    static_cast<unsigned int>(
                        m_capture.stackBytes[i + 6]
                        ),
                    static_cast<unsigned int>(
                        m_capture.stackBytes[i + 7]
                        )
                );
            }
        }

        ImGui::End();
    }
}