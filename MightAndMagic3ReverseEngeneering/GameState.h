#pragma once

#include "Position.h"

namespace MightAndMagic3
{
    class GameState
    {
    public:
        const Position& position() const
        {
            return m_position;
        }

        void setPosition(
            const Position& position
        )
        {
            m_position =
                position;
        }

    private:
        Position m_position;
    };
}
