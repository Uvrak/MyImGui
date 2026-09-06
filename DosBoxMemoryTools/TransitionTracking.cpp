#include "TransitionTracking.h"

#include "imgui.h"
#include "Zydis/Zydis.h"

#include <cstdio>

namespace DosBoxMemoryTools
{
    void TransitionTracking::draw(
        char* targetText,
        size_t targetTextSize
    )
    {
        updateAutoCapture();

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

                    bool decoded =
                        false;

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

                    const bool selected =
                        historyIndex ==
                        m_selectedHistoryInstruction;

                    ImGui::Text(
                        "%03zu",
                        historyIndex + 1
                    );

                    ImGui::SameLine();

                    char addressText[32];

                    sprintf_s(
                        addressText,
                        "0x%zX",
                        instruction.address
                    );

                    ImGui::PushID(
                        static_cast<int>(i)
                    );

                    ImGui::PushID(
                        static_cast<int>(
                            historyIndex
                            )
                    );

                    if (ImGui::Selectable(
                        addressText,
                        historyIndex ==
                        m_selectedHistoryInstruction,
                        ImGuiSelectableFlags_AllowDoubleClick,
                        ImVec2(
                            ImGui::CalcTextSize(
                                addressText
                            ).x,
                            0.0f
                        )
                    ))
                    {
                        m_selectedHistoryInstruction =
                            historyIndex;
                    }

                    ImGui::PopID();
                    ImGui::PopID();

                    if (selected &&
                        m_scrollToSelectedHistoryInstruction)
                    {
                        ImGui::SetScrollHereY(
                            0.5f
                        );

                        m_scrollToSelectedHistoryInstruction =
                            false;
                    }

                    if (ImGui::IsItemHovered() &&
                        ImGui::IsMouseDoubleClicked(
                            ImGuiMouseButton_Left
                        ))
                    {
                        std::snprintf(
                            targetText,
                            targetTextSize,
                            "0x%zX",
                            instruction.address
                        );
                    }

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

                    ImGui::NewLine();

                    ImGui::Text(
                        "CS:IP %04X:%04X",
                        static_cast<unsigned int>(
                            instruction.cs
                            ),
                        static_cast<unsigned int>(
                            instruction.ip
                            )
                    );

                    ImGui::NewLine();

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
                }
            }
        }
    }

    void TransitionTracking::drawNavigation()
    {
        const bool hasHistory =
            !m_transitionHistories.empty() &&
            !m_transitionHistories[0].empty();

        if (hasHistory &&
            m_selectedHistoryInstruction ==
            static_cast<size_t>(-1))
        {
            m_selectedHistoryInstruction =
                m_transitionHistories[0].size() - 1;

            m_scrollToSelectedHistoryInstruction =
                true;
        }

        if (hasHistory &&
            m_selectedHistoryInstruction >=
            m_transitionHistories[0].size())
        {
            m_selectedHistoryInstruction =
                m_transitionHistories[0].size() - 1;
        }

        const bool canPrevious =
            hasHistory &&
            m_selectedHistoryInstruction > 0;

        if (!canPrevious)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("<"))
        {
            --m_selectedHistoryInstruction;

            m_scrollToSelectedHistoryInstruction =
                true;
        }

        if (!canPrevious)
        {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (hasHistory)
        {
            ImGui::Text(
                "Instruction %zu / %zu",
                m_selectedHistoryInstruction + 1,
                m_transitionHistories[0].size()
            );
        }
        else
        {
            ImGui::TextUnformatted(
                "Instruction - / 0"
            );
        }

        ImGui::SameLine();

        const bool canNext =
            hasHistory &&
            m_selectedHistoryInstruction + 1 <
            m_transitionHistories[0].size();

        if (!canNext)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button(">"))
        {
            ++m_selectedHistoryInstruction;

            m_scrollToSelectedHistoryInstruction =
                true;
        }

        if (!canNext)
        {
            ImGui::EndDisabled();
        }
    }

    void TransitionTracking::drawRecorder(
        const char* targetText
    )
    {
        if (drawRecordButton())
        {
            if (recording())
            {
                char* end = nullptr;

                const unsigned long long targetAddress =
                    std::strtoull(
                        targetText,
                        &end,
                        0
                    );

                if (end != targetText &&
                    *end == '\0')
                {
                    startRecording(
                        static_cast<size_t>(
                            targetAddress
                            )
                    );
                }
                else
                {
                    stopRecording();
                }
            }
        }

        ImGui::SameLine();

        ImGui::TextUnformatted(
            "Transition"
        );
    }

    TransitionTracking::TransitionTracking(
        MemoryScanner& scanner
    )
        :
        m_scanner(
            scanner
        )
    {

    }

    bool TransitionTracking::drawRecordButton()
    {
        return m_recordButton.draw();
    }

    bool TransitionTracking::recording() const
    {
        return m_recordButton.recording();
    }

    void TransitionTracking::stopRecording()
    {
        m_recordButton.stop();
    }

    void TransitionTracking::startRecording(
        size_t targetAddress
    )
    {
        m_lastTransitionCount = 0;

        m_lastTransitionChangeTime =
            ImGui::GetTime();

        m_transitionSeen = false;

        m_scanner.setReadTrackingTransitionTarget(
            targetAddress
        );

        m_scanner.clearTransitionTracking();

        m_scanner.startTransitionTracking();
    }

    void TransitionTracking::stop()
    {
        m_scanner.stopTransitionTracking();
    }

    void TransitionTracking::captureTransitions()
    {
        stop();

        getTransitions(
            m_transitions
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
            if (!getTransitionHistory(
                i,
                m_transitionHistories[i]
            ))
            {
                break;
            }

            getTransitionNextInstruction(
                i,
                m_transitionNextInstructions[i]
            );
        }

        getTransitionContexts(
            m_transitionContexts
        );

        getTransitionBytes(
            m_transitionBytes
        );
    }

    bool TransitionTracking::updateAutoCapture()
    {
        if (!recording())
        {
            return false;
        }

        size_t transitionCount = 0;

        if (!getTransitionCount(
            transitionCount
        ))
        {
            return false;
        }

        const double currentTime =
            ImGui::GetTime();

        if (transitionCount !=
            m_lastTransitionCount)
        {
            m_lastTransitionCount =
                transitionCount;

            m_lastTransitionChangeTime =
                currentTime;

            if (transitionCount > 0)
            {
                m_transitionSeen =
                    true;
            }
        }

        constexpr double captureIdleTime =
            0.5;

        if (m_transitionSeen &&
            currentTime -
            m_lastTransitionChangeTime >=
            captureIdleTime)
        {
            captureTransitions();

            stopRecording();

            return true;
        }

        return false;
    }

    bool TransitionTracking::getTransitionCount(
        size_t& count
    ) const
    {
        return m_scanner.getReadTrackingTransitionCount(
            count
        );
    }

    bool TransitionTracking::getTransitions(
        std::vector<std::pair<size_t, size_t>>& transitions
    ) const
    {
        return m_scanner.getReadTrackingTransitions(
            transitions
        );
    }

    bool TransitionTracking::getTransitionHistory(
        size_t index,
        std::vector<RuntimeInstruction>& history
    ) const
    {
        return m_scanner.getReadTrackingTransitionHistory(
            index,
            history
        );
    }

    bool TransitionTracking::getTransitionNextInstruction(
        size_t index,
        RuntimeInstruction& instruction
    ) const
    {
        return m_scanner.getReadTrackingTransitionNextInstruction(
            index,
            instruction
        );
    }

    bool TransitionTracking::getTransitionContexts(
        std::vector<std::pair<uint16_t, uint16_t>>& contexts
    ) const
    {
        return m_scanner.getReadTrackingTransitionContexts(
            contexts
        );
    }

    bool TransitionTracking::getTransitionBytes(
        std::vector<std::array<uint8_t, 16>>& bytes
    ) const
    {
        return m_scanner.getReadTrackingTransitionBytes(
            bytes
        );
    }
}