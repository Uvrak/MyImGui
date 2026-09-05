#include "TrackingWindow.h"

#include "imgui.h"
#include "Zydis/Zydis.h"

#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <windows.h>
#include <commdlg.h>
#include <algorithm>

namespace
{
    const char* mm3ClassName(
        uint8_t classId
    )
    {
        static const char* names[] =
        {
            "knight",
            "paladin",
            "archer",
            "cleric",
            "sorcerer",
            "robber",
            "ninja",
            "barbarian",
            "druid",
            "ranger"
        };

        if (classId >=
            IM_ARRAYSIZE(names))
        {
            return "unknown";
        }

        return names[classId];
    }
}

namespace DosBoxMemoryTools
{
    TrackingWindow::TrackingWindow(
        MemoryScanner& scanner,
        const std::string& gameId
    )
        :
        m_scanner(
            scanner
        ),
        m_gameId(
            gameId
        ),
        m_traceTracking(
            scanner,
            gameId
        ),
        m_memoryWriteTracker(
            scanner
        )
    {
        
    }

    TrackingWindow::~TrackingWindow()
    {
      
    }

    void TrackingWindow::draw(
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
            "Tracking",
            isOpen
        ))
        {
            ImGui::End();
            return;
        }

        drawNavigation();

        // Handle load requests forwarded from UI buttons
        if (m_loadTraceARequested)
        {
            m_loadTraceARequested = false;
            m_traceComparisonWindow.openAndLoadTrace(true);
        }

        if (m_loadTraceBRequested)
        {
            m_loadTraceBRequested = false;
            m_traceComparisonWindow.openAndLoadTrace(false);
        }

        // Handle save requests
        if (m_saveTraceARequested)
        {
            m_saveTraceARequested = false;
            m_traceComparisonWindow.openAndSaveTrace(true);
        }

        if (m_saveTraceBRequested)
        {
            m_saveTraceBRequested = false;
            m_traceComparisonWindow.openAndSaveTrace(false);
        }

        ImGui::Separator();

        if (ImGui::BeginTabBar(
            "TrackingTabs"
        ))
        {
            if (ImGui::BeginTabItem(
                "Trace"
            ))
            {
                m_activeTab =
                    TrackingTab::Trace;

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(
                "Trans"
            ))
            {
                m_activeTab =
                    TrackingTab::Trans;

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(
                "Exec"
            ))
            {
                m_activeTab =
                    TrackingTab::Exec;

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(
                "MemWr"
            ))
            {
                m_activeTab =
                    TrackingTab::MemWr;

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        if (m_activeTab !=
            TrackingTab::Trace)
        {
            drawRecorder();
        }

        ImGui::Separator();

        ImGui::BeginChild(
            "TrackingContent",
            ImVec2(0.0f, 0.0f),
            false,
            ImGuiWindowFlags_HorizontalScrollbar
        );
        {
            switch (m_activeTab)
            {
            case TrackingTab::Trace:
                m_traceTracking.draw();

                {
                    const auto& liveTrace =
                        m_traceTracking.m_trace;

                    if (!m_traceComparisonWindow.hasLoadedTraceA() &&
                        !liveTrace.empty() &&
                        m_traceComparisonWindow.traceA().size() !=
                        liveTrace.size())
                    {
                        m_traceComparisonWindow.setTraceA(
                            liveTrace
                        );
                    }
                }
                break;

            case TrackingTab::Trans:
                drawTransitions();
                break;

            case TrackingTab::Exec:
                drawExecutionCapture();
                break;

            case TrackingTab::MemWr:
                m_memoryWriteTracker.draw(
                    m_targetText,
                    sizeof(m_targetText)
                );
                break;
            }
        }

        ImGui::EndChild();
        ImGui::End();

        }
    

        void TrackingWindow::saveSession() const
        {
            m_traceTracking.saveSession();
        }

    void TrackingWindow::setGameId(
        const std::string& gameId
    )
    {
        if (m_gameId == gameId)
        {
            return;
        }

        m_gameId =
            gameId;

        m_traceTracking.setGameId(
            gameId
        );

    }

    void TrackingWindow::drawNavigation()
    {
        if (m_activeTab ==
            TrackingTab::Trace)
        {
            ImGui::SameLine();

            if (ImGui::Button(
                "Load A"
            ))
            {
                m_loadTraceARequested =
                    true;
            }

            ImGui::SameLine();

            if (ImGui::Button(
                "Load B"
            ))
            {
                m_loadTraceBRequested =
                    true;
            }

            // Draw Side-by-side checkbox next to Load B in the top navigation
            ImGui::SameLine();
            {
                bool side = m_traceComparisonWindow.sideBySide();
                if (ImGui::Checkbox("Side-by-side##TraceNav", &side))
                {
                    m_traceComparisonWindow.setSideBySide(side);
                }
            }

            // (Save buttons removed; saving is available in the recorder toolbar)


            ImGui::SameLine();

            const bool canCompare =
                !m_traceComparisonWindow.traceA().empty() &&
                !m_traceComparisonWindow.traceB().empty();

            bool tracesEqual = false;

            if (!m_traceComparisonWindow.traceA().empty() &&
                m_traceComparisonWindow.traceA().size() == m_traceComparisonWindow.traceB().size())
            {
                tracesEqual = true;

                for (size_t i = 0;
                    i < m_traceComparisonWindow.traceA().size();
                    ++i)
                {
                    if (compareTraceInstructions(
                        m_traceComparisonWindow.traceA()[i],
                        m_traceComparisonWindow.traceB()[i]
                    ).any())
                    {
                        tracesEqual = false;
                        break;
                    }
                }

            }
            if (!canCompare)
            {
                ImGui::BeginDisabled();
            }

            // Diagnostic: show counts of loaded traces next to Compare
            ImGui::SameLine();
            ImGui::Text("sizes: A=%zu B=%zu", m_traceComparisonWindow.traceA().size(), m_traceComparisonWindow.traceB().size());

            if (ImGui::Button(
                "Compare"
            ))
            {
                m_traceComparisonWindow.setSelectedTraceIndex(
                    static_cast<size_t>(-1)
                );

                for (size_t i = 0;
                    i < m_traceComparisonWindow.traceA().size() &&
                    i < m_traceComparisonWindow.traceB().size();
                    ++i)
                {
                    if (compareTraceInstructions(
                        m_traceComparisonWindow.traceA()[i],
                        m_traceComparisonWindow.traceB()[i]
                    ).any())
                    {
                        m_traceComparisonWindow.setSelectedTraceIndex(i);

                        m_traceComparisonWindow.setScrollToSelectedTrace(true);

                        // ensure side-by-side view is enabled when user clicks Compare
                        m_traceComparisonWindow.setSideBySide(true);

                        // immediate debug feedback in UI to help diagnose why Compare may appear to do nothing
                        ImGui::SameLine();
                        ImGui::Text("Compare clicked: sel=%zu sideBySide=on A=%zu B=%zu", i, m_traceComparisonWindow.traceA().size(), m_traceComparisonWindow.traceB().size());

                        break;
                    }
                }
            }

            if (!canCompare)
            {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();

            if (!m_traceComparisonWindow.traceA().empty() &&
                !m_traceComparisonWindow.traceB().empty())
            {
                ImGui::Text(
                    "Is Equal: %s",
                    tracesEqual
                    ? "Yes"
                    : "No"
                );

                ImGui::SameLine();

                ImGui::Text(
                    "A: %zu  B: %zu",
                    m_traceComparisonWindow.traceA().size(),
                    m_traceComparisonWindow.traceB().size()
                );
            }
        }

        if (m_activeTab ==
            TrackingTab::Trace)
        {
            ImGui::Text(
                "A: %s",
                m_traceComparisonWindow.traceAFilename()[0] != '\0'
                ? m_traceComparisonWindow.traceAFilename()
                : "<not loaded>"
            );

            ImGui::Text(
                "B: %s",
                m_traceComparisonWindow.traceBFilename()[0] != '\0'
                ? m_traceComparisonWindow.traceBFilename()
                : "<not loaded>"
            );
        }

        ImGui::SameLine();

        
        if (m_activeTab ==
            TrackingTab::Trans)

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
    }

    void TrackingWindow::drawRecorder()
    {
        if (m_activeTab ==
            TrackingTab::Trans)
        {
            if (m_transitionRecordButton.draw())
            {
                if (m_transitionRecordButton.recording())
                {
                    char* end = nullptr;

                    const unsigned long long targetAddress =
                        std::strtoull(
                            m_targetText,
                            &end,
                            0
                        );

                    if (end != m_targetText &&
                        *end == '\0')
                    {
                        m_scanner.setReadTrackingTransitionTarget(
                            static_cast<size_t>(
                                targetAddress
                                )
                        );

                        m_scanner.clearTransitionTracking();

                        m_lastTransitionCount = 0;

                        m_lastTransitionChangeTime =
                            ImGui::GetTime();

                        m_transitionSeen = false;

                        m_scanner.startTransitionTracking();
                    }
                    else
                    {
                        m_transitionRecordButton.stop();
                    }
                }
            }

            ImGui::SameLine();

            ImGui::TextUnformatted(
                "Transition"
            );
        }
    }

    

    void TrackingWindow::drawTransitions()
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

        if (m_transitionRecordButton.recording())
        {
            size_t transitionCount = 0;

            if (m_scanner.getReadTrackingTransitionCount(
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

                constexpr double captureIdleTime =
                    0.5;

                if (m_transitionSeen &&
                    currentTime -
                    m_lastTransitionChangeTime >=
                    captureIdleTime)
                {
                    captureTransitions();

                    m_transitionRecordButton.stop();
                }
            }
        }

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
                        static_cast<int>(historyIndex)
                    );

                    if (ImGui::Selectable(
                        addressText,
                        historyIndex == m_selectedHistoryInstruction,
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
                            m_targetText,
                            sizeof(m_targetText),
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
                }
            }

        }
    }

    void TrackingWindow::captureTransitions()
    {
        m_scanner.stopTransitionTracking();

        m_scanner.getReadTrackingTransitions(
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
            if (!m_scanner.
                getReadTrackingTransitionHistory(
                    i,
                    m_transitionHistories[i]
                ))
            {
                break;
            }

            m_scanner.
                getReadTrackingTransitionNextInstruction(
                    i,
                    m_transitionNextInstructions[i]
                );
        }

        m_scanner.getReadTrackingTransitionContexts(
            m_transitionContexts
        );

        m_scanner.getReadTrackingTransitionBytes(
            m_transitionBytes
        );
    }

    void TrackingWindow::drawExecutionCapture()
    {
         if (m_executionRecordButton.draw())
        {
            if (m_executionRecordButton.recording())
            {
                char* end = nullptr;

                const unsigned long long targetAddress =
                    std::strtoull(
                        m_targetText,
                        &end,
                        0
                    );

                if (end != m_targetText &&
                    *end == '\0')
                {
                    if (m_scanner.setExecutionCaptureTarget(
                        static_cast<size_t>(
                            targetAddress
                            )
                    ))
                    {
                        m_executionCaptureHit =
                            false;
                    }
                }
            }
        }

        bool executionHit = false;

        if (m_scanner.getExecutionCaptureHit(
            executionHit
        ))
        {
            if (executionHit &&
                m_executionRecordButton.recording())
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

                    m_executionRecordButton.stop();
                }
            }
        }

        if (m_scanner.getExecutionCaptureHit(
            executionHit
        ))
        {
            ImGui::Text(
                "State: %s",
                executionHit
                ? "HIT"
                : m_executionRecordButton.recording()
                ? "ARMED / NO HIT"
                : "IDLE"
            );
        }
        else
        {
            ImGui::TextDisabled(
                "State unavailable."
            );
        }

        ImGui::Text(
            "Scanner status: %s",
            m_scanner.status().c_str()
        );
        
        if (m_executionCaptureHit)
        {
            ImGui::Separator();

            ImGui::Text(
                "Address: 0x%zX",
                m_executionCapture.address
            );

            ImGui::Text(
                "CS:IP %04X:%04X",
                static_cast<unsigned int>(
                    m_executionCapture.cs
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.ip
                    )
            );

            ImGui::Text(
                "AX=%04X BX=%04X CX=%04X DX=%04X",
                static_cast<unsigned int>(
                    m_executionCapture.registers.ax
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.registers.bx
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.registers.cx
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.registers.dx
                    )
            );

            ImGui::Text(
                "SI=%04X DI=%04X BP=%04X SP=%04X",
                static_cast<unsigned int>(
                    m_executionCapture.registers.si
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.registers.di
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.registers.bp
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.registers.sp
                    )
            );

            ImGui::Text(
                "DS=%04X ES=%04X SS=%04X",
                static_cast<unsigned int>(
                    m_executionCapture.registers.ds
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.registers.es
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.registers.ss
                    )
            );
        }

        ImGui::Separator();

        // Draw the trace comparison toolbar at top of the trace tab
        m_traceComparisonWindow.drawToolbar();

        ImGui::TextUnformatted(
            "Stack at SS:SP:"
        );

        for (size_t i = 0;
            i < m_executionCapture.stackBytes.size();
            i += 8)
        {
            ImGui::Text(
                "+%02zX: %02X %02X %02X %02X %02X %02X %02X %02X",
                i,
                static_cast<unsigned int>(
                    m_executionCapture.stackBytes[i + 0]
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.stackBytes[i + 1]
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.stackBytes[i + 2]
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.stackBytes[i + 3]
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.stackBytes[i + 4]
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.stackBytes[i + 5]
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.stackBytes[i + 6]
                    ),
                static_cast<unsigned int>(
                    m_executionCapture.stackBytes[i + 7]
                    )
            );
        }
    }

    void TrackingWindow::drawTraceComparison()
    {
        // Delegate to TraceComparisonWindow (new)
        m_traceComparisonWindow.draw(
            m_traceComparisonWindow.traceA(),
            m_traceComparisonWindow.traceB(),
            m_targetText,
            sizeof(m_targetText),
            [this](const RuntimeInstruction& instr, size_t& phys) { return tryGetPhysicalMemoryAddress(instr, phys); }
        );
    }

    void TrackingWindow::drawTraceComparisonInstruction(
        const RuntimeInstruction& instruction,
        const TraceInstructionDifference& difference,
        bool highlightChanges
    )
    {
        char instructionText[256] =
            "<decode failed>";

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
            "0x%zX  CS:IP %04X:%04X",
            instruction.address,
            static_cast<unsigned int>(
                instruction.cs
                ),
            static_cast<unsigned int>(
                instruction.ip
                )
        );

        if (decoded)
        {
            ImGui::PushID(
                &instruction
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
    size_t physicalAddress = 0;

    if (tryGetPhysicalMemoryAddress(
        instruction,
        physicalAddress
    ))
    {
        std::snprintf(
            m_targetText,
            sizeof(m_targetText),
            "0x%zX",
            physicalAddress
        );
    }
}

            ImGui::PopID();

        }
        else
        {
            ImGui::TextDisabled(
                "<decode failed>"
            );
        }

        auto drawRegister =
            [&](const char* name,
                uint16_t value,
                bool changed)
            {
                if (highlightChanges &&
                    changed)
                {
                    ImGui::PushStyleColor(
                        ImGuiCol_Text,
                        ImVec4(
                            1.0f,
                            1.0f,
                            0.0f,
                            1.0f
                        )
                    );
                }

                ImGui::Text(
                    "%s=%04X",
                    name,
                    static_cast<unsigned int>(
                        value
                        )
                );

                if (highlightChanges &&
                    changed)
                {
                    ImGui::PopStyleColor();
                }
            };

        drawRegister(
            "AX",
            instruction.registers.ax,
            difference.ax
        );

        ImGui::SameLine();

        drawRegister(
            "BX",
            instruction.registers.bx,
            difference.bx
        );

        ImGui::SameLine();

        drawRegister(
            "CX",
            instruction.registers.cx,
            difference.cx
        );

        ImGui::SameLine();

        drawRegister(
            "DX",
            instruction.registers.dx,
            difference.dx
        );

        drawRegister(
            "AX",
            instruction.registers.ax,
            difference.ax
        );

        ImGui::SameLine();

        drawRegister(
            "BX",
            instruction.registers.bx,
            difference.bx
        );

        ImGui::SameLine();

        drawRegister(
            "CX",
            instruction.registers.cx,
            difference.cx
        );

        ImGui::SameLine();

        drawRegister(
            "DX",
            instruction.registers.dx,
            difference.dx
        );

        drawRegister(
            "SI",
            instruction.registers.si,
            difference.si
        );

        ImGui::SameLine();

        drawRegister(
            "DI",
            instruction.registers.di,
            difference.di
        );

        ImGui::SameLine();

        drawRegister(
            "BP",
            instruction.registers.bp,
            difference.bp
        );

        ImGui::SameLine();

        drawRegister(
            "SP",
            instruction.registers.sp,
            difference.sp
        );

        drawRegister(
            "DS",
            instruction.registers.ds,
            difference.ds
        );

        ImGui::SameLine();

        drawRegister(
            "ES",
            instruction.registers.es,
            difference.es
        );

        ImGui::SameLine();

        drawRegister(
            "SS",
            instruction.registers.ss,
            difference.ss
        );
    }

    // Trace loading moved to TraceComparisonWindow::loadTraceFromFile
    
    bool TrackingWindow::tryGetPhysicalMemoryAddress(
        const RuntimeInstruction& instruction,
        size_t& physicalAddress
    ) const
    {
        physicalAddress = 0;

        if (instruction.registers.ds !=
            0x2053)
        {
            return false;
        }

        ZydisDecoder decoder;

        if (!ZYAN_SUCCESS(
            ZydisDecoderInit(
                &decoder,
                ZYDIS_MACHINE_MODE_LEGACY_16,
                ZYDIS_STACK_WIDTH_16
            )
        ))
        {
            return false;
        }

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
            return false;
        }

        for (uint8_t operandIndex = 0;
            operandIndex <
            decodedInstruction.operand_count_visible;
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

            uint16_t offset = 0;

            switch (operand.mem.base)
            {
            case ZYDIS_REGISTER_BX:
                offset =
                    instruction.registers.bx;
                break;

            case ZYDIS_REGISTER_SI:
                offset =
                    instruction.registers.si;
                break;

            case ZYDIS_REGISTER_DI:
                offset =
                    instruction.registers.di;
                break;

            case ZYDIS_REGISTER_BP:
                offset =
                    instruction.registers.bp;
                break;

            default:
                continue;
            }

            offset =
                static_cast<uint16_t>(
                    offset +
                    static_cast<uint16_t>(
                        operand.mem.disp.value
                        )
                    );

            physicalAddress =
                (static_cast<size_t>(
                    instruction.registers.ds
                    ) << 4) +
                offset;

            return true;
        }

        return false;
    }
}


