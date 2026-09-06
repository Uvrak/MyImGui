#include "ExecutionTracking.h"

#include "imgui.h"

#include <cstdlib>

namespace DosBoxMemoryTools
{
    ExecutionTracking::ExecutionTracking(
        MemoryScanner& scanner
    )
        :
        m_scanner(
            scanner
        )
    {

    }

    void ExecutionTracking::draw(
        const char* targetText
    )
    {
        if (m_recordButton.draw())
        {
            if (m_recordButton.recording())
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
                    m_scanner.setExecutionCaptureTarget(
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
        }

        bool executionHit = false;

        if (m_scanner.getExecutionCaptureHit(
            executionHit
        ))
        {
            if (executionHit &&
                m_recordButton.recording())
            {
                m_recordButton.stop();
            }
        }

        ImGui::Text(
            "State: %s",
            executionHit
            ? "HIT"
            : m_recordButton.recording()
            ? "ARMED / NO HIT"
            : "IDLE"
        );

        ImGui::Text(
            "Scanner status: %s",
            m_scanner.status().c_str()
        );
    }
}