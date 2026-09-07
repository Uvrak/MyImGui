#include "ExecutionTracking.h"

#include "imgui.h"

#include <cstdlib>

namespace DosBoxMemoryTools
{
    ExecutionTracking::ExecutionTracking(
        MemoryScanner& scanner
    )
        :
        m_scanner(
            scanner
        )
    {

    }

    void ExecutionTracking::draw()
    {
        ImGui::SetNextItemWidth(
            120.0f
        );

        ImGui::InputText(
            "Target",
            m_targetText,
            sizeof(m_targetText)
        );

        if (m_recordButton.draw())
        {
            if (m_recordButton.recording())
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
                    m_scanner.clearExecutionCapture();

                    m_hasExecutionCapture =
                        false;

                    m_scanner.setExecutionCaptureTarget(
                        static_cast<size_t>(
                            targetAddress
                            )
                    );
                }
                else
                {
                    m_recordButton.stop();
                }
            }
        }

        bool executionHit = false;

        m_scanner.getExecutionCaptureHit(
            executionHit
        );

        if (executionHit &&
            m_recordButton.recording())
        {
            if (m_scanner.getExecutionCapture(
                m_executionCapture
            ))
            {
                m_hasExecutionCapture =
                    true;
            }

            m_recordButton.stop();
        }

        ImGui::Text(
            "State: %s",
            executionHit
            ? "HIT"
            : m_recordButton.recording()
            ? "ARMED / NO HIT"
            : "IDLE"
        );

        ImGui::Text(
            "Records: %zu",
            m_hasExecutionCapture
            ? static_cast<size_t>(1)
            : static_cast<size_t>(0)
        );

        if (m_hasExecutionCapture)
        {
            ImGui::Separator();

            ImGui::Text(
                "Address: 0x%zX",
                m_executionCapture.address
            );

            ImGui::Text(
                "CS:IP %04X:%04X",
                static_cast<unsigned int>(
                    m_executionCapture.cs
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.ip
                    )
            );

            ImGui::Text(
                "AX=%04X BX=%04X CX=%04X DX=%04X",
                static_cast<unsigned int>(
                    m_executionCapture.registers.ax
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.registers.bx
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.registers.cx
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.registers.dx
                    )
            );

            ImGui::Text(
                "SI=%04X DI=%04X BP=%04X SP=%04X",
                static_cast<unsigned int>(
                    m_executionCapture.registers.si
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.registers.di
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.registers.bp
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.registers.sp
                    )
            );

            ImGui::Text(
                "DS=%04X ES=%04X SS=%04X",
                static_cast<unsigned int>(
                    m_executionCapture.registers.ds
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.registers.es
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.registers.ss
                    )
            );

            ImGui::TextUnformatted(
                "Bytes:"
            );

            for (size_t i = 0;
                i < m_executionCapture.bytes.size();
                ++i)
            {
                ImGui::SameLine();

                ImGui::Text(
                    "%02X",
                    static_cast<unsigned int>(
                        m_executionCapture.bytes[i]
                        )
                );
            }
        }
        ImGui::Text(
            "Scanner status: %s",
            m_scanner.status().c_str()
        );
    }
}