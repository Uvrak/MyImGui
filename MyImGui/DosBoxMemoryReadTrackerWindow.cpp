#include "pch.h"
#include "DosBoxMemoryReadTrackerWindow.h"

#include "DosBoxMemoryScanner.h"

#include <unordered_set>
#include <fstream>
#include <filesystem>

#include "imgui.h"
#include "DosBoxMemoryTools.h"
#include <windows.h>

namespace MyImGui
{
    DosBoxMemoryReadTrackerWindow::
        DosBoxMemoryReadTrackerWindow(
            DosBoxMemoryScanner& scanner,
            const std::string& gameId
        )
        : m_scanner(
            scanner
        ),
        m_gameId(
            gameId
        )
    {
        loadSession();
    }

    bool DosBoxMemoryTools::refreshMemory()
    {
        return m_scannerWindow.refreshMemory();
    }

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
    
    void DosBoxMemoryReadTrackerWindow::saveSession() const
    {

        if (m_idleReadAddresses.empty() &&
            m_attackReadAddresses.empty() &&
            m_attackOnlyReadAddresses.empty() &&
            m_scanner.candidates().empty())
        {
            return;
        }

        if (m_gameId.empty())
        {
            OutputDebugStringA(
                "saveSession: gameId is empty\n"
            );

            return;
        }
        
        OutputDebugStringA(
            "saveSession called\n"
        );

        std::filesystem::create_directories(
            "settings"
        );

        const std::string filename =
            "settings/memory_read_session_" +
            m_gameId +
            ".cfg";

        OutputDebugStringA(
            filename.c_str()
        );

        OutputDebugStringA(
            "\n"
        );

        std::ofstream file(
            filename
        );

        if (!file)
        {
            return;
        }

        file <<
            "GridBuilderMemoryReadSession 1\n";

        file <<
            "Idle\n";

        file <<
            m_idleReadAddresses.size() <<
            '\n';

        for (const size_t address :
        m_idleReadAddresses)
        {
            file <<
                address <<
                '\n';
        }

        file <<
            "Attack\n";

        file <<
            m_attackReadAddresses.size() <<
            '\n';

        for (const size_t address :
        m_attackReadAddresses)
        {
            file <<
                address <<
                '\n';
        }

        file <<
            "AttackOnly\n";

        file <<
            m_attackOnlyReadAddresses.size() <<
            '\n';

        for (const size_t address :
        m_attackOnlyReadAddresses)
        {
            file <<
                address <<
                '\n';
        }

        const auto& candidates =
            m_scanner.candidates();

        file <<
            "Candidates\n";

        file <<
            candidates.size() <<
            '\n';

        for (const DosBoxMemoryCandidate& candidate :
            candidates)
        {
            file <<
                candidate.address <<
                '\n';
        }


    }
    
    void DosBoxMemoryTools::saveSession()
    {
        m_readTrackerWindow.saveSession();
    }

    void DosBoxMemoryReadTrackerWindow::loadSession()
    {
        if (m_gameId.empty())
        {
            return;
        }

        const std::string filename =
            "settings/memory_read_session_" +
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
            "GridBuilderMemoryReadSession 1")
        {
            return;
        }

        auto readAddresses =
            [&file](
                const char* expectedSection,
                std::vector<size_t>& addresses
                ) -> bool
            {
                std::string section;

                if (!(file >> section) ||
                    section != expectedSection)
                {
                    return false;
                }

                size_t count = 0;

                if (!(file >> count))
                {
                    return false;
                }

                addresses.clear();
                addresses.reserve(
                    count
                );

                for (size_t i = 0;
                    i < count;
                    ++i)
                {
                    size_t address = 0;

                    if (!(file >> address))
                    {
                        addresses.clear();
                        return false;
                    }

                    addresses.push_back(
                        address
                    );
                }

                return true;
            };

        std::vector<size_t> candidates;

        if (!readAddresses(
            "Idle",
            m_idleReadAddresses
        ))
        {
            return;
        }

        if (!readAddresses(
            "Attack",
            m_attackReadAddresses
        ))
        {
            return;
        }

        if (!readAddresses(
            "AttackOnly",
            m_attackOnlyReadAddresses
        ))
        {
            return;
        }

        if (!readAddresses(
            "Candidates",
            candidates
        ))
        {
            return;
        }

        if (!candidates.empty())
        {
            m_scanner.setCandidatesFromAddresses(
                candidates
            );
        }
    }

    void DosBoxMemoryReadTrackerWindow::setGameId(
        const std::string& gameId
    )
    {
        m_gameId =
            gameId;

        loadSession();
    }

    void DosBoxMemoryTools::setGameId(
        const std::string& gameId
    )
    {
        m_scannerWindow.setGameId(
            gameId
        );

        m_readTrackerWindow.setGameId(
            gameId
        );
    }

}