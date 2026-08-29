#include "MightAndMagic1Reader.h"

#include "NamedPipeClient.h"

#include <cstdio>

bool MightAndMagic1Reader::update(
    DosBoxX::NamedPipeClient& pipeClient
)
{
    std::string response;

    if (!pipeClient.request(
        "MM1_STATE",
        response
    ))
    {
        m_state.valid = false;
        return false;
    }

    return parseResponse(
        response
    );
}

const MightAndMagic1State&
MightAndMagic1Reader::state() const
{
    return m_state;
}

bool MightAndMagic1Reader::parseResponse(
    const std::string& response
)
{
    int x = 0;
    int y = 0;
    int areaValueA = 0;
    int areaValueB = 0;
    int valid = 0;

    char direction[16]{};

    if (sscanf_s(
        response.c_str(),
        "MM1_STATE:%d:%d:%d:%d:%15[^:]:%d",
        &x,
        &y,
        &areaValueA,
        &areaValueB,
        direction,
        static_cast<unsigned int>(
            sizeof(direction)
            ),
        &valid
    ) != 6)
    {
        m_state.valid = false;
        return false;
    }

    MightAndMagic1Direction parsedDirection =
        MightAndMagic1Direction::Unknown;

    const std::string directionName =
        direction;

    if (directionName == "NORTH")
    {
        parsedDirection =
            MightAndMagic1Direction::North;
    }
    else if (directionName == "EAST")
    {
        parsedDirection =
            MightAndMagic1Direction::East;
    }
    else if (directionName == "SOUTH")
    {
        parsedDirection =
            MightAndMagic1Direction::South;
    }
    else if (directionName == "WEST")
    {
        parsedDirection =
            MightAndMagic1Direction::West;
    }

    m_state.x = x;
    m_state.y = y;
    m_state.areaValueA = areaValueA;
    m_state.areaValueB = areaValueB;
    m_state.direction =
        parsedDirection;

    m_state.valid =
        valid != 0 &&
        parsedDirection !=
        MightAndMagic1Direction::Unknown;

    return true;
}