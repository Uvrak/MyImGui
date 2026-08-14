#pragma once

#include "DosBoxMemoryReader.h"

namespace MyImGui
{
    class DosBoxMemoryViewerWindow
    {
    public:
        DosBoxMemoryViewerWindow(
            DosBoxMemoryReader& memoryReader
        );

        void draw(
            bool* isOpen = nullptr
        );

    private:
        DosBoxMemoryReader&
            m_memoryReader;
    };
}
