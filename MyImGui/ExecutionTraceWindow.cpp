#include "pch.h"
#include "ExecutionTraceWindow.h"

#include "imgui.h"
#include "Zydis/Zydis.h"

#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <cstring>

namespace MyImGui
{
    ExecutionTraceWindow::
        ExecutionTraceWindow(
            DosBoxMemoryScanner& scanner,
            const std::string& gameId
        )
        : m_scanner(
            scanner
        ),
        m_gameId(
            gameId
        )
    {
        loadSession();
    }

    void ExecutionTraceWindow::draw(
        bool* isOpen
    )
    {
        ZydisDecoder decoder;
        ZydisFormatter formatter;

        const bool zydisReady =
            ZYAN_SUCCESS(
                ZydisDecoderInit(
                    &decoder,
                    ZYDIS_MACHINE_MODE_LEGACY_16,
                    ZYDIS_STACK_WIDTH_16
                )
            ) &&
            ZYAN_SUCCESS(
                ZydisFormatterInit(
                    &formatter,
                    ZYDIS_FORMATTER_STYLE_INTEL
                )
            );

        if (isOpen &&
            !*isOpen)
        {
            return;
        }

        if (!ImGui::Begin(
            "Execution Trace",
            isOpen
        ))
        {
            ImGui::End();
            return;
        }

        ImGui::TextUnformatted(
            "Execution Trace"
        );

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

                const unsigned long long
                    targetAddress =
                    std::strtoull(
                        m_targetText,
                        &end,
                        0
                    );

                if (end != m_targetText &&
                    *end == '\0')
                {
                    m_trace.clear();

                    m_scanner.setReadTraceTarget(
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
            else
            {
                m_scanner.setReadTraceTarget(
                    0
                );
            }
        }

        bool traceActive = false;
        bool traceArmed = false;

        const bool hasTraceActive =
            m_scanner.getReadTraceActive(
                traceActive
            );

        const bool hasTraceArmed =
            m_scanner.getReadTraceArmed(
                traceArmed
            );

        if (hasTraceActive &&
            hasTraceArmed)
        {
            const char* state =
                traceActive
                ? "CAPTURING"
                : traceArmed
                ? "ARMED"
                : "IDLE / COMPLETE";

            ImGui::Text(
                "Trace state: %s",
                state
            );

            if (!traceActive &&
                !traceArmed &&
                m_recordButton.recording())
            {
                loadTrace();

                m_recordButton.stop();
            }
        }

        size_t traceTarget = 0;

        if (m_scanner.getReadTraceTarget(
            traceTarget
        ))
        {
            ImGui::Text(
                "Trace target: 0x%zX",
                traceTarget
            );
        }
        else
        {
            ImGui::TextDisabled(
                "Trace target unavailable."
            );
        }

        ImGui::Text(
            "Captured instructions: %zu",
            m_trace.size()
        );

        for (size_t i = 0;
            i < m_trace.size();
            ++i)
        {
            const RuntimeInstruction&
                instruction =
                m_trace[i];

            char instructionText[256] =
                "<decode failed>";

            ZydisDecodedInstruction
                decodedInstruction;

            ZydisDecodedOperand operands[
                ZYDIS_MAX_OPERAND_COUNT
            ];

            bool decoded = false;

            if (zydisReady &&
                ZYAN_SUCCESS(
                    ZydisDecoderDecodeFull(
                        &decoder,
                        instruction.bytes.data(),
                        instruction.bytes.size(),
                        &decodedInstruction,
                        operands
                    )
                ))
            {
                decoded =
                    ZYAN_SUCCESS(
                        ZydisFormatterFormatInstruction(
                            &formatter,
                            &decodedInstruction,
                            operands,
                            decodedInstruction.
                            operand_count_visible,
                            instructionText,
                            sizeof(instructionText),
                            static_cast<ZyanU64>(
                                instruction.address
                                ),
                            nullptr
                        )
                    );
            }

            ImGui::Separator();

            ImGui::Text(
                "%03zu  0x%zX  CS:IP %04X:%04X",
                i,
                instruction.address,
                static_cast<unsigned int>(
                    instruction.cs
                    ),
                static_cast<unsigned int>(
                    instruction.ip
                    )
            );

            ImGui::SameLine();

            if (decoded)
            {
                ImGui::TextUnformatted(
                    instructionText
                );
            }
            else
            {
                ImGui::TextDisabled(
                    "<decode failed>"
                );
            }

            ImGui::Text(
                "AX=%04X BX=%04X CX=%04X DX=%04X",
                static_cast<unsigned int>(
                    instruction.registers.ax
                    ),
                static_cast<unsigned int>(
                    instruction.registers.bx
                    ),
                static_cast<unsigned int>(
                    instruction.registers.cx
                    ),
                static_cast<unsigned int>(
                    instruction.registers.dx
                    )
            );

            ImGui::Text(
                "SI=%04X DI=%04X BP=%04X SP=%04X",
                static_cast<unsigned int>(
                    instruction.registers.si
                    ),
                static_cast<unsigned int>(
                    instruction.registers.di
                    ),
                static_cast<unsigned int>(
                    instruction.registers.bp
                    ),
                static_cast<unsigned int>(
                    instruction.registers.sp
                    )
            );

            ImGui::Text(
                "DS=%04X ES=%04X SS=%04X",
                static_cast<unsigned int>(
                    instruction.registers.ds
                    ),
                static_cast<unsigned int>(
                    instruction.registers.es
                    ),
                static_cast<unsigned int>(
                    instruction.registers.ss
                    )
            );
        }

        ImGui::End();
    }

    void ExecutionTraceWindow::saveSession() const
    {
        if (m_gameId.empty())
        {
            return;
        }

        std::filesystem::create_directories(
            "settings"
        );

        const std::string filename =
            "settings/execution_trace_session_" +
            m_gameId +
            ".cfg";

        std::ofstream file(
            filename
        );

        if (!file)
        {
            return;
        }

        file <<
            "ExecutionTraceSession 1\n";

        file <<
            "Target\n";

        file <<
            m_targetText <<
            '\n';
    }

    void ExecutionTraceWindow::loadSession()
    {
        if (m_gameId.empty())
        {
            return;
        }

        const std::string filename =
            "settings/execution_trace_session_" +
            m_gameId +
            ".cfg";

        std::ifstream file(
            filename
        );

        if (!file)
        {
            return;
        }

        std::string header;

        std::getline(
            file,
            header
        );

        if (header !=
            "ExecutionTraceSession 1")
        {
            return;
        }

        std::string section;

        if (!(file >> section) ||
            section != "Target")
        {
            return;
        }

        std::string target;

        if (!(file >> target))
        {
            return;
        }

        strncpy_s(
            m_targetText,
            sizeof(m_targetText),
            target.c_str(),
            _TRUNCATE
        );
    }

    void ExecutionTraceWindow::setGameId(
        const std::string& gameId
    )
    {
        if (m_gameId == gameId)
        {
            return;
        }

        m_gameId =
            gameId;

        loadSession();

    }

    void ExecutionTraceWindow::loadTrace()
    {
        size_t count = 0;

        if (!m_scanner.getReadTraceCount(
            count
        ))
        {
            return;
        }

        m_trace.clear();

        m_trace.reserve(
            count
        );

        for (size_t i = 0;
            i < count;
            ++i)
        {
            RuntimeInstruction instruction;

            if (!m_scanner.getReadTraceInstruction(
                i,
                instruction
            ))
            {
                break;
            }

            m_trace.push_back(
                instruction
            );
        }
    }
}
