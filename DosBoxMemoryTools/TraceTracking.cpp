#include "TraceTracking.h"

#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <filesystem>

#include "imgui.h"

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
        updateCapture();
        continueLoadTrace();
    }

    void TraceTracking::beginLoadTrace()
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

        m_traceLoadCount =
            count;

        m_traceLoadIndex =
            0;

        m_traceLoadPending =
            true;
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

    void TraceTracking::continueLoadTrace()
    {
        if (!m_traceLoadPending)
        {
            return;
        }

        constexpr size_t batchSize =
            64;

        size_t loadedThisFrame =
            0;

        while (m_traceLoadIndex < m_traceLoadCount &&
            loadedThisFrame < batchSize)
        {
            RuntimeInstruction instruction;

            if (!m_scanner.getReadTraceInstruction(
                m_traceLoadIndex,
                instruction
            ))
            {
                m_traceLoadPending =
                    false;

                return;
            }

            m_trace.push_back(
                instruction
            );

            ++m_traceLoadIndex;
            ++loadedThisFrame;
        }

        if (m_traceLoadIndex >=
            m_traceLoadCount)
        {
            m_traceLoadPending =
                false;

            m_traceCompleted =
                true;

            m_recordButton.stop();
        }
    }

    void TraceTracking::drawRecorder()
    {
        const bool recordChanged =
            m_recordButton.draw();

        ImGui::SameLine();

        ImGui::Text(
            "recording=%s",
            m_recordButton.recording()
            ? "true"
            : "false"
        );

        ImGui::SameLine();

        bool datasetA =
            m_targetDataset ==
            TargetDataset::A;

        if (ImGui::RadioButton(
            "A",
            datasetA
        ))
        {
            m_targetDataset =
                TargetDataset::A;
        }

        ImGui::SameLine();

        bool datasetB =
            m_targetDataset ==
            TargetDataset::B;

        if (ImGui::RadioButton(
            "B",
            datasetB
        ))
        {
            m_targetDataset =
                TargetDataset::B;
        }

        ImGui::SameLine();

        const char* traceLimitItems[] =
        {
            "1000",
            "5000",
            "10000",
            "25000",
            "50000",
            "100000"
        };

        int traceLimitIndex =
            0;

        switch (m_traceInstructionLimit)
        {
        case 5000:
            traceLimitIndex = 1;
            break;

        case 10000:
            traceLimitIndex = 2;
            break;

        case 25000:
            traceLimitIndex = 3;
            break;

        case 50000:
            traceLimitIndex = 4;
            break;

        case 100000:
            traceLimitIndex = 5;
            break;
        }

        ImGui::SetNextItemWidth(
            120.0f
        );

        if (ImGui::Combo(
            "Trace Limit",
            &traceLimitIndex,
            traceLimitItems,
            IM_ARRAYSIZE(
                traceLimitItems
            )
        ))
        {
            const size_t traceLimits[] =
            {
                1000,
                5000,
                10000,
                25000,
                50000,
                100000
            };

            m_traceInstructionLimit =
                traceLimits[
                    traceLimitIndex
                ];

            m_scanner.setReadTraceInstructionLimit(
                m_traceInstructionLimit
            );
        }
    
        if (!recordChanged)
        {
            return;
        }

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

                m_traceWasArmedOrActive =
                    false;

                m_scanner.setReadTraceInstructionLimit(
                    m_traceInstructionLimit
                );

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

    void TraceTracking::updateCapture()
    {
        if (!m_recordButton.recording())
        {
            return;
        }

        bool traceActive =
            false;

        bool traceArmed =
            false;

        if (!m_scanner.getReadTraceActive(
            traceActive
        ))
        {
            return;
        }

        if (!m_scanner.getReadTraceArmed(
            traceArmed
        ))
        {
            return;
        }

        if (traceActive ||
            traceArmed)
        {
            m_traceWasArmedOrActive =
                true;

            return;
        }

        if (!m_traceWasArmedOrActive ||
            !m_recordButton.recording())
        {
            return;
        }

        beginLoadTrace();

        m_traceWasArmedOrActive =
            false;
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

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            saveSession();
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
        }

    const std::vector<RuntimeInstruction>&
        TraceTracking::trace() const
    {
        return m_trace;
    }

    bool TraceTracking::targetDatasetA() const
    {
        return m_targetDataset ==
            TargetDataset::A;
    }

    bool TraceTracking::takeCompletedTrace()
    {
        if (!m_traceCompleted)
        {
            return false;
        }

        m_traceCompleted =
            false;

        return true;
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
    }
}
