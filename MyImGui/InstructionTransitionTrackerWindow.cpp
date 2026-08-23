#include "pch.h"
#include "InstructionTransitionTrackerWindow.h"

#include "imgui.h"

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

                m_scanner.clearReadTracking();
                m_scanner.startReadTracking();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button(
            "Capture"
        ))
        {
            m_scanner.stopReadTracking();

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

            for (size_t i = 0;
                i < m_transitions.size();
                ++i)
            {
                m_scanner.getReadTrackingTransitionHistory(
                    i,
                    m_transitionHistories[i]
                );
            }
        }

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

            if (i < m_transitionHistories.size())
            {
                const auto& history =
                    m_transitionHistories[i];

                ImGui::TextUnformatted(
                    "Runtime history:"
                );

                for (const auto& instruction :
                    history)
                {
                    ImGui::Text(
                        "  0x%zX  CS:IP %04X:%04X",
                        instruction.address,
                        static_cast<unsigned int>(
                            instruction.cs
                            ),
                        static_cast<unsigned int>(
                            instruction.ip
                            )
                    );

                    ImGui::SameLine();

                    for (size_t byteIndex = 0;
                        byteIndex < instruction.bytes.size();
                        ++byteIndex)
                    {
                        ImGui::SameLine(
                            0.0f,
                            4.0f
                        );

                        ImGui::Text(
                            "%02X",
                            static_cast<unsigned int>(
                                instruction.bytes[
                                    byteIndex
                                ]
                                )
                        );
                    }
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
                    "GridBuilderInstructionTransitionSession 1\n";

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
                            instruction.ip;

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
                    "GridBuilderInstructionTransitionSession 1")
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

                    if (!(file >> cs >> ip))
                    {
                        return;
                    }

                    m_transitionContexts.emplace_back(
                        static_cast<uint16_t>(cs),
                        static_cast<uint16_t>(ip)
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

                        if (!(file >>
                            instruction.address >>
                            cs >>
                            ip))
                        {
                            return;
                        }

                        instruction.cs =
                            static_cast<uint16_t>(cs);

                        instruction.ip =
                            static_cast<uint16_t>(ip);

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