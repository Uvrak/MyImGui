#include "InstructionTransitionTrackerNavigationWindow.h"

#include <cstdlib>
#include <utility>

#include "imgui.h"



namespace DosBoxMemoryTools
{
    void InstructionTransitionTrackerNavigationWindow::
        setExecutionCaptureCallback(
            std::function<void()> callback
        )
    {
        m_executionCaptureCallback =
            std::move(callback);
    }

    void InstructionTransitionTrackerNavigationWindow::draw(
        bool* isOpen
    )
    {
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

        if (m_targetText &&
            m_targetTextSize > 0)
        {
            ImGui::SetNextItemWidth(
                120.0f
            );

            ImGui::InputText(
                "Target",
                m_targetText,
                m_targetTextSize
            );
        }

        ImGui::PushID(
            "TransitionRecord"
        );

        const bool transitionClicked =
            m_transitionRecordButton.draw();

        ImGui::PopID();

        ImGui::SameLine();

        ImGui::TextUnformatted(
            "Transition"
        );

        if (transitionClicked)
        {
            if (!m_scanner ||
                !m_targetText)
            {
                m_transitionRecordButton.stop();
            }
            else if (!m_transitionRecordButton.recording())
            {
                if (m_captureCallback)
                {
                    m_captureCallback();
                }
            }
            else
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
                    m_scanner->
                        setReadTrackingTransitionTarget(
                            static_cast<size_t>(
                                targetAddress
                                )
                        );

                    m_scanner->
                        clearTransitionTracking();

                    m_lastTransitionCount = 0;

                    m_lastTransitionChangeTime =
                        ImGui::GetTime();

                    m_transitionSeen = false;

                    m_scanner->
                        startTransitionTracking();
                }
                else
                {
                    m_transitionRecordButton.stop();
                }
            }
        }
            
        

        if (m_scanner &&
            m_transitionRecordButton.recording())
        {
            size_t transitionCount = 0;

            if (m_scanner->
                getReadTrackingTransitionCount(
                    transitionCount
                ))
            {
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
                        m_transitionSeen = true;
                    }
                }

                constexpr double
                    captureIdleTime = 0.5;

                if (m_transitionSeen &&
                    currentTime -
                    m_lastTransitionChangeTime >=
                    captureIdleTime)
                {
                    if (m_captureCallback)
                    {
                        m_captureCallback();
                    }

                    m_transitionRecordButton.stop();

                    m_transitionSeen = false;
                }
            }
        }

        ImGui::PushID(
            "ExecutionRecord"
        );

        const bool executionClicked =
            m_executionRecordButton.draw();

		ImGui::PopID();
        ImGui::SameLine();

        ImGui::TextUnformatted(
            "Execution"
        );

        if (executionClicked)
        {
            if (!m_scanner ||
                !m_targetText)
            {
                m_executionRecordButton.stop();
            }
            else if (m_executionRecordButton.recording())
            {
                char* end = nullptr;

                const unsigned long long
                    targetAddress =
                    std::strtoull(
                        m_targetText,
                        &end,
                        0
                    );

                if (end !=
                    m_targetText &&
                    *end == '\0')
                {
                    m_scanner->
                        clearExecutionCapture();

                    m_scanner->
                        setExecutionCaptureTarget(
                            static_cast<size_t>(
                                targetAddress
                                )
                        );
                }
                else
                {
                    m_executionRecordButton.stop();
                }
            }
            else
            {
                m_scanner->
                    clearExecutionCapture();
            }
        }

        if (m_scanner &&
            m_executionRecordButton.recording())
        {
            bool executionHit = false;

            if (m_scanner->
                getExecutionCaptureHit(
                    executionHit
                ))
            {
                if (executionHit)
                {
                    if (m_executionCaptureCallback)
                    {
                        m_executionCaptureCallback();
                    }

                    m_executionRecordButton.stop();
                }
            }
        }

        ImGui::PushID(
            "MemoryWriteRecord"
        );

        const bool memoryWriteClicked =
            m_memoryWriteRecordButton.draw();

        ImGui::PopID();

        ImGui::SameLine();

        ImGui::TextUnformatted(
            "Memory Write"
        );

        if (memoryWriteClicked)
        {
            if (!m_scanner ||
                !m_targetText)
            {
                m_memoryWriteRecordButton.stop();
            }
            else if (m_memoryWriteRecordButton.recording())
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
                    m_scanner->
                        clearMemoryWriteWatch();

                    m_scanner->
                        setMemoryWriteWatchTarget(
                            static_cast<size_t>(
                                targetAddress
                                )
                        );
                }
                else
                {
                    m_memoryWriteRecordButton.stop();
                }
            }
            else
            {
                m_scanner->
                    clearMemoryWriteWatch();
            }
        }

        if (m_scanner &&
            m_memoryWriteRecordButton.recording())
        {
            bool hit = false;

            if (m_scanner->
                getMemoryWriteWatchHit(
                    hit
                ) &&
                hit)
            {
                m_memoryWriteRecordButton.stop();
            }
        }

        if (m_selectedHistoryInstruction &&
            m_transitionHistories &&
            !m_transitionHistories->empty() &&
            !(*m_transitionHistories)[0].empty())
        {
            
                const auto& history =

                (*m_transitionHistories)[0];

        if (*m_selectedHistoryInstruction >=
            history.size())
        {
            *m_selectedHistoryInstruction =
                history.size() - 1;
        }
        if (ImGui::Button(
            "<##PreviousInstruction"
        ))
        {
            if (*m_selectedHistoryInstruction > 0)
            {
                --(*m_selectedHistoryInstruction);

                m_historyNavigation = true;
            }
        }

            ImGui::SameLine();

            ImGui::Text(
                "Instruction %zu / %zu",
                *m_selectedHistoryInstruction + 1,
                history.size()
            );

            ImGui::SameLine();

            if (ImGui::Button(
                ">##NextInstruction"
            ))
            {
                if (*m_selectedHistoryInstruction + 1 <
                    history.size())
                {
                    ++(*m_selectedHistoryInstruction);

                    m_historyNavigation = true;
                }
            }

            const RuntimeInstruction& instruction =
                history[*m_selectedHistoryInstruction];

            ImGui::Text(
                "0x%zX",
                instruction.address
            );
        }
        else
        {
            ImGui::TextDisabled(
                "No runtime history."
            );
        }

        m_window.end();


        if (isOpen)
        {
            *isOpen =
                m_window.isOpen();
        }
    }

    void InstructionTransitionTrackerNavigationWindow::
        setScanner(
            MemoryScanner* scanner
        )
    {
        m_scanner =
            scanner;
    }

    void InstructionTransitionTrackerNavigationWindow::
        setTargetText(
            char* targetText,
            size_t targetTextSize
        )
    {
        m_targetText =
            targetText;

        m_targetTextSize =
            targetTextSize;
    }

    void InstructionTransitionTrackerNavigationWindow::
        setTransitionHistories(
            std::vector<
            std::vector<RuntimeInstruction>
            >* transitionHistories
        )
    {
        m_transitionHistories =
            transitionHistories;
    }

    void InstructionTransitionTrackerNavigationWindow::
        setCaptureCallback(
            std::function<void()> callback
        )
    {
        m_captureCallback =
            std::move(callback);
    }

}