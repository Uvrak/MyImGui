#pragma once

#include <string>

namespace DosBoxX
{
    class NamedPipeClient;
}

enum class MightAndMagic1Direction
{
    North,
    East,
    South,
    West,
    Unknown
};

struct MightAndMagic1State
{
    int x = 0;
    int y = 0;

    int areaValueA = 0;
    int areaValueB = 0;

    MightAndMagic1Direction direction =
        MightAndMagic1Direction::Unknown;

    bool valid = false;
};

class MightAndMagic1Reader
{
public:
    bool update(
        DosBoxX::NamedPipeClient& pipeClient
    );

    const MightAndMagic1State&
        state() const;

private:
    bool parseResponse(
        const std::string& response
    );

    MightAndMagic1State m_state;
};
