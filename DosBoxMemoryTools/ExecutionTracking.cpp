#include "ExecutionTracking.h"

#include "imgui.h"

#include <cstdlib>

namespace DosBoxMemoryTools
{
    ExecutionTracking::ExecutionTracking(
        MemoryScanner& scanner,
        ScannerAddress& scannerAddress
    )
        :
        m_scanner(
            scanner
        ),
        m_scannerAddress(
            scannerAddress
        )
    {}

    void ExecutionTracking::draw()
    {
        ImGui::Text(
            "Target: 0x%zX",
            m_scannerAddress.value
        );

        if (m_recordButton.draw())
        {
            if (m_recordButton.recording())
            {
                m_scanner.clearExecutionCapture();

                m_hasExecutionCapture = false;
                m_waitingForTrigger = true;
            }
            else
            {
                m_waitingForTrigger = false;
                m_scanner.clearExecutionCapture();
            }
        }

        if (m_waitingForTrigger &&
            ImGui::IsKeyPressed(
                ImGuiKey_Space,
                false
            ))
        {
            if (m_scanner.setExecutionCaptureTarget(
                m_scannerAddress.value
            ))
            {
                m_waitingForTrigger = false;
            }
            else
            {
                m_recordButton.stop();
                m_waitingForTrigger = false;
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

            ImGui::TextUnformatted(
                "Stack at SS:SP:"
            );

            for (size_t i = 0;
                i < m_executionCapture.stackBytes.size();
                i += 8)
            {
                ImGui::Text(
                    "+%02zX: %02X %02X %02X %02X %02X %02X %02X %02X",
                    i,
                    static_cast<unsigned int>(m_executionCapture.stackBytes[i + 0]),
                    static_cast<unsigned int>(m_executionCapture.stackBytes[i + 1]),
                    static_cast<unsigned int>(m_executionCapture.stackBytes[i + 2]),
                    static_cast<unsigned int>(m_executionCapture.stackBytes[i + 3]),
                    static_cast<unsigned int>(m_executionCapture.stackBytes[i + 4]),
                    static_cast<unsigned int>(m_executionCapture.stackBytes[i + 5]),
                    static_cast<unsigned int>(m_executionCapture.stackBytes[i + 6]),
                    static_cast<unsigned int>(m_executionCapture.stackBytes[i + 7])
                );
            }
        }
        ImGui::Text(
            "Scanner status: %s",
            m_scanner.status().c_str()
        );


    }
}