#include "DisassemblyView.h"

#include <cstdio>
#include <unordered_map>
#include <Zydis/Zydis.h>
#include "imgui.h"

namespace DosBoxMemoryTools
{
    void DisassemblyView::draw(DisassemblyState& state, const DisassemblyNavigate& goToAddress)
    {
        const auto& memory = state.m_disassemblyMemory;
        if (state.m_hasAddress &&
            state.m_address < memory.size())
        {
            ZydisDecoder decoder;

            if (ZYAN_SUCCESS(
                ZydisDecoderInit(
                    &decoder,
                    ZYDIS_MACHINE_MODE_LEGACY_16,
                    ZYDIS_STACK_WIDTH_16
                )))
            {
                ZydisFormatter formatter;

                if (ZYAN_SUCCESS(
                    ZydisFormatterInit(
                        &formatter,
                        ZYDIS_FORMATTER_STYLE_INTEL
                    )))
                {
                    if (ImGui::BeginTable(
                        "##Disassembly",
                        3,
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
                            90.0f
                        );

                        ImGui::TableSetupColumn(
                            "Bytes",
                            ImGuiTableColumnFlags_WidthFixed,
                            180.0f
                        );

                        ImGui::TableSetupColumn(
                            "Instruction",
                            ImGuiTableColumnFlags_WidthStretch
                        );

                        ImGui::TableSetupScrollFreeze(
                            0,
                            1
                        );

                        ImGui::TableHeadersRow();

                        if (state.m_scrollToTop)
                        {
                            ImGui::SetScrollY(0.0f);
                            state.m_scrollToTop = false;
                        }

                    size_t address =
                        state.m_address;

                    for (int i = 0;
                        i < 200 &&
                        address < memory.size();
                        ++i)
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
                            break;
                        }

                        char instructionText[256]{};

                        if (ZYAN_SUCCESS(
                            ZydisFormatterFormatInstruction(
                                &formatter,
                                &instruction,
                                operands,
                                instruction.operand_count_visible,
                                instructionText,
                                sizeof(instructionText),
                                static_cast<ZyanU64>(
                                    address
                                    ),
                                nullptr
                            )))
                        {
                            char bytesText[64] = {};
                            size_t offset = 0;

                            for (ZyanU8 byteIndex = 0;
                                byteIndex < instruction.length;
                                ++byteIndex)
                            {
                                offset += std::snprintf(
                                    bytesText + offset,
                                    sizeof(bytesText) - offset,
                                    "%02X ",
                                    static_cast<unsigned int>(
                                        memory[address + byteIndex]
                                        )
                                );

                                if (offset >= sizeof(bytesText))
                                {
                                    break;
                                }
                            }

                            bool hasBranchTarget = false;
                            size_t branchTarget = 0;

                            for (ZyanU8 operandIndex = 0;
                                operandIndex < instruction.operand_count_visible;
                                ++operandIndex)
                            {
                                const auto& operand =
                                    operands[operandIndex];

                                if (operand.type ==
                                    ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                                    operand.imm.is_relative)
                                {
                                    ZyanU64 absoluteAddress = 0;

                                    if (ZYAN_SUCCESS(
                                        ZydisCalcAbsoluteAddress(
                                            &instruction,
                                            &operand,
                                            static_cast<ZyanU64>(
                                                address
                                                ),
                                            &absoluteAddress
                                        )))
                                    {
                                        branchTarget =
                                            static_cast<size_t>(
                                                absoluteAddress
                                                );

                                        hasBranchTarget = true;
                                        break;
                                    }
                                }
                            }

                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);

                            char addressText[32];

                            std::snprintf(
                                addressText,
                                sizeof(addressText),
                                "0x%05zX",
                                address
                            );

                            ImGui::Selectable(
                                addressText,
                                false,
                                ImGuiSelectableFlags_AllowDoubleClick
                            );

                            if (ImGui::IsItemHovered() &&
                                ImGui::IsMouseDoubleClicked(
                                    ImGuiMouseButton_Left
                                ))
                            {
                                goToAddress(
                                    address
                                );
                            }

                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextUnformatted(
                                bytesText
                            );

                            ImGui::TableSetColumnIndex(2);

                            if (hasBranchTarget)
                            {
                                static const ImVec4 branchColors[] =
                                {
                                    ImVec4(1.00f, 0.45f, 0.45f, 1.00f),
                                    ImVec4(0.45f, 0.80f, 1.00f, 1.00f),
                                    ImVec4(0.55f, 1.00f, 0.55f, 1.00f),
                                    ImVec4(1.00f, 0.80f, 0.35f, 1.00f),
                                    ImVec4(0.85f, 0.55f, 1.00f, 1.00f),
                                    ImVec4(0.40f, 1.00f, 0.85f, 1.00f)
                                };

                                static std::unordered_map<size_t, size_t>
                                    branchColorIndices;

                                const size_t colorCount =
                                    sizeof(branchColors) /
                                    sizeof(branchColors[0]);

                                auto it =
                                    branchColorIndices.find(
                                        branchTarget
                                    );

                                if (it ==
                                    branchColorIndices.end())
                                {
                                    const size_t newColorIndex =
                                        branchColorIndices.size() %
                                        colorCount;

                                    it =
                                        branchColorIndices.emplace(
                                            branchTarget,
                                            newColorIndex
                                        ).first;
                                }

                                const size_t colorIndex =
                                    it->second;

                                ImGui::PushStyleColor(
                                    ImGuiCol_Text,
                                    branchColors[colorIndex]
                                );

                                ImGui::PushID(
                                    static_cast<int>(
                                        address
                                        )
                                );

                                ImGui::Selectable(
                                    instructionText,
                                    false,
                                    ImGuiSelectableFlags_AllowDoubleClick
                                );

                                if (ImGui::IsItemHovered() &&
                                    ImGui::IsMouseDoubleClicked(
                                        ImGuiMouseButton_Left
                                    ))
                                {
                                    goToAddress(
                                        branchTarget
                                    );
                                }

                                ImGui::PopID();
                                ImGui::PopStyleColor();
                            }
                            else
                            {
                                ImGui::TextUnformatted(
                                    instructionText
                                );
                            }                  }

                            address +=
                                instruction.length;
                    }

                    ImGui::EndTable();
                }
            }
        }
    }
    }

}
