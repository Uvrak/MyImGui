#include "TraceTracking.h"

#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <commdlg.h>

#include "imgui.h"
#include "Zydis/Zydis.h"

namespace DosBoxMemoryTools
{
    TraceTracking::TraceTracking(
        MemoryScanner& scanner,
        const std::string& gameId
    )
        :
        m_scanner(
            scanner
        ),
        m_gameId(
            gameId
        )
    {
        loadSession();
    }

    TraceTracking::~TraceTracking()
    {
        saveSession();
    }

    void TraceTracking::draw()
    {
        drawNavigation();
        drawRecorder();
        drawTrace();

        handleLoadTraceRequest();
        handleSaveTraceRequest();
    }

    void TraceTracking::loadTrace()
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

    void TraceTracking::setGameId(
        const std::string& gameId
    )
    {
        if (m_gameId ==
            gameId)
        {
            return;
        }

        saveSession();

        m_gameId =
            gameId;

        loadSession();
    }

    void TraceTracking::drawRecorder()
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

        // Place a small Save button next to the Record button to save the current trace (left/main)
        ImGui::SameLine();
        if (ImGui::Button("save"))
        {
            m_saveTraceRequested = true;
        }
    }
    
    void TraceTracking::drawTrace()
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
                m_traceWasArmedOrActive =
                    true;
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
                        m_trace[
                            m_selectedTraceIndex
                        ].address
                    );
                }

                m_recordButton.stop();

                m_traceWasArmedOrActive =
                    false;
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

        // Keep the recorder and navigation above the scrolling trace rows.
        ImGui::BeginChild(
            "TraceRecords",
            ImVec2(0.0f, 0.0f),
            false,
            ImGuiWindowFlags_HorizontalScrollbar
        );

        for (size_t i = 0;
            i < m_trace.size();
            ++i)
        {
            const RuntimeInstruction&
                instruction =
                m_trace[i];

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

            ImGui::Text(
                "AX=%04X BX=%04X CX=%04X DX=%04X",
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
        ImGui::EndChild();
    }
    
    bool TraceTracking::loadTraceFromFile(
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
    
    void TraceTracking::saveTraceToFile(
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

    void TraceTracking::drawNavigation()
    {
        ImGui::SetNextItemWidth(
            120.0f
        );

        ImGui::InputText(
            "Target",
            m_targetText,
            sizeof(m_targetText)
        );

        ImGui::SameLine();

        if (ImGui::Button("Load"))
        {
            m_loadTraceRequested =
                true;
        }

        ImGui::SameLine();

        if (ImGui::Button("Save"))
        {
            m_saveTraceRequested =
                true;
        }

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            saveSession();
        }
    }

    void TraceTracking::handleLoadTraceRequest()
    {
        if (!m_loadTraceRequested)
        {
            return;
        }

        m_loadTraceRequested =
            false;

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
                m_selectedTraceIndex =
                    static_cast<size_t>(-1);

                m_scrollToSelectedTrace =
                    false;
            }
        }
    }

    void TraceTracking::handleSaveTraceRequest()
    {
        if (!m_saveTraceRequested)
        {
            return;
        }

        m_saveTraceRequested =
            false;

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

    void TraceTracking::saveSession() const
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

    void TraceTracking::loadSession()
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
}
