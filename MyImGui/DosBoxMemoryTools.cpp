#include "pch.h"
#include "DosBoxMemoryTools.h"

#include "imgui.h"

namespace MyImGui
{
    DosBoxMemoryTools::
        DosBoxMemoryTools(
            const std::string& gameId,
            DosBoxView* dosBoxView
        )
        : m_scannerWindow(
            m_memoryReader,
            gameId,
            dosBoxView
        ),
        m_viewerWindow(
            m_memoryReader
        )
    {}

    void DosBoxMemoryTools::setGameId(
        const std::string& gameId
    )
    {
        m_scannerWindow.setGameId(
            gameId
        );
    }

    void DosBoxMemoryTools::draw()
    {
        if (m_liveView)
        {
            const double currentTime =
                ImGui::GetTime();

            if (currentTime -
                m_lastLiveRefresh >= 0.2)
            {
                m_scannerWindow.refreshMemory();

                m_lastLiveRefresh =
                    currentTime;
            }
        }

        if (m_liveView)
        {
            const double currentTime =
                ImGui::GetTime();

            if (currentTime -
                m_lastLiveRefresh >= 0.2)
            {
                m_scannerWindow.refreshMemory();

                m_lastLiveRefresh =
                    currentTime;
            }
        }

        m_scannerWindow.draw(
            nullptr,
            m_liveView
        );
        m_viewerWindow.draw(
            nullptr,
            m_liveView
        );
    }
}
