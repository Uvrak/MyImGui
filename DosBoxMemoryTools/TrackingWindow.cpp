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
    TrackingWindow::
        TrackingWindow(
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
        m_memoryWriteTracker(
            scanner
        )
    {
        loadSession();
    }

    TrackingWindow::~TrackingWindow()
    {
        saveSession();
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

        drawRecorder();

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
                drawTrace();
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

        if (m_activeTab ==
            TrackingTab::Trace)
            if (m_saveTraceRequested)
            {
                m_saveTraceRequested = false;

                char filename[4096] = {};

                size_t partySlot = 0;

                constexpr uint16_t
                    firstCharacterOffset =
                    0xB9E2;

                constexpr uint16_t
                    characterStride =
                    0x012F;

                for (auto traceIt = m_trace.rbegin();
                    traceIt != m_trace.rend();
                    ++traceIt)
                {
                    const uint16_t bx =
                        traceIt->registers.bx;

                    if (bx < firstCharacterOffset)
                    {
                        continue;
                    }

                    const uint16_t difference =
                        static_cast<uint16_t>(
                            bx - firstCharacterOffset
                            );

                    if (difference %
                        characterStride != 0)
                    {
                        continue;
                    }

                    const size_t candidateSlot =
                        difference /
                        characterStride +
                        1;

                    if (candidateSlot < 1 ||
                        candidateSlot > 6)
                    {
                        continue;
                    }

                    partySlot =
                        candidateSlot;

                    break;
                }

                uint8_t classId = 0;
                uint8_t level = 0;

                bool hasCharacterInfo = false;

                if (partySlot != 0 &&
                    m_scanner.refreshMemory())
                {
                    constexpr size_t
                        firstCharacterRecord =
                        0x2BF12;

                    constexpr size_t
                        characterStride =
                        0x012F;

                    const size_t characterRecord =
                        firstCharacterRecord +
                        (partySlot - 1) *
                        characterStride;

                    hasCharacterInfo =
                        m_scanner.readCurrentValue(
                            characterRecord + 0x13,
                            classId
                        ) &&
                        m_scanner.readCurrentValue(
                            characterRecord + 0x23,
                            level
                        );
                }

                if (partySlot != 0 &&
                    hasCharacterInfo)
                {
                    std::snprintf(
                        filename,
                        sizeof(filename),
                        "%s_slot%zu_%s_level%u",
                        m_targetText,
                        partySlot,
                        mm3ClassName(
                            classId
                        ),
                        static_cast<unsigned int>(
                            level
                            )
                    );
                }
                else
                {
                    strncpy_s(
                        filename,
                        sizeof(filename),
                        m_targetText,
                        _TRUNCATE
                    );
                }

                OPENFILENAMEA dialog{};
                dialog.lStructSize =
                    sizeof(dialog);

                dialog.lpstrFile =
                    filename;

                dialog.nMaxFile =
                    sizeof(filename);

                dialog.lpstrFilter =
                    "Execution Trace (*.trace)\0*.trace\0"
                    "All Files (*.*)\0*.*\0";

                dialog.nFilterIndex = 1;

                dialog.lpstrDefExt =
                    "trace";

                dialog.Flags =
                    OFN_PATHMUSTEXIST |
                    OFN_NOCHANGEDIR |
                    OFN_OVERWRITEPROMPT;

                if (GetSaveFileNameA(
                    &dialog
                ))
                {
                    saveTraceToFile(
                        filename
                    );
                }
            }

            if (m_loadTraceARequested)
            {
                m_loadTraceARequested = false;

                char filename[4096] = {};

                OPENFILENAMEA dialog{};
                dialog.lStructSize =
                    sizeof(dialog);

                dialog.lpstrFile =
                    filename;

                dialog.nMaxFile =
                    sizeof(filename);

                dialog.lpstrFilter =
                    "Execution Trace (*.trace)\0*.trace\0"
                    "All Files (*.*)\0*.*\0";

                dialog.nFilterIndex = 1;

                dialog.lpstrDefExt =
                    "trace";

                dialog.Flags =
                    OFN_FILEMUSTEXIST |
                    OFN_PATHMUSTEXIST |
                    OFN_NOCHANGEDIR;

                if (GetOpenFileNameA(
                    &dialog
                ))
                {
                    if (loadTraceFromFile(
                        filename,
                        m_traceA
                    ))
                    {
                        strncpy_s(
                            m_traceAFilename,
                            sizeof(m_traceAFilename),
                            filename,
                            _TRUNCATE
                        );

                        m_compareTraces = false;
                    }
                }
            }
            if (m_loadTraceRequested)
            {
                m_loadTraceRequested = false;

                char filename[4096] = {};

                OPENFILENAMEA dialog{};
                dialog.lStructSize =
                    sizeof(dialog);

                dialog.lpstrFile =
                    filename;

                dialog.nMaxFile =
                    sizeof(filename);

                dialog.lpstrFilter =
                    "Execution Trace (*.trace)\0*.trace\0"
                    "All Files (*.*)\0*.*\0";

                dialog.nFilterIndex = 1;

                dialog.lpstrDefExt =
                    "trace";

                dialog.Flags =
                    OFN_FILEMUSTEXIST |
                    OFN_PATHMUSTEXIST |
                    OFN_NOCHANGEDIR;

                if (GetOpenFileNameA(
                    &dialog
                ))
                {
                    if (loadTraceFromFile(
                        filename,
                        m_trace
                    ))
                    {
                        OutputDebugStringA(
                            "TRACE LOAD OK\n"
                        );

                        m_selectedTraceIndex =
                            static_cast<size_t>(-1);

                        m_scrollToSelectedTrace =
                            false;
                    }
                    else
                    {
                        OutputDebugStringA(
                            "TRACE LOAD FAILED\n"
                        );
                    }
                }

                
            }

            if (m_loadTraceBRequested)
            {
                m_loadTraceBRequested = false;

                char filename[4096] = {};

                OPENFILENAMEA dialog{};
                dialog.lStructSize =
                    sizeof(dialog);

                dialog.lpstrFile =
                    filename;

                dialog.nMaxFile =
                    sizeof(filename);

                dialog.lpstrFilter =
                    "Execution Trace (*.trace)\0*.trace\0"
                    "All Files (*.*)\0*.*\0";

                dialog.nFilterIndex = 1;

                dialog.lpstrDefExt =
                    "trace";

                dialog.Flags =
                    OFN_FILEMUSTEXIST |
                    OFN_PATHMUSTEXIST |
                    OFN_NOCHANGEDIR;

                if (GetOpenFileNameA(
                    &dialog
                ))
                {
                    if (loadTraceFromFile(
                        filename,
                        m_traceB
                    ))
                    {
                        strncpy_s(
                            m_traceBFilename,
                            sizeof(m_traceBFilename),
                            filename,
                            _TRUNCATE
                        );

                        m_compareTraces = false;
                    }
                }
            }

        }
    

    void TrackingWindow::saveSession() const
    {
        if (m_gameId.empty())
        {
            return;
        }

        std::filesystem::create_directories(
            "settings"
        );

        const std::string filename =
            "../settings/execution_trace_session_" +
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
            "ExecutionTraceSession 1\n";

        file <<
            "Target\n";

        file <<
            m_targetText <<
            '\n';

        const std::string traceFilename =
            "../settings/execution_trace_last_" +
            m_gameId +
            ".trace";

        saveTraceToFile(
            traceFilename
        );
    }

    void TrackingWindow::loadSession()
    {
        if (m_gameId.empty())
        {
            return;
        }

        const std::string filename =
            "../settings/execution_trace_session_" +
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
            "ExecutionTraceSession 1")
        {
            return;
        }

        std::string section;

        if (!(file >> section) ||
            section != "Target")
        {
            return;
        }

        std::string target;

        if (!(file >> target))
        {
            return;
        }

        strncpy_s(
            m_targetText,
            sizeof(m_targetText),
            target.c_str(),
            _TRUNCATE
        );

        const std::string traceFilename =
            "../settings/execution_trace_last_" +
            m_gameId +
            ".trace";

        loadTraceFromFile(
            traceFilename,
            m_trace
        );
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

        loadSession();

    }

    void TrackingWindow::drawNavigation()
    {
        ImGui::SetNextItemWidth(
            120.0f
        );

        ImGui::InputText(
            "Target",
            m_targetText,
            sizeof(m_targetText)
        );

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            saveSession();
        }

        ImGui::SameLine();

        if (ImGui::Button("Load"))
        {
            m_loadTraceRequested = true;
        }

        ImGui::SameLine();

        if (ImGui::Button("Save"))
        {
            m_saveTraceRequested = true;
        }

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

            ImGui::SameLine();

            const bool canCompare =
                !m_traceA.empty() &&
                !m_traceB.empty();

            bool tracesEqual = false;

            if (!m_traceA.empty() &&
                m_traceA.size() == m_traceB.size())
            {
                tracesEqual = true;

                for (size_t i = 0;
                    i < m_traceA.size();
                    ++i)
                {
                    if (compareTraceInstructions(
                        m_traceA[i],
                        m_traceB[i]
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

            if (ImGui::Button(
                "Compare"
            ))
            {
                m_compareTraces =
                    true;

                m_selectedTraceIndex =
                    static_cast<size_t>(-1);

                for (size_t i = 0;
                    i < m_traceA.size() &&
                    i < m_traceB.size();
                    ++i)
                {
                    if (compareTraceInstructions(
                        m_traceA[i],
                        m_traceB[i]
                    ).any())
                    {
                        m_selectedTraceIndex = i;

                        m_scrollToSelectedTrace =
                            true;

                        break;
                    }
                }
            }

            if (!canCompare)
            {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();

            if (!m_traceA.empty() &&
                !m_traceB.empty())
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
                    m_traceA.size(),
                    m_traceB.size()
                );
            }
        }

        if (m_activeTab ==
            TrackingTab::Trace)
        {
            ImGui::Text(
                "A: %s",
                m_traceAFilename[0] != '\0'
                ? m_traceAFilename
                : "<not loaded>"
            );

            ImGui::Text(
                "B: %s",
                m_traceBFilename[0] != '\0'
                ? m_traceBFilename
                : "<not loaded>"
            );
        }

        ImGui::SameLine();

        if (m_activeTab ==
            TrackingTab::Trace)

        {
            if (!m_trace.empty() &&
                (m_selectedTraceIndex ==
                    static_cast<size_t>(-1) ||
                    m_selectedTraceIndex >=
                    m_trace.size()))
            {
                m_selectedTraceIndex =
                    m_trace.size() - 1;

                m_scrollToSelectedTrace =
                    true;
            }

            const bool hasSelection =
                !m_trace.empty() &&
                m_selectedTraceIndex !=
                static_cast<size_t>(-1) &&
                m_selectedTraceIndex <
                m_trace.size();

            const bool canPrevious =
                hasSelection &&
                m_selectedTraceIndex > 0;

            if (!canPrevious)
            {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("<"))
            {
                --m_selectedTraceIndex;

                m_scrollToSelectedTrace =
                    true;
            }

            if (!canPrevious)
            {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();

            if (hasSelection)
            {
                ImGui::Text(
                    "Instruction %zu / %zu",
                    m_selectedTraceIndex + 1,
                    m_trace.size()
                );
            }
            else
            {
                ImGui::Text(
                    "Instruction - / %zu",
                    m_trace.size()
                );
            }

            ImGui::SameLine();

            const bool canNext =
                hasSelection &&
                m_selectedTraceIndex + 1 <
                m_trace.size();

            if (!canNext)
            {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button(">"))
            {
                ++m_selectedTraceIndex;

                m_scrollToSelectedTrace =
                    true;
            }

            if (!canNext)
            {
                ImGui::EndDisabled();
            }
        }
        else if (m_activeTab ==
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
            TrackingTab::Trace)
        {
            if (m_recordButton.draw())
            {
                if (m_recordButton.recording())
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
                        m_trace.clear();

                        m_selectedTraceIndex =
                            static_cast<size_t>(-1);

                        m_scrollToSelectedTrace =
                            false;

                        m_traceWasArmedOrActive =
                            false;

                        m_scanner.setReadTraceTarget(
                            static_cast<size_t>(
                                targetAddress
                                )
                        );
                    }
                    else
                    {
                        m_recordButton.stop();
                    }
                }
                else
                {
                    m_scanner.setReadTraceTarget(
                        0
                    );
                }
            }
        }
        else if (m_activeTab ==
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

    void TrackingWindow::drawTrace()
    {
        if (m_compareTraces)
        {
            drawTraceComparison();
            return;
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

        bool traceActive = false;
        bool traceArmed = false;

        const bool hasTraceActive =
            m_scanner.getReadTraceActive(
                traceActive
            );

        const bool hasTraceArmed =
            m_scanner.getReadTraceArmed(
                traceArmed
            );

        if (hasTraceActive &&
            hasTraceArmed)
        {
            if (traceActive ||
                traceArmed)
            {
                m_traceWasArmedOrActive = true;
            }

            if (!traceActive &&
                !traceArmed &&
                m_recordButton.recording() &&
                m_traceWasArmedOrActive)
            {
                loadTrace();

                if (!m_trace.empty())
                {
                    m_selectedTraceIndex =
                        m_trace.size() - 1;

                    m_scrollToSelectedTrace =
                        true;

                    std::snprintf(
                        m_targetText,
                        sizeof(m_targetText),
                        "0x%zX",
                        m_trace[m_selectedTraceIndex].address
                    );
                }

                m_recordButton.stop();

                m_traceWasArmedOrActive = false;
            }
        }

        ImGui::SameLine();

        if (m_selectedTraceIndex !=
            static_cast<size_t>(-1) &&
            m_selectedTraceIndex <
            m_trace.size())
        {
            ImGui::Text(
                "Found at trace #%zu - address 0x%zX",
                m_selectedTraceIndex + 1,
                m_trace[m_selectedTraceIndex].address
            );
        }
        else
        {
            ImGui::TextDisabled(
                "Not found"
            );
        }

        const std::vector<RuntimeInstruction>& displayedTrace =
            m_compareTraces
            ? m_traceA
            : m_trace;

        for (size_t i = 0;
            i < displayedTrace.size();
            ++i)
        {
            const RuntimeInstruction&
                instruction =
                displayedTrace[i];

            const bool hasComparison =
                m_compareTraces &&
                i < m_traceA.size() &&
                i < m_traceB.size();

            TraceInstructionDifference
                difference;

            if (hasComparison)
            {
                difference =
                    compareTraceInstructions(
                        m_traceA[i],
                        m_traceB[i]
                    );
            }
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

            ImGui::Separator();

            ImGui::Text(
                "%03zu",
                i + 1
            );

            ImGui::SameLine();

            char addressText[32];

            std::snprintf(
                addressText,
                sizeof(addressText),
                "0x%zX",
                instruction.address
            );

            ImGui::PushID(
                static_cast<int>(i)
            );

            if (hasComparison &&
                difference.any())
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

            if (ImGui::Selectable(
                addressText,
                i == m_selectedTraceIndex,
                ImGuiSelectableFlags_AllowDoubleClick,
                ImVec2(
                    ImGui::CalcTextSize(
                        addressText
                    ).x,
                    0.0f
                )
            ))
            {
                m_selectedTraceIndex = i;

                std::snprintf(
                    m_targetText,
                    sizeof(m_targetText),
                    "0x%zX",
                    instruction.address
                );
            }

            if (hasComparison &&
                difference.any())
            {
                ImGui::PopStyleColor();
            }

            ImGui::PopID();

            if (i == m_selectedTraceIndex &&
                m_scrollToSelectedTrace)
            {
                ImGui::SetScrollHereY(
                    0.5f
                );

                m_scrollToSelectedTrace =
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
                ImGui::Text(
                    "%02X",
                    static_cast<unsigned int>(
                        instruction.bytes[
                            byteIndex
                        ]
                        )
                );

                ImGui::SameLine();
            }

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

            auto drawRegister =
                [&](const char* name,
                    uint16_t value,
                    bool changed)
                {
                    if (hasComparison &&
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

                    if (hasComparison &&
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

            ImGui::Text(
                "SI=%04X DI=%04X BP=%04X SP=%04X",
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
                "DS=%04X ES=%04X SS=%04X",
                static_cast<unsigned int>(
                    instruction.registers.ds
                    ),
                static_cast<unsigned int>(
                    instruction.registers.es
                    ),
                static_cast<unsigned int>(
                    instruction.registers.ss
                    )
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
                        i == m_selectedTraceIndex,
                        ImGuiSelectableFlags_AllowDoubleClick,
                        ImVec2(
                            ImGui::CalcTextSize(
                                addressText
                            ).x,
                            0.0f
                        )
                    ))
                    {
                        m_selectedTraceIndex = i;
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
        const size_t count =
            (std::min)(
                m_traceA.size(),
                m_traceB.size()
            );

        for (size_t i = 0;
            i < count;
            ++i)
        {
            const TraceInstructionDifference
                difference =
                compareTraceInstructions(
                    m_traceA[i],
                    m_traceB[i]
                );

            if (!difference.any())
            {
                continue;
            }

            const bool previousIsDifferent =
                i > 0 &&
                compareTraceInstructions(
                    m_traceA[i - 1],
                    m_traceB[i - 1]
                ).any();

            if (i > 0 &&
                !previousIsDifferent)
            {
                ImGui::TextDisabled(
                    "Previous instruction %zu",
                    i
                );

                ImGui::PushID(
                    static_cast<int>(i - 1)
                );

                if (ImGui::BeginTable(
                    "PreviousTraceComparisonTable",
                    2,
                    ImGuiTableFlags_BordersInnerV |
                    ImGuiTableFlags_RowBg
                ))
                {
                    ImGui::TableSetupColumn(
                        "Trace A"
                    );

                    ImGui::TableSetupColumn(
                        "Trace B"
                    );

                    ImGui::TableHeadersRow();

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(
                        0
                    );

                    drawTraceComparisonInstruction(
                        m_traceA[i - 1],
                        compareTraceInstructions(
                            m_traceA[i - 1],
                            m_traceB[i - 1]
                        ),
                        false
                    );

                    ImGui::TableSetColumnIndex(
                        1
                    );

                    drawTraceComparisonInstruction(
                        m_traceB[i - 1],
                        compareTraceInstructions(
                            m_traceA[i - 1],
                            m_traceB[i - 1]
                        ),
                        true
                    );

                    ImGui::EndTable();
                }

                ImGui::PopID();
            }

            ImGui::Separator();

            ImGui::Text(
                "Difference at instruction %zu",
                i + 1
            );

            ImGui::PushID(
                static_cast<int>(i)
            );

            if (ImGui::BeginTable(
                "TraceComparisonTable",
                2,
                ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_RowBg
            ))
            {
                ImGui::TableSetupColumn(
                    "Trace A"
                );

                ImGui::TableSetupColumn(
                    "Trace B"
                );

                ImGui::TableHeadersRow();

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(
                    0
                );

                drawTraceComparisonInstruction(
                    m_traceA[i],
                    difference,
                    false
                );

                ImGui::TableSetColumnIndex(
                    1
                );

                drawTraceComparisonInstruction(
                    m_traceB[i],
                    difference,
                    true
                );

                ImGui::EndTable();

                ImGui::PopID();
            }
        }
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

    void TrackingWindow::loadTrace()
    {
        size_t count = 0;

        if (!m_scanner.getReadTraceCount(
            count
        ))
        {
            return;
        }

        m_trace.clear();

        m_trace.reserve(
            count
        );

        for (size_t i = 0;
            i < count;
            ++i)
        {
            RuntimeInstruction instruction;

            if (!m_scanner.getReadTraceInstruction(
                i,
                instruction
            ))
            {
                break;
            }

            m_trace.push_back(
                instruction
            );
        }
    }
    void TrackingWindow::saveTraceToFile(
        const std::string& filename
    ) const
    {
        std::ofstream file(
            filename
        );

        if (!file)
        {
            return;
        }

        file <<
            "ExecutionTrace 1\n";

        file <<
            m_trace.size() <<
            '\n';

        for (const RuntimeInstruction& instruction :
            m_trace)
        {
            file
                << instruction.address << ' '
                << instruction.cs << ' '
                << instruction.ip << ' '

                << instruction.registers.ax << ' '
                << instruction.registers.bx << ' '
                << instruction.registers.cx << ' '
                << instruction.registers.dx << ' '

                << instruction.registers.si << ' '
                << instruction.registers.di << ' '
                << instruction.registers.bp << ' '
                << instruction.registers.sp << ' '

                << instruction.registers.ds << ' '
                << instruction.registers.es << ' '
                << instruction.registers.ss;

            for (const uint8_t byte :
            instruction.bytes)
            {
                file <<
                    ' ' <<
                    static_cast<unsigned int>(
                        byte
                        );
            }

            file << '\n';
        }
    }

    bool TrackingWindow::loadTraceFromFile(
        const std::string& filename,
        std::vector<RuntimeInstruction>& trace
    )
    {
        std::ifstream file(
            filename
        );

        if (!file)
        {
            return false;
        }

        std::string header;

        std::getline(
            file,
            header
        );

        if (header !=
            "ExecutionTrace 1")
        {
            return false;
        }

        size_t instructionCount = 0;

        if (!(file >>
            instructionCount))
        {
            return false;
        }

        std::vector<RuntimeInstruction>
            loadedTrace;

        loadedTrace.reserve(
            instructionCount
        );

        for (size_t instructionIndex = 0;
            instructionIndex < instructionCount;
            ++instructionIndex)
        {
            RuntimeInstruction instruction;

            if (!(file
                >> instruction.address
                >> instruction.cs
                >> instruction.ip

                >> instruction.registers.ax
                >> instruction.registers.bx
                >> instruction.registers.cx
                >> instruction.registers.dx

                >> instruction.registers.si
                >> instruction.registers.di
                >> instruction.registers.bp
                >> instruction.registers.sp

                >> instruction.registers.ds
                >> instruction.registers.es
                >> instruction.registers.ss))
            {
                return false;
            }

            for (uint8_t& byte :
                instruction.bytes)
            {
                unsigned int value = 0;

                if (!(file >> value) ||
                    value > 0xff)
                {
                    return false;
                }

                byte =
                    static_cast<uint8_t>(
                        value
                        );
            }

            loadedTrace.push_back(
                instruction
            );
        }

        trace =
            std::move(
                loadedTrace
            );

        return true;
    }
    
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


