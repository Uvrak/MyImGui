#include "pch.h"

#include "ExecutionTraceNavigationWindow.h"
#include "ExecutionTraceWindow.h"

#include <cstdlib>

#include "imgui.h"
#include "Zydis/Zydis.h"

namespace
{
    uint16_t registerValue(
        ZydisRegister reg,
        const ::MyImGui::RuntimeInstruction&
        runtimeInstruction
    )
    {
        switch (reg)
        {
        case ZYDIS_REGISTER_AX:
            return runtimeInstruction.registers.ax;

        case ZYDIS_REGISTER_BX:
            return runtimeInstruction.registers.bx;

        case ZYDIS_REGISTER_CX:
            return runtimeInstruction.registers.cx;

        case ZYDIS_REGISTER_DX:
            return runtimeInstruction.registers.dx;

        case ZYDIS_REGISTER_SI:
            return runtimeInstruction.registers.si;

        case ZYDIS_REGISTER_DI:
            return runtimeInstruction.registers.di;

        case ZYDIS_REGISTER_BP:
            return runtimeInstruction.registers.bp;

        case ZYDIS_REGISTER_SP:
            return runtimeInstruction.registers.sp;

        case ZYDIS_REGISTER_NONE:
            return 0;

        default:
            return 0;
        }
    }

    uint16_t segmentValue(
        ZydisRegister reg,
        const ::MyImGui::RuntimeInstruction&
        runtimeInstruction
    )
    {
        switch (reg)
        {
        case ZYDIS_REGISTER_DS:
            return runtimeInstruction.registers.ds;

        case ZYDIS_REGISTER_ES:
            return runtimeInstruction.registers.es;

        case ZYDIS_REGISTER_SS:
            return runtimeInstruction.registers.ss;

        default:
            return 0;
        }
    }

    bool matchesInstruction(
        const ZydisDecodedInstruction&
        decodedInstruction,
        const ZydisDecodedOperand* operands,
        int instructionIndex,
        uint32_t memoryAddress,
        const ::MyImGui::RuntimeInstruction&
        runtimeInstruction
    )
    {
        switch (instructionIndex)
        {
        case 0:
            return
                decodedInstruction.mnemonic ==
                ZYDIS_MNEMONIC_DIV &&
                decodedInstruction.
                operand_count_visible >= 1 &&
                operands[0].type ==
                ZYDIS_OPERAND_TYPE_REGISTER &&
                operands[0].reg.value ==
                ZYDIS_REGISTER_DI;

        case 1:
            return
                decodedInstruction.mnemonic ==
                ZYDIS_MNEMONIC_MOV &&
                decodedInstruction.
                operand_count_visible >= 2 &&
                operands[0].type ==
                ZYDIS_OPERAND_TYPE_REGISTER &&
                operands[0].reg.value ==
                ZYDIS_REGISTER_DI &&
                operands[1].type ==
                ZYDIS_OPERAND_TYPE_IMMEDIATE;

        case 2:
            return
                decodedInstruction.mnemonic ==
                ZYDIS_MNEMONIC_MOV &&
                decodedInstruction.
                operand_count_visible >= 2 &&
                operands[0].type ==
                ZYDIS_OPERAND_TYPE_REGISTER &&
                operands[0].reg.value ==
                ZYDIS_REGISTER_BX &&
                operands[1].type ==
                ZYDIS_OPERAND_TYPE_REGISTER &&
                operands[1].reg.value ==
                ZYDIS_REGISTER_AX;

        case 3:
            return
                decodedInstruction.mnemonic ==
                ZYDIS_MNEMONIC_SHL &&
                decodedInstruction.
                operand_count_visible >= 2 &&
                operands[0].type ==
                ZYDIS_OPERAND_TYPE_REGISTER &&
                operands[0].reg.value ==
                ZYDIS_REGISTER_BX &&
                operands[1].type ==
                ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                operands[1].imm.value.u == 1;

        case 4:
            return
                decodedInstruction.mnemonic ==
                ZYDIS_MNEMONIC_JMP &&
                decodedInstruction.
                operand_count_visible >= 1 &&
                operands[0].type ==
                ZYDIS_OPERAND_TYPE_MEMORY &&
                operands[0].mem.segment ==
                ZYDIS_REGISTER_CS &&
                operands[0].mem.base ==
                ZYDIS_REGISTER_BX;

        case 6:
            return
                decodedInstruction.mnemonic ==
                ZYDIS_MNEMONIC_INC &&
                decodedInstruction.
                operand_count_visible >= 1 &&
                operands[0].type ==
                ZYDIS_OPERAND_TYPE_REGISTER &&
                operands[0].reg.value ==
                ZYDIS_REGISTER_SI;

        case 7:
            return
                decodedInstruction.mnemonic ==
                ZYDIS_MNEMONIC_MOV &&
                decodedInstruction.
                operand_count_visible >= 2 &&
                operands[0].type ==
                ZYDIS_OPERAND_TYPE_REGISTER &&
                operands[0].reg.value ==
                ZYDIS_REGISTER_SI;

        case 8:
            if (decodedInstruction.
                operand_count_visible < 1)
            {
                return false;
            }

            if (operands[0].type !=
                ZYDIS_OPERAND_TYPE_MEMORY)
            {
                return false;
            }

            if (operands[0].mem.base !=
                ZYDIS_REGISTER_BP)
            {
                return false;
            }

            if (static_cast<int16_t>(
                operands[0].mem.disp.value
                ) != -2)
            {
                return false;
            }

            return
                decodedInstruction.mnemonic ==
                ZYDIS_MNEMONIC_MOV ||
                decodedInstruction.mnemonic ==
                ZYDIS_MNEMONIC_ADD ||
                decodedInstruction.mnemonic ==
                ZYDIS_MNEMONIC_SUB ||
                decodedInstruction.mnemonic ==
                ZYDIS_MNEMONIC_INC ||
                decodedInstruction.mnemonic ==
                ZYDIS_MNEMONIC_DEC;

        case 9:
        {
            if (decodedInstruction.
                operand_count_visible < 1)
            {
                return false;
            }

            const ZydisDecodedOperand&
                operand =
                operands[0];

            if (operand.type !=
                ZYDIS_OPERAND_TYPE_MEMORY)
            {
                return false;
            }

            if ((operand.actions &
                ZYDIS_OPERAND_ACTION_WRITE) == 0)
            {
                return false;
            }

            const uint16_t base =
                registerValue(
                    operand.mem.base,
                    runtimeInstruction
                );

            const uint16_t index =
                registerValue(
                    operand.mem.index,
                    runtimeInstruction
                );

            const uint16_t displacement =
                static_cast<uint16_t>(
                    operand.mem.disp.value
                    );

            const uint16_t effectiveOffset =
                static_cast<uint16_t>(
                    base +
                    index +
                    displacement
                    );

            ZydisRegister segmentRegister =
                operand.mem.segment;

            if (segmentRegister ==
                ZYDIS_REGISTER_NONE)
            {
                if (operand.mem.base ==
                    ZYDIS_REGISTER_BP ||
                    operand.mem.base ==
                    ZYDIS_REGISTER_SP)
                {
                    segmentRegister =
                        ZYDIS_REGISTER_SS;
                }
                else
                {
                    segmentRegister =
                        ZYDIS_REGISTER_DS;
                }
            }

            const uint16_t segment =
                segmentValue(
                    segmentRegister,
                    runtimeInstruction
                );

            const uint32_t physicalAddress =
                (static_cast<uint32_t>(
                    segment
                    ) << 4) +
                effectiveOffset;

            return
                physicalAddress ==
                memoryAddress;
        }

        default:
            return false;
        };
        
    }
}

namespace MyImGui
{
    void ExecutionTraceNavigationWindow::draw(
        bool* isOpen
    )
    {
        ZydisDecoder decoder;

        const bool zydisReady =
            ZYAN_SUCCESS(
                ZydisDecoderInit(
                    &decoder,
                    ZYDIS_MACHINE_MODE_LEGACY_16,
                    ZYDIS_STACK_WIDTH_16
                )
            );

        if (isOpen &&
            !*isOpen)
        {
            m_window.close();
            return;
        }

        if (!m_window.isOpen())
        {
            m_window.open();
        }

        if (!m_window.begin())
        {
            m_window.end();

            if (isOpen)
            {
                *isOpen =
                    m_window.isOpen();
            }

            return;
        }

        if (m_scanner &&
            m_recordButton &&
            m_targetText)
        {
            ImGui::SetNextItemWidth(
                120.0f
            );

            ImGui::InputText(
                "Target",
                m_targetText,
                m_targetTextSize
            );

            m_window.sameLineIfFits(
                ImGui::CalcTextSize("Save Trace...").x,
                "Save Trace..."
            );

            if (ImGui::Button(
                "Save Trace..."
            ))
            {
                m_saveTraceRequested = true;
            }

            m_window.sameLineIfFits(
                ImGui::CalcTextSize("Load Trace...").x,
                "Load Trace..."
            );

            if (ImGui::Button(
                "Load Trace..."
            ))
            {
                m_loadTraceRequested = true;
            }

            if (m_recordButton->draw())
            {
                if (m_recordButton->recording())
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
                        if (m_trace)
                        {
                            m_trace->clear();
                        }

                        if (m_selectedTraceIndex)
                        {
                            *m_selectedTraceIndex =
                                static_cast<size_t>(-1);
                        }

                        m_scanner->setReadTraceTarget(
                            static_cast<size_t>(
                                targetAddress
                                )
                        );
                    }
                    else
                    {
                        m_recordButton->stop();
                    }
                }
                else
                {
                    m_scanner->setReadTraceTarget(
                        0
                    );
                }
            }

            bool traceActive = false;
            bool traceArmed = false;

            const bool hasTraceActive =
                m_scanner->getReadTraceActive(
                    traceActive
                );

            const bool hasTraceArmed =
                m_scanner->getReadTraceArmed(
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
            }

            size_t traceTarget = 0;

            if (m_scanner->getReadTraceTarget(
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

            if (m_trace)
            {
                ImGui::Text(
                    "Captured instructions: %zu",
                    m_trace->size()
                );
            }

            ImGui::Separator();
        }

        uint32_t instructionMemoryAddress = 0;

        char* instructionMemoryEnd = nullptr;

        const unsigned long long
            instructionMemoryValue =
            std::strtoull(
                m_memoryAddressText,
                &instructionMemoryEnd,
                0
            );

        if (instructionMemoryEnd !=
            m_memoryAddressText &&
            *instructionMemoryEnd == '\0')
        {
            instructionMemoryAddress =
                static_cast<uint32_t>(
                    instructionMemoryValue
                    );
        }

        if (m_selectedTraceIndex)
        {
               const char* instructionNames[] =
            {
                "DIV DI",
                "MOV DI, imm",
                "MOV BX, AX",
                "SHL BX, 1",
                "JMP CS:[BX+...]",
                "MOV AL, [0x9306]",
                "INC SI",
                "MOV SI, ...",
                "WRITE [BP-0x02]",
                "WRITE [Memory Address]"
            };
            ImGui::SetNextItemWidth(
                150.0f
            );

            ImGui::Combo(
                "Instruction",
                &m_findInstructionIndex,
                instructionNames,
                IM_ARRAYSIZE(
                    instructionNames
                )
            );
            m_window.sameLineIfFits(
                ImGui::CalcTextSize("Search Start").x,
                "Search Start"
            );

            if (ImGui::Button(
                "Search Start"
            ))
            {
                if (zydisReady &&
                    m_trace &&
                    !m_trace->empty())
                {
                    for (size_t searchIndex =
                        m_trace->size();
                        searchIndex > 0;
                        --searchIndex)
                    {
                        const size_t index =
                            searchIndex - 1;

                        const RuntimeInstruction&
                            instruction =
                            (*m_trace)[index];

                        ZydisDecodedInstruction
                            decodedInstruction;

                        ZydisDecodedOperand operands[
                            ZYDIS_MAX_OPERAND_COUNT
                        ];

                        if (!ZYAN_SUCCESS(
                            ZydisDecoderDecodeFull(
                                &decoder,
                                instruction.bytes.data(),
                                instruction.bytes.size(),
                                &decodedInstruction,
                                operands
                            )
                        ))
                        {
                            continue;
                        }

                        if (matchesInstruction(
                            decodedInstruction,
                            operands,
                            m_findInstructionIndex,
                            instructionMemoryAddress,
                            instruction
                        ))
                        {
                            *m_selectedTraceIndex =
                                index;

                            if (m_scrollToSelectedTrace)
                            {
                                *m_scrollToSelectedTrace =
                                    true;
                            }

                            break;
                        }
                    }
                }
            }

            const float previousInstructionWidth =
                ImGui::CalcTextSize(
                    "Previous Instruction"
                ).x +
                ImGui::GetStyle().
                FramePadding.x * 2.0f;

            m_window.beginInlineGroupIfFits(
                previousInstructionWidth
            );
            

            if (ImGui::Button(
                "Previous Instruction"
            ))
            {
                if (zydisReady &&
                    m_trace &&
                    !m_trace->empty() &&
                    *m_selectedTraceIndex !=
                    static_cast<size_t>(-1) &&
                    *m_selectedTraceIndex > 0)
                {
                    for (size_t searchIndex =
                        *m_selectedTraceIndex;
                        searchIndex > 0;
                        --searchIndex)
                    {
                        const size_t index =
                            searchIndex - 1;

                        const RuntimeInstruction&
                            instruction =
                            (*m_trace)[index];

                        ZydisDecodedInstruction
                            decodedInstruction;

                        ZydisDecodedOperand operands[
                            ZYDIS_MAX_OPERAND_COUNT
                        ];

                        if (!ZYAN_SUCCESS(
                            ZydisDecoderDecodeFull(
                                &decoder,
                                instruction.bytes.data(),
                                instruction.bytes.size(),
                                &decodedInstruction,
                                operands
                            )
                        ))
                        {
                            continue;
                        }

                        if (matchesInstruction(
                            decodedInstruction,
                            operands,
                            m_findInstructionIndex,
                            instructionMemoryAddress,
                            instruction
                        ))
                        {
                            *m_selectedTraceIndex =
                                index;

                            if (m_scrollToSelectedTrace)
                            {
                                *m_scrollToSelectedTrace =
                                    true;
                            }

                            break;
                        }
                    }
                }
            }

        }

        const ImGuiStyle& style =
            ImGui::GetStyle();

        const float memoryInputWidth =
            120.0f;

        const float memoryLabelWidth =
            ImGui::CalcTextSize(
                "Memory Address"
            ).x;

        const float findMemoryButtonWidth =
            ImGui::CalcTextSize(
                "Find Memory"
            ).x +
            style.FramePadding.x * 2.0f;

        const float memoryGroupWidth =
            memoryInputWidth +
            style.ItemInnerSpacing.x +
            memoryLabelWidth +
            style.ItemSpacing.x +
            findMemoryButtonWidth;                                                               

        m_window.beginInlineGroupIfFits(
            memoryGroupWidth
        );

        ImGui::SetNextItemWidth(
            120.0f
        );

        ImGui::InputText(
            "Memory Address",
            m_memoryAddressText,
            sizeof(m_memoryAddressText)
        );

        m_window.sameLineIfFits(
            ImGui::CalcTextSize("Find Memory").x,
			"Find Memory"
        );

        ImGui::SameLine();

        if (ImGui::Button(
            "Find Memory"
        ))
        {
            char* end = nullptr;

            const unsigned long long
                memoryAddress =
                std::strtoull(
                    m_memoryAddressText,
                    &end,
                    0
                );

            if (end != m_memoryAddressText &&
                *end == '\0' &&
                zydisReady &&
                m_trace &&
                !m_trace->empty())
            {
                for (size_t searchIndex =
                    m_trace->size();
                    searchIndex > 0;
                    --searchIndex)
                {
                    const size_t index =
                        searchIndex - 1;

                    const RuntimeInstruction&
                        instruction =
                        (*m_trace)[index];

                    ZydisDecodedInstruction
                        decodedInstruction;

                    ZydisDecodedOperand operands[
                        ZYDIS_MAX_OPERAND_COUNT
                    ];

                    if (!ZYAN_SUCCESS(
                        ZydisDecoderDecodeFull(
                            &decoder,
                            instruction.bytes.data(),
                            instruction.bytes.size(),
                            &decodedInstruction,
                            operands
                        )
                    ))
                    {
                        continue;
                    }

                    bool found = false;

                    for (ZyanU8 operandIndex = 0;
                        operandIndex <
                        decodedInstruction.
                        operand_count_visible;
                        ++operandIndex)
                    {
                        const ZydisDecodedOperand&
                            operand =
                            operands[operandIndex];

                        if (operand.type !=
                            ZYDIS_OPERAND_TYPE_MEMORY)
                        {
                            continue;
                        }

                        if (operand.mem.base ==
                            ZYDIS_REGISTER_NONE &&
                            operand.mem.index ==
                            ZYDIS_REGISTER_NONE &&
                            static_cast<uint16_t>(
                                operand.mem.disp.value
                                ) ==
                            static_cast<uint16_t>(
                                memoryAddress
                                ))
                        {
                            found = true;
                            break;
                        }
                    }

                    if (found)
                    {
                        *m_selectedTraceIndex =
                            index;

                        if (m_scrollToSelectedTrace)
                        {
                            *m_scrollToSelectedTrace =
                                true;
                        }

                        break;
                    }
                }
            }
        }

        static const char* registerNames[] =
        {
            "AX",
            "BX",
            "CX",
            "DX",
            "SI",
            "DI",
            "BP",
            "SP",
            "DS",
            "ES",
            "SS"
        };

        ImGui::SetNextItemWidth(
            80.0f
        );

        ImGui::Combo(
            "Register",
            &m_previousRegisterIndex,
            registerNames,
            IM_ARRAYSIZE(registerNames)
        );

        m_window.sameLineIfFits(
            ImGui::CalcTextSize(
                "Previous"
            ).x +
            ImGui::GetStyle().
            FramePadding.x * 2.0f
        );

        if (ImGui::Button(
            "Previous"
        ))
        {
                const size_t originalIndex =
                    *m_selectedTraceIndex;

                size_t foundIndex =
                    static_cast<size_t>(-1);

                for (size_t searchIndex =
    originalIndex > 0
    ? originalIndex - 1
    : 0;
    searchIndex > 0;
    --searchIndex)
                {
                    const RegisterSnapshot&
                        current =
                        (*m_trace)[searchIndex].
                        registers;

                    const RegisterSnapshot&
                        previous =
                        (*m_trace)[searchIndex - 1].
                        registers;

                    uint16_t currentValue = 0;
                    uint16_t previousValue = 0;

                    switch (m_previousRegisterIndex)
                    {
                    case 0:
                        currentValue = current.ax;
                        previousValue = previous.ax;
                        break;

                    case 1:
                        currentValue = current.bx;
                        previousValue = previous.bx;
                        break;

                    case 2:
                        currentValue = current.cx;
                        previousValue = previous.cx;
                        break;

                    case 3:
                        currentValue = current.dx;
                        previousValue = previous.dx;
                        break;

                    case 4:
                        currentValue = current.si;
                        previousValue = previous.si;
                        break;

                    case 5:
                        currentValue = current.di;
                        previousValue = previous.di;
                        break;

                    case 6:
                        currentValue = current.bp;
                        previousValue = previous.bp;
                        break;

                    case 7:
                        currentValue = current.sp;
                        previousValue = previous.sp;
                        break;

                    case 8:
                        currentValue = current.ds;
                        previousValue = previous.ds;
                        break;

                    case 9:
                        currentValue = current.es;
                        previousValue = previous.es;
                        break;

                    case 10:
                        currentValue = current.ss;
                        previousValue = previous.ss;
                        break;
                    }

                    if (currentValue !=
                        previousValue)
                    {
                        foundIndex =
                            searchIndex;

                        break;
                    }
                }

                if (foundIndex !=
                    static_cast<size_t>(-1))
                {
                    *m_selectedTraceIndex =
                        foundIndex;

                    if (m_scrollToSelectedTrace)
                    {
                        *m_scrollToSelectedTrace =
                            true;
                    }
                }
            


        }
    
        m_window.end();

        if (isOpen)
        {
            *isOpen =
                m_window.isOpen();
        }
    }

    bool ExecutionTraceNavigationWindow::
        saveTraceRequested()
    {
        const bool requested =
            m_saveTraceRequested;

        m_saveTraceRequested = false;

        return requested;
    }

    bool ExecutionTraceNavigationWindow::
        loadTraceRequested()
    {
        const bool requested =
            m_loadTraceRequested;

        m_loadTraceRequested = false;

        return requested;
    }

    void ExecutionTraceNavigationWindow::setScanner(
        DosBoxMemoryScanner* scanner
    )
    {
        m_scanner =
            scanner;
    }

    void ExecutionTraceNavigationWindow::setRecordButton(
        RecordButton* recordButton
    )
    {
        m_recordButton =
            recordButton;
    }

    void ExecutionTraceNavigationWindow::setTargetText(
        char* targetText,
        size_t targetTextSize
    )
    {
        m_targetText =
            targetText;

        m_targetTextSize =
            targetTextSize;
    }

    void ExecutionTraceNavigationWindow::
        setSelectedTraceIndex(
            size_t* selectedTraceIndex
        )
    {
        m_selectedTraceIndex =
            selectedTraceIndex;
    }

    void ExecutionTraceNavigationWindow::setTrace(
        std::vector<RuntimeInstruction>* trace
    )
    {
        m_trace =
            trace;
    }
    
    void ExecutionTraceNavigationWindow::
        setScrollToSelectedTrace(
            bool* scrollToSelectedTrace
        )
    {
        m_scrollToSelectedTrace =
            scrollToSelectedTrace;
    }
}