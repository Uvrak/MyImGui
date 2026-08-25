#include "pch.h"
#include "InstructionTransitionTrackerWindow.h"

#include "imgui.h"
#include "Zydis/Zydis.h"

#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <cstring>

namespace MyImGui
{
    InstructionTransitionTrackerWindow::
        InstructionTransitionTrackerWindow(
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

    void InstructionTransitionTrackerWindow::draw(
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
            "Instruction Transition Tracker",
            isOpen
        ))
        {
            ImGui::End();
            return;
        }

        ImGui::TextUnformatted(
            "Instruction Transition Tracker"
        );

        ImGui::SetNextItemWidth(
            120.0f
        );

        ImGui::InputText(
            "Transition Target",
            m_transitionTargetText,
            sizeof(m_transitionTargetText)
        );

        if (ImGui::Button(
            "Start Tracking"
        ))
        {
            char* end = nullptr;

            const unsigned long long targetAddress =
                std::strtoull(
                    m_transitionTargetText,
                    &end,
                    0
                );

            if (end != m_transitionTargetText &&
                *end == '\0')
            {
                m_scanner.setReadTrackingTransitionTarget(
                    static_cast<size_t>(
                        targetAddress
                        )
                );

                m_scanner.clearTransitionTracking();
                m_scanner.startTransitionTracking();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button(
            "Capture"
        ))
        {
            m_scanner.stopTransitionTracking();

            m_scanner.getReadTrackingTransitions(
                m_transitions
            );

            m_scanner.getReadTrackingTransitionContexts(
                m_transitionContexts
            );

            m_scanner.getReadTrackingTransitionBytes(
                m_transitionBytes
            );

            m_transitionHistories.clear();

            m_transitionHistories.resize(
                m_transitions.size()
            );

            m_transitionNextInstructions.clear();

            m_transitionNextInstructions.resize(
                m_transitions.size()
            );

            for (size_t i = 0;
                i < m_transitions.size();
                ++i)
            {
                if (!m_scanner.getReadTrackingTransitionHistory(
                    i,
                    m_transitionHistories[i]
                ))
                {
                    break;
                }

                m_scanner.getReadTrackingTransitionNextInstruction(
                    i,
                    m_transitionNextInstructions[i]
                );
            }
        }

        if (ImGui::Button(
            "Set Execution Target"
        ))
        {
            char* end = nullptr;

            const unsigned long long targetAddress =
                std::strtoull(
                    m_transitionTargetText,
                    &end,
                    0
                );

            if (end !=
                m_transitionTargetText &&
                *end == '\0')
            {
                const bool ok =
                    m_scanner.setExecutionCaptureTarget(
                        static_cast<size_t>(
                            targetAddress
                            )
                    );

                if (ok)
                {
                    m_executionCaptureHit =
                        false;

                    m_executionCapture =
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
                m_executionCaptureHit =
                    false;

                m_executionCapture =
                    RuntimeInstruction{};
            }
        }

        if (ImGui::Button(
            "Get Execution Capture"
        ))
        {
            RuntimeInstruction instruction;

            if (m_scanner.getExecutionCapture(
                instruction
            ))
            {
                m_executionCapture =
                    instruction;

                m_executionCaptureHit =
                    true;
            }
        }
        bool executionHit = false;

        if (m_scanner.getExecutionCaptureHit(
            executionHit
        ))
        {
            if (executionHit &&
                !m_executionCaptureHit)
            {
                if (m_scanner.getExecutionCapture(
                    m_executionCapture
                ))
                {
                    m_executionCaptureHit =
                        true;
                }
            }

            ImGui::Text(
                "Execution capture state: %s",
                m_executionCaptureHit
                ? "HIT"
                : "ARMED / NO HIT"
            );
        }
        else
        {
            ImGui::TextDisabled(
                "Execution capture state unavailable."
            );
        }

        size_t executionTarget = 0;

        if (m_scanner.getExecutionCaptureTarget(
            executionTarget
        ))
        {
            ImGui::Text(
                "DOSBox execution target: 0x%zX",
                executionTarget
            );
        }
        else
        {
            ImGui::TextDisabled(
                "DOSBox execution target unavailable."
            );
        }

        ImGui::Separator();

        ImGui::Separator();

      
        
        ImGui::Text(
            "Instruction transitions: %zu",
            m_transitions.size()
        );

        ImGui::Text(
            "Transition contexts: %zu",
            m_transitionContexts.size()
        );

        ImGui::Text(
            "Transition bytes: %zu",
            m_transitionBytes.size()
        );

        for (size_t i = 0;
            i < m_transitions.size();
            ++i)
        {
            const auto& transition =
                m_transitions[i];

            ImGui::Separator();

            if (i < m_transitionContexts.size())
            {
                const auto& context =
                    m_transitionContexts[i];

                ImGui::Text(
                    "0x%zX -> 0x%zX    CS:IP %04X:%04X",
                    transition.first,
                    transition.second,
                    static_cast<unsigned int>(
                        context.first
                        ),
                    static_cast<unsigned int>(
                        context.second
                        )
                );
            }
            else
            {
                ImGui::Text(
                    "0x%zX -> 0x%zX",
                    transition.first,
                    transition.second
                );
            }

            if (i < m_transitionBytes.size())
            {
                ImGui::TextUnformatted(
                    "Runtime bytes:"
                );

                ImGui::SameLine();

                for (size_t byteIndex = 0;
                    byteIndex < m_transitionBytes[i].size();
                    ++byteIndex)
                {
                    if (byteIndex != 0)
                    {
                        ImGui::SameLine(
                            0.0f,
                            4.0f
                        );
                    }

                    ImGui::Text(
                        "%02X",
                        static_cast<unsigned int>(
                            m_transitionBytes[i][byteIndex]
                            )
                    );
                }
            }

            if (i < m_transitionNextInstructions.size())
            {
                const auto& nextInstruction =
                    m_transitionNextInstructions[i];

                if (nextInstruction.address != 0)
                {
                    ImGui::Text(
                        "Next executed: 0x%zX    CS:IP %04X:%04X",
                        nextInstruction.address,
                        static_cast<unsigned int>(
                            nextInstruction.cs
                            ),
                        static_cast<unsigned int>(
                            nextInstruction.ip
                            )
                    );
                }
            }

            if (i < m_transitionHistories.size())
            {
                const auto& history =
                    m_transitionHistories[i];

                ImGui::TextUnformatted(
                    "Runtime history:"
                );

                for (size_t historyIndex = 0;
                    historyIndex < history.size();
                    ++historyIndex)
                {
                    const auto& instruction =
                        history[historyIndex];
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

                    ImGui::Text(
                        "%03zu",
                        historyIndex + 1
                    );

                    ImGui::SameLine();

                    ImGui::PushID(
                        static_cast<int>(
                            historyIndex
                            )
                    );

                    char addressText[32];

                    sprintf_s(
                        addressText,
                        "0x%zX",
                        instruction.address
                    );

                    ImGui::Selectable(
                        addressText,
                        false,
                        ImGuiSelectableFlags_AllowDoubleClick,
                        ImVec2(
                            ImGui::CalcTextSize(
                                addressText
                            ).x,
                            0.0f
                        )
                    );

                    if (ImGui::IsItemHovered() &&
                        ImGui::IsMouseDoubleClicked(
                            ImGuiMouseButton_Left
                        ))
                    {
                        sprintf_s(
                            m_transitionTargetText,
                            sizeof(m_transitionTargetText),
                            "0x%zX",
                            instruction.address
                        );
                    }

                    ImGui::PopID();

                    ImGui::SameLine();

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
                        if (byteIndex != 0)
                        {
                            ImGui::SameLine(
                                0.0f,
                                4.0f
                            );
                        }

                        ImGui::Text(
                            "%02X",
                            static_cast<unsigned int>(
                                instruction.bytes[
                                    byteIndex
                                ]
                                )
                        );
                    }

                    ImGui::SameLine();

                    ImGui::Text(
                        "CS:IP %04X:%04X",
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
                        "           AX=%04X BX=%04X CX=%04X DX=%04X",
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
                        "           SI=%04X DI=%04X BP=%04X SP=%04X",
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
                        "           DS=%04X ES=%04X",
                        static_cast<unsigned int>(
                            instruction.registers.ds
                            ),
                        static_cast<unsigned int>(
                            instruction.registers.es
                            )
                    );
                }
            }
        }

        ImGui::End();
    }

    void InstructionTransitionTrackerWindow::saveSession() const
            {
                if (m_gameId.empty())
                {
                    return;
                }

                std::filesystem::create_directories(
                    "settings"
                );

                const std::string filename =
                    "settings/instruction_transition_session_" +
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
                    "GridBuilderInstructionTransitionSession 2\n";

                file <<
                    "TransitionTarget\n";

                file <<
                    m_transitionTargetText <<
                    '\n';

                file <<
                    "Transitions\n";

                file <<
                    m_transitions.size() <<
                    '\n';

                for (const auto& transition :
                    m_transitions)
                {
                    file <<
                        transition.first << ' ' <<
                        transition.second << '\n';
                }

                file <<
                    "TransitionContexts\n";

                file <<
                    m_transitionContexts.size() <<
                    '\n';

                for (const auto& context :
                    m_transitionContexts)
                {
                    file <<
                        context.first << ' ' <<
                        context.second << '\n';
                }

                file <<
                    "TransitionBytes\n";

                file <<
                    m_transitionBytes.size() <<
                    '\n';

                for (const auto& bytes :
                    m_transitionBytes)
                {
                    for (size_t i = 0;
                        i < bytes.size();
                        ++i)
                    {
                        if (i != 0)
                        {
                            file << ' ';
                        }

                        file <<
                            static_cast<unsigned int>(
                                bytes[i]
                                );
                    }

                    file << '\n';
                }

                file <<
                    "TransitionHistories\n";

                file <<
                    m_transitionHistories.size() <<
                    '\n';

                for (const auto& history :
                    m_transitionHistories)
                {
                    file <<
                        history.size() <<
                        '\n';

                    for (const RuntimeInstruction& instruction :
                        history)
                    {
                        file <<
                            instruction.address << ' ' <<
                            instruction.cs << ' ' <<
                            instruction.ip << ' ' <<
                            instruction.registers.ax << ' ' <<
                            instruction.registers.bx << ' ' <<
                            instruction.registers.cx << ' ' <<
                            instruction.registers.dx << ' ' <<
                            instruction.registers.si << ' ' <<
                            instruction.registers.di << ' ' <<
                            instruction.registers.bp << ' ' <<
                            instruction.registers.sp << ' ' <<
                            instruction.registers.ds << ' ' <<
                            instruction.registers.es;

                        for (const uint8_t byte :
                        instruction.bytes)
                        {
                            file << ' ' <<
                                static_cast<unsigned int>(
                                    byte
                                    );
                        }

                        file << '\n';
                    }
                }
            }

    void InstructionTransitionTrackerWindow::loadSession()
    {
        m_transitions.clear();
        m_transitionContexts.clear();
        m_transitionBytes.clear();
        m_transitionHistories.clear();

        if (m_gameId.empty())
        {
            return;
        }

        const std::string filename =
            "settings/instruction_transition_session_" +
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
            "GridBuilderInstructionTransitionSession 2")
        {
            return;
        }

        std::string section;

        if (!(file >> section) ||
            section != "TransitionTarget")
        {
            return;
        }

        std::string target;

        if (!(file >> target))
        {
            return;
        }

        strncpy_s(
            m_transitionTargetText,
            sizeof(m_transitionTargetText),
            target.c_str(),
            _TRUNCATE
        );

        if (!(file >> section) ||
            section != "Transitions")
        {
            return;
        }

        size_t transitionCount = 0;

        if (!(file >> transitionCount))
        {
            return;
        }

        m_transitions.reserve(
            transitionCount
        );

        for (size_t i = 0;
            i < transitionCount;
            ++i)
        {
            size_t previousAddress = 0;
            size_t currentAddress = 0;

            if (!(file >>
                previousAddress >>
                currentAddress))
            {
                return;
            }

            m_transitions.emplace_back(
                previousAddress,
                currentAddress
            );
        }

        if (!(file >> section) ||
            section != "TransitionContexts")
        {
            return;
        }

        size_t contextCount = 0;

        if (!(file >> contextCount))
        {
            return;
        }

        m_transitionContexts.reserve(
            contextCount
        );

        for (size_t i = 0;
            i < contextCount;
            ++i)
        {
            unsigned int cs = 0;
            unsigned int ip = 0;

            if (!(file >>
                cs >>
                ip))
            {
                return;
            }

            m_transitionContexts.emplace_back(
                static_cast<uint16_t>(
                    cs
                    ),
                static_cast<uint16_t>(
                    ip
                    )
            );
        }
        

        if (!(file >> section) ||
            section != "TransitionBytes")
        {
            return;
        }

        size_t byteBlockCount = 0;

        if (!(file >> byteBlockCount))
        {
            return;
        }

        m_transitionBytes.resize(
            byteBlockCount
        );

        for (auto& bytes :
            m_transitionBytes)
        {
            for (uint8_t& byte :
                bytes)
            {
                unsigned int value = 0;

                if (!(file >> value) ||
                    value > 255)
                {
                    return;
                }

                byte =
                    static_cast<uint8_t>(
                        value
                        );
            }
        }

        if (!(file >> section) ||
            section != "TransitionHistories")
        {
            return;
        }

        size_t historyCount = 0;

        if (!(file >> historyCount))
        {
            return;
        }

        m_transitionHistories.resize(
            historyCount
        );

        for (auto& history :
            m_transitionHistories)
        {
            size_t instructionCount = 0;

            if (!(file >> instructionCount))
            {
                return;
            }

            history.resize(
                instructionCount
            );

            for (RuntimeInstruction& instruction :
                history)
            {

                unsigned int cs = 0;
                unsigned int ip = 0;

                unsigned int ax = 0;
                unsigned int bx = 0;
                unsigned int cx = 0;
                unsigned int dx = 0;

                unsigned int si = 0;
                unsigned int di = 0;
                unsigned int bp = 0;
                unsigned int sp = 0;

                unsigned int ds = 0;
                unsigned int es = 0;

                if (!(file >>
                    instruction.address >>
                    cs >>
                    ip >>
                    ax >>
                    bx >>
                    cx >>
                    dx >>
                    si >>
                    di >>
                    bp >>
                    sp >>
                    ds >>
                    es))
                {
                    return;
                }

                instruction.cs =
                    static_cast<uint16_t>(cs);

                instruction.ip =
                    static_cast<uint16_t>(ip);

                instruction.registers.ax =
                    static_cast<uint16_t>(ax);

                instruction.registers.bx =
                    static_cast<uint16_t>(bx);

                instruction.registers.cx =
                    static_cast<uint16_t>(cx);

                instruction.registers.dx =
                    static_cast<uint16_t>(dx);

                instruction.registers.si =
                    static_cast<uint16_t>(si);

                instruction.registers.di =
                    static_cast<uint16_t>(di);

                instruction.registers.bp =
                    static_cast<uint16_t>(bp);

                instruction.registers.sp =
                    static_cast<uint16_t>(sp);

                instruction.registers.ds =
                    static_cast<uint16_t>(ds);

                instruction.registers.es =
                    static_cast<uint16_t>(es);

                for (uint8_t& byte :
                    instruction.bytes)
                {
                    unsigned int value = 0;

                    if (!(file >> value) ||
                        value > 255)
                    {
                        return;
                    }

                    byte =
                        static_cast<uint8_t>(
                            value
                            );
                }
            }
        }
    }

    void InstructionTransitionTrackerWindow::setGameId(
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
}