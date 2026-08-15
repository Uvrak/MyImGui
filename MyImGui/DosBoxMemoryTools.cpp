#include "pch.h"
#include "DosBoxMemoryTools.h"

namespace MyImGui
{
    DosBoxMemoryTools::
        DosBoxMemoryTools(
            const std::string& gameId
        )
        : m_scannerWindow(
            m_memoryReader,
            gameId
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
        m_scannerWindow.draw();
        m_viewerWindow.draw();
    }
}
