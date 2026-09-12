#include "DisassemblyCallerSearch.h"

#include <cstdio>
#include <cstdlib>
#include <Zydis/Zydis.h>
#include "imgui.h"

namespace DosBoxMemoryTools
{
    void DisassemblyCallerSearch::drawControls(DisassemblyState& state)
    {
        const auto& memory = state.m_disassemblyMemory;
        ImGui::SetNextItemWidth(
            120.0f
        );

        ImGui::InputText(
            "Target##CallerTarget",
            state.m_callerTargetText,
            sizeof(state.m_callerTargetText)
        );

        ImGui::SameLine();

        if (ImGui::Button(
            "Find Callers"
        ))
        {
            state.m_callers.clear();
            state.m_callerSearchPerformed = true;

            char* end = nullptr;

            const unsigned long long target =
                std::strtoull(
                    state.m_callerTargetText,
                    &end,
                    0
                );

            if (end != state.m_callerTargetText &&
                *end == '\0' &&
                target < memory.size())
            {
                ZydisDecoder decoder;

                if (ZYAN_SUCCESS(
                    ZydisDecoderInit(
                        &decoder,
                        ZYDIS_MACHINE_MODE_LEGACY_16,
                        ZYDIS_STACK_WIDTH_16
                    )))
                {
                    for (size_t address = 0;
                        address < memory.size();
                        ++address)
                    {
                        ZydisDecodedInstruction
                            instruction;

                        ZydisDecodedOperand operands[
                            ZYDIS_MAX_OPERAND_COUNT
                        ];

                        const size_t bytesAvailable =
                            memory.size() - address;

                        if (!ZYAN_SUCCESS(
                            ZydisDecoderDecodeFull(
                                &decoder,
                                memory.data() + address,
                                bytesAvailable,
                                &instruction,
                                operands
                            )))
                        {
                            continue;
                        }

                        if (instruction.mnemonic !=
                        ZYDIS_MNEMONIC_CALL)
                        {
                            continue;
                        }

                        for (ZyanU8 operandIndex = 0;
                            operandIndex <
                            instruction.operand_count_visible;
                            ++operandIndex)
                        {
                            const auto& operand =
                                operands[operandIndex];

                            if (operand.type ==
                                ZYDIS_OPERAND_TYPE_POINTER)
                            {
                                const size_t segment =
                                    static_cast<size_t>(
                                        operand.ptr.segment
                                        );

                                const size_t offset =
                                    static_cast<size_t>(
                                        operand.ptr.offset
                                        );

                                const size_t physicalAddress =
                                    (segment << 4) +
                                    offset;

                                if (physicalAddress ==
                                    static_cast<size_t>(
                                        target
                                        ))
                                {
                                    state.m_callers.push_back(
                                        address
                                    );

                                    break;
                                }

                                continue;
                            }

                            if (operand.type !=
                                ZYDIS_OPERAND_TYPE_IMMEDIATE ||
                                !operand.imm.is_relative)
                            {
                                continue;
                            }

                            ZyanU64 absoluteAddress = 0;

                            if (!ZYAN_SUCCESS(
                                ZydisCalcAbsoluteAddress(
                                    &instruction,
                                    &operand,
                                    static_cast<ZyanU64>(
                                        address
                                        ),
                                    &absoluteAddress
                                )))
                            {
                                continue;
                            }

                            if (absoluteAddress ==
                                static_cast<ZyanU64>(
                                    target
                                    ))
                            {
                                state.m_callers.push_back(
                                    address
                                );

                                break;
                            }
                        }

                    }
                }
            }
        }

    }

    void DisassemblyCallerSearch::drawResults(DisassemblyState& state, const DisassemblyNavigate& goToAddress)
    {
        if (state.m_callerSearchPerformed)
        {
            if (state.m_callers.empty())
            {
                ImGui::TextUnformatted(
                    "No callers found."
                );
            }
            else
            {
                ImGui::Text(
                    "Callers found: %zu",
                    state.m_callers.size()
                );

                for (size_t callerAddress :
                state.m_callers)
                {
                    char callerText[32];

                    std::snprintf(
                        callerText,
                        sizeof(callerText),
                        "0x%05zX",
                        callerAddress
                    );

                    if (ImGui::Selectable(
                        callerText,
                        false,
                        ImGuiSelectableFlags_AllowDoubleClick
                    ))
                    {
                        if (ImGui::IsMouseDoubleClicked(
                            ImGuiMouseButton_Left
                        ))
                        {
                            goToAddress(
                                callerAddress
                            );
                        }
                    }
                }
            }
        }

    }

}
