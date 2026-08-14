#include "pch.h"
#include "DosBoxMemoryTools.h"

namespace MyImGui
{
    DosBoxMemoryTools::
        DosBoxMemoryTools()
        : m_scannerWindow(
            m_memoryReader
        ),
        m_viewerWindow(
            m_memoryReader
        )
    {}

    void DosBoxMemoryTools::draw()
    {
        m_scannerWindow.draw();
        m_viewerWindow.draw();
    }
}
