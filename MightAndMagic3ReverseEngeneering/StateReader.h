#pragma once

#include "GameState.h"
#include "../DosBoxMemoryTools/MemoryReader.h"

namespace MightAndMagic3
{
    class StateReader
    {
    public:
        StateReader(
            DosBoxMemoryTools::MemoryReader& memoryReader
        );

        bool update(
            GameState& state
        );

        int characterLevel(
            int characterIndex
        ) const;

    private:
        DosBoxMemoryTools::MemoryReader&
            m_memoryReader;
    };
}
