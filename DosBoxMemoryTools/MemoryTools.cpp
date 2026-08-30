
#include "MemoryTools.h"

#include "imgui.h"

namespace DosBoxMemoryTools
{
    MemoryTools::
        MemoryTools(
            const std::string& gameId,
            DosBoxX::View* dosBoxView
        )
        : m_scannerWindow(
            m_memoryReader,
            gameId,
            dosBoxView
        ),
        m_viewerWindow(
            m_memoryReader
        ),
        m_readTrackerWindow(
            m_scannerWindow.scanner(),
            m_scannerWindow,
            gameId
        ),
        m_trackingWindow(
            m_scannerWindow.scanner(),
            gameId
        ),
        m_disassemblyWindow(
            m_memoryReader
        )
    {}

    void MemoryTools::draw()
    {
        if (!m_initialRefreshDone)
        {
            if (m_scannerWindow.refreshMemory())
            {
                m_scannerWindow.refreshPinnedValues();
                m_initialRefreshDone = true;
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

        m_readTrackerWindow.draw(
            nullptr
        );

        m_trackingWindow.draw(
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

        m_disassemblyWindow.draw(
            nullptr
        );
    }

    MemoryReader&
        MemoryTools::memoryReader()
    {
        return m_memoryReader;
    }

    void MemoryTools::
        refreshPinnedValues()
    {
        m_scannerWindow.refreshPinnedValues();
    }

    void MemoryTools::saveSession()
    {
        // m_transitionTrackerWindow.saveSession();

        m_trackingWindow.saveSession();
    }

    void MemoryTools::setGameId(
        const std::string& gameId
    )
    {
        m_scannerWindow.setGameId(
            gameId
        );

        m_readTrackerWindow.setGameId(
            gameId
        );

        // m_transitionTrackerWindow.setGameId(
            //gameId
        //);

        m_trackingWindow.setGameId(
            gameId
        );
    }
}
