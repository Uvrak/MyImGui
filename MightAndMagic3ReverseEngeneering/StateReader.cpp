#include "StateReader.h"

namespace MightAndMagic3
{
    namespace
    {
        constexpr size_t
            FacingAddress =
            0x2EE30;

        constexpr size_t
            XAddress =
            0x2EE31;

        constexpr size_t
            YAddress =
            0x2EE32;

        Direction decodeDirection(
            uint8_t value
        )
        {
            switch (value)
            {
            case 0:
                return Direction::North;

            case 1:
                return Direction::South;

            case 2:
                return Direction::East;

            case 3:
                return Direction::West;

            default:
                return Direction::Unknown;
            }
        }
    }

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

        if (memory.size() <=
            YAddress)
        {
            return false;
        }

        Position position;

        position.x =
            static_cast<int>(
                memory[XAddress]
                );

        position.y =
            static_cast<int>(
                memory[YAddress]
                );

        position.direction =
            decodeDirection(
                memory[FacingAddress]
            );

        position.valid =
            position.direction !=
            Direction::Unknown;

        state.setPosition(
            position
        );

        return true;
    }
}