#include "StateReader.h"

namespace MightAndMagic3
{
    StateReader::StateReader(
        DosBoxMemoryTools::MemoryReader& memoryReader
    )
        :
        m_memoryReader(
            memoryReader
        )
    {}

    bool StateReader::update(
        GameState& state
    )
    {
        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        if (memory.empty())
        {
            return false;
        }

        Position position;

        // X, Y und Direction kommen hier hinein.

        state.setPosition(
            position
        );

        return true;
    }
}