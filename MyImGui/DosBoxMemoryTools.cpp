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
        m_readTrackerWindow(
            m_scannerWindow.scanner()
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

        m_scannerWindow.draw(
            nullptr,
            m_liveView
        );

        m_readTrackerWindow.draw(
            nullptr
        );

        size_t selectedAddress = 0;

        if (m_scannerWindow.takeSelectedAddress(
            selectedAddress
        ))
        {
            m_viewerWindow.goToAddress(
                selectedAddress
            );
        }

        m_viewerWindow.draw(
            nullptr,
            m_liveView
        );
    }

    DosBoxMemoryReader&
        DosBoxMemoryTools::memoryReader()
    {
        return m_memoryReader;
    }
}
