#include "pch.h"
#include "DosBoxMemoryReadTrackerWindow.h"

#include "DosBoxMemoryScanner.h"

#include <unordered_set>

#include "imgui.h"

namespace MyImGui
{
    DosBoxMemoryReadTrackerWindow::
        DosBoxMemoryReadTrackerWindow(
            DosBoxMemoryScanner& scanner
        )
        : m_scanner(
            scanner
        )
    {}

    void DosBoxMemoryReadTrackerWindow::draw(
        bool* isOpen
    )
    {
        if (isOpen &&
            !*isOpen)
        {
            return;
        }

        if (!ImGui::Begin(
            "DOSBox Memory Read Tracker",
            isOpen
        ))
        {
            ImGui::End();
            return;
        }

        ImGui::TextUnformatted(
            "Memory Read Tracker"
        );

        if (ImGui::Button(
            "Start Read Tracking"
        ))
        {
            m_scanner.startReadTracking();
        }

        ImGui::SameLine();

        if (ImGui::Button(
            "Stop Read Tracking"
        ))
        {
            m_scanner.stopReadTracking();
        }

        ImGui::SameLine();

        if (ImGui::Button(
            "Read Count"
        ))
        {
            size_t count = 0;

            m_scanner.getReadTrackingCount(
                count
            );
        }

        if (ImGui::Button(
            "Capture Idle"
        ))
        {
            m_scanner.getReadTrackingAddresses(
                m_idleReadAddresses
            );
        }

        ImGui::SameLine();

        if (ImGui::Button(
            "Capture Attack"
        ))
        {
            m_scanner.getReadTrackingAddresses(
                m_attackReadAddresses
            );
        }
        if (ImGui::Button(
            "Compare"
        ))
        {
            m_attackOnlyReadAddresses.clear();

            const std::unordered_set<size_t> idleAddresses(
                m_idleReadAddresses.begin(),
                m_idleReadAddresses.end()
            );

            for (const size_t address :
            m_attackReadAddresses)
            {
                if (idleAddresses.find(address) ==
                    idleAddresses.end())
                {
                    m_attackOnlyReadAddresses.push_back(
                        address
                    );
                }
            }
            m_scanner.setCandidatesFromAddresses(
                m_attackOnlyReadAddresses
            );
        }

        ImGui::Text(
            "Idle addresses: %zu",
            m_idleReadAddresses.size()
        );

        ImGui::Text(
            "Attack addresses: %zu",
            m_attackReadAddresses.size()
        );

        ImGui::Text(
            "Attack only: %zu",
            m_attackOnlyReadAddresses.size()
        );

        ImGui::Separator();

        ImGui::TextWrapped(
            "%s",
            m_scanner.status().c_str()
        );

        ImGui::End();
    }
}