#include "TraceRecordView.h"

#include "imgui.h"
#include <Zydis/Zydis.h>

namespace DosBoxMemoryTools
{
    void TraceRecordView::draw(
        size_t index,
        const RuntimeInstruction& instruction,
        const TraceInstructionDifference& difference,
        bool selected
    )
    {
        ImVec4 differenceColor(
            1.0f,
            1.0f,
            0.0f,
            1.0f
        );

        if (selected)
        {
            const ImVec2 min =
                ImGui::GetCursorScreenPos();

            const float height =
                ImGui::GetTextLineHeightWithSpacing() *
                5.0f;

            const ImVec2 bottomRight(
                min.x + ImGui::GetContentRegionAvail().x,
                min.y + height
            );

            ImGui::GetWindowDrawList()->AddRectFilled(
                min,
                bottomRight,
                IM_COL32(
                    80,
                    80,
                    80,
                    100
                )
            );
        }

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
                        decodedInstruction.operand_count_visible,
                        instructionText,
                        sizeof(instructionText),
                        static_cast<ZyanU64>(
                            instruction.address
                            ),
                        nullptr
                    )
                );
        }

        ImGui::Text(
            "%03zu",
            index + 1
        );

        ImGui::SameLine();

        if (difference.address)
        {
            ImGui::TextColored(
                differenceColor,
                "0x%zX",
                instruction.address
            );
        }
        else
        {
            ImGui::Text(
                "0x%zX",
                instruction.address
            );
        }

        const size_t byteCount =
            decoded
            ? static_cast<size_t>(
                decodedInstruction.length
                )
            : instruction.bytes.size();

        for (size_t byteIndex = 0;
            byteIndex < byteCount;
            ++byteIndex)
        {
            ImGui::SameLine();

            ImGui::Text(
                "%02X",
                static_cast<unsigned int>(
                    instruction.bytes[byteIndex]
                    )
            );
        }

        ImGui::Text(
            "CS:IP %04X:%04X",
            static_cast<unsigned int>(instruction.cs),
            static_cast<unsigned int>(instruction.ip)
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

        if (difference.ax)
        {
            ImGui::TextColored(
                differenceColor,
                "AX=%04X",
                static_cast<unsigned int>(
                    instruction.registers.ax
                    )
            );
        }
        else
        {
            ImGui::Text(
                "AX=%04X",
                static_cast<unsigned int>(
                    instruction.registers.ax
                    )
            );
        }

        ImGui::SameLine();

        if (difference.bx)
        {
            ImGui::TextColored(
                differenceColor,
                "BX=%04X",
                static_cast<unsigned int>(
                    instruction.registers.bx
                    )
            );
        }
        else
        {
            ImGui::Text(
                "BX=%04X",
                static_cast<unsigned int>(
                    instruction.registers.bx
                    )
            );
        }

        ImGui::SameLine();

        if (difference.cx)
        {
            ImGui::TextColored(
                differenceColor,
                "CX=%04X",
                static_cast<unsigned int>(
                    instruction.registers.cx
                    )
            );
        }
        else
        {
            ImGui::Text(
                "CX=%04X",
                static_cast<unsigned int>(
                    instruction.registers.cx
                    )
            );
        }

        ImGui::SameLine();

        if (difference.dx)
        {
            ImGui::TextColored(
                differenceColor,
                "DX=%04X",
                static_cast<unsigned int>(
                    instruction.registers.dx
                    )
            );
        }
        else
        {
            ImGui::Text(
                "DX=%04X",
                static_cast<unsigned int>(
                    instruction.registers.dx
                    )
            );
        }

        if (difference.si)
        {
            ImGui::TextColored(
                differenceColor,
                "SI=%04X",
                static_cast<unsigned int>(
                    instruction.registers.si
                    )
            );
        }
        else
        {
            ImGui::Text(
                "SI=%04X",
                static_cast<unsigned int>(
                    instruction.registers.si
                    )
            );
        }

        ImGui::SameLine();

        if (difference.di)
        {
            ImGui::TextColored(
                differenceColor,
                "DI=%04X",
                static_cast<unsigned int>(
                    instruction.registers.di
                    )
            );
        }
        else
        {
            ImGui::Text(
                "DI=%04X",
                static_cast<unsigned int>(
                    instruction.registers.di
                    )
            );
        }

        ImGui::SameLine();

        if (difference.bp)
        {
            ImGui::TextColored(
                differenceColor,
                "BP=%04X",
                static_cast<unsigned int>(
                    instruction.registers.bp
                    )
            );
        }
        else
        {
            ImGui::Text(
                "BP=%04X",
                static_cast<unsigned int>(
                    instruction.registers.bp
                    )
            );
        }

        ImGui::SameLine();

        if (difference.sp)
        {
            ImGui::TextColored(
                differenceColor,
                "SP=%04X",
                static_cast<unsigned int>(
                    instruction.registers.sp
                    )
            );
        }
        else
        {
            ImGui::Text(
                "SP=%04X",
                static_cast<unsigned int>(
                    instruction.registers.sp
                    )
            );
        }

        if (difference.ds)
        {
            ImGui::TextColored(
                differenceColor,
                "DS=%04X",
                static_cast<unsigned int>(
                    instruction.registers.ds
                    )
            );
        }
        else
        {
            ImGui::Text(
                "DS=%04X",
                static_cast<unsigned int>(
                    instruction.registers.ds
                    )
            );
        }

        ImGui::SameLine();

        if (difference.es)
        {
            ImGui::TextColored(
                differenceColor,
                "ES=%04X",
                static_cast<unsigned int>(
                    instruction.registers.es
                    )
            );
        }
        else
        {
            ImGui::Text(
                "ES=%04X",
                static_cast<unsigned int>(
                    instruction.registers.es
                    )
            );
        }

        ImGui::SameLine();

        if (difference.ss)
        {
            ImGui::TextColored(
                differenceColor,
                "SS=%04X",
                static_cast<unsigned int>(
                    instruction.registers.ss
                    )
            );
        }
        else
        {
            ImGui::Text(
                "SS=%04X",
                static_cast<unsigned int>(
                    instruction.registers.ss
                    )
            );
        }
    }
}