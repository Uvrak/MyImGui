#include "MemoryReadRecording.h"
#include "MemoryReadTrackerWindow.h"


#include "imgui.h"

namespace DosBoxMemoryTools
{
    void MemoryReadRecording::draw(
        MemoryReadTrackerWindow& window
    )
    {
        ImGui::TextUnformatted(
            "Idle"
        );

        ImGui::PushID(
            "IdleRecord"
        );

        if (window.m_idleRecordButton.draw())
        {
            if (window.m_idleRecordButton.recording())
            {
                window.m_previousAttackOnlyReadAddresses =
                    window.m_attackOnlyReadAddresses;

                window.m_scanner.clearReadTracking();
                window.m_scanner.startReadTracking();
            }
            else
            {
                window.m_scanner.stopReadTracking();

                window.m_scanner.getReadTrackingAddresses(
                    window.m_idleReadAddresses
                );
            }
        }

        ImGui::PopID();

        ImGui::TextUnformatted(
            "Attack"
        );

        ImGui::PushID(
            "AttackRecord"
        );

        if (window.m_attackRecordButton.draw())
        {
            if (window.m_attackRecordButton.recording())
            {
                window.m_scanner.clearReadTracking();
                window.m_scanner.startReadTracking();
            }
            else
            {
                window.m_scanner.stopReadTracking();

                window.m_scanner.getReadTrackingAddresses(
                    window.m_attackReadAddresses
                );

                window.m_scanner.getReadTrackingInstructions(
                    window.m_attackReadInstructions
                );
            }
        }

		ImGui::PopID();
    }
}
