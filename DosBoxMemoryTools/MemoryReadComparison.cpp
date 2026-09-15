#include "MemoryReadComparison.h"
#include "MemoryReadTrackerWindow.h"

#include "MemoryScannerWindow.h"

#include <algorithm>
#include <unordered_set>

#include "imgui.h"

namespace DosBoxMemoryTools
{
    void MemoryReadComparison::draw(
        MemoryReadTrackerWindow& window
    )
    {
        if (ImGui::Button(
            "Compare"
        ))
        {
            window.m_previousAttackOnlyReadAddresses =
                window.m_attackOnlyReadAddresses;

            window.m_attackOnlyReadAddresses.clear();

            const std::unordered_set<size_t> idleAddresses(
                window.m_idleReadAddresses.begin(),
                window.m_idleReadAddresses.end()
            );

            for (const size_t address :
            window.m_attackReadAddresses)
            {
                if (idleAddresses.find(address) ==
                    idleAddresses.end())
                {
                    window.m_attackOnlyReadAddresses.push_back(
                        address
                    );
                }
            }
            window.m_scanner.setCandidatesFromAddresses(
                window.m_attackOnlyReadAddresses
            );
        }

        if (ImGui::Button(
            "Intersect Attack Only"
        ))
        {
            std::unordered_set<size_t> currentAddresses(
                window.m_attackOnlyReadAddresses.begin(),
                window.m_attackOnlyReadAddresses.end()
            );

            std::vector<size_t> intersection;

            for (size_t address :
            window.m_previousAttackOnlyReadAddresses)
            {
                if (currentAddresses.contains(
                    address
                ))
                {
                    intersection.push_back(
                        address
                    );
                }
            }

            window.m_attackOnlyReadAddresses =
                std::move(
                    intersection
                );

            window.m_scanner.setCandidatesFromAddresses(
                window.m_attackOnlyReadAddresses
            );
        }

        ImGui::Text(
            "Idle addresses: %zu",
            window.m_idleReadAddresses.size()
        );

        ImGui::Text(
            "Attack addresses: %zu",
            window.m_attackReadAddresses.size()
        );

        ImGui::Text(
            "Attack only: %zu",
            window.m_attackOnlyReadAddresses.size()
        );



        ImGui::SameLine();

        if (ImGui::Button("Previous - Current"))
        {
            std::vector<size_t> difference;

            for (const size_t address :
            window.m_previousAttackOnlyReadAddresses)
            {
                if (std::find(
                    window.m_attackOnlyReadAddresses.begin(),
                    window.m_attackOnlyReadAddresses.end(),
                    address
                ) == window.m_attackOnlyReadAddresses.end())
                {
                    difference.push_back(address);
                }
            }

            window.m_attackOnlyReadAddresses =
                std::move(difference);
        }

        if (ImGui::Button(
            "Pin Attack Only"
        ))
        {
            window.m_scannerWindow.scanner().
                setCandidatesFromAddresses(
                    window.m_attackOnlyReadAddresses
                );

            window.m_scannerWindow.pinAddresses(
                window.m_attackOnlyReadAddresses
            );
        }

        if (ImGui::Button(
            "Pin Previous Attack Only"
        ))
        {
            window.m_scannerWindow.pinAddresses(
                window.m_previousAttackOnlyReadAddresses
            );
        }

        ImGui::Text(
            "Previous attack only: %zu",
            window.m_previousAttackOnlyReadAddresses.size()
        );
    }
}
