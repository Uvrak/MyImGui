#include "mightandmagic1.h"

#include "mem.h"

MightAndMagic1State
MightAndMagic1::readState()
{
    const uint8_t axisA =
        phys_readb(
            static_cast<PhysPt>(
                PositionAxisAAddress
                )
        );

    const uint8_t axisB =
        phys_readb(
            static_cast<PhysPt>(
                PositionAxisBAddress
                )
        );

    const uint8_t directionValue =
        phys_readb(
            static_cast<PhysPt>(
                DirectionAddress
                )
        );

    const uint8_t areaValueA =
        phys_readb(
            static_cast<PhysPt>(
                AreaValueAAddress
                )
        );

    const uint8_t areaValueB =
        phys_readb(
            static_cast<PhysPt>(
                AreaValueBAddress
                )
        );
    MightAndMagic1State state;

    state.x =
        static_cast<int>(
            axisA
            );

    state.y =
        -static_cast<int>(
            axisB
            );

    state.areaValueA =
        static_cast<int>(areaValueA);

    state.areaValueB =
        static_cast<int>(areaValueB);

    state.direction =
        decodeDirection(
            directionValue
        );

    state.valid =
        state.direction !=
        MightAndMagic1Direction::Unknown;

    return state;
}

MightAndMagic1Direction
MightAndMagic1::decodeDirection(
    uint8_t value
)
{
    switch(value)
    {
    case 192:
        return MightAndMagic1Direction::North;

    case 48:
        return MightAndMagic1Direction::East;

    case 12:
        return MightAndMagic1Direction::South;

    case 3:
        return MightAndMagic1Direction::West;

    default:
        return MightAndMagic1Direction::Unknown;
    }
}
