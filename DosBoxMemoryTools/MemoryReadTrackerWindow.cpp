#include "MemoryReadTrackerWindow.h"

#include "MemoryReadRecording.h"
#include "MemoryReadComparison.h"
#include "MemoryReadInstructionView.h"
#include "MemoryReadPersistence.h"

#include "imgui.h"

namespace DosBoxMemoryTools
{
    MemoryReadTrackerWindow::
        MemoryReadTrackerWindow(
            MemoryScanner& scanner,
            MemoryScannerWindow& scannerWindow,
            const std::string& gameId
        )
        : m_scanner(
            scanner
        ),
        m_scannerWindow(
            scannerWindow
        ),
        m_gameId(
            gameId
        )
    {
        loadSession();
    }

    void MemoryReadTrackerWindow::draw(
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

        MemoryReadRecording::draw(
            *this
        );

        MemoryReadComparison::draw(
            *this
        );

        MemoryReadInstructionView::draw(
            *this
        );

        ImGui::Separator();

        ImGui::TextWrapped(
            "%s",
            m_scanner.status().c_str()
        );

        ImGui::End();
    }

    void MemoryReadTrackerWindow::saveSession() const
    {
        MemoryReadPersistence::save(
            *this
        );
    }

    void MemoryReadTrackerWindow::loadSession()
    {
        MemoryReadPersistence::load(
            *this
        );
    }

    void MemoryReadTrackerWindow::setGameId(
        const std::string& gameId
    )
    {
        m_gameId =
            gameId;

        loadSession();
    }
    
}