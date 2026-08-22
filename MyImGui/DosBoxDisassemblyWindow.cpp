#include "pch.h"
#include "DosBoxDisassemblyWindow.h"

#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <fstream>

#include "imgui.h"
#include <Zydis/Zydis.h>

namespace MyImGui
{
    DosBoxDisassemblyWindow::
        DosBoxDisassemblyWindow(
            DosBoxMemoryReader& memoryReader
        )
        : m_memoryReader(
            memoryReader
        )
    {}

    void DosBoxDisassemblyWindow::draw(
        bool* isOpen
    )
    {
        if (!m_savedAddressesLoaded)
        {
            loadSession();
            m_savedAddressesLoaded = true;
        }

        if (isOpen &&
            !*isOpen)
        {
            return;
        }

        m_wasOpen = true;

        ImGui::SetNextWindowSize(
            ImVec2(
                700.0f,
                500.0f
            ),
            ImGuiCond_FirstUseEver
        );

        const bool windowVisible =
            ImGui::Begin(
                "DOSBox Disassembly",
                isOpen
            );

        if (isOpen &&
            !*isOpen)
        {
            saveSession();
            m_wasOpen = false;
        }

        if (!windowVisible)
        {
            ImGui::End();
            return;
        }

        m_wasOpen = true;

        const auto& memory =
            m_memoryReader.memory();

        ImGui::SetNextItemWidth(
            120.0f
        );

        ImGui::InputText(
            "Address",
            m_addressText,
            sizeof(m_addressText)
        );

        ImGui::SameLine();

        if (ImGui::Button("Go"))
        {
            char* end = nullptr;

            const unsigned long long address =
                std::strtoull(
                    m_addressText,
                    &end,
                    0
                );

            if (end != m_addressText &&
                *end == '\0' &&
                address < memory.size())
            {
                goToAddress(
                    static_cast<size_t>(
                        address
                        )
                );
            }
        }

        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            120.0f
        );

        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            160.0f
        );

        ImGui::InputText(
            "Name##SavedAddressName",
            m_sessionName,
            sizeof(m_sessionName)
        );

        ImGui::SameLine();

        if (ImGui::Button(
            "Save Address"
        ))
        {
            addSavedAddress();
        }

        ImGui::SetNextItemWidth(
            180.0f
        );

        if (ImGui::BeginCombo(
            "Saved Addresses",
            "Select..."
        ))
        {
            if (m_savedAddresses.empty())
            {
                ImGui::TextDisabled(
                    "No saved addresses."
                );
            }
            else
            {
                for (size_t i = 0;
                    i < m_savedAddresses.size();
                    ++i)
                {
                    const SavedAddress& entry =
                        m_savedAddresses[i];

                    char label[256];

                    if (entry.name.empty())
                    {
                        std::snprintf(
                            label,
                            sizeof(label),
                            "0x%05zX",
                            entry.address
                        );
                    }
                    else
                    {
                        std::snprintf(
                            label,
                            sizeof(label),
                            "0x%05zX - %s",
                            entry.address,
                            entry.name.c_str()
                        );
                    }

                    ImGui::PushID(
                        static_cast<int>(i)
                    );

                    bool deleteEntry = false;

                    if (ImGui::Selectable(
                        label
                    ))
                    {
                        goToAddress(
                            entry.address
                        );
                    }

                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::MenuItem(
                            "Delete"
                        ))
                        {
                            deleteEntry = true;
                        }

                        ImGui::EndPopup();
                    }

                    if (deleteEntry)
                    {
                        m_savedAddresses.erase(
                            m_savedAddresses.begin() + i
                        );

                        saveSession();

                        ImGui::PopID();
                        break;
                    }

                    ImGui::PopID();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::SetNextItemWidth(
            120.0f
        );

        ImGui::InputText(
            "Target##CallerTarget",
            m_callerTargetText,
            sizeof(m_callerTargetText)
        );

        ImGui::SameLine();

        if (ImGui::Button(
            "Find Callers"
        ))
        {
            m_callers.clear();
            m_callerSearchPerformed = true;

            char* end = nullptr;

            const unsigned long long target =
                std::strtoull(
                    m_callerTargetText,
                    &end,
                    0
                );

            if (end != m_callerTargetText &&
                *end == '\0' &&
                target < memory.size())
            {
                ZydisDecoder decoder;

                if (ZYAN_SUCCESS(
                    ZydisDecoderInit(
                        &decoder,
                        ZYDIS_MACHINE_MODE_LONG_COMPAT_16,
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
                                    m_callers.push_back(
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
                                m_callers.push_back(
                                    address
                                );

                                break;
                            }
                        }
                    }
                }
            }
        }

        if (m_callerSearchPerformed)
        {
            if (m_callers.empty())
            {
                ImGui::TextUnformatted(
                    "No callers found."
                );
            }
            else
            {
                ImGui::Text(
                    "Callers found: %zu",
                    m_callers.size()
                );

                for (size_t callerAddress :
                m_callers)
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

        ImGui::Separator();

        if (m_hasAddress &&
            m_address < memory.size())
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
                        
                        if (m_scrollToTop)
                        {
                            ImGui::SetScrollY(0.0f);
                            m_scrollToTop = false;
                        }

                    size_t address =
                        m_address;

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

    ImGui::End();
}

    void DosBoxDisassemblyWindow::goToAddress(
        size_t address
    )
    {
        m_address = address;
        m_hasAddress = true;
        m_scrollToTop = true;

        if (m_callerTargetText[0] == '\0')
        {
            std::snprintf(
                m_callerTargetText,
                sizeof(m_callerTargetText),
                "0x%zX",
                address
            );
        }
        std::snprintf(
            m_addressText,
            sizeof(m_addressText),
            "0x%zX",
            address
        );
    }
    
    void DosBoxDisassemblyWindow::saveSession()
    {
        std::ofstream file(
            "settings/dosbox_disassembly.cfg"
        );

        if (!file)
        {
            m_status =
                "Could not save disassembly session.";

            return;
        }

        for (const SavedAddress& entry :
            m_savedAddresses)
        {
            file << std::hex
                << entry.address
                << '\t'
                << entry.name
                << '\n';
        }

        m_status =
            "Disassembly session saved.";
    }

    void DosBoxDisassemblyWindow::loadSession()
    {
        m_savedAddresses.clear();

        std::ifstream file(
            "settings/dosbox_disassembly.cfg"
        );

        if (!file)
        {
            m_status =
                "No disassembly session found.";

            return;
        }

        size_t address = 0;

        while (file >> std::hex >> address)
        {
            std::string name;

            std::getline(
                file,
                name
            );

            if (!name.empty() &&
                name[0] == '\t')
            {
                name.erase(
                    0,
                    1
                );
            }

            SavedAddress entry;

            entry.address =
                address;

            entry.name =
                name;

            m_savedAddresses.push_back(
                entry
            );
        }

        m_status =
            "Disassembly session loaded.";
    }
    
    void DosBoxDisassemblyWindow::addSavedAddress()
    {
        char* end = nullptr;

        const unsigned long long address =
            std::strtoull(
                m_addressText,
                &end,
                0
            );

        if (end == m_addressText ||
            *end != '\0')
        {
            m_status =
                "Invalid address.";

            return;
        }

        m_address =
            static_cast<size_t>(
                address
                );

        m_hasAddress = true;

        for (SavedAddress& entry :
            m_savedAddresses)
        {
            if (entry.address == m_address)
            {
                entry.name =
                    m_sessionName;

                saveSession();

                m_status =
                    "Address updated.";

                return;
            }
        }

        SavedAddress entry;

        entry.address =
            m_address;

        entry.name =
            m_sessionName;

        m_savedAddresses.push_back(
            entry
        );

        saveSession();

        m_status =
            "Address saved.";
    }
}